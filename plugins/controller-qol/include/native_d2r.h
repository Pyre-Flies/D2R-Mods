#pragma once
#include <cstdint>
#include <windows.h>
#include <array>

namespace D2R {

namespace Native {
    // D2R.exe-relative RVAs inherited from the research corpus. Version strings
    // alone do not admit these calls. Belt admission/evidence: docs/BELT-NATIVE-CONTRACT.md.
    constexpr uintptr_t GetLocalDataContextRva          = 0x08B2D0;
    constexpr uintptr_t GetLocalPlayerRva               = 0x09A480;
    constexpr uintptr_t CanDepositToAdvancedStashRva     = 0x15A0B0;
    constexpr uintptr_t GetAdvancedStashDestinationRva  = 0x46DA50;
    constexpr uintptr_t TransferItemToInventoryPageRva  = 0x15F8B0;
    constexpr uintptr_t FinishInventoryInteractionRva   = 0x1A0780;
    constexpr uintptr_t ShiftRightClickPlaceActionRva   = 0x15F660;
    constexpr uintptr_t GetUnitInventoryRva             = 0x34A360;
    constexpr uintptr_t GetFreeBeltSlotRva              = 0x3862D0;
    constexpr uintptr_t FindTopLevelPanelByNameRva      = 0x846170;
    constexpr uintptr_t FindChildWidgetByNameRva        = 0x856220;

    using GetLocalDataContextFn         = int32_t(__fastcall*)() noexcept;
    using GetLocalPlayerFn              = void*(__fastcall*)(int32_t) noexcept;
    using GetUnitInventoryFn            = void*(__fastcall*)(void* player) noexcept;
    using GetFreeBeltSlotFn             = int32_t(__fastcall*)(void* inventory, void* item, int32_t* freeSlot, bool allowAnyBeltable) noexcept;
    using CanDepositToAdvancedStashFn   = bool(__fastcall*)(void* item) noexcept;
    using GetAdvancedStashDestinationFn = void*(__fastcall*)(void* player) noexcept;
    using TransferItemToInventoryPageFn = bool(__fastcall*)(void* item, void* destinationUnit, uint8_t destPage, uint8_t srcPage, bool flag, void* placementOut) noexcept;
    using FinishInventoryInteractionFn  = void(__fastcall*)(int32_t a1, void* a2, int32_t a3, int32_t a4, bool a5) noexcept;
    using ShiftRightClickPlaceActionFn  = void(__fastcall*)(void* item, void* player, uint8_t page, uint8_t flag, void* state) noexcept;
    using FindTopLevelPanelByNameFn     = void*(__fastcall*)(const char* name) noexcept;
    using FindChildWidgetByNameFn       = void*(__fastcall*)(void* panel, const char* name) noexcept;

    inline void* GetLocalPlayerUnit(uintptr_t exeBase) noexcept {
        if (!exeBase) return nullptr;
        __try {
            auto fnCtx = reinterpret_cast<GetLocalDataContextFn>(exeBase + GetLocalDataContextRva);
            auto fnPlayer = reinterpret_cast<GetLocalPlayerFn>(exeBase + GetLocalPlayerRva);
            if (fnCtx && fnPlayer) {
                return fnPlayer(fnCtx());
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return nullptr;
    }

    struct BeltPlacementState {
        int32_t targetSlot{-1};
        uint8_t engaged{1};
        uint8_t pad[59]{};
    };

    inline bool CanDepositToAdvancedStash(uintptr_t exeBase, void* item) noexcept {
        if (!exeBase || !item) return false;
        __try {
            auto fnCan = reinterpret_cast<CanDepositToAdvancedStashFn>(exeBase + CanDepositToAdvancedStashRva);
            return fnCan ? fnCan(item) : false;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    inline bool DepositToAdvancedStash(uintptr_t exeBase, void* item, void* player) noexcept {
        if (!exeBase || !item || !player) return false;
        __try {
            auto fnGetDest  = reinterpret_cast<GetAdvancedStashDestinationFn>(exeBase + GetAdvancedStashDestinationRva);
            auto fnTransfer = reinterpret_cast<TransferItemToInventoryPageFn>(exeBase + TransferItemToInventoryPageRva);
            auto fnFinish   = reinterpret_cast<FinishInventoryInteractionFn>(exeBase + FinishInventoryInteractionRva);
            if (fnGetDest && fnTransfer) {
                void* dest = fnGetDest(player);
                if (dest) {
                    std::array<uint8_t, 16> placement{};
                    bool transferred = fnTransfer(item, dest, 4, 0, true, placement.data());
                    if (fnFinish) fnFinish(3, nullptr, 0, 0, false);
                    return transferred;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    constexpr uintptr_t BankPanelGetSelectedTabRva      = 0x23AF50;

    // Bank+0x278 contains UI widgets, not storage units. The native resolver
    // selects the owner from current/previous-season pages using +0x168/+0x170.
    inline void* GetStashContainerUnit(uintptr_t exeBase, uint32_t tabIndex) noexcept {
        if (!exeBase || tabIndex!=1) return nullptr;
        __try {
            auto find=reinterpret_cast<FindTopLevelPanelByNameFn>(exeBase+FindTopLevelPanelByNameRva);
            void* bank=find("BankExpansionLayout");
            if (!bank) return nullptr;
            void* owner=reinterpret_cast<void*(__fastcall*)(void*)>(exeBase+0x23AD80)(bank);
            if (!owner) return nullptr;
            // Native Bank refresh converts the owner record to a client unit.
            const auto id=reinterpret_cast<uint32_t(__fastcall*)(void*)>(exeBase+0x2EF880)(owner);
            return reinterpret_cast<void*(__fastcall*)(uint32_t,uint32_t)>(exeBase+0x09A5D0)(id,0);
        } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    inline uint32_t GetActiveStashTabIndex(uintptr_t exeBase) noexcept {
        if (!exeBase) return 0xFFFFFFFF;
        __try {
            auto fnFindPanel = reinterpret_cast<FindTopLevelPanelByNameFn>(exeBase + FindTopLevelPanelByNameRva);
            if (fnFindPanel) {
                void* pBank = fnFindPanel("BankExpansionLayout");
                if (pBank) {
                    auto fnGetTab = reinterpret_cast<uint8_t(__fastcall*)(void*) noexcept>(exeBase + BankPanelGetSelectedTabRva);
                    if (fnGetTab) {
                        return fnGetTab(pBank);
                    }
                    auto fnFindChild = reinterpret_cast<FindChildWidgetByNameFn>(exeBase + FindChildWidgetByNameRva);
                    if (fnFindChild) {
                        void* pTabs = fnFindChild(pBank, "BankTabs");
                        if (pTabs) {
                            return *reinterpret_cast<const uint32_t*>(static_cast<const char*>(pTabs) + 0x16C4);
                        }
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0xFFFFFFFF;
    }

    // Bank refresh 0x23E8A7 sets grid+0x630 to native page 4.
    // Category index 1 is NOT the native storage page.
    inline bool SubmitSharedTransfer(TransferItemToInventoryPageFn transfer, void* item, void* owner) noexcept {
        if (!transfer || !item || !owner) return false;
        std::array<uint8_t, 16> placement{};
        return transfer(item, owner, 4, 0, true, placement.data());
    }

    inline bool DepositToActiveStashPage(uintptr_t exeBase, void* item, uint32_t tabIndex) noexcept {
        if (!exeBase || !item) return false;
        __try {
            void* destContainer = GetStashContainerUnit(exeBase, tabIndex);
            if (!destContainer) return false;

            auto fnTransfer = reinterpret_cast<TransferItemToInventoryPageFn>(exeBase + TransferItemToInventoryPageRva);
            auto fnFinish   = reinterpret_cast<FinishInventoryInteractionFn>(exeBase + FinishInventoryInteractionRva);
            if (fnTransfer) {
                bool transferred = SubmitSharedTransfer(fnTransfer,item,destContainer);
                if (fnFinish) fnFinish(3, nullptr, 0, 0, false);
                return transferred;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    // Inverse of Shared deposit: native stash source 4 -> inventory 0.
    inline bool SubmitSharedWithdrawal(TransferItemToInventoryPageFn transfer, void* item, void* player) noexcept {
        if (!transfer || !item || !player) return false;
        std::array<uint8_t, 16> placement{};
        return transfer(item, player, 0, 4, true, placement.data());
    }

    inline bool TransferFromStashToInventory(uintptr_t exeBase, void* item, void* player) noexcept {
        if (!exeBase || !item || !player) return false;
        __try {
            auto fnTransfer = reinterpret_cast<TransferItemToInventoryPageFn>(exeBase + TransferItemToInventoryPageRva);
            auto fnFinish   = reinterpret_cast<FinishInventoryInteractionFn>(exeBase + FinishInventoryInteractionRva);
            if (fnTransfer) {
                bool transferred = SubmitSharedWithdrawal(fnTransfer,item,player);
                if (fnFinish) fnFinish(3, nullptr, 0, 0, false);
                return transferred;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }
}

} // namespace D2R
