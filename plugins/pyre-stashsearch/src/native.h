#pragma once

#include "stash_search.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace StashSearch {

// Native D2R.exe RVAs verified by StashSearch.dll 1.10.12 (0x10500 / 0xC5A0)
namespace Rva {
inline constexpr uintptr_t GetLocalPlayerSlot           = 0x008B2D0;
inline constexpr uintptr_t GetLocalPlayerUnit           = 0x009A480;
inline constexpr uintptr_t FindClientUnitByIdAndType    = 0x009A5D0;
inline constexpr uintptr_t BankPanelDraw                = 0x023AA60;
inline constexpr uintptr_t BankPanelGetSelectedRecord   = 0x023AD80;
inline constexpr uintptr_t BankPanelGetSelectedTab      = 0x023AF50;
inline constexpr uintptr_t BankPanelGetSharedPageCount  = 0x023B060;
inline constexpr uintptr_t BankPanelIsShowingPrevSeason = 0x023B910;
inline constexpr uintptr_t BankPanelUpdateActiveView    = 0x023E630;
inline constexpr uintptr_t InventoryGridGetItems        = 0x02C49F0;
inline constexpr uintptr_t BuildItemStatsString         = 0x02DA0E0;
inline constexpr uintptr_t BuildSocketableStatsString   = 0x02DA310;
inline constexpr uintptr_t GetUnitStat                  = 0x02F5020;
inline constexpr uintptr_t GetDataTables                = 0x0300A90;
inline constexpr uintptr_t GetUnitInventory             = 0x034A360;
inline constexpr uintptr_t ItemGetLocationPage          = 0x036CFE0;
inline constexpr uintptr_t InventoryGetFirstItem        = 0x0388C10;
inline constexpr uintptr_t ItemGetNextInventoryItem     = 0x038ABA0;
inline constexpr uintptr_t GetLocalizedStringByKey      = 0x05F4B90;
inline constexpr uintptr_t DrawFilledRect               = 0x0657B90;
inline constexpr uintptr_t WidgetFindChild              = 0x0855C90;
inline constexpr uintptr_t WidgetGetSize                = 0x08562A0;
inline constexpr uintptr_t WidgetGetScreenPos           = 0x08562F0;
inline constexpr uintptr_t InitTextStyle                = 0x086D1B0;
inline constexpr uintptr_t TextStyleCallSite            = 0x086D489;
inline constexpr uintptr_t TabBarSelectTab              = 0x0878FB0;
inline constexpr uintptr_t DrawTextCallSite             = 0x0888B8D;
inline constexpr uintptr_t DrawStyledText               = 0x0902E20;
inline constexpr uintptr_t MeasureStyledText            = 0x0903CD0;
inline constexpr uintptr_t BuildItemFullDescription     = 0x3E2B634;
inline constexpr uintptr_t GetLocalizedStringById       = 0x3E2B730;
} // namespace Rva

// D2RCore.dll offsets verified by 0x10220 / 0xC5A0
namespace CoreRva {
inline constexpr uintptr_t PtrGetLocalPlayerSlot           = 0x7004A8;
inline constexpr uintptr_t PtrGetLocalPlayerUnit           = 0x7004D8;
inline constexpr uintptr_t PtrBankPanelGetSelectedTab      = 0x700DC0;
inline constexpr uintptr_t PtrBankPanelIsShowingPrevSeason = 0x700DD8;
inline constexpr uintptr_t PtrBankPanelGetSharedPageCount  = 0x700DF0;
inline constexpr uintptr_t PtrBankPanelGetSelectedRecord   = 0x700E08;
inline constexpr uintptr_t PtrBankPanelUpdateActiveView    = 0x700E20;
inline constexpr uintptr_t ActiveSharedStashUnitId         = 0x700E38;
} // namespace CoreRva

// Expected byte guards from StashSearch.dll .rdata (0x14520..0x1472F)
inline constexpr uint8_t kBytes_BankPanelDraw_23AA60[17] = {
	0x40, 0x53, 0x55, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x58, 0x4C, 0x89, 0xB4, 0x24, 0x98, 0x00, 0x00, 0x00
};
inline constexpr uint8_t kBytes_GetLocalPlayerSlot_08B2D0[16] = {
	0x8B, 0x05, 0x2E, 0x84, 0x99, 0x02, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
};
inline constexpr uint8_t kBytes_GetLocalPlayerUnit_09A480[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x83, 0xF9, 0x08, 0x0F, 0x83, 0x85
};
inline constexpr uint8_t kBytes_FindClientUnitByIdAndType_09A5D0[16] = {
	0x4C, 0x63, 0xCA, 0x48, 0x8D, 0x05, 0x36, 0x93, 0x98, 0x02, 0x8B, 0xD1, 0x44, 0x8B, 0xC1, 0x49
};
inline constexpr uint8_t kBytes_GetUnitInventory_34A360[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x18, 0x56, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xF1, 0x48, 0x85, 0xC9
};
inline constexpr uint8_t kBytes_InventoryGetFirstItem_388C10[16] = {
	0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xD9, 0x48, 0x85, 0xC9, 0x74, 0x2E, 0x81, 0x39
};
inline constexpr uint8_t kBytes_ItemGetNextInventoryItem_38ABA0[16] = {
	0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xD9, 0x48, 0x85, 0xC9, 0x75, 0x10, 0x88, 0x4C
};
inline constexpr uint8_t kBytes_BuildItemFullDescription_3E2B634[16] = {
	0xFF, 0x25, 0x86, 0xF6, 0xFF, 0xFF, 0xFF, 0x25, 0x88, 0xF6, 0xFF, 0xFF, 0xFF, 0x25, 0x8A, 0xF6
};
inline constexpr uint8_t kBytes_GetDataTables_300A90[16] = {
	0x48, 0x83, 0xEC, 0x28, 0x0F, 0xB6, 0xC1, 0x48, 0x89, 0x44, 0x24, 0x38, 0x48, 0x83, 0xF8, 0x04
};
inline constexpr uint8_t kBytes_GetLocalizedStringById_3E2B730[6] = {
	0xFF, 0x25, 0xDA, 0xF6, 0xFF, 0xFF
};
inline constexpr uint8_t kBytes_InventoryGridGetItems_2C49F0[32] = {
	0x48, 0x89, 0x5C, 0x24, 0x20, 0x56, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x4C, 0x8B,
	0xFA, 0x48, 0x8B, 0xF1, 0x33, 0xDB, 0xE8, 0x05, 0x2E, 0xFE, 0xFF, 0x4C, 0x8B, 0xF0, 0x48, 0x85
};
inline constexpr uint8_t kBytes_WidgetFindChild_855C90[16] = {
	0x40, 0x53, 0x55, 0x56, 0x57, 0x41, 0x54, 0x48, 0x83, 0xEC, 0x20, 0x4C, 0x89, 0x7C, 0x24, 0x68
};
inline constexpr uint8_t kBytes_BankPanelGetSelectedTab_23AF50[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8D, 0x15, 0x3F, 0x9C, 0xAA
};
inline constexpr uint8_t kBytes_TabBarSelectTab_878FB0[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x18, 0x89, 0x54, 0x24, 0x10, 0x55, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x30
};
inline constexpr uint8_t kBytes_WidgetGetScreenPos_8562F0[16] = {
	0x48, 0x8B, 0xC4, 0x48, 0x89, 0x70, 0x20, 0x57, 0x48, 0x81, 0xEC, 0xA0, 0x00, 0x00, 0x00, 0x0F
};
inline constexpr uint8_t kBytes_WidgetGetSize_8562A0[16] = {
	0x40, 0x53, 0x48, 0x83, 0xEC, 0x30, 0x80, 0x79, 0x52, 0x00, 0x48, 0x8B, 0xDA, 0x74, 0x2A, 0x48
};
inline constexpr uint8_t kBytes_InitTextStyle_86D1B0[32] = {
	0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0xC7, 0x41, 0x30, 0x00, 0x00, 0x80, 0x3F, 0x33, 0xC0, 0x89,
	0x41, 0x2C, 0x0F, 0x57, 0xC0, 0x89, 0x41, 0x3C, 0x48, 0x8B, 0xD9, 0x88, 0x41, 0x46, 0x89, 0x41
};
inline constexpr uint8_t kBytes_DrawStyledText_902E20[32] = {
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x70, 0x0F,
	0x29, 0x74, 0x24, 0x60, 0x49, 0x8B, 0xF0, 0x0F, 0x28, 0xF3, 0x48, 0x8B, 0xFA, 0x48, 0x8B, 0xD9
};
inline constexpr uint8_t kBytes_MeasureStyledText_903CD0[32] = {
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x0F,
	0x29, 0x74, 0x24, 0x40, 0x49, 0x8B, 0xF9, 0x0F, 0x28, 0xF2, 0x48, 0x8B, 0xF2, 0x48, 0x8B, 0xD9
};
inline constexpr uint8_t kBytes_DrawFilledRect_657B90[32] = {
	0x44, 0x3B, 0xC1, 0x0F, 0x8C, 0xD3, 0x02, 0x00, 0x00, 0x4C, 0x8B, 0xDC, 0x55, 0x41, 0x54, 0x41,
	0x55, 0x41, 0x57, 0x49, 0x8D, 0x6B, 0xA8, 0x48, 0x81, 0xEC, 0x38, 0x01, 0x00, 0x00, 0x48, 0x8B
};
inline constexpr uint8_t kBytes_TextStyleCallSite_86D489[31] = {
	0x8B, 0x97, 0xB0, 0x00, 0x00, 0x00, 0x4C, 0x8D, 0x87, 0xB4, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x4C,
	0x24, 0x50, 0x4C, 0x89, 0xB4, 0x24, 0xF8, 0x01, 0x00, 0x00, 0xE8, 0x08, 0xFD, 0xFF, 0xFF
};
inline constexpr uint8_t kBytes_DrawTextCallSite_888B8D[29] = {
	0xFF, 0x90, 0xC8, 0x00, 0x00, 0x00, 0x0F, 0x28, 0xDF, 0x4C, 0x8D, 0x87, 0x58, 0x06, 0x00, 0x00,
	0x48, 0x8D, 0x54, 0x24, 0x48, 0x48, 0x8B, 0x08, 0xE8, 0x76, 0xA2, 0x07, 0x00
};
inline constexpr uint8_t kBytes_GetSharedStashPageCount_23B060[16] = {
	0x48, 0x83, 0xEC, 0x28, 0xE8, 0xA7, 0x08, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x2D, 0xE8, 0x5E, 0x02
};
inline constexpr uint8_t kBytes_IsShowingPrevSeason_23B910[16] = {
	0x40, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8D, 0x15, 0x7B, 0x93, 0xAA, 0x01, 0xE8, 0x6E, 0xA3
};
inline constexpr uint8_t kBytes_GetSelectedSharedRecord_23AD80[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57
};
inline constexpr uint8_t kBytes_UpdateActiveStashView_23E630[16] = {
	0x48, 0x89, 0x5C, 0x24, 0x10, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57
};
inline constexpr uint8_t kBytes_BuildItemStatsString_2DA0E0[16] = {
	0x40, 0x53, 0x55, 0x56, 0x57, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x81, 0xEC, 0x80, 0x04
};
inline constexpr uint8_t kBytes_BuildSocketableStatsString_2DA310[16] = {
	0x48, 0x85, 0xD2, 0x0F, 0x84, 0xF0, 0x01, 0x00, 0x00, 0x44, 0x89, 0x4C, 0x24, 0x20, 0x4C, 0x89
};
// Tail bytes at GetUnitStat + 10 (0x2F502A), unaffected by D2RLoader's 10-byte entry detour at 0x2F5020
inline constexpr uint8_t kBytes_GetUnitStatTail_2F502A[22] = {
	0x48, 0x89, 0x74, 0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x41, 0x0F, 0xB7, 0xE8, 0x8B, 0xFA,
	0x48, 0x8B, 0xD9, 0x48, 0x85, 0xC9
};

using BankPanelDrawFn = void(__fastcall*)(void* panel);

// Resolved native function & global pointers (mirrors DAT_18001b500..0x1b5e0)
struct ResolvedNativePointers {
	uintptr_t       exeBase                      = 0;       // 0x1b500
	HMODULE         d2rCoreModule                = nullptr; // 0x1b508
	BankPanelDrawFn originalBankPanelDraw        = nullptr; // 0x1b510
	uintptr_t       fnGetLocalPlayerSlot         = 0;       // 0x1b518 (0x08B2D0)
	uintptr_t       fnGetLocalPlayerUnit         = 0;       // 0x1b520 (0x09A480)
	uintptr_t       fnFindClientUnitByIdAndType  = 0;       // 0x1b528 (0x09A5D0)
	uintptr_t       fnGetUnitInventory           = 0;       // 0x1b530 (0x34A360)
	uintptr_t       fnInventoryGetFirstItem      = 0;       // 0x1b538 (0x388C10)
	uintptr_t       fnItemGetNextInventoryItem   = 0;       // 0x1b540 (0x38ABA0)
	uintptr_t       fnItemGetLocationPage        = 0;       // 0x1b548 (0x36CFE0)
	uintptr_t       fnBuildItemFullDescription   = 0;       // 0x1b550 (0x3E2B634)
	uintptr_t       fnGetDataTables              = 0;       // 0x1b558 (0x300A90)
	uintptr_t       fnGetLocalizedStringById     = 0;       // 0x1b560 (0x3E2B730)
	uintptr_t       fnInventoryGridGetItems      = 0;       // 0x1b568 (0x2C49F0)
	uintptr_t       fnGetLocalizedStringByKey    = 0;       // 0x1b570 (0x5F4B90)
	uintptr_t       fnWidgetFindChild            = 0;       // 0x1b578 (0x855C90)
	uintptr_t       fnBankPanelGetSelectedTab    = 0;       // 0x1b580 (0x23AF50)
	uintptr_t       fnTabBarSelectTab            = 0;       // 0x1b588 (0x878FB0)
	uintptr_t       fnBankPanelGetSharedCount    = 0;       // 0x1b590 (0x23B060)
	uintptr_t       fnBankPanelIsPrevSeason      = 0;       // 0x1b598 (0x23B910)
	uintptr_t       fnBankPanelGetSelectedRecord = 0;       // 0x1b5a0 (0x23AD80)
	uintptr_t       fnBankPanelUpdateActiveView  = 0;       // 0x1b5a8 (0x23E630)
	uint32_t*       ptrActiveSharedStashUnitId   = nullptr; // 0x1b5b0 (D2RCore+0x700E38)
	uintptr_t       fnInitTextStyle              = 0;       // 0x1b5b8 (0x86D1B0)
	uintptr_t       fnDrawStyledText             = 0;       // 0x1b5c0 (0x902E20)
	uintptr_t       fnMeasureStyledText          = 0;       // 0x1b5c8 (0x903CD0)
	uintptr_t       fnDrawFilledRect             = 0;       // 0x1b5d0 (0x657B90)
	uintptr_t       fnWidgetGetScreenPos         = 0;       // 0x1b5d8 (0x8562F0)
	uintptr_t       fnWidgetGetSize              = 0;       // 0x1b5e0 (0x8562A0)
	uintptr_t       fnBuildItemStatsString       = 0;       // (0x2DA0E0)
	uintptr_t       fnBuildSocketableStatsString = 0;       // (0x2DA310)
	uintptr_t       fnGetUnitStat                = 0;       // (0x2F5020)
};

extern ResolvedNativePointers g_Native;

void ClearResolvedPointers() noexcept;
auto VerifyD2RExecutableSignatures(uintptr_t exeBase) noexcept -> bool;
auto VerifyD2RCoreExports(uintptr_t exeBase, uintptr_t coreBase) noexcept -> bool;
auto ResolveNativePointers(uintptr_t exeBase, HMODULE coreModule) noexcept -> bool;

// SEH-guarded memory & call wrappers (0xEB00..0xF2B0)
auto SafeReadMemory(uintptr_t srcAddr, void* dstBuf, size_t size) noexcept -> bool;
auto SafeReadU32(const uint32_t* addr, uint32_t* outVal) noexcept -> bool;
auto SafeReadU64(const uint64_t* addr, uint64_t* outVal) noexcept -> bool;
auto SafeReadU8(const uint8_t* addr, uint8_t* outVal) noexcept -> bool;
auto SafeWriteU32(uint32_t* addr, uint32_t val) noexcept -> bool;
auto SafeWriteU64(uint64_t* addr, uint64_t val) noexcept -> bool;
auto SafeCopyCStringBounded(const char* src, char* dst, size_t capacity, size_t* outLen) noexcept -> bool;

auto SafeCall_GetU32NoArg(uintptr_t fn, uint32_t* outVal) noexcept -> bool;
auto SafeCall_PtrFromU32(uintptr_t fn, uint32_t arg1, uintptr_t* outVal) noexcept -> bool;
auto SafeCall_PtrFromU32U32(uintptr_t fn, uint32_t arg1, uint32_t arg2, uintptr_t* outVal) noexcept -> bool;
auto SafeCall_PtrFromPtr(uintptr_t fn, uintptr_t arg1, uintptr_t* outVal) noexcept -> bool;
auto SafeCall_U8FromPtr(uintptr_t fn, uintptr_t arg1, uint8_t* outVal) noexcept -> bool;
auto SafeCall_BoolFromPtr(uintptr_t fn, uintptr_t arg1, uint8_t* outVal) noexcept -> bool;
auto SafeCall_U64FromPtr(uintptr_t fn, uintptr_t arg1, uint64_t* outVal) noexcept -> bool;
auto SafeCall_VoidFromPtr(uintptr_t fn, void* arg1) noexcept -> bool;
auto SafeCall_VoidFromPtrPtr(uintptr_t fn, uintptr_t arg1, uintptr_t arg2) noexcept -> bool;
auto SafeCall_FindChild(uintptr_t fn, uintptr_t parent, const char* name, uintptr_t* outChild) noexcept -> bool;
auto SafeCall_SelectTab(uintptr_t fn, uintptr_t tabBar, uint32_t tabIndex, uint8_t flag) noexcept -> bool;
auto SafeCall_WidgetGetScreenPos(uintptr_t fn, uintptr_t widget, int32_t* outPos2) noexcept -> bool;
auto SafeCall_WidgetGetRect(uintptr_t fn, uintptr_t widget, int32_t* outRect4) noexcept -> bool;
auto SafeCall_GetGridCellItem(uintptr_t fn, uintptr_t gridWidget, const int32_t* cellCoord2, uintptr_t* outItemUnit) noexcept -> bool;
auto SafeCall_BuildItemDescription(uintptr_t fn, uintptr_t itemUnit, char* outBuf, uintptr_t* outRet) noexcept -> bool;
auto SafeCall_BuildItemStatsString(uintptr_t fn, uintptr_t itemUnit, char* outBuf, int capacity) noexcept -> bool;
auto SafeCall_BuildSocketableStatsString(uintptr_t fn, uintptr_t playerUnit, uintptr_t itemUnit, char* outBuf, int capacity) noexcept -> bool;
auto SafeCall_InitTextStyle(uintptr_t fn, void* styleBuf, uint32_t fontIndex, const float* color4f) noexcept -> bool;
auto SafeCall_MeasureStyledText(
	uintptr_t      fn,
	const char*    utf8Text,
	const void*    styleBuf,
	float          scale,
	const IntRect* rect,
	uint32_t*      outWidth,
	uint32_t*      outHeight) noexcept -> bool;
auto SafeCall_DrawStyledText(
	uintptr_t      fn,
	const char*    utf8Text,
	const IntRect* rect,
	const void*    styleBuf,
	float          scale) noexcept -> bool;
auto SafeCall_DrawFilledRect(
	uintptr_t    fn,
	int          left,
	int          top,
	int          right,
	int          bottom,
	const float* color4f) noexcept -> bool;
auto SafeCall_GetLocalizedStringByKey(uintptr_t fn, const void* keyStruct, const char** outStr) noexcept -> bool;
auto SafeGetItemBaseDisplayName(uintptr_t itemUnit, char* outBuf, size_t capacity, size_t* outLen) noexcept -> bool;

// High-level native UI & item helpers
auto LookupLocalizedString(const char* key, size_t keyLen, std::wstring& outText) -> bool;
void BuildItemFullTooltipText(
	uintptr_t     itemUnit,
	std::wstring& outFullTooltip,
	std::wstring& outShortName,
	std::wstring* outFullNameLine = nullptr);
auto DoesItemMatchQuery(uintptr_t itemUnit, std::wstring_view normalizedQuery, std::wstring_view fullTooltip) -> bool;
auto MeasureNativeText(
	std::wstring_view text,
	const IntRect&    rect,
	const void*       styleBuf,
	float             scale,
	uint32_t*         outWidth,
	uint32_t*         outHeight) -> bool;
auto DrawNativeText(
	std::wstring_view text,
	const IntRect&    rect,
	const void*       styleBuf,
	float             scale) -> bool;
void DrawFilledRect(const IntRect& rect, const Color4f& color) noexcept;
void DrawRectBorder(const IntRect& rect, const Color4f& color, int thickness) noexcept;

// Rounding & scaling helpers (0xF2D0, 0xF350, 0xF430)
auto SafeReadFloat(const float* addr, float* outVal) noexcept -> bool;
auto ScaleBorderThickness(int baseThickness, float scale) noexcept -> int;
auto ScalePositiveDimension(int baseDim, float scale, int* outScaled) noexcept -> bool;
auto ScalePositiveCoord(int baseCoord, float scale, int* outScaled) noexcept -> bool;

} // namespace StashSearch
