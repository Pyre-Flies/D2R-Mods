#pragma once
#include <D2RLPlugin/api.h>
namespace GemBag::ServerFrame {
using Pump=void(*)(void*) noexcept;
bool Install(const D2RL::PluginContext*,Pump) noexcept;
void Stop() noexcept;
uintptr_t CurrentGame(const D2RL::PluginContext*) noexcept;
}
