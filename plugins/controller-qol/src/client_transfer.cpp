#include "client_transfer.h"
#include "client_transfer_profile.h"
#include "client_request_profile.h"
#include "native_d2r.h"
#include "native_identify.h"
#include "native_identify_profile.h"
#include "native_input_profile.h"
#include "core_compatibility.h"
#include "controller_input.h"
#include "belt_actions.h"
#include "belt_signatures.h"
#include "materials_actions.h"
#include "materials_signatures.h"
#include "bulk_stash.h"
#include "vendor_signatures.h"
#include "identify_probe_profile.h"
#include "shared_sdk_selection.h"
#include "shared_deposit_signatures.h"
#include "shared_owner_compatibility.h"
#include "storage_focus.h"
#include "storage_focus_profile.h"
#include <atomic>
#include <mutex>
#include <cstdio>
namespace QolClientTransfer {
namespace {
std::recursive_mutex mutex;
std::atomic<bool> active{false};
bool ready{};
RequestFence fence{};
const D2RL::PluginContext* context{};
const D2RL::ItemService* items{};
const D2RL::InventoryService* inventory{};
const D2RL::ThreadService* threads{};
struct Work {
    Info source{};C destination{C::Unknown};bool captured{};
    D2RL::PlayerHandle player{};
    uint32_t tab{UINT32_MAX};QolSharedSdk::Selection selection{};
    bool stashAction{},advanced{},cubeInStash{};
    int64_t quantityBefore{};
} work;
template<class T>T At(uintptr_t rva) noexcept {return reinterpret_cast<T>(context->exeBase+rva);}
bool Guard() noexcept {
    if(!context)return false;
    for(const auto& site:Profile::Sites)if(!context->CheckExpectedBytes(site.rva,site.bytes,site.size))return false;
    using namespace QolBelt::Signatures;
    using namespace QolNativeIdentifyProfile;
    const Profile::Site helpers[]={
        {0x8b2d0,LocalContext,sizeof(LocalContext)},{0x9a480,LocalPlayer,sizeof(LocalPlayer)},
        {0x9a5d0,QolVendor::ClientUnitBytes,sizeof(QolVendor::ClientUnitBytes)},
        {0x36cfe0,QolVendor::ItemPageBytes,sizeof(QolVendor::ItemPageBytes)},
        {0x36ef50,Code,sizeof(Code)},{0x14f1e0,Cursor,sizeof(Cursor)},{0x14f210,Mode,sizeof(Mode)},
        {0x1c7360,QolIdentifyProbe::Blocked,sizeof(QolIdentifyProbe::Blocked)},
        {0xce500,QolMaterials::Signatures::UiMode,sizeof(QolMaterials::Signatures::UiMode)},
        {0x846170,FindPanel,sizeof(FindPanel)},
        {0x23af50,SharedPageNative::SelectedTab,sizeof(SharedPageNative::SelectedTab)},
        {0x2ef880,SharedOwnerIdBytes,sizeof(SharedOwnerIdBytes)}
    };
    for(const auto& site:helpers)if(!context->CheckExpectedBytes(site.rva,site.bytes,site.size))return false;
    // Core redirects one CALL inside this owner resolver. Reuse the reviewed
    // relocation-aware contract: unchanged body, exact thunk target and wrapper.
    if(!QolShared::ValidateOwner(context->exeBase)) {
        context->LogWarn("[QOL/ClientTransfer] Shared owner resolver contract mismatch; remote transfers disabled.");
        return false;
    }
    return QolClientRequest::Guard(context->exeBase,reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll")));
}
bool Mode(int mode) noexcept {return At<bool(__fastcall*)(int)>(0xce500)(mode);}
bool DepositGuard() noexcept {
    using namespace QolMaterials::Signatures;
    return context->CheckExpectedBytes(0x15a0b0,QolStorageProfile::DepositEligible,sizeof(QolStorageProfile::DepositEligible)) &&
        context->CheckExpectedBytes(0x46da50,CurrentOwner,sizeof(CurrentOwner)) &&
        context->CheckExpectedBytes(0x23b980,PreviousSeason,sizeof(PreviousSeason)) &&
        context->CheckExpectedBytes(0x1a0780,FinishInteraction,sizeof(FinishInteraction));
}
void Finish(const char* reason) noexcept {
    if(context) {
        char line[256];std::snprintf(line,sizeof(line),"[QOL/ClientTransfer] %s item=%u source=%u destination=%u advanced=%u sharedPage=%u submitted=%u; no retry.",
            reason,work.source.runtimeId,static_cast<unsigned>(work.source.container),static_cast<unsigned>(work.destination),work.advanced?1u:0u,work.selection.page,fence.submitted?1u:0u);
        context->LogInfo(line);
    }
    active=false;fence.Cancel();
}
void __cdecl SessionChanged(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent*,void*) noexcept {
    std::lock_guard lock(mutex);
    if(active)Finish("session-changed");else fence.Cancel();work={};
}
bool StashContext(bool capture) noexcept {
    if(!work.stashAction && !work.cubeInStash && !NeedsStash(work.source.container,work.destination))return true;
    if(!Mode(0x18))return false;
    const auto tab=D2R::Native::GetActiveStashTabIndex(context->exeBase);
    const auto expected=(work.stashAction || work.cubeInStash)?work.tab:(NeedsShared(work.source.container,work.destination)?1u:0u);
    if(tab!=expected || tab>4)return false;
    if(((work.stashAction && work.source.container==C::Inventory) || work.cubeInStash) &&
       (!DepositGuard() || At<bool(__fastcall*)()>(0x23b980)()))return false;
    if(capture)work.tab=tab;
    else if(tab!=work.tab)return false;
    if(expected==1) {
        const auto selected=QolSharedSdk::Selected(context->exeBase);
        if(!selected.valid || selected.previousSeason || selected.page==UINT32_MAX ||
           (work.source.container==C::SharedStash && work.source.sharedStashPage!=selected.page))return false;
        if(capture)work.selection=selected;
        else if(!QolSharedSdk::SameSelection(work.selection,selected))return false;
    }
    return true;
}
bool HasCube() noexcept {
    bool found=false;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(C::Inventory),0};
    return inventory->forEachInventoryItem(context,work.player,&filter,
        [](const D2RL::PluginContext*,const Info* item,void* user) noexcept {
            if(item && QolIdentify::Code(item->code,D2RL::Items::MakeItemCode("box"))) {
                *static_cast<bool*>(user)=true;return D2RL::Inventory::IterationAction::Stop;
            }
            return D2RL::Inventory::IterationAction::Continue;
        },&found)==D2RL::Inventory::Result::Success && found;
}
bool SourceQuantity(int64_t& value) noexcept {
    struct Sum {uint32_t code;int64_t value{};} sum{work.source.code};
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(work.source.container),0};
    const auto result=inventory->forEachInventoryItem(context,work.player,&filter,
        [](const D2RL::PluginContext*,const Info* item,void* user) noexcept {
            auto& sum=*static_cast<Sum*>(user);
            if(item && item->code==sum.code)sum.value+=item->quantity>0?item->quantity:1;
            return D2RL::Inventory::IterationAction::Continue;
        },&sum);
    value=sum.value;return result==D2RL::Inventory::Result::Success;
}
bool ReadItem(Info& out) noexcept {
    out={.structSize=D2RL::Items::ItemInfoSize};
    if(items->getItemInfo(context,work.source.handle,&out)==D2RL::Items::Result::Success && Identity(work.source,out))return true;
    // Container changes may invalidate SDK handles. Search only for this exact
    // runtime identity, including seeds, never a matching code or cell.
    struct Search {const Info* expected;Info* out;bool found{};} search{&work.source,&out};
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,
        D2RL::Items::ContainerBit(work.source.container)|D2RL::Items::ContainerBit(work.destination),0};
    return inventory->forEachInventoryItem(context,work.player,&filter,
        [](const D2RL::PluginContext*,const Info* item,void* user) noexcept {
            auto& search=*static_cast<Search*>(user);
            if(item && Identity(*search.expected,*item)) { *search.out=*item;search.found=true;return D2RL::Inventory::IterationAction::Stop; }
            return D2RL::Inventory::IterationAction::Continue;
        },&search)==D2RL::Inventory::Result::Success && search.found;
}
// UI-only, pointers acquired and discarded within this call. Mirrors the ordinary
// grid action's blocked/owner/eligibility checks before its native transfer helper.
bool Submit() noexcept {
    const auto base=context->exeBase;
    if(!QolStorageFocus::Matches(context,work.source))return false;
    auto player=D2R::Native::GetLocalPlayerUnit(base);
    auto item=static_cast<const uint32_t*>(At<void*(__fastcall*)(uint32_t,uint32_t)>(0x9a5d0)(work.source.runtimeId,4));
    if(!player || !item || item[0]!=4 || item[2]!=work.source.runtimeId || item[3]!=0 ||
       At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(item)!=work.source.code ||
       At<uint8_t(__fastcall*)(const void*)>(0x36cfe0)(item)!=NativePage(work.source.container) ||
       At<int(__fastcall*)(const void*)>(0x1c7360)(item) ||
       At<void*(__fastcall*)()>(0x14f1e0)() || At<int(__fastcall*)()>(0x14f210)()==5)return false;
    void* destination=player;
    if(work.stashAction && (work.source.container==C::Inventory || work.cubeInStash)) {
        if(!DepositGuard())return false;
        const auto route=ChooseStashRoute(work.tab,work.source.container,At<bool(__fastcall*)(const void*)>(0x15a0b0)(item));
        work.advanced=route==StashRoute::Advanced;
        if(route==StashRoute::Refuse)return false;
        if(work.advanced) {
            destination=At<void*(__fastcall*)(void*)>(0x46da50)(player);
            if(!destination || !SourceQuantity(work.quantityBefore))return false;
        }
    }
    if(!work.advanced && work.destination==C::Cube && !HasCube())return false;
    if(!work.advanced && NativePage(work.destination)==4) {
        destination=At<void*(__fastcall*)(bool)>(0x15eec0)(true);
        const auto expected=work.destination==C::SharedStash?D2R::Native::GetStashContainerUnit(base,1):player;
        if(!destination || destination!=expected || !At<bool(__fastcall*)(const void*)>(0x15a340)(item))return false;
    }
    alignas(8) unsigned char placement[16]{}; // Disengaged; native free-space search owns X/Y.
    if(!fence.SubmitOnce(GetTickCount64()))return false; // A false native return must never cause a second submit.
    const auto accepted=At<D2R::Native::TransferItemToInventoryPageFn>(0x15f8b0)(const_cast<uint32_t*>(item),destination,
        work.advanced?4:NativePage(work.destination),NativePage(work.source.container),true,placement);
    if(work.advanced)At<D2R::Native::FinishInventoryInteractionFn>(0x1a0780)(3,nullptr,0,0,false);
    return accepted;
}
void __cdecl Tick(const D2RL::PluginContext*,void*) noexcept;
void Continue() noexcept {
    if(threads->runOnUiThread(context,Tick,reinterpret_cast<void*>(fence.generation))!=D2RL::Threads::Result::Success)Finish("UI-scheduling-unavailable");
}
void TickUnsafe() noexcept {
    __try {
        const auto now=GetTickCount64();
        if(!ready || !Guard() || !ControllerQoL::IsControllerUiActive() || !fence.Poll(now)) {
            Finish("cancelled-or-expired");return;
        }
        D2RL::PlayerHandle player{};
        if(inventory->getLocalPlayer(context,&player)!=D2RL::Inventory::Result::Success || !player ||
           (work.player && player!=work.player)){Finish("player-changed");return;}
        work.player=player;
        if(!Mode(1) && !Mode(0x18) && !Mode(0x19)){Finish("inventory-closed");return;}
        if(!StashContext(!work.captured)){Finish("stash-page-changed-or-unsupported");return;}
        work.captured=true;
        Info live{};
        const bool found=ReadItem(live);
        if(fence.submitted && work.advanced) {
            int64_t after{};
            if(!SourceQuantity(after)){Finish("advanced-observation-unavailable");return;}
            // Advanced counters do not retain this item's runtime identity.
            // This is evidence of source consumption, not a counter/server ACK.
            if((!found || !Source(work.source,live)) && after<work.quantityBefore)
                Finish("advanced-source-decrease-observed-destination-unverified");
            else Continue();
            return;
        }
        if(!found){if(fence.submitted){Continue();return;}Finish("source-unavailable");return;}
        if(fence.submitted) {
            if(Confirmed(work.source,live,work.destination,work.selection.page))Finish("client-confirmed");
            else if(!Source(work.source,live))Finish("item-moved-elsewhere");
            else Continue();
            return;
        }
        if(!Source(work.source,live) ||
           (work.source.container==C::Cube && !CubeView(Mode(0x19),work.cubeInStash && Mode(0x18),work.tab)) || QolNativeIdentify::Busy() || QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy()) {
            Finish("source-changed-Cube-unavailable-or-action-busy");return;
        }
        const bool accepted=Submit();
        if(!fence.submitted){Finish("native-preflight-refused");return;}
        char line[192];std::snprintf(line,sizeof(line),"[QOL/ClientTransfer] submitted item=%u nativePage=%u->%u advanced=%u return=%u; awaiting client state.",work.source.runtimeId,NativePage(work.source.container),work.advanced?4:NativePage(work.destination),work.advanced?1u:0u,accepted?1u:0u);context->LogInfo(line);
        Continue();
    } __except(EXCEPTION_EXECUTE_HANDLER) {ready=false;Finish("native-contract-fault-disabled");}
}
void __cdecl Tick(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(active && ctx==context && fence.Current(reinterpret_cast<uintptr_t>(token)))TickUnsafe();
}
bool CaptureStashRequest() noexcept {
    __try {
        if(!Guard() || !Mode(0x18))return false;
        const auto tab=D2R::Native::GetActiveStashTabIndex(context->exeBase);
        work.cubeInStash=EmbeddedCube(true,tab,work.source.container);
        if(!work.cubeInStash && ChooseStashRoute(tab,work.source.container,false)==StashRoute::Refuse)return false;
        work.stashAction=true;work.tab=tab;
        work.destination=work.source.container==C::Inventory?(tab>1?C::Cube:tab==1?C::SharedStash:C::PersonalStash):C::Inventory;
        if(!StashContext(true))return false;
        work.captured=true;return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool CaptureExplicitCube() noexcept {
    __try {
        if(!Guard())return false;
        if(work.source.container!=C::Cube || !Mode(0x18))return true;
        work.tab=D2R::Native::GetActiveStashTabIndex(context->exeBase);
        work.cubeInStash=EmbeddedCube(true,work.tab,work.source.container);
        if(!work.cubeInStash)return true;
        if(!StashContext(true))return false;
        work.captured=true;return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex);context=ctx;ready=false;active=false;fence.Cancel();work={};
    if(!ctx || ctx->QueryService(&items)!=D2RL::ServiceQueryResult::Success || !items || !items->getItemInfo ||
       ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success || !inventory || !inventory->getLocalPlayer || !inventory->forEachInventoryItem ||
       ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success || !threads || !threads->runOnUiThread ||
       !QolCore::VerifyFileHash(GetModuleHandleW(L"D2RCore.dll"),QolNativeProfile::CoreHash) || !Guard())return false;
    const D2RL::LifecycleService* lifecycle{};
    if(ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) || !lifecycle->registerGameplayEventListener)return false;
    for(auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize,0,kind,0,SessionChanged,nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        if(lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success)return false;
    }
    ready=true;ctx->LogInfo("[QOL/ClientTransfer] Native remote single-transfer profile admitted.");return true;
}
void Shutdown() noexcept {std::lock_guard lock(mutex);ready=false;active=false;fence.Cancel();context=nullptr;work={};}
bool Busy() noexcept {return active.load();}
static BatchDepositResult BatchDepositUnsafe(const D2RL::PluginContext* ctx,const Info& expected) noexcept {
    __try {
        if(ctx!=context || !ready || active || expected.container!=C::Inventory ||
           !Guard() || !DepositGuard() || !Mode(0x18) || At<bool(__fastcall*)()>(0x23b980)())return BatchDepositResult::Refused;
        Info live{.structSize=D2RL::Items::ItemInfoSize};
        if(items->getItemInfo(ctx,expected.handle,&live)!=D2RL::Items::Result::Success || !Source(expected,live))return BatchDepositResult::Refused;
        auto player=D2R::Native::GetLocalPlayerUnit(ctx->exeBase);
        auto item=static_cast<uint32_t*>(At<void*(__fastcall*)(uint32_t,uint32_t)>(0x9a5d0)(live.runtimeId,4));
        if(!player || !item || item[0]!=4 || item[1]!=live.classId || item[2]!=live.runtimeId || item[3]!=0 ||
           At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(item)!=live.code ||
           At<uint8_t(__fastcall*)(const void*)>(0x36cfe0)(item)!=0 || At<int(__fastcall*)(const void*)>(0x1c7360)(item) ||
           At<void*(__fastcall*)()>(0x14f1e0)() || At<int(__fastcall*)()>(0x14f210)()==5)return BatchDepositResult::Refused;
        if(!At<bool(__fastcall*)(const void*)>(0x15a0b0)(item))return BatchDepositResult::Ineligible;
        auto destination=At<void*(__fastcall*)(void*)>(0x46da50)(player);
        if(!destination)return BatchDepositResult::Refused;
        alignas(8) unsigned char placement[16]{};
        (void)At<D2R::Native::TransferItemToInventoryPageFn>(0x15f8b0)(item,destination,4,0,true,placement);
        At<D2R::Native::FinishInventoryInteractionFn>(0x1a0780)(3,nullptr,0,0,false);
        return BatchDepositResult::Submitted;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ready=false; // Caller stops the entire batch, including a possibly sent request.
        return BatchDepositResult::Refused;
    }
}
BatchDepositResult BatchDeposit(const D2RL::PluginContext* ctx,const Info& expected) noexcept {
    std::lock_guard lock(mutex);
    return BatchDepositUnsafe(ctx,expected);
}
bool Request(const D2RL::PluginContext* ctx,const Info& info,bool cube) noexcept {
    std::lock_guard lock(mutex);
    if(ctx!=context || !ready || !Storage(info.container))return false;
    if(active)return true;
    work={};work.source=info;
    if(cube) {
        work.destination=info.container==C::Cube?C::Inventory:C::Cube;
        // Explicit LB+Y remains a Cube/Inventory move even in the embedded view.
        if(!CaptureExplicitCube())return false;
    }
    else {
        // Ordinary stash routing is captured on the UI thread before queuing.
        if(!CaptureStashRequest())return false;
    }
    if(!Legal(info.container,work.destination,info.code))return false;
    active=true;fence.Begin(GetTickCount64());
    Continue();return active.load();
}
}
