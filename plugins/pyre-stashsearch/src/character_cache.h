#pragma once

// Platform-independent helpers for the cross-character item snapshot cache.
// Nothing in this header touches game memory or Win32, so it is unit-tested on
// every platform. plugin.cpp owns the native walk and the file I/O.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace StashSearch {

inline constexpr uint32_t kCharacterSnapshotFormatVersion = 1;
inline constexpr size_t   kMaxCharacterNameChars          = 15;
inline constexpr size_t   kMaxSnapshotItems               = 2048;
inline constexpr size_t   kMaxSnapshotFieldChars          = 4096;
inline constexpr size_t   kMaxSnapshotFileBytes           = 8u * 1024u * 1024u;

// Native inventory page byte returned by Item_GetLocationPage (D2R.exe+0x36CFE0).
inline constexpr uint8_t kNativePageInventory = 0;
inline constexpr uint8_t kNativePageCube      = 3;
inline constexpr uint8_t kNativePageStash     = 4;
inline constexpr uint8_t kNativePageNone      = 0xFF;

// Native item animation mode (UnitAny::dwAnimMode at unit + 0x0C).
inline constexpr uint32_t kItemModeStored   = 0;
inline constexpr uint32_t kItemModeEquipped = 1;
inline constexpr uint32_t kItemModeBelt     = 2;
inline constexpr uint32_t kItemModeGround   = 3;
inline constexpr uint32_t kItemModeCursor   = 4;
inline constexpr uint32_t kItemModeDropping = 5;
inline constexpr uint32_t kItemModeSocketed = 6;

// Where a character's item lives. Values are stable because the cache file
// stores them as text codes (see ItemSourceCode).
enum class ItemSource : uint8_t {
	PersonalStash = 0,
	Inventory     = 1,
	Cube          = 2,
	Equipped      = 3,
	Belt          = 4,
	Cursor        = 5,
	ExtraPage     = 6, // D2RLoader custom player pages (native page 5..254)
};

// Maps (native page, item mode) to a searchable location. Returns false for
// items that should never be listed on their own: ground, dropping, socketed
// (their stats already appear on the host item) and unknown combinations.
auto ClassifyCharacterItem(uint8_t nativePage, uint32_t itemMode, ItemSource* outSource) noexcept -> bool;

auto ItemSourceCode(ItemSource source) noexcept -> std::string_view;          // "stash", "inv", ...
auto ParseItemSourceCode(std::string_view code, ItemSource* outSource) noexcept -> bool;
auto ItemSourceShortLabel(ItemSource source) noexcept -> std::wstring_view;   // "P", "Inv", "Cube", "Worn", ...
auto ItemSourceFullLabel(ItemSource source) noexcept -> std::wstring_view;    // "Personal", "Inventory", ...

struct CachedCharacterItem {
	ItemSource   source = ItemSource::Inventory;
	std::wstring shortName;   // Dropdown name ("Harlequin Crest")
	std::wstring fullName;    // Hover name line ("Shako Harlequin Crest")
	std::wstring searchText;  // Normalized full tooltip used for matching
};

struct CharacterSnapshot {
	std::wstring                     name;
	uint64_t                         savedUnixSeconds = 0;
	std::vector<CachedCharacterItem> items;
};

// D2R names are 2..15 characters. This also rejects anything that is unsafe as
// a Windows file name so the name can be used directly as the cache file stem.
auto IsValidCharacterName(std::wstring_view name) noexcept -> bool;

auto SerializeCharacterSnapshot(const CharacterSnapshot& snapshot) -> std::string;
auto ParseCharacterSnapshot(std::string_view text, CharacterSnapshot& outSnapshot) -> bool;

// "just now", "5 min ago", "3 h ago", "2 days ago"
auto FormatSnapshotAge(uint64_t nowUnixSeconds, uint64_t savedUnixSeconds) -> std::wstring;

// Case-insensitive (ASCII) equality used to exclude the logged-in character.
auto CharacterNamesEqual(std::wstring_view a, std::wstring_view b) noexcept -> bool;

// UTF-8 <-> wide helpers that behave the same on Windows (UTF-16 wchar_t) and
// on test hosts (UTF-32 wchar_t). Invalid input is replaced with U+FFFD.
auto CacheUtf8ToWide(std::string_view utf8) -> std::wstring;
auto CacheWideToUtf8(std::wstring_view wide) -> std::string;

} // namespace StashSearch
