#pragma once
#include <D2RLPlugin/api.h>
#include "client_loot_policy.h"
namespace QolClientLoot {
using RankFn=int(*)(const char*,uint32_t) noexcept;
using BlockPickupFn=bool(*)(uint32_t guid,uint32_t code) noexcept;
using HeldFn=bool(*)() noexcept;
void Initialize(const D2RL::PluginContext*,RankFn,BlockPickupFn=nullptr,HeldFn=nullptr) noexcept;
void SetLifecycleAvailable(bool) noexcept;
void OnLifecycle(const D2RL::Lifecycle::GameplayEvent*) noexcept;
// Server hooks may inspect copied identity/one-use permits, never UI pointers.
bool IsLocalActor(uint32_t type,uint32_t id) noexcept;
bool TakeNativePickupPermit(uint32_t actorId,uint32_t guid) noexcept;
void Shutdown() noexcept;
bool Activate() noexcept;
void Deactivate() noexcept;
bool Active() noexcept;
bool HiddenForPickup(uint32_t guid) noexcept; // Qualified presentation query only.
bool CachedHiddenForPickup(uint32_t guid) noexcept; // Owned IDs only; safe for native/server guard.
bool Ready() noexcept; // Lock-free admission check for the native input gate.
uint64_t Epoch() noexcept;
// Called by the existing filtered tooltip callback; stores IDs, never units/buffers.
void Track(uint32_t guid,bool visible,const void* labelUnit=nullptr) noexcept;
int SlotFor(uint32_t guid) noexcept;
Candidate Slot(unsigned index) noexcept;
Request Capture(unsigned index) noexcept;
// UI-thread only. held includes controller/menu/feature eligibility.
bool Refresh(uint32_t radius,bool held) noexcept;
void Submit(const Request&,uint32_t radius,bool held) noexcept;
}
