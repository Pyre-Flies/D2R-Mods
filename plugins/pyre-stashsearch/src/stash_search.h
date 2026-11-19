#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace StashSearch {

inline constexpr size_t kMaxQueryChars            = 64;
inline constexpr size_t kMaxTabNameChars          = 24;
inline constexpr size_t kMaxMatches               = 4096;
inline constexpr size_t kMaxDropdownVisibleRows   = 6;
inline constexpr size_t kMaxTooltipDisplayLines   = 20;
inline constexpr size_t kPagePickerParentTabRows  = 4; // Personal, Gems, Materials, Runes
inline constexpr size_t kMaxPageGridVisibleRows   = 18;
inline constexpr size_t kMaxPageGridVisibleCells  = kPagePickerParentTabRows + kMaxPageGridVisibleRows;
inline constexpr int    kItemLocationStashPage    = 4;

inline constexpr UINT kWorkerMsgSyncEditFocus   = 0x8001;
inline constexpr UINT kWorkerMsgUnfocusHideEdit = 0x8002;
inline constexpr UINT kWorkerMsgDestroyEdit     = 0x8003;
inline constexpr UINT kWorkerMsgShutdownThread  = 0x8004;
inline constexpr UINT_PTR kWorkerTimerId        = 1;
inline constexpr UINT     kWorkerTimerIntervalMs = 100;

struct IntRect {
	int x      = 0;
	int y      = 0;
	int width  = 0;
	int height = 0;

	[[nodiscard]] constexpr auto IsValid() const noexcept -> bool {
		return width > 0 && height > 0;
	}

	[[nodiscard]] constexpr auto Contains(int px, int py) const noexcept -> bool {
		if (!IsValid()) {
			return false;
		}
		const int64_t right  = static_cast<int64_t>(x) + static_cast<int64_t>(width);
		const int64_t bottom = static_cast<int64_t>(y) + static_cast<int64_t>(height);
		return px >= x && static_cast<int64_t>(px) < right && py >= y && static_cast<int64_t>(py) < bottom;
	}
};

struct Color4f {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
};

// RGBA float4 palette from StashSearch.dll .rdata (0x144A8..0x14518, 0x14B2C)
inline constexpr Color4f kColorSearchBoxBg       = { 0.025f, 0.022f, 0.018f, 0.94f };
inline constexpr Color4f kColorDropdownRowBg     = { 0.018f, 0.016f, 0.014f, 0.97f };
inline constexpr Color4f kColorDropdownRowSelBg  = { 0.34f,  0.27f,  0.15f,  0.96f };
inline constexpr Color4f kColorFocusedBorder     = { 0.82f,  0.62f,  0.22f,  1.00f };
inline constexpr Color4f kColorUnfocusedBorder   = { 0.19f,  0.13f,  0.055f, 0.98f };
inline constexpr Color4f kColorScrollbarTrack    = { 0.055f, 0.045f, 0.032f, 0.98f };
inline constexpr Color4f kColorScrollbarThumb    = { 0.50f,  0.37f,  0.14f,  0.98f };
inline constexpr Color4f kColorGridDimOverlay    = { 0.00f,  0.00f,  0.00f,  0.60f };
inline constexpr Color4f kColorTextSelectionBg   = { 0.52f,  0.40f,  0.14f,  0.80f };
inline constexpr Color4f kColorMatchEdgeOuter    = { 1.00f,  0.82f,  0.22f,  0.98f };
inline constexpr Color4f kColorMatchEdgeInner    = { 1.00f,  0.95f,  0.60f,  0.90f };
inline constexpr Color4f kColorPageBtnHoverBg    = { 0.22f,  0.17f,  0.09f,  0.95f };

struct StashPageTarget {
	bool     isShared  = false;
	uint64_t pageIndex = 0;

	[[nodiscard]] constexpr auto operator==(const StashPageTarget& other) const noexcept -> bool {
		return isShared == other.isShared && pageIndex == other.pageIndex;
	}
};

enum class DuplicateKeyKind : uint8_t {
	None         = 0,
	UniqueOrSet  = 1,
	Runeword     = 2,
	BaseItem     = 3,
	NamedQuality = 4,
};

struct ItemDuplicateKey {
	DuplicateKeyKind kind      = DuplicateKeyKind::None;
	int32_t          quality   = 0;
	uint32_t         primaryId = 0;
	std::wstring     foldedName;

	[[nodiscard]] constexpr auto IsValid() const noexcept -> bool {
		return kind != DuplicateKeyKind::None;
	}

	[[nodiscard]] auto operator==(const ItemDuplicateKey& other) const noexcept -> bool {
		return kind == other.kind &&
			   quality == other.quality &&
			   primaryId == other.primaryId &&
			   foldedName == other.foldedName;
	}
};

struct ItemDuplicateKeyHash {
	[[nodiscard]] auto operator()(const ItemDuplicateKey& key) const noexcept -> size_t {
		uint64_t h = static_cast<uint64_t>(key.kind) * 0x9E3779B97F4A7C15ULL;
		h ^= static_cast<uint64_t>(static_cast<uint32_t>(key.quality)) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
		h ^= static_cast<uint64_t>(key.primaryId) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
		if (!key.foldedName.empty()) {
			const size_t nameHash = std::hash<std::wstring_view>{}(key.foldedName);
			h ^= static_cast<uint64_t>(nameHash) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
		}
		return static_cast<size_t>(h);
	}
};

struct DuplicateQueryFlags {
	bool usesDuplicateTerm        = false;
	bool hasPositiveDuplicateTerm = false;
	bool hasNegativeDuplicateTerm = false;
};

struct MatchEntry {
	std::wstring              displayText;      // Shortened dropdown label (e.g. "Devourer of Dreams - Sh.5")
	uint64_t                  count = 0;
	StashPageTarget           target{};
	bool                      hasTarget = false;
	std::wstring              shortName;        // Concise item display name on the target page (e.g. "Devourer of Dreams")
	std::wstring              fullTooltip;      // Normalized full searchable tooltip for exact item/affix match isolation
	std::wstring              baseLabel;        // Shortened deduplication label
	std::wstring              hoverText;        // Full unabbreviated label for hover tooltip
	std::wstring              shortTabSuffix;   // Right-side tab suffix (e.g. " - Sh.5" or " - P") preserved on ellipsis truncation
	std::vector<std::wstring> tooltipLines;     // Multi-line formatted item tooltip lines (title + base type + affixes/stats)
	size_t                    headerLineCount         = 1; // Number of header lines (item name + optional base type) at start of tooltipLines
	ItemDuplicateKey          duplicateKey{};
	bool                      matchedWithoutDuplicate = false;
	std::vector<uintptr_t>    matchedUnits;
	bool                      displayOnly             = false; // Item row with no stash page to jump to (inventory, cube, worn, other characters)
	bool                      snapshotRow             = false; // Display-only row read from another character's snapshot file
	uint64_t                  reserved = 0;
};

enum class SlashCommandState : uint8_t {
	None          = 0, // Not a slash command
	MissingNumber = 1, // Just "/" -> "Enter shared page number"
	InvalidNumber = 2, // "/0" or "/abc" -> "Invalid page number"
	ValidPage     = 3, // "/<N>" (N >= 1) -> "Press Enter to jump to shared page <N>"
};

struct SlashPageCommand {
	SlashCommandState state      = SlashCommandState::None;
	uint64_t          pageNumber = 0; // 1-based shared page number when state == ValidPage
};

struct SearchBarLayout {
	bool     valid                 = false;
	bool     collapsedToButtonOnly = false;
	float    scale                 = 0.0f;
	IntRect  searchBox{};
	IntRect  searchTextRect{};
	IntRect  pageButtonRect{};
	IntRect  dropdownRows[kMaxDropdownVisibleRows]{};
	IntRect  dropdownTextRects[kMaxDropdownVisibleRows]{};
	uint64_t dropdownMatchIndices[kMaxDropdownVisibleRows]{};
	size_t   visibleRowCount     = 0;
	IntRect  dropdownBox{};
	IntRect  scrollbarTrack{};
	IntRect  scrollbarThumb{};
	float    dropdownScale       = 0.0f;
	size_t   clampedScrollOffset = 0;
	bool     hasScrollbar        = false;

	// PoE-style side tab flyout layout (docked to the right outer edge of the Stash panel)
	bool     pagePickerOpen       = false;
	IntRect  pagePickerBox{};
	IntRect  pagePickerDividerRect{};
	IntRect  pagePickerScrollbarTrack{};
	IntRect  pagePickerScrollbarThumb{};
	bool     pagePickerHasScrollbar = false;
	IntRect  pageCellRects[kMaxPageGridVisibleCells]{};
	uint64_t pageCellIndices[kMaxPageGridVisibleCells]{};  // 0-based shared page index (or parent tab index when !pageCellIsShared[i])
	bool     pageCellIsShared[kMaxPageGridVisibleCells]{}; // true = Shared subpage, false = Parent tab (0=Personal, 2=Gems, 3=Materials, 4=Runes)
	size_t   pageCellCount        = 0;
	size_t   pagePickerTotalPages = 0;
	size_t   pagePickerScrollRow  = 0;
	size_t   pagePickerTotalRows  = 0;
	size_t   pagePickerVisRows    = 0;
};

// Pure string & command helpers (implemented in native.cpp, tested in unit_tests.cpp)
auto Utf8ToWideWithAcpFallback(std::string_view input) -> std::wstring;
auto Utf8ToWideStrict(const char* input) -> std::wstring;
auto WideToUtf8Strict(std::wstring_view input, std::string& outUtf8) -> bool;
auto NormalizeNarrowTooltipLine(const char* raw) -> std::string;
auto NormalizeNarrowAffixText(const char* raw) -> std::string;
auto NormalizeWideTooltipLine(std::wstring_view raw) -> std::wstring;
auto ExtractShortItemDisplayName(std::string_view rawNarrow) -> std::wstring;
auto FoldStringLowerInvariant(std::wstring_view input) -> std::wstring;
auto ContainsCaseInsensitive(std::wstring_view needle, std::wstring_view haystack) -> bool;
auto EvaluateSearchQuery(std::wstring_view query, std::wstring_view haystack, bool isDuplicate = false) -> bool;
auto InspectDuplicateQueryTerms(std::wstring_view query) -> DuplicateQueryFlags;
auto IsCharmOrJewelItemCode(std::string_view itemCode) noexcept -> bool;
auto BuildItemDuplicateKey(
	int32_t           quality,
	uint32_t          flags,
	uint32_t          classId,
	uint32_t          fileIndex,
	std::wstring_view shortName        = {},
	std::string_view  itemCode         = {},
	bool              hasSocketedItems = false) -> ItemDuplicateKey;
void PruneSingletonDuplicateMatches(
	std::vector<MatchEntry>& entries,
	std::vector<uintptr_t>*  outPrunedUnits = nullptr);
void SortDuplicateMatchEntries(std::vector<MatchEntry>& entries);
auto ParseSlashPageCommand(std::wstring_view query) noexcept -> SlashPageCommand;
auto FormatSlashPagePrompt(const SlashPageCommand& cmd) -> std::wstring;
void AdjustDropdownSelectionAndScroll(
	size_t  matchCount,
	size_t  visibleCapacity,
	size_t  desiredIndex,
	size_t& inOutSelectedIndex,
	size_t& inOutScrollOffset) noexcept;
struct StashSearchConfig {
	bool                                           tabFlyoutExpanded = false;
	bool                                           searchCarriedItems    = true; // search_carried_items (read-only key)
	bool                                           searchOtherCharacters = true; // search_other_characters (read-only key)
	std::wstring                                   personalTabName;
	std::vector<std::pair<uint64_t, std::wstring>> sharedTabNames; // (0-based shared page index, custom label)
};

auto ParseStashSearchConfig(std::string_view tomlText, bool defaultExpanded = false) -> StashSearchConfig;
auto SerializeStashSearchConfig(std::string_view existingToml, const StashSearchConfig& cfg) -> std::string;
auto ParseTabFlyoutExpandedConfig(std::string_view tomlText, bool defaultValue = false) noexcept -> bool;
auto UpdateTabFlyoutExpandedConfig(std::string_view tomlText, bool expanded) -> std::string;
auto BuildItemTooltipLines(
	std::string_view  rawDescNarrow,
	std::string_view  rawBaseNameNarrow,
	std::string_view  rawStatsNarrow,
	std::string_view  rawSockNarrow,
	std::wstring_view fullTabSuffix,
	size_t*           outHeaderLineCount = nullptr) -> std::vector<std::wstring>;
auto DoesItemMatchActiveFilter(
	bool                   queryMatched,
	std::wstring_view      itemShortName,
	const StashPageTarget& currentPage,
	bool                   lockedMatchActive,
	const StashPageTarget& lockedMatchTarget,
	std::wstring_view      lockedMatchShortName,
	std::wstring_view      itemFullTooltip        = {},
	std::wstring_view      lockedMatchFullTooltip = {}) noexcept -> bool;
auto IsPointInActiveSearchOverlay(
	const SearchBarLayout& layout,
	bool                   showSearchDropdown,
	int                    clientX,
	int                    clientY) noexcept -> bool;

[[nodiscard]] constexpr auto ShouldPointFlyoutChevronLeft(
	bool pagePickerOpen,
	bool isControllerLayout = false) noexcept -> bool {
	(void)isControllerLayout;
	return pagePickerOpen;
}

struct ControllerFlyoutCursorState {
	POINT   savedPreFlyoutCursorPt{ -1, -1 };
	bool    pendingFlyoutFocus        = false;
	bool    pendingCursorRestore      = false;
	bool    overlayAPressLatched      = false;
	uint8_t overlayAPostReleaseFrames = 0;
};

void ResetControllerFlyoutCursorState(ControllerFlyoutCursorState& state) noexcept;
void LatchControllerOverlayAPress(ControllerFlyoutCursorState& state) noexcept;
auto UpdateControllerOverlayAPressLatch(ControllerFlyoutCursorState& state, bool aDown) noexcept -> bool;
void TrackControllerPreFlyoutCursor(
	ControllerFlyoutCursorState& state,
	const SearchBarLayout&       layout,
	POINT                        cursorPt) noexcept;
void OnControllerFlyoutToggled(
	ControllerFlyoutCursorState& state,
	const SearchBarLayout&       layout,
	bool                         newPagePickerOpen,
	POINT                        cursorPt) noexcept;
auto ConsumeControllerFlyoutFocusTarget(
	ControllerFlyoutCursorState& state,
	const SearchBarLayout&       layout,
	uint8_t                      activeParentTab,
	int64_t                      activeSharedPage,
	POINT*                       outTargetPt) noexcept -> bool;
auto ConsumeControllerFlyoutCursorRestore(
	ControllerFlyoutCursorState& state,
	const SearchBarLayout&       layout,
	POINT*                       outRestorePt) noexcept -> bool;

struct OccludedCursorSuppressionDecision {
	bool clearHoveredWidget = false;
	bool clearFocusedWidget = false;
};

[[nodiscard]] constexpr auto ComputeOccludedCursorSuppression(
	uintptr_t expectedBankPanel,
	uintptr_t coOpenInventoryPanel,
	bool      overFlyoutOrLatched,
	uintptr_t hoveredPanel,
	uintptr_t focusedPanel) noexcept -> OccludedCursorSuppressionDecision {
	if (expectedBankPanel == 0) {
		return {};
	}
	const bool allowInventory = overFlyoutOrLatched && (coOpenInventoryPanel != 0);
	return {
		.clearHoveredWidget = (hoveredPanel == expectedBankPanel) || (allowInventory && hoveredPanel == coOpenInventoryPanel),
		.clearFocusedWidget = (focusedPanel == expectedBankPanel) || (allowInventory && focusedPanel == coOpenInventoryPanel),
	};
}

struct XInputGamepadState {
	uint16_t wButtons      = 0;
	uint8_t  bLeftTrigger  = 0;
	uint8_t  bRightTrigger = 0;
	int16_t  sThumbLX      = 0;
	int16_t  sThumbLY      = 0;
	int16_t  sThumbRX      = 0;
	int16_t  sThumbRY      = 0;
};

struct XInputControllerState {
	uint32_t           dwPacketNumber = 0;
	XInputGamepadState Gamepad{};
};

using XInputGetStateFn = uint32_t(WINAPI*)(uint32_t dwUserIndex, XInputControllerState* pState);

inline constexpr uint32_t kXInputMaxControllers       = 4;
inline constexpr uint64_t kXInputDisconnectedProbeMs  = 1000;
inline constexpr uint16_t kXInputGamepadLeftShoulder  = 0x0100;
inline constexpr uint16_t kXInputGamepadRightShoulder = 0x0200;
inline constexpr uint16_t kXInputGamepadA             = 0x1000;
inline constexpr uint8_t  kControllerTriggerThreshold = 30;
inline constexpr int      kControllerStickDeadzone    = 12000;

struct XInputPollCache {
	uint64_t lastProbeTickMs                      = 0;
	uint8_t  connectedMask                        = 0;
	uint8_t  activeSlot                           = 0xFF;
	bool     hasEverProbed                        = false;
	uint32_t lastPacketNumbers[kXInputMaxControllers]{};
	bool     hasPacketNumbers[kXInputMaxControllers]{};
};

[[nodiscard]] constexpr auto HasXInputGamepadActivity(const XInputGamepadState& pad) noexcept -> bool {
	if (pad.wButtons != 0 ||
		pad.bLeftTrigger > kControllerTriggerThreshold ||
		pad.bRightTrigger > kControllerTriggerThreshold) {
		return true;
	}
	const int lx = (pad.sThumbLX < 0) ? -static_cast<int>(pad.sThumbLX) : static_cast<int>(pad.sThumbLX);
	const int ly = (pad.sThumbLY < 0) ? -static_cast<int>(pad.sThumbLY) : static_cast<int>(pad.sThumbLY);
	const int rx = (pad.sThumbRX < 0) ? -static_cast<int>(pad.sThumbRX) : static_cast<int>(pad.sThumbRX);
	const int ry = (pad.sThumbRY < 0) ? -static_cast<int>(pad.sThumbRY) : static_cast<int>(pad.sThumbRY);
	return lx > kControllerStickDeadzone ||
		   ly > kControllerStickDeadzone ||
		   rx > kControllerStickDeadzone ||
		   ry > kControllerStickDeadzone;
}

auto PollActiveXInputState(
	XInputGetStateFn       fnXInput,
	uint64_t               nowTickMs,
	XInputPollCache&       cache,
	XInputControllerState* outState) noexcept -> bool;

} // namespace StashSearch
