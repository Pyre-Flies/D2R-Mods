#include "bulk_stash.h"
#include "bulk_stash_policy.h"
#include "native_d2r.h"
#include "belt_signatures.h"
#include "materials_signatures.h"
#include "client_transfer.h"
#include "materials_actions.h"
#include "shared_sdk_selection.h"
#include <mutex>
#include <cstdio>
#include <cstring>
namespace QolBulkStash {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};const D2RL::ItemService* items{};
const D2RL::InventoryService* inventory{};const D2RL::ThreadService* threads{};
Available available{};bool active{},collected{},waiting{};uintptr_t generation{};
bool remote{},sessionEvents{};uint32_t tab{UINT32_MAX};QolSharedSdk::Selection selection{};
D2RL::PlayerHandle player{};Plan plan{};ULONGLONG started{},submittedAt{};const char* stopReason{};
// Full inherited eligibility body from reviewed runtime capture; see BULK-STASH-REV37.md.
constexpr unsigned char eligible[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xb9,0x18,0x00,0x00,0x00,0xe8,0x3d,0x44,0xf7,0xff,0x84,0xc0,0x74,0x3e,0xe8,0xb4,0x12,0xf3,0xff,0x0f,0xb6,0xc8,0xba,0x02,0x00,0x00,0x00,0xe8,0xe7,0x65,0x25,0x00,0x84,0xc0,0x74,0x28,0x41,0xb8,0xf8,0x04,0x00,0x00,0x48,0x8d,0x15,0x46,0x31,0xb7,0x01,0x48,0x8b,0xcb,0xe8,0x6e,0xf7,0x1e,0x00,0x8b,0xc8,0xe8,0x27,0x52,0x00,0x00,0x84,0xc0,0x74,0x08,0xb0,0x01,0x48,0x83,0xc4,0x20,0x5b,0xc3,0x32,0xc0,0x48,0x83,0xc4,0x20,0x5b,0xc3};
bool Match(uintptr_t rva,const unsigned char* bytes,size_t size) noexcept {
    unsigned char actual[256]{};SIZE_T got{};
    return size<=sizeof(actual) && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(context->exeBase+rva),actual,size,&got) && got==size && !std::memcmp(actual,bytes,size);
}
bool Guard() noexcept {
    using namespace D2R::Native;
    return context && context->exeBase &&
        Match(CanDepositToAdvancedStashRva,eligible,sizeof(eligible)) &&
        Match(GetLocalDataContextRva,QolBelt::Signatures::LocalContext,sizeof(QolBelt::Signatures::LocalContext)) &&
        Match(GetLocalPlayerRva,QolBelt::Signatures::LocalPlayer,sizeof(QolBelt::Signatures::LocalPlayer)) &&
        Match(GetAdvancedStashDestinationRva,QolMaterials::Signatures::CurrentOwner,sizeof(QolMaterials::Signatures::CurrentOwner)) &&
        Match(TransferItemToInventoryPageRva,QolBelt::Signatures::TransferPage,sizeof(QolBelt::Signatures::TransferPage)) &&
        Match(FinishInventoryInteractionRva,QolBelt::Signatures::FinishInteraction,sizeof(QolBelt::Signatures::FinishInteraction));
}
void Finish(const char* reason) noexcept {
    char line[256];std::snprintf(line,sizeof(line),"[QOL/BulkStash] %s snapshot=%u submitted=%u removedFromInventory=%u skipped=%u.",reason,plan.count,plan.submitted,plan.removed,plan.skipped);
    if(context)context->LogInfo(line);active=false;waiting=false;
}
void __cdecl Ui(const D2RL::PluginContext*,void*) noexcept;
void __cdecl Game(const D2RL::PluginContext*,void*) noexcept;
void Remote(const D2RL::PluginContext*) noexcept;
bool Current(const D2RL::PluginContext* ctx,void* token) noexcept {return active && ctx==context && reinterpret_cast<uintptr_t>(token)==generation;}
void QueueUi() noexcept {
    if(threads->runOnUiThread(context,Ui,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)Finish("UI scheduling unavailable");
}
void __cdecl Ui(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);if(!Current(ctx,token))return;
    if(!available || !available() || GetTickCount64()-started>30000){Finish("Stash closed, conflicting action, or request expired");return;}
    if(remote){Remote(ctx);return;}
    const auto result=threads->runOnGameThread(ctx,Game,token);
    if(result==D2RL::Threads::Result::Unavailable && sessionEvents){remote=true;Remote(ctx);return;}
    if(result!=D2RL::Threads::Result::Success)Finish("Game scheduling unavailable");
}
void Remote(const D2RL::PluginContext* ctx) noexcept {
    if(!Guard() || !ctx->CheckExpectedBytes(0x23b980,QolMaterials::Signatures::PreviousSeason,sizeof(QolMaterials::Signatures::PreviousSeason))) {
        Finish("Remote profile unavailable");return;
    }
    const auto currentTab=QolMaterials::SelectedStashTab(ctx);
    if(currentTab>4 || reinterpret_cast<bool(__fastcall*)()>(ctx->exeBase+0x23b980)()) {Finish("Remote stash closed or previous season selected");return;}
    const auto selected=currentTab==1?QolSharedSdk::Selected(ctx->exeBase):QolSharedSdk::Selection{};
    if((collected && currentTab!=tab) || (currentTab==1 && (!selected.valid || selected.previousSeason ||
       (collected && !QolSharedSdk::SameSelection(selection,selected))))) {Finish("Remote stash page changed");return;}
    D2RL::PlayerHandle current{};
    if(inventory->getLocalPlayer(ctx,&current)!=D2RL::Inventory::Result::Success || !current || (player && player!=current)) {Finish("Remote player changed");return;}
    player=current;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory),0};
    if(!collected) {
        if(inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext*,const Info* i,void*) noexcept {
            if(i)plan.Observe(*i);return D2RL::Inventory::IterationAction::Continue;
        },nullptr)!=D2RL::Inventory::Result::Success || plan.overflow) {Finish("Remote snapshot unavailable");return;}
        tab=currentTab;selection=selected;collected=true;
    }
    if(plan.next>=plan.count){Finish("Remote batch ended; source removals observed, advanced counters unverified");return;}
    const auto& expected=plan.items[plan.next];
    struct Search {uint32_t id;Info live{};bool found{};} search{expected.runtimeId};
    if(inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext*,const Info* i,void* user) noexcept {
        auto& s=*static_cast<Search*>(user);if(i && i->runtimeId==s.id){s.live=*i;s.found=true;}
        return D2RL::Inventory::IterationAction::Continue;
    },&search)!=D2RL::Inventory::Result::Success) {Finish("Remote observation unavailable; no retry");return;}
    switch(CheckRemote(expected,search.found?&search.live:nullptr,waiting,GetTickCount64()-submittedAt>=1500)) {
    case RemoteStep::Removed: ++plan.removed;++plan.next;waiting=false;QueueUi();return;
    case RemoteStep::Wait: QueueUi();return;
    case RemoteStep::Stop: Finish("Remote source changed or deposit not observed; no retry");return;
    case RemoteStep::Ready: break;
    }
    const auto result=QolClientTransfer::BatchDeposit(ctx,search.live);
    if(result==QolClientTransfer::BatchDepositResult::Refused){Finish("Remote deposit refused or faulted; batch stopped without retry");return;}
    if(result==QolClientTransfer::BatchDepositResult::Ineligible){++plan.skipped;++plan.next;QueueUi();return;}
    plan.Submitted(plan.next);waiting=true;submittedAt=GetTickCount64();QueueUi();
}
void __cdecl Session(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent*,void*) noexcept {
    std::lock_guard lock(mutex);++generation;
    if(active)Finish("Session changed; batch cancelled");
}
struct Deposit {bool inspected{},eligible{},submitted{};};
void __cdecl Native(const D2RL::PluginContext* ctx,void* item,void* user) noexcept {
    auto& d=*static_cast<Deposit*>(user);if(!item)return;
    d.inspected=true;d.eligible=D2R::Native::CanDepositToAdvancedStash(ctx->exeBase,item);
    if(d.eligible)d.submitted=D2R::Native::DepositToAdvancedStash(ctx->exeBase,item,D2R::Native::GetLocalPlayerUnit(ctx->exeBase));
}
void __cdecl Game(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);if(!Current(ctx,token))return;
    if(GetTickCount64()-started>30000 || !Guard()){Finish("Deadline or native contract mismatch; no fallback");return;}
    D2RL::PlayerHandle current{};
    if(inventory->getLocalPlayer(ctx,&current)!=D2RL::Inventory::Result::Success || !current || (player && player!=current)){Finish("Player/session changed");return;}
    player=current;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory),0};
    if(!collected) {
        const auto result=inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext*,const Info* i,void*) noexcept {
            if(i)plan.Observe(*i);return D2RL::Inventory::IterationAction::Continue;
        },nullptr);
        if(result!=D2RL::Inventory::Result::Success || plan.overflow){Finish("Snapshot unavailable or oversized; no transfers");return;}
        collected=true;
    }
    if(waiting) {
        plan.BeginVerification();
        const auto result=inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext*,const Info* i,void*) noexcept {
            if(i)plan.ObserveRemaining(*i);return D2RL::Inventory::IterationAction::Continue;
        },nullptr);
        if(result!=D2RL::Inventory::Result::Success){Finish("Batch verification unavailable; no retry");return;}
        if(plan.Remaining()==0){Finish(stopReason?stopReason:"Complete");return;}
        if(GetTickCount64()-submittedAt>=1000){Finish("Batch partially unconfirmed; no retry");return;}
        QueueUi();return;
    }
    // All eligible submissions occur in this one authoritative game update.
    // Re-resolve each identity after prior native mutations; never retain native pointers.
    for(;plan.next<plan.count;++plan.next) {
        const auto& expected=plan.items[plan.next];
        struct Search {uint32_t id;Info live{};bool found{};} search{expected.runtimeId};
        const auto result=inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext*,const Info* i,void* u) noexcept {
            auto& s=*static_cast<Search*>(u);if(i && i->runtimeId==s.id){s.live=*i;s.found=true;return D2RL::Inventory::IterationAction::Stop;}
            return D2RL::Inventory::IterationAction::Continue;
        },&search);
        if(result!=D2RL::Inventory::Result::Success){stopReason="Submission stopped: inventory read failed";break;}
        if(Validate(expected,search.found?&search.live:nullptr)!=Check::Ready){stopReason="Submission stopped: snapshot item changed";break;}
        Deposit d{};
        if(items->editNativeItem(ctx,search.live.handle,Native,&d)!=D2RL::Items::Result::Success || !d.inspected){stopReason="Submission stopped: native access failed";break;}
        if(!d.eligible){++plan.skipped;continue;}
        if(!d.submitted){stopReason="Submission stopped: eligible deposit refused";break;}
        plan.Submitted(plan.next);
    }
    if(!plan.submitted){Finish(stopReason?stopReason:"No eligible items");return;}
    waiting=true;submittedAt=GetTickCount64();QueueUi();
}
}
void Initialize(const D2RL::PluginContext* ctx,Available check) noexcept {
    std::lock_guard lock(mutex);context=ctx;available=check;
    sessionEvents=false;
    if(ctx){
        if(ctx->QueryService(&items)!=D2RL::ServiceQueryResult::Success)items=nullptr;
        if(ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success)inventory=nullptr;
        if(ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success)threads=nullptr;
        const D2RL::LifecycleService* lifecycle{};
        if(ctx->QueryService(&lifecycle)==D2RL::ServiceQueryResult::Success &&
           D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) && lifecycle->registerGameplayEventListener) {
            sessionEvents=true;
            for(auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
                const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize,0,kind,0,Session,nullptr};
                D2RL::Lifecycle::ListenerHandle handle{};
                if(lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success)sessionEvents=false;
            }
        }
    }
}
bool Busy() noexcept {std::lock_guard lock(mutex);return active;}
bool Request() noexcept {
    std::lock_guard lock(mutex);
    if(!context || !available || !available())return false;
    if(active && GetTickCount64()-started<=30000)return true;
    if(!items || !items->editNativeItem || !inventory || !inventory->getLocalPlayer || !inventory->forEachInventoryItem || !threads || !threads->runOnUiThread || !threads->runOnGameThread)return false;
    if(!Guard()){context->LogWarn("[QOL/BulkStash] Native deposit contract unavailable; batch disabled.");return true;}
    ++generation;active=true;remote=false;tab=UINT32_MAX;selection={};collected=waiting=false;player=0;plan={};stopReason=nullptr;started=GetTickCount64();QueueUi();return true;
}
void Shutdown() noexcept {std::lock_guard lock(mutex);++generation;active=false;context=nullptr;available=nullptr;items=nullptr;inventory=nullptr;threads=nullptr;}
}
