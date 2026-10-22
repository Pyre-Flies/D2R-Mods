#pragma once
#include "layout.h"
#include <windows.h>
namespace GemBag::Geometry {
struct Bounds {POINT origin{};float scale{};int width{},height{};};
bool Initialize(const D2RL::PluginContext*,const D2RL::WidgetService*) noexcept;
void UsePortable(bool) noexcept;
// UI callback only. Resolve and validate fresh native objects every time.
bool Place(POINT anchor,int clientWidth,int clientHeight,bool controller,Bounds&) noexcept;
bool Hit(POINT clientPoint,int&) noexcept;
// Update only qualified reserved buffers in this selector's owned TextBoxes.
// No native pointers survive the UI callback. Failure affects names only.
bool SetLabels(const GemLabels& current,const GemLabels& previous) noexcept;
const char* LastReason() noexcept;
}
