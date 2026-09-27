#pragma once
#include <D2RLPlugin/api.h>
namespace QolGlyphs {
void Initialize(const D2RL::PluginContext* context, bool compatible) noexcept;
void PublishHeader(const char* modifier, const char* a, const char* x, const char* y, const char* stick) noexcept;
void Shutdown() noexcept;
void Status() noexcept;
}
