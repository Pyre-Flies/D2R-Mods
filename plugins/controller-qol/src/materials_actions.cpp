#include "materials_actions.h"
#include "materials_policy.h"
#include "materials_native_contract.h"
#include "materials_signatures.h"
#include "plugin_compatibility.h"
#include "native_d2r.h"
#include "storage_focus.h"
#include <windows.h>
#include <mutex>
#include <cstdio>

namespace QolMaterials {
namespace {
const D2RL::InventoryService* inventory{};
const D2RL::ThreadService* threads{};
std::mutex mutex;
bool ready{}, active{}, queued{}, refill{}, waiting{}, seasonSet{}, expectedPrevious{};
uint32_t code{}, expectedTab{}, selectedRefillCode{};
unsigned confirmed{}, family{};
int64_t before{};
ULONGLONG deadline{}, queuedAt{};
D2RL::PlayerHandle player{};
Destination destination{Destination::Inventory};
D2RL::Items::ItemInfo requested{};
uint64_t generation{};
ULONGLONG requestedAt{};

template<class T> T At(const D2RL::PluginContext* ctx, uintptr_t rva) noexcept {
    return reinterpret_cast<T>(ctx->exeBase+rva);
}
void Reset() noexcept {
    ++generation;destination=Destination::Inventory;requested={};
    requestedAt=GetTickCount64();
    active=queued=refill=waiting=seasonSet=false; confirmed=family=0; player=0; expectedTab=0; selectedRefillCode=0;
}
bool Validate(const D2RL::PluginContext* ctx) noexcept {
    for (const auto& site: Signatures::All) if (!(site.rva==0x3862D0 ? QolCompat::ValidateBeltEntry(ctx->exeBase+site.rva) : ctx->CheckExpectedBytes(site.rva,site.bytes,site.size))) {
        char message[160];
        std::snprintf(message,sizeof(message),"[QOL/Materials] Native profile mismatch at RVA 0x%llX; advanced withdrawals disabled.",
            static_cast<unsigned long long>(site.rva));
        ctx->LogWarn(message);
        return false;
    }
    return true;
}
void Finish(const D2RL::PluginContext* ctx, const char* why) noexcept {
    char message[192];
    std::snprintf(message,sizeof(message),"[QOL/Materials] %s; confirmed=%u.",why,confirmed);
    ctx->LogInfo(message); Reset();
}
// All native widget/item pointers in this module are resolved and used on the
// UI thread in the same callback; none are stored in the cross-thread mailbox.
struct Slot {
    void* widget{}; void* item{}; uint32_t tab{0xFFFFFFFF};
    const char* stage{"stash-closed"};
};
void TraceResolve(const D2RL::PluginContext* ctx,const Slot& slot,uint32_t itemCode,const char* action) noexcept {
    char message[240];
    std::snprintf(message,sizeof(message),"[QOL/Materials] Resolve action=%s panel=BankExpansionLayout stage=%s tab=%u code=0x%08X widget=%p item=%p.",
        action,slot.stage,slot.tab,itemCode,slot.widget,slot.item);
    ctx->LogInfo(message);
}
Slot Resolve(const D2RL::PluginContext* ctx, uint32_t itemCode) noexcept {
    Slot result{};
    __try {
        if (!At<bool(__fastcall*)(uint32_t)>(ctx,0xCE500)(0x18)) return result;
        // Layout root name, not its BankPanel type. Native lookup compares widget+8.
        result.stage="panel-missing";
        void* bank=At<D2R::Native::FindTopLevelPanelByNameFn>(ctx,0x846170)("BankExpansionLayout");
        if (!bank) return result;
        result.tab=At<uint8_t(__fastcall*)(void*)>(ctx,0x23AF50)(bank);
        result.stage="ordinary-or-unknown-tab";
        const char* category=Category(result.tab);
        char name[5]{};
        if (!category || !WidgetName(itemCode,name)) return result;
        const auto find=At<D2R::Native::FindChildWidgetByNameFn>(ctx,0x856220);
        result.stage="category-missing";
        void* container=find(bank,category);
        if (!container) return result;
        result.stage="slot-missing";
        void* widget=find(container,name);
        if (!widget) return result;
        // Verified AdvancedStashSlotWidget getter in vtable +0xC8.
        result.stage="slot-type-mismatch";
        auto table=*static_cast<uintptr_t**>(widget);
        if (!table || table[0xC8/sizeof(uintptr_t)]!=ctx->exeBase+0x2CE900) return result;
        result.stage="binding-missing-or-code-mismatch";
        void* item=At<void*(__fastcall*)(void*)>(ctx,0x2CE900)(widget);
        if (!item || At<uint32_t(__fastcall*)(void*)>(ctx,0x36EF50)(item)!=itemCode) return result;
        result.widget=widget; result.item=item; result.stage="bound";
    } __except(EXCEPTION_EXECUTE_HANDLER) { result.widget=nullptr; result.item=nullptr; result.stage="native-read-exception"; }
    return result;
}
bool FocusMatches(const D2RL::PluginContext* ctx, const Slot& slot, const D2RL::Items::ItemInfo& info) noexcept {
    if (!slot.widget || !slot.item) return false;
    __try {
        const auto id=At<uint32_t(__fastcall*)(void*)>(ctx,0x34A330);
        if (id(slot.item)==info.runtimeId) return true;
        // Native widget also uses +0x600 as its display item (0x2CF4BA).
        void* display=*reinterpret_cast<void**>(static_cast<char*>(slot.widget)+0x600);
        return display && id(display)==info.runtimeId &&
            At<uint32_t(__fastcall*)(void*)>(ctx,0x36EF50)(display)==info.code;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
enum class Submission { Submitted, EmptyOrFull, Failed };
bool HasCarriedCube(const D2RL::PluginContext* ctx) noexcept {
    bool found=false;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory),0};
    return inventory->forEachInventoryItem(ctx,player,&filter,
        [](const D2RL::PluginContext*,const D2RL::Items::ItemInfo* item,void* user) noexcept {
            if(item && QolIdentify::Code(item->code,D2RL::Items::MakeItemCode("box"))) {
                *static_cast<bool*>(user)=true;return D2RL::Inventory::IterationAction::Stop;
            }
            return D2RL::Inventory::IterationAction::Continue;
        },&found)==D2RL::Inventory::Result::Success && found;
}
Submission Submit(const D2RL::PluginContext* ctx, const Slot& slot, Destination destination) noexcept {
    if (!slot.widget || !slot.item) return Submission::EmptyOrFull;
    __try {
        if (!At<uint8_t(__fastcall*)(void*)>(ctx,0x46D9A0)(slot.item)) {
            ctx->LogInfo("[QOL/Materials] Proxy has no withdrawable stock.");
            return Submission::EmptyOrFull;
        }
        if (destination==Destination::Inventory) {
            // R8B=0 means inventory PAGE here; native wrapper maps it to
            // withdrawal DESTINATION 1, selects season owner, and finishes UI.
            if (!SubmitInventory(At<WithdrawWidgetFn>(ctx,0x2CF680),slot.widget)) return Submission::Failed;
        } else if(destination==Destination::Cube) {
            if(!HasCarriedCube(ctx))return Submission::EmptyOrFull;
            if(!SubmitCube(At<WithdrawWidgetFn>(ctx,0x2CF680),slot.widget))return Submission::Failed;
        } else {
            void* local=D2R::Native::GetLocalPlayerUnit(ctx->exeBase);
            if (!local) return Submission::Failed;
            void* inv=At<D2R::Native::GetUnitInventoryFn>(ctx,0x34A360)(local);
            int32_t target=-1;
            if (!inv || !At<D2R::Native::GetFreeBeltSlotFn>(ctx,0x3862D0)(inv,slot.item,&target,true) || target<0 || target>=16) {
                ctx->LogInfo("[QOL/Materials] Native belt planner found no compatible free slot.");
                return Submission::EmptyOrFull;
            }
            const bool previous=At<bool(__fastcall*)()>(ctx,0x23B980)();
            void* owner=At<void*(__fastcall*)(void*)>(ctx,previous?0x46D9C0:0x46DA50)(local);
            if (!owner) return Submission::Failed;
            if (!SubmitBelt(At<WithdrawOneFn>(ctx,0x159B30),slot.item,owner)) return Submission::Failed;
            const int32_t finishMode=At<int32_t(__fastcall*)(void*)>(ctx,0x1C3420)(slot.item);
            At<D2R::Native::FinishInventoryInteractionFn>(ctx,0x1A0780)(finishMode,nullptr,0,0,false);
        }
        return Submission::Submitted;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return Submission::Failed; }
}
bool Quantity(const D2RL::PluginContext* ctx, Destination destination, int64_t& value) noexcept {
    struct Sum { uint32_t code; int64_t quantity; } sum{code,0};
    const auto container=DestinationContainer(destination);
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(container),0};
    const auto status=inventory->forEachInventoryItem(ctx,player,&filter,
        [](const D2RL::PluginContext*,const D2RL::Items::ItemInfo* info,void* user) noexcept {
            auto& s=*static_cast<Sum*>(user);
            if (info->code==s.code) s.quantity+=info->quantity>0?info->quantity:1;
            return D2RL::Inventory::IterationAction::Continue;
        },&sum);
    value=sum.quantity;
    return status==D2RL::Inventory::Result::Success;
}
void __cdecl Tick(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(reinterpret_cast<uintptr_t>(token)!=generation)return;
    queued=false;
    if (!ready || !active) return;
    if(!refill && !waiting && GetTickCount64()-requestedAt>750) {Finish(ctx,"Cancelled: single withdrawal expired before submission");return;}
    D2RL::PlayerHandle current{};
    if (inventory->getLocalPlayer(ctx,&current)!=D2RL::Inventory::Result::Success || !current || (player && player!=current)) {
        Finish(ctx,"Cancelled: player/session changed"); return;
    }
    player=current;
    D2RL::ItemHandle cursor{};
    const auto cursorResult=inventory->getCursorItem(ctx,player,&cursor);
    if ((cursorResult!=D2RL::Inventory::Result::Success && cursorResult!=D2RL::Inventory::Result::NotFound) || cursor) {
        Finish(ctx,"Stopped: cursor occupied or cursor state unavailable"); return;
    }
    if (!waiting && refill) code=RefillCode(selectedRefillCode,family);
    const auto slot=Resolve(ctx,code);
    if (!waiting) TraceResolve(ctx,slot,code,refill?"refill":"withdraw");
    if (slot.tab!=expectedTab) { Finish(ctx,"Stopped: stash closed/tab changed"); return; }
    const bool previous=At<bool(__fastcall*)()>(ctx,0x23B980)();
    if (seasonSet && previous!=expectedPrevious) { Finish(ctx,"Stopped: stash season changed"); return; }
    expectedPrevious=previous; seasonSet=true;
    if (waiting) {
        int64_t after{};
        if (!Quantity(ctx,destination,after)) { Finish(ctx,"Stopped: SDK observation unavailable"); return; }
        if (Confirmed(before,after)) {
            ++confirmed; waiting=false;
            char message[180];
            std::snprintf(message,sizeof(message),"[QOL/Materials] Observed destination increase code=0x%08X destination=%s before=%lld after=%lld.",
                code,DestinationName(destination),static_cast<long long>(before),static_cast<long long>(after));
            ctx->LogInfo(message);
            if (!refill || !CanContinue(confirmed)) Finish(ctx,"Withdrawal complete");
            return;
        }
        if (GetTickCount64()>=deadline) Finish(ctx,"Stopped: withdrawal not observed; no retry sent");
        return;
    }
    if (!Validate(ctx)) { ready=false; Finish(ctx,"Disabled: contract changed"); return; }
    if(!refill && (!FocusMatches(ctx,slot,requested) || !QolStorageFocus::Matches(ctx,requested))) {
        Finish(ctx,"Stopped: controller selection changed before withdrawal");return;
    }
    if (!Quantity(ctx,destination,before)) { Finish(ctx,"Stopped: SDK baseline unavailable"); return; }
    const auto result=Submit(ctx,slot,destination);
    if (result==Submission::EmptyOrFull) {
        if (refill && RefillFallback(selectedRefillCode,family)) { ++family; return; }
        Finish(ctx,"No further eligible stock/belt room"); return;
    }
    if (result==Submission::Failed) { Finish(ctx,"Stopped: native withdrawal failed"); return; }
    waiting=true; deadline=GetTickCount64()+1500;
    char message[128];
    std::snprintf(message,sizeof(message),"[QOL/Materials] Submitted code=0x%08X destination=%s; awaiting observation.",code,DestinationName(destination));
    ctx->LogInfo(message);
}
void __cdecl Session(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent*,void*) noexcept {
    std::lock_guard lock(mutex); Reset();
}
}
bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex); Reset(); ready=false;
    const D2RL::LifecycleService* lifecycle{};
    if (ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success ||
        !D2RL::HasInventoryServiceField(inventory,D2RL::InventoryServiceRequiredSize) || !inventory->getLocalPlayer || !inventory->getCursorItem || !inventory->forEachInventoryItem ||
        ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(threads,D2RL::ThreadServiceRequiredSize) || !threads->runOnUiThread ||
        ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success ||
        !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) || !lifecycle->registerGameplayEventListener || !Validate(ctx)) return false;
    for (auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize,0,kind,0,Session,nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        if (lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success) return false;
    }
    ready=true; ctx->LogInfo("[QOL/Materials] Native advanced-stash UI profile admitted."); return true;
}
void Shutdown() noexcept { std::lock_guard lock(mutex); ready=false; Reset(); }
bool Busy() noexcept { std::lock_guard lock(mutex); return active; }
static bool TryWithdraw(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info,Destination target) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !AdvancedProxyCandidate(info.container)) return false;
    if (!Validate(ctx)) { ready=false; return false; }
    const auto slot=Resolve(ctx,info.code);
    TraceResolve(ctx,slot,info.code,"focus");
    if (!FocusMatches(ctx,slot,info) || !QolStorageFocus::Matches(ctx,info)) {
        if (slot.tab==0 || slot.tab==1) return false;
        char message[160];
        std::snprintf(message,sizeof(message),"[QOL/Materials] Focus rejected runtimeId=%u container=%u; unresolved/advanced stash must not use ordinary fallback.",
            info.runtimeId,static_cast<unsigned>(info.container));
        ctx->LogWarn(message);
        ctx->LogWarn("[QOL/Materials] Focus does not match a current advanced-stash binding; ordinary same-code fallback suppressed.");
        return true;
    }
    if(target==Destination::Belt && (slot.tab!=3 ||
       (info.code!=RejuvenationCode(0) && info.code!=RejuvenationCode(1))))return true;
    if (active) return true;
    Reset(); active=true; code=info.code; expectedTab=slot.tab;destination=target;requested=info;
    expectedPrevious=At<bool(__fastcall*)()>(ctx,0x23B980)();seasonSet=true;
    // Tick is queued later; no pointer leaves this UI callback.
    return true;
}
bool TryWithdrawFocused(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info) noexcept {return TryWithdraw(ctx,info,Destination::Inventory);}
bool TryWithdrawToBelt(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info) noexcept {return TryWithdraw(ctx,info,Destination::Belt);}
bool TryWithdrawToCube(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info) noexcept {return TryWithdraw(ctx,info,Destination::Cube);}
bool RequestSingleBelt(const D2RL::Items::ItemInfo& info) noexcept {
    std::lock_guard lock(mutex);
    if(!ready || !AdvancedProxyCandidate(info.container) ||
       (info.code!=RejuvenationCode(0) && info.code!=RejuvenationCode(1)))return false;
    if(active)return true;
    Reset();active=true;code=info.code;expectedTab=3;destination=Destination::Belt;requested=info;
    return true;
}
uint32_t SelectedStashTab(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !Validate(ctx)) return 0xFFFFFFFF;
    return Resolve(ctx,0).tab;
}
bool RequestFocusedRefill(const D2RL::PluginContext* ctx,const D2RL::Items::ItemInfo& info) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !Validate(ctx)) return false;
    const auto slot=Resolve(ctx,info.code);
    if (slot.tab!=3 || !FocusMatches(ctx,slot,info)) return false;
    if (active) return true;
    Reset(); active=refill=true; expectedTab=3; selectedRefillCode=info.code;destination=Destination::Belt;
    TraceResolve(ctx,slot,info.code,"focused-refill");
    return true;
}
void RequestRefill() noexcept {
    std::lock_guard lock(mutex);
    if (!ready || active) return;
    Reset(); active=refill=true; expectedTab=3;destination=Destination::Belt;
}
void Pump(const D2RL::PluginContext* ctx) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || !active) return;
    if (queued) {
        if (GetTickCount64()-queuedAt>5000) Finish(ctx,"Cancelled: UI task unavailable");
        return;
    }
    queued=true; queuedAt=GetTickCount64();
    if (threads->runOnUiThread(ctx,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success) Finish(ctx,"Cancelled: UI scheduling refused");
}
}
