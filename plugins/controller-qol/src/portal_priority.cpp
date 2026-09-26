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
char modifier[16] = "bumper";
uint32_t range = 10;
using UnitScoreFn = float(__fastcall*)(void*, void*, void*, int);
UnitScoreFn originalUnitScore = nullptr;
using ContactFn = int(__fastcall*)(void*, void*);
ContactFn originalContact = nullptr;

bool InstallContactCalls(const D2RL::PluginContext* ctx) noexcept;

bool ReadPortalDistance(void* player, void* candidate, int& measured) noexcept {
    __try {
        if (!context || !player || !candidate || *static_cast<uint32_t*>(candidate) != 2) return false;
        const auto base = context->exeBase;
        auto getClass = reinterpret_cast<uint32_t(__fastcall*)(void*, const char*, uint32_t)>(base + 0x349860);
        auto getVersion = reinterpret_cast<uint8_t(__fastcall*)(void*)>(base + 0x34A0E0);
        auto getObject = reinterpret_cast<const uint8_t*(__fastcall*)(uint8_t, uint32_t)>(base + 0x38FD00);
        auto distance = reinterpret_cast<int(__fastcall*)(void*, void*)>(base + 0x325140);
        const auto record = getObject(getVersion(candidate), getClass(candidate, "Controller QOL Updates", 0));
        if (!record || !(record[0x127] & 4)) return false;
        measured = distance(player, candidate);
        return measured >= 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

__declspec(noinline) int __fastcall CandidateContact(void* player, void* candidate) {
    // A JMP-only relay preserves the native CALL return address. Call the shared
    // entry (including other owners), never a cached trampoline past their hooks.
    const auto returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const int nativeContact = originalContact(player, candidate);
    if (!active.load() || !context) return nativeContact;
    const uintptr_t caller = returnAddress - context->exeBase;
    if (!IsCandidateContactCaller(caller)) return nativeContact;
    const bool modified = ControllerQoL::IsGroundPickupActive(modifier);
    if (modified) return nativeContact;
    int measured = -1;
    const bool portal = ReadPortalDistance(player, candidate, measured);
    const bool extended = ExtendCandidateContact(true, modified, caller, portal, measured, range);
    if (portal && trace) {
        static thread_local ULONGLONG lastLog = 0;
        const auto now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            char message[224];
            std::snprintf(message, sizeof(message),
                "[QOL/Portal] Candidate contact: caller=0x%llX distance=%d configured=%u native=%d extended=%d.",
                static_cast<unsigned long long>(caller), measured, range, nativeContact, extended);
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
    if (!active.load() || ControllerQoL::IsGroundPickupActive(modifier)) return nativeScore;
    int measured = -1;
    if (!ReadPortalDistance(player, candidate, measured)) return nativeScore;
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
                "[QOL/Portal] Candidate score: distance=%d configured=%u profile=%d native=%.3f result=%.3f retry=%d centerLimit=%.2f->%.2f.",
                measured, range, profile, nativeScore, result, retried, oldLimit, newLimit);
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
        if (ControllerQoL::IsGroundPickupActive(modifier)) return false;
        int measured = -1;
        if (!ReadPortalDistance(*state->player, candidate, measured)) return false;
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
bool CaptureLootPortalSelection(Selection* state,void* candidate,void*& selected,float& best) noexcept {
    __try {
        if(!active.load() || !state || !state->skill || !state->selected || !state->bestScore ||
            !state->player || !*state->player || !candidate ||
            *state->skill!=InteractSkill || !ControllerQoL::IsGroundPickupActive(modifier)) return false;
        int distance=-1;
        if(!LootOwnsPortalSelection(true,true,*state->skill,ReadPortalDistance(*state->player,candidate,distance))) return false;
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
    if(CaptureLootPortalSelection(state,candidate,previousSelection,previousScore)) {
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
            std::snprintf(message, sizeof(message), "[QOL/Portal] Interact comparison: distance=%d configured=%u accepted=%d boosted=%d.",
                window.distance, range, *state->selected == candidate, changed);
            context->LogInfo(message);
        }
    }
}
}

void Initialize(const D2RL::PluginContext* ctx, bool enabled, bool debug,
    const char* groundModifier, uint32_t radius) noexcept {
    if (!ctx || !enabled) return;
    for (const auto& site : PortalNative::Sites) {
        if (!ctx->CheckExpectedBytes(site.rva, site.bytes, site.size)) {
            ctx->LogWarn("[QOL/Portal] Native profile mismatch; portal priority disabled and normal targeting preserved.");
            return;
        }
    }
    context = ctx;
    trace = debug;
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
    char message[160];
    std::snprintf(message, sizeof(message), "[QOL/Portal] Three contact CALL patches (shared entry untouched), scoring and Interact range enabled within %u native units; diagnostics=%d.", range, trace);
    ctx->LogInfo(message);
}
void Shutdown() noexcept { active.store(false); }
}
