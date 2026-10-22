#include "runtime.h"
#include "settings.h"
#include "hash.h"
#include <D2RLPlugin/api.h>
#include <D2RLChat/native_commands.h>
#include <mutex>
#include <cstdio>

namespace {
DayNight::Runtime runtime;
DayNight::Preview preview;
DayNight::Settings settings;
D2RLChat::NativeCommands chatCommands;
std::mutex commandMutex;
const D2RL::PluginContext* pluginContext{};
const D2RL::ThreadService* uiThreads{};
const D2RL::SharedEventService* uiEvents{};
D2RL::SharedEvents::ListenerHandle startupListener{};
constexpr char CommandHelp[]="/daynight [on|off|day|night|auto|toggle|status|help]";
static_assert(sizeof(CommandHelp)<=64);
struct Pending {bool active{},queued{},startup{};DayNight::Settings desired;std::string document;} pending;
DayNight::Preview::Observation lastStatus{};
bool lastStatusValid{};
void Reply(const char* text,bool warning=false) noexcept {
    if(!pluginContext)return;
    pluginContext->WriteConsoleMessage(text,warning?D2RL::ConsoleMessageKind::Warning:D2RL::ConsoleMessageKind::Output);
    if(warning)pluginContext->LogWarn(text);
}
void __cdecl RunPending(const D2RL::PluginContext*,void*) noexcept;
bool QueuePending() noexcept {
    if(!pending.active || pending.queued || !uiThreads || !pluginContext)return false;
    pending.queued=uiThreads->runOnUiThread(pluginContext,RunPending,nullptr)==D2RL::Threads::Result::Success;
    return pending.queued;
}
D2RL::SharedEvents::UiMessageAction __cdecl RetryStartup(const D2RL::PluginContext*,const D2RL::SharedEvents::UiMessageEvent*,void*) noexcept {
    const std::lock_guard guard(commandMutex);
    if(pending.active && pending.startup && !pending.queued)QueuePending();
    return D2RL::SharedEvents::UiMessageAction::Continue;
}
bool Apply(DayNight::Settings requested) noexcept {
    if(!runtime.Healthy())return false;
    if(!requested.enabled)return preview.DayForOff() && runtime.SetEnabled(false);
    return runtime.SetEnabled(true) && (requested.mode==DayNight::Mode::Auto?preview.Release():preview.Force(requested.mode));
}
void __cdecl RunPending(const D2RL::PluginContext* context,void*) noexcept {
    const std::lock_guard guard(commandMutex);
    if(!pluginContext || context!=pluginContext || !pending.active)return;
    pending.queued=false;
    DayNight::Preview::Checkpoint before;
    if(!preview.SaveCheckpoint(before)) {
        if(pending.startup && preview.AwaitingRenderer())return;
        pending={};Reply("Day Night Cycle: renderer unavailable or preview guards refused; setting unchanged.",true);return;
    }
    auto work=std::move(pending);pending={};
    const auto previous=settings;
    if(!Apply(work.desired)) {
        const bool rendererRestored=preview.Restore(before);
        const bool patchRestored=runtime.SetEnabled(previous.enabled);
        Reply(rendererRestored && patchRestored?"Day Night Cycle: mode change refused; previous state restored.":
            "Day Night Cycle: mode change/rollback refused; restart required.",true);return;
    }
    if(!work.startup && !pluginContext->WriteConfig(work.document.c_str())) {
        const bool rendererRestored=preview.Restore(before);
        const bool patchRestored=runtime.SetEnabled(previous.enabled);
        Reply(rendererRestored && patchRestored?"Day Night Cycle: config save failed; previous setting restored.":
            "Day Night Cycle: config save and rollback failed; restart required.",true);return;
    }
    settings=work.desired;
    char message[256]{};
    DayNight::Preview::Observation observed{};
    const bool sampled=preview.Inspect(observed);
    std::snprintf(message,sizeof(message),"Day Night Cycle: %s; mode=%s%s; renderer stage=%d blend=%.4f override=%u.",
        settings.enabled?"on":"off (daylight)",DayNight::ModeName(settings.mode),work.startup?"":" (saved)",
        sampled?observed.Stage():-1,sampled?observed.Blend():0.0f,sampled?observed.overrideTime:0);
    Reply(message);pluginContext->LogInfo(message);
}
void Status() noexcept {
    DayNight::Preview::Observation observed{};
    unsigned char gate{},noDebug{};
    const bool sampled=preview.Inspect(observed);
    DayNight::Read(pluginContext->exeBase+DayNight::Profile::NativeNoDebugRva,&noDebug,1);
    const auto core=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll"));
    if(core)DayNight::Read(core+DayNight::Profile::CoreSafetyFlagRva,&gate,1);
    char message[320]{};
    std::snprintf(message,sizeof(message),"Day Night Cycle: %s; mode=%s%s; renderer stage=%d blend=%.4f override=%u; changed=%s; coreGate=%u debug=%s.",
        runtime.Enabled()?"on":"off",DayNight::ModeName(settings.mode),pending.active?" (change pending)":"",
        sampled?observed.Stage():-1,sampled?observed.Blend():0.0f,sampled?observed.overrideTime:0,
        !sampled?"unavailable":!lastStatusValid?"first sample":observed==lastStatus?"no":"yes",gate&1,noDebug?"off":"on");
    lastStatus=observed;lastStatusValid=sampled;Reply(message);pluginContext->LogInfo(message);
}
D2RL::ConsoleCommandResult __cdecl Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept {
    using Result=D2RL::ConsoleCommandResult;
    using DayNight::Mode;
    if(!command || command->contextSize<D2RL::ConsoleCommandContextSize || !pluginContext || command->plugin!=pluginContext ||
       command->argsLength>32 || (!command->args && command->argsLength))return Result::Failed;
    try {
        const auto args=ChatConfig::Trim(std::string_view(command->args?command->args:"",command->argsLength));
        const std::lock_guard guard(commandMutex);
        if(args=="help"){Reply(CommandHelp);return Result::Handled;}
        if(args!="on" && args!="off" && args!="day" && args!="night" && args!="auto" && args!="toggle" && args!="status" && !args.empty()){
            Reply(CommandHelp,true);return Result::InvalidArguments;
        }
        if(!runtime.Healthy()){
            Reply("Day Night Cycle: unavailable; native compatibility or ownership check failed. Restart to retry.",true);return Result::Failed;
        }
        if(args=="status"){Status();return Result::Handled;}
        if(pending.active){Reply("Day Night Cycle: another change is pending; retry after the next frame.",true);return Result::Failed;}
        DayNight::Settings desired{.enabled=args=="off"?false:(args.empty() || args=="toggle")?!runtime.Enabled():true,
            .mode=args=="day"?Mode::Day:args=="night"?Mode::Night:Mode::Auto};
        const auto text=DayNight::ReadSettings(pluginContext);
        const auto parsed=text?DayNight::ParseConfig(*text):std::nullopt;
        const auto updated=parsed?DayNight::UpdateConfig(*text,desired):std::nullopt;
        if(!updated || updated->size()>=DayNight::ConfigCapacity){
            Reply("Day Night Cycle: config read/validation failed; setting unchanged.",true);return Result::Failed;
        }
        pending={true,false,false,desired,*updated};
        if(!QueuePending()){pending={};Reply("Day Night Cycle: UI queue unavailable; setting unchanged.",true);return Result::Failed;}
        return Result::Handled;
    }catch(...){Reply("Day Night Cycle: command failed; check the plugin log.",true);return Result::Failed;}
}
void ChatCommand(const char* args,void*) noexcept {
    const auto length=args?strnlen_s(args,33):0;
    if(length>32){Reply(CommandHelp,true);return;}
    const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<uint32_t>(length),.args=args,.plugin=pluginContext};
    Command(nullptr,&command,nullptr);
}
void ChatHelp(const char*,void*) noexcept {Reply(CommandHelp);}
bool RegisterChat(){return chatCommands.Start(pluginContext) && chatCommands.Register("daynight",ChatCommand,nullptr,CommandHelp,ChatHelp);}
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept;
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{
        .infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
        .id="day-night-cycle",.name="Day Night Cycle",.version="1.3.1+rev.1",.author="PyreFly",
        .description="Native day/night cycle with saved day, night and auto controls.",
        .flags=D2RL::PluginFlags::Client|D2RL::PluginFlags::NativeHooks};
    return &info;
}
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* ctx) noexcept -> bool {
    using namespace DayNight;
    if(!ctx || ctx->contextSize<sizeof(D2RL::PluginContext) || ctx->abiVersion!=D2RL_PLUGIN_ABI_VERSION || !ctx->GetApi())return false;
    try {
        std::unique_lock guard(commandMutex);
        if(pluginContext)return false;
        const auto coreModule=GetModuleHandleW(L"D2RCore.dll");const auto core=reinterpret_cast<uintptr_t>(coreModule);
        if(!FileHash(reinterpret_cast<HMODULE>(ctx->exeBase),Profile::HostHash) || !FileHash(coreModule,Profile::CoreHash)){
            ctx->LogWarn("Day Night Cycle: unsupported host/Core; lighting behavior retained.");return true;
        }
        const auto wrapper=GetProcAddress(coreModule,MAKEINTRESOURCEA(Profile::CoreWrapperExport));
        if(reinterpret_cast<uintptr_t>(wrapper)!=core+Profile::CoreWrapperRva || !runtime.Admit(ctx->exeBase,core)){
            ctx->LogWarn("Day Night Cycle: native code or binding guard refused; lighting behavior retained.");return true;
        }
        pluginContext=ctx;settings={};pending={};lastStatusValid=false;
        if(!preview.Start(ctx->exeBase,core))ctx->LogWarn("Day Night Cycle: instant-lighting guards refused; automatic cycle remains available.");
        if(ctx->QueryService(&uiThreads)!=D2RL::ServiceQueryResult::Success || ctx->QueryService(&uiEvents)!=D2RL::ServiceQueryResult::Success ||
           !D2RL::HasThreadServiceField(uiThreads,D2RL::ThreadServiceRequiredSize) || !uiThreads->runOnUiThread ||
           !D2RL::HasSharedEventServiceField(uiEvents,D2RL::SharedEventServiceRequiredSize) || !uiEvents->registerUiMessageListener || !uiEvents->unregisterUiMessageListener || !RegisterChat()){
            ctx->LogWarn("Day Night Cycle: chat/UI service registration refused.");guard.unlock();D2RLoaderUnloadPlugin();return false;
        }
        const D2RL::SharedEvents::UiMessageListener listener{sizeof(listener),0,0,0,RetryStartup,nullptr};
        if(uiEvents->registerUiMessageListener(ctx,&listener,&startupListener)!=D2RL::SharedEvents::Result::Success){guard.unlock();D2RLoaderUnloadPlugin();return false;}
        if(!ctx->RegisterConsoleCommand("daynight",Command,CommandHelp))ctx->LogWarn("Day Night Cycle: developer console command unavailable; normal chat retained.");
        const auto text=ReadSettings(ctx);const auto configured=text?ParseConfig(*text):std::nullopt;
        if(!configured){settings.enabled=false;ctx->LogWarn("Day Night Cycle: config unreadable or ambiguous; starting off.");}
        else settings=*configured;
        if(!runtime.SetEnabled(settings.enabled)){guard.unlock();D2RLoaderUnloadPlugin();return false;}
        if(settings.enabled && settings.mode!=Mode::Auto){pending={true,false,true,settings,{}};QueuePending();}
        ctx->LogInfo("Day Night Cycle 1.3.1+rev.1: ready; /daynight day, night, auto, off and status.");return true;
    }catch(...){ctx->LogError("Day Night Cycle: startup failed; restoring owned state.");D2RLoaderUnloadPlugin();return false;}
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    chatCommands.Stop();
    D2RL::SharedEvents::ListenerHandle listener{};
    const D2RL::PluginContext* context{};const D2RL::SharedEventService* service{};
    {const std::lock_guard guard(commandMutex);listener=startupListener;startupListener=0;context=pluginContext;service=uiEvents;}
    if(listener && context && service)service->unregisterUiMessageListener(context,listener);
    const std::lock_guard guard(commandMutex);
    pending={};
    if(!preview.Stop() && pluginContext)pluginContext->LogWarn("Day Night Cycle: preview restoration refused; a later owner was preserved.");
    const auto result=runtime.Stop();
    if(pluginContext && (result==DayNight::PatchResult::ProtectionFailed || result==DayNight::PatchResult::ProtectionRestoreFailed))pluginContext->LogError("Day Night Cycle: code restore failed; restart required.");
    pluginContext=nullptr;uiThreads=nullptr;uiEvents=nullptr;
}
