#pragma once

#include <D2RLPlugin/api.h>

namespace InventoryDepositButton {

using Predicate = bool(*)() noexcept;
using Positioner = bool(*)(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button) noexcept;
using StateApplier = bool(*)(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button, bool enabled) noexcept;

// Register during plugin load. All predicates run on the UI thread; request is
// invoked only from a later UI update, never inside native message dispatch.
// An optional positioner runs on that UI thread before showing/enabling each
// child and again before accepting a queued click. False hides that child.
// A state applier synchronizes native button visuals after SDK input enablement,
// before revealing the widget. Failure hides it and refuses queued clicks.
bool Initialize(const D2RL::PluginContext* context, Predicate visible,
    Predicate enabled, Predicate request, int x = 860, int y = 1620,
    Positioner position = nullptr, StateApplier applyState = nullptr) noexcept;

// Safe for the plugin's polling worker. At most one refresh is queued.
void Refresh() noexcept;
void Shutdown() noexcept;

}
