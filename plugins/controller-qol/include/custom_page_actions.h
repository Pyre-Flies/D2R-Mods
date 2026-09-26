#pragma once
#include <D2RLPlugin/api.h>
namespace QolCustomPage {
bool Initialize(const D2RL::PluginContext*) noexcept;
void Shutdown() noexcept;
// UI thread only. No widget or provider context survives the call.
bool Visible(const D2RL::PluginContext*) noexcept;
// true includes refused moves: never fall through to an unrelated stash.
bool TryTransfer(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
}
