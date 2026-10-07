#pragma once
#include "client_transfer_policy.h"
namespace QolClientTransfer {
bool Initialize(const D2RL::PluginContext*) noexcept;
void Shutdown() noexcept;
bool Busy() noexcept;
// Only after an Unavailable game-scheduler result. Stash requests are UI-only;
// explicit Cube requests can originate on the poller (UI preflight follows).
bool Request(const D2RL::PluginContext*,const Info&,bool cube) noexcept;
enum class BatchDepositResult { Refused, Ineligible, Submitted };
// UI-only, for the lifecycle-bound bulk queue after Unavailable scheduling.
// Caller owns observation; Submitted includes a native false return (no retry).
BatchDepositResult BatchDeposit(const D2RL::PluginContext*,const Info&) noexcept;
}
