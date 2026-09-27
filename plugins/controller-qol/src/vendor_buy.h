#pragma once
#include <D2RLPlugin/api.h>
namespace QolVendorBuy {
// UI-thread only. Resolve stock through the merchant grid, never by container enum.
bool Check(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
void Submit(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&) noexcept;
}
