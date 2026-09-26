#include "belt_actions.h"
#include "materials_actions.h"
#include "belt_policy.h"
#include "belt_signatures.h"
#include "belt_native_contract.h"
#include "native_d2r.h"
#include "plugin_compatibility.h"
#include <windows.h>
#include <array>
#include <mutex>
#include <cstdio>

namespace QolBelt {
namespace {
using Info = D2RL::Items::ItemInfo;
const D2RL::ItemService* items{};
const D2RL::InventoryService* inventory{};
const D2RL::ThreadService* threads{};
std::mutex mutex;
bool ready{}, active{}, queued{}, collect{}, includeStash{}, validate{};
D2RL::PlayerHandle owner{};
std::array<Info, 512> candidates{};
size_t count{}, index{};
uint32_t confirmed{}, skipped{};
Phase phase{Phase::Ready};
ULONGLONG deadline{}, queuedAt{};

void Reset() noexcept {
    active = queued = collect = includeStash = false;
    count = index = confirmed = skipped = 0;
    owner = D2RL::InvalidPlayerHandle;
    phase = Phase::Ready;
    validate = true;
}
bool Validate(const D2RL::PluginContext* ctx) noexcept {
    for (const auto& site : Signatures::All) {
        const bool matched = site.rva == D2R::Native::GetFreeBeltSlotRva
            ? QolCompat::ValidateBeltEntry(ctx->exeBase + site.rva)
            : ctx->CheckExpectedBytes(site.rva, site.bytes, site.size);
        if (!matched) {
            char message[128];
            std::snprintf(message, sizeof(message), "[QOL/Belt] Native admission mismatch at RVA 0x%llX.", static_cast<unsigned long long>(site.rva));
            ctx->LogWarn(message);
            return false;
        }
    }
    return true;
}
void Finish(const D2RL::PluginContext* ctx, const char* reason) noexcept {
    char message[240];
    std::snprintf(message, sizeof(message), "[QOL/Belt] %s; confirmed=%u skipped=%u remaining=%zu.",
        reason, confirmed, skipped, count-index);
    ctx->LogInfo(message);
    Reset();
}
constexpr uint32_t ScanMask = D2RL::Items::ContainerBit(Container::Inventory) |
    D2RL::Items::ContainerBit(Container::PersonalStash) |
    D2RL::Items::ContainerBit(Container::SharedStash) |
    D2RL::Items::ContainerBit(Container::Belt);

bool Find(const D2RL::PluginContext* ctx, const Info& identity, Info& output) noexcept {
    struct Search { const Info* identity; Info* output; bool found; } search{&identity, &output, false};
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize, 0, ScanMask, 0};
    const auto result = inventory->forEachInventoryItem(ctx, owner, &filter,
        [](const D2RL::PluginContext*, const Info* info, void* user) noexcept {
            auto& s = *static_cast<Search*>(user);
            if (SameItem(*info, *s.identity)) {
                *s.output = *info;
                s.found = true;
                return D2RL::Inventory::IterationAction::Stop;
            }
            return D2RL::Inventory::IterationAction::Continue;
        }, &search);
    return result == D2RL::Inventory::Result::Success && search.found;
}

enum class Action { Probe, Place, WithdrawShared };
enum class NativeResult { Failed, NoRoom, Submitted, Room };
struct NativeCall { Action action; NativeResult result{NativeResult::Failed}; int32_t slot{-1}; };

// editNativeItem owns the pointer lifetime. Nothing saves item or player pointers.
void __cdecl NativeCallback(const D2RL::PluginContext* ctx, void* item, void* user) noexcept {
    auto& call = *static_cast<NativeCall*>(user);
    if (!ctx->exeBase || !item) return;
    __try {
        const auto base = static_cast<uintptr_t>(ctx->exeBase);
        void* player = D2R::Native::GetLocalPlayerUnit(base);
        if (!player) return;
        void* inv = reinterpret_cast<D2R::Native::GetUnitInventoryFn>(base + D2R::Native::GetUnitInventoryRva)(player);
        if (!inv) return;
        const auto freeSlot = reinterpret_cast<D2R::Native::GetFreeBeltSlotFn>(base + D2R::Native::GetFreeBeltSlotRva);
        if (!freeSlot(inv, item, &call.slot, true) || call.slot < 0 || call.slot >= 16) {
            call.result = NativeResult::NoRoom;
            return;
        }
        if (call.action == Action::Probe) {
            call.result = NativeResult::Room;
        } else if (call.action == Action::WithdrawShared) {
            if (D2R::Native::TransferFromStashToInventory(base, item, player))
                call.result = NativeResult::Submitted;
        } else {
            // Native callers 0x2AA7C5/0x2C7D54 use R9B=1 for STORED items.
            // R8B=0 is inventory page. This helper already finishes interaction.
            if (SubmitStoredPlacement(reinterpret_cast<D2R::Native::ShiftRightClickPlaceActionFn>(base + D2R::Native::ShiftRightClickPlaceActionRva), item, player, call.slot))
                call.result = NativeResult::Submitted; // NOT proof of server mutation.
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        call.result = NativeResult::Failed;
    }
}
NativeCall Native(const D2RL::PluginContext* ctx, const Info& info, Action action) noexcept {
    NativeCall call{action};
    if (items->editNativeItem(ctx, info.handle, NativeCallback, &call) != D2RL::Items::Result::Success)
        call.result = NativeResult::Failed;
    return call;
}

void __cdecl Tick(const D2RL::PluginContext* ctx, void*) noexcept {
    std::lock_guard lock(mutex);
    queued = false;
    if (!ready || !active) return;
    if (validate) {
        if (!Validate(ctx)) { Finish(ctx, "Disabled: native contract changed"); ready = false; return; }
        validate = false;
    }
    D2RL::PlayerHandle current{};
    if (inventory->getLocalPlayer(ctx, &current) != D2RL::Inventory::Result::Success || !current || current != owner) {
        Finish(ctx, "Cancelled: player/session changed");
        return;
    }
    if (collect) {
        // Bulk refill uses inventory, then Materials proxies below. Never silently
        // drain Personal or Shared pages. Single-item actions retain their source.
        for (const auto container : {Container::Inventory}) {
            const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize, 0,
                D2RL::Items::ContainerBit(container), 0};
            const auto result = inventory->forEachInventoryItem(ctx, owner, &filter,
                [](const D2RL::PluginContext*, const Info* item, void*) noexcept {
                    if (BeltCandidate(item->code) && SupportedSource(item->container) && count < candidates.size())
                        candidates[count++] = *item;
                    return count == candidates.size() ? D2RL::Inventory::IterationAction::Stop : D2RL::Inventory::IterationAction::Continue;
                }, nullptr);
            if (result != D2RL::Inventory::Result::Success) {
                Finish(ctx, "Cancelled: inventory enumeration unavailable");
                return;
            }
        }
        collect = false;
        if (count == candidates.size()) ctx->LogWarn("[QOL/Belt] Candidate limit reached (512); press refill again for remaining items.");
    }
    if (index >= count) {
        if (includeStash) QolMaterials::RequestRefill();
        Finish(ctx, "Batch complete"); return;
    }
    Info live{.structSize = D2RL::Items::ItemInfoSize};
    const bool found = Find(ctx, candidates[index], live);
    if (phase != Phase::Ready) {
        const auto observation = Observe(phase, live.container, found, GetTickCount64() >= deadline);
        if (observation == Observation::Wait) return;
        if (observation != Observation::Confirmed) {
            Finish(ctx, "Stopped: submitted move was not confirmed; inspect item before retrying");
            return;
        }
        if (phase == Phase::AwaitBelt) {
            ++confirmed;
            char message[160];
            std::snprintf(message, sizeof(message), "[QOL/Belt] Confirmed item runtimeId=%u code=0x%08X in belt (x=%d y=%d).",
                live.runtimeId, live.code, live.x, live.y);
            ctx->LogInfo(message);
            ++index;
            phase = Phase::Ready;
            return; // One submission at most per SDK tick; observe client sync next tick.
        }
        candidates[index] = live; // Withdrawal observed in inventory; re-probe capacity.
        phase = Phase::Ready;
    }
    if (!found || !SameSource(candidates[index], live) || !SupportedSource(live.container)) {
        char message[192];
        std::snprintf(message, sizeof(message), "[QOL/Belt] Skipped stale/changed source runtimeId=%u code=0x%08X; found=%d container=%u cell=(%d,%d).",
            candidates[index].runtimeId, candidates[index].code, found ? 1 : 0,
            static_cast<uint32_t>(live.container), live.x, live.y);
        ctx->LogWarn(message);
        ++skipped; ++index;
        return; // Never replace a stale identity with a same-code/position item.
    }
    const auto probe = Native(ctx, live, Action::Probe);
    if (probe.result == NativeResult::NoRoom) { ++skipped; ++index; return; }
    if (probe.result != NativeResult::Room) { Finish(ctx, "Stopped: native belt preflight failed"); return; }
    if (live.container == Container::Inventory) {
        if (live.inventoryPage != 0) { Finish(ctx, "Stopped: unexpected inventory page"); return; }
        const auto placed = Native(ctx, live, Action::Place);
        if (placed.result == NativeResult::NoRoom) { ++skipped; ++index; return; }
        if (placed.result != NativeResult::Submitted) { Finish(ctx, "Stopped: native placement failed"); return; }
        phase = Phase::AwaitBelt;
        char message[160];
        std::snprintf(message, sizeof(message), "[QOL/Belt] Submitted stored-item placement runtimeId=%u slot=%d; awaiting SDK confirmation.", live.runtimeId, placed.slot);
        ctx->LogInfo(message);
    } else if (live.container == Container::PersonalStash) {
        D2RL::Items::ExistingItemOperation operation{};
        operation.structSize = D2RL::Items::ExistingItemOperationSize;
        operation.kind = D2RL::Items::ExistingItemOperationKind::Move;
        operation.item = live.handle;
        operation.move.destination.structSize = D2RL::Items::ItemDestinationSize;
        operation.move.destination.container = Container::Inventory;
        operation.move.destination.placement = D2RL::Items::Placement::Automatic;
        const D2RL::Items::ExistingItemTransaction transaction{
            .structSize = D2RL::Items::ExistingItemTransactionSize,
            .player = owner, .operationCount = 1, .operations = &operation};
        D2RL::Items::ExistingItemTransactionResult result{.structSize = D2RL::Items::ExistingItemTransactionResultSize};
        if (items->executeExistingItemTransaction(ctx, &transaction, &result) != D2RL::Items::Result::Success || result.committedOperationCount != 1) {
            Finish(ctx, "Stopped: SDK stash withdrawal refused (inventory may be full)"); return;
        }
        phase = Phase::AwaitInventory;
    } else {
        // Shared stash is not supported by SDK transactions. Keep the existing
        // native withdrawal, but never place until its result is observed.
        if (live.inventoryPage != 1 || Native(ctx, live, Action::WithdrawShared).result != NativeResult::Submitted) {
            Finish(ctx, "Stopped: shared-stash withdrawal unavailable"); return;
        }
        phase = Phase::AwaitInventory;
    }
    deadline = GetTickCount64() + 1500;
}

void __cdecl SessionChanged(const D2RL::PluginContext*, const D2RL::Lifecycle::GameplayEvent*, void*) noexcept {
    std::lock_guard lock(mutex);
    Reset(); // Loader discards old session game tasks; clear our queue marker too.
}
}

bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex);
    Reset(); ready = false;
    if (!ctx || !ctx->exeBase || ctx->QueryService(&items) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasItemServiceField(items, D2RL::ItemServiceRequiredSize) || !items->editNativeItem || !items->executeExistingItemTransaction ||
        ctx->QueryService(&inventory) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasInventoryServiceField(inventory, D2RL::InventoryServiceRequiredSize) || !inventory->forEachInventoryItem || !inventory->getLocalPlayer ||
        ctx->QueryService(&threads) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(threads, D2RL::ThreadServiceRequiredSize) || !threads->runOnGameThread) return false;
    if (!Validate(ctx)) return false;
    const D2RL::LifecycleService* lifecycle{};
    if (ctx->QueryService(&lifecycle) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasLifecycleServiceField(lifecycle, D2RL::LifecycleServiceRequiredSize) || !lifecycle->registerGameplayEventListener) return false;
    for (auto kind : {D2RL::Lifecycle::GameplayEventKind::GameJoined, D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize, 0, kind, 0, SessionChanged, nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        if (lifecycle->registerGameplayEventListener(ctx, &listener, &handle) != D2RL::Lifecycle::Result::Success) return false;
    }
    ready = true;
    ctx->LogInfo("[QOL/Belt] Native contract admitted; SDK scheduling, snapshots, scoped native access and confirmation enabled.");
    return true;
}
void Shutdown() noexcept { std::lock_guard lock(mutex); ready = false; Reset(); }
bool Busy() noexcept { std::lock_guard lock(mutex); return active; }
bool RequestSingle(D2RL::PlayerHandle player, const Info& item) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !player || !SupportedSource(item.container) || !BeltCandidate(item.code)) return false;
    if (active || QolMaterials::Busy()) return true; // Consume repeated LB+A; do not also drink the item.
    Reset(); owner = player; candidates[0] = item; count = 1; active = true;
    return true;
}
bool RequestRefill(bool stash) noexcept {
    std::lock_guard lock(mutex);
    if (!ready) return false;
    if (active || QolMaterials::Busy()) return true;
    Reset(); collect = true; includeStash = stash; active = true;
    return true;
}
void Pump(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !active) return;
    // Do not enumerate or resolve players on this worker. The first game task
    // resolves refill's owner. No heap userData can leak when SDK drops a task.
    if (queued) {
        if (GetTickCount64() - queuedAt > 5000) Finish(ctx, "Cancelled: scheduled game task unavailable");
        return;
    }
    queued = true; queuedAt = GetTickCount64();
    const auto result = threads->runOnGameThread(ctx, [](const D2RL::PluginContext* context, void*) noexcept {
        {
            std::lock_guard ownerLock(mutex);
            if (active && collect && !owner) {
                if (inventory->getLocalPlayer(context, &owner) != D2RL::Inventory::Result::Success) owner = 0;
            }
        }
        Tick(context, nullptr);
    }, nullptr);
    if (result != D2RL::Threads::Result::Success) Finish(ctx, "Cancelled: loader rejected game-thread scheduling");
}
}
