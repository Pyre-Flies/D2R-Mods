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
};

struct MatchEntry {
	std::wstring    displayText;      // Shortened dropdown label (e.g. "Devourer of Dreams - Sh.5")
	uint64_t        count = 0;
	StashPageTarget target{};
	bool            hasTarget = false;
	std::wstring    baseLabel;        // Shortened deduplication label
	std::wstring    hoverText;        // Full unabbreviated label for hover tooltip
	std::wstring    shortTabSuffix;   // Right-side tab suffix (e.g. " - Sh.5" or " - P") preserved on ellipsis truncation
	uint64_t        reserved = 0;
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
	bool     valid               = false;
	float    scale               = 0.0f;
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
auto EvaluateSearchQuery(std::wstring_view query, std::wstring_view haystack) -> bool;
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
	std::wstring                                   personalTabName;
	std::vector<std::pair<uint64_t, std::wstring>> sharedTabNames; // (0-based shared page index, custom label)
};

auto ParseStashSearchConfig(std::string_view tomlText, bool defaultExpanded = false) -> StashSearchConfig;
auto SerializeStashSearchConfig(std::string_view existingToml, const StashSearchConfig& cfg) -> std::string;
auto ParseTabFlyoutExpandedConfig(std::string_view tomlText, bool defaultValue = false) noexcept -> bool;
auto UpdateTabFlyoutExpandedConfig(std::string_view tomlText, bool expanded) -> std::string;

} // namespace StashSearch
