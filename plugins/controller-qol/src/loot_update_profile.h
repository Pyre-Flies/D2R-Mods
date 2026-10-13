#pragma once
#include <cstdint>
// Exact capture provenance and client restore/update/save ordering are recorded
// in docs/LOOT-APPROACH-FILTER-2026-10-13.md. This is a client update, not the
// authoritative server scheduler (which does not exist on remote clients).
namespace QolLootUpdate {
inline constexpr uintptr_t RestoreSite=0x94c87;
inline constexpr unsigned char Restore[]={0xe8,0x74,0x76,0x00,0x00};
inline constexpr uintptr_t WindowSite=0x95033,CallSite=0x95041;
inline constexpr unsigned char Window[]={
    0x8b,0x0d,0xcb,0xe6,0x98,0x02,0xe8,0x42,0x54,0x00,0x00,0x48,0x8b,0xc8,
    0xe8,0x7a,0x4b,0x06,0x00,0xe8,0xf5,0x76,0x00,0x00
};
}
