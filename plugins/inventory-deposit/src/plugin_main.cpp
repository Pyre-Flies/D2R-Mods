#include <D2RLPlugin/api.h>
#include "bulk_stash.h"
#include "deposit_button.h"
#include "button_state.h"
#include "deposit_native.h"
#include "gold_anchor.h"
#include "settings.h"
#include "stash_feedback.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>

namespace {
constexpr D2RL::PluginInfo Info{
    .infoSize=D2RL::PluginInfoSize,.abiVersion=D2RL_PLUGIN_ABI_VERSION,
    .id="inventory-deposit",.name="Inventory Deposit",.version="0.1.0-beta.13",
    .author="PyreFly",.description="Deposit eligible inventory items into advanced stash storage from a mouse button.",
    .flags=D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};
std::atomic<bool> running{};
std::thread worker;
std::mutex wakeMutex;
std::condition_variable wake;

void Stop() noexcept {
    running.store(false);wake.notify_all();
    if(worker.joinable())worker.join();
    InventoryDepositButton::Shutdown();
    InventoryDepositButtonState::Shutdown();
    InventoryDepositGoldAnchor::Shutdown();
    QolBulkStash::Shutdown();
    InventoryDepositStashFeedback::Shutdown();
    InventoryDepositNative::Shutdown();
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
        ? "[InventoryDeposit] 0.1.0-beta.13 loaded; embedded artwork, gold-relative placement (button_x/button_y reserved for manual mode)."
        : "[InventoryDeposit] 0.1.0-beta.13 loaded; embedded artwork, manual fixed panel-local placement.");
    return true;
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {Stop();}
