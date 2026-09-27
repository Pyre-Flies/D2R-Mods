#pragma once
#include <D2RLPlugin/api.h>
namespace QolBulkStash {
using Available=bool(*)() noexcept;
void Initialize(const D2RL::PluginContext*,Available) noexcept;
bool Request() noexcept;
bool Busy() noexcept;
void Shutdown() noexcept;
}
