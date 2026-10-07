#pragma once
#include <D2RLPlugin/api.h>
#include "client_loot_policy.h"
namespace QolClientLoot {
using RankFn=int(*)(const char*,uint32_t) noexcept;
void Initialize(const D2RL::PluginContext*,RankFn) noexcept;
void Shutdown() noexcept;
bool Activate() noexcept;
void Deactivate() noexcept;
bool Active() noexcept;
bool Ready() noexcept; // Lock-free admission check for the native input gate.
// Called by the existing filtered tooltip callback; stores IDs, never units/buffers.
void Track(uint32_t guid,bool visible) noexcept;
int SlotFor(uint32_t guid) noexcept;
Candidate Slot(unsigned index) noexcept;
Request Capture(unsigned index) noexcept;
// UI-thread only. held includes controller/menu/feature eligibility.
bool Refresh(uint32_t radius,bool held) noexcept;
void Submit(const Request&,uint32_t radius,bool held) noexcept;
}
