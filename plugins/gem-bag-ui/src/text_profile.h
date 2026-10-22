#pragma once
#include <cstddef>
#include <cstdint>
namespace GemBag::TextProfile {
// Exact admitted game, live native Countdown TextBox references in PID 54084.
// Data-only contract: no new native call, hook, allocator or resize operation.
inline constexpr uintptr_t VtableRva=0x1cf4610,UpdateRva=0x86e630;
inline constexpr size_t Data=0x88,Size=0x90,Capacity=0x98,ObjectBytes=0x230;
}
