#pragma once
#include <D2RLPlugin/api.h>
namespace QolIdentifyStat {
void Initialize(const D2RL::PluginContext*) noexcept;
bool Read(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle,int32_t&) noexcept;
}
