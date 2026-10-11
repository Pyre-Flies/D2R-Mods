#include "deposit_button.h"
#include "button_art.h"
#include "button_art_resource.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace InventoryDepositButton {
namespace {

constexpr char LayoutPath[] = "data/global/ui/layouts/inventory-deposit/DepositAllButtonhd.json";
constexpr char ChildLocalId[] = "DepositAllButton";
constexpr char ChildName[] = "inventory-deposit/DepositAllButton";
constexpr char ClickPayload[] = "inventory-deposit/DepositAll";
struct ArtResource {unsigned id;const char* path;};
constexpr std::array<ArtResource,2> ArtResources{{
    {INVENTORY_DEPOSIT_BUTTON_SPRITE_RESOURCE_ID,
        "data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.sprite"},
    {INVENTORY_DEPOSIT_BUTTON_LOWEND_RESOURCE_ID,
        "data/hd/global/ui/d2rloader/inventory-deposit/deposit-button.lowend.sprite"},
}};

// The child inherits the inventory panel's origin. Coordinates match the
// Reimagined HD inventory footer; no stock layout or Charm Inventory resource
// is replaced. Custom four-state artwork is embedded in this DLL and served
// through the loader's virtual resource namespace, requiring no companion MPQ.
constexpr char LayoutTemplate[] = R"json({
    "type": "ButtonWidget",
    "name": "inventory-deposit/DepositAllButton",
    "fields": {
        "visible": false,
        "rect": { "x": %d, "y": %d, "width": 144, "height": 136 },
        "filename": "d2rloader/inventory-deposit/deposit-button",
        "normalFrame": 0,
        "hoveredFrame": 1,
        "pressedFrame": 2,
        "disabledFrame": 3,
        "tooltipString": "Deposit All Gems, Materials and Runes",
        "onClickMessage": "PanelManager:OpenPanel:inventory-deposit/DepositAll"
    }
})json";

constexpr std::array<const char*, 2> InventoryPanels{
    "PlayerInventoryExpansionLayout", "PlayerInventoryOriginalLayout"
};

std::recursive_mutex mutex;
const D2RL::PluginContext* context{};
const D2RL::ResourceService* resources{};
const D2RL::PanelService* panels{};
const D2RL::WidgetService* widgets{};
const D2RL::ThreadService* threads{};
const D2RL::SharedEventService* events{};
const D2RL::LifecycleService* lifecycle{};
D2RL::Resources::RegistrationHandle resource{};
std::array<D2RL::Resources::RegistrationHandle,ArtResources.size()> artResources{};
D2RL::Panels::ChildLayoutHandle child{};
D2RL::SharedEvents::ListenerHandle listener{};
std::array<D2RL::Lifecycle::ListenerHandle, 2> sessionListeners{};
std::array<D2RL::Widgets::WidgetHandle, InventoryPanels.size()> parents{}, buttons{};
Predicate visibleCheck{}, enabledCheck{}, requestAction{};
Positioner positionAction{};
StateApplier stateAction{};
std::uintptr_t generation{};
std::uintptr_t ownerGeneration{};
bool ready{}, refreshQueued{}, clickQueued{}, schedulingReported{};

bool Current(const D2RL::PluginContext* ctx, void* token) noexcept {
    return ready && ctx == context && reinterpret_cast<std::uintptr_t>(token) == generation;
}

bool UpdateWidgets() noexcept {
    const bool visible = visibleCheck && visibleCheck();
    const bool enabled = visible && enabledCheck && enabledCheck();
    bool usable{};
    for (std::size_t i = 0; i < InventoryPanels.size(); ++i) {
        // These handles are loader-owned paths, not native pointers. They
        // resolve again on every call and remain usable when a panel reopens.
        if (!parents[i] && widgets->findPanel(context, InventoryPanels[i], &parents[i]) != D2RL::Widgets::Result::Success)
            continue;
        if (!parents[i]) continue;
        if (!buttons[i] && widgets->findWidget(context, parents[i], ChildName, &buttons[i]) != D2RL::Widgets::Result::Success)
            continue;
        if (!buttons[i]) continue;
        if (!visible || (positionAction && !positionAction(InventoryPanels[i], parents[i], buttons[i]))) {
            (void)widgets->setWidgetEnabled(context, buttons[i], false);
            (void)widgets->setWidgetVisible(context, buttons[i], false);
            continue;
        }
        // The proven JSON field is visible; initialize native enabled state
        // through the SDK before ever showing the initially hidden child.
        if (widgets->setWidgetEnabled(context, buttons[i], enabled) != D2RL::Widgets::Result::Success) {
            (void)widgets->setWidgetVisible(context, buttons[i], false);
            continue;
        }
        // SDK enablement only changes the input flag. The guarded native
        // ButtonWidget operation also selects its disabled/enabled artwork.
        if (stateAction && !stateAction(InventoryPanels[i], parents[i], buttons[i], enabled)) {
            (void)widgets->setWidgetEnabled(context, buttons[i], false);
            (void)widgets->setWidgetVisible(context, buttons[i], false);
            continue;
        }
        if (widgets->setWidgetVisible(context, buttons[i], true) != D2RL::Widgets::Result::Success) {
            (void)widgets->setWidgetEnabled(context, buttons[i], false);
            continue;
        }
        usable = usable || enabled;
    }
    return usable;
}

void __cdecl Apply(const D2RL::PluginContext* ctx, void* token) noexcept {
    std::lock_guard lock(mutex);
    if (!Current(ctx, token)) return;
    refreshQueued = false;
    UpdateWidgets();
}

void __cdecl Click(const D2RL::PluginContext* ctx, void* token) noexcept {
    std::lock_guard lock(mutex);
    if (!Current(ctx, token)) return;
    // The stash, input mode, feature configuration, and conflicting activity
    // may have changed since the native button delivered its message.
    // Resolve and position a live child again in this callback. A stale click
    // must not move items if every child is missing, misaligned, or unwritable.
    if (UpdateWidgets() && visibleCheck && visibleCheck() &&
        enabledCheck && enabledCheck() && requestAction)
        (void)requestAction();
    clickQueued = false;
    UpdateWidgets();
}

bool Queue(D2RL::Threads::Callback callback, bool& pending) noexcept {
    if (pending) return true;
    pending = true;
    const auto result = threads->runOnUiThread(context, callback, reinterpret_cast<void*>(generation));
    if (result == D2RL::Threads::Result::Success) {
        schedulingReported = false;
        return true;
    }
    pending = false;
    if (!schedulingReported) {
        context->LogWarn("[InventoryDeposit] UI scheduling unavailable; button action refused.");
        schedulingReported = true;
    }
    return false;
}

auto __cdecl OnMessage(const D2RL::PluginContext* ctx,
    const D2RL::SharedEvents::UiMessageEvent* event, void* owner) noexcept
    -> D2RL::SharedEvents::UiMessageAction {
    using Action = D2RL::SharedEvents::UiMessageAction;
    if (!event || event->structSize < D2RL::SharedEvents::UiMessageEventRequiredSize ||
        !event->target || !event->command || !event->text ||
        std::strcmp(event->target, "PanelManager") != 0 ||
        std::strcmp(event->command, "OpenPanel") != 0 ||
        std::strcmp(event->text, ClickPayload) != 0)
        return Action::Continue;
    std::lock_guard lock(mutex);
    if (!ready || ctx != context || reinterpret_cast<std::uintptr_t>(owner) != ownerGeneration)
        return Action::Continue;
    (void)Queue(Click, clickQueued);
    // This reserved payload is an action, not a panel to open. Never allow it
    // to reach native PanelManager handling, even while the action is disabled.
    return Action::Consume;
}

void __cdecl OnSession(const D2RL::PluginContext* ctx,
    const D2RL::Lifecycle::GameplayEvent*, void* owner) noexcept {
    std::lock_guard lock(mutex);
    if (!ready || ctx != context || reinterpret_cast<std::uintptr_t>(owner) != ownerGeneration) return;
    // UI tasks survive session changes in the SDK. Invalidate clicks from the
    // previous session explicitly before a new player's inventory can appear.
    ++generation;
    refreshQueued = false;
    clickQueued = false;
    (void)Queue(Apply, refreshQueued);
}

bool Register(const D2RL::PluginContext* ctx, Predicate visible,
    Predicate enabled, Predicate request, int x, int y, Positioner position, StateApplier applyState) noexcept {
    std::lock_guard lock(mutex);
    if (!ctx || !visible || !enabled || !request || x < 0 || y < 0 || x > 4096 || y > 4096) return false;
    context = ctx;
    visibleCheck = visible;
    enabledCheck = enabled;
    requestAction = request;
    positionAction = position;
    stateAction = applyState;
    if (ctx->QueryService(&resources) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasResourceServiceField(resources, D2RL::ResourceServiceRequiredSize) ||
        !resources->registerResource || !resources->unregisterResource ||
        ctx->QueryService(&panels) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasPanelServiceField(panels, D2RL::PanelServiceUnregisterChildLayoutFieldEnd) ||
        !panels->registerChildLayout || !panels->unregisterChildLayout ||
        ctx->QueryService(&widgets) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasWidgetServiceField(widgets, D2RL::WidgetServiceRequiredSize) ||
        !widgets->findPanel || !widgets->findWidget || !widgets->setWidgetVisible || !widgets->setWidgetEnabled ||
        ctx->QueryService(&threads) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(threads, D2RL::ThreadServiceRequiredSize) || !threads->runOnUiThread ||
        ctx->QueryService(&events) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasSharedEventServiceField(events, D2RL::SharedEventServiceRequiredSize) ||
        !events->registerUiMessageListener || !events->unregisterUiMessageListener ||
        ctx->QueryService(&lifecycle) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasLifecycleServiceField(lifecycle, D2RL::LifecycleServiceRequiredSize) ||
        !lifecycle->registerGameplayEventListener || !lifecycle->unregisterGameplayEventListener) {
        ctx->LogWarn("[InventoryDeposit] Inventory button unavailable: required SDK services are missing.");
        return false;
    }
    char layoutBytes[sizeof(LayoutTemplate) + 32]{};
    const int layoutLength = std::snprintf(layoutBytes, sizeof(layoutBytes), LayoutTemplate, x, y);
    if (layoutLength < 0 || static_cast<std::size_t>(layoutLength) >= sizeof(layoutBytes)) return false;
    // Register both sprite variants before exposing a layout that refers to
    // them. The loader takes its own copy and owns each registration handle.
    for (std::size_t i=0;i<ArtResources.size();++i) {
        InventoryDepositButtonArt::Blob blob{};
        if (!InventoryDepositButtonArt::Load(ArtResources[i].id,blob) || !blob.bytes ||
            !blob.byteCount || blob.byteCount>InventoryDepositButtonArt::MaximumByteCount) {
            ctx->LogWarn("[InventoryDeposit] Embedded button artwork missing or invalid; button disabled.");
            return false;
        }
        const D2RL::Resources::ResourceRegistration art{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .path = ArtResources[i].path,
            .bytes = blob.bytes,
            .byteCount = static_cast<std::uint64_t>(blob.byteCount),
        };
        if (resources->registerResource(ctx,&art,&artResources[i]) != D2RL::Resources::Result::Success) {
            ctx->LogWarn("[InventoryDeposit] Button artwork registration failed; button disabled.");
            return false;
        }
    }
    const D2RL::Resources::ResourceRegistration layout{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .path = LayoutPath,
        .bytes = layoutBytes,
        .byteCount = static_cast<std::uint64_t>(layoutLength),
    };
    if (resources->registerResource(ctx, &layout, &resource) != D2RL::Resources::Result::Success) {
        ctx->LogWarn("[InventoryDeposit] Inventory button layout registration failed.");
        return false;
    }
    const D2RL::Panels::ChildLayoutRegistration registration{
        .structSize = D2RL::Panels::ChildLayoutRegistrationSize,
        .flags = D2RL::Panels::ChildLayoutFlags::KeyboardMouseOnly,
        .stockPanel = D2RL::Panels::StockPanel::PlayerInventory,
        .localId = ChildLocalId,
    };
    if (panels->registerChildLayout(ctx, &registration, &child) != D2RL::Panels::Result::Success) {
        ctx->LogWarn("[InventoryDeposit] Inventory button child registration failed.");
        return false;
    }
    const D2RL::SharedEvents::UiMessageListener subscription{
        .structSize = D2RL::SharedEvents::UiMessageListenerSize,
        .priority = 100,
        .callback = OnMessage,
        .userData = reinterpret_cast<void*>(ownerGeneration),
    };
    if (events->registerUiMessageListener(ctx, &subscription, &listener) != D2RL::SharedEvents::Result::Success) {
        ctx->LogWarn("[InventoryDeposit] Inventory button listener registration failed.");
        return false;
    }
    constexpr std::array<D2RL::Lifecycle::GameplayEventKind, 2> sessionKinds{
        D2RL::Lifecycle::GameplayEventKind::GameJoined, D2RL::Lifecycle::GameplayEventKind::GameLeft
    };
    for (std::size_t i = 0; i < sessionKinds.size(); ++i) {
        const D2RL::Lifecycle::GameplayEventListener session{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .kind = sessionKinds[i],
            .callback = OnSession,
            .userData = reinterpret_cast<void*>(ownerGeneration),
        };
        if (lifecycle->registerGameplayEventListener(ctx, &session, &sessionListeners[i]) != D2RL::Lifecycle::Result::Success) {
            ctx->LogWarn("[InventoryDeposit] Inventory button session listener registration failed.");
            return false;
        }
    }
    ready = true;
    ctx->LogInfo("[InventoryDeposit] Keyboard/mouse Deposit All inventory child registered.");
    return true;
}

}

bool Initialize(const D2RL::PluginContext* ctx, Predicate visible,
    Predicate enabled, Predicate request, int x, int y, Positioner position, StateApplier applyState) noexcept {
    Shutdown();
    if (Register(ctx, visible, enabled, request, x, y, position, applyState)) return true;
    Shutdown();
    return false;
}

void Refresh() noexcept {
    std::lock_guard lock(mutex);
    if (ready) (void)Queue(Apply, refreshQueued);
}

void Shutdown() noexcept {
    const D2RL::PluginContext* oldContext{};
    const D2RL::ResourceService* oldResources{};
    const D2RL::PanelService* oldPanels{};
    const D2RL::SharedEventService* oldEvents{};
    const D2RL::LifecycleService* oldLifecycle{};
    D2RL::Resources::RegistrationHandle oldResource{};
    std::array<D2RL::Resources::RegistrationHandle,ArtResources.size()> oldArtResources{};
    D2RL::Panels::ChildLayoutHandle oldChild{};
    D2RL::SharedEvents::ListenerHandle oldListener{};
    std::array<D2RL::Lifecycle::ListenerHandle, 2> oldSessionListeners{};
    {
        std::lock_guard lock(mutex);
        ready = false;
        ++generation;
        ++ownerGeneration;
        oldContext = context; oldResources = resources; oldPanels = panels; oldEvents = events;
        oldResource = resource; oldArtResources = artResources; oldChild = child; oldListener = listener;
        oldLifecycle = lifecycle; oldSessionListeners = sessionListeners;
        context = nullptr; resources = nullptr; panels = nullptr; widgets = nullptr;
        threads = nullptr; events = nullptr;
        lifecycle = nullptr; sessionListeners = {};
        resource = 0; artResources = {}; child = 0; listener = 0;
        parents = {}; buttons = {};
        visibleCheck = nullptr; enabledCheck = nullptr; requestAction = nullptr;
        positionAction = nullptr; stateAction = nullptr;
        refreshQueued = false; clickQueued = false; schedulingReported = false;
    }
    // Never hold our mutex while unregistering: the loader may wait for a
    // callback already in progress. Owner unload also discards queued UI work
    // and removes owner registrations if native teardown is already underway.
    if (!oldContext) return;
    if (oldListener && oldEvents && oldEvents->unregisterUiMessageListener)
        (void)oldEvents->unregisterUiMessageListener(oldContext, oldListener);
    if ((oldSessionListeners[0] || oldSessionListeners[1]) && oldLifecycle && oldLifecycle->unregisterGameplayEventListener)
        for (const auto handle : oldSessionListeners)
            if (handle) (void)oldLifecycle->unregisterGameplayEventListener(oldContext, handle);
    if (oldChild && oldPanels && oldPanels->unregisterChildLayout)
        (void)oldPanels->unregisterChildLayout(oldContext, oldChild);
    if (oldResource && oldResources && oldResources->unregisterResource)
        (void)oldResources->unregisterResource(oldContext, oldResource);
    if (oldResources && oldResources->unregisterResource)
        for (std::size_t i=oldArtResources.size();i>0;--i)
            if (oldArtResources[i-1])
                (void)oldResources->unregisterResource(oldContext,oldArtResources[i-1]);
}

}
