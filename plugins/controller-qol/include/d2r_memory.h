#pragma once
#include "d2r_defs.h"
#include <string>
#include <vector>

namespace D2R {

class Memory {
public:
    static bool Initialize();
    static uintptr_t FindPattern(const char* pattern, const char* mask, const char* moduleName = nullptr);
    static uintptr_t GetModuleBase(const char* moduleName = nullptr);

    // High-level accessors
    static UnitAny* GetPlayerUnit();
    static UnitAny* GetHoveredItemUnit();
    static UnitAny* FindItemInInventory(UnitAny* player, uint32_t itemCode);
    static int GetItemQuantity(UnitAny* item);
    
    // Panel checks
    static bool IsInventoryOpen();
    static bool IsStashOpen();
    static bool IsCubeOpen();

    // Action dispatch
    static bool ExecuteIdentify(UnitAny* player, UnitAny* tomeOrScroll, UnitAny* targetItem);
    static bool ExecuteMoveToContainer(UnitAny* player, UnitAny* item, ContainerType destination);
    static bool ExecuteFeedMercenary(UnitAny* player, uint32_t beltSlotIndex);

private:
    static uintptr_t s_d2rBase;
    static uintptr_t s_pPlayerUnitAddr;
    static uintptr_t s_pHoveredUnitAddr;
    static uintptr_t s_pUseItemTargetAddr;
    static uintptr_t s_pMoveItemAddr;
    static bool s_initialized;
};

} // namespace D2R
