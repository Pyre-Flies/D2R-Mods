#pragma once
#include <D2RLPlugin/api.h>

namespace QolBelt {
bool Initialize(const D2RL::PluginContext* context) noexcept;
void Shutdown() noexcept;
bool Busy() noexcept;
bool RequestSingle(D2RL::PlayerHandle player, const D2RL::Items::ItemInfo& item,bool bindSharedFocus=false) noexcept;
bool RequestRefill(bool includeStash) noexcept;
bool RequestVendorRefill(const D2RL::PluginContext*, const D2RL::Items::ItemInfo&,
    bool (*shopOpen)() noexcept) noexcept;
// Called by the existing input polling loop; all inventory work is SDK-scheduled.
void Pump(const D2RL::PluginContext* context) noexcept;
}
