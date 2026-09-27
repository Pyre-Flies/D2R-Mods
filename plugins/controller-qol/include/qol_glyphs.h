#pragma once
#include <D2RLPlugin/api.h>
namespace QolGlyphs {
void Initialize(const D2RL::PluginContext* context, bool compatible) noexcept;
void PublishHeader(const char* modifier, const char* a, const char* x, const char* y, const char* stick, const char* leftStick=nullptr) noexcept;
using BulkHeaderContext=const char*(*)() noexcept;
void SetBulkHeaderContext(BulkHeaderContext) noexcept;
void Shutdown() noexcept;
void Status() noexcept;
}
