#pragma once
#include "native_d2r.h"
#include <cstddef>

namespace QolBelt {
static_assert(offsetof(D2R::Native::BeltPlacementState, engaged) == 4);
static_assert(sizeof(D2R::Native::BeltPlacementState) == 64);
// Entry/branch/caller evidence in docs/BELT-NATIVE-CONTRACT.md.
inline bool SubmitStoredPlacement(D2R::Native::ShiftRightClickPlaceActionFn function,
                                  void* item, void* player, int32_t slot,uint8_t sourcePage=0) noexcept {
    if (!function || !item || !player || slot < 0 || slot >= 16 || (sourcePage!=0 && sourcePage!=3 && sourcePage!=4)) return false;
    D2R::Native::BeltPlacementState state{};
    state.targetSlot = slot;
    state.engaged = 1;
    function(item, player, sourcePage, 1, &state);
    return true; // Submitted only. The caller must observe authoritative state.
}
}
