#pragma once
#include <cstdint>
namespace QolLootRun {
// Exact native approach -> movement dispatcher CALL; documented in
// LOOT-APPROACH-FILTER-2026-10-13.md. No global walk/run state is changed.
inline constexpr uintptr_t CallSite=0xfbf30;
inline constexpr unsigned char Expected[]={0xe8,0xab,0xec,0xff,0xff};
inline uint32_t Action(uint32_t nativeAction,uint32_t type,bool owned) noexcept {
    return owned && type==4 && (nativeAction==2 || nativeAction==4)?4:nativeAction;
}
}
