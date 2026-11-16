#pragma once
#include <D2RLPlugin/api.h>

namespace QolOptionsSettings {
#ifdef QOL_OPTIONS_SETTINGS_ENABLED
void Register(const D2RL::PluginContext* context) noexcept;
void Shutdown() noexcept;
#else
inline void Register(const D2RL::PluginContext*) noexcept {}
inline void Shutdown() noexcept {}
#endif
} // namespace QolOptionsSettings
