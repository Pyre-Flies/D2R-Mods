#include "native.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace StashSearch {

ResolvedNativePointers g_Native{};

namespace {

[[nodiscard]] constexpr auto SafeAddRva(uintptr_t base, uintptr_t rva) noexcept -> uintptr_t {
	return (base != 0 && base <= UINTPTR_MAX - rva) ? (base + rva) : 0;
}

auto VerifyBytes16(uintptr_t exeBase, uintptr_t rva, const void* expected16) noexcept -> bool {
	const uintptr_t addr = SafeAddRva(exeBase, rva);
	if (addr == 0) {
		return false;
	}
	uint8_t buf[16]{};
	if (!SafeReadMemory(addr, buf, sizeof(buf))) {
		return false;
	}
	return std::memcmp(buf, expected16, sizeof(buf)) == 0;
}

auto VerifyBytes32(uintptr_t exeBase, uintptr_t rva, const void* expected32) noexcept -> bool {
	const uintptr_t addr = SafeAddRva(exeBase, rva);
	if (addr == 0) {
		return false;
	}
	uint8_t buf[32]{};
	if (!SafeReadMemory(addr, buf, sizeof(buf))) {
		return false;
	}
	return std::memcmp(buf, expected32, sizeof(buf)) == 0;
}

auto VerifyBytesN(uintptr_t exeBase, uintptr_t rva, const void* expected, size_t len) noexcept -> bool {
	const uintptr_t addr = SafeAddRva(exeBase, rva);
	if (addr == 0) {
		return false;
	}
	uint8_t buf[64]{};
	if (len > sizeof(buf) || !SafeReadMemory(addr, buf, len)) {
		return false;
	}
	return std::memcmp(buf, expected, len) == 0;
}

} // namespace

void ClearResolvedPointers() noexcept {
	g_Native = ResolvedNativePointers{};
}

// 0xF1F0..0xF2B0 SEH memory helpers
auto SafeReadMemory(uintptr_t srcAddr, void* dstBuf, size_t size) noexcept -> bool {
	if (srcAddr == 0 || dstBuf == nullptr) {
		return false;
	}
	__try {
		const auto* src = reinterpret_cast<const volatile uint8_t*>(srcAddr);
		auto*       dst = static_cast<uint8_t*>(dstBuf);
		for (size_t i = 0; i < size; ++i) {
			dst[i] = src[i];
		}
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeReadU32(const uint32_t* addr, uint32_t* outVal) noexcept -> bool {
	if (addr == nullptr || outVal == nullptr) {
		return false;
	}
	__try {
		*outVal = *addr;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeReadFloat(const float* addr, float* outVal) noexcept -> bool {
	if (addr == nullptr || outVal == nullptr) {
		return false;
	}
	__try {
		*outVal = *addr;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeReadU64(const uint64_t* addr, uint64_t* outVal) noexcept -> bool {
	if (addr == nullptr || outVal == nullptr) {
		return false;
	}
	__try {
		*outVal = *addr;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeReadU8(const uint8_t* addr, uint8_t* outVal) noexcept -> bool {
	if (addr == nullptr || outVal == nullptr) {
		return false;
	}
	__try {
		*outVal = *addr;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeWriteU32(uint32_t* addr, uint32_t val) noexcept -> bool {
	if (addr == nullptr) {
		return false;
	}
	__try {
		*addr = val;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeWriteU64(uint64_t* addr, uint64_t val) noexcept -> bool {
	if (addr == nullptr) {
		return false;
	}
	__try {
		*addr = val;
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

// 0xF170: Bounded C-string copy with SEH protection
auto SafeCopyCStringBounded(const char* src, char* dst, size_t capacity, size_t* outLen) noexcept -> bool {
	if (outLen != nullptr) {
		*outLen = 0;
	}
	if (src == nullptr || dst == nullptr || capacity < 2 || outLen == nullptr) {
		return false;
	}
	__try {
		size_t idx = 0;
		while (true) {
			if (idx >= capacity) {
				*outLen = 0;
				return false;
			}
			const char ch = src[idx];
			if (ch == '\0') {
				break;
			}
			dst[idx] = ch;
			idx      = *outLen + 1;
			*outLen  = idx;
		}
		dst[idx] = '\0';
		return *outLen != 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		*outLen = 0;
		return false;
	}
}

// 0xEB00..0xF050: SEH-guarded native call wrappers
auto SafeCall_GetU32NoArg(uintptr_t fn, uint32_t* outVal) noexcept -> bool {
	if (fn == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uint32_t(__fastcall*)();
		*outVal  = reinterpret_cast<Fn>(fn)();
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_PtrFromU32(uintptr_t fn, uint32_t arg1, uintptr_t* outVal) noexcept -> bool {
	if (fn == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uintptr_t(__fastcall*)(uint32_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_PtrFromU32U32(uintptr_t fn, uint32_t arg1, uint32_t arg2, uintptr_t* outVal) noexcept -> bool {
	if (fn == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uintptr_t(__fastcall*)(uint32_t, uint32_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1, arg2);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_PtrFromPtr(uintptr_t fn, uintptr_t arg1, uintptr_t* outVal) noexcept -> bool {
	if (fn == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uintptr_t(__fastcall*)(uintptr_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_U8FromPtr(uintptr_t fn, uintptr_t arg1, uint8_t* outVal) noexcept -> bool {
	if (fn == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uint8_t(__fastcall*)(uintptr_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_BoolFromPtr(uintptr_t fn, uintptr_t arg1, uint8_t* outVal) noexcept -> bool {
	if (fn == 0 || arg1 == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uint8_t(__fastcall*)(uintptr_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_U64FromPtr(uintptr_t fn, uintptr_t arg1, uint64_t* outVal) noexcept -> bool {
	if (fn == 0 || arg1 == 0 || outVal == nullptr) {
		return false;
	}
	__try {
		using Fn = uint64_t(__fastcall*)(uintptr_t);
		*outVal  = reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_VoidFromPtr(uintptr_t fn, void* arg1) noexcept -> bool {
	if (fn == 0) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(void*);
		reinterpret_cast<Fn>(fn)(arg1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_VoidFromPtrPtr(uintptr_t fn, uintptr_t arg1, uintptr_t arg2) noexcept -> bool {
	if (fn == 0 || arg1 == 0 || arg2 == 0) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(uintptr_t, uintptr_t);
		reinterpret_cast<Fn>(fn)(arg1, arg2);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_FindChild(uintptr_t fn, uintptr_t parent, const char* name, uintptr_t* outChild) noexcept -> bool {
	if (fn == 0 || parent == 0 || name == nullptr || outChild == nullptr) {
		return false;
	}
	__try {
		using Fn  = uintptr_t(__fastcall*)(uintptr_t, const char*);
		*outChild = reinterpret_cast<Fn>(fn)(parent, name);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_SelectTab(uintptr_t fn, uintptr_t tabBar, uint32_t tabIndex, uint8_t flag) noexcept -> bool {
	if (fn == 0 || tabBar == 0) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(uintptr_t, uint32_t, uint8_t);
		reinterpret_cast<Fn>(fn)(tabBar, tabIndex, flag);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_WidgetGetScreenPos(uintptr_t fn, uintptr_t widget, int32_t* outPos2) noexcept -> bool {
	if (fn == 0 || widget == 0 || outPos2 == nullptr) {
		return false;
	}
	__try {
		using Fn            = int32_t*(__fastcall*)(uintptr_t, int32_t*);
		int32_t* const ret  = reinterpret_cast<Fn>(fn)(widget, outPos2);
		return ret == outPos2;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_WidgetGetRect(uintptr_t fn, uintptr_t widget, int32_t* outRect4) noexcept -> bool {
	if (fn == 0 || widget == 0 || outRect4 == nullptr) {
		return false;
	}
	__try {
		using Fn            = int32_t*(__fastcall*)(uintptr_t, int32_t*);
		int32_t* const ret  = reinterpret_cast<Fn>(fn)(widget, outRect4);
		return ret == outRect4;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_GetGridCellItem(uintptr_t fn, uintptr_t gridWidget, const int32_t* cellCoord2, uintptr_t* outItemUnit) noexcept -> bool {
	if (fn == 0 || gridWidget == 0 || cellCoord2 == nullptr || outItemUnit == nullptr) {
		return false;
	}
	__try {
		using Fn     = uintptr_t(__fastcall*)(uintptr_t, const int32_t*);
		*outItemUnit = reinterpret_cast<Fn>(fn)(gridWidget, cellCoord2);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_BuildItemDescription(uintptr_t fn, uintptr_t itemUnit, char* outBuf, uintptr_t* outRet) noexcept -> bool {
	if (fn == 0 || outBuf == nullptr || outRet == nullptr) {
		return false;
	}
	__try {
		using Fn = uintptr_t(__fastcall*)(uintptr_t, char*);
		*outRet  = reinterpret_cast<Fn>(fn)(itemUnit, outBuf);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_BuildItemStatsString(uintptr_t fn, uintptr_t itemUnit, char* outBuf, int capacity) noexcept -> bool {
	if (fn == 0 || itemUnit == 0 || outBuf == nullptr || capacity <= 0) {
		return false;
	}
	__try {
		struct ItemStatOptArg {
			uint64_t value    = 0;
			uint8_t  hasValue = 0;
			uint8_t  pad[7]{};
		};
		using Fn = void(__fastcall*)(uintptr_t, char*, int, uint8_t, uintptr_t, uintptr_t, ItemStatOptArg*);
		ItemStatOptArg opt{};
		reinterpret_cast<Fn>(fn)(itemUnit, outBuf, capacity, 1, 0, 0, &opt);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_BuildSocketableStatsString(uintptr_t fn, uintptr_t playerUnit, uintptr_t itemUnit, char* outBuf, int capacity) noexcept -> bool {
	if (fn == 0 || playerUnit == 0 || itemUnit == 0 || outBuf == nullptr || capacity <= 0) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(uintptr_t, uintptr_t, char*, int);
		reinterpret_cast<Fn>(fn)(playerUnit, itemUnit, outBuf, capacity);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_InitTextStyle(uintptr_t fn, void* styleBuf, uint32_t fontIndex, const float* color4f) noexcept -> bool {
	if (fn == 0 || styleBuf == nullptr || color4f == nullptr) {
		return false;
	}
	__try {
		using Fn = uintptr_t(__fastcall*)(void*, uint32_t, const float*);
		return reinterpret_cast<Fn>(fn)(styleBuf, fontIndex, color4f) != 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_MeasureStyledText(
	uintptr_t      fn,
	const char*    utf8Text,
	const void*    styleBuf,
	float          scale,
	const IntRect* rect,
	uint32_t*      outWidth,
	uint32_t*      outHeight) noexcept -> bool {
	if (fn == 0 || utf8Text == nullptr || styleBuf == nullptr || rect == nullptr || outWidth == nullptr || outHeight == nullptr) {
		return false;
	}
	if (!std::isfinite(scale) || scale <= 0.0f) {
		return false;
	}
	__try {
		using Fn              = uint64_t(__fastcall*)(const char*, const void*, float, const IntRect*);
		const uint64_t packed = reinterpret_cast<Fn>(fn)(utf8Text, styleBuf, scale, rect);
		const auto     w      = static_cast<uint32_t>(packed & 0xFFFFFFFFu);
		const auto     h      = static_cast<uint32_t>(packed >> 32);
		if (w < 0x80000000u && h < 0x80000000u) {
			*outWidth  = w;
			*outHeight = h;
			return true;
		}
		return false;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_DrawStyledText(
	uintptr_t      fn,
	const char*    utf8Text,
	const IntRect* rect,
	const void*    styleBuf,
	float          scale) noexcept -> bool {
	if (fn == 0 || utf8Text == nullptr || rect == nullptr || styleBuf == nullptr) {
		return false;
	}
	if (!std::isfinite(scale) || scale <= 0.0f) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(const char*, const IntRect*, const void*, float);
		reinterpret_cast<Fn>(fn)(utf8Text, rect, styleBuf, scale);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_DrawFilledRect(
	uintptr_t    fn,
	int          left,
	int          top,
	int          right,
	int          bottom,
	const float* color4f) noexcept -> bool {
	if (fn == 0 || color4f == nullptr) {
		return false;
	}
	__try {
		using Fn = void(__fastcall*)(int, int, int, int, const float*, int);
		reinterpret_cast<Fn>(fn)(left, top, right, bottom, color4f, 1);
		return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

auto SafeCall_GetLocalizedStringByKey(uintptr_t fn, const void* keyStruct, const char** outStr) noexcept -> bool {
	if (fn == 0 || keyStruct == nullptr || outStr == nullptr) {
		return false;
	}
	const auto* words = static_cast<const int64_t*>(keyStruct);
	if (words[0] == 0 || words[1] < 1) {
		return false;
	}
	__try {
		using Fn         = const char*(__fastcall*)(const void*);
		const char* res  = reinterpret_cast<Fn>(fn)(keyStruct);
		*outStr          = res;
		return res != nullptr;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

// 0xF050: Resolves base item type localized name via DataTables (0x300A90) & GetLocalizedStringById (0x3E2B730)
auto SafeGetItemBaseDisplayName(uintptr_t itemUnit, char* outBuf, size_t capacity, size_t* outLen) noexcept -> bool {
	if (outLen != nullptr) {
		*outLen = 0;
	}
	if (itemUnit == 0 || g_Native.fnGetDataTables == 0 || g_Native.fnGetLocalizedStringById == 0 || outLen == nullptr) {
		return false;
	}
	__try {
		const auto* itemData = *reinterpret_cast<const int32_t* const*>(itemUnit + 0x10);
		if (itemData == nullptr || itemData[0] < 1 || itemData[0] > 9) {
			return false;
		}
		const uint8_t tableGroup = *reinterpret_cast<const uint8_t*>(itemUnit + 0x1BD);
		if (tableGroup >= 4) {
			return false;
		}
		using GetDataTablesFn     = uintptr_t(__fastcall*)(uint8_t);
		const uintptr_t dataTable = reinterpret_cast<GetDataTablesFn>(g_Native.fnGetDataTables)(tableGroup);
		if (dataTable == 0) {
			return false;
		}
		const uintptr_t itemsArray = *reinterpret_cast<const uintptr_t*>(dataTable + 0x13A8);
		const uint64_t  itemsCount = *reinterpret_cast<const uint64_t*>(dataTable + 0x13B0);
		const auto      classIdx   = static_cast<uint64_t>(static_cast<uint32_t>(itemData[0xD]));
		if (itemsArray == 0 || classIdx >= itemsCount) {
			return false;
		}
		const int16_t nameIdx = *reinterpret_cast<const int16_t*>(itemsArray + classIdx * 0x1CC + 0x30);
		const uintptr_t stringsArray = *reinterpret_cast<const uintptr_t*>(dataTable + 0x1388); // 5000 = 0x1388
		const uint64_t  stringsCount = *reinterpret_cast<const uint64_t*>(dataTable + 0x1390);
		if (stringsArray == 0 || nameIdx < 0 || static_cast<uint64_t>(nameIdx) >= stringsCount) {
			return false;
		}
		const uint16_t stringId = *reinterpret_cast<const uint16_t*>(stringsArray + static_cast<uint64_t>(nameIdx) * 0x160 + 2);
		using GetLocalizedByIdFn = const char*(__fastcall*)(uint16_t);
		const char* localized    = reinterpret_cast<GetLocalizedByIdFn>(g_Native.fnGetLocalizedStringById)(stringId);
		return SafeCopyCStringBounded(localized, outBuf, capacity, outLen);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		*outLen = 0;
		return false;
	}
}

// 0x10500: Verifies all 25 expected native byte signatures in D2R.exe
auto VerifyD2RExecutableSignatures(uintptr_t exeBase) noexcept -> bool {
	if (exeBase == 0) {
		return false;
	}
	return VerifyBytesN(exeBase, Rva::BankPanelDraw, kBytes_BankPanelDraw_23AA60, sizeof(kBytes_BankPanelDraw_23AA60)) &&
		VerifyBytes16(exeBase, Rva::GetLocalPlayerSlot, kBytes_GetLocalPlayerSlot_08B2D0) &&
		VerifyBytes16(exeBase, Rva::GetLocalPlayerUnit, kBytes_GetLocalPlayerUnit_09A480) &&
		VerifyBytes16(exeBase, Rva::FindClientUnitByIdAndType, kBytes_FindClientUnitByIdAndType_09A5D0) &&
		VerifyBytes16(exeBase, Rva::GetUnitInventory, kBytes_GetUnitInventory_34A360) &&
		VerifyBytes16(exeBase, Rva::InventoryGetFirstItem, kBytes_InventoryGetFirstItem_388C10) &&
		VerifyBytes16(exeBase, Rva::ItemGetNextInventoryItem, kBytes_ItemGetNextInventoryItem_38ABA0) &&
		VerifyBytes16(exeBase, Rva::BuildItemFullDescription, kBytes_BuildItemFullDescription_3E2B634) &&
		VerifyBytes16(exeBase, Rva::GetDataTables, kBytes_GetDataTables_300A90) &&
		VerifyBytesN(exeBase, Rva::GetLocalizedStringById, kBytes_GetLocalizedStringById_3E2B730, sizeof(kBytes_GetLocalizedStringById_3E2B730)) &&
		VerifyBytes32(exeBase, Rva::InventoryGridGetItems, kBytes_InventoryGridGetItems_2C49F0) &&
		VerifyBytes16(exeBase, Rva::WidgetFindChild, kBytes_WidgetFindChild_855C90) &&
		VerifyBytes16(exeBase, Rva::BankPanelGetSelectedTab, kBytes_BankPanelGetSelectedTab_23AF50) &&
		VerifyBytes16(exeBase, Rva::TabBarSelectTab, kBytes_TabBarSelectTab_878FB0) &&
		VerifyBytes16(exeBase, Rva::BankPanelGetSharedPageCount, kBytes_GetSharedStashPageCount_23B060) &&
		VerifyBytes16(exeBase, Rva::BankPanelIsShowingPrevSeason, kBytes_IsShowingPrevSeason_23B910) &&
		VerifyBytes16(exeBase, Rva::BankPanelGetSelectedRecord, kBytes_GetSelectedSharedRecord_23AD80) &&
		VerifyBytes16(exeBase, Rva::BankPanelUpdateActiveView, kBytes_UpdateActiveStashView_23E630) &&
		VerifyBytes16(exeBase, Rva::WidgetGetScreenPos, kBytes_WidgetGetScreenPos_8562F0) &&
		VerifyBytes16(exeBase, Rva::WidgetGetSize, kBytes_WidgetGetSize_8562A0) &&
		VerifyBytes32(exeBase, Rva::InitTextStyle, kBytes_InitTextStyle_86D1B0) &&
		VerifyBytes32(exeBase, Rva::DrawStyledText, kBytes_DrawStyledText_902E20) &&
		VerifyBytes32(exeBase, Rva::MeasureStyledText, kBytes_MeasureStyledText_903CD0) &&
		VerifyBytes32(exeBase, Rva::DrawFilledRect, kBytes_DrawFilledRect_657B90) &&
		VerifyBytesN(exeBase, Rva::TextStyleCallSite, kBytes_TextStyleCallSite_86D489, sizeof(kBytes_TextStyleCallSite_86D489)) &&
		VerifyBytesN(exeBase, Rva::DrawTextCallSite, kBytes_DrawTextCallSite_888B8D, sizeof(kBytes_DrawTextCallSite_888B8D));
}

// 0x10220: Verifies D2RCore.dll function pointer table matches D2R.exe RVAs
auto VerifyD2RCoreExports(uintptr_t exeBase, uintptr_t coreBase) noexcept -> bool {
	if (exeBase == 0 || coreBase == 0) {
		return false;
	}
	struct Pair {
		uintptr_t coreOffset;
		uintptr_t exeRva;
	};
	constexpr Pair kExpectedPairs[] = {
		{ CoreRva::PtrGetLocalPlayerSlot,           Rva::GetLocalPlayerSlot           },
		{ CoreRva::PtrGetLocalPlayerUnit,           Rva::GetLocalPlayerUnit           },
		{ CoreRva::PtrBankPanelGetSelectedTab,      Rva::BankPanelGetSelectedTab      },
		{ CoreRva::PtrBankPanelIsShowingPrevSeason, Rva::BankPanelIsShowingPrevSeason },
		{ CoreRva::PtrBankPanelGetSharedPageCount,  Rva::BankPanelGetSharedPageCount  },
		{ CoreRva::PtrBankPanelGetSelectedRecord,   Rva::BankPanelGetSelectedRecord   },
		{ CoreRva::PtrBankPanelUpdateActiveView,    Rva::BankPanelUpdateActiveView    },
	};
	for (const auto& pair : kExpectedPairs) {
		const uintptr_t coreSlot = SafeAddRva(coreBase, pair.coreOffset);
		const uintptr_t expected = SafeAddRva(exeBase, pair.exeRva);
		uint64_t        actual   = 0;
		if (coreSlot == 0 || expected == 0 || !SafeReadU64(reinterpret_cast<const uint64_t*>(coreSlot), &actual) || actual != expected) {
			return false;
		}
	}
	const uintptr_t activeUnitIdAddr = SafeAddRva(coreBase, CoreRva::ActiveSharedStashUnitId);
	uint32_t        dummy            = 0;
	return activeUnitIdAddr != 0 && SafeReadU32(reinterpret_cast<const uint32_t*>(activeUnitIdAddr), &dummy);
}

auto ResolveNativePointers(uintptr_t exeBase, HMODULE coreModule) noexcept -> bool {
	const auto coreBase = reinterpret_cast<uintptr_t>(coreModule);
	if (exeBase == 0 || coreBase == 0) {
		return false;
	}
	if (!VerifyD2RExecutableSignatures(exeBase) || !VerifyD2RCoreExports(exeBase, coreBase)) {
		return false;
	}
	const uintptr_t activeUnitIdAddr = SafeAddRva(coreBase, CoreRva::ActiveSharedStashUnitId);
	if (activeUnitIdAddr == 0) {
		return false;
	}

	g_Native.exeBase                      = exeBase;
	g_Native.d2rCoreModule                = coreModule;
	g_Native.fnGetLocalPlayerSlot         = SafeAddRva(exeBase, Rva::GetLocalPlayerSlot);
	g_Native.fnGetLocalPlayerUnit         = SafeAddRva(exeBase, Rva::GetLocalPlayerUnit);
	g_Native.fnFindClientUnitByIdAndType  = SafeAddRva(exeBase, Rva::FindClientUnitByIdAndType);
	g_Native.fnGetUnitInventory           = SafeAddRva(exeBase, Rva::GetUnitInventory);
	g_Native.fnInventoryGetFirstItem      = SafeAddRva(exeBase, Rva::InventoryGetFirstItem);
	g_Native.fnItemGetNextInventoryItem   = SafeAddRva(exeBase, Rva::ItemGetNextInventoryItem);
	g_Native.fnItemGetLocationPage        = SafeAddRva(exeBase, Rva::ItemGetLocationPage);
	g_Native.fnBuildItemFullDescription   = SafeAddRva(exeBase, Rva::BuildItemFullDescription);
	g_Native.fnGetDataTables              = SafeAddRva(exeBase, Rva::GetDataTables);
	g_Native.fnGetLocalizedStringById     = SafeAddRva(exeBase, Rva::GetLocalizedStringById);
	g_Native.fnInventoryGridGetItems      = SafeAddRva(exeBase, Rva::InventoryGridGetItems);
	g_Native.fnDrawFilledRect             = SafeAddRva(exeBase, Rva::DrawFilledRect);
	g_Native.fnWidgetFindChild            = SafeAddRva(exeBase, Rva::WidgetFindChild);
	g_Native.fnBankPanelGetSelectedTab    = SafeAddRva(exeBase, Rva::BankPanelGetSelectedTab);
	g_Native.fnTabBarSelectTab            = SafeAddRva(exeBase, Rva::TabBarSelectTab);
	g_Native.fnBankPanelGetSharedCount    = SafeAddRva(exeBase, Rva::BankPanelGetSharedPageCount);
	g_Native.fnBankPanelIsPrevSeason      = SafeAddRva(exeBase, Rva::BankPanelIsShowingPrevSeason);
	g_Native.fnBankPanelGetSelectedRecord = SafeAddRva(exeBase, Rva::BankPanelGetSelectedRecord);
	g_Native.fnBankPanelUpdateActiveView  = SafeAddRva(exeBase, Rva::BankPanelUpdateActiveView);
	g_Native.ptrActiveSharedStashUnitId   = reinterpret_cast<uint32_t*>(activeUnitIdAddr);
	g_Native.fnInitTextStyle              = SafeAddRva(exeBase, Rva::InitTextStyle);
	g_Native.fnDrawStyledText             = SafeAddRva(exeBase, Rva::DrawStyledText);
	g_Native.fnMeasureStyledText          = SafeAddRva(exeBase, Rva::MeasureStyledText);
	g_Native.fnGetLocalizedStringByKey    = SafeAddRva(exeBase, Rva::GetLocalizedStringByKey);
	g_Native.fnWidgetGetScreenPos         = SafeAddRva(exeBase, Rva::WidgetGetScreenPos);
	g_Native.fnWidgetGetSize              = SafeAddRva(exeBase, Rva::WidgetGetSize);
	if (VerifyBytes16(exeBase, Rva::BuildItemStatsString, kBytes_BuildItemStatsString_2DA0E0)) {
		g_Native.fnBuildItemStatsString = SafeAddRva(exeBase, Rva::BuildItemStatsString);
	}
	if (VerifyBytes16(exeBase, Rva::BuildSocketableStatsString, kBytes_BuildSocketableStatsString_2DA310)) {
		g_Native.fnBuildSocketableStatsString = SafeAddRva(exeBase, Rva::BuildSocketableStatsString);
	}
	if (VerifyBytesN(exeBase, Rva::GetUnitStat + 10, kBytes_GetUnitStatTail_2F502A, sizeof(kBytes_GetUnitStatTail_2F502A))) {
		g_Native.fnGetUnitStat = SafeAddRva(exeBase, Rva::GetUnitStat);
	}
	return true;
}

// 0x6540: UTF-8 to UTF-16 conversion with CP_ACP fallback
auto Utf8ToWideWithAcpFallback(std::string_view input) -> std::wstring {
	if (input.empty()) {
		return {};
	}
	const int srcLen = input.size() > 0x7FFFFFFFu ? 0x7FFFFFFF : static_cast<int>(input.size());
	UINT      cp     = CP_UTF8;
	DWORD     flags  = MB_ERR_INVALID_CHARS;
	int       dstLen = MultiByteToWideChar(cp, flags, input.data(), srcLen, nullptr, 0);
	if (dstLen < 1) {
		cp     = CP_ACP;
		flags  = 0;
		dstLen = MultiByteToWideChar(cp, flags, input.data(), srcLen, nullptr, 0);
		if (dstLen < 1) {
			return {};
		}
	}
	std::wstring result(static_cast<size_t>(dstLen), L'\0');
	if (MultiByteToWideChar(cp, flags, input.data(), srcLen, result.data(), dstLen) < 1) {
		return {};
	}
	return result;
}

// 0x6210: Strict UTF-8 to UTF-16 conversion
auto Utf8ToWideStrict(const char* input) -> std::wstring {
	if (input == nullptr || *input == '\0') {
		return {};
	}
	const size_t rawLen = std::strlen(input);
	const int    srcLen = rawLen > 0x7FFFFFFFu ? 0x7FFFFFFF : static_cast<int>(rawLen);
	const int    dstLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, srcLen, nullptr, 0);
	if (dstLen < 1) {
		return {};
	}
	std::wstring result(static_cast<size_t>(dstLen), L'\0');
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, srcLen, result.data(), dstLen) < 1) {
		return {};
	}
	return result;
}

// 0x63C0: Strict UTF-16 to UTF-8 conversion
auto WideToUtf8Strict(std::wstring_view input, std::string& outUtf8) -> bool {
	outUtf8.clear();
	if (input.empty()) {
		return true;
	}
	const int srcLen = input.size() > 0x7FFFFFFFu ? 0x7FFFFFFF : static_cast<int>(input.size());
	const int dstLen = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), srcLen, nullptr, 0, nullptr, nullptr);
	if (dstLen < 1) {
		return false;
	}
	outUtf8.assign(static_cast<size_t>(dstLen), '\0');
	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), srcLen, outUtf8.data(), dstLen, nullptr, nullptr) < 1) {
		outUtf8.clear();
		return false;
	}
	return true;
}

// 0xF710: Strips D2R color codes (0xFF 'c' X and UTF-8 0xC3 0xBF 'c' X), stops at '*', and collapses whitespace on narrow C-string
auto NormalizeNarrowTooltipLine(const char* raw) -> std::string {
	std::string result;
	if (raw == nullptr) {
		return result;
	}
	size_t len = 0;
	while (len < 0x3FF && raw[len] != '\0') {
		++len;
	}
	result.reserve(len);
	bool   pendingSpace = false;
	size_t i            = 0;
	while (i < len) {
		const auto b = static_cast<uint8_t>(raw[i]);
		if (b == 0x2A) { // '*'
			break;
		}
		if (b == 0xFF && i + 2 < len && raw[i + 1] == 'c') {
			i += 3;
			continue;
		}
		if (b == 0xC3 && i + 3 < len && static_cast<uint8_t>(raw[i + 1]) == 0xBF && raw[i + 2] == 'c') {
			i += 4;
			continue;
		}
		if (b == '\t' || b == '\n' || b == '\r' || std::isspace(static_cast<unsigned char>(b)) != 0) {
			pendingSpace = !result.empty();
			do {
				++i;
				if (i >= len) {
					return result;
				}
				const auto nextB = static_cast<uint8_t>(raw[i]);
				if (nextB != '\r' && nextB != '\t' && nextB != '\n' && std::isspace(static_cast<unsigned char>(nextB)) == 0) {
					break;
				}
			} while (true);
			continue;
		}
		if (pendingSpace) {
			result.push_back(' ');
			pendingSpace = false;
		}
		result.push_back(static_cast<char>(b));
		++i;
	}
	return result;
}

// Strips D2R color codes and collapses whitespace across multi-line item stat/affix descriptions without stopping at '*'
auto NormalizeNarrowAffixText(const char* raw) -> std::string {
	std::string result;
	if (raw == nullptr) {
		return result;
	}
	size_t len = 0;
	while (len < 0xFFF && raw[len] != '\0') {
		++len;
	}
	result.reserve(len);
	bool   pendingSpace = false;
	size_t i            = 0;
	while (i < len) {
		const auto b = static_cast<uint8_t>(raw[i]);
		if (b == 0xFF && i + 2 < len && raw[i + 1] == 'c') {
			i += 3;
			continue;
		}
		if (b == 0xC3 && i + 3 < len && static_cast<uint8_t>(raw[i + 1]) == 0xBF && raw[i + 2] == 'c') {
			i += 4;
			continue;
		}
		if (b == '\t' || b == '\n' || b == '\r' || std::isspace(static_cast<unsigned char>(b)) != 0) {
			pendingSpace = !result.empty();
			do {
				++i;
				if (i >= len) {
					return result;
				}
				const auto nextB = static_cast<uint8_t>(raw[i]);
				if (nextB != '\r' && nextB != '\t' && nextB != '\n' && std::isspace(static_cast<unsigned char>(nextB)) == 0) {
					break;
				}
			} while (true);
			continue;
		}
		if (pendingSpace) {
			result.push_back(' ');
			pendingSpace = false;
		}
		result.push_back(static_cast<char>(b));
		++i;
	}
	return result;
}

namespace {

auto StripLootFilterTagsWide(std::wstring_view line) -> std::wstring {
	std::wstring stripped;
	stripped.reserve(line.size());
	bool   pendingSpace = false;
	size_t i            = 0;
	while (i < line.size()) {
		const wchar_t ch = line[i];
		// Strip bracketed filter tags like "[N]", "[X]", "[E]"
		if (ch == L'[') {
			const size_t closePos = line.find(L']', i + 1);
			if (closePos != std::wstring_view::npos && (closePos - i) <= 16) {
				i            = closePos + 1;
				pendingSpace = !stripped.empty();
				continue;
			}
		}
		// Strip parenthesized numeric tags like "(8)", "(12)"
		if (ch == L'(') {
			const size_t closePos = line.find(L')', i + 1);
			if (closePos != std::wstring_view::npos && closePos > i + 1 && (closePos - i) <= 10) {
				bool allDigitsOrSpace = true;
				for (size_t k = i + 1; k < closePos; ++k) {
					const wchar_t c = line[k];
					if ((c < L'0' || c > L'9') && c != L' ' && c != L'+' && c != L'-') {
						allDigitsOrSpace = false;
						break;
					}
				}
				if (allDigitsOrSpace) {
					i            = closePos + 1;
					pendingSpace = !stripped.empty();
					continue;
				}
			}
		}
		if (ch == L' ' || ch == L'\t' || iswspace(static_cast<wint_t>(ch)) != 0) {
			pendingSpace = !stripped.empty();
			++i;
			continue;
		}
		if (pendingSpace) {
			stripped.push_back(L' ');
			pendingSpace = false;
		}
		stripped.push_back(ch);
		++i;
	}
	return stripped;
}

} // namespace

// Extracts a concise display name from raw D2R item description (e.g. "Skull Cap [N]\nDevourer of Dreams (8)" -> "Devourer of Dreams")
auto ExtractShortItemDisplayName(std::string_view rawNarrow) -> std::wstring {
	if (rawNarrow.empty()) {
		return {};
	}
	// Truncate at '*' if present
	const size_t starPos = rawNarrow.find('*');
	if (starPos != std::string_view::npos) {
		rawNarrow = rawNarrow.substr(0, starPos);
	}

	// Split by '\r' or '\n' and keep the last non-empty normalized line
	std::wstring lastNonEmptyLine;
	size_t       pos = 0;
	while (pos < rawNarrow.size()) {
		size_t nextBreak = rawNarrow.find_first_of("\r\n", pos);
		if (nextBreak == std::string_view::npos) {
			nextBreak = rawNarrow.size();
		}
		const std::string segment(rawNarrow.substr(pos, nextBreak - pos));
		const std::string normSeg = NormalizeNarrowTooltipLine(segment.c_str());
		if (!normSeg.empty()) {
			std::wstring wideSeg = NormalizeWideTooltipLine(Utf8ToWideWithAcpFallback(normSeg));
			if (!wideSeg.empty()) {
				lastNonEmptyLine = std::move(wideSeg);
			}
		}
		pos = nextBreak + 1;
	}

	if (lastNonEmptyLine.empty()) {
		return {};
	}

	// If a single line contains a tier bracket like "Skull Cap [N] Devourer of Dreams (8)", prefer the name after ']'
	const size_t openBracket = lastNonEmptyLine.find(L'[');
	if (openBracket != std::wstring::npos) {
		const size_t closeBracket = lastNonEmptyLine.find(L']', openBracket + 1);
		if (closeBracket != std::wstring::npos && (closeBracket - openBracket) <= 6 && closeBracket + 1 < lastNonEmptyLine.size()) {
			std::wstring afterBracket = StripLootFilterTagsWide(std::wstring_view(lastNonEmptyLine).substr(closeBracket + 1));
			if (!afterBracket.empty()) {
				return afterBracket;
			}
		}
	}

	std::wstring cleaned = StripLootFilterTagsWide(lastNonEmptyLine);
	return cleaned.empty() ? lastNonEmptyLine : cleaned;
}

// 0x49F0: Strips D2R color codes (L"\x00FFcX" and L"\x00C3\x00BFcX"), stops at L'*', and collapses whitespace on wide string
auto NormalizeWideTooltipLine(std::wstring_view raw) -> std::wstring {
	std::wstring result;
	result.reserve(raw.size());
	bool   pendingSpace = false;
	size_t i            = 0;
	while (i < raw.size()) {
		const wchar_t ch = raw[i];
		if (ch == L'*') {
			break;
		}
		if (ch == 0x00FF && i + 2 < raw.size() && raw[i + 1] == L'c') {
			i += 3;
			continue;
		}
		if (ch == 0x00C3 && i + 3 < raw.size() && raw[i + 1] == 0x00BF && raw[i + 2] == L'c') {
			i += 4;
			continue;
		}
		if (ch == L'\r' || ch == L'\t' || ch == L'\n' || iswspace(static_cast<wint_t>(ch)) != 0) {
			pendingSpace = !result.empty();
			do {
				++i;
				if (i >= raw.size()) {
					return result;
				}
				const wchar_t nextCh = raw[i];
				if (nextCh != L'\r' && nextCh != L'\t' && nextCh != L'\n' && iswspace(static_cast<wint_t>(nextCh)) == 0) {
					break;
				}
			} while (true);
			continue;
		}
		if (pendingSpace) {
			result.push_back(L' ');
			pendingSpace = false;
		}
		result.push_back(ch);
		++i;
	}
	return result;
}

// 0xCD70: Folds wide string to lowercase using LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE)
auto FoldStringLowerInvariant(std::wstring_view input) -> std::wstring {
	if (input.empty()) {
		return {};
	}
	const int srcLen = input.size() > 0x7FFFFFFFu ? 0x7FFFFFFF : static_cast<int>(input.size());
	const int dstLen = LCMapStringEx(L"", LCMAP_LOWERCASE, input.data(), srcLen, nullptr, 0, nullptr, nullptr, 0);
	if (dstLen < 1) {
		return {};
	}
	std::wstring folded(static_cast<size_t>(dstLen), L'\0');
	if (LCMapStringEx(L"", LCMAP_LOWERCASE, input.data(), srcLen, folded.data(), dstLen, nullptr, nullptr, 0) < 1) {
		return {};
	}
	return folded;
}

// 0xDF70: Case-insensitive substring check
auto ContainsCaseInsensitive(std::wstring_view needle, std::wstring_view haystack) -> bool {
	const std::wstring foldedNeedle   = FoldStringLowerInvariant(needle);
	const std::wstring foldedHaystack = FoldStringLowerInvariant(haystack);
	if (foldedNeedle.empty() || foldedHaystack.empty() || foldedNeedle.size() > foldedHaystack.size()) {
		return false;
	}
	return foldedHaystack.find(foldedNeedle) != std::wstring::npos;
}

namespace {

[[nodiscard]] constexpr auto TrimWideWhitespace(std::wstring_view sv) noexcept -> std::wstring_view {
	while (!sv.empty() && (sv.front() == L' ' || sv.front() == L'\t' || sv.front() == L'\r' || sv.front() == L'\n')) {
		sv.remove_prefix(1);
	}
	while (!sv.empty() && (sv.back() == L' ' || sv.back() == L'\t' || sv.back() == L'\r' || sv.back() == L'\n')) {
		sv.remove_suffix(1);
	}
	return sv;
}

[[nodiscard]] constexpr auto IsAsciiAlpha(wchar_t ch) noexcept -> bool {
	return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

struct ShorthandRule {
	std::wstring_view token;
	std::wstring_view target1;
	std::wstring_view target2;
	bool              exactExpansionOnly;
};

inline constexpr ShorthandRule kShorthandRules[] = {
	// Sockets
	{ L"0os",      L"socketed (0)",                         L"",                              true  },
	{ L"1os",      L"socketed (1)",                         L"",                              true  },
	{ L"2os",      L"socketed (2)",                         L"",                              true  },
	{ L"3os",      L"socketed (3)",                         L"",                              true  },
	{ L"4os",      L"socketed (4)",                         L"",                              true  },
	{ L"5os",      L"socketed (5)",                         L"",                              true  },
	{ L"6os",      L"socketed (6)",                         L"",                              true  },
	{ L"os",       L"socketed (",                           L"",                              true  },
	{ L"sock",     L"socketed (",                           L"",                              true  },
	{ L"sockets",  L"socketed (",                           L"",                              true  },

	// Item quality, flags, and tier tags
	{ L"eth",      L"ethereal",                             L"[flag:ethereal]",               true  },
	{ L"rw",       L"[flag:runeword]",                      L"runeword",                      true  },
	{ L"runeword", L"[flag:runeword]",                      L"runeword",                      false },
	{ L"unid",     L"[flag:unid]",                          L"unidentified",                  true  },
	{ L"uni",      L"[qual:unique]",                        L"",                              true  },
	{ L"unique",   L"[qual:unique]",                        L"",                              false },
	{ L"set",      L"[qual:set]",                           L"",                              true  },
	{ L"rare",     L"[qual:rare]",                          L"",                              false },
	{ L"mag",      L"[qual:magic]",                         L"",                              true  },
	{ L"magic",    L"[qual:magic]",                         L"",                              false },
	{ L"craft",    L"[qual:crafted]",                       L"",                              true  },
	{ L"crafted",  L"[qual:crafted]",                       L"",                              false },
	{ L"sup",      L"[qual:superior]",                      L"superior",                      true  },
	{ L"norm",     L"[n]",                                  L"[qual:normal]",                 true  },
	{ L"exc",      L"[x]",                                  L"",                              true  },
	{ L"eli",      L"[e]",                                  L"",                              true  },
	{ L"elite",    L"[e]",                                  L"",                              false },
	{ L"corr",     L"corrupted",                            L"",                              false },

	// Common D2R / Reimagined affix shorthands
	{ L"fcr",      L"faster cast rate",                     L"",                              true  },
	{ L"ias",      L"increased attack speed",               L"",                              true  },
	{ L"fhr",      L"faster hit recovery",                  L"",                              true  },
	{ L"frw",      L"faster run/walk",                      L"",                              true  },
	{ L"fbr",      L"faster block rate",                    L"",                              true  },
	{ L"mf",       L"better chance of getting magic items", L"",                              true  },
	{ L"gf",       L"extra gold from monsters",             L"",                              true  },
	{ L"eg",       L"extra gold from monsters",             L"",                              true  },
	{ L"ll",       L"life stolen per hit",                  L"",                              true  },
	{ L"ml",       L"mana stolen per hit",                  L"",                              true  },
	{ L"cb",       L"crushing blow",                        L"",                              true  },
	{ L"ds",       L"deadly strike",                        L"",                              true  },
	{ L"ow",       L"open wounds",                          L"",                              true  },
	{ L"cbf",      L"cannot be frozen",                     L"",                              true  },
	{ L"itd",      L"ignore target's defense",              L"",                              true  },
	{ L"pmh",      L"prevent monster heal",                 L"",                              true  },
	{ L"ed",       L"enhanced damage",                      L"enhanced defense",              true  },
	{ L"ar",       L"attack rating",                        L"",                              true  },
	{ L"@",        L"all resistances",                      L"",                              true  },
	{ L"@res",     L"all resistances",                      L"",                              true  },
	{ L"allres",   L"all resistances",                      L"",                              true  },
	{ L"ares",     L"all resistances",                      L"",                              true  },
	{ L"fres",     L"fire resist",                          L"",                              true  },
	{ L"cres",     L"cold resist",                          L"",                              true  },
	{ L"lres",     L"lightning resist",                     L"",                              true  },
	{ L"pres",     L"poison resist",                        L"",                              true  },
	{ L"pdr",      L"physical damage received reduced",     L"damage reduced by",             true  },
	{ L"mdr",      L"magic damage reduced",                 L"",                              true  },
	{ L"laek",     L"life after each kill",                 L"",                              true  },
	{ L"lak",      L"life after each kill",                 L"",                              true  },
	{ L"maek",     L"mana after each kill",                 L"",                              true  },
	{ L"mak",      L"mana after each kill",                 L"",                              true  },
	{ L"plr",      L"poison length reduced",                L"",                              true  },
	{ L"skills",   L"to all skills",                        L"skill levels",                  false },
	{ L"+skills",  L"to all skills",                        L"skill levels",                  true  },
	{ L"tele",     L"teleport",                             L"",                              false },
	{ L"bo",       L"battle orders",                        L"",                              true  },
	{ L"conv",     L"conviction",                           L"",                              false },
	{ L"fana",     L"fanaticism",                           L"",                              false },
	{ L"conc",     L"concentration",                        L"",                              false },
	{ L"med",      L"meditation",                           L"",                              false },
};

auto MatchFoldedAtomicTerm(std::wstring_view foldedTerm, std::wstring_view foldedHaystack) noexcept -> bool {
	if (foldedTerm.empty() || foldedHaystack.empty()) {
		return false;
	}
	for (const auto& rule : kShorthandRules) {
		if (foldedTerm == rule.token) {
			if (!rule.target1.empty() && foldedHaystack.find(rule.target1) != std::wstring_view::npos) {
				return true;
			}
			if (!rule.target2.empty() && foldedHaystack.find(rule.target2) != std::wstring_view::npos) {
				return true;
			}
			if (rule.exactExpansionOnly) {
				return false;
			}
			break;
		}
	}
	return foldedHaystack.find(foldedTerm) != std::wstring_view::npos;
}

// Evaluates a single sub-clause (with optional leading '!' or '-<letter>' negation, preserving hyphenated words like "Bul-Kathos")
auto EvaluateSingleClause(std::wstring_view rawClause, std::wstring_view foldedHaystack, bool* outIsNonEmpty) -> bool {
	std::wstring_view clause = TrimWideWhitespace(rawClause);
	if (clause.empty()) {
		if (outIsNonEmpty != nullptr) {
			*outIsNonEmpty = false;
		}
		return true;
	}
	bool negated = false;
	if (clause.front() == L'!') {
		negated = true;
		clause  = TrimWideWhitespace(clause.substr(1));
	} else if (clause.front() == L'-' && clause.size() >= 2 && IsAsciiAlpha(clause[1])) {
		// Treat leading '-<letter>' as negation only when the token itself is not a hyphenated compound word
		const size_t innerHyphen = clause.find(L'-', 1);
		if (innerHyphen == std::wstring_view::npos) {
			negated = true;
			clause  = TrimWideWhitespace(clause.substr(1));
		}
	}

	if (clause.empty()) {
		if (outIsNonEmpty != nullptr) {
			*outIsNonEmpty = false;
		}
		return true;
	}
	if (outIsNonEmpty != nullptr) {
		*outIsNonEmpty = true;
	}

	const std::wstring foldedTerm = FoldStringLowerInvariant(clause);
	if (foldedTerm.empty()) {
		return true;
	}
	bool matched = MatchFoldedAtomicTerm(std::wstring_view(foldedTerm), foldedHaystack);
	if (!matched && !negated && foldedTerm.find(L' ') != std::wstring::npos) {
		// Fall back to matching all space-separated tokens when a positive multi-word clause is not a contiguous phrase
		const std::wstring_view view(foldedTerm);
		bool                    allWordsMatch = true;
		bool                    anyWord       = false;
		size_t                  pos           = 0;
		while (pos < view.size()) {
			while (pos < view.size() && view[pos] == L' ') {
				++pos;
			}
			if (pos >= view.size()) {
				break;
			}
			size_t end = view.find(L' ', pos);
			if (end == std::wstring_view::npos) {
				end = view.size();
			}
			anyWord = true;
			if (!MatchFoldedAtomicTerm(view.substr(pos, end - pos), foldedHaystack)) {
				allWordsMatch = false;
				break;
			}
			pos = end + 1;
		}
		matched = anyWord && allWordsMatch;
	}
	return negated ? !matched : matched;
}

// Evaluates an '&' clause, also splitting any space-preceded " !term" or " -<letter>term" exclusions while keeping "Bul-Kathos" intact
auto EvaluateAndClauseWithInlineExclusions(std::wstring_view rawAndClause, std::wstring_view foldedHaystack, bool* outHasAnyTerm) -> bool {
	std::wstring_view s = TrimWideWhitespace(rawAndClause);
	if (s.empty()) {
		return true;
	}
	size_t segStart = 0;
	for (size_t i = 1; i < s.size(); ++i) {
		const bool prevSpace = (s[i - 1] == L' ' || s[i - 1] == L'\t');
		if (!prevSpace) {
			continue;
		}
		const bool isBangNeg = (s[i] == L'!' && i + 1 < s.size() && s[i + 1] != L' ');
		const bool isDashNeg = (s[i] == L'-' && i + 1 < s.size() && IsAsciiAlpha(s[i + 1]));
		if (isBangNeg || isDashNeg) {
			bool subNonEmpty = false;
			if (!EvaluateSingleClause(s.substr(segStart, i - segStart), foldedHaystack, &subNonEmpty)) {
				if (subNonEmpty && outHasAnyTerm != nullptr) {
					*outHasAnyTerm = true;
				}
				return false;
			}
			if (subNonEmpty && outHasAnyTerm != nullptr) {
				*outHasAnyTerm = true;
			}
			segStart = i;
		}
	}
	bool tailNonEmpty = false;
	const bool tailOk = EvaluateSingleClause(s.substr(segStart), foldedHaystack, &tailNonEmpty);
	if (tailNonEmpty && outHasAnyTerm != nullptr) {
		*outHasAnyTerm = true;
	}
	return tailOk;
}

} // namespace

// Evaluates a boolean search query supporting '|' (OR), '&' (AND), '!' / '-<word>' (NOT), hyphenated names, and D2R shorthands
auto EvaluateSearchQuery(std::wstring_view query, std::wstring_view haystack) -> bool {
	const std::wstring_view trimmedQuery = TrimWideWhitespace(query);
	if (trimmedQuery.empty() || haystack.empty()) {
		return false;
	}
	const std::wstring foldedHaystack = FoldStringLowerInvariant(haystack);
	if (foldedHaystack.empty()) {
		return false;
	}

	bool   anyBranchEvaluated = false;
	size_t orStart            = 0;
	while (orStart <= trimmedQuery.size()) {
		size_t orEnd = trimmedQuery.find(L'|', orStart);
		if (orEnd == std::wstring_view::npos) {
			orEnd = trimmedQuery.size();
		}
		const std::wstring_view orBranch = TrimWideWhitespace(trimmedQuery.substr(orStart, orEnd - orStart));
		if (!orBranch.empty()) {
			bool   branchHasTerm = false;
			bool   branchMatched = true;
			size_t andStart      = 0;
			while (andStart <= orBranch.size()) {
				size_t andEnd = orBranch.find(L'&', andStart);
				if (andEnd == std::wstring_view::npos) {
					andEnd = orBranch.size();
				}
				const std::wstring_view andClause = TrimWideWhitespace(orBranch.substr(andStart, andEnd - andStart));
				if (!andClause.empty()) {
					if (!EvaluateAndClauseWithInlineExclusions(andClause, foldedHaystack, &branchHasTerm)) {
						branchMatched = false;
						break;
					}
				}
				if (andEnd == orBranch.size()) {
					break;
				}
				andStart = andEnd + 1;
			}
			if (branchHasTerm) {
				anyBranchEvaluated = true;
				if (branchMatched) {
					return true;
				}
			}
		}
		if (orEnd == trimmedQuery.size()) {
			break;
		}
		orStart = orEnd + 1;
	}
	return anyBranchEvaluated ? false : false;
}

// 0xD650: Parses "/<page>" shared stash jump command
auto ParseSlashPageCommand(std::wstring_view query) noexcept -> SlashPageCommand {
	size_t start = 0;
	size_t end   = query.size();
	while (start < end) {
		const wchar_t ch = query[start];
		if (ch == L' ' || static_cast<uint16_t>(ch - 9) < 5) {
			++start;
		} else {
			break;
		}
	}
	while (start < end) {
		const wchar_t ch = query[end - 1];
		if (ch == L' ' || static_cast<uint16_t>(ch - 9) < 5) {
			--end;
		} else {
			break;
		}
	}
	if (start == end || query[start] != L'/') {
		return { SlashCommandState::None, 0 };
	}
	++start;
	if (start == end) {
		return { SlashCommandState::MissingNumber, 0 };
	}
	uint64_t pageNumber = 0;
	for (size_t i = start; i < end; ++i) {
		const auto ch    = static_cast<uint16_t>(query[i]);
		const auto digit = static_cast<uint16_t>(ch - L'0');
		if (digit > 9 || pageNumber > (~static_cast<uint64_t>(digit)) / 10) {
			return { SlashCommandState::InvalidNumber, 0 };
		}
		pageNumber = pageNumber * 10 + digit;
	}
	if (pageNumber == 0) {
		return { SlashCommandState::InvalidNumber, 0 };
	}
	return { SlashCommandState::ValidPage, pageNumber };
}

// 0xD3C0: Formats prompt row for "/<page>" shared stash jump command
auto FormatSlashPagePrompt(const SlashPageCommand& cmd) -> std::wstring {
	switch (cmd.state) {
	case SlashCommandState::MissingNumber:
		return L"Enter shared page number";
	case SlashCommandState::InvalidNumber:
		return L"Invalid page number";
	case SlashCommandState::ValidPage:
		return L"Press Enter to jump to shared page " + std::to_wstring(cmd.pageNumber);
	case SlashCommandState::None:
	default:
		return {};
	}
}

// Clamps dropdown selection & scroll window (used by 0xB280, 0xBAB0, 0xDD50)
void AdjustDropdownSelectionAndScroll(
	size_t  matchCount,
	size_t  visibleCapacity,
	size_t  desiredIndex,
	size_t& inOutSelectedIndex,
	size_t& inOutScrollOffset) noexcept {
	const size_t cap = visibleCapacity != 0 ? visibleCapacity : kMaxDropdownVisibleRows;
	if (matchCount == 0 || cap == 0) {
		inOutSelectedIndex = 0;
		inOutScrollOffset  = 0;
		return;
	}
	const size_t effectiveCap = std::min(matchCount, cap);
	inOutSelectedIndex        = std::min(desiredIndex, matchCount - 1);
	const size_t maxScroll    = matchCount > effectiveCap ? (matchCount - effectiveCap) : 0;
	if (inOutScrollOffset > maxScroll) {
		inOutScrollOffset = maxScroll;
	}
	if (inOutSelectedIndex < inOutScrollOffset) {
		inOutScrollOffset = std::min(inOutSelectedIndex, maxScroll);
	} else if (inOutSelectedIndex >= inOutScrollOffset + effectiveCap) {
		inOutScrollOffset = std::min(inOutSelectedIndex - effectiveCap + 1, maxScroll);
	}
}

// 0xAB40: Looks up a localized string by key ("LobbySearch", "personal", "shared")
auto LookupLocalizedString(const char* key, size_t keyLen, std::wstring& outText) -> bool {
	if (key == nullptr || keyLen == 0) {
		return false;
	}
	struct KeyView {
		const char* ptr;
		size_t      len;
	};
	const KeyView keyStruct{ key, keyLen };
	const char*   rawStr = nullptr;
	if (!SafeCall_GetLocalizedStringByKey(g_Native.fnGetLocalizedStringByKey, &keyStruct, &rawStr) || rawStr == nullptr) {
		return false;
	}
	char   buf[256]{};
	size_t copiedLen = 0;
	if (!SafeCopyCStringBounded(rawStr, buf, sizeof(buf), &copiedLen) || copiedLen == 0) {
		return false;
	}
	std::wstring wide = Utf8ToWideStrict(buf);
	std::wstring norm = NormalizeWideTooltipLine(wide);
	if (norm.empty()) {
		return false;
	}
	outText = std::move(norm);
	return true;
}

namespace {

void SafeAppendItemQualityAndFlagTags(uintptr_t itemUnit, std::wstring& outFullTooltip) noexcept {
	if (itemUnit == 0) {
		return;
	}
	int32_t  quality = 0;
	uint32_t flags   = 0;
	bool     ok      = false;
	__try {
		const auto* itemData = *reinterpret_cast<const int32_t* const*>(itemUnit + 0x10);
		if (itemData != nullptr) {
			quality = itemData[0];
			flags   = static_cast<uint32_t>(itemData[6]);
			ok      = true;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		ok = false;
	}
	if (!ok) {
		return;
	}
	switch (quality) {
	case 1:
	case 2:
		outFullTooltip.append(L" [qual:normal]");
		break;
	case 3:
		outFullTooltip.append(L" [qual:superior] Superior");
		break;
	case 4:
		outFullTooltip.append(L" [qual:magic]");
		break;
	case 5:
		outFullTooltip.append(L" [qual:set]");
		break;
	case 6:
		outFullTooltip.append(L" [qual:rare]");
		break;
	case 7:
		outFullTooltip.append(L" [qual:unique]");
		break;
	case 8:
		outFullTooltip.append(L" [qual:crafted]");
		break;
	case 9:
		outFullTooltip.append(L" [qual:tempered]");
		break;
	default:
		break;
	}
	if ((flags & 0x04000000u) != 0) {
		outFullTooltip.append(L" [flag:runeword] Runeword");
	}
	if ((flags & 0x00400000u) != 0) {
		outFullTooltip.append(L" [flag:ethereal] Ethereal");
	}
	if ((flags & 0x00000010u) == 0) {
		outFullTooltip.append(L" [flag:unid] Unidentified");
	}

	int32_t numSockets = 0;
	if ((flags & 0x00000800u) != 0 && g_Native.fnGetUnitStat != 0) {
		__try {
			using GetStatFn = int32_t(__fastcall*)(uintptr_t, uint32_t, uint16_t);
			numSockets      = reinterpret_cast<GetStatFn>(g_Native.fnGetUnitStat)(itemUnit, 194u, 0u);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			numSockets = 0;
		}
	}
	if (numSockets > 0 && numSockets <= 16) {
		wchar_t sockTag[32]{};
		std::swprintf(sockTag, sizeof(sockTag) / sizeof(wchar_t), L" Socketed (%d)", static_cast<int>(numSockets));
		outFullTooltip.append(sockTag);
	} else {
		outFullTooltip.append(L" Socketed (0)");
	}
}

} // namespace

// 0xA960: Builds item full description (0x3E2B634) + affix stats (0x2DA0E0 / 0x2DA310) and extracts concise display name
void BuildItemFullTooltipText(
	uintptr_t     itemUnit,
	std::wstring& outFullTooltip,
	std::wstring& outShortName,
	std::wstring* outFullNameLine) {
	outFullTooltip.clear();
	outShortName.clear();
	if (outFullNameLine != nullptr) {
		outFullNameLine->clear();
	}
	if (itemUnit == 0) {
		return;
	}
	char      rawBuf[1024]{};
	uintptr_t ret = 0;
	if (!SafeCall_BuildItemDescription(g_Native.fnBuildItemFullDescription, itemUnit, rawBuf, &ret)) {
		return;
	}
	rawBuf[sizeof(rawBuf) - 1] = '\0';
	if (rawBuf[0] == '\0') {
		return;
	}

	const std::string  normFullNarrow = NormalizeNarrowTooltipLine(rawBuf);
	const std::wstring wideFull       = Utf8ToWideWithAcpFallback(normFullNarrow);
	const std::wstring normNameLine   = NormalizeWideTooltipLine(wideFull);
	outFullTooltip                    = normNameLine;
	if (outFullNameLine != nullptr) {
		*outFullNameLine = normNameLine;
	}

	outShortName = ExtractShortItemDisplayName(rawBuf);
	if (outShortName.empty()) {
		outShortName = normNameLine;
	}

	// Append item quality & flag tags ([qual:unique], [flag:runeword], [flag:ethereal], etc.)
	SafeAppendItemQualityAndFlagTags(itemUnit, outFullTooltip);

	// Append base item display name so searching base type (e.g. "Skull Cap") always matches
	char   baseNameBuf[512]{};
	size_t baseNameLen = 0;
	if (SafeGetItemBaseDisplayName(itemUnit, baseNameBuf, sizeof(baseNameBuf), &baseNameLen) && baseNameLen > 0) {
		const std::string  normBaseNarrow = NormalizeNarrowTooltipLine(baseNameBuf);
		const std::wstring wideBase       = NormalizeWideTooltipLine(Utf8ToWideWithAcpFallback(normBaseNarrow));
		if (!wideBase.empty()) {
			outFullTooltip.push_back(L' ');
			outFullTooltip.append(wideBase);
		}
	}

	// Append item magic/rare/unique/set/runeword affix stats (0x2DA0E0)
	if (g_Native.fnBuildItemStatsString != 0) {
		char statsBuf[1024]{};
		if (SafeCall_BuildItemStatsString(g_Native.fnBuildItemStatsString, itemUnit, statsBuf, sizeof(statsBuf))) {
			statsBuf[sizeof(statsBuf) - 1] = '\0';
			if (statsBuf[0] != '\0') {
				const std::string  normStatsNarrow = NormalizeNarrowAffixText(statsBuf);
				const std::wstring wideStats       = Utf8ToWideWithAcpFallback(normStatsNarrow);
				if (!wideStats.empty()) {
					outFullTooltip.push_back(L' ');
					outFullTooltip.append(wideStats);
				}
			}
		}
	}

	// Append socketable rune/gem/jewel bonus stats (0x2DA310)
	if (g_Native.fnBuildSocketableStatsString != 0 &&
		g_Native.fnGetLocalPlayerSlot != 0 &&
		g_Native.fnGetLocalPlayerUnit != 0) {
		uint32_t  localSlot  = 0;
		uintptr_t playerUnit = 0;
		if (SafeCall_GetU32NoArg(g_Native.fnGetLocalPlayerSlot, &localSlot) &&
			SafeCall_PtrFromU32(g_Native.fnGetLocalPlayerUnit, localSlot, &playerUnit) &&
			playerUnit != 0) {
			char sockBuf[1024]{};
			if (SafeCall_BuildSocketableStatsString(g_Native.fnBuildSocketableStatsString, playerUnit, itemUnit, sockBuf, sizeof(sockBuf))) {
				sockBuf[sizeof(sockBuf) - 1] = '\0';
				if (sockBuf[0] != '\0') {
					const std::string  normSockNarrow = NormalizeNarrowAffixText(sockBuf);
					const std::wstring wideSock       = Utf8ToWideWithAcpFallback(normSockNarrow);
					if (!wideSock.empty()) {
						outFullTooltip.push_back(L' ');
						outFullTooltip.append(wideSock);
					}
				}
			}
		}
	}
}

// 0xE100: Checks whether an item matches the query via boolean & shorthand evaluation on fullTooltip
auto DoesItemMatchQuery(uintptr_t itemUnit, std::wstring_view normalizedQuery, std::wstring_view fullTooltip) -> bool {
	if (EvaluateSearchQuery(normalizedQuery, fullTooltip)) {
		return true;
	}
	char   baseNameBuf[1024]{};
	size_t baseNameLen = 0;
	if (!SafeGetItemBaseDisplayName(itemUnit, baseNameBuf, sizeof(baseNameBuf), &baseNameLen)) {
		return false;
	}
	const std::string  normNarrow = NormalizeNarrowTooltipLine(baseNameBuf);
	const std::wstring wideBase   = Utf8ToWideStrict(normNarrow.c_str());
	const std::wstring normWide   = NormalizeWideTooltipLine(wideBase);
	return EvaluateSearchQuery(normalizedQuery, normWide);
}

// 0xCF40: Measures UTF-16 text using native MeasureStyledText (0x903CD0)
auto MeasureNativeText(
	std::wstring_view text,
	const IntRect&    rect,
	const void*       styleBuf,
	float             scale,
	uint32_t*         outWidth,
	uint32_t*         outHeight) -> bool {
	std::string utf8;
	if (!WideToUtf8Strict(text, utf8)) {
		return false;
	}
	return SafeCall_MeasureStyledText(
		g_Native.fnMeasureStyledText,
		utf8.c_str(),
		styleBuf,
		scale,
		&rect,
		outWidth,
		outHeight);
}

// 0xFBC0: Draws UTF-16 text using native DrawStyledText (0x902E20)
auto DrawNativeText(
	std::wstring_view text,
	const IntRect&    rect,
	const void*       styleBuf,
	float             scale) -> bool {
	std::string utf8;
	if (!WideToUtf8Strict(text, utf8)) {
		return false;
	}
	return SafeCall_DrawStyledText(
		g_Native.fnDrawStyledText,
		utf8.c_str(),
		&rect,
		styleBuf,
		scale);
}

// 0x7D70: Draws filled rectangle (passes left, top, right, bottom to D2R+0x657B90)
void DrawFilledRect(const IntRect& rect, const Color4f& color) noexcept {
	if (!rect.IsValid() || color.a <= 0.0f) {
		return;
	}
	const int64_t right  = static_cast<int64_t>(rect.x) + static_cast<int64_t>(rect.width);
	const int64_t bottom = static_cast<int64_t>(rect.y) + static_cast<int64_t>(rect.height);
	if (right < -2147483648LL || right > 2147483647LL ||
		bottom < -2147483648LL || bottom > 2147483647LL ||
		static_cast<int>(right) <= rect.x ||
		static_cast<int>(bottom) <= rect.y) {
		return;
	}
	SafeCall_DrawFilledRect(
		g_Native.fnDrawFilledRect,
		rect.x,
		rect.y,
		static_cast<int>(right),
		static_cast<int>(bottom),
		&color.r);
}

// 0x7BF0: Draws rectangle border (top, bottom, left, right)
void DrawRectBorder(const IntRect& rect, const Color4f& color, int thickness) noexcept {
	if (!rect.IsValid() || thickness < 1 || color.a <= 0.0f) {
		return;
	}
	const int64_t right  = static_cast<int64_t>(rect.x) + static_cast<int64_t>(rect.width);
	const int64_t bottom = static_cast<int64_t>(rect.y) + static_cast<int64_t>(rect.height);
	if (right < -2147483648LL || right > 2147483647LL ||
		bottom < -2147483648LL || bottom > 2147483647LL ||
		static_cast<int>(right) <= rect.x ||
		static_cast<int>(bottom) <= rect.y) {
		return;
	}
	const int r      = static_cast<int>(right);
	const int b      = static_cast<int>(bottom);
	const int t      = std::min({ thickness, rect.width, rect.height });
	const int topEnd = static_cast<int>(static_cast<int64_t>(rect.y) + static_cast<int64_t>(t));
	const int botBeg = static_cast<int>(bottom - static_cast<int64_t>(t));
	const int lftEnd = static_cast<int>(static_cast<int64_t>(rect.x) + static_cast<int64_t>(t));
	const int rgtBeg = static_cast<int>(right - static_cast<int64_t>(t));

	if (topEnd > rect.y) {
		SafeCall_DrawFilledRect(g_Native.fnDrawFilledRect, rect.x, rect.y, r, topEnd, &color.r);
	}
	if (b > botBeg) {
		SafeCall_DrawFilledRect(g_Native.fnDrawFilledRect, rect.x, botBeg, r, b, &color.r);
	}
	if (lftEnd > rect.x) {
		SafeCall_DrawFilledRect(g_Native.fnDrawFilledRect, rect.x, rect.y, lftEnd, b, &color.r);
	}
	if (r > rgtBeg) {
		SafeCall_DrawFilledRect(g_Native.fnDrawFilledRect, rgtBeg, rect.y, r, b, &color.r);
	}
}

// 0xF2D0: Scales border thickness, minimum 1
auto ScaleBorderThickness(int baseThickness, float scale) noexcept -> int {
	const double prod = static_cast<double>(baseThickness) * static_cast<double>(scale);
	if (std::isfinite(prod) && prod < 2147483647.0) {
		const double r = std::round(prod);
		if (r <= 1.0) {
			return 1;
		}
		if (r < 2147483647.0) {
			return static_cast<int>(r);
		}
	}
	return 0x7FFFFFFF;
}

// 0xF350: Scales positive dimension
auto ScalePositiveDimension(int baseDim, float scale, int* outScaled) noexcept -> bool {
	if (baseDim <= 0 || outScaled == nullptr || !std::isfinite(scale) || scale <= 0.0f) {
		return false;
	}
	const double prod = static_cast<double>(baseDim) * static_cast<double>(scale);
	if (!std::isfinite(prod) || prod <= 0.0 || prod > 2147483647.0) {
		return false;
	}
	const double r = std::round(prod);
	if (!std::isfinite(r) || r <= 0.0 || r > 2147483647.0) {
		return false;
	}
	const auto val = static_cast<int>(static_cast<int64_t>(r));
	if (val <= 0) {
		return false;
	}
	*outScaled = val;
	return true;
}

// 0xF430: Scales positive coordinate
auto ScalePositiveCoord(int baseCoord, float scale, int* outScaled) noexcept -> bool {
	if (outScaled == nullptr) {
		return false;
	}
	const double prod = static_cast<double>(baseCoord) * static_cast<double>(scale);
	if (!std::isfinite(prod) || prod <= 0.0 || prod > 2147483647.0) {
		return false;
	}
	const double r = std::round(prod);
	if (r <= 0.0 || r > 2147483647.0) {
		return false;
	}
	const auto val = static_cast<int>(static_cast<int64_t>(r));
	if (val <= 0) {
		return false;
	}
	*outScaled = val;
	return true;
}

namespace {

auto TrimAscii(std::string_view sv) noexcept -> std::string_view {
	const size_t first = sv.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos) {
		return {};
	}
	const size_t last = sv.find_last_not_of(" \t\r\n");
	return sv.substr(first, last - first + 1);
}

auto TrimAndClampTabName(std::wstring_view input) -> std::wstring {
	size_t first = 0;
	while (first < input.size() && (input[first] == L' ' || input[first] == L'\t' || input[first] == L'\r' || input[first] == L'\n')) {
		++first;
	}
	size_t last = input.size();
	while (last > first && (input[last - 1] == L' ' || input[last - 1] == L'\t' || input[last - 1] == L'\r' || input[last - 1] == L'\n')) {
		--last;
	}
	std::wstring result(input.substr(first, last - first));
	if (result.size() > kMaxTabNameChars) {
		result.resize(kMaxTabNameChars);
		const auto lastUnit = static_cast<uint16_t>(result[kMaxTabNameChars - 1]);
		if (static_cast<uint16_t>(lastUnit - 0xD800u) < 0x0400u) {
			result.resize(kMaxTabNameChars - 1);
		}
	}
	return result;
}

// Finds '#' outside of double-quoted or single-quoted TOML strings
auto FindTomlCommentStart(std::string_view line) noexcept -> size_t {
	bool inDouble = false;
	bool inSingle = false;
	for (size_t i = 0; i < line.size(); ++i) {
		const char c = line[i];
		if (inDouble) {
			if (c == '\\' && i + 1 < line.size()) {
				++i;
			} else if (c == '"') {
				inDouble = false;
			}
		} else if (inSingle) {
			if (c == '\'') {
				inSingle = false;
			}
		} else {
			if (c == '"') {
				inDouble = true;
			} else if (c == '\'') {
				inSingle = true;
			} else if (c == '#') {
				return i;
			}
		}
	}
	return std::string_view::npos;
}

auto ParseTomlStringValue(std::string_view rawVal) -> std::wstring {
	const std::string_view trimmed = TrimAscii(rawVal);
	if (trimmed.empty()) {
		return {};
	}
	std::string unescaped;
	if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
		const std::string_view inner = trimmed.substr(1, trimmed.size() - 2);
		unescaped.reserve(inner.size());
		for (size_t i = 0; i < inner.size(); ++i) {
			if (inner[i] == '\\' && i + 1 < inner.size()) {
				const char next = inner[++i];
				if (next == 't') {
					unescaped.push_back(' ');
				} else if (next != 'r' && next != 'n') {
					unescaped.push_back(next);
				}
			} else {
				unescaped.push_back(inner[i]);
			}
		}
	} else if (trimmed.size() >= 2 && trimmed.front() == '\'' && trimmed.back() == '\'') {
		unescaped.assign(trimmed.substr(1, trimmed.size() - 2));
	} else {
		unescaped.assign(trimmed);
	}
	return TrimAndClampTabName(Utf8ToWideWithAcpFallback(unescaped));
}

auto FormatTomlQuotedString(std::wstring_view wideVal) -> std::string {
	std::string utf8;
	if (!WideToUtf8Strict(wideVal, utf8)) {
		return "\"\"";
	}
	std::string out;
	out.reserve(utf8.size() + 4);
	out.push_back('"');
	for (unsigned char c : utf8) {
		if (c < 0x20u || c == 0x7Fu) {
			continue;
		}
		if (c == '\\' || c == '"') {
			out.push_back('\\');
		}
		out.push_back(static_cast<char>(c));
	}
	out.push_back('"');
	return out;
}

auto ParseSharedTabKeyOneBased(std::string_view key, uint64_t* outZeroBasedIndex) noexcept -> bool {
	if (outZeroBasedIndex == nullptr || key.size() <= 4 || key.substr(0, 4) != "tab_") {
		return false;
	}
	const std::string_view digits = key.substr(4);
	if (digits.empty() || digits.size() > 4) {
		return false;
	}
	uint64_t pageNum = 0;
	for (char c : digits) {
		if (c < '0' || c > '9') {
			return false;
		}
		pageNum = pageNum * 10 + static_cast<uint64_t>(c - '0');
	}
	if (pageNum == 0 || pageNum > 512) {
		return false;
	}
	*outZeroBasedIndex = pageNum - 1;
	return true;
}

auto IsCustomTabNameTomlKey(std::string_view key) noexcept -> bool {
	if (key == "tab_personal") {
		return true;
	}
	uint64_t dummy = 0;
	return ParseSharedTabKeyOneBased(key, &dummy);
}

} // namespace

// Parses `tab_flyout_expanded`, `tab_personal`, and `tab_<N>` custom names from the plugin TOML configuration
auto ParseStashSearchConfig(std::string_view tomlText, bool defaultExpanded) -> StashSearchConfig {
	StashSearchConfig cfg{};
	cfg.tabFlyoutExpanded = defaultExpanded;
	for (size_t cursor = 0; cursor < tomlText.size();) {
		const size_t           newline = tomlText.find('\n', cursor);
		const size_t           end     = (newline == std::string_view::npos) ? tomlText.size() : newline;
		std::string_view       line    = tomlText.substr(cursor, end - cursor);
		if (cursor == 0 && line.size() >= 3 &&
			static_cast<uint8_t>(line[0]) == 0xEF &&
			static_cast<uint8_t>(line[1]) == 0xBB &&
			static_cast<uint8_t>(line[2]) == 0xBF) {
			line.remove_prefix(3);
		}
		const size_t           commentPos = FindTomlCommentStart(line);
		const std::string_view logical    = TrimAscii(line.substr(0, commentPos));
		if (!logical.empty() && logical.front() != '[') {
			const size_t eqPos = logical.find('=');
			if (eqPos != std::string_view::npos) {
				const std::string_view key = TrimAscii(logical.substr(0, eqPos));
				const std::string_view val = TrimAscii(logical.substr(eqPos + 1));
				if (key == "tab_flyout_expanded") {
					if (val == "true" || val == "1") {
						cfg.tabFlyoutExpanded = true;
					} else if (val == "false" || val == "0") {
						cfg.tabFlyoutExpanded = false;
					}
				} else if (key == "tab_personal") {
					cfg.personalTabName = ParseTomlStringValue(val);
				} else {
					uint64_t pageIdx = 0;
					if (ParseSharedTabKeyOneBased(key, &pageIdx)) {
						std::wstring parsedName = ParseTomlStringValue(val);
						bool         found      = false;
						for (auto it = cfg.sharedTabNames.begin(); it != cfg.sharedTabNames.end(); ++it) {
							if (it->first == pageIdx) {
								if (parsedName.empty()) {
									cfg.sharedTabNames.erase(it);
								} else {
									it->second = std::move(parsedName);
								}
								found = true;
								break;
							}
						}
						if (!found && !parsedName.empty()) {
							cfg.sharedTabNames.emplace_back(pageIdx, std::move(parsedName));
						}
					}
				}
			}
		}
		cursor = (newline == std::string_view::npos) ? tomlText.size() : (newline + 1);
	}
	std::sort(
		cfg.sharedTabNames.begin(),
		cfg.sharedTabNames.end(),
		[](const auto& a, const auto& b) noexcept { return a.first < b.first; });
	return cfg;
}

// Serializes `tab_flyout_expanded` and custom tab names into the plugin TOML configuration while preserving comments
auto SerializeStashSearchConfig(std::string_view existingToml, const StashSearchConfig& cfg) -> std::string {
	const std::string_view valStr = cfg.tabFlyoutExpanded ? "true" : "false";
	const std::string_view eol    = (existingToml.find("\r\n") != std::string_view::npos) ? "\r\n" : "\n";

	std::string output;
	output.reserve(existingToml.size() + 256);
	bool replacedExpanded = false;

	if (TrimAscii(existingToml).empty()) {
		output.append("# Pyre Stash Search configuration");
		output.append(eol);
		output.append("[pyre-stashsearch]");
		output.append(eol);
		output.append("# Keep the side tab flyout expanded across stash opens and game launches.");
		output.append(eol);
		output.append("tab_flyout_expanded = ");
		output.append(valStr);
		output.append(eol);
		replacedExpanded = true;
	} else {
		for (size_t cursor = 0; cursor < existingToml.size();) {
			const size_t           newline = existingToml.find('\n', cursor);
			const size_t           end     = (newline == std::string_view::npos) ? existingToml.size() : newline;
			const std::string_view rawLine = existingToml.substr(cursor, end - cursor);

			std::string_view lineNoBom = rawLine;
			size_t           bomLen    = 0;
			if (cursor == 0 && lineNoBom.size() >= 3 &&
				static_cast<uint8_t>(lineNoBom[0]) == 0xEF &&
				static_cast<uint8_t>(lineNoBom[1]) == 0xBB &&
				static_cast<uint8_t>(lineNoBom[2]) == 0xBF) {
				bomLen = 3;
				lineNoBom.remove_prefix(3);
			}

			const size_t           commentPos = FindTomlCommentStart(lineNoBom);
			const std::string_view codePart   = lineNoBom.substr(0, commentPos);
			const size_t           eqPos      = codePart.find('=');

			bool skipLine = false;
			if (eqPos != std::string_view::npos) {
				const std::string_view key = TrimAscii(codePart.substr(0, eqPos));
				if (IsCustomTabNameTomlKey(key)) {
					skipLine = true;
				} else if (!replacedExpanded && key == "tab_flyout_expanded") {
					size_t valStart = eqPos + 1;
					size_t valEnd   = (commentPos == std::string_view::npos) ? lineNoBom.size() : commentPos;
					while (valStart < valEnd && (lineNoBom[valStart] == ' ' || lineNoBom[valStart] == '\t')) {
						++valStart;
					}
					while (valEnd > valStart && (lineNoBom[valEnd - 1] == ' ' || lineNoBom[valEnd - 1] == '\t' || lineNoBom[valEnd - 1] == '\r')) {
						--valEnd;
					}
					output.append(rawLine.substr(0, bomLen + valStart));
					output.append(valStr);
					output.append(rawLine.substr(bomLen + valEnd));
					if (newline != std::string_view::npos) {
						output.push_back('\n');
					}
					replacedExpanded = true;
					skipLine         = true;
				}
			}

			if (!skipLine) {
				output.append(rawLine);
				if (newline != std::string_view::npos) {
					output.push_back('\n');
				}
			}
			cursor = (newline == std::string_view::npos) ? existingToml.size() : (newline + 1);
		}
	}

	if (!replacedExpanded) {
		if (!output.empty() && output.back() != '\n') {
			output.append(eol);
		}
		if (output.find("[pyre-stashsearch]") == std::string::npos &&
			output.find("[stash_search]") == std::string::npos &&
			output.find("[stash-search]") == std::string::npos) {
			output.append("[pyre-stashsearch]");
			output.append(eol);
		}
		output.append("tab_flyout_expanded = ");
		output.append(valStr);
		output.append(eol);
	}

	const std::wstring trimmedPersonal = TrimAndClampTabName(cfg.personalTabName);
	if (!trimmedPersonal.empty()) {
		if (!output.empty() && output.back() != '\n') {
			output.append(eol);
		}
		output.append("tab_personal = ");
		output.append(FormatTomlQuotedString(trimmedPersonal));
		output.append(eol);
	}

	std::vector<std::pair<uint64_t, std::wstring>> sortedShared;
	sortedShared.reserve(cfg.sharedTabNames.size());
	for (const auto& [pageIdx, rawName] : cfg.sharedTabNames) {
		if (pageIdx < 512) {
			std::wstring trimmed = TrimAndClampTabName(rawName);
			if (!trimmed.empty()) {
				sortedShared.emplace_back(pageIdx, std::move(trimmed));
			}
		}
	}
	std::sort(
		sortedShared.begin(),
		sortedShared.end(),
		[](const auto& a, const auto& b) noexcept { return a.first < b.first; });

	for (const auto& [pageIdx, name] : sortedShared) {
		if (!output.empty() && output.back() != '\n') {
			output.append(eol);
		}
		output.append("tab_");
		output.append(std::to_string(pageIdx + 1));
		output.append(" = ");
		output.append(FormatTomlQuotedString(name));
		output.append(eol);
	}

	return output;
}

// Parses `tab_flyout_expanded = true|false` from the plugin TOML configuration
auto ParseTabFlyoutExpandedConfig(std::string_view tomlText, bool defaultValue) noexcept -> bool {
	return ParseStashSearchConfig(tomlText, defaultValue).tabFlyoutExpanded;
}

// Updates or inserts `tab_flyout_expanded = true|false` in the plugin TOML configuration while preserving comments and custom tab names
auto UpdateTabFlyoutExpandedConfig(std::string_view tomlText, bool expanded) -> std::string {
	StashSearchConfig cfg = ParseStashSearchConfig(tomlText, expanded);
	cfg.tabFlyoutExpanded = expanded;
	return SerializeStashSearchConfig(tomlText, cfg);
}

} // namespace StashSearch
