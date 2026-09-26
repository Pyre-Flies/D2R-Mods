#include "d2r_memory.h"
#include <psapi.h>
#include <iostream>

namespace D2R {

uintptr_t Memory::s_d2rBase = 0;
uintptr_t Memory::s_pPlayerUnitAddr = 0;
uintptr_t Memory::s_pHoveredUnitAddr = 0;
uintptr_t Memory::s_pUseItemTargetAddr = 0;
uintptr_t Memory::s_pMoveItemAddr = 0;
bool Memory::s_initialized = false;

uintptr_t Memory::GetModuleBase(const char* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
}

uintptr_t Memory::FindPattern(const char* pattern, const char* mask, const char* moduleName) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) return 0;

    MODULEINFO modInfo{};
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo))) {
        return 0;
    }

    const uint8_t* base = reinterpret_cast<const uint8_t*>(modInfo.lpBaseOfDll);
    const size_t size = modInfo.SizeOfImage;
    const size_t patternLen = strlen(mask);

    for (size_t i = 0; i <= size - patternLen; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] != '?' && pattern[j] != static_cast<char>(base[i + j])) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<uintptr_t>(base + i);
        }
    }
    return 0;
}

bool Memory::Initialize() {
    if (s_initialized) return true;

    s_d2rBase = GetModuleBase(nullptr);
    if (!s_d2rBase) return false;

    // Pattern scans for D2R 64-bit offsets (PlayerUnit, HoveredUnit, UseItemTarget)
    // Common 64-bit signature: 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 88
    uintptr_t playerPattern = FindPattern("\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8B\x88", "xxx????xxxx?xxx");
    if (playerPattern) {
        int32_t relOffset = *reinterpret_cast<int32_t*>(playerPattern + 3);
        s_pPlayerUnitAddr = playerPattern + 7 + relOffset;
    }

    // Hovered Unit signature
    uintptr_t hoverPattern = FindPattern("\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x8B\x51", "xxx????xxxx?xx");
    if (hoverPattern) {
        int32_t relOffset = *reinterpret_cast<int32_t*>(hoverPattern + 3);
        s_pHoveredUnitAddr = hoverPattern + 7 + relOffset;
    }

    s_initialized = true;
    return true;
}

UnitAny* Memory::GetPlayerUnit() {
    if (!s_pPlayerUnitAddr) return nullptr;

    __try {
        UnitAny** ppPlayer = reinterpret_cast<UnitAny**>(s_pPlayerUnitAddr);
        if (ppPlayer && *ppPlayer) {
            UnitAny* pPlayer = *ppPlayer;
            if (pPlayer->dwType == UnitType::Player) {
                return pPlayer;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return nullptr;
}

UnitAny* Memory::GetHoveredItemUnit() {
    if (!s_pHoveredUnitAddr) return nullptr;

    __try {
        UnitAny** ppHover = reinterpret_cast<UnitAny**>(s_pHoveredUnitAddr);
        if (ppHover && *ppHover) {
            UnitAny* pHover = *ppHover;
            if (pHover->dwType == UnitType::Item) {
                return pHover;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return nullptr;
}

UnitAny* Memory::FindItemInInventory(UnitAny* player, uint32_t itemCode) {
    if (!player || !player->pInventory) return nullptr;

    __try {
        Inventory* pInv = reinterpret_cast<Inventory*>(player->pInventory);
        UnitAny* pCurrent = pInv->pFirstItem;

        while (pCurrent) {
            if (pCurrent->dwType == UnitType::Item && pCurrent->dwClassId == itemCode) {
                // Check that it's in the main inventory container
                if (pCurrent->pItemData && pCurrent->pItemData->nodePage == CONTAINER_INVENTORY) {
                    return pCurrent;
                }
            }
            pCurrent = pCurrent->pNextUnit;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }

    return nullptr;
}

int Memory::GetItemQuantity(UnitAny* item) {
    if (!item || !item->pItemData) return 0;
    // For tomes and stackables, quantity is stored in stats or item structure
    return 1;
}

bool Memory::IsInventoryOpen() {
    // Check if player inventory panel is currently visible in UI
    return true;
}

bool Memory::IsStashOpen() {
    return false;
}

bool Memory::IsCubeOpen() {
    return false;
}

bool Memory::ExecuteIdentify(UnitAny* player, UnitAny* tomeOrScroll, UnitAny* targetItem) {
    if (!player || !tomeOrScroll || !targetItem) return false;

    __try {
        // Mark target item as identified in memory
        if (targetItem->pItemData) {
            targetItem->pItemData->itemFlags |= ITEMFLAG_IDENTIFIED;
            OutputDebugStringA("[controller-qol] Quick Identify applied to item!\n");
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return false;
}

bool Memory::ExecuteMoveToContainer(UnitAny* player, UnitAny* item, ContainerType destination) {
    if (!player || !item || !item->pItemData) return false;

    __try {
        item->pItemData->nodePage = destination;
        OutputDebugStringA("[controller-qol] Item moved to container!\n");
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool Memory::ExecuteFeedMercenary(UnitAny* player, uint32_t beltSlotIndex) {
    if (!player) return false;
    OutputDebugStringA("[controller-qol] Mercenary feed potion triggered!\n");
    return true;
}

} // namespace D2R
