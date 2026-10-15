#include <D2RLPlugin/api.h>
#include <D2RLChat/native_commands.h>
#include "hash.h"
#include "scale.h"
#include "config.h"
#include "ring_resources.h"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <string_view>

extern "C" { void* SocketPostIconOriginal{}; void SocketPostIconEntry(); }
extern "C" { void* SocketScaleOriginal{}; void SocketScaleEntry(); }
extern "C" { void* SocketCircleOriginal{}; void SocketCircleEntry(); }

namespace {
const D2RL::PluginContext* ctx{};
std::atomic<bool> ready{},enabled{};
std::atomic<bool> alwaysVisible{};
std::atomic<bool> scaleReady{},scaleObserved{};
std::atomic<bool> opacityReady{},opacityObserved{};
std::atomic<bool> circleReady{},circleObserved{};
std::atomic<unsigned> alignmentSamples{};
Sockets::RingResources ringResources;
struct NativeSprite {std::uintptr_t hd{},classic{};};
thread_local NativeSprite socketRing{},socketCenter{};
thread_local bool ringAttempted{};
std::atomic<bool> ringRegistered{};
using LoadSpriteFn=void(__fastcall*)(NativeSprite*,const char*,bool);
using DrawSpriteFn=void(__fastcall*)(void*,Sockets::Point*,void*,float);
DrawSpriteFn drawSprite{};
std::atomic<unsigned> scalePercent{Sockets::DefaultScalePercent};
std::atomic<unsigned> opacityPercent{Sockets::DefaultOpacityPercent};
std::atomic<unsigned> rimOpacityPercent{Sockets::DefaultRimOpacityPercent};
std::atomic<unsigned> centerOpacityPercent{Sockets::DefaultCenterOpacityPercent};
std::atomic<unsigned> insertedLiftPixels{Sockets::DefaultInsertedLiftPixels};
using IsHdFn=bool(__fastcall*)();
IsHdFn isHd{};
D2RLChat::NativeCommands chatCommands;
constexpr char CommandHelp[]="always/default/off; size/opacity; ring/center; lift; status";
// Native description[64] includes its terminator; an overlong help string
// rejects the entire slash-command registration.
static_assert(sizeof(CommandHelp)<=64);
using Draw=void(__fastcall*)(void*,void*);
void* socketOriginal{};
// Post-icon native frame: RDI=widget, RSI=item, RSP aligned to 16.
constexpr unsigned char PostIconBytes[]={0x0f,0x28,0x74,0x24,0x50,0x48,0x8b,0x5c,0x24,0x60,0x48,0x83,0xc4,0x68};
constexpr unsigned char SocketBytes[]={0x40,0x55,0x53,0x41,0x54,0x48,0x8d,0xac,0x24,0x70,0xff,0xff,0xff,0x48,0x81,0xec,0x90,0x01,0x00,0x00};
constexpr unsigned char ScaleBytes[]={0x0f,0x28,0xde,0x4c,0x8d,0x44,0x24,0x60,0x48,0x8d,0x54,0x24,0x28,0x49,0x8b,0xcf};
constexpr unsigned char ModeBytes[]={0x48,0x8b,0x05,0x59,0x9f,0xbf,0x02,0x48,0x85,0xc0,0x74,0x08,0x0f,0xb6,0x80,0xb9,0x00,0x00,0x00};
constexpr unsigned char LayoutBytes[]={0x41,0x8b,0x86,0xd0,0x00,0x00,0x00};
constexpr unsigned char OpacityBytes[]={0x80,0x7f,0x04,0x00,0xf3,0x0f,0x11,0x44,0x24,0x70,0x74,0x0a,0xf3,0x0f,0x10,0x07,0xf3,0x0f,0x11,0x44,0x24,0x70};
constexpr unsigned char OpacityReaderBytes[]={0xf3,0x44,0x0f,0x10,0x66,0x10};
constexpr unsigned char CircleBytes[]={0xe8,0x21,0x0e,0x5b,0x00,0x4d,0x85,0xed,0x0f,0x84,0x84,0x00,0x00,0x00};
constexpr unsigned char CircleAlphaBytes[]={0xc7,0x44,0x24,0x70,0x00,0x00,0x80,0x3e};
constexpr unsigned char LoadSpriteEntryBytes[]={0x40,0x53,0x56,0x57,0x48,0x81,0xec,0x40,0x01,0x00,0x00};
constexpr unsigned char LoadSpriteBodyBytes[]={0xe8,0x51,0x59,0xf4,0xff,0x48,0x8b,0x4b,0x08,0xe8,0x38,0x48,0xf4,0xff,0x33,0xc0,0x48,0x89,0x03,0x48,0x89,0x43,0x08};
constexpr unsigned char DrawSpriteBytes[]={0x40,0x55,0x53,0x57,0x41,0x56,0x48,0x8d,0x6c,0x24,0xd8,0x48,0x81,0xec,0x28,0x01,0x00,0x00};
constexpr unsigned char DrawSettingsBytes[]={0xf2,0x44,0x0f,0x10,0x56,0x70};
struct CircleState {float* opacity;std::uintptr_t insertedEntry;};
static_assert(sizeof(CircleState)==16 && offsetof(CircleState,insertedEntry)==8);
thread_local unsigned socketDrawDepth{};
struct PendingCircle {Sockets::SpriteCenter center{};bool valid{};};
thread_local PendingCircle pendingCircle{};
struct SocketDrawScope {
    PendingCircle previous;
    SocketDrawScope() noexcept :previous(pendingCircle) {++socketDrawDepth;pendingCircle={};}
    ~SocketDrawScope() {pendingCircle=previous;--socketDrawDepth;}
};
void DrawSockets(void* widget,void* item) noexcept {
    const SocketDrawScope scope;
    reinterpret_cast<Draw>(socketOriginal)(widget,item);
}
bool Bytes(std::uintptr_t address,void* output,std::size_t size) noexcept {
    if(address<65536 || size>65536 || address>0x7fffffffffffULL-size) return false;
    SIZE_T read{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,size,&read) && read==size;
}
bool ReadBounds(void* sprite,Sockets::SpriteBounds& bounds) noexcept {
    std::uintptr_t hd{};std::int32_t fields[6]{};
    const auto object=reinterpret_cast<std::uintptr_t>(sprite);
    if(!Bytes(object,&hd,sizeof(hd)) || hd<65536 || hd>0x7fffffffffffULL-0xd4 || !Bytes(hd+0xbc,fields,sizeof(fields))) return false;
    bounds={fields[5],fields[1],fields[2],fields[3]};return true;
}
bool KnownSpriteLoader() noexcept {
    if(!ctx->CheckExpectedBytes(0x8589ba,LoadSpriteBodyBytes,sizeof(LoadSpriteBodyBytes))) return false;
    if(ctx->CheckExpectedBytes(0x858990,LoadSpriteEntryBytes,sizeof(LoadSpriteEntryBytes))) return true;
    const auto quality=GetModuleHandleW(L"d2rl-item-quality-backgrounds.dll");
    if(!quality || !Sockets::Hash(quality,"F3B6C2D532D22FD7BF32F1AE0EC8782B7735D1D028EEE52539E44D761B9861BA")) return false;
    const auto expected=reinterpret_cast<std::uintptr_t>(quality)+0xc8a0;
    constexpr unsigned char entry[]={0x40,0x53,0x48,0x83,0xec,0x20,0x45,0x0f,0xb6,0xd8,0x4c,0x8b,0xd2,0x48,0x8b,0xd9};
    unsigned char mapped[sizeof(entry)]{};
    if(!Bytes(expected,mapped,sizeof(mapped)) || std::memcmp(mapped,entry,sizeof(mapped))) return false;
    auto address=ctx->exeBase+0x858990;
    for(unsigned n=0;n<3;++n) {
        if(address==expected) return true;
        unsigned char code[14]{};
        if(!Bytes(address,code,sizeof(code))) return false;
        if(code[0]==0xe9) {std::int32_t displacement{};std::memcpy(&displacement,code+1,4);address=address+5+displacement;}
        else if(code[0]==0xff && code[1]==0x25 && !code[2] && !code[3] && !code[4] && !code[5]) std::memcpy(&address,code+6,8);
        else return false;
    }
    return address==expected;
}
bool GetRing(Sockets::SpriteBounds& bounds) noexcept {
    if(!ringAttempted && ringRegistered) {
        ringAttempted=true;
        if(KnownSpriteLoader()) {
            const auto load=reinterpret_cast<LoadSpriteFn>(ctx->exeBase+0x858990);
            load(&socketRing,"d2rloader/always-show-sockets/empty-outline",false);
            load(&socketCenter,"d2rloader/always-show-sockets/socket-center",false);
        }
        if(!socketRing.hd || !socketCenter.hd) ctx->LogWarn("Sockets: ring layer/loader unavailable; native circles retained.");
    }
    // One native wrapper per drawing thread, retained to process exit so queued
    // native renderer commands never reference a freed texture.
    Sockets::SpriteBounds centerBounds{};
    return socketRing.hd && socketCenter.hd && ReadBounds(&socketRing,bounds) && ReadBounds(&socketCenter,centerBounds) &&
        bounds.width==centerBounds.width && bounds.height==centerBounds.height &&
        bounds.pivotX==centerBounds.pivotX && bounds.pivotY==centerBounds.pivotY;
}
void ScaleSprite(void* sprite,Sockets::Point* position,float* scale,float* opacity) noexcept {
    const auto paired=pendingCircle;pendingCircle={};
    if(!ready.load() || !enabled.load() || !scaleReady.load() || !socketDrawDepth || !isHd || !isHd() || !sprite || !position || !scale) return;
    if(opacityReady.load() && opacity && Sockets::FadeOpacity(*opacity,opacityPercent.load()) && !opacityObserved.exchange(true))
        ctx->LogInfo("Sockets: inserted-item opacity executed through the native ethereal sprite-opacity input.");
    Sockets::SpriteBounds bounds{};
    if(!ReadBounds(sprite,bounds)) return;
    Sockets::SpriteCenter original{};
    const bool observed=Sockets::CenterOf(*position,*scale,bounds,original);
    auto artworkCenter=paired.center;
    artworkCenter.y-=insertedLiftPixels.load(); // Rendered pixels; circle layers stay fixed.
    const bool aligned=paired.valid && Sockets::ScaleAtCenter(*position,*scale,bounds,scalePercent.load(),artworkCenter);
    if(aligned && observed && alignmentSamples.load()<8 && alignmentSamples.fetch_add(1)<8) {
        char message[192]{};
        std::snprintf(message,sizeof(message),"Sockets: paired inserted sprite %dx%d center correction x=%.2f y=%.2f pixels.",bounds.width,bounds.height,artworkCenter.x-original.x,artworkCenter.y-original.y);
        ctx->LogInfo(message);
    }
    if((aligned || Sockets::CenterScale(*position,*scale,bounds,scalePercent.load())) && !scaleObserved.exchange(true))
        ctx->LogInfo("Sockets: centered inserted-item scaling executed; loose-item draws remain native.");
}
void* CircleSprite(void* sprite,Sockets::Point* position,float* scale,const CircleState* state) noexcept {
    pendingCircle={};
    if(!ready.load() || !enabled.load() || !circleReady.load() || !socketDrawDepth || !isHd || !isHd() || !sprite || !position || !scale || !state) return sprite;
    Sockets::SpriteBounds bounds{};
    if(!ReadBounds(sprite,bounds)) return sprite;
    Sockets::SpriteBounds replacement{};
    alignas(16) unsigned char settings[0x78]{};
    // HD consumer reads settings through +0x77; use an owned copy for the
    // additional center draw and change only its alpha at +0x10.
    if(drawSprite && state->opacity && std::isfinite(*state->opacity) && *state->opacity>=0 && *state->opacity<=1 &&
       Bytes(reinterpret_cast<std::uintptr_t>(state->opacity)-0x10,settings,sizeof(settings)) &&
       GetRing(replacement) && Sockets::FitRing(*position,*scale,bounds,replacement,Sockets::CircleScalePercent(scalePercent.load()))) {
        const auto center=centerOpacityPercent.load(),rim=rimOpacityPercent.load();
        const float centerAlpha=static_cast<float>(center)/100;
        std::memcpy(settings+0x10,&centerAlpha,sizeof(centerAlpha));
        if(center) {auto centerPoint=*position;drawSprite(&socketCenter,&centerPoint,settings,*scale);}
        *state->opacity=static_cast<float>(rim)/100;
        pendingCircle.valid=state->insertedEntry && Sockets::CenterOf(*position,*scale,replacement,pendingCircle.center);
        if(!circleObserved.exchange(true)) ctx->LogInfo("Sockets: independent native ring/center layers executed; circle size is artwork +15 points (max 115%).");
        return &socketRing;
    }
    Sockets::CenterScale(*position,*scale,bounds,std::min(Sockets::CircleScalePercent(scalePercent.load()),100u));
    return sprite;
}
thread_local void* lastWidget{};
thread_local void* lastItem{};
void AfterIcon(void* widget,void* item) noexcept {
    lastWidget=nullptr;lastItem=nullptr;
    if(ready.load() && enabled.load() && alwaysVisible.load() && widget && item) {
        DrawSockets(widget,item);
        lastWidget=widget;lastItem=item;
    }
}
void __fastcall SocketHook(void* widget,void* item) noexcept {
    // Both existing selected-item paths draw the icon immediately before sockets.
    // Consume only a matching duplicate, preserving other native invocations.
    if(ready.load() && enabled.load() && alwaysVisible.load() && widget==lastWidget && item==lastItem) {
        lastWidget=nullptr;lastItem=nullptr;return;
    }
    DrawSockets(widget,item);
}
bool SaveAppearance(const char* key,unsigned value) noexcept {
    try {
        static std::mutex configMutex;
        const std::lock_guard guard(configMutex);
        char config[Sockets::ConfigCapacity]{};unsigned required{};
        if(!ctx->ReadConfig(config,sizeof(config),&required) || required>sizeof(config)) return false;
        const auto length=strnlen_s(config,sizeof(config));
        if(length==sizeof(config)) return false;
        const auto updated=Sockets::UpdateConfig(std::string_view(config,length),key,value);
        if(updated.size()>=sizeof(config)) return false;
        return ctx->WriteConfig(updated.c_str());
    } catch(...) {return false;}
}
D2RL::ConsoleCommandResult __cdecl Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept {
    if(!command || command->contextSize<D2RL::ConsoleCommandContextSize || command->plugin!=ctx ||
       command->argsLength>16 || (!command->args && command->argsLength)) return D2RL::ConsoleCommandResult::Failed;
    const auto arg=Sockets::Trim(std::string_view(command->args?command->args:"",command->argsLength));
    const bool sizeCommand=arg.starts_with("size "),opacityCommand=arg.starts_with("opacity ");
    const bool liftCommand=arg.starts_with("lift ");
    const bool ringCommand=arg.starts_with("ring "),centerCommand=arg.starts_with("center ");unsigned requested{};
    if(sizeCommand || opacityCommand) {
        if(!Sockets::ParsePercent(arg.substr(sizeCommand?5:8),requested)) return D2RL::ConsoleCommandResult::InvalidArguments;
    } else if(ringCommand || centerCommand) {
        if(!Sockets::ParseRingOpacity(arg.substr(ringCommand?5:7),requested)) return D2RL::ConsoleCommandResult::InvalidArguments;
    } else if(liftCommand) {
        if(!Sockets::ParseLift(arg.substr(5),requested)) return D2RL::ConsoleCommandResult::InvalidArguments;
    } else if(arg!="always" && arg!="default" && arg!="on" && arg!="off" && arg!="status" && arg!="toggle" && !arg.empty()) return D2RL::ConsoleCommandResult::InvalidArguments;
    if(!ready.load()) {ctx->WriteConsoleMessage("Sockets: unsupported build or hook unavailable.",D2RL::ConsoleMessageKind::Warning);return D2RL::ConsoleCommandResult::Failed;}
    if(sizeCommand) {scalePercent.store(requested);scaleObserved.store(false);circleObserved.store(false);}
    if(opacityCommand) {opacityPercent.store(requested);opacityObserved.store(false);}
    if(ringCommand) rimOpacityPercent.store(requested);
    if(centerCommand) centerOpacityPercent.store(requested);
    if(liftCommand) insertedLiftPixels.store(requested);
    if(arg=="always" || arg=="on") {alwaysVisible.store(true);enabled.store(true);}
    if(arg=="default") {alwaysVisible.store(false);enabled.store(true);}
    if(arg=="off") enabled.store(false);
    if(arg.empty() || arg=="toggle") enabled.store(!enabled.load());
    const char* changedKey=sizeCommand?"inserted_item_scale_percent":opacityCommand?"inserted_item_opacity_percent":
        ringCommand?"ring_opacity_percent":centerCommand?"ring_center_opacity_percent":liftCommand?"inserted_item_lift_pixels":nullptr;
    const bool saved=!changedKey || SaveAppearance(changedKey,requested);
    char message[256]{};
    const auto size=scalePercent.load();
    const auto mode=enabled.load()?(alwaysVisible.load()?"always (styled)":"default (styled hover)"):"off (native appearance)";
    std::snprintf(message,sizeof(message),"Sockets: %s; art size %u%% (%s); circles %u%% (%s); inserted opacity %u%% (%s); ring %u%%; center %u%%; lift %upx.",mode,scaleReady.load()?size:100,scaleReady.load()?"HD":"native",circleReady.load()?Sockets::CircleScalePercent(size):100,circleReady.load()?"HD":"native",opacityPercent.load(),opacityReady.load()?"native fade input":"native fallback",rimOpacityPercent.load(),centerOpacityPercent.load(),insertedLiftPixels.load());
    ctx->WriteConsoleMessage(message);
    if(arg!="status") ctx->LogInfo(message);
    if(!saved) {
        constexpr auto warning="Sockets: appearance applied for this session, but config save failed. Check config access/size and retry the command.";
        ctx->WriteConsoleMessage(warning,D2RL::ConsoleMessageKind::Warning);ctx->LogWarn(warning);
        return D2RL::ConsoleCommandResult::Failed;
    }
    if(changedKey) D2RL::LogInfoF(ctx,"Sockets: saved %s=%u to config.",changedKey,requested);
    return D2RL::ConsoleCommandResult::Handled;
}
void ChatCommand(const char* args,void*) noexcept {
    if(!ctx) return;
    const auto length=args?strnlen_s(args,17):0;
    if(length>16) return;
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<std::uint32_t>(length),.args=args,.plugin=ctx};
    Command(nullptr,&command,nullptr);
}
bool RegisterChatCommand() {
    return chatCommands.Start(ctx) && chatCommands.Register("sockets",ChatCommand,nullptr,CommandHelp);
}
}
extern "C" void SocketAfterIcon(void* widget,void* item) noexcept {AfterIcon(widget,item);}
extern "C" void SocketScaleSprite(void* sprite,Sockets::Point* position,float* scale,float* opacity) noexcept {ScaleSprite(sprite,position,scale,opacity);}
extern "C" void* SocketCircleSprite(void* sprite,Sockets::Point* position,float* scale,const CircleState* state) noexcept {return CircleSprite(sprite,position,scale,state);}
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{.infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
        .id="always-show-sockets",.name="Always Show Sockets",.version="1.3.1+rev.1",.author="PyreFly",
        .description="Native socket visibility and appearance controls.",.flags=D2RL::PluginFlags::Client|D2RL::PluginFlags::NativeHooks};
    return &info;
}
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if(!context || !context->GetApi()) return false;
    ctx=context;
    unsigned rejected{},required{};char config[Sockets::ConfigCapacity]{};
    if(ctx->ReadConfig(config,sizeof(config),&required) && required<=sizeof(config) && strnlen_s(config,sizeof(config))<sizeof(config)) {
        const auto settings=Sockets::ParseConfig(config,rejected);scalePercent.store(settings.scalePercent);opacityPercent.store(settings.opacityPercent);
        rimOpacityPercent.store(settings.rimOpacityPercent);centerOpacityPercent.store(settings.centerOpacityPercent);
        insertedLiftPixels.store(settings.insertedLiftPixels);
    }
    if(rejected) ctx->LogWarn("Sockets: invalid setting ignored; valid values or per-setting defaults retained.");
    if(!ctx->RegisterConsoleCommand("sockets",Command,CommandHelp)) return false;
    if(!Sockets::Hash(reinterpret_cast<HMODULE>(ctx->exeBase),"93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10") ||
       !Sockets::Hash(GetModuleHandleW(L"D2RCore.dll"),"2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2") ||
       !ctx->CheckExpectedBytes(0x2a73a0,PostIconBytes,sizeof(PostIconBytes)) ||
       !ctx->CheckExpectedBytes(0x2a73c0,SocketBytes,sizeof(SocketBytes))) {
        ctx->LogWarn("Sockets: exact-build guard refused; native display preserved.");return true;
    }
    if(!ctx->InstallInlineHook(0x2a73c0,SocketBytes,sizeof(SocketBytes),reinterpret_cast<void*>(SocketHook),&socketOriginal) || !socketOriginal) return true;
    if(!ctx->InstallInlineHook(0x2a73a0,PostIconBytes,sizeof(PostIconBytes),reinterpret_cast<void*>(SocketPostIconEntry),&SocketPostIconOriginal) || !SocketPostIconOriginal) return true;
    const bool spriteProfile=ctx->CheckExpectedBytes(0x846210,ModeBytes,sizeof(ModeBytes)) && ctx->CheckExpectedBytes(0x79daf1,LayoutBytes,sizeof(LayoutBytes));
    if(spriteProfile) isHd=reinterpret_cast<IsHdFn>(ctx->exeBase+0x846210);
    if(spriteProfile && ctx->CheckExpectedBytes(0x858510,DrawSpriteBytes,sizeof(DrawSpriteBytes)) &&
       ctx->CheckExpectedBytes(0x79dc3e,DrawSettingsBytes,sizeof(DrawSettingsBytes)))
        drawSprite=reinterpret_cast<DrawSpriteFn>(ctx->exeBase+0x858510);
    else ctx->LogWarn("Sockets: layered sprite draw guard refused; native circles retained.");
    if(spriteProfile && ctx->CheckExpectedBytes(0xc71e3,ScaleBytes,sizeof(ScaleBytes)) &&
       ctx->InstallInlineHook(0xc71e3,ScaleBytes,sizeof(ScaleBytes),reinterpret_cast<void*>(SocketScaleEntry),&SocketScaleOriginal) && SocketScaleOriginal) {
        scaleReady.store(true);
        opacityReady.store(ctx->CheckExpectedBytes(0xc71cd,OpacityBytes,sizeof(OpacityBytes)) && ctx->CheckExpectedBytes(0x79e062,OpacityReaderBytes,sizeof(OpacityReaderBytes)));
        if(!opacityReady.load()) ctx->LogWarn("Sockets: opacity guards refused; native opacity retained, scaling still available.");
    } else ctx->LogWarn("Sockets: inserted-size guard/hook refused; native-size sockets remain available.");
    if(spriteProfile && ctx->CheckExpectedBytes(0x2a76ea,CircleBytes,sizeof(CircleBytes)) && ctx->CheckExpectedBytes(0x2a75f2,CircleAlphaBytes,sizeof(CircleAlphaBytes)) &&
       ctx->CheckExpectedBytes(0x79e062,OpacityReaderBytes,sizeof(OpacityReaderBytes)) &&
       ctx->InstallInlineHook(0x2a76ea,CircleBytes,sizeof(CircleBytes),reinterpret_cast<void*>(SocketCircleEntry),&SocketCircleOriginal) && SocketCircleOriginal) circleReady.store(true);
    else ctx->LogWarn("Sockets: circle guards/hook refused; rings retain native size and opacity.");
    if(circleReady.load()) {
        ringRegistered=ringResources.Start(ctx);
        if(!ringRegistered) ctx->LogWarn("Sockets: ring resources unavailable; native circles retained.");
    }
    ready.store(true);alwaysVisible.store(false);enabled.store(true);
    try {
        if(!RegisterChatCommand())
            D2RL::LogWarnF(ctx,"Sockets: native chat registration unavailable (%s); debug-console command remains available.",chatCommands.Reason().c_str());
    } catch(...) {ctx->LogWarn("Sockets: native chat registration failed; debug-console command remains available.");}
    ctx->LogInfo("Sockets 1.3.1+rev.1: admitted; startup default (styled native hover); appearance commands save automatically.");
    return true;
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {chatCommands.Stop();enabled.store(false);ready.store(false);scaleReady.store(false);opacityReady.store(false);circleReady.store(false);ringRegistered=false;ringResources.Stop(ctx);}
