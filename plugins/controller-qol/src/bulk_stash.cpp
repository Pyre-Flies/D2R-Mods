#include "bulk_stash.h"
#include "bulk_stash_policy.h"
#include "native_d2r.h"
#include "belt_signatures.h"
#include "materials_signatures.h"
#include <mutex>
#include <cstdio>
#include <cstring>
namespace QolBulkStash {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};const D2RL::ItemService* items{};
const D2RL::InventoryService* inventory{};const D2RL::ThreadService* threads{};
Available available{};bool active{},collected{},waiting{};uintptr_t generation{};
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
bool Current(const D2RL::PluginContext* ctx,void* token) noexcept {return active && ctx==context && reinterpret_cast<uintptr_t>(token)==generation;}
void QueueUi() noexcept {
    if(threads->runOnUiThread(context,Ui,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)Finish("UI scheduling unavailable");
}
void __cdecl Ui(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);if(!Current(ctx,token))return;
    if(!available || !available() || GetTickCount64()-started>30000){Finish("Stash closed, conflicting action, or request expired");return;}
    if(threads->runOnGameThread(ctx,Game,token)!=D2RL::Threads::Result::Success)Finish("Game scheduling unavailable");
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
    if(ctx){
        if(ctx->QueryService(&items)!=D2RL::ServiceQueryResult::Success)items=nullptr;
        if(ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success)inventory=nullptr;
        if(ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success)threads=nullptr;
    }
}
bool Busy() noexcept {std::lock_guard lock(mutex);return active;}
bool Request() noexcept {
    std::lock_guard lock(mutex);
    if(!context || !available || !available())return false;
    if(active && GetTickCount64()-started<=30000)return true;
    if(!items || !items->editNativeItem || !inventory || !inventory->getLocalPlayer || !inventory->forEachInventoryItem || !threads || !threads->runOnUiThread || !threads->runOnGameThread)return false;
    if(!Guard()){context->LogWarn("[QOL/BulkStash] Native deposit contract unavailable; batch disabled.");return true;}
    ++generation;active=true;collected=waiting=false;player=0;plan={};stopReason=nullptr;started=GetTickCount64();QueueUi();return true;
}
void Shutdown() noexcept {std::lock_guard lock(mutex);++generation;active=false;context=nullptr;available=nullptr;items=nullptr;inventory=nullptr;threads=nullptr;}
}
