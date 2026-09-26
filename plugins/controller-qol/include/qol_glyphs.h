#pragma once
#include <D2RLPlugin/api.h>
namespace QolGlyphs {
void Initialize(const D2RL::PluginContext* context, bool compatible) noexcept;
void Shutdown() noexcept;
void Status() noexcept;
}
