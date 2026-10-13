// Production glue with private SDK/native fixtures. No game process, game files,
// installed DLL, network connection or authoritative game inventory is opened.
#include "../src/controller_input.cpp"
#include "../src/client_loot.cpp"
#include "../src/plugin_main.cpp"
#include "../src/qol_navigation.cpp"
#include <cstdlib>

namespace Audit {
std::atomic<unsigned> checks{};
unsigned uiCalls{},gameCalls{},moves{},nativePickups{},sends{},installed{},enumerations{};
bool guard=true,serverAvailable=true,season=false;
bool localAvailable=true;
D2RL::PlayerHandle localHandle=7;
D2RL::Items::Result transactionResult=D2RL::Items::Result::Success;
D2RL::Items::ItemInfo item{};
D2RL::Items::ItemContainer destination{};
struct Task {D2RL::Threads::Callback fn{};void* user{};};
Task ui{},game{};
unsigned char* image{};
unsigned char bank[0x180]{},manager[0x200]{},focus[0x200]{},widget[0x650]{},node[0x1800]{};
uintptr_t table[32]{},nodeTable[16]{};
uint32_t local[16]{},peer[16]{},unit[16]{},quality=4;
unsigned char* modes{};
void Check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
void Commit(uintptr_t rva,size_t bytes=64) {
    const auto start=rva&~uintptr_t(4095),end=(rva+bytes+4095)&~uintptr_t(4095);
    Check(VirtualAlloc(image+start,end-start,MEM_COMMIT,PAGE_EXECUTE_READWRITE)!=nullptr,"private fixture page");
}
void Copy(uintptr_t rva,const unsigned char* bytes,size_t size) {Commit(rva,size);std::memcpy(image+rva,bytes,size);}
void Jump(uintptr_t rva,const void* fn) {Commit(rva);QolXInput::Jump(image+rva,fn);}
void RelativeCall(uintptr_t rva,unsigned offset,const void* fn) {
    int32_t delta{};std::memcpy(&delta,image+rva+offset+1,4);
    Jump(rva+offset+5+delta,fn);
}
void* __fastcall Node(const void*) {return node;}
void* __fastcall CurrentNode() {return node;}
void* __fastcall Bank(const char*) {return bank;}
int32_t __fastcall Context() noexcept {return 1;}
void* __fastcall Player(int32_t) noexcept {return local;}
void* __fastcall Lookup(uint32_t id,uint32_t type) {return type==4 && id==unit[2]?unit:nullptr;}
void* __fastcall Focused(void*,const void*) {return unit;}
uint32_t __fastcall Code(const void*) {return item.code;}
uint8_t __fastcall Page(const void*) {return QolClientTransfer::NativePage(item.container);}
int __fastcall Distance(void*,void*) {return 1;}
int __fastcall Collision(void*,void*,uint32_t) {return 0;}
void __fastcall Send(uint16_t type,uint32_t guid) {Check(type==4 && guid==item.runtimeId,"native request uses captured GUID");++sends;}
bool __cdecl InGame() noexcept {return true;}
bool __fastcall Pickup(void*,uint32_t,bool,uint32_t,bool,bool) {++nativePickups;return true;}
int64_t __fastcall Action(void*,void*,void*,int32_t) {return 123;}
void SetSource(D2RL::Items::ItemContainer source,uint32_t page=0) {
    item={.structSize=D2RL::Items::ItemInfoSize};item.handle=42;item.runtimeId=42;item.classId=3;
    item.code=123;item.container=source;item.sharedStashPage=page;
    unit[0]=4;unit[1]=3;unit[2]=42;unit[3]=source==D2RL::Items::ItemContainer::Ground?3:0;
    *reinterpret_cast<const uint32_t**>(reinterpret_cast<unsigned char*>(unit)+0x10)=&quality;
    s_FocusIdentity=item;s_FocusIdentityTick=GetTickCount64();
}
void FlushUi(const D2RL::PluginContext* ctx) {const auto task=ui;ui={};if(task.fn)task.fn(ctx,task.user);}
void FlushGame(const D2RL::PluginContext* ctx) {const auto task=game;game={};if(task.fn)task.fn(ctx,task.user);}
void Lifecycle(uint64_t generation,D2RL::Lifecycle::GameplayEventKind kind,uint32_t id=0) {
    D2RL::Lifecycle::GameplayEvent e{};e.structSize=D2RL::Lifecycle::GameplayEventSize;
    e.kind=kind;e.sessionGeneration=generation;e.playerId=id;
    if(kind==D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady)GroundPlayerReady(nullptr,&e,nullptr);
    else IdentifySessionChanged(nullptr,&e,nullptr);
}
void Held(unsigned buttons=0x100) {
    ControllerQoL::s_NativeSnapshot=(GetTickCount64()<<16)|buttons;
    ControllerQoL::s_NativeInstalled=true;groundInputActive=true;g_PollingRunning=true;
}
void PauseThrottle(){std::this_thread::sleep_for(std::chrono::milliseconds(260));}
}
int main() {
    using namespace Audit;using C=D2RL::Items::ItemContainer;using Kind=D2RL::Lifecycle::GameplayEventKind;
    image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000000,MEM_RESERVE,PAGE_NOACCESS));Check(image!=nullptr,"private image reservation");
    D2RL::PluginApi api{};api.apiSize=D2RL::PluginApiSize;
    api.checkExpectedBytes=+[](const D2RL::PluginContext*,uintptr_t,const void*,uint32_t) noexcept {return guard;};
    api.installInlineHook=+[](const D2RL::PluginContext*,const D2RL::InlineHookRegistration* hook) noexcept {
        Check(hook->rva==QolCompat::Native::Actions[4].rva,"only local Guided Arrow packet hook retained");
        ++installed;*hook->original=reinterpret_cast<void*>(&Action);return true;
    };
    D2RL::PluginContext context{};context.contextSize=D2RL::PluginContextSize;context.api=&api;context.exeBase=reinterpret_cast<uintptr_t>(image);
    D2RL::InventoryService inventory{};
    inventory.getLocalPlayer=+[](const D2RL::PluginContext*,D2RL::PlayerHandle* out) noexcept {
        *out=localAvailable?localHandle:0;return localAvailable?D2RL::Inventory::Result::Success:D2RL::Inventory::Result::Unavailable;
    };
    inventory.forEachInventoryItem=+[](const D2RL::PluginContext*,D2RL::PlayerHandle,const D2RL::Inventory::ItemFilter*,D2RL::Inventory::ItemCallback,void*) noexcept {
        ++enumerations;return D2RL::Inventory::Result::Success;
    };
    D2RL::ItemService items{};items.serviceVersion=1;items.serviceSize=D2RL::ItemServiceSize;
    items.capabilities=D2RL::Items::ItemServiceCapabilityBit(D2RL::Items::ItemServiceCapability::SharedStashWrite);
    items.getItemInfo=+[](const D2RL::PluginContext*,D2RL::ItemHandle handle,D2RL::Items::ItemInfo* out) noexcept {
        if(handle!=item.handle)return D2RL::Items::Result::InvalidArgument;*out=item;return D2RL::Items::Result::Success;
    };
    items.executeExistingItemTransaction=+[](const D2RL::PluginContext*,const D2RL::Items::ExistingItemTransaction* txn,D2RL::Items::ExistingItemTransactionResult* out) noexcept {
        Check(txn->player==localHandle && txn->operationCount==1 && txn->operations->item==item.handle,"one existing-item operation for local owner");
        ++moves;destination=txn->operations->move.destination.container;
        out->committedOperationCount=transactionResult==D2RL::Items::Result::Success?1:0;return transactionResult;
    };
    D2RL::ThreadService threads{};
    threads.runOnUiThread=+[](const D2RL::PluginContext*,D2RL::Threads::Callback fn,void* user) noexcept {++uiCalls;ui={fn,user};return D2RL::Threads::Result::Success;};
    threads.runOnGameThread=+[](const D2RL::PluginContext*,D2RL::Threads::Callback fn,void* user) noexcept {
        ++gameCalls;if(!serverAvailable)return D2RL::Threads::Result::Unavailable;game={fn,user};return D2RL::Threads::Result::Success;
    };
    g_PluginContext=&context;g_Inventory=&inventory;g_Items=&items;g_Threads=&threads;s_IdentifySessionEvents=true;
    Jump(0x8b2d0,reinterpret_cast<void*>(&Context));Jump(0x9a480,reinterpret_cast<void*>(&Player));
    Jump(0x9a5d0,reinterpret_cast<void*>(&Audit::Lookup));Jump(0x2c49f0,reinterpret_cast<void*>(&Focused));
    Jump(0x36ef50,reinterpret_cast<void*>(&Code));Jump(0x36cfe0,reinterpret_cast<void*>(&Page));
    Jump(0x325140,reinterpret_cast<void*>(&Distance));Jump(0x350550,reinterpret_cast<void*>(&Collision));Jump(0xfa180,reinterpret_cast<void*>(&Send));
    Copy(0xce500,QolMaterials::Signatures::UiMode,sizeof(QolMaterials::Signatures::UiMode));
    int32_t delta{};std::memcpy(&delta,QolMaterials::Signatures::UiMode+6,4);const uintptr_t modeRva=0xce500+10+delta;
    Commit(modeRva,128);modes=image+modeRva;
    Commit(QolNativeProfile::ManagerSlotRva);*reinterpret_cast<void**>(image+QolNativeProfile::ManagerSlotRva)=manager;
    Commit(QolNativeProfile::ActiveIndexRva);*reinterpret_cast<unsigned*>(image+QolNativeProfile::ActiveIndexRva)=0;
    *reinterpret_cast<unsigned*>(manager+0xdc)=1;*reinterpret_cast<void**>(manager+0xd0)=focus;
    *reinterpret_cast<void**>(focus+0x190)=widget;*reinterpret_cast<uintptr_t**>(widget)=table;
    widget[0x50]=widget[0x51]=1;table[0xc8/8]=context.exeBase+0x2c49f0;
    ControllerQoL::s_NativeGame=context.exeBase;local[0]=peer[0]=0;local[2]=10;peer[2]=11;
    // Execute existing Shared selection witnesses with mock callees/data only.
    Copy(0x846170,QolCustomProfile::Game846170,sizeof(QolCustomProfile::Game846170));
    int32_t branch{};std::memcpy(&branch,QolCustomProfile::Game846170+15,4);Jump(0x846170+19+branch,reinterpret_cast<void*>(&Bank));
    Copy(0x23af50,SharedPageNative::SelectedTab,sizeof(SharedPageNative::SelectedTab));
    RelativeCall(0x23af50,17,reinterpret_cast<void*>(&Node));RelativeCall(0x23af50,30,reinterpret_cast<void*>(&CurrentNode));
    Copy(0x23b910,QolSharedSdk::SeasonBytes,sizeof(QolSharedSdk::SeasonBytes));
    RelativeCall(0x23b910,13,reinterpret_cast<void*>(&Node));RelativeCall(0x23b910,31,reinterpret_cast<void*>(&CurrentNode));
    Copy(0x23aef9,QolSharedSdk::NormalIndexBytes,sizeof(QolSharedSdk::NormalIndexBytes));
    *reinterpret_cast<uintptr_t**>(node)=nodeTable;nodeTable[0x58/8]=reinterpret_cast<uintptr_t>(&CurrentNode);
    *reinterpret_cast<int*>(node+0x16c4)=1;bank[0x50]=bank[0x51]=1;
    FlushInstructionCache(GetCurrentProcess(),image,0x4000000);

    // #3/#7: actual input state, physical lifetime, two export threads and reentry.
    ControllerQoL::SetActiveModifier("bumper",30);ControllerQoL::SetInputFilteringEnabled(true);
    static std::atomic<unsigned> callbacks{};
    ControllerQoL::SetQuickMoveCallback(+[]() noexcept {++callbacks;});
    for(unsigned button:{0x4000u,0x8000u,0x2000u,0x200u,0x80u}) {
        ControllerQoL::ResetNativeTracking();XINPUT_STATE state{};
        state.Gamepad.wButtons=static_cast<WORD>(0x100|button);ControllerQoL::ProcessGamepadShortcuts(0,&state);
        Check(!(state.Gamepad.wButtons&button),"chord consumed");
        state.Gamepad.wButtons=static_cast<WORD>(button);ControllerQoL::ProcessGamepadShortcuts(0,&state);
        Check(!(state.Gamepad.wButtons&button),"LB-first release never synthesizes native press");
        state.Gamepad.wButtons=static_cast<WORD>(0x100|button);ControllerQoL::ProcessGamepadShortcuts(0,&state);
        state.Gamepad.wButtons=0;ControllerQoL::ProcessGamepadShortcuts(0,&state);
        state.Gamepad.wButtons=static_cast<WORD>(button);ControllerQoL::ProcessGamepadShortcuts(0,&state);
        Check(state.Gamepad.wButtons&button,"new unmodified physical press reaches native");
    }
    Check(callbacks==1,"same held X not dispatched again on LB repress");
    ControllerQoL::SetInputFilteringEnabled(false);XINPUT_STATE disabled{};disabled.Gamepad.wButtons=0xffff;
    disabled.Gamepad.bLeftTrigger=disabled.Gamepad.bRightTrigger=255;ControllerQoL::ProcessGamepadShortcuts(0,&disabled);
    Check(disabled.Gamepad.wButtons==0xffff && disabled.Gamepad.bLeftTrigger==255 && disabled.Gamepad.bRightTrigger==255 && callbacks==1,"#4 disabled preserves full raw input and callbacks");
    ControllerQoL::SetInputFilteringEnabled(true);QolXInput::Gate gate;gate.Enable();unsigned nested{};
    gate.Run([&]{++nested;gate.Run([&]{nested+=100;});});Check(nested==1,"different callback types share recursion ownership");
    for(unsigned iteration=0;iteration<100;++iteration) {
        ControllerQoL::ResetNativeTracking();callbacks=0;std::atomic<unsigned> started{};std::atomic<bool> go{};
        auto run=[&]{++started;while(!go)std::this_thread::yield();XINPUT_STATE state{};state.Gamepad.wButtons=0x4100;
            gate.Run([&]{ControllerQoL::ProcessGamepadShortcuts(0,&state);});Check(state.Gamepad.wButtons==0x100,"concurrent raw state consumed");};
        std::thread a(run),b(run);while(started!=2)std::this_thread::yield();go=true;a.join();b.join();Check(callbacks==1,"concurrent callbacks dispatch one physical X");
    }
    ControllerQoL::ResetNativeTracking();
    static std::atomic<unsigned> inside{},peak{};
    ControllerQoL::SetQuickMoveCallback(+[]() noexcept {
        const auto count=inside.fetch_add(1)+1;auto prior=peak.load();
        while(prior<count && !peak.compare_exchange_weak(prior,count)){}
        std::this_thread::sleep_for(std::chrono::milliseconds(10));inside.fetch_sub(1);
    });
    auto pad=[&](unsigned index){XINPUT_STATE state{};state.Gamepad.wButtons=0x4100;gate.Run([&]{ControllerQoL::ProcessGamepadShortcuts(index,&state);});};
    std::thread padA([&]{pad(0);}),padB([&]{pad(1);});padA.join();padB.join();
    Check(peak==1,"different pads serialize global shortcut/hold state");
    gate.Disable();ControllerQoL::SetQuickMoveCallback(TriggerQuickMoveOnFocusedItem);
    Check(QolCubeShortcut::Current(1,1,1000,2000) && !QolCubeShortcut::Current(1,1,1000,2001) &&
        !QolCubeShortcut::Current(1,2,1000,1001),"Cube expiry and generation boundaries");
    Check(!QolCubeShortcut::Context(C::PersonalStash,true,false,false) &&
        !QolCubeShortcut::Context(C::SharedStash,true,false,false) &&
        !QolCubeShortcut::Context(C::Unknown,true,true,true),"source-specific storage context rejects unrelated open panel");

    // #1/#2: requests use local lifecycle identity and client native routing in all modes.
    QolClientLoot::context=&context;QolClientLoot::rankItem=GroundLoot::GetItemCodeRank;QolClientLoot::inGame=InGame;QolClientLoot::ready=true;
    QolClientLoot::SetLifecycleAvailable(true);Lifecycle(100,Kind::GameJoined);Lifecycle(100,Kind::LocalPlayerReady,10);
    Check(QolClientLoot::IsLocalActor(0,10) && !QolClientLoot::IsLocalActor(0,11),"peer cannot become local actor");
    SetSource(C::Ground);Held();Check(QolClientLoot::Activate(),"local loot admission");QolClientLoot::Track(42,true);
    QolClientLoot::Refresh(10,true);const auto request=QolClientLoot::Capture(0);Check(request.item.guid==42,"owned visible loot slot");
    GroundLoot::s_Pickup=Pickup;GroundLoot::s_PickupCallsActive=true;
    Check(GroundLoot::HookPickup(peer,42,false,10,false,false) && nativePickups==1,"guest native pickup passes through host LB");
    Check(!GroundLoot::HookPickup(local,42,false,10,false,false) && nativePickups==1,"local native chord pickup blocked");
    QolClientLoot::Submit(request,10,true);Check(sends==1,"one local UI native request");
    QolClientLoot::Submit(request,10,true);Check(sends==1,"outstanding local request is not resent");
    Check(!QolClientLoot::TakeNativePickupPermit(11,42),"guest cannot steal local one-use permit");
    Check(GroundLoot::HookPickup(local,42,false,10,false,false) && nativePickups==2,"local explicit request allowed once");
    Check(!GroundLoot::HookPickup(local,42,false,10,false,false),"permit consumed once");
    g_Settings.enabled=false;Check(GroundLoot::HookPickup(local,42,false,10,false,false),"disabled host pickup passes through");g_Settings.enabled=true;
    for(bool available:{true,false}) {
        serverAvailable=available;const auto before=gameCalls;QueueGroundPickup(&context,0);
        Check(ui.fn && gameCalls==before,"offline/host/client loot never queues authoritative game work");FlushUi(&context);
    }
    const auto beforeLeave=sends;QueueGroundPickup(&context,0);const auto stale=ui;ui={};
    QueueClientGroundRefresh(&context);const auto staleRefresh=ui;ui={};
    PlacardOverlay::s_TrackedPlacards[42].textBuffer=reinterpret_cast<void*>(1);
    Lifecycle(100,Kind::GameLeft);Check(PlacardOverlay::s_TrackedPlacards.empty(),"session forgets expired placard buffer without restoring it");
    Lifecycle(101,Kind::GameJoined);Lifecycle(100,Kind::LocalPlayerReady,10);
    Check(!QolClientLoot::Ready() && !QolClientLoot::Active() && !QolClientLoot::Slot(0).guid,"old ready event and slots rejected after rejoin");
    Lifecycle(101,Kind::LocalPlayerReady,10);QolClientLoot::Activate();stale.fn(&context,stale.user);
    QueueClientGroundRefresh(&context);const auto currentRefresh=s_ClientLootRefreshToken.load();
    Check(currentRefresh!=0,"new session refresh is not blocked by abandoned UI work");
    staleRefresh.fn(&context,staleRefresh.user);Check(s_ClientLootRefreshToken==currentRefresh,"old refresh cannot clear new session's queued ownership");
    FlushUi(&context);
    QolClientLoot::Submit(request,10,true);Check(sends==beforeLeave,"old queued callback and same-ID request cannot cross games");
    QolClientLoot::Track(42,true);
    QolClientLoot::Refresh(10,false); // Release the empty hold captured before Track.
    QolClientLoot::Refresh(10,true);guard=false;
    QolClientLoot::Submit(QolClientLoot::Capture(0),10,true);Check(sends==beforeLeave,"changed native guard cancels request");guard=true;
    Held();Check(GroundLoot::HookPickup(local,42,false,10,false,false),"unavailable loot profile fails open for local native pickup");
    std::array<uint8_t,5> packet{5,1,0,2,0},projected{};
    GroundLoot::s_GetSelectedSkill=+[](void*) -> void* {Check(false,"guest selected skill must not be inspected");return nullptr;};
    Check(!GroundLoot::ProjectGuidedArrowPacket(peer,packet.data(),5,projected),"host correction leaves guest Guided Arrow packets untouched");
    GroundLoot::s_GetSelectedSkill=nullptr;

    // #5/#6: current source panel/identity, local owner/page, atomic Cube destination.
    serverAvailable=true;moves=0;SetSource(C::Inventory);std::memset(modes,0,128);
    TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);FlushGame(&context);Check(moves==0 && !game.fn,"closed panels reject fresh cached focus");
    modes[UI_MODE_INVENTORY]=1;TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);Check(game.fn!=nullptr,"valid inventory Cube queued");
    modes[UI_MODE_INVENTORY]=0;FlushGame(&context);Check(moves==0,"panel closed after UI dispatch cancels Cube");
    modes[UI_MODE_INVENTORY]=1;PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();item.runtimeId=43;s_FocusIdentity=item;
    FlushUi(&context);Check(!game.fn && moves==0,"changed physical-press focus cannot substitute another item");SetSource(C::Inventory);
    TriggerQuickMoveToCubeOnFocusedItem();const auto oldCube=ui;ui={};Lifecycle(101,Kind::GameLeft);Lifecycle(102,Kind::GameJoined);
    SetSource(C::Inventory);oldCube.fn(&context,oldCube.user);Check(!game.fn,"cancelled UI Cube callback cannot cross session");
    modes[UI_MODE_INVENTORY]=0;modes[UI_MODE_STASH]=1;
    for(uint32_t page:{0u,3u}) {
        SetSource(C::SharedStash,page);*reinterpret_cast<uint64_t*>(bank+0x168)=page;
        Check(QolSharedSdk::Selected(context.exeBase).page==page,"native selected normal Shared page fixture");
        PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);Check(game.fn!=nullptr,"Shared Cube captures UI request");
        FlushGame(&context);Check(destination==C::Cube && moves==page/3+1,"Shared Cube commits one Cube transaction on selected page");
    }
    const auto beforeRejected=moves;PauseThrottle();transactionResult=D2RL::Items::Result::PolicyRejected;
    TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);FlushGame(&context);
    Check(moves==beforeRejected+1 && destination==C::Cube,"full Cube/rejection never causes Inventory fallback or retry");transactionResult=D2RL::Items::Result::Success;
    PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);*reinterpret_cast<uint64_t*>(bank+0x168)=4;FlushGame(&context);
    Check(moves==beforeRejected+1,"page changed across queue cancels Shared Cube");*reinterpret_cast<uint64_t*>(bank+0x168)=3;
    PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);localHandle=8;FlushGame(&context);
    Check(moves==beforeRejected+1,"local owner changed across queue cancels Cube");localHandle=7;
    PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);localAvailable=false;FlushGame(&context);
    Check(moves==beforeRejected+1,"missing local owner cannot fall back to remembered Cube owner");localAvailable=true;
    PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);node[0xb88]=1;FlushGame(&context);
    Check(moves==beforeRejected+1,"season changed across queue cancels Cube");node[0xb88]=0;
    items.capabilities=0;PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);FlushGame(&context);
    Check(moves==beforeRejected+1,"older authoritative Shared route refuses Cube without Inventory fallback");
    items.capabilities=D2RL::Items::ItemServiceCapabilityBit(D2RL::Items::ItemServiceCapability::SharedStashWrite);

    // Live beta.3 failure: LB+X can send the carried Cube into its own open
    // storage. Test real item-code padding and every ordinary mutation boundary.
    for(const auto code:{D2RL::Items::MakeItemCode("box"),uint32_t(0x00786f62)}) {
        SetSource(C::Inventory);item.code=code;s_FocusIdentity=item;
        std::memset(modes,0,128);modes[UI_MODE_INVENTORY]=modes[UI_MODE_CUBE]=1;
        const auto before=moves;
        QueueLegacyQuickMove(item,true);FlushGame(&context);
        Check(moves==before && !game.fn,"LB+X open Cube rejects Cube self insertion");
        PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);FlushGame(&context);
        Check(moves==before,"LB+Y rejects both Cube code padding forms");
        for(const auto source:{C::Inventory,C::PersonalStash,C::SharedStash}) {
            SetSource(source);item.code=code;s_FocusIdentity=item;modes[UI_MODE_STASH]=1;
            MoveRequest request{};request.targetItem=item.handle;
            request.targetCode=D2RL::Items::MakeItemCode("cap"); // Cached code must not decide safety.
            request.identity=request.sharedIdentity=item;request.sourceContainer=source;
            request.primaryDestination=C::Cube;
            ExecuteMove(&context,&request);
            Check(moves==before,"live Cube identity rejects all authoritative Cube source routes");
        }
        SetSource(C::Inventory);item.code=code;
        MoveRequest request{};request.targetItem=item.handle;request.identity=item;
        request.sourceContainer=C::Inventory;request.primaryDestination=C::Inventory;request.secondaryDestination=C::Cube;
        ExecuteMove(&context,&request);
        Check(moves==before,"Cube secondary destination cannot bypass executor guard");
        SetSource(C::Cube);item.code=code;s_FocusIdentity=item;
        PauseThrottle();TriggerQuickMoveToCubeOnFocusedItem();FlushUi(&context);FlushGame(&context);
        Check(moves==before+1 && destination==C::Inventory,"existing nested Cube can still withdraw to Inventory");
    }
    SetSource(C::Inventory);std::memset(modes,0,128);modes[UI_MODE_CUBE]=1;
    const auto beforeOrdinary=moves;QueueLegacyQuickMove(item,true);FlushGame(&context);
    Check(moves==beforeOrdinary+1 && destination==C::Cube,"LB+X ordinary item still moves to Cube");

    // Exercise the actual BankPanel hook with native message decoding and a
    // physical snapshot. Opposite-trigger begins/repeats never reach TabBar.
    ctx=&context;enabled=useL1=sharedPageHook=true;*reinterpret_cast<int*>(node+0x16c4)=1;
    static unsigned bankCalls{},bankAction{};
    originalBankMessage=+[](void*,void* message) {
        ++bankCalls;auto* payload=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(message)+0x110);
        bankAction=*reinterpret_cast<unsigned*>(payload+0x10);
        Check(payload[0x18]==0,"mapped Shared page action clears repeat flag");
    };
    for(const unsigned first:{7u,8u})for(const bool repeat:{false,true}) {
        sharedPageGesture={};Held();MenuCopy message;
        *reinterpret_cast<uint64_t*>(message.message)=0x4ae067c6248b6042ULL;
        *reinterpret_cast<uint64_t*>(message.message+0x88)=0x1e439768f7dd4594ULL;
        *reinterpret_cast<void**>(message.message+0x110)=message.payload;
        *reinterpret_cast<unsigned*>(message.payload+0x10)=first;
        const auto before=bankCalls;HookBankMessage(bank,message.message);
        Check(bankCalls==before+1 && bankAction==first+12,"physical first trigger maps only Shared page");
        *reinterpret_cast<unsigned*>(message.payload+0x10)=first==7?8:7;message.payload[0x18]=repeat;
        HookBankMessage(bank,message.message);
        Check(bankCalls==before+1,"opposite trigger cannot escape Shared via actual hook");
        Held(0);HookBankMessage(bank,message.message);
        Check(bankCalls==before+1,"opposite trigger stays owned when LB releases first");
    }
    originalBankMessage=nullptr;sharedPageHook=false;sharedPageGesture={};ctx=nullptr;

    // #8: no packet observers or scans installed for loot, only existing projection.
    const auto scanned=enumerations;Check(QolCompat::ActionHooks::Install(&context,nullptr,+[](void*,const void*,int32_t,std::array<uint8_t,5>&) noexcept {return false;}),"projection-only hooks install");
    Check(installed==1 && enumerations==scanned,"loot installation does not observe/enumerate server actors");QolCompat::ActionHooks::Shutdown();
    g_PollingRunning=false;QolClientLoot::Shutdown();ControllerQoL::s_NativeGame=0;ControllerQoL::s_NativeInstalled=false;
    VirtualFree(image,0,MEM_RELEASE);std::printf("Controller QOL production audit regressions: %u checks passed.\n",checks.load());
}
