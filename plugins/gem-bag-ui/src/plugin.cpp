#include "native.h"
#include "layout.h"
#include "geometry.h"
#include "rpc.h"
#include "server_frame.h"
#include "native_input_profile.h"
#include "core_compatibility.h"
#include "profile.h"
#include "catalog.h"
#include <windows.h>
#include <shellapi.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

namespace {
using namespace D2RL;
using namespace GemBag;
const char* FullName="gem-bag-ui/Selector";
const PluginContext* ctx{};
const ItemService* items{};const InventoryService* inventory{};const PanelService* panels{};
const ResourceService* resources{};const WidgetService* widgets{};const SharedEventService* events{};
const ItemInteractionService* interactions{};const ThreadService* threads{};const LifecycleService* lifecycle{};
const OverlayService* overlay{};
const LocalizationService* localization{};
Panels::RegistrationHandle panel{};
std::array<Panels::RegistrationHandle,2> ownedPanels{};
std::array<Resources::RegistrationHandle,4> resource{};
SharedEvents::ListenerHandle uiListener{},tooltipListener{};
ItemInteractions::ListenerHandle interactionListener{};
Lifecycle::ListenerHandle readyListener{},leftListener{};
Overlay::CallbackHandle drawListener{};
std::recursive_mutex mutex;
bool stopping{},open{},busy{},ready{},halted{},inputInstalled{};
uint64_t generation{1},serial{},presentation{},pendingSerial{},lastRefresh{};
unsigned selected{},pendingGem{};
bool pendingWithdrawal{},pendingDeposit{},directDeposit{};
Info chosen{};
Rpc::Service transport;
Native::Snapshot snapshot{};
std::string message="Reading the Gem Bag...";
Gesture gesture[8]{};
HHOOK mouseHook{};
uint64_t mouseSerial{},mouseUsed{},mouseTick{};bool rightMouse{};
std::atomic<bool> uiQueued{};
enum class UiAction { OpenFocused, OpenHovered, DepositHovered, Close, Withdraw, ClickAt, HoverAt, Up, Down, Left, Right };
struct UiCommand { UiAction action; uint64_t generation,serial; uint32_t id{},cls{}; Info hovered{}; uint64_t tick{}; POINT point{}; };
std::deque<UiCommand> uiCommands;
bool commandQueued{};
std::array<bool,256> keyboardOwned{};
struct Hover {Info bag{};uint64_t tick{};POINT point{};bool valid{};} hover;
bool consumedRight{},consumedLeft{},tipLogged{};
HWND gameWindow{};
unsigned rightSeen{},leftSeen{};
POINT popupAnchor{};
Geometry::Bounds popupBounds{};
bool popupController{},positioned{},showFocus{},requestFailed{},frameLogged{};
bool localizationLogged{};
GemLabels localizedLabels{};
std::array<GemLabels,2> panelLabels{};
bool labelsReady{},labelWarningLogged{};
using Caller=bool(__fastcall*)(void*);
Caller originalCaller{};
thread_local bool insideCaller{};

template<class T> bool Read(uintptr_t p,T& out) noexcept {
    SIZE_T n{};return p>=0x10000&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(p),&out,sizeof(out),&n)&&n==sizeof(out);
}
bool PanelOpen() {
    Panels::PanelInfo info{.structSize=Panels::PanelInfoSize};
    return panels&&panel&&panels->getPanelInfo(ctx,panel,&info)==Panels::Result::Success&&info.presentationState==Panels::PresentationState::Open;
}
void Close() {
    open=false;directDeposit=false;ready=false;busy=false;pendingSerial=++serial;++presentation;
    if(panels&&panel)panels->closePanel(ctx,panel);
}
bool ReadInfo(ItemHandle h,Info& info) {
    info={.structSize=sizeof(Info)};return items->getItemInfo(ctx,h,&info)==Items::Result::Success;
}
void RefreshGemNames();
void Update() {
    if(!open)return;
    if(!PanelOpen()){open=false;++serial;return;}
    Info now{};
    if(!Native::SafeUi(ctx)||!ReadInfo(chosen.handle,now)||!Same(now,chosen)){Close();return;}
    if(!Catalog::Candidate(now)){Close();return;}
    RefreshGemNames();
    RECT client{};
    if(!gameWindow||!GetClientRect(gameWindow,&client))return;
    const int width=client.right-client.left,height=client.bottom-client.top;
    if(!positioned||popupBounds.width!=width||popupBounds.height!=height) {
        if(!Geometry::Place(popupAnchor,width,height,popupController,popupBounds)) {
            char line[240];std::snprintf(line,sizeof(line),"[GemBag] Picker geometry unavailable (%s); native bag actions retained.",Geometry::LastReason());ctx->LogWarn(line);Close();return;
        }
        positioned=true;
        char line[240];std::snprintf(line,sizeof(line),"[GemBag] Picker placed: cursor=(%ld,%ld) origin=(%ld,%ld) scale=%.4f client=%dx%d controller=%u.",popupAnchor.x,popupAnchor.y,popupBounds.origin.x,popupBounds.origin.y,popupBounds.scale,width,height,popupController?1u:0u);ctx->LogInfo(line);
    }
    Widgets::WidgetHandle root{},card{};
    if(widgets->findPanel(ctx,FullName,&root)!=Widgets::Result::Success || widgets->findWidget(ctx,root,"Card",&card)!=Widgets::Result::Success)return;
    const bool usable=ready&&!busy&&!halted&&snapshot.credits&&snapshot.cell;
    const auto definition=Catalog::Find(snapshot.bank,chosen);
    const unsigned mask=definition?definition->mask:0;
    auto visible=[&](const char* prefix,unsigned i,bool value) {
        char name[32];Widgets::WidgetHandle h{};std::snprintf(name,sizeof(name),"%s%u",prefix,i);
        if(widgets->findWidget(ctx,card,name,&h)==Widgets::Result::Success)widgets->setWidgetVisible(ctx,h,value);
    };
    for(unsigned i=0;i<Gems.size();++i) {
        const bool available=(mask&(1u<<i))!=0;
        visible("Gem",i,available);visible("Focus",i,available&&showFocus&&i==selected);visible("Disabled",i,available&&!usable);
        visible("Name",i,available&&labelsReady&&usable&&!requestFailed&&showFocus&&i==selected);
    }
    const unsigned status=halted||requestFailed?3:busy?(pendingWithdrawal?4:0):!ready?3:!snapshot.credits?1:!snapshot.cell?2:5;
    for(unsigned i=0;i<5;++i)visible("Status",i,i==status);
}
void __cdecl Refresh(const PluginContext*,void*) noexcept {
    uiQueued.store(false);
    try {std::lock_guard lock(mutex);if(!stopping)Update();}catch(...){ }
}
void QueueRefresh() {
    if(!uiQueued.exchange(true) && threads->runOnUiThread(ctx,Refresh,nullptr)!=Threads::Result::Success)uiQueued.store(false);
}
bool CursorEmpty(PlayerHandle p) {
    ItemHandle cursor{};const auto r=inventory->getCursorItem(ctx,p,&cursor);
    return (r==Inventory::Result::Success&&!cursor)||r==Inventory::Result::NotFound;
}
void QueueWork(bool withdraw,bool deposit=false);
void HostResult(const Rpc::Result& result) {
    try {
        std::lock_guard lock(mutex);
        if(stopping||(!open&&!directDeposit)||result.ticket.generation!=generation||result.ticket.presentation!=presentation||result.ticket.serial!=pendingSerial)return;
        const auto& reply=result.reply;const bool ok=(reply.flags&Protocol::Success)!=0;
        busy=false;halted|=(reply.flags&Protocol::Halted)!=0;ready=(reply.flags&Protocol::HasSnapshot)!=0;
        snapshot={};
        if(ready) {
            snapshot.bag=chosen;snapshot.credits=reply.credits;snapshot.width=reply.width;snapshot.height=reply.height;
            snapshot.bank=reply.bank;snapshot.definition=reply.definition;snapshot.gemMask=reply.gemMask;
            const auto definition=Catalog::Find(reply.bank,chosen);
            if(!definition||definition->fingerprint!=reply.definition||definition->mask!=reply.gemMask) {
                ready=false;requestFailed=true;directDeposit=false;message="Client and host bag definitions differ.";
                ctx->LogWarn("[GemBag/Client] Bag definitions differ from the host; reopen using matching game data.");QueueRefresh();return;
            }
            if(!(reply.gemMask&(1u<<selected)))for(unsigned i=0;i<8;++i)if(reply.gemMask&(1u<<i)){selected=i;break;}
            if(reply.flags&Protocol::HasCell)snapshot.cell=Cell{reply.cellX,reply.cellY};
        }
        message=reply.message;requestFailed=!ok;
        char line[320];std::snprintf(line,sizeof(line),"[GemBag/Client] %s reply success=%u credits=%u: %s",pendingDeposit?"Deposit":pendingWithdrawal?Gems[pendingGem].name:"Inspection",ok?1u:0u,snapshot.credits,message.c_str());
        if(ok)ctx->LogInfo(line);else ctx->LogWarn(line);
        if(directDeposit) {
            if(ok&&ready&&!pendingWithdrawal){QueueWork(true,true);return;}
            directDeposit=false;
        }
        QueueRefresh();
    }catch(...){std::lock_guard lock(mutex);busy=false;ready=false;directDeposit=false;message="The host result could not be displayed safely.";}
}
void QueueWork(bool withdraw,bool deposit) {
    const auto definition=Catalog::Find(snapshot.bank,chosen);
    if(stopping||(!open&&!directDeposit)||busy||halted)return;
    if(withdraw&&!ready)return;
    if(deposit&&(!definition||definition->deposits.empty())){directDeposit=false;ctx->LogWarn("[GemBag/Deposit] No compatible deposit recipes for this bag.");return;}
    if(withdraw&&!deposit&&(!snapshot.credits||!snapshot.cell||selected>=8||!(snapshot.gemMask&(1u<<selected))))return;
    Info now{};if(!Native::SafeUi(ctx)||!ReadInfo(chosen.handle,now)||!Same(now,chosen)){Close();return;}
    pendingGem=selected;pendingWithdrawal=withdraw;pendingDeposit=deposit;busy=true;requestFailed=false;pendingSerial=++serial;
    message=deposit?"Depositing gems and clusters...":withdraw?"Withdrawing gem...":"Reading the Gem Bag...";
    if(!transport.Begin(chosen,deposit?0:selected,withdraw,{generation,presentation,pendingSerial},deposit)) {
        busy=false;ready=false;directDeposit=false;message="A Gem Bag host request is pending or unavailable.";
    }
    Update();
}
void RefreshGemNames() {
    if(!localization&&ctx->QueryService(&localization)!=ServiceQueryResult::Success)localization=nullptr;
    const auto definition=Catalog::Find(snapshot.bank,chosen);
    std::array<uint32_t,8> ids{};
    if(definition)for(unsigned i=0;i<8;++i)ids[i]=definition->choices[i].nameId;
    GemLabelIssue issue{};
    const auto current=ReadGemLabels(ctx,localization,ids,definition?definition->mask:0,&issue);
    labelsReady=current&&Geometry::SetLabels(*current,localizedLabels);
    if(!labelsReady) {
        if(!labelWarningLogged) {
            char line[320];
            if(current)std::snprintf(line,sizeof(line),"[GemBag] Gem names hidden (%s); icon picker remains available.",Geometry::LastReason());
            else std::snprintf(line,sizeof(line),"[GemBag] Gem names hidden: gem=%u nameId=%u result=%u required=%u (%s); icon picker remains available.",issue.gem,issue.id,static_cast<unsigned>(issue.result),issue.required,issue.reason);
            ctx->LogWarn(line);labelWarningLogged=true;
        }
        return;
    }
    localizedLabels=*current;
    panelLabels[panel==ownedPanels[1]?1:0]=*current;
    if(!localizationLogged) {
        ctx->LogInfo("[GemBag] Active compiled item names applied; plain localized text in owned reserved buffers.");
        localizationLogged=true;
    }
}
bool Open(ItemHandle h,const Info& info,POINT anchor,bool controller) {
    transport.Tick();
    if(stopping||busy||directDeposit||transport.Pending()||!Supported(info)||!Catalog::Candidate(info)||!Native::SafeUi(ctx)){ctx->LogWarn("[GemBag] Open refused: busy, unsupported bag/location, or native UI/cursor guard.");return false;}
    PlayerHandle local{};
    if(inventory->getLocalPlayer(ctx,&local)!=Inventory::Result::Success||!local||!CursorEmpty(local)){ctx->LogWarn("[GemBag] Open refused: no local player or occupied cursor.");return false;}
    const auto definition=Catalog::Find(0,info);if(!definition)return false;
    const bool portable=!(definition->mask&128)||definition->choices[7].code!=Gems[7].code;
    panel=ownedPanels[portable?1:0];FullName=portable?"gem-bag-ui/SelectorPortable":"gem-bag-ui/Selector";Geometry::UsePortable(portable);
    localizedLabels=panelLabels[portable?1:0];
    const auto opened=panels->openPanel(ctx,panel);
    if(opened!=Panels::Result::Success){char line[120];std::snprintf(line,sizeof(line),"[GemBag] Native selector open failed, result=%u.",static_cast<unsigned>(opened));ctx->LogWarn(line);return false;}
    ctx->LogInfo("[GemBag] Native selector opened.");
    open=true;ready=false;chosen=info;chosen.handle=h;selected=0;snapshot={};++presentation;
    while(selected<7&&!(definition->mask&(1u<<selected)))++selected;
    popupAnchor=anchor;popupController=controller;positioned=false;showFocus=controller;requestFailed=false;labelsReady=false;
    QueueWork(false);return true;
}
void StartDeposit(const Info& bag) {
    if(stopping||open||busy||halted||directDeposit)return;
    transport.Tick();
    if(stopping||open||busy||halted||directDeposit||transport.Pending()||!Supported(bag)||!Catalog::Candidate(bag)||!Native::SafeUi(ctx))return;
    PlayerHandle local{};
    if(inventory->getLocalPlayer(ctx,&local)!=Inventory::Result::Success||!local||!CursorEmpty(local))return;
    chosen=bag;selected=0;snapshot={};ready=false;directDeposit=true;++presentation;
    QueueWork(false);
}
struct Find {uint32_t id{},cls{};Info item{};bool found{};};
Inventory::IterationAction __cdecl FindItem(const PluginContext*,const Info* i,void* data) noexcept {
    auto& f=*static_cast<Find*>(data);
    if(i&&i->runtimeId==f.id&&i->classId==f.cls&&Catalog::Candidate(*i)){f.item=*i;f.found=true;return Inventory::IterationAction::Stop;}
    return Inventory::IterationAction::Continue;
}
bool OpenFocused() {
    Find f{};PlayerHandle local{};
    if(!Native::Focus(ctx,f.id,f.cls)||inventory->getLocalPlayer(ctx,&local)!=Inventory::Result::Success)return false;
    const Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=StorageMask};
    if(inventory->forEachInventoryItem(ctx,local,&filter,FindItem,&f)!=Inventory::Result::Success||!f.found)return false;
    return Open(f.item.handle,f.item,{},true);
}
void MoveSelection(UiAction action) {
    const auto definition=Catalog::Find(snapshot.bank,chosen);if(!definition)return;
    unsigned next=selected;
    for(unsigned attempt=0;attempt<4;++attempt) {
        if(action==UiAction::Up||action==UiAction::Down)next=((selected^4)&4)|((selected+attempt)&3);
        else if(action==UiAction::Left)next=(next&4)|((next+3)&3);
        else next=(next&4)|((next+1)&3);
        if(definition->mask&(1u<<next)){selected=next;break;}
    }
    showFocus=true;Update();
}
void __cdecl Commands(const PluginContext*,void*) noexcept {
    try {
        std::lock_guard lock(mutex);commandQueued=false;
        while(!uiCommands.empty()) {
            const auto command=uiCommands.front();uiCommands.pop_front();
            if(stopping||command.generation!=generation)continue;
            if(command.action==UiAction::OpenFocused) {
                uint32_t id{},cls{};
                if(!open&&Native::Focus(ctx,id,cls)&&id==command.id&&cls==command.cls)OpenFocused();
                continue;
            }
            if(command.action==UiAction::OpenHovered||command.action==UiAction::DepositHovered) {
                Info current{};
                if(!open&&GetTickCount64()-command.tick<=400&&ReadInfo(command.hovered.handle,current)&&Same(current,command.hovered)) {
                    if(command.action==UiAction::DepositHovered)StartDeposit(current);
                    else Open(current.handle,current,command.point,false);
                }
                else ctx->LogWarn("[GemBag] Right-click hover expired or the bag changed before the UI callback.");
                continue;
            }
            if(!open||command.serial!=presentation)continue;
            switch(command.action) {
            case UiAction::Close:Close();break;
            case UiAction::Withdraw:QueueWork(true);break;
            case UiAction::ClickAt:case UiAction::HoverAt: {
                int hit=Ui::None;
                if(!positioned||!Geometry::Hit(command.point,hit)){ctx->LogWarn("[GemBag] Native picker hit test unavailable.");Close();break;}
                if(hit>=0&&hit<static_cast<int>(Gems.size())) {
                    selected=static_cast<unsigned>(hit);showFocus=true;
                    if(command.action==UiAction::ClickAt) {
                        char line[160];std::snprintf(line,sizeof(line),"[GemBag] Mouse selected %s at client=(%ld,%ld).",Gems[selected].name,command.point.x,command.point.y);ctx->LogInfo(line);QueueWork(true);
                    } else Update();
                } else if(command.action==UiAction::ClickAt&&(hit==Ui::CloseHit||hit==Ui::Outside))Close();
                else {showFocus=false;Update();}
                break;
            }
            case UiAction::Up:case UiAction::Down:case UiAction::Left:case UiAction::Right:MoveSelection(command.action);break;
            default:break;
            }
        }
    }catch(...){std::lock_guard lock(mutex);uiCommands.clear();commandQueued=false;}
}
bool QueueCommand(UiAction action,uint32_t id=0,uint32_t cls=0,POINT point={}) {
    if(stopping||uiCommands.size()>=32)return false;
    UiCommand command{action,generation,presentation,id,cls};command.point=point;
    if(action==UiAction::OpenHovered||action==UiAction::DepositHovered) {
        command.hovered=hover.bag;command.tick=hover.tick;
        if(!gameWindow||!ScreenToClient(gameWindow,&command.point))return false;
    }
    if(action==UiAction::HoverAt&&!uiCommands.empty()&&uiCommands.back().action==action)uiCommands.pop_back();
    uiCommands.push_back(command);
    if(commandQueued)return true;
    commandQueued=true;
    if(threads->runOnUiThread(ctx,Commands,nullptr)==Threads::Result::Success)return true;
    uiCommands.clear();commandQueued=false;return false;
}
bool HandleController(void* event) {
    if(!inputInstalled||stopping||!Native::Controller(ctx))return false;
    unsigned source{},raw{},index{},active{};uint8_t down{};
    const auto p=reinterpret_cast<uintptr_t>(event);
    if(!Read(p+0xc,source)||!Read(p+0x10,raw)||!Read(p+0x14,down)||down>1||!Read(ctx->exeBase+QolNativeProfile::ActiveIndexRva,active)||active>=8)return false;
    index=reinterpret_cast<unsigned(__fastcall*)(unsigned)>(ctx->exeBase+0x13e450)(source);
    if(index!=active)return false;
    auto input=reinterpret_cast<void*(__fastcall*)()>(ctx->exeBase+0x13ce90)();
    const unsigned key=reinterpret_cast<unsigned(__fastcall*)(void*,unsigned)>(ctx->exeBase+0x13d080)(input,raw);
    if(!key||key>0x8000||(key&(key-1)))return false;
    auto& state=gesture[index];const bool edge=state.Edge(key,down!=0);
    if(state.ReleaseOwned(key,down!=0))return true;
    if(!open) {
        const bool lb=(state.down&0x100)!=0;
        uint32_t id{},cls{};
        // Only discovered bag classes can own this press. Re-resolve the
        // complete item identity in the queued SDK UI callback.
        if(key==0x2000&&edge&&lb&&Native::Focus(ctx,id,cls)&&Catalog::CandidateClass(cls)&&QueueCommand(UiAction::OpenFocused,id,cls)){state.owned|=key;return true;}
        return false;
    }
    // A release for a key already delivered before this modal opened must
    // still reach the original path. Only complete new presses are owned.
    if(!down)return false;
    state.owned|=key;
    if(!edge)return true;
    if(key==0x2000)QueueCommand(UiAction::Close);
    else if(key==0x1000)QueueCommand(UiAction::Withdraw);
    else if(key==1)QueueCommand(UiAction::Up);
    else if(key==2)QueueCommand(UiAction::Down);
    else if(key==4)QueueCommand(UiAction::Left);
    else if(key==8)QueueCommand(UiAction::Right);
    return true;
}
bool FilterController(void* event) noexcept {
    __try {return HandleController(event);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool __fastcall ControllerCaller(void* event) noexcept {
    if(insideCaller||!originalCaller)return originalCaller?originalCaller(event):true;
    insideCaller=true;bool consumed=false;
    try {std::lock_guard lock(mutex);consumed=FilterController(event);}catch(...){ }
    insideCaller=false;
    return consumed?true:originalCaller(event);
}
LRESULT CALLBACK Mouse(int code,WPARAM removed,LPARAM value) {
    if(code>=0&&removed==PM_REMOVE&&value) {
        const auto& m=*reinterpret_cast<const MSG*>(value);
        std::lock_guard lock(mutex);
        if(m.message==WM_RBUTTONDOWN||m.message==WM_RBUTTONDBLCLK||m.message==WM_LBUTTONDOWN||m.message==WM_LBUTTONDBLCLK) {
            rightMouse=m.message==WM_RBUTTONDOWN||m.message==WM_RBUTTONDBLCLK;mouseTick=GetTickCount64();++mouseSerial;
            if(rightMouse&&rightSeen++<4) {
                char line[180];std::snprintf(line,sizeof(line),"[GemBag] Right-button observed; bagHover=%u age=%llu delta=(%ld,%ld).",hover.valid?1u:0u,static_cast<unsigned long long>(mouseTick-hover.tick),m.pt.x-hover.point.x,m.pt.y-hover.point.y);ctx->LogInfo(line);
            }
        }
        if(m.message==WM_RBUTTONUP&&consumedRight) {
            consumedRight=false;reinterpret_cast<MSG*>(value)->message=WM_NULL;
        } else if(!open&&!stopping&&(m.message==WM_RBUTTONDOWN||m.message==WM_RBUTTONDBLCLK)&&
            !(m.wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON))&&!(GetKeyState(VK_MENU)&0x8000)&&hover.valid&&
            FreshHover(hover.tick,GetTickCount64(),m.pt.x-hover.point.x,m.pt.y-hover.point.y)&&QueueCommand((m.wParam&MK_SHIFT)?UiAction::DepositHovered:UiAction::OpenHovered,0,0,m.pt)) {
            consumedRight=true;mouseUsed=mouseSerial;reinterpret_cast<MSG*>(value)->message=WM_NULL;
            ctx->LogInfo("[GemBag] Fresh bag right-click queued to the UI thread.");
        }
        if(m.message==WM_LBUTTONUP&&consumedLeft) {
            consumedLeft=false;reinterpret_cast<MSG*>(value)->message=WM_NULL;
        } else if(open&&(m.message==WM_LBUTTONDOWN||m.message==WM_LBUTTONDBLCLK)) {
            POINT point=m.pt;
            if(gameWindow&&ScreenToClient(gameWindow,&point)) {
                const bool queued=QueueCommand(UiAction::ClickAt,0,0,point);
                if(leftSeen++<8){char line[160];std::snprintf(line,sizeof(line),"[GemBag] Left-button observed at client=(%ld,%ld), queued=%u.",point.x,point.y,queued?1u:0u);ctx->LogInfo(line);}
            }
            consumedLeft=true;reinterpret_cast<MSG*>(value)->message=WM_NULL;
        } else if(open&&(m.message==WM_RBUTTONDOWN||m.message==WM_RBUTTONDBLCLK)) {
            QueueCommand(UiAction::Close);consumedRight=true;reinterpret_cast<MSG*>(value)->message=WM_NULL;
        }
        if(open&&m.message==WM_MOUSEMOVE) {
            POINT point=m.pt;if(gameWindow&&ScreenToClient(gameWindow,&point))QueueCommand(UiAction::HoverAt,0,0,point);
        }
        if(m.message==WM_KEYUP&&m.wParam<keyboardOwned.size()&&keyboardOwned[m.wParam]) {
            keyboardOwned[m.wParam]=false;reinterpret_cast<MSG*>(value)->message=WM_NULL;
        } else if(open&&m.message==WM_KEYDOWN&&m.wParam<keyboardOwned.size()) {
            bool consumed=false;const bool repeated=(m.lParam&(1LL<<30))!=0;
            if(m.wParam==VK_ESCAPE)consumed=repeated||QueueCommand(UiAction::Close);
            else if(m.wParam==VK_RETURN||m.wParam==VK_SPACE)consumed=repeated||QueueCommand(UiAction::Withdraw);
            else if(m.wParam==VK_UP)consumed=QueueCommand(UiAction::Up);
            else if(m.wParam==VK_DOWN)consumed=QueueCommand(UiAction::Down);
            else if(m.wParam==VK_LEFT)consumed=QueueCommand(UiAction::Left);
            else if(m.wParam==VK_RIGHT)consumed=QueueCommand(UiAction::Right);
            if(consumed){keyboardOwned[m.wParam]=true;reinterpret_cast<MSG*>(value)->message=WM_NULL;}
        }
    }
    return CallNextHookEx(mouseHook,code,removed,value);
}
void InstallMouse() {
    if(mouseHook)return;
    HWND window{};
    EnumWindows([](HWND candidate,LPARAM data)->BOOL {
        DWORD process{};GetWindowThreadProcessId(candidate,&process);
        if(process==GetCurrentProcessId()&&IsWindowVisible(candidate)&&!GetWindow(candidate,GW_OWNER)){*reinterpret_cast<HWND*>(data)=candidate;return FALSE;}return TRUE;
    },reinterpret_cast<LPARAM>(&window));
    if(window){gameWindow=window;mouseHook=SetWindowsHookExW(WH_GETMESSAGE,Mouse,nullptr,GetWindowThreadProcessId(window,nullptr));}
    if(mouseHook)ctx->LogInfo("[GemBag] Game-window right-click input installed.");
    else {char line[128];std::snprintf(line,sizeof(line),"[GemBag] Game-window input unavailable, error=%lu.",GetLastError());ctx->LogWarn(line);}
}
ItemInteractions::Decision __cdecl Interaction(const PluginContext*,const ItemInteractions::ItemInteractionEvent* e,void*) noexcept {
    try {
        std::lock_guard lock(mutex);
        if(stopping||!e||e->structSize<ItemInteractions::ItemInteractionEventSize||e->inputSource!=ItemInteractions::InputSource::KeyboardMouse||
            e->action!=ItemInteractions::Action::Activate||!rightMouse||mouseSerial==mouseUsed||GetTickCount64()-mouseTick>500)return ItemInteractions::Decision::Continue;
        Info info{};if(!ReadInfo(e->item,info)||!Catalog::Candidate(info))return ItemInteractions::Decision::Continue;
        mouseUsed=mouseSerial;
        POINT point{};GetCursorPos(&point);ScreenToClient(gameWindow,&point);
        return Open(e->item,info,point,false)?ItemInteractions::Decision::Consume:ItemInteractions::Decision::Continue;
    }catch(...){return ItemInteractions::Decision::Continue;}
}
SharedEvents::UiMessageAction __cdecl Message(const PluginContext*,const SharedEvents::UiMessageEvent* e,void*) noexcept {
    try {
        if(!e||!e->target||!e->command||!e->text||std::strcmp(e->target,"PanelManager")||std::strcmp(e->command,"OpenPanel"))return SharedEvents::UiMessageAction::Continue;
        const std::string_view text(e->text);
        const bool withdrawal=text.starts_with("gem-bag-ui/Withdraw")&&text.size()==std::string_view("gem-bag-ui/Withdraw").size()+1&&text.back()>='0'&&text.back()<='7';
        if(text!="gem-bag-ui/Close"&&!withdrawal)return SharedEvents::UiMessageAction::Continue;
        std::lock_guard lock(mutex);
        if(text=="gem-bag-ui/Close")Close();
        else if(text.size()==std::string_view("gem-bag-ui/Withdraw").size()+1&&text.back()>='0'&&text.back()<='7'&&open){selected=static_cast<unsigned>(text.back()-'0');QueueWork(true);}
        return SharedEvents::UiMessageAction::Consume;
    }catch(...){return SharedEvents::UiMessageAction::Consume;}
}
void __cdecl Tooltip(const PluginContext*,SharedEvents::ItemTooltipEvent* e,void*) noexcept {
    try {
        std::lock_guard lock(mutex);Info info{};
        if(stopping||!e||!e->text)return;
        if(!mouseHook)InstallMouse();
        if(!ReadInfo(e->item,info)||!Supported(info)||!Catalog::Candidate(info)){hover.valid=false;return;}
        hover={info,GetTickCount64(),{},true};GetCursorPos(&hover.point);
        if(!tipLogged){tipLogged=true;ctx->LogInfo("[GemBag] Bag tooltip received; right-click target is ready.");}
        const char* text=Native::Controller(ctx)?"LB+B: Choose gem":"Right-click: Choose gem\nShift+right-click: Deposit gems + clusters";
        const size_t n=std::strlen(text);
        if(n+1<=e->capacity){std::memcpy(e->text,text,n+1);e->length=static_cast<uint32_t>(n);}
    }catch(...){ }
}
void __cdecl Draw(const PluginContext*,const Overlay::Frame* f,void*) noexcept {
    try {
        std::lock_guard lock(mutex);
        if(stopping||!f)return;
        transport.Tick();
        if(!open)return;
        const auto now=GetTickCount64();
        if(now-lastRefresh>100){lastRefresh=now;QueueRefresh();}
        // Rendering, labels and disabled shading all belong to the native
        // widget tree. The SDK frame is only an optional stale-state refresh.
        if(!frameLogged){frameLogged=true;ctx->LogInfo("[GemBag] SDK frame refresh received; native widgets own all picker rendering.");}
    }catch(...){ }
}
void __cdecl Ready(const PluginContext*,const Lifecycle::GameplayEvent* e,void*) noexcept {
    try {std::lock_guard lock(mutex);if(stopping||!e)return;generation=e->sessionGeneration;transport.ResetClient();InstallMouse();}catch(...){ }
}
void __cdecl Left(const PluginContext*,const Lifecycle::GameplayEvent*,void*) noexcept {
    std::lock_guard lock(mutex);++generation;Close();transport.ResetClient();busy=false;halted=false;chosen={};snapshot={};hover={};tipLogged=false;localizationLogged=false;labelsReady=false;labelWarningLogged=false;rightSeen=0;uiCommands.clear();for(auto& g:gesture)g={};
}
bool DedicatedLaunch() {
    int count{};auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);bool dedicated{};
    if(arguments){for(int i=1;i<count;++i)if(!_wcsicmp(arguments[i],L"-d2rds"))dedicated=true;LocalFree(arguments);}
    return dedicated;
}
void PumpHostGame(void*) noexcept {try {transport.PumpScopedGame();}catch(...){ }}
bool Load(const PluginContext* c,bool dedicated=DedicatedLaunch()) {
    ctx=c;stopping=false;
    if(!Native::Initialize(c)){c->LogWarn("[GemBag] Compatibility checks failed; see [GemBag/Admission] entries above. Selector disabled; ordinary Gem Bag recipes remain available.");return true;}
    if(!Catalog::Initialize(c)){c->LogWarn("[GemBag/Data] Compiled-table lifecycle unavailable; selector disabled.");return true;}
    const NetworkService* network{};
    if(c->QueryService(&items)!=ServiceQueryResult::Success||!HasItemServiceField(items,ItemServiceCapabilitiesFieldEnd)||
        c->QueryService(&inventory)!=ServiceQueryResult::Success||!HasInventoryServiceField(inventory,InventoryServiceRequiredSize)||
        c->QueryService(&threads)!=ServiceQueryResult::Success||!HasThreadServiceField(threads,ThreadServiceRequiredSize)||
        c->QueryService(&network)!=ServiceQueryResult::Success)return false;
    if(!items->getItemInfo||!items->createItem||!items->editNativeItem||!items->destroyItem)return false;
    if(!transport.Start(c,network,threads,inventory,items,dedicated?nullptr:HostResult))return false;
    const bool scopedPump=ServerFrame::Install(c,PumpHostGame);transport.EnableScopedPump(scopedPump);
    if(!scopedPump)c->LogWarn("[GemBag/Host] Per-game frame bridge unavailable; ordinary SDK game scheduling retained.");
    if(dedicated){c->LogInfo("[GemBag/Host] Server service loaded; bag inspection, space checks, debit and identified gem creation run for the sending peer.");return true;}
    if(c->QueryService(&panels)!=ServiceQueryResult::Success||
        c->QueryService(&resources)!=ServiceQueryResult::Success||c->QueryService(&widgets)!=ServiceQueryResult::Success||
        c->QueryService(&events)!=ServiceQueryResult::Success||c->QueryService(&interactions)!=ServiceQueryResult::Success||
        c->QueryService(&lifecycle)!=ServiceQueryResult::Success||
        c->QueryService(&overlay)!=ServiceQueryResult::Success)return false;
    if(!HasInventoryServiceField(inventory,InventoryServiceRequiredSize)||!HasPanelServiceField(panels,PanelServiceRequiredSize)||
       !HasResourceServiceField(resources,ResourceServiceRequiredSize)||!HasWidgetServiceField(widgets,WidgetServiceRequiredSize)||
       !HasSharedEventServiceField(events,SharedEventServiceRequiredSize)||!HasThreadServiceField(threads,ThreadServiceRequiredSize)||
       !HasLifecycleServiceField(lifecycle,LifecycleServiceRequiredSize)||!HasOverlayServiceField(overlay,OverlayServiceRequiredSize))return false;
    if(!items->getItemInfo||!items->createItem||!items->editNativeItem||!items->destroyItem)return false;
    if(c->QueryService(&localization)!=ServiceQueryResult::Success)localization=nullptr;
    if(!Geometry::Initialize(c,widgets)){c->LogWarn("[GemBag] Native picker geometry profile unavailable; ordinary bag recipes remain available.");return true;}
    // Keep both panel variants and input-mode layouts present during Load.
    // Owned fixed-capacity text reserves are populated after native creation.
    const char* paths[]={"data/global/ui/layouts/gem-bag-ui/Selectorhd.json","data/global/ui/layouts/controller/gem-bag-ui/Selectorhd.json",
        "data/global/ui/layouts/gem-bag-ui/SelectorPortablehd.json","data/global/ui/layouts/controller/gem-bag-ui/SelectorPortablehd.json"};
    for(unsigned variant=0;variant<2;++variant) {
        const auto layout=Layout(ReservedGemLabels(),variant==1);
        for(unsigned mode=0;mode<2;++mode) {
            const auto i=variant*2+mode;
            const Resources::ResourceRegistration r{.structSize=Resources::ResourceRegistrationSize,.path=paths[i],.bytes=layout.data(),.byteCount=layout.size()};
            if(resources->registerResource(c,&r,&resource[i])!=Resources::Result::Success||!resource[i])return false;
        }
        const Panels::PanelRegistration p{.structSize=Panels::PanelRegistrationSize,.flags=Panels::PanelFlags::CloseOnEscape,.localId=variant?"SelectorPortable":"Selector"};
        if(panels->registerPanel(c,&p,&ownedPanels[variant])!=Panels::Result::Success)return false;
    }
    panel=ownedPanels[0];
    const SharedEvents::UiMessageListener msg{.structSize=SharedEvents::UiMessageListenerSize,.priority=5000,.callback=Message};
    if(events->registerUiMessageListener(c,&msg,&uiListener)!=SharedEvents::Result::Success)return false;
    const SharedEvents::ItemTooltipListener tip{.structSize=SharedEvents::ItemTooltipListenerSize,.slot=130,.region=SharedEvents::ItemTooltipRegion::ActionFooter,
        .position=SharedEvents::ItemTooltipPosition::Bottom,.anchor=SharedEvents::ItemTooltipAnchor::None,.fallback=SharedEvents::ItemTooltipFallback::Omit,.callback=Tooltip};
    if(events->registerItemTooltipListener(c,&tip,&tooltipListener)!=SharedEvents::Result::Success)return false;
    if(!HasItemInteractionServiceField(interactions,ItemInteractionServiceSupportedContainerMaskFieldEnd) || (interactions->supportedContainerMask&StorageMask)!=StorageMask)return false;
    const ItemInteractions::ItemInteractionListener click{.structSize=ItemInteractions::ItemInteractionListenerSize,.priority=5000,.callback=Interaction,.containerMask=StorageMask};
    if(interactions->registerListener(c,&click,&interactionListener)!=ItemInteractions::Result::Success)return false;
    Lifecycle::GameplayEventListener life{.structSize=Lifecycle::GameplayEventListenerSize,.kind=Lifecycle::GameplayEventKind::LocalPlayerReady,.callback=Ready};
    if(lifecycle->registerGameplayEventListener(c,&life,&readyListener)!=Lifecycle::Result::Success)return false;
    life.kind=Lifecycle::GameplayEventKind::GameLeft;life.callback=Left;
    if(lifecycle->registerGameplayEventListener(c,&life,&leftListener)!=Lifecycle::Result::Success)return false;
    const Overlay::CallbackRegistration frame{.structSize=Overlay::CallbackRegistrationSize,.phase=Overlay::Phase::AfterGameUi,.priority=-100,.callback=Draw};
    if(overlay->registerFrameCallback(c,&frame,&drawListener)!=Overlay::Result::Success)return false;
    // A distinct upstream caller, before Controller QOL's event entry hook.
    // Never install over an existing caller detour or replace QOL's hook.
    if(c->CheckExpectedBytes(QolNativeProfile::EventCallerRva,QolNativeProfile::EventCallerBytes,sizeof(QolNativeProfile::EventCallerBytes)) &&
        c->CheckExpectedBytes(0x13d080,Profile::KeyTranslator,sizeof(Profile::KeyTranslator)) &&
        c->CheckExpectedBytes(0x13e450,Profile::DeviceMapperPrefix,sizeof(Profile::DeviceMapperPrefix)) &&
        c->CheckExpectedBytes(QolNativeProfile::InputRva,QolNativeProfile::InputBytes,sizeof(QolNativeProfile::InputBytes))) {
        inputInstalled=c->InstallInlineHook<Caller>(QolNativeProfile::EventCallerRva,QolNativeProfile::EventCallerBytes,18,&ControllerCaller,&originalCaller);
    }
    c->LogInfo(inputInstalled?"[GemBag] Eight-gem selector loaded: right-click bag or LB+B; withdrawals use the bag's exact storage page.":"[GemBag] Eight-gem mouse selector loaded; controller caller unavailable, existing controller input retained.");
    return true;
}
void Unload() {
    {std::lock_guard lock(mutex);stopping=true;inputInstalled=false;uiCommands.clear();++generation;++serial;}
    transport.Stop();
    ServerFrame::Stop();
    Catalog::Stop();
    if(mouseHook){UnhookWindowsHookEx(mouseHook);mouseHook=nullptr;}
    // SDK unregisters callbacks, hooks, panel and resources with owner lifetime.
    // Avoid holding the plugin lock while callbacks drain.
    if(overlay&&drawListener)overlay->unregisterFrameCallback(ctx,drawListener);
    if(interactions&&interactionListener)interactions->unregisterListener(ctx,interactionListener);
    if(events&&uiListener)events->unregisterUiMessageListener(ctx,uiListener);
    if(events&&tooltipListener)events->unregisterItemTooltipListener(ctx,tooltipListener);
    if(lifecycle&&readyListener)lifecycle->unregisterGameplayEventListener(ctx,readyListener);
    if(lifecycle&&leftListener)lifecycle->unregisterGameplayEventListener(ctx,leftListener);
    for(auto h:ownedPanels)if(panels&&h)panels->unregisterPanel(ctx,h);
    for(auto h:resource)if(resources&&h)resources->unregisterResource(ctx,h);
}
constexpr PluginInfo InfoBlock{.infoSize=PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,.id="gem-bag-ui",.name="Gem Bag UI",.version="1.3.1+rev.1",
    .author="PyreFly",.description="Choose any gem from a Gem Bag with server-authoritative withdrawal.",.flags=PluginFlags::Shared|PluginFlags::NativeHooks};
}
extern "C" __declspec(dllexport) const D2RL::PluginInfo* D2RLoaderGetPluginInfo() noexcept {return &InfoBlock;}
extern "C" __declspec(dllexport) bool D2RLoaderLoadPlugin(const D2RL::PluginContext* c) noexcept {
    if(!c||c->contextSize<sizeof(D2RL::PluginContext)||c->abiVersion!=D2RL_PLUGIN_ABI_VERSION||!D2RL::HasApi(c->api))return false;
    try {if(Load(c))return true;}catch(...){ }
    Unload();return false;
}
extern "C" __declspec(dllexport) void D2RLoaderUnloadPlugin() noexcept {try{Unload();}catch(...){ }}
