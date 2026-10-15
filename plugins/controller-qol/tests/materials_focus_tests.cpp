// Exercise the production counter resolver/mailbox with private native/SDK
// fixtures. No game process, save, network request, or installed DLL is used.
#include "../src/materials_actions.cpp"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

namespace Fixture {
using C=D2RL::Items::ItemContainer;
unsigned checks{},submissions{};
bool guard=true,open=true,previous=false,stock=true,cube=true,quantityAvailable=true;
uint32_t tab=2,itemCode=D2RL::Items::MakeItemCode("gcy");
D2RL::PlayerHandle owner=7;
D2RL::ItemHandle cursor{};
int64_t quantity{};
uint8_t lastPage{};
unsigned char *image{},manager[0x200]{},focus[0x200]{},widget[0x610]{},other[0x610]{};
uint32_t bound[16]{4,577,28},display[16]{4,577,0};
uintptr_t vtable[32]{};
int bank{},category{};
bool wrongSlot{};
bool refuseSchedule{};
bool beltGuard=true;
unsigned nativeReads{};
D2RL::InventoryService inventory{};
D2RL::ThreadService threads{};
D2RL::LifecycleService lifecycle{};
struct Task {D2RL::Threads::Callback fn{};void* user{};};
Task task{};
void Check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
void Jump(uintptr_t rva,const void* fn) {
    auto* page=image+(rva&~uintptr_t(4095));
    Check(VirtualAlloc(page,4096,MEM_COMMIT,PAGE_EXECUTE_READWRITE)!=nullptr,"private fixture page");
    const unsigned char jump[]={0xff,0x25,0,0,0,0};
    std::memcpy(image+rva,jump,sizeof(jump));std::memcpy(image+rva+6,&fn,sizeof(fn));
}
bool __fastcall Open(uint32_t mode){++nativeReads;return mode==0x18 && open;}
void* __fastcall Panel(const char* name){return std::strcmp(name,"BankExpansionLayout")==0?&bank:nullptr;}
uint8_t __fastcall Tab(void*){return static_cast<uint8_t>(tab);}
void* __fastcall Child(void* parent,const char* name) {
    if(parent==&bank)return QolMaterials::Category(tab) && std::strcmp(name,QolMaterials::Category(tab))==0?&category:nullptr;
    char expected[5]{};
    if(parent==&category && QolMaterials::WidgetName(itemCode,expected) && std::strcmp(name,expected)==0)
        return wrongSlot?other:widget;
    return nullptr;
}
void* __fastcall Bound(void* slot){return *reinterpret_cast<void**>(static_cast<unsigned char*>(slot)+0x608);}
uint32_t __fastcall Code(const void*){return itemCode;}
uint32_t __fastcall Id(const void* item){return static_cast<const uint32_t*>(item)[2];}
bool __fastcall Previous(){return previous;}
uint8_t __fastcall Stock(void*){return stock?1:0;}
void __fastcall Withdraw(void* slot,const int32_t* cell,uint8_t page) {
    Check(slot==widget && Bound(slot)==bound,"withdraw uses bound slot, never display proxy");
    Check(cell && cell[0]==0 && cell[1]==0 && (page==0 || page==3),"reviewed page/coordinate ABI");
    ++submissions;lastPage=page;
}
void Flush(const D2RL::PluginContext& ctx){const auto queued=task;task={};if(queued.fn)queued.fn(&ctx,queued.user);}
void Pump(const D2RL::PluginContext& ctx){QolMaterials::Pump(&ctx,true);Flush(ctx);}
void Reset(const D2RL::PluginContext& ctx) {
    QolMaterials::Session(nullptr,nullptr,nullptr);task={};
    QolMaterials::ready=true;refuseSchedule=false;beltGuard=true;
    guard=open=stock=cube=quantityAvailable=true;previous=wrongSlot=false;
    owner=7;cursor=0;tab=2;itemCode=D2RL::Items::MakeItemCode("gcy");quantity=0;
    bound[0]=4;bound[2]=28;
    widget[0x50]=widget[0x51]=1;vtable[0xc8/8]=ctx.exeBase+0x2ce900;
    *reinterpret_cast<void**>(focus+0x190)=widget;
    *reinterpret_cast<void**>(widget+0x600)=display;*reinterpret_cast<void**>(widget+0x608)=bound;
}
void Observe(const D2RL::PluginContext& ctx){QolMaterials::ObserveControllerFocus(&ctx,true);}
void RefuseBefore(const D2RL::PluginContext& ctx,const std::function<void()>& change,const char* why) {
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"capture selected counter at physical press");
    const auto count=submissions;change();Pump(ctx);
    Check(submissions==count && !QolMaterials::Busy(),why);
}
}
namespace QolCompat {bool ValidateBeltEntry(uintptr_t) noexcept{return Fixture::guard && Fixture::beltGuard;}}
int main() {
    using namespace Fixture;
    image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000000,MEM_RESERVE,PAGE_NOACCESS));
    Check(image!=nullptr,"private image reservation");
    D2RL::PluginApi api{};api.apiSize=sizeof(api);
    api.checkExpectedBytes=+[](const D2RL::PluginContext*,uint64_t rva,const void*,uint32_t) noexcept{return guard && (!QolMaterials::BeltOnly(rva) || beltGuard);};
    api.queryService=+[](const D2RL::PluginContext*,D2RL::ServiceId id,uint32_t,const void** out) noexcept {
        if(id==D2RL::ServiceId::Inventory)*out=&inventory;
        else if(id==D2RL::ThreadService::Id)*out=&threads;
        else if(id==D2RL::ServiceId::Lifecycle)*out=&lifecycle;
        else return D2RL::ServiceQueryResult::Unavailable;
        return D2RL::ServiceQueryResult::Success;
    };
    D2RL::PluginContext ctx{};ctx.contextSize=sizeof(ctx);ctx.api=&api;ctx.exeBase=reinterpret_cast<uintptr_t>(image);
    inventory.serviceSize=sizeof(inventory);inventory.serviceVersion=D2RL::InventoryService::AbiVersion;
    inventory.getLocalPlayer=+[](const D2RL::PluginContext*,D2RL::PlayerHandle* out) noexcept{*out=owner;return D2RL::Inventory::Result::Success;};
    inventory.getCursorItem=+[](const D2RL::PluginContext*,D2RL::PlayerHandle,D2RL::ItemHandle* out) noexcept{*out=cursor;return D2RL::Inventory::Result::Success;};
    inventory.forEachInventoryItem=+[](const D2RL::PluginContext* context,D2RL::PlayerHandle,const D2RL::Inventory::ItemFilter* filter,D2RL::Inventory::ItemCallback callback,void* user) noexcept {
        if(!quantityAvailable)return D2RL::Inventory::Result::Unavailable;
        if(cube && filter->containerMask&D2RL::Items::ContainerBit(C::Inventory)) {
            D2RL::Items::ItemInfo info{};info.code=D2RL::Items::MakeItemCode("box");info.container=C::Inventory;
            if(callback(context,&info,user)==D2RL::Inventory::IterationAction::Stop)return D2RL::Inventory::Result::Success;
        }
        if(quantity>0) {
            D2RL::Items::ItemInfo info{};info.code=itemCode;info.quantity=static_cast<int32_t>(quantity);
            callback(context,&info,user);
        }
        return D2RL::Inventory::Result::Success;
    };
    threads.serviceSize=sizeof(threads);threads.serviceVersion=D2RL::ThreadService::AbiVersion;
    threads.runOnUiThread=+[](const D2RL::PluginContext*,D2RL::Threads::Callback fn,void* user) noexcept{
        if(refuseSchedule)return D2RL::Threads::Result::Unavailable;
        task={fn,user};return D2RL::Threads::Result::Success;
    };
    lifecycle.serviceSize=sizeof(lifecycle);lifecycle.serviceVersion=D2RL::LifecycleService::AbiVersion;
    lifecycle.registerGameplayEventListener=+[](const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEventListener*,D2RL::Lifecycle::ListenerHandle* out) noexcept{*out=1;return D2RL::Lifecycle::Result::Success;};
    Jump(0xce500,reinterpret_cast<void*>(&Open));Jump(0x846170,reinterpret_cast<void*>(&Panel));
    Jump(0x23af50,reinterpret_cast<void*>(&Tab));Jump(0x856220,reinterpret_cast<void*>(&Child));
    Jump(0x2ce900,reinterpret_cast<void*>(&Bound));Jump(0x36ef50,reinterpret_cast<void*>(&Code));
    Jump(0x34a330,reinterpret_cast<void*>(&Id));Jump(0x23b980,reinterpret_cast<void*>(&Previous));
    Jump(0x46d9a0,reinterpret_cast<void*>(&Stock));Jump(0x2cf680,reinterpret_cast<void*>(&Withdraw));
    Check(VirtualAlloc(image+0x3440000,4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"focus manager slot");
    *reinterpret_cast<void**>(image+0x3440170)=manager;*reinterpret_cast<void**>(manager+0xd0)=focus;
    *reinterpret_cast<uintptr_t**>(widget)=vtable;*reinterpret_cast<uintptr_t**>(other)=vtable;
    FlushInstructionCache(GetCurrentProcess(),image,0x4000000);
    Check(QolMaterials::Initialize(&ctx),"production module initializes with SDK fixtures");
    for(const auto categoryTab:{2u,3u,4u})for(const bool toCube:{false,true}) {
        Reset(ctx);tab=categoryTab;itemCode=D2RL::Items::MakeItemCode(tab==2?"gcy":tab==3?"rvs":"r22");
        // No SDK tooltip/handle at all: idle UI observation must still work.
        QolMaterials::focusQueuedAt=0;Pump(ctx);
        Check(QolMaterials::observedFocus.identity.handle==0 && display[2]==0,"no fabricated SDK handle for ID-zero display");
        Check(QolMaterials::RequestFocusedCounter(toCube),"native selected counter captured without a tooltip event");
        const auto count=submissions;Pump(ctx);
        Check(submissions==count+1 && lastPage==(toCube?3:0) && QolMaterials::Busy(),"gem/material/rune submits once to requested destination");
        Pump(ctx);Check(submissions==count+1 && QolMaterials::Busy(),"return alone cannot confirm or resubmit");
        quantity=1;Pump(ctx);Check(!QolMaterials::Busy() && submissions==count+1,"destination increase confirms one withdrawal");
    }
    for(const auto& mutation:std::vector<std::function<void()>>{
        []{*reinterpret_cast<void**>(focus+0x190)=nullptr;},[]{widget[0x51]=0;},[]{widget[0x50]=0;},
        []{*reinterpret_cast<void**>(widget+0x608)=nullptr;},[]{bound[2]=0;},[]{bound[0]=1;},
        []{wrongSlot=true;},[]{tab=1;},[]{tab=5;},[]{open=false;},[]{guard=false;},
        []{vtable[0xc8/8]=0;},[]{owner=0;}}) {
        Reset(ctx);mutation();Observe(ctx);Check(!QolMaterials::RequestFocusedCounter(false),"unproven/empty/non-counter focus cannot be captured");
    }
    RefuseBefore(ctx,[]{*reinterpret_cast<void**>(focus+0x190)=other;},"changed controller widget cancels captured counter");
    RefuseBefore(ctx,[]{bound[2]=29;},"changed bound runtime identity cancels");
    RefuseBefore(ctx,[]{itemCode=D2RL::Items::MakeItemCode("gcg");},"changed bound code cancels");
    RefuseBefore(ctx,[]{tab=4;},"tab change cancels");
    RefuseBefore(ctx,[]{previous=true;},"season change cancels");
    RefuseBefore(ctx,[]{owner=8;},"player change cancels");
    RefuseBefore(ctx,[]{cursor=100;},"occupied cursor cancels");
    RefuseBefore(ctx,[]{guard=false;},"changed native profile cancels");
    QolMaterials::ready=true;
    RefuseBefore(ctx,[]{stock=false;},"empty stock cannot send");
    RefuseBefore(ctx,[]{quantityAvailable=false;},"missing destination baseline cannot send");
    RefuseBefore(ctx,[]{QolMaterials::requestedAt-=751;},"expired physical press cannot send");
    Reset(ctx);Observe(ctx);QolMaterials::observedFocus.tick-=251;
    Check(!QolMaterials::RequestFocusedCounter(false),"stale UI observation cannot arm an action");
    Reset(ctx);Observe(ctx);QolMaterials::ObserveControllerFocus(&ctx,false);
    Check(!QolMaterials::RequestFocusedCounter(false),"keyboard/inactive focus clears counter observation");
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(true),"Cube request captures selected gem");
    cube=false;const auto beforeCube=submissions;Pump(ctx);
    Check(submissions==beforeCube && !QolMaterials::Busy(),"absent carried Cube refuses without inventory fallback");
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"timeout fixture armed");Pump(ctx);
    const auto beforeTimeout=submissions;QolMaterials::deadline=0;Pump(ctx);
    Check(submissions==beforeTimeout && !QolMaterials::Busy(),"unobserved request times out with no retry");
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"session fixture armed");
    QolMaterials::Pump(&ctx,true);const auto old=task;task={};QolMaterials::Session(nullptr,nullptr,nullptr);
    const auto beforeSession=submissions;old.fn(&ctx,old.user);
    Check(submissions==beforeSession && !QolMaterials::Busy(),"queued callback cannot cross a session generation");
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"context-loss fixture armed");
    const auto beforeInactive=submissions;QolMaterials::Pump(&ctx,false);Flush(ctx);
    Check(!QolMaterials::Busy() && submissions==beforeInactive,"loss of controller stash context cancels before submission");
    Reset(ctx);Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"contract-loss fixture armed");
    const auto beforeGuard=nativeReads;guard=false;Pump(ctx);
    Check(nativeReads==beforeGuard && !QolMaterials::Busy(),"changed guard is refused before any native resolver call");
    Reset(ctx);QolMaterials::focusQueuedAt=0;QolMaterials::Pump(&ctx,true);QolMaterials::focusQueuedAt-=251;Flush(ctx);
    Check(!QolMaterials::RequestFocusedCounter(false),"late observer cannot refresh a physical-press snapshot");
    Reset(ctx);QolMaterials::focusQueuedAt=0;QolMaterials::Pump(&ctx,true);const auto staleObserver=task;
    QolMaterials::focusQueuedAt-=5001;QolMaterials::Pump(&ctx,true);staleObserver.fn(&ctx,staleObserver.user);
    Check(QolMaterials::focusQueued && !QolMaterials::observedFocus.tick,"expired observer cannot clear or overwrite replacement work");
    Flush(ctx);Check(QolMaterials::RequestFocusedCounter(false),"new generation observer recovers from stalled UI queue");
    Check(QolMaterials::RequestFocusedCounter(true) && QolMaterials::destination==QolMaterials::Destination::Inventory,"second press consumes busy input without changing original destination");
    Reset(ctx);QolMaterials::focusQueuedAt=0;refuseSchedule=true;Pump(ctx);
    Check(!QolMaterials::focusQueued && !QolMaterials::Busy() && !QolMaterials::RequestFocusedCounter(false),"scheduler refusal cannot arm stale counter work");
    Reset(ctx);beltGuard=false;Check(QolMaterials::Initialize(&ctx),"belt incompatibility cannot prevent counter module initialization");
    Observe(ctx);Check(QolMaterials::RequestFocusedCounter(false),"counter withdrawal remains available after belt-only mismatch");
    const auto beforeBeltMismatch=submissions;Pump(ctx);
    Check(submissions==beforeBeltMismatch+1,"Inventory counter route does not depend on the unused belt planner");
    Reset(ctx);beltGuard=false;tab=3;QolMaterials::RequestRefill();const auto beforeBelt=submissions;Pump(ctx);
    Check(submissions==beforeBelt && !QolMaterials::Busy() && QolMaterials::ready,"belt mismatch refuses refill while retaining independent counter feature");
    QolMaterials::Shutdown();VirtualFree(image,0,MEM_RELEASE);
    std::printf("Production counter focus/mailbox: %u checks passed.\n",checks);
}
