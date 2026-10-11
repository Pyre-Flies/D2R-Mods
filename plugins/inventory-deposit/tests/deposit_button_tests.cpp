#include "deposit_button.h"
#include "button_art.h"
#include "button_art_resource.h"
#include <D2RLPlugin/api.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {
constexpr char LayoutPath[]="data/global/ui/layouts/inventory-deposit/DepositAllButtonhd.json";
constexpr char ArtPath[]="data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.sprite";
constexpr char LowendArtPath[]="data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.lowend.sprite";
constexpr unsigned char ArtBytes[]{0x53,0x70,0x41,0x31,0x00,0xff,0x0a,0x80,0x00};
constexpr unsigned char LowendArtBytes[]{0x53,0x70,0x41,0x31,0x00,0xee,0x0d,0x90,0x00};
void Check(bool ok,const char* message) {
    if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
using Action=D2RL::SharedEvents::UiMessageAction;
using Service=D2RL::ServiceId;
struct Task {const D2RL::PluginContext* context;D2RL::Threads::Callback callback;void* data;};
struct Fixture;
Fixture* fixture{};

struct Fixture {
    D2RL::PluginApi api{};
    D2RL::PluginContext context{};
    D2RL::ResourceService resources{};
    D2RL::PanelService panels{};
    D2RL::WidgetService widgets{};
    D2RL::SharedEventService events{};
    D2RL::ThreadService threads{};
    D2RL::LifecycleService lifecycle{};
    Service missing=static_cast<Service>(0);
    std::map<uint64_t,std::string> liveResources;
    std::map<uint64_t,std::string> liveChildren;
    std::map<std::string,std::string> resourceBytes;
    std::map<uint64_t,D2RL::Lifecycle::GameplayEventListener> gameplay;
    D2RL::SharedEvents::UiMessageListener listener{};
    uint64_t listenerHandle{},nextHandle=1;
    unsigned resourceAttempts{},failResourceAttempt{},childAttempts{},listenerAttempts{};
    unsigned artLoadAttempts{},failArtLoadAttempt{},malformedArt{};
    unsigned gameplayAttempts{},failGameplayAttempt{};
    bool failChild{},failListener{},rejectQueue{},missingWidget{},rejectVisible{},rejectEnabled{},uiThread{};
    bool visible{},enabled=true,requestResult=true;
    unsigned requests{},uiCalls{},stateWrites{},queues{};
    bool requirePosition{},positionResult=true;
    bool stateResult=true;
    unsigned visualCalls{};
    std::map<uint64_t,bool> visualEnablement;
    const char* refusedPositionPanel{};
    unsigned positionCalls{},positionsAtRequest{};
    std::map<uint64_t,bool> positioned;
    std::deque<Task> tasks;
    std::map<uint64_t,bool> visibility,enablement;
    std::vector<std::string> widgetNames;

    Fixture();
    ~Fixture() {InventoryDepositButton::Shutdown();Check(Clean(),"shutdown releases all owned registrations");fixture=nullptr;}
    bool Clean() const {return liveResources.empty() && liveChildren.empty() && !listenerHandle && gameplay.empty();}
    bool Start(InventoryDepositButton::Positioner position=nullptr) {
        return InventoryDepositButton::Initialize(&context,&Visible,&Enabled,&Request,860,1620,position,&ApplyState);
    }
    void Pump() {
        unsigned budget=32;
        while(!tasks.empty()) {
            Check(budget--!=0,"UI work terminates without endlessly rescheduling itself");
            const auto task=tasks.front();tasks.pop_front();Run(task);
        }
    }
    void Run(const Task& task) {
        Check(!uiThread,"fixture must not nest UI callbacks");uiThread=true;
        task.callback(task.context,task.data);uiThread=false;
    }
    Action Send(const char* target,const char* command,const char* text,
                uint32_t size=D2RL::SharedEvents::UiMessageEventSize) {
        Check(listener.callback!=nullptr,"UI message listener is registered");
        D2RL::SharedEvents::UiMessageEvent event{};
        event.structSize=size;event.target=target;event.command=command;event.text=text;
        uiThread=true;const auto result=listener.callback(&context,&event,listener.userData);uiThread=false;
        return result;
    }
    void Gameplay(D2RL::Lifecycle::GameplayEventKind kind) {
        D2RL::Lifecycle::GameplayEvent event{};event.structSize=sizeof(event);event.kind=kind;
        uiThread=true;
        for(const auto& [handle,entry]:gameplay) {
            (void)handle;if(entry.kind==kind)entry.callback(&context,&event,entry.userData);
        }
        uiThread=false;
    }
    static bool Visible() noexcept {Check(fixture->uiThread,"visibility predicate runs on UI thread");return fixture->visible;}
    static bool Enabled() noexcept {Check(fixture->uiThread,"enabled predicate runs on UI thread");return fixture->enabled;}
    static bool ApplyState(const char* panelName,uint64_t parent,uint64_t button,bool enabled) noexcept {
        auto& f=*fixture;
        Check(f.uiThread && panelName && parent && button==parent+1,
            "visual state callback uses the live widget on the UI thread");
        Check(f.enablement.contains(button) && f.enablement[button]==enabled,
            "SDK input enablement is set before synchronizing native button visuals");
        if(f.requirePosition)Check(f.positioned[button],"position is validated before applying button visuals");
        ++f.visualCalls;
        if(!f.stateResult)return false;
        f.visualEnablement[button]=enabled;
        return true;
    }
    static bool Request() noexcept {
        Check(fixture->uiThread,"deposit request runs on UI thread");
        ++fixture->requests;fixture->positionsAtRequest=fixture->positionCalls;return fixture->requestResult;
    }
    static bool Position(const char* panelName,uint64_t parent,uint64_t button) noexcept {
        Check(fixture->uiThread,"position callback runs only on the UI thread");
        Check(panelName && parent && button==parent+1,"position callback receives the live panel and its child handles");
        Check((std::strcmp(panelName,"PlayerInventoryExpansionLayout")==0 && parent==101) ||
              (std::strcmp(panelName,"PlayerInventoryOriginalLayout")==0 && parent==201),
              "position callback names the matching inventory variant");
        ++fixture->positionCalls;
        const bool accepted=fixture->positionResult && (!fixture->refusedPositionPanel ||
            std::strcmp(panelName,fixture->refusedPositionPanel)!=0);
        fixture->positioned[button]=accepted;
        return accepted;
    }
};

D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,Service id,uint32_t version,const void** result) noexcept {
    *result=nullptr;
    if(id==fixture->missing)return D2RL::ServiceQueryResult::Unavailable;
    Check(version==1,"optional SDK services request the supported ABI");
    switch(id) {
        case Service::Resource:*result=&fixture->resources;break;
        case Service::Panel:*result=&fixture->panels;break;
        case Service::Widget:*result=&fixture->widgets;break;
        case Service::SharedEvent:*result=&fixture->events;break;
        case Service::Thread:*result=&fixture->threads;break;
        case Service::Lifecycle:*result=&fixture->lifecycle;break;
        default:return D2RL::ServiceQueryResult::UnknownService;
    }
    return D2RL::ServiceQueryResult::Success;
}
D2RL::Resources::Result __cdecl RegisterResource(const D2RL::PluginContext*,const D2RL::Resources::ResourceRegistration* reg,uint64_t* handle) noexcept {
    Check(reg && reg->structSize>=D2RL::Resources::ResourceRegistrationRequiredSize && !reg->flags,"resource registration honors SDK contract");
    Check(reg->path && reg->bytes && reg->byteCount,"resource contains a path and bytes");
    ++fixture->resourceAttempts;
    const std::string path=reg->path;
    const char* expectedPaths[]{ArtPath,LowendArtPath,LayoutPath};
    Check(fixture->liveResources.size()<3 && path==expectedPaths[fixture->liveResources.size()],
        "unique owned HD and lowend artwork is registered before its inventory layout");
    if(fixture->resourceAttempts==fixture->failResourceAttempt)return D2RL::Resources::Result::Unavailable;
    *handle=fixture->nextHandle++;
    fixture->liveResources.emplace(*handle,path);
    fixture->resourceBytes[path]=std::string(static_cast<const char*>(reg->bytes),static_cast<size_t>(reg->byteCount));
    return D2RL::Resources::Result::Success;
}
D2RL::Resources::Result __cdecl UnregisterResource(const D2RL::PluginContext*,uint64_t handle) noexcept {
    Check(fixture->liveChildren.empty(),"child registration is removed before its layout or artwork");
    Check(!fixture->liveResources.empty() && fixture->liveResources.rbegin()->first==handle,
        "layout and artwork registrations unwind in reverse dependency order");
    Check(fixture->liveResources.erase(handle)==1,"resource cleanup uses a live owned handle once");
    return D2RL::Resources::Result::Success;
}
D2RL::Panels::Result __cdecl RegisterChild(const D2RL::PluginContext*,const D2RL::Panels::ChildLayoutRegistration* reg,uint64_t* handle) noexcept {
    ++fixture->childAttempts;
    Check(reg && reg->structSize>=D2RL::Panels::ChildLayoutRegistrationRequiredSize && !reg->reserved,"child registration honors SDK contract");
    Check(reg->stockPanel==D2RL::Panels::StockPanel::PlayerInventory,"button composes with the stock player inventory");
    Check(reg->flags==D2RL::Panels::ChildLayoutFlags::KeyboardMouseOnly,"button leaves the controller layout alone");
    Check(reg->localId && *reg->localId,"child registration has a plugin-local identity");
    Check(fixture->liveResources.size()==3,"both art variants and the layout exist before child registration");
    if(fixture->failChild)return D2RL::Panels::Result::Unavailable;
    *handle=fixture->nextHandle++;fixture->liveChildren.emplace(*handle,reg->localId);
    return D2RL::Panels::Result::Success;
}
D2RL::Panels::Result __cdecl UnregisterChild(const D2RL::PluginContext*,uint64_t handle) noexcept {
    Check(fixture->liveChildren.erase(handle)==1,"child cleanup uses a live owned handle once");
    return D2RL::Panels::Result::Success;
}
D2RL::SharedEvents::Result __cdecl RegisterListener(const D2RL::PluginContext*,const D2RL::SharedEvents::UiMessageListener* listener,uint64_t* handle) noexcept {
    ++fixture->listenerAttempts;
    Check(listener && listener->structSize>=D2RL::SharedEvents::UiMessageListenerRequiredSize && listener->callback,"listener registration honors SDK contract");
    if(fixture->failListener)return D2RL::SharedEvents::Result::Unavailable;
    Check(!fixture->listenerHandle,"there is only one button listener");
    fixture->listener=*listener;*handle=fixture->listenerHandle=fixture->nextHandle++;
    return D2RL::SharedEvents::Result::Success;
}
D2RL::SharedEvents::Result __cdecl UnregisterListener(const D2RL::PluginContext*,uint64_t handle) noexcept {
    Check(handle && handle==fixture->listenerHandle,"listener cleanup uses its live owned handle once");
    fixture->listenerHandle=0;fixture->listener={};return D2RL::SharedEvents::Result::Success;
}
D2RL::Lifecycle::Result __cdecl RegisterGameplay(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEventListener* listener,uint64_t* handle) noexcept {
    ++fixture->gameplayAttempts;
    Check(listener && listener->structSize>=D2RL::Lifecycle::GameplayEventListenerRequiredSize && listener->callback,"gameplay listener honors SDK contract");
    Check(listener->kind==D2RL::Lifecycle::GameplayEventKind::GameJoined || listener->kind==D2RL::Lifecycle::GameplayEventKind::GameLeft,"button subscribes to session boundaries");
    if(fixture->gameplayAttempts==fixture->failGameplayAttempt)return D2RL::Lifecycle::Result::Unavailable;
    *handle=fixture->nextHandle++;fixture->gameplay.emplace(*handle,*listener);return D2RL::Lifecycle::Result::Success;
}
D2RL::Lifecycle::Result __cdecl UnregisterGameplay(const D2RL::PluginContext*,uint64_t handle) noexcept {
    Check(fixture->gameplay.erase(handle)==1,"gameplay cleanup uses a live owned handle once");return D2RL::Lifecycle::Result::Success;
}
void UiCall() {Check(fixture->uiThread,"all widget service calls run on the UI thread");++fixture->uiCalls;}
D2RL::Widgets::Result __cdecl FindPanel(const D2RL::PluginContext*,const char* name,uint64_t* handle) noexcept {
    UiCall();Check(name && *name,"widget lookup has a panel name");
    fixture->widgetNames.emplace_back(name);
    if(fixture->missingWidget)return D2RL::Widgets::Result::NotFound;
    *handle=std::strcmp(name,"PlayerInventoryExpansionLayout")==0?101:201;
    return D2RL::Widgets::Result::Success;
}
D2RL::Widgets::Result __cdecl FindWidget(const D2RL::PluginContext*,uint64_t parent,const char* name,uint64_t* handle) noexcept {
    UiCall();Check(parent && name && *name,"widget lookup is scoped to a parent");fixture->widgetNames.emplace_back(name);
    if(fixture->missingWidget)return D2RL::Widgets::Result::NotFound;
    *handle=parent+1;return D2RL::Widgets::Result::Success;
}
D2RL::Widgets::Result __cdecl SetVisible(const D2RL::PluginContext*,uint64_t handle,bool value) noexcept {
    UiCall();Check(handle!=0,"visibility writes target a valid widget handle");++fixture->stateWrites;
    if(value && fixture->requirePosition) {
        Check(fixture->positioned[handle],"position is admitted before showing the child");
        fixture->positioned[handle]=false;
    }
    if(fixture->missingWidget)return D2RL::Widgets::Result::NotFound;
    if(fixture->rejectVisible)return D2RL::Widgets::Result::Unavailable;
    fixture->visibility[handle]=value;
    return D2RL::Widgets::Result::Success;
}
D2RL::Widgets::Result __cdecl SetEnabled(const D2RL::PluginContext*,uint64_t handle,bool value) noexcept {
    UiCall();Check(handle!=0,"enabled writes target a valid widget handle");++fixture->stateWrites;
    if(value && fixture->requirePosition)
        Check(fixture->positioned[handle],"position is admitted before enabling the child");
    if(fixture->missingWidget)return D2RL::Widgets::Result::NotFound;
    if(fixture->rejectEnabled)return D2RL::Widgets::Result::Unavailable;
    fixture->enablement[handle]=value;
    // The real SDK setter only changes native byte +0x50, leaving the
    // ButtonWidget visual-disabled flag and sprite frame unchanged.
    return D2RL::Widgets::Result::Success;
}
D2RL::Threads::Result __cdecl Queue(const D2RL::PluginContext* context,D2RL::Threads::Callback callback,void* data) noexcept {
    Check(callback!=nullptr,"queued work has a callback");++fixture->queues;
    if(fixture->rejectQueue)return D2RL::Threads::Result::Unavailable;
    fixture->tasks.push_back({context,callback,data});return D2RL::Threads::Result::Success;
}
Fixture::Fixture() {
    Check(fixture==nullptr,"fixtures run serially");fixture=this;
    api.apiSize=sizeof(api);api.queryService=&Query;
    context.contextSize=sizeof(context);context.api=&api;
    resources.serviceSize=sizeof(resources);resources.serviceVersion=1;
    resources.registerResource=&RegisterResource;resources.unregisterResource=&UnregisterResource;
    panels.serviceSize=sizeof(panels);panels.serviceVersion=1;
    panels.registerChildLayout=&RegisterChild;panels.unregisterChildLayout=&UnregisterChild;
    widgets.serviceSize=sizeof(widgets);widgets.serviceVersion=1;
    widgets.findPanel=&FindPanel;widgets.findWidget=&FindWidget;
    widgets.setWidgetVisible=&SetVisible;widgets.setWidgetEnabled=&SetEnabled;
    events.serviceSize=sizeof(events);events.serviceVersion=1;
    events.registerUiMessageListener=&RegisterListener;events.unregisterUiMessageListener=&UnregisterListener;
    threads.serviceSize=sizeof(threads);threads.serviceVersion=1;threads.runOnUiThread=&Queue;
    lifecycle.serviceSize=sizeof(lifecycle);lifecycle.serviceVersion=1;
    lifecycle.registerGameplayEventListener=&RegisterGameplay;lifecycle.unregisterGameplayEventListener=&UnregisterGameplay;
}

constexpr const char* ClickText="inventory-deposit/DepositAll";
Action Click(Fixture& f) {return f.Send("PanelManager","OpenPanel",ClickText);}
void CheckState(const std::map<uint64_t,bool>& values,bool expected,const char* message) {
    Check(!values.empty(),message);for(const auto& [handle,value]:values) { (void)handle;Check(value==expected,message); }
}
void CapabilityTests() {
    {Fixture f;Check(!InventoryDepositButton::Initialize(nullptr,&Fixture::Visible,&Fixture::Enabled,&Fixture::Request),"null context declines the optional button");}
    for(const auto id:{Service::Resource,Service::Panel,Service::Widget,Service::SharedEvent,Service::Thread,Service::Lifecycle}) {
        Fixture f;f.missing=id;Check(!f.Start() && f.Clean(),"missing optional service leaves no partial button");
    }
    for(unsigned test=0;test<12;++test) {
        Fixture f;
        uint32_t* size[]{&f.resources.serviceSize,&f.panels.serviceSize,&f.widgets.serviceSize,&f.events.serviceSize,&f.threads.serviceSize,&f.lifecycle.serviceSize};
        uint32_t* version[]{&f.resources.serviceVersion,&f.panels.serviceVersion,&f.widgets.serviceVersion,&f.events.serviceVersion,&f.threads.serviceVersion,&f.lifecycle.serviceVersion};
        if(test<6)*size[test]=8;else *version[test-6]=99;
        Check(!f.Start() && f.Clean(),"truncated or unsupported service ABI leaves native inventory untouched");
    }
    for(unsigned test=0;test<13;++test) {
        Fixture f;
        switch(test) {
            case 0:f.resources.registerResource=nullptr;break;
            case 1:f.resources.unregisterResource=nullptr;break;
            case 2:f.panels.registerChildLayout=nullptr;break;
            case 3:f.panels.unregisterChildLayout=nullptr;break;
            case 4:f.widgets.findPanel=nullptr;break;
            case 5:f.widgets.findWidget=nullptr;break;
            case 6:f.widgets.setWidgetVisible=nullptr;break;
            case 7:f.widgets.setWidgetEnabled=nullptr;break;
            case 8:f.events.registerUiMessageListener=nullptr;break;
            case 9:f.events.unregisterUiMessageListener=nullptr;break;
            case 10:f.threads.runOnUiThread=nullptr;break;
            case 11:f.lifecycle.registerGameplayEventListener=nullptr;break;
            case 12:f.lifecycle.unregisterGameplayEventListener=nullptr;break;
        }
        Check(!f.Start() && f.Clean(),"missing required operation declines the optional button cleanly");
    }
    {Fixture f;Check(!InventoryDepositButton::Initialize(&f.context,nullptr,&Fixture::Enabled,&Fixture::Request) && f.Clean(),"missing visibility policy declines registration");}
    {Fixture f;Check(!InventoryDepositButton::Initialize(&f.context,&Fixture::Visible,nullptr,&Fixture::Request) && f.Clean(),"missing enabled policy declines registration");}
    {Fixture f;Check(!InventoryDepositButton::Initialize(&f.context,&Fixture::Visible,&Fixture::Enabled,nullptr) && f.Clean(),"missing action declines registration");}
}
void RegistrationTests() {
    unsigned count{};
    {Fixture f;Check(f.Start(),"complete SDK registers button");count=f.resourceAttempts;
        Check(count==3 && f.liveResources.size()==3 && f.liveChildren.size()==1 && f.listenerHandle && f.gameplay.size()==2,
            "two artwork resources, one inventory layout and its UI/session listeners are owned");
        Check(f.resourceBytes.at(ArtPath)==std::string(reinterpret_cast<const char*>(ArtBytes),sizeof(ArtBytes)) &&
            f.resourceBytes.at(LowendArtPath)==std::string(reinterpret_cast<const char*>(LowendArtBytes),sizeof(LowendArtBytes)),
            "resource registration copies full binary art including embedded NUL bytes");
        const auto& layout=f.resourceBytes.at(LayoutPath);
        Check(layout.find("\"filename\": \"d2rloader/inventory-deposit/deposit-button\"")!=layout.npos &&
            layout.find("\"normalFrame\": 0")!=layout.npos && layout.find("\"hoveredFrame\": 1")!=layout.npos &&
            layout.find("\"pressedFrame\": 2")!=layout.npos && layout.find("\"disabledFrame\": 3")!=layout.npos,
            "layout selects the custom artwork and its four intended interaction states");
        Check(layout.find("\"width\": 144, \"height\": 136")!=layout.npos,
            "layout preserves the converter's logical 144-by-136 button geometry");}
    for(unsigned fail=1;fail<=count;++fail) {
        Fixture f;f.failResourceAttempt=fail;
        Check(!f.Start() && f.Clean() && !f.childAttempts,
            "failure during any artwork or layout registration rolls back earlier resources before exposing a child");
    }
    for(unsigned fail=1;fail<=2;++fail) {
        Fixture f;f.failArtLoadAttempt=fail;
        Check(!f.Start() && f.Clean() && !f.childAttempts && f.resourceAttempts==fail-1,
            "missing embedded HD or lowend artwork releases any prior art without exposing a layout");
    }
    for(unsigned invalid=1;invalid<=3;++invalid) {
        Fixture f;f.malformedArt=invalid;
        Check(!f.Start() && f.Clean() && !f.resourceAttempts,
            "null, empty or oversized art is refused before resource registration");
    }
    {Fixture f;f.failChild=true;Check(!f.Start() && f.Clean(),"child registration failure releases resources and listener");}
    {Fixture f;f.failListener=true;Check(!f.Start() && f.Clean(),"listener registration failure releases resources and child");}
    for(unsigned fail=1;fail<=2;++fail) {
        Fixture f;f.failGameplayAttempt=fail;
        Check(!f.Start() && f.Clean(),"partial session-listener registration failure rolls back every owned registration");
    }
}
void RoutingTests() {
    Fixture f;f.visible=true;Check(f.Start(),"routing fixture starts");f.Pump();
    struct Message {const char* target;const char* command;const char* text;};
    const Message native[]{
        {"PanelManager","OpenPanel","charm-inventory/charm-inventory"},
        {"PanelManager","OpenPanel","StashPanel"},
        {"PanelManager","ClosePanel",ClickText},
        {"PanelManager","TogglePanel",ClickText},
        {"inventory-deposit","OpenPanel",ClickText},
        {"PanelManager","OpenPanel","inventory-deposit/DepositAll-extra"},
        {"PanelManager","OpenPanel","inventory-deposit/depositall"},
        {nullptr,"OpenPanel",ClickText},{"PanelManager",nullptr,ClickText},{"PanelManager","OpenPanel",nullptr}
    };
    for(const auto& message:native)
        Check(f.Send(message.target,message.command,message.text)==Action::Continue,"unrelated or malformed UI messages preserve native handling");
    f.uiThread=true;const auto nullEvent=f.listener.callback(&f.context,nullptr,f.listener.userData);f.uiThread=false;
    Check(nullEvent==Action::Continue,"null event cannot suppress native handling or trigger a deposit");
    Check(f.Send("PanelManager","OpenPanel",ClickText,D2RL::SharedEvents::UiMessageEventRequiredSize-1)==Action::Continue,"short event ABI cannot trigger a deposit");
    Check(f.requests==0 && f.tasks.empty(),"native and malformed messages queue no action");
    Check(Click(f)==Action::Consume,"exact private button payload is consumed before native panel opening");
    Check(f.requests==0 && !f.tasks.empty(),"click queues the deposit instead of invoking it inside message dispatch");
    f.Pump();Check(f.requests==1,"valid click invokes the shared deposit request once");
}
void StateAndQueueTests() {
    Fixture f;Check(f.Start(),"state fixture starts");f.Pump();
    InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visibility,false,"button is hidden outside stash context");
    f.visible=true;InventoryDepositButton::Refresh();const auto before=f.stateWrites;
    for(unsigned i=0;i<100;++i)InventoryDepositButton::Refresh();
    Check(f.tasks.size()==1 && f.stateWrites==before,"polling coalesces Refresh and does not touch widgets on the caller thread");
    f.Pump();CheckState(f.visibility,true,"open stash exposes the button");CheckState(f.enablement,true,"available action enables the button");
    f.enabled=false;InventoryDepositButton::Refresh();f.Pump();CheckState(f.enablement,false,"busy action disables the button");
    Check(Click(f)==Action::Consume,"disabled private click remains owned by plugin");f.Pump();Check(!f.requests,"disabled button cannot request item movement");
    f.enabled=true;InventoryDepositButton::Refresh();f.Pump();
    Check(Click(f)==Action::Consume,"enabled click accepted");
    for(unsigned i=0;i<100;++i)Check(Click(f)==Action::Consume,"duplicate private clicks stay consumed");
    Check(f.tasks.size()<=2,"duplicate clicks have a fixed queue bound");
    f.visible=false;f.Pump();Check(!f.requests,"closing stash before queued click executes cancels the deposit");
    InventoryDepositButton::Refresh();f.Pump();CheckState(f.visibility,false,"closed stash hides button after revalidation");
    f.visible=true;Check(Click(f)==Action::Consume,"new click accepted after cancelled work");f.enabled=false;f.Pump();
    Check(!f.requests,"new busy state before queued click executes cancels the deposit");
    f.enabled=true;for(unsigned i=0;i<100;++i)Click(f);f.Pump();Check(f.requests==1,"click storm produces only one request per queued batch");
    f.rejectQueue=true;Click(f);InventoryDepositButton::Refresh();Check(f.tasks.empty(),"scheduler rejection leaves no queued work");
    f.rejectQueue=false;Click(f);f.Pump();Check(f.requests==2,"scheduler rejection does not leave action permanently latched");
    f.requestResult=false;Click(f);f.Pump();Check(f.requests==3 && f.tasks.empty(),"declined deposit does not automatically retry item movement");
    f.requestResult=true;Click(f);f.Pump();Check(f.requests==4,"a later explicit click can retry a declined request");
    InventoryDepositButton::Refresh();f.Pump();CheckState(f.enablement,true,"refresh recovers after scheduler rejection");
    f.missingWidget=true;InventoryDepositButton::Refresh();f.Pump();
    f.missingWidget=false;InventoryDepositButton::Refresh();f.Pump();CheckState(f.visibility,true,"temporarily absent inventory widget can be found again");
    f.rejectEnabled=true;InventoryDepositButton::Refresh();f.Pump();CheckState(f.visibility,false,"failed enabled-state update hides the button");
    f.rejectEnabled=false;InventoryDepositButton::Refresh();f.Pump();CheckState(f.visibility,true,"button recovers after enabled-state update failure");
    f.rejectVisible=true;InventoryDepositButton::Refresh();f.Pump();CheckState(f.enablement,false,"failed visibility update disables the button");
    f.rejectVisible=false;InventoryDepositButton::Refresh();f.Pump();CheckState(f.enablement,true,"button recovers after visibility update failure");
}
void VisualStateTests() {
    Fixture f;f.visible=true;Check(f.Start(),"visual state fixture starts");
    InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visualEnablement,true,"eligible inventory uses native enabled visuals");
    f.enabled=false;InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visibility,true,"disabled button stays visible");
    CheckState(f.enablement,false,"disabled button blocks clicks through SDK");
    CheckState(f.visualEnablement,false,"native visual callback also selects the disabled appearance");
    InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visualEnablement,false,"repeated refresh preserves disabled appearance");
    Click(f);f.Pump();Check(!f.requests,"grey disabled state cannot deposit");
    f.enabled=true;InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visualEnablement,true,"eligible items restore the enabled appearance without a deposit");
    Check(!f.requests,"state transitions do not submit a deposit");
    f.stateResult=false;Click(f);f.Pump();
    Check(!f.requests,"visual-state failure refuses a queued click");
    CheckState(f.visibility,false,"unsupported native visual state hides the button");
    CheckState(f.enablement,false,"visual-state failure also disables input");
    f.stateResult=true;Click(f);f.Pump();
    Check(f.requests==1,"a later explicit click recovers when visual state is supported");
    f.visible=false;const auto calls=f.visualCalls;InventoryDepositButton::Refresh();f.Pump();
    Check(f.visualCalls==calls,"hidden contexts do not invoke native visual updates");
}
void ShutdownTests() {
    Fixture f;f.visible=true;Check(f.Start(),"shutdown fixture starts");f.Pump();
    Click(f);InventoryDepositButton::Refresh();Check(!f.tasks.empty(),"shutdown fixture has stale queued callbacks");
    const auto stale=f.tasks;f.tasks.clear();
    const auto listener=f.listener;
    const auto oldGameplay=f.gameplay;
    InventoryDepositButton::Shutdown();Check(f.Clean(),"unload unregisters the optional UI extension");
    const auto writes=f.stateWrites;
    for(const auto& task:stale)f.Run(task);
    Check(!f.requests && f.stateWrites==writes,"queued callbacks after unload cannot touch widgets or request deposits");
    Check(f.Start(),"button can initialize again after unload");f.Pump();
    const auto newWrites=f.stateWrites;
    for(const auto& task:stale)f.Run(task);
    Check(!f.requests && f.stateWrites==newWrites,"prior-generation callbacks cannot act on a new registration");
    D2RL::SharedEvents::UiMessageEvent event{};event.structSize=sizeof(event);event.target="PanelManager";event.command="OpenPanel";event.text=ClickText;
    f.uiThread=true;const auto result=listener.callback(&f.context,&event,listener.userData);f.uiThread=false;
    Check(result==Action::Continue && f.tasks.empty(),"stale listener cannot consume or queue a click for a new plugin generation");
    Click(f);const auto pending=f.tasks.size();
    for(const auto& task:stale)f.Run(task);
    D2RL::Lifecycle::GameplayEvent joined{};joined.structSize=sizeof(joined);joined.kind=D2RL::Lifecycle::GameplayEventKind::GameJoined;
    f.uiThread=true;
    for(const auto& [handle,entry]:oldGameplay) {
        (void)handle;if(entry.kind==joined.kind)entry.callback(&f.context,&joined,entry.userData);
    }
    f.uiThread=false;
    Check(f.tasks.size()==pending && !f.requests,"stale callback leaves new-generation queued work untouched");
    Click(f);f.Pump();Check(f.requests==1,"new generation keeps its duplicate-click guard after stale callbacks");
}
void SessionTests() {
    Fixture f;f.visible=true;Check(f.Start(),"session fixture starts");f.Pump();
    for(const auto kind:{D2RL::Lifecycle::GameplayEventKind::GameLeft,D2RL::Lifecycle::GameplayEventKind::GameJoined}) {
        Click(f);InventoryDepositButton::Refresh();
        const auto old=f.tasks;f.tasks.clear();
        f.Gameplay(kind);f.Pump();
        // A new character can already have opened a stash when old callbacks
        // arrive; predicates alone cannot identify the original game session.
        f.visible=true;Click(f);const auto pending=f.tasks.size();
        const auto requested=f.requests;
        for(const auto& task:old)f.Run(task);
        Check(f.requests==requested && f.tasks.size()==pending,"session boundary invalidates old queued clicks even if stash is open again");
        Click(f);f.Pump();Check(f.requests==requested+1,"new session accepts one bounded click after cancellation");
    }
}
void LayoutBoundsTests() {
    for(const auto coords:{std::pair{-1,100},std::pair{100,-1},std::pair{4097,100},std::pair{100,4097}}) {
        Fixture f;
        Check(!InventoryDepositButton::Initialize(&f.context,&Fixture::Visible,&Fixture::Enabled,&Fixture::Request,coords.first,coords.second)
            && f.Clean() && !f.resourceAttempts,"invalid layout coordinates are rejected before any UI resource is registered");
    }
    Fixture f;
    Check(InventoryDepositButton::Initialize(&f.context,&Fixture::Visible,&Fixture::Enabled,&Fixture::Request,0,4096),"documented layout coordinate boundaries are accepted");
}
void DelayedInventoryTests() {
    Fixture f;f.visible=true;f.missingWidget=true;Check(f.Start(),"inventory need not exist during plugin load");
    InventoryDepositButton::Refresh();f.Pump();Check(!f.stateWrites,"absent inventory root cannot receive widget state writes");
    f.missingWidget=false;InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visibility,true,"inventory opened after startup receives the button");
}
void PositionTests() {
    Fixture f;f.visible=true;f.requirePosition=true;f.positionResult=false;
    Check(f.Start(&Fixture::Position),"optional position callback registers");
    Check(!f.positionCalls,"registration does not position native widgets off the UI thread");
    InventoryDepositButton::Refresh();f.Pump();
    Check(f.positionCalls==2,"both inventory variants are considered for positioning");
    CheckState(f.visibility,false,"refused positions keep all children hidden");
    CheckState(f.enablement,false,"refused positions keep all children disabled");
    Click(f);f.Pump();Check(!f.requests,"click cannot request movement when every position is refused");

    f.positionResult=true;InventoryDepositButton::Refresh();f.Pump();
    CheckState(f.visibility,true,"button recovers when its position becomes supported");
    CheckState(f.enablement,true,"recovered position can enable the action");
    const auto positionedBefore=f.positionCalls;
    Click(f);Check(f.positionCalls==positionedBefore,"native message dispatch queues rather than invoking the positioner");
    f.Pump();Check(f.requests==1 && f.positionsAtRequest>=positionedBefore+2,
        "queued click revalidates position before the deposit request");

    Click(f);f.positionResult=false;f.Pump();
    Check(f.requests==1,"position lost after message dispatch cancels a queued click");
    CheckState(f.visibility,false,"lost position hides a previously visible button");
    CheckState(f.enablement,false,"lost position disables a previously enabled button");

    f.positionResult=true;f.refusedPositionPanel="PlayerInventoryExpansionLayout";
    Click(f);f.Pump();Check(f.requests==2,"one admitted visible variant is enough for a valid action");
    Check(!f.visibility[102] && !f.enablement[102] && f.visibility[202] && f.enablement[202],
        "one failed anchor does not show that child or suppress the other admitted child");

    f.refusedPositionPanel=nullptr;f.visible=false;
    const auto beforeClose=f.positionCalls;Click(f);f.Pump();
    Check(f.requests==2 && f.positionCalls==beforeClose,"closed stash hides without invoking the positioner or action");
    CheckState(f.visibility,false,"closing stash hides both positioned variants");
    f.visible=true;f.enabled=false;Click(f);f.Pump();
    Check(f.requests==2,"busy action stays refused with valid positions");
    CheckState(f.visibility,true,"busy state keeps correctly positioned button visible");
    CheckState(f.enablement,false,"busy state disables both correctly positioned variants");

    f.enabled=true;f.missingWidget=true;Click(f);f.Pump();
    Check(f.requests==2,"cached path handles do not admit a click when native widgets disappear");
    f.missingWidget=false;f.rejectEnabled=true;Click(f);f.Pump();
    Check(f.requests==2,"failed native enable operation refuses a positioned click");
    f.rejectEnabled=false;f.rejectVisible=true;Click(f);f.Pump();
    Check(f.requests==2,"failed native show operation refuses a positioned click");
    f.rejectVisible=false;Click(f);f.Pump();Check(f.requests==3,"later explicit click recovers after widget failures");

    InventoryDepositButton::Shutdown();f.requirePosition=false;f.positionResult=false;
    const auto beforeManual=f.positionCalls;
    Check(f.Start(),"manual fixed-coordinate mode can follow automatic positioning");
    Click(f);f.Pump();Check(f.requests==4 && f.positionCalls==beforeManual,
        "shutdown clears the old position callback and null callback retains manual behavior");
}
void EmitLayouts(const char* defaultPath,const char* customPath) {
    Fixture f;Check(f.Start(),"default layout fixture starts");
    const auto write=[&](const char* path) {
        Check(f.resourceBytes.size()==3 && f.resourceBytes.contains(LayoutPath),
            "layout export selects JSON explicitly and excludes both binary sprites");
        std::ofstream output(path,std::ios::binary|std::ios::trunc);
        output<<f.resourceBytes.at(LayoutPath);output.close();Check(output.good(),"generated layout written for strict JSON validation");
    };
    write(defaultPath);
    Check(InventoryDepositButton::Initialize(&f.context,&Fixture::Visible,&Fixture::Enabled,&Fixture::Request,123,456),"custom coordinate layout fixture starts");
    write(customPath);
}
}
bool InventoryDepositButtonArt::Load(unsigned resourceId,Blob& output) noexcept {
    Check(fixture!=nullptr,"art loading belongs to an active fixture");
    Check(resourceId==INVENTORY_DEPOSIT_BUTTON_SPRITE_RESOURCE_ID ||
        resourceId==INVENTORY_DEPOSIT_BUTTON_LOWEND_RESOURCE_ID,"art loader requests only the two plugin RCDATA IDs");
    ++fixture->artLoadAttempts;
    if(fixture->artLoadAttempts==fixture->failArtLoadAttempt)return false;
    const bool lowend=resourceId==INVENTORY_DEPOSIT_BUTTON_LOWEND_RESOURCE_ID;
    output={lowend?LowendArtBytes:ArtBytes,lowend?sizeof(LowendArtBytes):sizeof(ArtBytes)};
    if(fixture->malformedArt==1)output.bytes=nullptr;
    else if(fixture->malformedArt==2)output.byteCount=0;
    else if(fixture->malformedArt==3)output.byteCount=MaximumByteCount+1;
    return true;
}
int main(int argc,char** argv) {
    Check(argc==1 || argc==3,"usage: inventory-deposit-ui-tests [default-layout.json custom-layout.json]");
    CapabilityTests();RegistrationTests();RoutingTests();StateAndQueueTests();VisualStateTests();ShutdownTests();SessionTests();LayoutBoundsTests();DelayedInventoryTests();PositionTests();
    if(argc==3)EmitLayouts(argv[1],argv[2]);
    std::puts("deposit button embedded artwork, optional SDK, routing, UI queue, and lifecycle regressions passed");
}
