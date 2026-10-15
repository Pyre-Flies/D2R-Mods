#include "../src/plugin.cpp"
// Test-only access to the existing backend's resolved native entries. Exercise
// real registration/retained dispatch without weakening production admission.
#include "../vendor/src/native_commands.cpp"
#include <cstdlib>
#include <array>
void Check(bool value,const char* why) {if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
D2RL::Threads::Callback queuedChat{};
const D2RL::PluginContext* queuedContext{};
void* queuedData{};
bool chatOnUi{};
unsigned chatListeners{},chatUnlisteners{};
const D2RLChat::Definition* nativeChatEntry{};
std::string savedConfig="# keep this note\n[always-show-sockets]\ninserted_item_scale_percent = 65\nring_opacity_percent = 80 # outline\n[other]\nkeep = 123\n";
unsigned configReads{},configWrites{},saveWarnings{};
bool failConfigRead{},failConfigWrite{};
bool __cdecl ReadConfig(const D2RL::PluginContext*,char* output,std::uint32_t size,std::uint32_t* required) noexcept {
    ++configReads;const auto bytes=static_cast<std::uint32_t>(savedConfig.size()+1);
    if(required) *required=bytes;
    if(failConfigRead || !output || size<bytes) return false;
    std::memcpy(output,savedConfig.c_str(),bytes);return true;
}
bool __cdecl WriteConfig(const D2RL::PluginContext*,const char* text) noexcept {
    ++configWrites;if(failConfigWrite) return false;
    savedConfig=text;return true;
}
void __cdecl Warn(const D2RL::PluginContext*,const char* text) noexcept {
    if(std::strstr(text,"config save failed")) ++saveWarnings;
}
D2RL::ConsoleCommandResult Invoke(const char* args) noexcept {
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<std::uint32_t>(std::strlen(args)),.args=args,.plugin=ctx};
    return Command(nullptr,&command,nullptr);
}
D2RL::Threads::Result __cdecl QueueChat(const D2RL::PluginContext* context,D2RL::Threads::Callback callback,void* data) noexcept {
    queuedContext=context;queuedChat=callback;queuedData=data;return D2RL::Threads::Result::Success;
}
D2RL::SharedEvents::Result __cdecl ListenChat(const D2RL::PluginContext*,const D2RL::SharedEvents::UiMessageListener*,std::uint64_t* handle) noexcept {
    ++chatListeners;*handle=1;return D2RL::SharedEvents::Result::Success;
}
D2RL::SharedEvents::Result __cdecl UnlistenChat(const D2RL::PluginContext*,std::uint64_t handle) noexcept {
    Check(handle==1,"remove only acquired chat listener");++chatUnlisteners;return D2RL::SharedEvents::Result::Success;
}
const D2RL::ThreadService chatThreads{sizeof(chatThreads),1,QueueChat,nullptr};
const D2RL::SharedEventService chatEvents{.serviceSize=sizeof(chatEvents),.serviceVersion=1,
    .registerUiMessageListener=ListenChat,.unregisterUiMessageListener=UnlistenChat};
D2RL::ServiceQueryResult __cdecl QueryChat(const D2RL::PluginContext*,D2RL::ServiceId id,std::uint32_t,const void** output) noexcept {
    if(id==D2RL::ServiceId::Thread) {*output=&chatThreads;return D2RL::ServiceQueryResult::Success;}
    if(id==D2RL::ServiceId::SharedEvent) {*output=&chatEvents;return D2RL::ServiceQueryResult::Success;}
    return D2RL::ServiceQueryResult::UnknownService;
}
std::uint64_t* __fastcall ChatParser() {
    Check(chatOnUi,"resolve parser only from queued UI work");static std::uint64_t parser=123;return &parser;
}
std::uint32_t __fastcall RegisterNativeChat(std::uint64_t parser,const D2RLChat::Definition* definition) {
    Check(chatOnUi && parser==123,"register on UI thread using resolved parser");
    Check(std::strcmp(definition->name,"sockets")==0 && std::strcmp(definition->description,CommandHelp)==0 && definition->execute,"actual socket registration uses bounded help and retained dispatcher");
    nativeChatEntry=definition;return 0;
}
extern "C" int TestPostIconGate(void*,void*);
extern "C" void TestPostIconContinuation();
extern "C" int TestScaleGate(void*,Sockets::Point*,float*,float*);
extern "C" void TestScaleContinuation();
extern "C" int TestCircleGate(void*,Sockets::Point*,float*,float*,bool,void* expectedSprite=nullptr);
extern "C" void TestCircleContinuation();
bool hdMode=true;
bool __fastcall FakeHd() {return hdMode;}
void* observedWidget{};
void* observedItem{};
unsigned draws{};
void __fastcall CaptureDraw(void* widget,void* item) {observedWidget=widget;observedItem=item;++draws;}
unsigned centerDraws{};
float centerAlpha{},centerScale{};
Sockets::Point centerPoint{};
void __fastcall CaptureCenter(void* sprite,Sockets::Point* point,void* settings,float scale) {
    Check(sprite==&socketCenter,"extra native draw uses center layer only");
    const auto* bytes=static_cast<const unsigned char*>(settings);
    std::memcpy(&centerAlpha,bytes+0x10,sizeof(centerAlpha));
    float red{};std::memcpy(&red,bytes,sizeof(red));Check(red==1,"center settings preserve original tint");
    centerPoint=*point;centerScale=scale;++centerDraws;
    // Mutate the owned arguments to prove the original native inputs are isolated.
    point->x=-999;std::memset(settings,0,sizeof(float));
}
struct SocketRenderCase {
    void* circle{};void* art{};bool styled{},filled{};
    Sockets::Point circlePoint{100,200},artPoint{100,220};
    float circleScale=1,circleOpacity=0.25f,artScale=1,artOpacity=0.5f;
};
SocketRenderCase* socketRenderCase{};
void __fastcall CaptureSocketRender(void* widget,void* item) {
    CaptureDraw(widget,item);
    Check(socketDrawDepth==1 && socketRenderCase,"native socket entry scopes its actual original draw");
    auto& draw=*socketRenderCase;
    Check(TestCircleGate(draw.circle,&draw.circlePoint,&draw.circleScale,&draw.circleOpacity,draw.filled,draw.styled?&socketRing:nullptr)==1,"native socket path preserves circle adapter state in all modes");
    if(draw.filled) Check(TestScaleGate(draw.art,&draw.artPoint,&draw.artScale,&draw.artOpacity)==1,"native socket path preserves inserted adapter state in all modes");
}
int main() {
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.logWarn=Warn,.readConfig=ReadConfig,.writeConfig=WriteConfig,.queryService=QueryChat};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    Check(!alwaysVisible.load(),"startup visibility policy defaults to native hover");
    ctx=&context;ready.store(true);enabled.store(true);
    D2RLChat::getParser=ChatParser;D2RLChat::registerCommand=RegisterNativeChat;
    Check(RegisterChatCommand() && chatCommands.Status("sockets").state==D2RLChat::NativeCommandState::Pending,"production chat setup accepts its real help text and queues registration");
    Check(queuedChat && chatListeners==1 && !nativeChatEntry,"native registration is deferred until UI work");
    chatOnUi=true;queuedChat(queuedContext,queuedData);chatOnUi=false;
    Check(nativeChatEntry && chatCommands.Status("sockets").state==D2RLChat::NativeCommandState::Registered && chatUnlisteners==1,"native success enables retained socket command dispatch");
    Check(!chatCommands.Register("overlong",ChatCommand,nullptr,std::string(64,'x')) && chatCommands.Reason()=="invalid name, description, or callback","backend still refuses overlong descriptions rather than overflowing native record");
    nativeChatEntry->execute("always");
    Check(enabled.load() && alwaysVisible.load(),"always mode enables styling and forced display");
    socketOriginal=reinterpret_cast<void*>(CaptureDraw);
    SocketPostIconOriginal=reinterpret_cast<void*>(TestPostIconContinuation);
    int widget{},item{};
    Check(TestPostIconGate(&widget,&item)==1,"post-icon gateway preserves volatile GPR/XMM/flags and stack alignment");
    Check(observedWidget==&widget && observedItem==&item && draws==1,"native frame widget/item forwarded exactly once");
    SocketHook(&widget,&item);Check(draws==1,"selected native socket call does not draw twice");
    enabled.store(false);
    Check(TestPostIconGate(&widget,&item)==1 && draws==1,"disabled gateway preserves state without added draw");
    SocketHook(&widget,&item);Check(draws==2,"disabled mode preserves native socket call");
    enabled.store(true);
    scaleReady.store(true);opacityReady.store(true);isHd=FakeHd;
    SocketScaleOriginal=reinterpret_cast<void*>(TestScaleContinuation);
    std::array<unsigned char,0xd4> hd{};
    const std::int32_t metadata[6]={64,64,0,64,1,64};
    std::memcpy(hd.data()+0xbc,metadata,sizeof(metadata));
    auto hdPointer=reinterpret_cast<std::uintptr_t>(hd.data());
    Sockets::Point point{100,200};float nativeScale=1,nativeOpacity=1;
    Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && point.x==100 && point.y==200 && nativeScale==1,"loose item bypasses scaling with preserved registers");
    Check(nativeOpacity==1,"loose item remains opaque");
    {
        SocketDrawScope scope;
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1,"scale gate preserves GPR/XMM and flags");
        Check(point.x==111 && point.y==189 && std::abs(nativeScale-0.65f)<0.00001f,"inserted sprite centered at 65 percent");
        Check(std::abs(nativeOpacity-0.65f)<0.00001f,"native opacity stack input forwarded and faded");
        nativeOpacity=0.5f;point={100,200};nativeScale=1;
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && std::abs(nativeOpacity-0.325f)<0.00001f,"existing ethereal/native fade is preserved multiplicatively");
        nativeOpacity=1;
        point={100,200};nativeScale=1;enabled.store(false);
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeScale==1 && point.y==200,"disabled feature preserves full-size inserted art");
        Check(nativeOpacity==1,"disabled feature preserves native opacity");
        enabled.store(true);scalePercent.store(100);
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeScale==1 && point.x==100,"100 percent keeps native anchor and scale");
        nativeOpacity=0.5f;opacityPercent.store(100);
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeOpacity==0.5f,"100 percent opacity retains original fade");
        opacityPercent.store(65);nativeOpacity=1;opacityReady.store(false);
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeOpacity==1,"refused opacity guards preserve native fade input");
        opacityReady.store(true);
        scalePercent.store(65);scaleReady.store(false);
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeScale==1,"unavailable optional scale hook keeps native input");
        Check(nativeOpacity==1,"unavailable style hook leaves opacity unchanged");
        scaleReady.store(true);
        hdMode=false;point={100,200};nativeScale=1;
        Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeScale==1 && point.y==200,"classic rendering remains native");
        Check(nativeOpacity==1,"classic opacity remains native");
        hdMode=true;
    }
    Check(socketDrawDepth==0,"socket scope exits without affecting following loose items");
    Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && nativeOpacity==1,"loose draw after socket scope remains native");
    circleReady.store(true);drawSprite=CaptureCenter;SocketCircleOriginal=reinterpret_cast<void*>(TestCircleContinuation);
    std::array<unsigned char,0xd4> ringHd{},runeHd{};
    const std::int32_t ringMetadata[6]={128,128,0,128,1,128},runeMetadata[6]={64,96,0,96,1,64};
    std::memcpy(ringHd.data()+0xbc,ringMetadata,sizeof(ringMetadata));
    std::memcpy(runeHd.data()+0xbc,runeMetadata,sizeof(runeMetadata));
    socketRing.hd=reinterpret_cast<std::uintptr_t>(ringHd.data());socketCenter.hd=socketRing.hd;ringAttempted=true;
    auto runePointer=reinterpret_cast<std::uintptr_t>(runeHd.data());
    float ringScale=1,ringOpacity=0.25f;Sockets::Point ringPoint{100,200};
    Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringScale==1 && ringOpacity==0.25f,"circle adapter preserves native arguments outside socket scope");
    {
        SocketDrawScope scope;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1,"circle adapter forwards owned ring while preserving GPR/SIMD/flags and native occupancy predicate");
        Check(std::abs(ringScale-0.4f)<0.00001f && ringPoint.x==106 && ringPoint.y==194 && std::abs(ringOpacity-0.8f)<0.00001f,"filled ring uses 80 percent size and 80 percent opacity for 65 percent art");
        Check(centerDraws==1 && std::abs(centerAlpha-0.40f)<0.00001f && centerPoint.x==ringPoint.x && centerPoint.y==ringPoint.y && centerScale==ringScale,"black center draws before ring at independent 40 percent opacity with identical geometry");
        Check(pendingCircle.valid,"filled circle captures copied center for following inserted sprite");
        Sockets::Point runePoint{100,220};float runeScale=1,runeOpacity=1;
        Check(TestScaleGate(&runePointer,&runePoint,&runeScale,&runeOpacity)==1 && runePoint.x==111 && runePoint.y==199 && std::abs(runeScale-0.65f)<0.00001f,"paired rune moves one rendered pixel up while circle and horizontal alignment stay fixed");
        Check(!pendingCircle.valid,"paired circle center consumed once");
        runePoint={100,220};runeScale=1;runeOpacity=1;
        Check(TestScaleGate(&runePointer,&runePoint,&runeScale,&runeOpacity)==1 && runePoint.y==203,"later unpaired sprite does not reuse prior circle center");
        for(unsigned lift:{0u,1u,2u,8u}) {
            const char* command=lift==0?"lift 0":lift==1?"lift 1":lift==2?"lift 2":"lift 8";
            nativeChatEntry->execute(command);
            ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
            Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1 && ringPoint.x==106 && ringPoint.y==194 && centerPoint.y==194,"lift command leaves both circle layers fixed");
            runePoint={100,220};runeScale=1;runeOpacity=1;
            Check(TestScaleGate(&runePointer,&runePoint,&runeScale,&runeOpacity)==1 && runePoint.x==111 && runePoint.y==200-static_cast<int>(lift),"lift command moves only paired inserted artwork by requested pixels");
        }
        ChatCommand("lift 1",nullptr);
        ringScale=1;ringPoint={100,200};ringOpacity=0.25f;opacityPercent.store(10);
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false,&socketRing)==1 && std::abs(ringOpacity-0.8f)<0.00001f && std::abs(ringScale-0.4f)<0.00001f && std::abs(centerAlpha-0.40f)<0.00001f,"empty layers use independent defaults regardless of inserted opacity");
        Check(!pendingCircle.valid,"empty circle never claims a following item draw");
        rimOpacityPercent.store(40);centerOpacityPercent.store(70);ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1 && std::abs(ringOpacity-0.4f)<0.00001f && std::abs(centerAlpha-0.7f)<0.00001f,"rim and center can be adjusted independently during rendering");
        const auto beforeClear=centerDraws;centerOpacityPercent.store(0);rimOpacityPercent.store(0);ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1 && ringOpacity==0 && centerDraws==beforeClear && pendingCircle.valid,"both layers can be invisible without losing item alignment");
        rimOpacityPercent.store(100);centerOpacityPercent.store(35);
        for(unsigned size:{85u,90u,100u}) {
            char command[16]{};std::snprintf(command,sizeof(command),"size %u",size);
            nativeChatEntry->execute(command);
            for(bool filled:{false,true}) {
                ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
                Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,filled,&socketRing)==1 &&
                      std::abs(ringScale-(size+15)/200.0f)<0.00001f && ringOpacity==1 &&
                      centerScale==ringScale && centerPoint.x==ringPoint.x && centerPoint.y==ringPoint.y,"chat size 85/90/100 keeps both empty/filled circle layers growing at +15 points");
                Sockets::SpriteCenter circleCenter{};
                Check(Sockets::CenterOf(ringPoint,ringScale,{128,128,0,128},circleCenter) &&
                      std::abs(circleCenter.x-132)<=0.5 && std::abs(circleCenter.y-168)<=0.5,"enlarged ring retains native slot center");
                if(filled) {
                    runePoint={100,220};runeScale=1;runeOpacity=1;
                    Sockets::SpriteCenter artCenter{};
                    Check(TestScaleGate(&runePointer,&runePoint,&runeScale,&runeOpacity)==1 &&
                          std::abs(runeScale-size/100.0f)<0.00001f &&
                          Sockets::CenterOf(runePoint,runeScale,{64,96,0,96},artCenter) &&
                          std::abs(artCenter.x-circleCenter.x)<=0.5 && std::abs(artCenter.y-(circleCenter.y-1))<=0.5,"full-size rune preserves paired alignment and accepted one-pixel lift");
                } else Check(!pendingCircle.valid,"enlarged empty ring does not pair a loose item");
            }
        }
        // Establish an outer pending pair for the nested-scope checks below.
        ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1 && pendingCircle.valid,"outer filled ring retains a pending pair");
        {
            const auto outer=pendingCircle;SocketDrawScope nested;
            Check(!pendingCircle.valid,"nested socket scope cannot consume outer pending center");
            ringScale=1;ringPoint={100,200};ringOpacity=0.25f;
            Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false,&socketRing)==1 && !pendingCircle.valid,"nested empty socket leaves its scope unpaired");
            Check(outer.valid,"outer target retained by nested scope");
        }
        Check(pendingCircle.valid,"outer circle pairing restored after nested draw");
        const auto ringPointer=socketRing.hd;socketRing.hd=0;ringOpacity=0.25f;ringScale=1;ringPoint={100,200};
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && ringScale==1 && !pendingCircle.valid,"unavailable ring retains native sprite/alpha without stale centering");
        socketRing.hd=ringPointer;
        socketCenter.hd=0;ringOpacity=0.25f;ringScale=1;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && !pendingCircle.valid,"missing center layer refuses only custom rendering");
        socketCenter.hd=ringPointer;drawSprite=nullptr;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && !pendingCircle.valid,"refused layered draw guard retains native circles");
        drawSprite=CaptureCenter;
        ringOpacity=std::numeric_limits<float>::quiet_NaN();
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && std::isnan(ringOpacity),"invalid native opacity passes through untouched");
        scalePercent.store(65);enabled.store(false);ringOpacity=0.25f;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && ringScale==1,"disabled mode preserves circle size and opacity");
        enabled.store(true);hdMode=false;
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && ringScale==1,"classic circle display stays native");
        hdMode=true;circleReady.store(false);
        Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,false)==1 && ringOpacity==0.25f && ringScale==1,"refused circle hook keeps native behavior");
    }
    Check(!pendingCircle.valid,"socket scope exits without leaking circle center");
    circleReady.store(true);socketOriginal=reinterpret_cast<void*>(CaptureSocketRender);
    for(const char* arg:{"size 75","opacity 40","ring 70","center 20","lift 2"}) nativeChatEntry->execute(arg);
    for(const char* mode:{"default","always","off"}) {
        nativeChatEntry->execute(mode);
        const bool styled=std::strcmp(mode,"off")!=0,forced=std::strcmp(mode,"always")==0;
        Check(enabled.load()==styled && (!styled || alwaysVisible.load()==forced),"chat modes select forced/styled native/native appearance policies");
        for(bool filled:{false,true}) {
            SocketRenderCase draw{.circle=&hdPointer,.art=&runePointer,.styled=styled,.filled=filled};socketRenderCase=&draw;
            const auto before=draws;
            Check(TestPostIconGate(&widget,&item)==1 && draws==before+(forced?1:0),"default/off do not add socket draws; always forces one after icon");
            SocketHook(&widget,&item);
            Check(draws==before+1 && socketDrawDepth==0 && !pendingCircle.valid,"native hover draws once, forced duplicate is suppressed, and no scope/pair leaks");
            if(styled) {
                Check(std::abs(draw.circleScale-0.45f)<0.00001f && std::abs(draw.circleOpacity-0.7f)<0.00001f &&
                      std::abs(centerAlpha-0.2f)<0.00001f && centerScale==draw.circleScale,"default and always share size/ring/center settings for empty and filled sockets");
                if(filled) {
                    Sockets::SpriteCenter ringCenter{},artCenter{};
                    Check(std::abs(draw.artScale-0.75f)<0.00001f && std::abs(draw.artOpacity-0.2f)<0.00001f &&
                          Sockets::CenterOf(draw.circlePoint,draw.circleScale,{128,128,0,128},ringCenter) &&
                          Sockets::CenterOf(draw.artPoint,draw.artScale,{64,96,0,96},artCenter) &&
                          std::abs(artCenter.x-ringCenter.x)<=0.5 && std::abs(artCenter.y-(ringCenter.y-2))<=0.5,"native hover retains inserted size/fade and paired lift just like always mode");
                }
            } else {
                Check(draw.circleScale==1 && draw.circleOpacity==0.25f && draw.circlePoint.x==100 && draw.circlePoint.y==200 &&
                      draw.artScale==1 && draw.artOpacity==0.5f && draw.artPoint.x==100 && draw.artPoint.y==220,"off restores native sprite geometry and alpha without discarding settings");
            }
            point={100,200};nativeScale=1;nativeOpacity=0.5f;
            Check(TestScaleGate(&hdPointer,&point,&nativeScale,&nativeOpacity)==1 && point.x==100 && point.y==200 && nativeScale==1 && nativeOpacity==0.5f,"loose art stays native after every display mode and slot state");
        }
        Check(scalePercent.load()==75 && opacityPercent.load()==40 && rimOpacityPercent.load()==70 && centerOpacityPercent.load()==20 && insertedLiftPixels.load()==2,"mode changes retain every appearance setting");
    }
    socketOriginal=reinterpret_cast<void*>(CaptureDraw);socketRenderCase=nullptr;
    nativeChatEntry->execute("default");nativeChatEntry->execute("default");
    Check(enabled.load() && !alwaysVisible.load(),"default is an idempotent mode selection");
    for(const char* arg:{"size 65","opacity 65","ring 100","center 35","lift 1"}) nativeChatEntry->execute(arg);
    opacityPercent.store(65);circleReady.store(false);
    scaleReady.store(false);
    ChatCommand("",nullptr);Check(!enabled.load(),"bare slash arguments toggle off");
    ChatCommand(nullptr,nullptr);Check(enabled.load() && !alwaysVisible.load(),"bare toggle resumes the selected default mode");
    ChatCommand("off",nullptr);Check(!enabled.load(),"explicit off");
    ChatCommand("status",nullptr);Check(!enabled.load(),"status preserves state");
    ChatCommand("on",nullptr);Check(enabled.load() && alwaysVisible.load(),"legacy on selects always mode");
    ChatCommand("size 80",nullptr);Check(scalePercent.load()==80 && enabled.load(),"session size command");
    circleReady.store(true);
    {SocketDrawScope scope;ringScale=1;ringOpacity=0.25f;ringPoint={100,200};
     Check(TestCircleGate(&hdPointer,&ringPoint,&ringScale,&ringOpacity,true,&socketRing)==1 && std::abs(ringScale-0.475f)<0.00001f,"size 80 command sets custom ring to 95 percent native-relative extent");}
    circleReady.store(false);
    ChatCommand("size 101",nullptr);Check(scalePercent.load()==80,"out of range command ignored");
    ChatCommand("opacity 45",nullptr);Check(opacityPercent.load()==45 && scalePercent.load()==80,"opacity command is independent of size");
    ChatCommand("opacity 100",nullptr);Check(opacityPercent.load()==100,"native opacity command");
    ChatCommand("opacity 101",nullptr);Check(opacityPercent.load()==100,"out of range opacity ignored");
    ChatCommand("ring 45",nullptr);Check(rimOpacityPercent.load()==45 && centerOpacityPercent.load()==35 && opacityPercent.load()==100,"ring command is independent of center and inserted opacity");
    ChatCommand("center 70",nullptr);Check(centerOpacityPercent.load()==70 && rimOpacityPercent.load()==45,"center command does not alter rim");
    ChatCommand("ring 0",nullptr);ChatCommand("center 0",nullptr);
    Check(rimOpacityPercent.load()==0 && centerOpacityPercent.load()==0,"both ring controls accept invisible zero");
    ChatCommand("ring 100",nullptr);ChatCommand("center 100",nullptr);
    Check(rimOpacityPercent.load()==100 && centerOpacityPercent.load()==100,"both ring controls accept fully opaque 100");
    ChatCommand("ring -1",nullptr);ChatCommand("center 101",nullptr);
    Check(rimOpacityPercent.load()==100 && centerOpacityPercent.load()==100,"invalid ring opacity commands preserve state");
    const auto beforeRemovedAlias=configWrites;
    Check(Invoke("rim 60")==D2RL::ConsoleCommandResult::InvalidArguments && rimOpacityPercent.load()==100 && configWrites==beforeRemovedAlias,"removed rim command does not change state or write config");
    ChatCommand("lift 2",nullptr);Check(insertedLiftPixels.load()==2 && scalePercent.load()==80 && centerOpacityPercent.load()==100,"lift control is independent of size and opacity");
    for(const char* arg:{"lift -1","lift 9","lift 1.5","lift 2px"}) ChatCommand(arg,nullptr);
    Check(insertedLiftPixels.load()==2,"invalid lift commands preserve previous value");
    const auto beforeAppearanceSaves=configWrites;
    for(const char* arg:{"size 72","opacity 55","ring 81","center 42","lift 3"}) nativeChatEntry->execute(arg);
    unsigned rejected{};auto persisted=Sockets::ParseConfig(savedConfig,rejected);
    Check(configWrites==beforeAppearanceSaves+5 && !rejected && persisted.scalePercent==72 && persisted.opacityPercent==55 && persisted.rimOpacityPercent==81 && persisted.centerOpacityPercent==42 && persisted.insertedLiftPixels==3,"all five native chat appearance commands save values that startup parsing restores");
    Check(savedConfig.starts_with("# keep this note\n") && savedConfig.find("# outline")!=std::string::npos && savedConfig.ends_with("[other]\nkeep = 123\n"),"automatic saves preserve comments and unrelated section contents");
    const auto beforeModes=configWrites,beforeModeReads=configReads;
    for(const char* arg:{"default","always","off","status","on","toggle","toggle"}) Check(Invoke(arg)==D2RL::ConsoleCommandResult::Handled,"session mode/status command succeeds");
    for(const char* arg:{"size 101","opacity 9","ring -1","center 101","lift 9","rim 80"}) Check(Invoke(arg)==D2RL::ConsoleCommandResult::InvalidArguments,"invalid appearance and removed alias are refused");
    Check(configWrites==beforeModes && configReads==beforeModeReads && savedConfig.find("mode =")==std::string::npos,"modes/status/invalid commands perform no config IO and do not persist startup mode");
    const auto beforeFailedWrite=savedConfig;failConfigWrite=true;
    Check(Invoke("center 44")==D2RL::ConsoleCommandResult::Failed && centerOpacityPercent.load()==44 && savedConfig==beforeFailedWrite && saveWarnings==1,"write failure is reported while session appearance remains usable");
    failConfigWrite=false;Check(Invoke("center 44")==D2RL::ConsoleCommandResult::Handled,"retry saves after write failure");
    failConfigRead=true;const auto beforeReadFailure=configWrites;
    Check(Invoke("ring 83")==D2RL::ConsoleCommandResult::Failed && rimOpacityPercent.load()==83 && configWrites==beforeReadFailure && saveWarnings==2,"read failure never replaces an unread config");
    failConfigRead=false;Check(Invoke("ring 83")==D2RL::ConsoleCommandResult::Handled,"retry saves after read failure");
    const auto beforeOversize=savedConfig;savedConfig=std::string(Sockets::ConfigCapacity,'#');const auto beforeOversizeWrites=configWrites;
    Check(Invoke("lift 4")==D2RL::ConsoleCommandResult::Failed && configWrites==beforeOversizeWrites && saveWarnings==3,"oversized config is not truncated or replaced");
    savedConfig=beforeOversize;
    ChatCommand("invalid",nullptr);Check(enabled.load(),"invalid arguments preserve state");
    ChatCommand("off followed by overlong arguments",nullptr);Check(enabled.load(),"overlong arguments rejected");
    const auto beforeUnsupported=configWrites;
    ready.store(false);ChatCommand("off",nullptr);Check(enabled.load(),"unsupported build cannot alter feature");
    ChatCommand("size 65",nullptr);Check(configWrites==beforeUnsupported,"unsupported build cannot save appearance commands");
    ChatCommand("default",nullptr);Check(alwaysVisible.load(),"unsupported build cannot alter visibility mode");
    ready.store(true);
    D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,.argsLength=6,.args="toggle",.plugin=&context};
    Check(Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Handled && !enabled.load(),"console shares toggle handler");
    command.args=nullptr;command.argsLength=1;
    Check(Command(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Failed,"malformed borrowed arguments refused");
    D2RLoaderUnloadPlugin();ctx=nullptr;
    std::puts("Native chat registration/retained dispatch, lift rendering, console controls and unsupported refusal passed.");
}
