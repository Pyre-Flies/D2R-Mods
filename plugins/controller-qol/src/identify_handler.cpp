#include "identify_handler.h"
#include "d2r_memory.h"
#include <iostream>

namespace D2R {

bool IdentifyHandler::CanIdentify(UnitAny* item) {
    if (!item || item->dwType != UnitType::Item) return false;
    if (!item->pItemData) return false;

    // Check if already identified
    if (item->pItemData->itemFlags & ITEMFLAG_IDENTIFIED) {
        return false;
    }

    // Must be an item in inventory, stash, or cube
    uint8_t loc = item->pItemData->nodePage;
    if (loc != CONTAINER_INVENTORY && loc != CONTAINER_STASH && loc != CONTAINER_CUBE) {
        return false;
    }

    return true;
}

bool IdentifyHandler::HandleIdentifyAction() {
    UnitAny* pHovered = Memory::GetHoveredItemUnit();
    if (!pHovered || !CanIdentify(pHovered)) {
        return false;
    }

    UnitAny* pPlayer = Memory::GetPlayerUnit();
    if (!pPlayer) {
        return false;
    }

    // 1. Search for Tome of Identify ('ibk ')
    UnitAny* pTome = Memory::FindItemInInventory(pPlayer, ITEM_CODE_IBK);
    if (pTome) {
        if (Memory::ExecuteIdentify(pPlayer, pTome, pHovered)) {
            OutputDebugStringA("[controller-qol] Identified item using Tome of Identify!\n");
            return true;
        }
    }

    // 2. Fallback: Search for Scroll of Identify ('isc ')
    UnitAny* pScroll = Memory::FindItemInInventory(pPlayer, ITEM_CODE_ISC);
    if (pScroll) {
        if (Memory::ExecuteIdentify(pPlayer, pScroll, pHovered)) {
            OutputDebugStringA("[controller-qol] Identified item using Scroll of Identify!\n");
            return true;
        }
    }

    OutputDebugStringA("[controller-qol] No Tome or Scroll of Identify available.\n");
    return false;
}

} // namespace D2R
