#include "transfer_handler.h"
#include "d2r_memory.h"
#include <iostream>

namespace D2R {

bool TransferHandler::HandleTransferAction() {
    UnitAny* pHovered = Memory::GetHoveredItemUnit();
    if (!pHovered || !pHovered->pItemData) return false;

    UnitAny* pPlayer = Memory::GetPlayerUnit();
    if (!pPlayer) return false;

    uint8_t currentLoc = pHovered->pItemData->nodePage;

    if (Memory::IsStashOpen()) {
        ContainerType dest = (currentLoc == CONTAINER_INVENTORY) ? CONTAINER_STASH : CONTAINER_INVENTORY;
        return Memory::ExecuteMoveToContainer(pPlayer, pHovered, dest);
    } else if (Memory::IsCubeOpen()) {
        ContainerType dest = (currentLoc == CONTAINER_INVENTORY) ? CONTAINER_CUBE : CONTAINER_INVENTORY;
        return Memory::ExecuteMoveToContainer(pPlayer, pHovered, dest);
    }

    return false;
}

bool TransferHandler::HandleDropAction() {
    UnitAny* pHovered = Memory::GetHoveredItemUnit();
    if (!pHovered) return false;

    UnitAny* pPlayer = Memory::GetPlayerUnit();
    if (!pPlayer) return false;

    OutputDebugStringA("[controller-qol] Quick Drop triggered for hovered item.\n");
    return true;
}

bool TransferHandler::HandleMercenaryPotion(uint32_t slotIndex) {
    UnitAny* pPlayer = Memory::GetPlayerUnit();
    if (!pPlayer) return false;

    return Memory::ExecuteFeedMercenary(pPlayer, slotIndex);
}

bool TransferHandler::HandleBeltRestock() {
    UnitAny* pPlayer = Memory::GetPlayerUnit();
    if (!pPlayer) return false;

    OutputDebugStringA("[controller-qol] Belt Auto-Restock triggered!\n");
    return true;
}

} // namespace D2R
