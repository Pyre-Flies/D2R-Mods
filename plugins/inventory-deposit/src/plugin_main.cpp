#include <D2RLPlugin/api.h>
#include <D2RLChat/native_commands.h>
#include <D2RLSettings/settings.h>
#define INVENTORY_DEPOSIT_HAS_SETTINGS_LIB 1
#include "bulk_stash.h"
#include "deposit_button.h"
#include "button_state.h"
#include "deposit_native.h"
#include "gold_anchor.h"
#include "settings.h"
#include "stash_feedback.h"
#include "shared/chat_config.h"
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>

namespace {
constexpr D2RL::PluginInfo Info{
    .infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
    .id="inventory-deposit",.name="Inventory Deposit",.version="1.3.1+rev.16",
    .author="PyreFly",.description="Deposit eligible inventory items into advanced stash storage from a mouse button.",
    .flags=D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};
std::atomic<bool> running{};
// Never destroyed: a global std::thread still joinable when the process exits
// without unloading plugins is std::terminate, a 0xC0000409 crash (see time-played).
std::thread& worker = *new std::thread;
std::mutex wakeMutex;
std::condition_variable wake;
std::mutex commandMutex;
const D2RL::PluginContext* activeContext{};
D2RLChat::NativeCommands chatCommands;
#if INVENTORY_DEPOSIT_HAS_SETTINGS_LIB
D2RLSettings::Toggle rejuvDepositSetting;

void OnRejuvSettingChanged(bool value) noexcept {
    InventoryDepositNative::SetRejuvenationDepositEnabled(value);
    if(running.load()) {
        InventoryDepositButton::Refresh();
        wake.notify_all();
    }
    const std::lock_guard guard(commandMutex);
    if(activeContext) {
        activeContext->LogInfo(value
            ? "[InventoryDeposit] Rejuvenation potion deposit enabled (saved)."
            : "[InventoryDeposit] Rejuvenation potion deposit disabled (saved).");
    }
}

void SyncRejuvDepositSetting(bool value) noexcept {
    (void)rejuvDepositSetting.ReloadFromConfig(false);
    rejuvDepositSetting.SetValue(value,false,false);
}
#endif

void Reply(const char* text,bool warning=false) noexcept {
    if(!activeContext)return;
    activeContext->WriteConsoleMessage(text,warning?D2RL::ConsoleMessageKind::Warning:D2RL::ConsoleMessageKind::Output);
    if(warning)activeContext->LogWarn(text);
    else activeContext->LogInfo(text);
}

D2RL::ConsoleCommandResult __cdecl OnAutodepositCommand(
    D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept {
    using Result=D2RL::ConsoleCommandResult;
    using Action=InventoryDeposit::CommandAction;
    if(!command || command->contextSize<D2RL::ConsoleCommandContextSize ||
       !activeContext || command->plugin!=activeContext ||
       command->argsLength>64 || (!command->args && command->argsLength))return Result::Failed;
    const std::lock_guard guard(commandMutex);
    const std::string_view args(command->args?command->args:"",command->argsLength);
    const auto action=InventoryDeposit::ParseCommand(args);
    if(action==Action::Help) {
        Reply(InventoryDeposit::CommandHelp);
        return Result::Handled;
    }
    if(action==Action::Invalid) {
        char msg[128]{};
        std::snprintf(msg,sizeof(msg),"Inventory Deposit: invalid command. Usage: %s",InventoryDeposit::CommandHelp);
        Reply(msg,true);
        return Result::InvalidArguments;
    }
    const bool current=InventoryDepositNative::RejuvenationDepositEnabled();
    if(action==Action::Status) {
        char msg[128]{};
        std::snprintf(msg,sizeof(msg),"Inventory Deposit: rejuvenation potion deposit is %s (/autodeposit rejuv [on|off|toggle]).",
            current?"on":"off");
        Reply(msg);
        return Result::Handled;
    }
    const bool next=action==Action::RejuvToggle?!current:(action==Action::RejuvOn);
    if(!ChatConfig::Save(activeContext,"inventory_deposit",{{"deposit_rejuvenation_potions",next}},8192)) {
        char msg[160]{};
        std::snprintf(msg,sizeof(msg),
            "Inventory Deposit: could not save inventory-deposit.toml; rejuvenation potion deposit remains %s.",
            current?"on":"off");
        Reply(msg,true);
        return Result::Failed;
    }
    InventoryDepositNative::SetRejuvenationDepositEnabled(next);
#if INVENTORY_DEPOSIT_HAS_SETTINGS_LIB
    SyncRejuvDepositSetting(next);
#endif
    if(running.load()) {
        InventoryDepositButton::Refresh();
        wake.notify_all();
    }
    char msg[128]{};
    std::snprintf(msg,sizeof(msg),"Inventory Deposit: rejuvenation potion deposit %s (saved).",
        next?"enabled":"disabled");
    Reply(msg);
    return Result::Handled;
}

void OnNativeChatCommand(const char* args,void*) noexcept {
    if(!activeContext)return;
    const auto length=args?strnlen_s(args,65):0;
    if(length>64) {
        const std::lock_guard guard(commandMutex);
        Reply(InventoryDeposit::CommandHelp,true);
        return;
    }
    const D2RL::ConsoleCommandContext command{
        .contextSize=D2RL::ConsoleCommandContextSize,
        .argsLength=static_cast<uint32_t>(length),
        .args=args,
        .plugin=activeContext,
    };
    (void)OnAutodepositCommand(nullptr,&command,nullptr);
}

void OnNativeChatHelp(const char*,void*) noexcept {
    const std::lock_guard guard(commandMutex);
    Reply(InventoryDeposit::CommandHelp);
}

void Stop() noexcept {
#if INVENTORY_DEPOSIT_HAS_SETTINGS_LIB
    rejuvDepositSetting.Shutdown();
#endif
    chatCommands.Stop();
    running.store(false);wake.notify_all();
    if(worker.joinable())worker.join();
    InventoryDepositButton::Shutdown();
    InventoryDepositButtonState::Shutdown();
    InventoryDepositGoldAnchor::Shutdown();
    QolBulkStash::Shutdown();
    InventoryDepositStashFeedback::Shutdown();
    InventoryDepositNative::Shutdown();
    const std::lock_guard guard(commandMutex);
    activeContext=nullptr;
}
bool Visible() noexcept {return InventoryDepositNative::Visible();}
bool Enabled() noexcept {return !QolBulkStash::Busy() && InventoryDepositNative::HasEligibleItems();}
bool Request() noexcept {
    return Visible() && Enabled() && QolBulkStash::Request();
}
}
D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {return &Info;}
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if(!context)return false;
    InventoryDeposit::Settings settings{};
    char config[8192]{};uint32_t required{};
    if(context->ReadConfig(config,sizeof(config)-1,&required)) {
        if(!InventoryDeposit::ParseSettings(config,settings)) {
            context->LogWarn("[InventoryDeposit] Invalid configuration; plugin disabled.");return true;
        }
    } else if(required>=sizeof(config)) {
        context->LogWarn("[InventoryDeposit] Configuration too large; plugin disabled.");return true;
    }
    if(!settings.enabled) {context->LogInfo("[InventoryDeposit] Disabled by configuration.");return true;}
    InventoryDepositNative::SetRejuvenationDepositEnabled(settings.depositRejuvenationPotions);
    if(!InventoryDepositNative::Initialize(context)) {
        context->LogWarn("[InventoryDeposit] Required services/native profile unavailable; button disabled.");
        Stop();return true;
    }
    if(!InventoryDepositButtonState::Initialize(context)) {
        context->LogWarn("[InventoryDeposit] Native button visual state unavailable; button disabled.");
        Stop();return true;
    }
    // Feedback observes successful source-removal scans. Its callbacks only
    // copy identifiers; native tab drawing stays on the SDK UI thread.
    InventoryDepositStashFeedback::Initialize(context);
    const QolBulkStash::Observer feedback{
        nullptr,
        [](void*) noexcept { InventoryDepositStashFeedback::BeginBatch(); },
        [](void*,const D2RL::Items::ItemInfo& item) noexcept { InventoryDepositStashFeedback::ItemRemoved(item); },
        [](void*) noexcept { InventoryDepositStashFeedback::EndBatch(); },
        [](void*,const D2RL::Items::ItemInfo& item) noexcept { return InventoryDepositNative::AllowItem(item); },
    };
    // The queue must retain strict cursor preflight, independent of whether
    // its disabled button is still visible. Busy or an empty eligibility scan
    // cannot gate an active batch that still needs to confirm its last removal.
    QolBulkStash::Initialize(context,InventoryDepositNative::Available,&feedback);
    if(settings.anchorToGold && !InventoryDepositGoldAnchor::Initialize(context,settings.goldGap,settings.goldOffsetY)) {
        context->LogWarn("[InventoryDeposit] Required gold anchoring unavailable; plugin disabled.");Stop();return true;
    }
    if(!InventoryDepositButton::Initialize(context,Visible,Enabled,Request,settings.x,settings.y,
        settings.anchorToGold ? InventoryDepositGoldAnchor::Position : nullptr, InventoryDepositButtonState::Apply)) {
        context->LogWarn("[InventoryDeposit] UI extension unavailable; plugin disabled.");Stop();return true;
    }
    {
        const std::lock_guard guard(commandMutex);
        activeContext=context;
    }
#if INVENTORY_DEPOSIT_HAS_SETTINGS_LIB
    (void)rejuvDepositSetting.Register(context,{
        .key="deposit_rejuvenation_potions",
        .label="Deposit Rejuvenation Potions",
        .tooltip="Include rejuvenation and full rejuvenation potions when clicking the Deposit All button.",
        .section=D2RLSettings::Section::Ui,
        .defaultValue=true,
        .onChanged=OnRejuvSettingChanged,
        .tomlSection="inventory_deposit",
    });
    rejuvDepositSetting.SetValue(settings.depositRejuvenationPotions,false,false);
#endif
    if(!context->RegisterConsoleCommand("autodeposit",OnAutodepositCommand,InventoryDeposit::CommandHelp)) {
        context->LogWarn("[InventoryDeposit] Console command registration unavailable; Deposit All UI remains active.");
    }
    try {
        if(!chatCommands.Start(context) ||
           !chatCommands.Register("autodeposit",OnNativeChatCommand,nullptr,InventoryDeposit::CommandHelp,OnNativeChatHelp)) {
            context->LogWarn("[InventoryDeposit] Native chat command (/autodeposit) unavailable; console command remains active.");
        }
    } catch(...) {
        context->LogWarn("[InventoryDeposit] Native chat command registration failed; console command remains active.");
    }
    try {
        running.store(true);
        worker=std::thread([] {
            std::unique_lock lock(wakeMutex);
            while(running.load()) {
                lock.unlock();
                InventoryDepositButton::Refresh();
                InventoryDepositStashFeedback::Refresh();
                lock.lock();
                wake.wait_for(lock,std::chrono::milliseconds(100),[]{return !running.load();});
            }
        });
    } catch(...) {Stop();context->LogWarn("[InventoryDeposit] UI refresh worker unavailable; plugin disabled.");return false;}
    context->LogInfo(settings.anchorToGold
        ? "[InventoryDeposit] 1.3.1+rev.16 loaded; embedded artwork, gold-relative placement (button_x/button_y reserved for manual mode)."
        : "[InventoryDeposit] 1.3.1+rev.16 loaded; embedded artwork, manual fixed panel-local placement.");
    return true;
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {Stop();}
