#pragma once
#include <D2RLPlugin/api.h>

namespace InventoryDepositNative {
// Admission is read-only: no input hooks or native patches are installed.
bool Initialize(const D2RL::PluginContext* context) noexcept;
void Shutdown() noexcept;
bool Supported() noexcept;

// UI-thread only. The bulk queue may call this while holding its own mutex;
// this adapter must never call back into that queue while holding its mutex.
// Visibility keeps the supported stash context while allowing a held cursor
// item; Available retains the strict cursor checks required for any transfer.
bool Visible() noexcept;
bool Available() noexcept;
// UI-thread only. Strict cursor preflight plus a fresh, bounded inventory-only
// eligibility scan. Button admission only; never gate an active batch with it.
bool HasEligibleItems() noexcept;
}
