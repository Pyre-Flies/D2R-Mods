#pragma once
#include <D2RLPlugin/api.h>

namespace InventoryDepositGoldAnchor {
bool Initialize(const D2RL::PluginContext* context, int gap, int offsetY) noexcept;
// UI thread only, called before revealing the child and again before a click.
// Re-resolves native widgets every call; never retains their pointers.
bool Position(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button) noexcept;
// Caller first stops/unregisters the button's UI callbacks.
void Shutdown() noexcept;
}
