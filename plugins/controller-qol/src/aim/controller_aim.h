#pragma once
namespace D2RL { struct PluginContext; }
namespace QolAim {
bool Initialize(const D2RL::PluginContext*,bool qolEnabled) noexcept;
void Shutdown() noexcept;
bool OwnsGuidedArrow() noexcept;
}
