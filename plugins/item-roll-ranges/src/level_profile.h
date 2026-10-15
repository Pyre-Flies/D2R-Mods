#pragma once
#include <cstddef>
namespace LevelProfile {
// D2R 3.3.93787, observed in the qualified loaded game image. Read these
// fields directly: the native item-level getter repairs values below one.
inline constexpr std::size_t ItemLevelOffset=0x38,QualityLevelOffset=0x10d;
inline constexpr std::size_t ItemLevelRva=0x36cd36,QualityLevelRva=0x3719cc;
inline constexpr unsigned char ItemLevel[]={0x8b,0x40,0x38,0x83,0xf8,0x01,0x7d,0xf2,0xb8,0x01,0x00,0x00,0x00,0x89,0x41,0x38};
inline constexpr unsigned char QualityLevel[]={0x0f,0xb6,0x80,0x0d,0x01,0x00,0x00};
}
