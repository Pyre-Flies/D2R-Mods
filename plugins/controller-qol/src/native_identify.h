#pragma once
#include "identify_action.h"
namespace QolNativeIdentify {
void Initialize(const D2RL::PluginContext*,const D2RL::ItemService*,const D2RL::ThreadService*) noexcept;
void Shutdown() noexcept;
bool Forwarding() noexcept;
bool Busy() noexcept;
// UI item-interaction callback only. Native client request; never edits SDK state.
bool RequestRemote(const D2RL::PluginContext*,D2RL::PlayerHandle,const QolIdentify::Info&) noexcept;
// Called only while handling the UI item action. Revalidated during native execution.
bool PersonalStashOpen() noexcept;
bool RequestAll(const D2RL::PluginContext*,D2RL::PlayerHandle,const QolIdentify::Info&,bool nativeMode,bool allowRemote) noexcept;
int Request(const D2RL::PluginContext*,D2RL::PlayerHandle,const QolIdentify::Info&,const QolIdentify::Info&) noexcept;
}
