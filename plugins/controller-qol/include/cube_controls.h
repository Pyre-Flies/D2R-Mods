#pragma once
#include <cstdint>
namespace D2RL {struct PluginContext;struct WidgetService;namespace Lifecycle {struct GameplayEvent;}}
namespace QolCubeControls {
void Initialize(const D2RL::PluginContext*,const D2RL::WidgetService*,bool swap,bool transmute,const char* modifier) noexcept;
void OnLifecycle(const D2RL::Lifecycle::GameplayEvent*) noexcept;
void Cancel() noexcept;
void Shutdown() noexcept;
// Called exclusively inside the admitted native input/UI callback, never XInput.
uint16_t Input(uint16_t buttons,bool modifier,uint64_t now,bool busy) noexcept;
// Render thread uses recent owned observations; never dereferences native UI.
const char* PrimaryHint() noexcept;
const char* TransmuteHint() noexcept;
}
