#include <D2RLPlugin/api.h>
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include "controller_input.h"
#include "portal_priority.h"
#include "portal_policy.h"
#include "portal_signatures.h"
#include "portal_calls.h"
#include "interaction_priority_signatures.h"

namespace QolPortal {
namespace {
// Stack closure constructed by GetSelectedTargetForSkill at 18DEEE..18DF20.
struct Selection {
    float* bestScore;
    const int* skill;
    void** player;
    void** selected;
};
static_assert(offsetof(Selection, player) == 0x10);
static_assert(offsetof(Selection, selected) == 0x18);
using CompareFn = void(__fastcall*)(Selection*, void*, float, int);
CompareFn original = nullptr;
using RangeFn = bool(__fastcall*)(void*, void*, int);
RangeFn originalRange = nullptr;
thread_local RangeOverride rangeOverride;
const D2RL::PluginContext* context = nullptr;
std::atomic<bool> active{false};
bool trace = false;
bool directLoot = false;
bool portals = true;
bool stash = true;
bool waypoints = true;
bool shrines = true;
bool chests = false;
char modifier[16] = "bumper";
uint32_t range = 10;
using UnitScoreFn = float(__fastcall*)(void*, void*, void*, int);
UnitScoreFn originalUnitScore = nullptr;
using ContactFn = int(__fastcall*)(void*, void*);
ContactFn originalContact = nullptr;
using InteractionFn = void*(__fastcall*)(void*, unsigned);
InteractionFn originalInteraction = nullptr;
thread_local bool recoveringInteraction = false;

bool InstallContactCalls(const D2RL::PluginContext* ctx) noexcept;

bool ExecutableAddress(uintptr_t address) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const DWORD protection = info.Protect & 0xff;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

bool AdmitClassGetter(const D2RL::PluginContext* ctx, bool& sharedDetour) noexcept {
    sharedDetour = false;
    if (!ctx || !ctx->exeBase) return false;
    unsigned char prefix[5]{};
    SIZE_T read{};
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(ctx->exeBase + 0x349860),
            prefix, sizeof(prefix), &read) || read != sizeof(prefix)) return false;
    bool executable = false;
    if (prefix[0] == 0xe9) {
        int32_t displacement{};
        std::memcpy(&displacement, prefix + 1, sizeof(displacement));
        const uintptr_t target = static_cast<uintptr_t>(
            static_cast<int64_t>(ctx->exeBase + 0x349865) + displacement);
        executable = ExecutableAddress(target);
        sharedDetour = executable;
    }
    return AdmitSharedEntryPrefix(prefix, PortalNative::Class, executable);
}

PriorityKind ReadPriorityObjectDistance(void* player, void* candidate, int& measured) noexcept {
    __try {
        if (!context || !player || !candidate || *static_cast<uint32_t*>(candidate) != 2) return PriorityKind::None;
        const auto base = context->exeBase;
        auto getClass = reinterpret_cast<uint32_t(__fastcall*)(void*, const char*, uint32_t)>(base + 0x349860);
        auto getVersion = reinterpret_cast<uint8_t(__fastcall*)(void*)>(base + 0x34A0E0);
        auto getObject = reinterpret_cast<const uint8_t*(__fastcall*)(uint8_t, uint32_t)>(base + 0x38FD00);
        auto distance = reinterpret_cast<int(__fastcall*)(void*, void*)>(base + 0x325140);
        const auto classId = getClass(candidate, "Controller QOL Updates", 0);
        const auto record = getObject(getVersion(candidate), classId);
        if (!record) return PriorityKind::None;
        const auto kind = ClassifyPriorityObject(classId, record[0x127]);
        if (!KindEnabled(kind, portals, stash, waypoints, shrines, chests)) return PriorityKind::None;
        measured = distance(player, candidate);
        return measured >= 0 ? kind : PriorityKind::None;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return PriorityKind::None; }
}

bool ModifiedLoot() noexcept {
    return directLoot && ControllerQoL::IsGroundPickupActive(modifier);
}

void* __fastcall Interaction(void* controller, unsigned index) {
    // Preserve the complete native arbitration, including other owners of its
    // callees. Recovery never promotes loot or fabricates an operation/packet.
    void* const nativeResult = originalInteraction(controller, index);
    void* player = nullptr;
    int distance = -1;
    PriorityKind kind = PriorityKind::None;
    const bool permitted = active.load() && context && controller && index < 8 && !recoveringInteraction;
    void* const result = RecoverInteraction(permitted, ControllerQoL::IsControllerUiActive(), ModifiedLoot(), nativeResult,
        [&]() -> void* {
            struct Scope {
                bool previous = recoveringInteraction;
                Scope() { recoveringInteraction = true; }
                ~Scope() { recoveringInteraction = previous; }
            } scope;
            // Same player-index helper and Selected(Interact) call used by the
            // original. Call current entries so their hook chains remain intact.
            player = reinterpret_cast<void*(__fastcall*)(unsigned)>(context->exeBase + InteractionNative::LocalPlayerRva)(index);
            if (!player) return nullptr;
            return reinterpret_cast<void*(__fastcall*)(void*, int)>(context->exeBase + InteractionNative::SelectedRva)(controller, InteractSkill);
        }, [&](void* candidate) {
            kind = ReadPriorityObjectDistance(player, candidate, distance);
            return kind != PriorityKind::None && WithinPriorityDistance(distance, range);
        });
    if (trace && context && result != nativeResult) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            char message[192];
            std::snprintf(message, sizeof(message), "[QOL/Priority] Neutral interaction recovered after combat arbitration: kind=%u distance=%d configured=%u.",
                static_cast<unsigned>(kind), distance, range);
            context->LogInfo(message);
        }
    }
    return result;
}

__declspec(noinline) int __fastcall CandidateContact(void* player, void* candidate) {
    // A JMP-only relay preserves the native CALL return address. Call the shared
    // entry (including other owners), never a cached trampoline past their hooks.
    const auto returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const int nativeContact = originalContact(player, candidate);
    if (!active.load() || !context) return nativeContact;
    const uintptr_t caller = returnAddress - context->exeBase;
    if (!IsCandidateContactCaller(caller)) return nativeContact;
    const bool modified = ModifiedLoot();
    if (modified) return nativeContact;
    int measured = -1;
    const auto kind = ReadPriorityObjectDistance(player, candidate, measured);
    const bool priority = kind != PriorityKind::None;
    const bool extended = ExtendCandidateContact(true, modified, caller, priority, measured, range);
    if (priority && trace) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            char message[224];
            std::snprintf(message, sizeof(message),
                "[QOL/Priority] Candidate contact: kind=%u caller=0x%llX distance=%d configured=%u native=%d extended=%d.",
                static_cast<unsigned>(kind), static_cast<unsigned long long>(caller), measured, range, nativeContact, extended);
            context->LogInfo(message);
        }
    }
    return nativeContact ? nativeContact : (extended ? 1 : 0);
}

bool InstallContactCalls(const D2RL::PluginContext* ctx) noexcept {
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&CandidateContact), &pinned)) return false;
    SYSTEM_INFO info{}; GetSystemInfo(&info);
    const uintptr_t step = info.dwAllocationGranularity;
    const uintptr_t aligned = (ctx->exeBase + QolPortalCalls::Calls[0].rva) & ~(step-1);
    unsigned char* relay = nullptr;
    for (uintptr_t offset=step; offset<0x40000000 && !relay; offset+=step)
        relay = static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(aligned+offset),
            64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!relay) return false;
    // Indirect JMP preserves arguments, registers, stack and return address.
    relay[0]=0xff; relay[1]=0x25; std::memset(relay+2,0,4);
    const auto destination=&CandidateContact; std::memcpy(relay+6,&destination,8);
    DWORD old{};
    if (!VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old) ||
        !FlushInstructionCache(GetCurrentProcess(),relay,64)) {
        VirtualFree(relay,0,MEM_RELEASE); return false;
    }
    originalContact = reinterpret_cast<ContactFn>(ctx->exeBase + QolPortalCalls::Target);
    // Keep the relay and pinned wrapper for process lifetime after publication
    // is attempted, even on partial/ambiguous failure. active remains false.
    return QolPortalCalls::Publish(ctx->exeBase,reinterpret_cast<uintptr_t>(relay),
        [ctx](uintptr_t rva,const unsigned char* expected,const unsigned char* replacement) {
            return ctx->PatchBytes(rva,expected,5,replacement,5);
        });
}

bool PreparePortalScore(void* controller, void* player, void* candidate, int profile,
    unsigned char* view, float* position, float& oldLimit, float& newLimit) noexcept {
    __try {
        if (!controller || !ScoringProfileOffset(profile)) return false;
        const auto path = *reinterpret_cast<const unsigned char* const*>(static_cast<unsigned char*>(candidate) + 0x38);
        if (!path) return false;
        position[0] = static_cast<float>(*reinterpret_cast<const uint32_t*>(path + 0x10));
        position[1] = static_cast<float>(*reinterpret_cast<const uint32_t*>(path + 0x14));
        // Same native position/facing helpers used by the original point scorer.
        auto getAim = reinterpret_cast<void*(__fastcall*)()>(context->exeBase + 0x144640);
        auto readAim = reinterpret_cast<void(__fastcall*)(void*, void*, float*, float*)>(context->exeBase + 0x1446C0);
        float playerPosition[2]{}, facing[2]{};
        readAim(getAim(), player, playerPosition, facing);
        const float dx = position[0] - playerPosition[0], dy = position[1] - playerPosition[1];
        return PrepareScoringView(view, static_cast<const unsigned char*>(controller), profile,
            std::sqrt(dx*dx + dy*dy), oldLimit, newLimit);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

float __fastcall ScoreUnit(void* controller, void* player, void* candidate, int profile) {
    const float nativeScore = originalUnitScore(controller, player, candidate, profile);
    if (!active.load() || ModifiedLoot()) return nativeScore;
    int measured = -1;
    const auto kind = ReadPriorityObjectDistance(player, candidate, measured);
    if (kind == PriorityKind::None) return nativeScore;
    float result = nativeScore, oldLimit = 0, newLimit = 0;
    bool retried = false;
    if (WithinPriorityDistance(measured, range) && std::isfinite(nativeScore) && nativeScore < 0) {
        alignas(16) unsigned char view[ScoringControllerBytes]{};
        float position[2]{};
        if (PreparePortalScore(controller, player, candidate, profile, view, position, oldLimit, newLimit)) {
            // Pure native point scoring: retain all angle curves/weights. Only
            // retry an actual distance failure; never replace a valid native score.
            auto scorePoint = reinterpret_cast<float(__fastcall*)(void*, void*, const float*, int, int)>(context->exeBase + 0x18AF30);
            const float expanded = scorePoint(view, player, position, 2, profile);
            if (std::isfinite(expanded) && expanded >= 0) result = expanded;
            retried = true;
        }
    }
    if (trace) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 2000) {
            lastLog = now;
            char message[256];
            std::snprintf(message, sizeof(message),
                "[QOL/Priority] Candidate score: kind=%u distance=%d configured=%u profile=%d native=%.3f result=%.3f retry=%d centerLimit=%.2f->%.2f.",
                static_cast<unsigned>(kind), measured, range, profile, nativeScore, result, retried, oldLimit, newLimit);
            context->LogInfo(message);
        }
    }
    return result;
}

// Only reads current game-thread arguments. No retained unit pointers, world
// scan, polling task, direct portal operation, or network packet construction.
bool Qualify(Selection* state, void* candidate, bool& prefer, RangeOverride& window) noexcept {
    __try {
        if (!active.load() || !context || !state || !candidate || !state->skill ||
            *state->skill != InteractSkill || !state->selected ||
            !state->player || !*state->player || !state->bestScore) return false;
        const auto currentType = *state->selected ? *static_cast<const uint32_t*>(*state->selected) : UINT32_MAX;
        const auto candidateType = *static_cast<const uint32_t*>(candidate);
        if (candidateType != 2) return false;
        if (ModifiedLoot()) return false;
        int measured = -1;
        if (ReadPriorityObjectDistance(*state->player, candidate, measured) == PriorityKind::None) return false;
        if (!WithinPriorityDistance(measured, range)) return false;
        prefer = currentType == 4;
        window = {*state->player, candidate, measured, range};
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool __fastcall InteractionRange(void* player, void* candidate, int allowance) {
    // 18AED0 is strictly a distance predicate, not an entry/visibility validator.
    // Always preserve its native execution. Only this comparison's already
    // qualified portal may replace a false result; every other caller passes through.
    const bool nativeAllowed = originalRange(player, candidate, allowance);
    const bool expanded = rangeOverride.Allows(active.load(), player, candidate);
    if (!nativeAllowed && expanded && trace && context) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            char message[192];
            std::snprintf(message, sizeof(message),
                "[QOL/Portal] Interact distance gate extended: distance=%d configured=%u nativeAllowance=%d.",
                rangeOverride.distance, rangeOverride.radius, allowance);
            context->LogInfo(message);
        }
    }
    return nativeAllowed || expanded;
}

// Scope exclusion to the same proven Interact comparison, not UI A presses.
bool CaptureProtectedSelection(Selection* state,void* candidate,void*& selected,float& best) noexcept {
    __try {
        if(!active.load() || !state || !state->skill || !state->selected || !state->bestScore ||
            !state->player || !*state->player || !candidate ||
            *state->skill!=InteractSkill) return false;
        const bool modified = ModifiedLoot();
        if (modified) {
            int distance=-1;
            if (ReadPriorityObjectDistance(*state->player,candidate,distance)==PriorityKind::None) return false;
        } else {
            if (*static_cast<const uint32_t*>(candidate) != 4 || !*state->selected) return false;
            int distance=-1;
            if (ReadPriorityObjectDistance(*state->player,*state->selected,distance)==PriorityKind::None ||
                !WithinPriorityDistance(distance,range)) return false;
        }
        selected=*state->selected;best=*state->bestScore;return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void RestoreLootPortalSelection(Selection* state,void* candidate,void* selected,float best) noexcept {
    __try {
        if(*state->selected==candidate) {*state->selected=selected;*state->bestScore=best;}
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void __fastcall Compare(Selection* state, void* candidate, float score, int targetType) {
    // Call the original exactly once. Exceptions in native execution must not
    // cause a duplicate call; fault containment is only around our reads.
    void* previousSelection=nullptr;
    float previousScore=0;
    if(CaptureProtectedSelection(state,candidate,previousSelection,previousScore)) {
        RangeScope scope(rangeOverride,{});
        original(state,candidate,score,targetType);
        RestoreLootPortalSelection(state,candidate,previousSelection,previousScore);
        return;
    }
    bool prefer = false;
    RangeOverride window;
    const bool qualified = Qualify(state, candidate, prefer, window);
    // Empty scope masks an outer candidate during a nested ordinary comparison.
    RangeScope scope(rangeOverride, window);
    if (!qualified) {
        original(state, candidate, score, targetType);
        return;
    }
    const bool changed = EvaluatePortal(prefer, candidate, score, *state->bestScore,
        *state->selected, [&](float comparison) { original(state, candidate, comparison, targetType); });
    if (trace && context) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            char message[192];
            std::snprintf(message, sizeof(message), "[QOL/Priority] Interact comparison: distance=%d configured=%u accepted=%d boosted=%d.",
                window.distance, range, *state->selected == candidate, changed);
            context->LogInfo(message);
        }
    }
}
}

void Initialize(const D2RL::PluginContext* ctx, bool enabled, bool directLootEnabled,
    bool prioritizePortals, bool prioritizeStash, bool prioritizeWaypoints,
    bool prioritizeShrines, bool prioritizeChests, bool debug,
    const char* groundModifier, uint32_t radius) noexcept {
    if (!ctx || !enabled) return;
    bool sharedClassDetour = false;
    if (!AdmitClassGetter(ctx, sharedClassDetour)) {
        ctx->LogWarn("[QOL/Priority] Class getter entry is neither original nor a bounded executable E9 detour; priority disabled.");
        return;
    }
    for (const auto& site : PortalNative::Sites) {
        if (!ctx->CheckExpectedBytes(site.rva, site.bytes, site.size)) {
            ctx->LogWarn("[QOL/Portal] Native profile mismatch; portal priority disabled and normal targeting preserved.");
            return;
        }
    }
    context = ctx;
    trace = debug;
    directLoot = directLootEnabled;
    portals = prioritizePortals;
    stash = prioritizeStash;
    waypoints = prioritizeWaypoints;
    shrines = prioritizeShrines;
    chests = prioritizeChests;
    if (sharedClassDetour)
        ctx->LogInfo("[QOL/Priority] Shared class getter E9 detour admitted; current entry retained and called, reviewed tail unchanged.");
    range = radius;
    if (groundModifier) {
        std::strncpy(modifier, groundModifier, sizeof(modifier)-1);
        modifier[sizeof(modifier)-1] = '\0';
    }
    if (!InstallContactCalls(ctx) ||
        !ctx->InstallInlineHook(0x18B350, PortalNative::UnitScore, 16,
        reinterpret_cast<void*>(&ScoreUnit), reinterpret_cast<void**>(&originalUnitScore)) || !originalUnitScore ||
        !ctx->InstallInlineHook(0x18AED0, PortalNative::InteractionRange, 16,
        reinterpret_cast<void*>(&InteractionRange), reinterpret_cast<void**>(&originalRange)) || !originalRange ||
        !ctx->InstallInlineHook(0x18A650, PortalNative::Compare, 16,
        reinterpret_cast<void*>(&Compare), reinterpret_cast<void**>(&original)) || !original) {
        ctx->LogWarn("[QOL/Portal] Native comparison/range hooks unavailable; normal targeting preserved.");
        return;
    }
    active.store(true);
    // This extension has separate admission: a conflict leaves the established
    // object-versus-loot priority running and preserves native combat arbitration.
    bool interactionCompatible = true;
    for (const auto& site : InteractionNative::Sites)
        if (!ctx->CheckExpectedBytes(site.rva, site.bytes, site.size)) interactionCompatible = false;
    if (interactionCompatible && ctx->InstallInlineHook(InteractionNative::InteractionRva,
            InteractionNative::Interaction, 16, reinterpret_cast<void*>(&Interaction),
            reinterpret_cast<void**>(&originalInteraction)) && originalInteraction)
        ctx->LogInfo("[QOL/Priority] Neutral interaction combat arbitration recovery installed.");
    else
        ctx->LogWarn("[QOL/Priority] Combat arbitration recovery unavailable; existing object/loot priority retained.");
    char message[160];
    std::snprintf(message, sizeof(message), "[QOL/Priority] portal=%d stash=%d waypoint=%d shrine=%d chest=%d; contact/scoring/range enabled within %u native units; diagnostics=%d.", portals, stash, waypoints, shrines, chests, range, trace);
    ctx->LogInfo(message);
}
bool OwnsContactDestination(std::uintptr_t destination) noexcept {
    return originalContact && destination==reinterpret_cast<std::uintptr_t>(&CandidateContact);
}
void Shutdown() noexcept { active.store(false); }
}
