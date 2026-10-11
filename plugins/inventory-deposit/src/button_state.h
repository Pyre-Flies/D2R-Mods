#pragma once
#include <D2RLPlugin/api.h>

namespace InventoryDepositButtonState {
bool Initialize(const D2RL::PluginContext* context) noexcept;
// UI thread only, after SDK setWidgetEnabled and before showing the child.
// Applies native ButtonWidget artwork state to this plugin's own child only.
bool Apply(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button, bool enabled) noexcept;
// Caller first stops and unregisters all button UI callbacks.
void Shutdown() noexcept;
}
