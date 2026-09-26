#pragma once
#include <D2RLPlugin/api.h>
namespace QolMaterials {
bool Initialize(const D2RL::PluginContext*) noexcept;
void Shutdown() noexcept;
bool Busy() noexcept;
// UI-thread only. Returns true if the focused item is an advanced-stash proxy,
// including empty/refused requests, so it cannot fall through to normal moves.
bool TryWithdrawFocused(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
// UI-thread only; UINT32_MAX means closed/unavailable.
uint32_t SelectedStashTab(const D2RL::PluginContext*) noexcept;
bool RequestFocusedRefill(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
void RequestRefill() noexcept;
void Pump(const D2RL::PluginContext*) noexcept;
}
