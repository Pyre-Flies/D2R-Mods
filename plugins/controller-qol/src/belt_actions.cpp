#include "belt_actions.h"
#include "materials_actions.h"
#include "vendor_buy.h"
#include "vendor_policy.h"
#include "belt_policy.h"
#include "belt_signatures.h"
#include "belt_compatibility.h"
#include "belt_native_contract.h"
#include "native_d2r.h"
#include "plugin_compatibility.h"
#include "vendor_signatures.h"
#include "client_transfer_policy.h"
#include "shared_sdk_selection.h"
#include "shared_owner_compatibility.h"
#include "identify_probe_profile.h"
#include "native_identify_profile.h"
#include "materials_signatures.h"
#include "client_transfer_profile.h"
#include "shared_deposit_signatures.h"
#include "storage_focus.h"
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
bool clientRoute{};
bool bindSharedFocus{};
ULONGLONG singleStarted{};
uintptr_t generation{};
bool vendor{}, vendorReady{};
bool (*shopOpen)() noexcept{};
Info stock{};
QolVendorBuy::TomePurchase tomePurchase{};
ULONGLONG vendorStarted{};
D2RL::PlayerHandle owner{};
std::array<Info, 512> candidates{};
size_t count{}, index{};
uint32_t confirmed{}, skipped{};
Phase phase{Phase::Ready};
ULONGLONG deadline{}, queuedAt{};

void Reset() noexcept {
    ++generation;
    active = queued = collect = includeStash = vendor = vendorReady = false;
    clientRoute=false;
    bindSharedFocus=false;singleStarted=0;
    shopOpen = nullptr; stock = {}; vendorStarted = 0;
    tomePurchase={};
    count = index = confirmed = skipped = 0;
    owner = D2RL::InvalidPlayerHandle;
    phase = Phase::Ready;
    validate = true;
}
bool Validate(const D2RL::PluginContext* ctx) noexcept {
    for (const auto& site : Signatures::All) {
        const bool matched = site.rva == D2R::Native::GetFreeBeltSlotRva
            ? QolCompat::ValidateBeltEntry(ctx->exeBase + site.rva)
            : site.rva == D2R::Native::ShiftRightClickPlaceActionRva
                ? QolBeltCompat::Stored(ctx->exeBase+site.rva)
                : QolBeltCompat::Match(ctx->exeBase+site.rva,site.bytes,site.size);
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
    D2RL::Items::ContainerBit(Container::Cube) |
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
struct NativeCall { Action action; NativeResult result{NativeResult::Failed}; int32_t slot{-1}; Container source{Container::Inventory}; const char* refusal{"native-access"}; };

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
            // R8B carries the source page (Inventory=0, Cube=3, stash=4).
            // This helper already finishes interaction.
            void* sourceOwner=call.source==Container::SharedStash?D2R::Native::GetStashContainerUnit(base,1):player;
            if (SubmitStoredPlacement(reinterpret_cast<D2R::Native::ShiftRightClickPlaceActionFn>(base + D2R::Native::ShiftRightClickPlaceActionRva), item, sourceOwner, call.slot,QolClientTransfer::NativePage(call.source)))
                call.result = NativeResult::Submitted; // NOT proof of server mutation.
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        call.result = NativeResult::Failed;
    }
}
NativeCall Native(const D2RL::PluginContext* ctx, const Info& info, Action action) noexcept {
    NativeCall call{action};
    if(clientRoute) {
        call.source=info.container;
        // UI-owned client unit: resolve copied identity afresh. Never ask the
        // authoritative editNativeItem service to expose a remote client unit.
        if(!ClientBeltSource(info.container,info.inventoryPage) || action==Action::WithdrawShared)return call;
        __try {
            call.refusal="client-helper-guard";
            if(!QolBeltCompat::Match(ctx->exeBase+0x9a5d0,QolVendor::ClientUnitBytes,sizeof(QolVendor::ClientUnitBytes)) ||
                !QolBeltCompat::Match(ctx->exeBase+0x36ef50,QolVendor::ItemCodeBytes,sizeof(QolVendor::ItemCodeBytes)) ||
                !QolBeltCompat::Match(ctx->exeBase+0x36cfe0,QolVendor::ItemPageBytes,sizeof(QolVendor::ItemPageBytes)) ||
                !QolBeltCompat::Match(ctx->exeBase+0xce500,QolMaterials::Signatures::UiMode,sizeof(QolMaterials::Signatures::UiMode)) ||
                !QolBeltCompat::Match(ctx->exeBase+0x1c7360,QolIdentifyProbe::Blocked,sizeof(QolIdentifyProbe::Blocked)) ||
                !QolBeltCompat::Match(ctx->exeBase+0x14f1e0,QolNativeIdentifyProfile::Cursor,sizeof(QolNativeIdentifyProfile::Cursor)))return call;
            const auto mode=reinterpret_cast<bool(__fastcall*)(int)>(ctx->exeBase+0xce500);
            if(info.container==Container::Cube && !mode(0x19)) {
                call.refusal="embedded-Cube-context-or-focus";
                const auto tab=QolMaterials::SelectedStashTab(ctx);
                if(!QolClientTransfer::EmbeddedCube(mode(0x18),tab,info.container) ||
                   !QolStorageFocus::Matches(ctx,info))return call;
            }
            if(info.container==Container::PersonalStash || info.container==Container::SharedStash) {
                call.refusal="stash-context-or-guard";
                if(!mode(0x18) ||
                   !ctx->CheckExpectedBytes(0x846170,QolCustomProfile::Game846170,sizeof(QolCustomProfile::Game846170)) ||
                   !ctx->CheckExpectedBytes(0x15edb0,QolClientTransfer::Profile::OwnerSelection,sizeof(QolClientTransfer::Profile::OwnerSelection)) ||
                   !ctx->CheckExpectedBytes(0x23af50,QolMaterials::Signatures::SelectedTab,sizeof(QolMaterials::Signatures::SelectedTab)))return call;
                const auto tab=D2R::Native::GetActiveStashTabIndex(ctx->exeBase);
                call.refusal="stash-tab";
                if(tab!=(info.container==Container::SharedStash?1u:0u))return call;
                if(info.container==Container::SharedStash) {
                    call.refusal="Shared-page-or-owner-guard";
                    const auto selected=QolSharedSdk::Selected(ctx->exeBase);
                    if(!selected.valid || selected.previousSeason || selected.page!=info.sharedStashPage || !QolShared::ValidateOwner(ctx->exeBase) ||
                       !ctx->CheckExpectedBytes(0x2ef880,SharedOwnerIdBytes,sizeof(SharedOwnerIdBytes)) || !QolStorageFocus::Matches(ctx,info))return call;
                }
            }
            void* unit=reinterpret_cast<void*(__fastcall*)(uint32_t,uint32_t)>(ctx->exeBase+0x9a5d0)(info.runtimeId,4);
            call.refusal="native-item-identity-page-or-busy";
            const auto* words=static_cast<const uint32_t*>(unit);
            if(!unit || words[0]!=4 || words[1]!=info.classId || words[2]!=info.runtimeId || words[3]!=0 ||
                reinterpret_cast<uint32_t(__fastcall*)(void*)>(ctx->exeBase+0x36ef50)(unit)!=info.code ||
                reinterpret_cast<uint8_t(__fastcall*)(void*)>(ctx->exeBase+0x36cfe0)(unit)!=QolClientTransfer::NativePage(info.container) ||
                reinterpret_cast<int(__fastcall*)(void*)>(ctx->exeBase+0x1c7360)(unit) ||
                reinterpret_cast<void*(__fastcall*)()>(ctx->exeBase+0x14f1e0)())return call;
            call.refusal="belt-capacity-or-owner";
            NativeCallback(ctx,unit,&call); // guarded native client request, synchronous pointer lifetime
        } __except(EXCEPTION_EXECUTE_HANDLER) {call.result=NativeResult::Failed;}
        return call;
    }
    if (items->editNativeItem(ctx, info.handle, NativeCallback, &call) != D2RL::Items::Result::Success)
        call.result = NativeResult::Failed;
    return call;
}

void __cdecl Tick(const D2RL::PluginContext* ctx, void* token) noexcept {
    std::lock_guard lock(mutex);
    if (reinterpret_cast<uintptr_t>(token)!=generation) return;
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
                    if (BeltCandidate(item->code) && (!vendor || item->code==stock.code) && SupportedSource(item->container) && count < candidates.size())
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
        if (vendor) { vendorReady=true; return; }
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
    if(clientRoute && !ClientBeltSource(live.container,live.inventoryPage)) {
        Finish(ctx,"Stopped: remote belt source unsupported");return;
    }
    const auto probe = Native(ctx, live, Action::Probe);
    if (probe.result == NativeResult::NoRoom) { ++skipped; ++index; return; }
    if (probe.result != NativeResult::Room) {
        char line[224];std::snprintf(line,sizeof(line),"Stopped: native belt preflight failed reason=%s item=%u container=%u sdkPage=%u sharedPage=%u",probe.refusal,live.runtimeId,static_cast<unsigned>(live.container),live.inventoryPage,live.sharedStashPage);
        Finish(ctx,line);return;
    }
    if (live.container == Container::Inventory || clientRoute) {
        if (live.container==Container::Inventory && live.inventoryPage != 0) { Finish(ctx, "Stopped: unexpected inventory page"); return; }
        const auto placed = Native(ctx, live, Action::Place);
        if (placed.result == NativeResult::NoRoom) { ++skipped; ++index; return; }
        if (placed.result != NativeResult::Submitted) { Finish(ctx, "Stopped: native placement failed"); return; }
        phase = Phase::AwaitBelt;
        char message[160];
        std::snprintf(message, sizeof(message), "[QOL/Belt] Submitted stored-item placement runtimeId=%u slot=%d; awaiting SDK confirmation.", live.runtimeId, placed.slot);
        ctx->LogInfo(message);
    } else if (live.container == Container::PersonalStash || live.container==Container::Cube) {
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
bool RequestSingle(D2RL::PlayerHandle player, const Info& item,bool bindFocus) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !player || !SupportedSource(item.container) || !BeltCandidate(item.code)) return false;
    if (active || QolMaterials::Busy()) return true; // Consume repeated LB+A; do not also drink the item.
    Reset(); owner = player; candidates[0] = item; count = 1; active = true;
    bindSharedFocus=bindFocus;singleStarted=GetTickCount64();
    return true;
}
bool RequestRefill(bool stash) noexcept {
    std::lock_guard lock(mutex);
    if (!ready) return false;
    if (active || QolMaterials::Busy()) return true;
    Reset(); collect = true; includeStash = stash; active = true;
    return true;
}
bool RequestVendorRefill(const D2RL::PluginContext* ctx, const Info& info, bool (*isShopOpen)() noexcept) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !threads->runOnUiThread || active || QolMaterials::Busy() || !isShopOpen || !isShopOpen() ||
        (!BeltCandidate(info.code) && !QolVendor::TomeForScroll(info.code)) ||
        !QolVendorBuy::Check(ctx,info)) return false;
    D2RL::PlayerHandle player{};
    if (inventory->getLocalPlayer(ctx,&player)!=D2RL::Inventory::Result::Success || !player) return false;
    Reset(); stock=info; owner=player; shopOpen=isShopOpen;
    vendor=active=true;vendorReady=QolVendor::TomeForScroll(info.code)!=0;collect=!vendorReady;vendorStarted=GetTickCount64();
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
    if (vendor && GetTickCount64()-vendorStarted>10000) { Finish(ctx,"Cancelled: vendor refill expired"); return; }
    queued = true; queuedAt = GetTickCount64();
    if(bindSharedFocus) {
        const auto result=threads->runOnUiThread?threads->runOnUiThread(ctx,[](const D2RL::PluginContext* context,void* token) noexcept {
            std::lock_guard lock(mutex);
            if(reinterpret_cast<uintptr_t>(token)!=generation || !active || !bindSharedFocus)return;
            queued=false;
            D2RL::PlayerHandle current{};Info live{.structSize=D2RL::Items::ItemInfoSize};
            const auto selected=QolSharedSdk::Selected(context->exeBase);
            if(GetTickCount64()-singleStarted>750 ||
               inventory->getLocalPlayer(context,&current)!=D2RL::Inventory::Result::Success || current!=owner ||
               !Find(context,candidates[0],live) || !selected.valid || selected.previousSeason ||
               !BindShared(candidates[0],live,selected.page,QolStorageFocus::Matches(context,live))) {
                Finish(context,"Shared potion selection changed before binding; no action");return;
            }
            // Capture the source from the same enumeration used for subsequent
            // checks, only while the actual controller cell still selects it.
            // Never relax SameSource after this initial binding.
            if(!SameSource(candidates[0],live)) {
                char line[256];std::snprintf(line,sizeof(line),"[QOL/Belt] Bound selected Shared item=%u tooltip page=%u shared=%u cell=%d,%d -> snapshot page=%u shared=%u cell=%d,%d.",live.runtimeId,candidates[0].inventoryPage,candidates[0].sharedStashPage,candidates[0].x,candidates[0].y,live.inventoryPage,live.sharedStashPage,live.x,live.y);context->LogInfo(line);
            }
            candidates[0]=live;bindSharedFocus=false;
        },reinterpret_cast<void*>(generation)):D2RL::Threads::Result::Unavailable;
        if(result!=D2RL::Threads::Result::Success)Finish(ctx,"Shared focus binding scheduling unavailable");
        return;
    }
    if (vendorReady) {
        const auto result=threads->runOnUiThread(ctx,[](const D2RL::PluginContext* context,void* token) noexcept {
            std::lock_guard uiLock(mutex);
            if (reinterpret_cast<uintptr_t>(token)!=generation) return;
            queued=false;
            if (!ready || !active || !vendorReady) return;
            D2RL::PlayerHandle current{};
            if (GetTickCount64()-vendorStarted>10000 || !shopOpen || !shopOpen() || !Validate(context) ||
                inventory->getLocalPlayer(context,&current)!=D2RL::Inventory::Result::Success || !current || current!=owner) {
                Finish(context,"Vendor context changed; no retry");return;
            }
            if(QolVendor::TomeForScroll(stock.code)) {
                if(!tomePurchase.submitted) {
                    if(!QolVendorBuy::SubmitScroll(context,stock,tomePurchase)) {
                        Finish(context,"Tome refill refused or faulted; no retry");return;
                    }
                    deadline=GetTickCount64()+1500;return;
                }
                const auto observed=QolVendorBuy::ObserveScroll(context,tomePurchase);
                if(observed==QolVendorBuy::TomeObservation::Increased)Finish(context,"Tome purchase observed");
                else if(observed==QolVendorBuy::TomeObservation::Changed || GetTickCount64()>=deadline)
                    Finish(context,"Tome changed or purchase unconfirmed; no retry");
                return;
            }
            QolVendorBuy::Submit(context,stock);
            Finish(context,"Vendor refill finished (purchase submission is not confirmation)");
        },reinterpret_cast<void*>(generation));
        if (result!=D2RL::Threads::Result::Success) Finish(ctx,"Cancelled: vendor UI scheduling rejected");
        return;
    }
    const auto callback=+[](const D2RL::PluginContext* context, void* token) noexcept {
        {
            std::lock_guard ownerLock(mutex);
            if (reinterpret_cast<uintptr_t>(token)!=generation) return;
            if (active && collect && !owner) {
                if (inventory->getLocalPlayer(context, &owner) != D2RL::Inventory::Result::Success) owner = 0;
            }
        }
        Tick(context, token);
    };
    auto result=clientRoute
        ? threads->runOnUiThread(ctx,callback,reinterpret_cast<void*>(generation))
        : threads->runOnGameThread(ctx,callback,reinterpret_cast<void*>(generation));
    if(!clientRoute && result==D2RL::Threads::Result::Unavailable && threads->runOnUiThread) {
        clientRoute=true;
        ctx->LogInfo("[QOL/Belt] No local authority: using client inventory snapshots and native belt requests.");
        result=threads->runOnUiThread(ctx,callback,reinterpret_cast<void*>(generation));
    }
    if(result!=D2RL::Threads::Result::Success)Finish(ctx,"Cancelled: loader rejected belt scheduling");
}
}
