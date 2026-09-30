#pragma once
#include <cstdint>
namespace D2RL { struct PluginContext; }
namespace QolPortal {
void Initialize(const D2RL::PluginContext*, bool enabled, bool directLootEnabled,
    bool prioritizePortals, bool prioritizeStash, bool prioritizeWaypoints,
    bool prioritizeShrines, bool prioritizeChests, bool debug,
    const char* groundModifier, uint32_t radius) noexcept;
bool OwnsContactDestination(std::uintptr_t destination) noexcept;
void Shutdown() noexcept;
}
