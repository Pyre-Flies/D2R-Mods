#pragma once
#include <cstdint>
#include <cstring>
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
    constexpr uintptr_t FindAdvancedStashCounterRva     = 0x46D8F0;
    constexpr uintptr_t GetAdvancedStashCounterByteRva  = 0x46D9A0;
    constexpr uintptr_t HasUnitStateRva                 = 0x3351B0;
    constexpr uintptr_t TransferItemToInventoryPageRva  = 0x15F8B0;
    constexpr uintptr_t FinishInventoryInteractionRva   = 0x1A0780;
    constexpr uintptr_t ShiftRightClickPlaceActionRva   = 0x15F660;
    constexpr uintptr_t GetUnitInventoryRva             = 0x34A360;
    constexpr uintptr_t GetFreeBeltSlotRva              = 0x3862D0;
    constexpr uintptr_t FindTopLevelPanelByNameRva      = 0x846170;
    constexpr uintptr_t FindChildWidgetByNameRva        = 0x856220;

    // D2RCore.dll RVAs for the modded materials stack limit and full-counter status
    // (ord_109 at 0x3A0F20, get_materials_limit at 0x1BB960).
    constexpr uintptr_t CoreGetMaterialsLimitRva        = 0x1BB960;
    constexpr uintptr_t CoreGetCounterFullStatusRva     = 0x3A0F20;
    constexpr uint32_t  DefaultAdvancedStashStackLimit  = 99u;

    using GetLocalDataContextFn         = int32_t(__fastcall*)() noexcept;
    using GetLocalPlayerFn              = void*(__fastcall*)(int32_t) noexcept;
    using GetUnitInventoryFn            = void*(__fastcall*)(void* player) noexcept;
    using GetFreeBeltSlotFn             = int32_t(__fastcall*)(void* inventory, void* item, int32_t* freeSlot, bool allowAnyBeltable) noexcept;
    using CanDepositToAdvancedStashFn   = bool(__fastcall*)(void* item) noexcept;
    using GetAdvancedStashDestinationFn = void*(__fastcall*)(void* player) noexcept;
    using FindAdvancedStashCounterFn    = void*(__fastcall*)(void* owner, uint32_t classId) noexcept;
    using GetAdvancedStashCounterByteFn = uint8_t(__fastcall*)(const void* counterItem) noexcept;
    using HasUnitStateFn                = int32_t(__fastcall*)(const void* unit, uint32_t state) noexcept;
    using CoreGetMaterialsLimitFn       = uint32_t(__fastcall*)() noexcept;
    using CoreGetCounterFullStatusFn    = uint8_t(__fastcall*)(const void* counterItem) noexcept;
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

    inline uint32_t GetAdvancedStashStackLimit(uintptr_t coreBase) noexcept {
        if (coreBase) {
            __try {
                auto fnLimit = reinterpret_cast<CoreGetMaterialsLimitFn>(coreBase + CoreGetMaterialsLimitRva);
                const uint32_t limit = fnLimit ? fnLimit() : 0u;
                if (limit > 0) return limit;
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        return DefaultAdvancedStashStackLimit;
    }

    struct RejectedAdvancedStashCap {
        uint32_t classId{};
        uint32_t liveCount{};
        uint32_t minQuantity{};
        bool active{};
    };

    inline SRWLOCK g_rejectedAdvancedStashCapsLock = SRWLOCK_INIT;
    inline std::array<RejectedAdvancedStashCap, 256> g_rejectedAdvancedStashCaps{};

    inline void ClearAdvancedStashRejections() noexcept {
        AcquireSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
        g_rejectedAdvancedStashCaps = {};
        ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
    }

    inline void RecordAdvancedStashRejection(uint32_t classId, uint32_t liveCount, int32_t itemQuantity = 1) noexcept {
        const uint32_t incoming = itemQuantity > 0 ? static_cast<uint32_t>(itemQuantity) : 1u;
        if (liveCount < DefaultAdvancedStashStackLimit && (DefaultAdvancedStashStackLimit - liveCount) >= incoming) {
            return;
        }
        const uint32_t qty = liveCount >= DefaultAdvancedStashStackLimit ? 1u : (DefaultAdvancedStashStackLimit - liveCount + 1u);
        AcquireSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
        RejectedAdvancedStashCap* freeSlot = nullptr;
        for (auto& entry : g_rejectedAdvancedStashCaps) {
            if (entry.active && entry.classId == classId) {
                if (entry.liveCount != liveCount) {
                    entry.liveCount = liveCount;
                    entry.minQuantity = qty;
                } else if (qty < entry.minQuantity) {
                    entry.minQuantity = qty;
                }
                ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
                return;
            }
            if (!entry.active && !freeSlot) freeSlot = &entry;
        }
        if (!freeSlot) freeSlot = &g_rejectedAdvancedStashCaps[0];
        *freeSlot = RejectedAdvancedStashCap{classId, liveCount, qty, true};
        ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
    }

    inline bool IsAdvancedStashCountRejected(uint32_t classId, uint32_t liveCount, int32_t itemQuantity = 1) noexcept {
        const uint32_t qty = itemQuantity > 0 ? static_cast<uint32_t>(itemQuantity) : 1u;
        AcquireSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
        for (auto& entry : g_rejectedAdvancedStashCaps) {
            if (!entry.active || entry.classId != classId) continue;
            if (entry.liveCount != liveCount) {
                entry = {};
                ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
                return false;
            }
            const bool rejected = qty >= entry.minQuantity;
            ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
            return rejected;
        }
        ReleaseSRWLockExclusive(&g_rejectedAdvancedStashCapsLock);
        return false;
    }

    // Reads the current-season advanced-stash counter quantity for `classId` on `owner`.
    // When no counter unit exists yet, the stored quantity is 0 and returns true.
    inline bool ReadAdvancedStashCounterCount(
        uintptr_t exeBase,
        uintptr_t coreBase,
        void* owner,
        uint32_t classId,
        uint32_t* countOut,
        bool* fullOut = nullptr) noexcept {
        if (countOut) *countOut = 0;
        if (fullOut) *fullOut = false;
        if (!exeBase || !owner) return false;
        __try {
            auto fnFind = reinterpret_cast<FindAdvancedStashCounterFn>(exeBase + FindAdvancedStashCounterRva);
            void* counter = fnFind ? fnFind(owner, classId) : nullptr;
            if (!counter) return true;
            const void* itemData = *reinterpret_cast<const void* const*>(static_cast<const unsigned char*>(counter) + 0x10);
            if (!itemData) return false;
            const uint32_t count = *reinterpret_cast<const uint32_t*>(static_cast<const unsigned char*>(itemData) + 0x9C);
            if (countOut) *countOut = count;
            const uint32_t limit = GetAdvancedStashStackLimit(coreBase);
            bool full = count >= limit;
            if (!full && coreBase) {
                auto fnFull = reinterpret_cast<CoreGetCounterFullStatusFn>(coreBase + CoreGetCounterFullStatusRva);
                if (fnFull && fnFull(counter) >= DefaultAdvancedStashStackLimit) full = true;
            }
            if (fullOut) *fullOut = full;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool HasAdvancedStashCapacity(
        uintptr_t exeBase,
        uintptr_t coreBase,
        void* owner,
        uint32_t classId,
        uint32_t pendingCount = 0,
        uint32_t* liveCountOut = nullptr,
        int32_t itemQuantity = 1) noexcept {
        uint32_t liveCount = 0;
        bool nativeFull = false;
        if (!ReadAdvancedStashCounterCount(exeBase, coreBase, owner, classId, &liveCount, &nativeFull)) return false;
        if (liveCountOut) *liveCountOut = liveCount;
        const uint32_t limit = GetAdvancedStashStackLimit(coreBase);
        if (nativeFull || (limit > DefaultAdvancedStashStackLimit && IsAdvancedStashCountRejected(classId, liveCount, itemQuantity))) return false;
        const uint32_t incoming = itemQuantity > 0 ? static_cast<uint32_t>(itemQuantity) : 1u;
        return liveCount < limit && (limit - liveCount) > pendingCount && (limit - liveCount - pendingCount) >= incoming;
    }

    inline constexpr unsigned char FindAdvancedStashCounterWitness[] = {
        0x48, 0x89, 0x5c, 0x24, 0x08, 0x57, 0x48, 0x83, 0xec, 0x20, 0x8b, 0xfa, 0x48, 0x8b, 0xd9, 0xba,
        0xd1, 0x00, 0x00, 0x00, 0xe8, 0xa7, 0x78, 0xec, 0xff, 0x85, 0xc0, 0x75, 0x13, 0x48, 0x8d, 0x4c,
    };
    inline constexpr unsigned char GetAdvancedStashCounterByteWitness[] = {
        0x48, 0x85, 0xc9, 0x75, 0x03, 0x32, 0xc0, 0xc3, 0x48, 0x8b, 0x41, 0x10, 0x0f, 0xb6, 0x80, 0x9c,
        0x00, 0x00, 0x00, 0xc3,
    };
    inline constexpr unsigned char CoreMaterialsLimitWitness[] = {
        0x8b, 0x05, 0xba, 0x20, 0x54, 0x00, 0xc3,
    };
    inline constexpr unsigned char CoreCounterFullWitness[] = {
        0x55, 0x56, 0x48, 0x83, 0xec, 0x28, 0x48, 0x8d, 0x6c, 0x24, 0x20, 0x48, 0xc7, 0x45, 0x00, 0xfe,
        0xff, 0xff, 0xff, 0x40, 0xb6, 0x63, 0x48, 0x85, 0xc9, 0x74, 0x22, 0xff, 0x15, 0xcf, 0xf6, 0x35,
        0x00, 0x90, 0x48, 0x85, 0xc0, 0x74, 0x16, 0x8b, 0x0d, 0xd3, 0xca, 0x35, 0x00, 0x31, 0xd2, 0x39,
        0x88, 0x9c, 0x00, 0x00, 0x00, 0xbe, 0x63, 0x00, 0x00, 0x00, 0x0f, 0x42, 0xf2, 0x89, 0xf0, 0x48,
        0x83, 0xc4, 0x28, 0x5e, 0x5d, 0xc3,
    };

    inline bool MatchesNativeBytes(uintptr_t address, const unsigned char* expected, size_t size) noexcept {
        if (!address || !expected || !size) return false;
        __try {
            return std::memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool HasVerifiedAdvancedStashCounterContract(uintptr_t exeBase) noexcept {
        return exeBase &&
            MatchesNativeBytes(exeBase + FindAdvancedStashCounterRva,
                FindAdvancedStashCounterWitness, sizeof(FindAdvancedStashCounterWitness)) &&
            MatchesNativeBytes(exeBase + GetAdvancedStashCounterByteRva,
                GetAdvancedStashCounterByteWitness, sizeof(GetAdvancedStashCounterByteWitness));
    }

    inline uintptr_t VerifiedAdvancedStashCoreBase(uintptr_t coreBase) noexcept {
        if (!coreBase) return 0;
        return (MatchesNativeBytes(coreBase + CoreGetMaterialsLimitRva,
                    CoreMaterialsLimitWitness, sizeof(CoreMaterialsLimitWitness)) &&
                MatchesNativeBytes(coreBase + CoreGetCounterFullStatusRva,
                    CoreCounterFullWitness, sizeof(CoreCounterFullWitness)))
            ? coreBase
            : 0;
    }

    inline bool DepositToAdvancedStash(uintptr_t exeBase, void* item, void* player,uint8_t sourcePage=0,int32_t itemQuantity=1) noexcept {
        if (!exeBase || !item || !player || (sourcePage!=0 && sourcePage!=3)) return false;
        __try {
            if (!HasVerifiedAdvancedStashCounterContract(exeBase)) return false;
            auto fnGetDest  = reinterpret_cast<GetAdvancedStashDestinationFn>(exeBase + GetAdvancedStashDestinationRva);
            auto fnTransfer = reinterpret_cast<TransferItemToInventoryPageFn>(exeBase + TransferItemToInventoryPageRva);
            auto fnFinish   = reinterpret_cast<FinishInventoryInteractionFn>(exeBase + FinishInventoryInteractionRva);
            if (fnGetDest && fnTransfer) {
                void* dest = fnGetDest(player);
                if (dest) {
                    const auto coreBase = VerifiedAdvancedStashCoreBase(
                        reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll")));
                    const uint32_t classId = static_cast<const uint32_t*>(item)[1];
                    if (!HasAdvancedStashCapacity(exeBase, coreBase, dest, classId, 0, nullptr, itemQuantity)) return false;
                    std::array<uint8_t, 16> placement{};
                    bool transferred = fnTransfer(item, dest, 4, sourcePage, true, placement.data());
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
