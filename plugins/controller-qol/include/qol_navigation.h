#pragma once
#include <D2RLPlugin/api.h>
namespace QolNavigation {
bool Initialize(const D2RL::PluginContext* context, bool featureEnabled, bool traceEnabled = false) noexcept;
void SetTrace(bool enabled) noexcept;
void RefreshGroundLabels() noexcept;
void PumpLabels(const D2RL::PluginContext*,const D2RL::ThreadService*) noexcept;
void GetGlyphModes(bool& primary, bool& secondary) noexcept;
bool SharedPageInputActive() noexcept;
bool SharedPageRemapEnabled() noexcept;
bool OptionsRemapEnabled() noexcept;
bool ChronicleRemapEnabled() noexcept;
bool RangesRemapEnabled() noexcept;
bool GroundShortcutsAllowed() noexcept;
void Shutdown() noexcept;
}
