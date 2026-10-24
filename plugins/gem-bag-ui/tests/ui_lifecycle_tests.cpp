// Exercise the real plugin Load/Open callbacks with owned SDK services. Native
// admission, geometry and game work are substituted; this is not D2R rendering.
#include "../src/plugin.cpp"
#include "data_fixture.h"
#include "deposit_fixture.h"
#include <cstdlib>
#include <vector>
#include <fstream>

namespace Test {
using namespace D2RL;
using namespace GemBag;
void Check(bool value,const char* what){if(!value){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
unsigned reads{},registeredPanels{},registeredResources{},nativeLayouts{},openedPanels{},gameJobs{},listeners{},removedResources{},labelWrites{};
bool languageReady{},panelVisible{},changedName{},loading=true,rejectLabelWrite{},french{},hostNameDifferences{},hostEffectMismatch{};
bool hostMode{};
bool portableGeometry{};
bool controllerActive{},focusedPresent{};
uint32_t focusedId{},focusedClass{};
SharedEvents::ItemTooltipListener tooltipRegistration{};
TestData::Fixture tableFixture;
Lifecycle::DataTablesLoadedListener dataListener{};
Network::ChannelRegistration channelRegistration{};
std::deque<std::pair<Threads::Callback,void*>> uiQueue;
GemLabels appliedLabels{};
std::array<GemLabels,2> storedPanelLabels{};
volatile bool abortOnUnexpected=true;
template<class T>T Unexpected(){if(abortOnUnexpected)std::abort();return T{};}
uint64_t nextHandle=1;
Info bag{};
std::vector<std::string> layouts;
template<class R,class T> R Register(const PluginContext*,const T*,uint64_t* handle) noexcept {*handle=nextHandle++;++listeners;return R::Success;}
template<class R> R Unregister(const PluginContext*,uint64_t) noexcept {return R::Success;}

ItemService itemService{.serviceSize=ItemServiceSize,.serviceVersion=ItemService::AbiVersion,
    .getItemInfo=[](const PluginContext*,ItemHandle,Info* info) noexcept {*info=bag;return Items::Result::Success;},
    .createItem=[](const PluginContext*,PlayerHandle,const Items::ItemCreateSpec*,ItemHandle*) noexcept {return Unexpected<Items::Result>();},
    .destroyItem=[](const PluginContext*,PlayerHandle,ItemHandle,Items::SocketedItemPolicy) noexcept {return Unexpected<Items::Result>();},
    .editNativeItem=[](const PluginContext*,ItemHandle,Items::NativeItemEditCallback,void*) noexcept {return Unexpected<Items::Result>();}};
InventoryService inventoryService{.serviceSize=InventoryServiceSize,.serviceVersion=InventoryService::AbiVersion,
    .getLocalPlayer=[](const PluginContext*,PlayerHandle* owner) noexcept {*owner=1;return Inventory::Result::Success;},
    .getCursorItem=[](const PluginContext*,PlayerHandle,ItemHandle* cursor) noexcept {*cursor=0;return Inventory::Result::NotFound;},
    .forEachInventoryItem=[](const PluginContext* c,PlayerHandle,const Inventory::ItemFilter*,Inventory::ItemCallback cb,void* data) noexcept {
        if(!focusedPresent)return Unexpected<Inventory::Result>();
        cb(c,&bag,data);return Inventory::Result::Success;
    }};
PanelService panelService{.serviceSize=PanelServiceSize,.serviceVersion=PanelService::AbiVersion,
    .registerPanel=[](const PluginContext*,const Panels::PanelRegistration*,uint64_t* handle) noexcept {*handle=nextHandle++;++registeredPanels;return Panels::Result::Success;},
    .unregisterPanel=Unregister<Panels::Result>,
    .getPanelInfo=[](const PluginContext*,uint64_t,Panels::PanelInfo* info) noexcept {info->presentationState=panelVisible?Panels::PresentationState::Open:Panels::PresentationState::Closed;return Panels::Result::Success;},
    .openPanel=[](const PluginContext*,uint64_t) noexcept {if(nativeLayouts!=4)return Panels::Result::NotFound;++openedPanels;panelVisible=true;return Panels::Result::Success;},
    .closePanel=[](const PluginContext*,uint64_t) noexcept {panelVisible=false;return Panels::Result::Success;}};
ResourceService resourceService{.serviceSize=ResourceServiceSize,.serviceVersion=ResourceService::AbiVersion,
    .registerResource=[](const PluginContext*,const Resources::ResourceRegistration* r,uint64_t* handle) noexcept {
        layouts.emplace_back(static_cast<const char*>(r->bytes),static_cast<size_t>(r->byteCount));
        // Model the observed native layout-availability boundary: resources
        // supplied after Load can register successfully yet fail native Open.
        if(loading)++nativeLayouts;
        *handle=nextHandle++;++registeredResources;return Resources::Result::Success;
    },.unregisterResource=[](const PluginContext*,uint64_t) noexcept {Check(registeredResources>0,"owned layout removed once");--registeredResources;++removedResources;return Resources::Result::Success;}};
WidgetService widgetService{.serviceSize=WidgetServiceSize,.serviceVersion=WidgetService::AbiVersion};
SharedEventService eventService{.serviceSize=SharedEventServiceSize,.serviceVersion=SharedEventService::AbiVersion,
    .registerItemTooltipListener=[](const PluginContext* c,const SharedEvents::ItemTooltipListener* r,uint64_t* handle) noexcept {
        tooltipRegistration=*r;return Register<SharedEvents::Result>(c,r,handle);},
    .unregisterItemTooltipListener=Unregister<SharedEvents::Result>,
    .registerUiMessageListener=Register<SharedEvents::Result,SharedEvents::UiMessageListener>,
    .unregisterUiMessageListener=Unregister<SharedEvents::Result>};
ItemInteractionService interactionService{.serviceSize=ItemInteractionServiceSize,.serviceVersion=ItemInteractionService::AbiVersion,
    .registerListener=Register<ItemInteractions::Result,ItemInteractions::ItemInteractionListener>,
    .unregisterListener=Unregister<ItemInteractions::Result>,.supportedContainerMask=StorageMask};
ThreadService threadService{.serviceSize=ThreadServiceSize,.serviceVersion=ThreadService::AbiVersion,
    .runOnUiThread=[](const PluginContext*,Threads::Callback callback,void* data) noexcept {uiQueue.emplace_back(callback,data);return Threads::Result::Success;},
    .runOnGameThread=[](const PluginContext*,Threads::Callback,void*) noexcept {++gameJobs;return Threads::Result::Success;}};
NetworkService networkService{.serviceSize=NetworkServiceSize,.serviceVersion=NetworkService::AbiVersion,
    .registerChannel=[](const PluginContext*,const Network::ChannelRegistration* registration,uint64_t* handle) noexcept {channelRegistration=*registration;*handle=1;return Network::Result::Success;},
    .unregisterChannel=[](const PluginContext*,uint64_t) noexcept {return Network::Result::Success;},
    .getChannelInfo=[](const PluginContext*,uint64_t,Network::ChannelInfo* info) noexcept {info->connectionState=Network::ConnectionState::Connected;return Network::Result::Success;},
    .connectToHost=[](const PluginContext*,uint64_t) noexcept {return Network::Result::Success;},
    .sendToHost=[](const PluginContext* context,uint64_t channel,uint16_t messageId,const void* data,uint32_t size) noexcept {
        Protocol::Request request{};Check(messageId==Protocol::RequestMessage&&Protocol::Copy(data,size,request),"UI emits valid copied server requests");
        Protocol::Reply reply{};reply.request=request;reply.serverSession=55;reply.flags=Protocol::Success;reply.status=Protocol::Status::Ok;
        if(request.action!=Protocol::Action::Hello){++gameJobs;reply.lease=request.sequence+100;reply.flags|=Protocol::HasSnapshot|Protocol::HasCell;reply.credits=10;reply.width=10;reply.height=8;
            const auto definition=Catalog::Find(3,bag);Check(definition!=nullptr,"UI request waits for discovered bag definitions");
            reply.definition=definition->fingerprint;reply.bank=3;reply.gemMask=definition->mask;
            if(hostNameDifferences) {
                auto host=tableFixture;
                for(unsigned id:host.output)host.Set(0,id,0xfc,uint16_t{41000});
                if(hostEffectMismatch)host.Row(2,0)[0x38]^=1;
                Catalog::Definitions found;std::string why;Check(Catalog::Build(host.Views(),3,found,why)&&found.size()==1,"separate headless definitions build without client localization");
                reply.definition=found[0].fingerprint;
            }}
        channelRegistration.clientMessage(context,channel,Protocol::ReplyMessage,&reply,sizeof(reply),channelRegistration.userData);return Network::Result::Success;
    },
    .sendToClient=[](const PluginContext*,uint64_t,uint64_t,uint16_t,const void*,uint32_t) noexcept {return Unexpected<Network::Result>();},
    .getPeerPlayer=[](const PluginContext*,uint64_t,uint64_t,PlayerHandle*) noexcept {return Unexpected<Network::Result>();}};
LifecycleService lifecycleService{.serviceSize=LifecycleServiceSize,.serviceVersion=LifecycleService::AbiVersion,
    .registerDataTablesLoadedListener=[](const PluginContext* c,const Lifecycle::DataTablesLoadedListener* registration,uint64_t* handle) noexcept {
        dataListener=*registration;return Register<Lifecycle::Result>(c,registration,handle);},
    .unregisterDataTablesLoadedListener=Unregister<Lifecycle::Result>,
    .registerGameplayEventListener=Register<Lifecycle::Result,Lifecycle::GameplayEventListener>,
    .unregisterGameplayEventListener=Unregister<Lifecycle::Result>};
OverlayService overlayService{.serviceSize=OverlayServiceSize,.serviceVersion=OverlayService::AbiVersion,
    .registerFrameCallback=Register<Overlay::Result,Overlay::CallbackRegistration>,.unregisterFrameCallback=Unregister<Overlay::Result>};
LocalizationService localizationService{.serviceSize=LocalizationServiceSize,.serviceVersion=LocalizationService::AbiVersion,
    .getStringById=[](const PluginContext*,uint32_t id,char* out,uint32_t size,uint32_t* required) noexcept {
        ++reads;if(!languageReady)return Localization::Result::Unavailable;
        if(id<1000||id>=1008)return Localization::Result::NotFound;
        const auto text=std::string("\xee\x81\xbe")+"U"+(french?"Nom localis\xc3\xa9":changedName?"Changed name":Gems[id-1000].name);
        *required=static_cast<uint32_t>(text.size()+1);if(size<*required)return Localization::Result::BufferTooSmall;
        std::memcpy(out,text.c_str(),*required);return Localization::Result::Success;},
    .getStringByKey=[](const PluginContext*,const char* key,char* out,uint32_t size,uint32_t* required) noexcept {
        ++reads;if(!languageReady)return Localization::Result::Unavailable;
        for(const auto& gem:Gems)if(!std::strcmp(key,gem.nameKey+1)) {
            const auto text=std::string("\xee\x81\xbe")+"U"+(changedName?"Changed name":gem.name);
            *required=static_cast<uint32_t>(text.size()+1);
            if(size<*required)return Localization::Result::BufferTooSmall;
            std::memcpy(out,text.c_str(),*required);return Localization::Result::Success;
        }
        return Localization::Result::NotFound;
    }};
ServiceQueryResult __cdecl Query(const PluginContext*,ServiceId id,uint32_t version,const void** out) noexcept {
    if(hostMode)Check(id==ServiceId::Item||id==ServiceId::Inventory||id==ServiceId::Thread||id==ServiceId::Network||id==ServiceId::Lifecycle||id==ServiceId::DataTable,"headless host never queries UI, input or localization services");
    const auto match=[&]<class T>(T& service){if(id!=T::Id||version!=T::AbiVersion)return false;*out=&service;return true;};
    return match(itemService)||match(inventoryService)||match(panelService)||match(resourceService)||match(widgetService)||
        match(eventService)||match(interactionService)||match(threadService)||match(lifecycleService)||match(overlayService)||match(localizationService)||match(networkService)||match(TestData::Service)?
        ServiceQueryResult::Success:ServiceQueryResult::UnknownService;
}
void DrainUi(const PluginContext* context) {
    unsigned count{};while(!uiQueue.empty()) {Check(++count<64,"bounded UI callback loop");const auto work=uiQueue.front();uiQueue.pop_front();work.first(context,work.second);}
}
}
namespace GemBag::Native {
bool Deposit(const D2RL::PluginContext*,const D2RL::ItemService*,const D2RL::InventoryService*,D2RL::PlayerHandle,const Info&,Snapshot&,std::string&,bool&){return Test::Unexpected<bool>();}
bool Initialize(const D2RL::PluginContext*){return true;}
bool Inspect(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle,const Info&,Snapshot&,std::string&){return Test::Unexpected<bool>();}
bool Execute(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::PlayerHandle,const Info&,unsigned,Snapshot&,std::string&,bool&){return Test::Unexpected<bool>();}
bool Focus(const D2RL::PluginContext*,uint32_t& id,uint32_t& cls) noexcept{if(!Test::focusedPresent)return false;id=Test::focusedId;cls=Test::focusedClass;return true;}
bool Controller(const D2RL::PluginContext*) noexcept{return Test::controllerActive;}
bool SafeUi(const D2RL::PluginContext*) noexcept{return true;}
}
namespace GemBag::Geometry {
bool Initialize(const D2RL::PluginContext*,const D2RL::WidgetService*) noexcept{return true;}
void UsePortable(bool portable) noexcept {Test::portableGeometry=portable;}
bool Place(POINT,int,int,bool,Bounds&) noexcept{return Test::Unexpected<bool>();}
bool Hit(POINT,int&) noexcept{return Test::Unexpected<bool>();}
bool SetLabels(const GemLabels& current,const GemLabels& previous) noexcept {
    if(Test::rejectLabelWrite)return false;
    const auto index=Test::portableGeometry?1u:0u;
    if(!Test::storedPanelLabels[index][0].empty())Test::Check(previous==Test::storedPanelLabels[index],"previous text ownership belongs to the active panel");
    for(const auto& text:current)Test::Check(text.find("\xee\x81\xbe")==std::string::npos,"copied label reaches buffer adapter without native color token");
    Test::storedPanelLabels[index]=current;Test::appliedLabels=current;++Test::labelWrites;return true;
}
const char* LastReason() noexcept{return "test substitute";}
}
int main(int argc,char** argv) {
    using namespace Test;
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.queryService=Query};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api,.loadScope=D2RL::LoadScope::Global};
    if(argc==2&&!std::strcmp(argv[1],"--host")) {
        hostMode=true;Check(Load(&context,true),"dedicated host loads only authoritative services and its channel");
        Check(!registeredPanels&&!registeredResources&&listeners==1&&!reads&&channelRegistration.hostMessage,"headless host registers table discovery without UI, resources, input, localization or local-player listener");
        for(unsigned id:tableFixture.output)tableFixture.Set(0,id,0xfc,uint16_t{41000});
        TestData::active=&tableFixture;const Lifecycle::DataTablesLoadedEvent loaded{.structSize=Lifecycle::DataTablesLoadedEventSize,.revision=1};
        dataListener.callback(&context,&loaded,nullptr);
        Info identity{};identity.classId=tableFixture.bag;identity.code=BagCode;
        const auto definition=Catalog::Find(3,identity);
        Check(definition&&Catalog::Recheck(&context,*definition)&&!reads,"headless catalog accepts its own name references without requesting translations");
        D2RLoaderUnloadPlugin();std::puts("Headless host Load/Unload passes with only SDK authority/network services; native admission substituted.");return 0;
    }
    Check(D2RLoaderLoadPlugin(&context),"plugin loads while startup localization is unavailable");
    Check(reads==0,"startup must not read unavailable localized gem names");
    Check(registeredPanels==2&&listeners==7,"table, panel and input listeners register before label preparation");
    Check(registeredResources==4&&nativeLayouts==4&&!openedPanels&&!gameJobs,"startup publishes both panel/input variants without localized lookup or game work");
    Check(layouts.size()==4&&layouts[0]==layouts[1]&&layouts[2]==layouts[3]&&layouts[0].find("__GEM_BAG_LABEL_0__")!=std::string::npos,"startup layouts contain reserved native text buffers");
    const bool portable=argc==2&&!std::strcmp(argv[1],"--portable");
    if(portable){tableFixture=TestData::Fixture(915,421,450);tableFixture.Row(5,7)[0]=0;tableFixture.Set(0,915,0x80,TestData::Fixture::ItemsCode("pou"));}
    loading=false;
    TestData::active=&tableFixture;const Lifecycle::DataTablesLoadedEvent loaded{.structSize=Lifecycle::DataTablesLoadedEventSize,.revision=1};
    Check(dataListener.callback!=nullptr,"definition discovery registers during startup");dataListener.callback(&context,&loaded,nullptr);
    bag={.structSize=sizeof(Info)};bag.handle=1;bag.runtimeId=153;bag.classId=tableFixture.bag;bag.code=Catalog::Read<uint32_t>(tableFixture.Row(0,tableFixture.bag),0x80);bag.container=Container::Inventory;bag.quality=Items::Quality::Normal;bag.sharedStashPage=UINT32_MAX;
    Check(Open(bag.handle,bag,{},false),"unavailable labels cannot block the icon picker");
    DrainUi(&context);
    Check(!labelsReady&&!labelWrites&&openedPanels==1&&gameJobs==1,"missing labels stay hidden while native picker and inspection work");
    Close();
    if(portable) {
        languageReady=true;Check(Open(bag.handle,bag,{},true),"generic seven-gem bag opens");DrainUi(&context);
        Check(ready&&snapshot.gemMask==127&&portableGeometry&&panel==ownedPanels[1]&&!std::strcmp(FullName,"gem-bag-ui/SelectorPortable"),"generic definitions select the portable panel and active mask");
        Check(layouts[2].find("perfect_chaos_onyx")==std::string::npos&&labelsReady&&appliedLabels[7]==" ","portable picker needs no mod-only art or missing localization");
        selected=6;MoveSelection(UiAction::Right);Check(selected==4,"controller skips unavailable gem slots");
        selected=7;const auto before=gameJobs;QueueWork(true);Check(gameJobs==before,"unavailable gem cannot send a withdrawal");
        Rpc::Result mismatched{};mismatched.ticket={generation,presentation,pendingSerial};mismatched.reply.flags=Protocol::Success|Protocol::HasSnapshot;
        mismatched.reply.bank=3;mismatched.reply.gemMask=127;mismatched.reply.definition=snapshot.definition^1;
        HostResult(mismatched);Check(!ready&&requestFailed,"different host definitions keep the client picker non-mutating");DrainUi(&context);
        Close();tableFixture=TestData::Fixture();dataListener.callback(&context,&loaded,nullptr);
        bag.classId=746;bag.code=BagCode;Check(Open(bag.handle,bag,{},true)&&labelsReady,"switch from portable to Onyx panel preserves label ownership");DrainUi(&context);
        Close();tableFixture=TestData::Fixture(915,421,450);tableFixture.Row(5,7)[0]=0;tableFixture.Set(0,915,0x80,TestData::Fixture::ItemsCode("pou"));dataListener.callback(&context,&loaded,nullptr);
        bag.classId=915;bag.code=TestData::Fixture::ItemsCode("pou");Check(Open(bag.handle,bag,{},true)&&labelsReady,"switch back qualifies the portable panel's previous buffers");DrainUi(&context);
        Close();D2RLoaderUnloadPlugin();std::puts("Generic client lifecycle: no mod name, relocated bag/stat/property, seven gems, portable art, active compiled names and disabled-slot input passed.");return 0;
    }
    languageReady=true;
    rejectLabelWrite=true;
    Check(Open(bag.handle,bag,{},false)&&!labelsReady&&openedPanels==2,"buffer guard refusal hides labels without blocking the picker");
    DrainUi(&context);
    Close();rejectLabelWrite=false;
    Check(Open(bag.handle,bag,{},true)&&labelsReady&&labelWrites>0,"later open populates labels after native creation");
    DrainUi(&context);
    Check(appliedLabels[7]=="Chaos Onyx"&&registeredResources==4&&openedPanels==3&&gameJobs==3,"plain localized names use the owned buffer adapter without replacing resources");
    Close();
    changedName=true;
    Check(Open(bag.handle,bag,{},false)&&labelsReady&&appliedLabels[0]=="Changed name"&&registeredResources==4,"fresh localized names update owned labels without a layout replacement");
    DrainUi(&context);
    Close();
    french=true;hostNameDifferences=true;
    Check(Open(bag.handle,bag,{},false)&&labelsReady&&appliedLabels[0]=="Nom localis\xc3\xa9","client applies its own UTF-8 translation");DrainUi(&context);
    Check(ready&&!requestFailed&&snapshot.definition==Catalog::Find(3,bag)->fingerprint,"host with different name IDs passes the actual client definition check");
    const auto beforeWithdraw=gameJobs;QueueWork(true);DrainUi(&context);
    Check(ready&&!requestFailed&&gameJobs==beforeWithdraw+1&&appliedLabels[5]=="Nom localis\xc3\xa9","localized picker still routes withdrawal through host transport");
    Close();hostEffectMismatch=true;
    Check(Open(bag.handle,bag,{},false),"open for semantic host mismatch inspection");DrainUi(&context);
    Check(!ready&&requestFailed,"changed host gem effects still disable client withdrawals despite name neutrality");
    const auto beforeRefusal=gameJobs;QueueWork(true);Check(gameJobs==beforeRefusal,"host definition mismatch sends no withdrawal");Close();
    hostEffectMismatch=false;TestData::AddDeposits(tableFixture);dataListener.callback(&context,&loaded,nullptr);
    const auto beforeDirect=gameJobs,beforePanels=openedPanels;
    StartDeposit(bag);StartDeposit(bag);DrainUi(&context);
    if(gameJobs!=beforeDirect+2||directDeposit)std::fprintf(stderr,"Direct pending=%u busy=%u open=%u jobs=%u before=%u ready=%u failed=%u message=%s\n",directDeposit,busy,open,gameJobs,beforeDirect,ready,requestFailed,message.c_str());
    Check(!directDeposit&&!busy&&!open&&gameJobs==beforeDirect+2&&openedPanels==beforePanels,"bag gesture inspects then deposits once without opening a panel");
    focusedPresent=true;focusedId=bag.runtimeId;focusedClass=bag.classId;controllerActive=true;
    char tooltipBuffer[128]{};SharedEvents::ItemTooltipEvent tipEvent{.structSize=SharedEvents::ItemTooltipEventSize,.item=bag.handle,.text=tooltipBuffer,.capacity=sizeof(tooltipBuffer)};
    Check(tooltipRegistration.callback!=nullptr,"tooltip listener registered");tooltipRegistration.callback(&context,&tipEvent,nullptr);
    Check(!std::strcmp(tooltipBuffer,"LB+A: Choose gem\nLB+B: Deposit gems + clusters"),"controller tooltip advertises LB+A picker and LB+B deposit");
    Check(QueueCommand(UiAction::DepositFocused,bag.runtimeId,bag.classId),"controller LB+B queues focused bag deposit");
    DrainUi(&context);
    Check(!directDeposit&&!busy&&!open&&gameJobs==beforeDirect+4&&openedPanels==beforePanels,"controller focused deposit inspects then deposits without opening a panel");
    Check(QueueCommand(UiAction::DepositFocused,bag.runtimeId+1,bag.classId),"stale focused bag command queued");
    DrainUi(&context);
    Check(gameJobs==beforeDirect+4,"moved focus before UI callback refuses controller deposit");
    focusedPresent=false;controllerActive=false;
    hostEffectMismatch=true;StartDeposit(bag);DrainUi(&context);
    Check(!directDeposit&&!ready&&gameJobs==beforeDirect+5,"direct deposit refuses mismatched host definitions and clears pending state");
    D2RLoaderUnloadPlugin();
    Check(registeredResources==0&&removedResources==4,"unload releases all owned startup layouts");
    if(argc==2){std::ofstream out(argv[1]);out<<layouts[0];}
    std::puts("Real plugin lifecycle: startup resource visibility, late localized labels, native open before label writes, label-only failures and resource cache reuse passed; owned SDK substitutes only.");
}
