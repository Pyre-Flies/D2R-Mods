#pragma once
#include <cstdint>
namespace QolCompat {
bool IsPluginCaller(uintptr_t address) noexcept;
// Native original or the reviewed RuffnecKk 1.3.3 cooperative detour only.
bool ValidateBeltEntry(uintptr_t address) noexcept;
}
