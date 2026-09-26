#include <D2RLPlugin/api.h>

#include "map_config.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

namespace {

constexpr auto KeyCode = D2RL::Items::MakeItemCode("key");

std::atomic<std::int32_t> currentLevelId{0};
const D2RL::ItemService* itemService = nullptr;
MapAssistance::Configuration configuration;

bool Append(D2RL::SharedEvents::ItemTooltipEvent* event, std::string_view text) noexcept {
    if (event->length >= event->capacity || text.size() > event->capacity - event->length - 1) return false;
    std::memcpy(event->text + event->length, text.data(), text.size());
    event->length += static_cast<std::uint32_t>(text.size());
    event->text[event->length] = '\0';
    return true;
}

void __cdecl OnLevelChanged(const D2RL::PluginContext*, const D2RL::Lifecycle::GameplayEvent* event, void*) noexcept {
    if (!D2RL::Lifecycle::HasGameplayEventField(event, D2RL::Lifecycle::GameplayEventRequiredSize)) return;
    currentLevelId.store(event->currentValue, std::memory_order_release);
}

void __cdecl OnGameLeft(const D2RL::PluginContext*, const D2RL::Lifecycle::GameplayEvent* event, void*) noexcept {
    if (!D2RL::Lifecycle::HasGameplayEventField(event, D2RL::Lifecycle::GameplayEventRequiredSize)) return;
    currentLevelId.store(0, std::memory_order_release);
}

void __cdecl OnItemTooltip(const D2RL::PluginContext* context, D2RL::SharedEvents::ItemTooltipEvent* event, void*) noexcept {
    if (context == nullptr || event == nullptr || event->structSize < D2RL::SharedEvents::ItemTooltipEventRequiredSize ||
        event->text == nullptr || event->capacity == 0 || itemService == nullptr) return;

    D2RL::Items::ItemInfo info{};
    info.structSize = D2RL::Items::ItemInfoSize;
    if (itemService->getItemInfo(context, event->item, &info) != D2RL::Items::Result::Success || info.code != KeyCode) return;

    if (!configuration.enabled) return;
    const auto* zone = MapAssistance::FindZone(configuration, currentLevelId.load(std::memory_order_acquire));
    if (zone == nullptr) return;

    event->length = 0;
    event->text[0] = '\0';
    bool complete = Append(event, "Map Assistance - ") && Append(event, zone->name) && Append(event, " (") &&
        Append(event, zone->layout) && Append(event, ")\n");
    for (std::size_t index = 0; complete && index < zone->lines.size(); ++index) {
        if (index != 0) complete = Append(event, "\n");
        if (complete) complete = Append(event, zone->lines[index]);
    }
    if (!complete) {
        event->text[0] = '\0';
        event->length = 0;
    }
}

bool RegisterGameplayListener(const D2RL::PluginContext* context, const D2RL::LifecycleService* lifecycle,
    D2RL::Lifecycle::GameplayEventKind kind, D2RL::Lifecycle::GameplayEventCallback callback) noexcept {
    const D2RL::Lifecycle::GameplayEventListener listener{
        .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
        .kind = kind,
        .callback = callback,
    };
    D2RL::Lifecycle::ListenerHandle handle = D2RL::Lifecycle::InvalidHandle;
    return lifecycle->registerGameplayEventListener(context, &listener, &handle) == D2RL::Lifecycle::Result::Success &&
        handle != D2RL::Lifecycle::InvalidHandle;
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{
        .infoSize = D2RL::PluginInfoSize,
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .id = "map-assistance",
        .name = "Map Assistance",
        .version = "1.3.1+rev.1",
        .author = "PyreFly",
        .description = "Keys-tooltip map guidance inspired by Kryszard's PD2 Loot Filter.",
        .flags = D2RL::PluginFlags::Client,
    };
    return &info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (context == nullptr || context->GetApi() == nullptr) return false;

    try {
        std::vector<char> buffer(64 * 1024, '\0');
        std::uint32_t requiredSize = 0;
        if (!context->ReadConfig(buffer.data(), static_cast<std::uint32_t>(buffer.size() - 1), &requiredSize)) {
            if (requiredSize == 0 || requiredSize > 1024 * 1024) {
                context->LogError("Map Assistance: map-assistance.toml could not be read or exceeds 1 MiB.");
                return false;
            }
            buffer.assign(static_cast<std::size_t>(requiredSize) + 1, '\0');
            if (!context->ReadConfig(buffer.data(), static_cast<std::uint32_t>(buffer.size() - 1), &requiredSize)) {
                context->LogError("Map Assistance: map-assistance.toml could not be read.");
                return false;
            }
        }
        configuration = MapAssistance::ParseConfiguration(std::string_view(buffer.data()));
    } catch (...) {
        context->LogError("Map Assistance: configuration allocation or parsing failed.");
        return false;
    }
    if (configuration.zones.empty()) {
        context->LogError("Map Assistance: map-assistance.toml contains no valid zones.");
        return false;
    }

    const D2RL::SharedEventService* sharedEvents = nullptr;
    const D2RL::LifecycleService* lifecycle = nullptr;
    if (context->QueryService(&itemService) != D2RL::ServiceQueryResult::Success ||
        context->QueryService(&sharedEvents) != D2RL::ServiceQueryResult::Success ||
        context->QueryService(&lifecycle) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasItemServiceField(itemService, D2RL::ItemServiceRequiredSize) ||
        !D2RL::HasSharedEventServiceField(sharedEvents, D2RL::SharedEventServiceRequiredSize) ||
        !D2RL::HasLifecycleServiceField(lifecycle, D2RL::LifecycleServiceRequiredSize)) {
        context->LogError("Map Assistance: required Item, Shared Event, or Lifecycle service is unavailable.");
        return false;
    }

    const D2RL::SharedEvents::ItemTooltipListener tooltip{
        .structSize = D2RL::SharedEvents::ItemTooltipListenerSize,
        .slot = 100,
        .region = D2RL::SharedEvents::ItemTooltipRegion::Description,
        .position = D2RL::SharedEvents::ItemTooltipPosition::Bottom,
        .callback = OnItemTooltip,
    };
    D2RL::SharedEvents::ListenerHandle tooltipHandle = D2RL::SharedEvents::InvalidHandle;
    if (sharedEvents->registerItemTooltipListener(context, &tooltip, &tooltipHandle) != D2RL::SharedEvents::Result::Success ||
        tooltipHandle == D2RL::SharedEvents::InvalidHandle ||
        !RegisterGameplayListener(context, lifecycle, D2RL::Lifecycle::GameplayEventKind::LevelChanged, OnLevelChanged) ||
        !RegisterGameplayListener(context, lifecycle, D2RL::Lifecycle::GameplayEventKind::GameLeft, OnGameLeft)) {
        context->LogError("Map Assistance: listener registration failed; no tooltip text will be added.");
        return false;
    }

    char status[160]{};
    std::snprintf(status, sizeof(status), "Map Assistance 1.3.1+rev.1: loaded %zu configured zones (%zu rejected).",
        configuration.zones.size(), configuration.rejectedEntries);
    context->LogInfo(status);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    currentLevelId.store(0, std::memory_order_release);
    itemService = nullptr;
    configuration = {};
}
