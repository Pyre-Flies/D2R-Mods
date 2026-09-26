#pragma once
#include "d2r_defs.h"

namespace D2R {

class TransferHandler {
public:
    // Quick transfer to stash or cube (LT + X)
    static bool HandleTransferAction();

    // Quick drop item (LT + Y)
    static bool HandleDropAction();

    // Feed mercenary potion from belt slot (LT + D-Pad)
    static bool HandleMercenaryPotion(uint32_t slotIndex);

    // Auto-restock belt potions from inventory (R3 / Right Thumb click)
    static bool HandleBeltRestock();
};

} // namespace D2R
