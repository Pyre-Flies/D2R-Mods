// Platform-independent tests for the cross-character snapshot cache.
// Builds with MSVC (via CMake) and with any C++20 compiler on its own:
//   g++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/character_cache_tests.cpp src/character_cache.cpp
#include "character_cache.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

void Check(bool condition, const char* message) {
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

using namespace StashSearch;

void TestClassification() {
	ItemSource s{};
	Check(ClassifyCharacterItem(0, kItemModeStored, &s) && s == ItemSource::Inventory, "page 0 stored is inventory");
	Check(ClassifyCharacterItem(3, kItemModeStored, &s) && s == ItemSource::Cube, "page 3 stored is cube");
	Check(ClassifyCharacterItem(4, kItemModeStored, &s) && s == ItemSource::PersonalStash, "page 4 stored is personal stash");
	Check(ClassifyCharacterItem(6, kItemModeStored, &s) && s == ItemSource::ExtraPage, "page 6 stored is a custom page");
	Check(!ClassifyCharacterItem(1, kItemModeStored, &s), "page 1 stored is skipped");
	Check(!ClassifyCharacterItem(2, kItemModeStored, &s), "page 2 (trade) stored is skipped");
	Check(!ClassifyCharacterItem(0xFF, kItemModeStored, &s), "no-page stored is skipped");
	Check(ClassifyCharacterItem(0xFF, kItemModeEquipped, &s) && s == ItemSource::Equipped, "equipped mode");
	Check(ClassifyCharacterItem(0xFF, kItemModeBelt, &s) && s == ItemSource::Belt, "belt mode");
	Check(ClassifyCharacterItem(0xFF, kItemModeCursor, &s) && s == ItemSource::Cursor, "cursor mode");
	Check(!ClassifyCharacterItem(0xFF, kItemModeSocketed, &s), "socketed children are skipped");
	Check(!ClassifyCharacterItem(0, kItemModeGround, &s), "ground items are skipped");
	Check(!ClassifyCharacterItem(0, kItemModeDropping, &s), "dropping items are skipped");
	Check(!ClassifyCharacterItem(0, 99, &s), "unknown mode is skipped");
	Check(!ClassifyCharacterItem(0, kItemModeStored, nullptr), "null output is rejected");
}

void TestSourceCodes() {
	const ItemSource all[] = { ItemSource::PersonalStash, ItemSource::Inventory, ItemSource::Cube, ItemSource::Equipped,
		ItemSource::Belt, ItemSource::Cursor, ItemSource::ExtraPage };
	for (const ItemSource s : all) {
		ItemSource parsed{};
		Check(ParseItemSourceCode(ItemSourceCode(s), &parsed) && parsed == s, "source code round-trips");
		Check(!ItemSourceShortLabel(s).empty() && !ItemSourceFullLabel(s).empty(), "source labels are present");
	}
	ItemSource parsed{};
	Check(!ParseItemSourceCode("merc", &parsed), "unknown source code is rejected");
}

void TestNames() {
	Check(IsValidCharacterName(L"MuleSorc"), "plain name");
	Check(IsValidCharacterName(L"Bul-Kathos_two"), "dash and underscore");
	Check(IsValidCharacterName(L"\x00C9lodie"), "non-ASCII letter");
	Check(!IsValidCharacterName(L"A"), "too short");
	Check(!IsValidCharacterName(L"ThisNameIsTooLong"), "too long");
	Check(!IsValidCharacterName(L"..\\evil"), "path traversal");
	Check(!IsValidCharacterName(L"a/b"), "slash");
	Check(!IsValidCharacterName(L"con:"), "colon");
	Check(!IsValidCharacterName(L"tab\tname"), "control char");
	Check(!IsValidCharacterName(L""), "empty");
	Check(CharacterNamesEqual(L"MuleSorc", L"mulesorc"), "ASCII case-insensitive equality");
	Check(!CharacterNamesEqual(L"MuleSorc", L"MuleSorc2"), "different names");
}

void TestRoundTrip() {
	CharacterSnapshot snap{};
	snap.name             = L"MuleSorc";
	snap.savedUnixSeconds = 1763000000;
	snap.items.push_back({ ItemSource::Cube, L"Harlequin Crest", L"Shako Harlequin Crest",
		L"Harlequin Crest Shako +2 To All Skills [qual:unique] Socketed (1)" });
	snap.items.push_back({ ItemSource::Equipped, L"Tab\tName", L"Back\\slash\nNew line\rCR",
		L"Spirit \x00D7 Rune\x00E9 \U0001F525" });
	snap.items.push_back({ ItemSource::Belt, L"", L"", L"Full Rejuvenation Potion" });

	const std::string text = SerializeCharacterSnapshot(snap);
	Check(text.find("version\t1\n") != std::string::npos, "version line written");
	// Each record must stay on one line despite embedded tabs/newlines.
	size_t lines = 0;
	for (const char ch : text) {
		lines += ch == '\n' ? 1 : 0;
	}
	Check(lines == 4 + snap.items.size(), "one line per header and item");

	CharacterSnapshot parsed{};
	Check(ParseCharacterSnapshot(text, parsed), "round-trip parse succeeds");
	Check(parsed.name == snap.name, "name round-trips");
	Check(parsed.savedUnixSeconds == snap.savedUnixSeconds, "timestamp round-trips");
	Check(parsed.items.size() == snap.items.size(), "item count round-trips");
	for (size_t i = 0; i < snap.items.size(); ++i) {
		Check(parsed.items[i].source == snap.items[i].source, "item source round-trips");
		Check(parsed.items[i].shortName == snap.items[i].shortName, "item short name round-trips");
		Check(parsed.items[i].fullName == snap.items[i].fullName, "item full name round-trips (escapes)");
		Check(parsed.items[i].searchText == snap.items[i].searchText, "item search text round-trips (unicode)");
	}

	// CRLF line endings and a UTF-8 BOM (e.g. after hand-editing in Notepad).
	std::string crlf = "\xEF\xBB\xBF";
	for (const char ch : text) {
		if (ch == '\n') {
			crlf.push_back('\r');
		}
		crlf.push_back(ch);
	}
	Check(ParseCharacterSnapshot(crlf, parsed) && parsed.items.size() == snap.items.size(), "CRLF + BOM parse");
}

void TestRejects() {
	CharacterSnapshot parsed{};
	Check(!ParseCharacterSnapshot("", parsed), "empty file rejected");
	Check(!ParseCharacterSnapshot("version\t2\nname\tMule\n", parsed), "future version rejected");
	Check(!ParseCharacterSnapshot("name\tMule\n", parsed), "missing version rejected");
	Check(!ParseCharacterSnapshot("version\t1\n", parsed), "missing name rejected");
	Check(!ParseCharacterSnapshot("version\t1\nname\t..\\x\n", parsed), "unsafe name rejected");
	Check(!ParseCharacterSnapshot("version\t1\nname\tMule\\q\n", parsed), "bad escape in name rejected");

	const char* mixed =
		"version\t1\n"
		"name\tMule\n"
		"saved\t5\n"
		"item\tinv\tA\tA\tGood One\n"
		"item\tmerc\tB\tB\tunknown source\n"
		"item\tinv\tC\tC\n"
		"item\tinv\tD\tD\tbad \\q escape\n"
		"item\tinv\tE\tE\t\n"
		"future_key\twhatever\n"
		"item\tcube\tF\tF\tGood Two\n";
	Check(ParseCharacterSnapshot(mixed, parsed), "malformed item rows do not reject the file");
	Check(parsed.items.size() == 2, "only well-formed items are kept");
	Check(parsed.items[0].searchText == L"Good One" && parsed.items[1].source == ItemSource::Cube, "kept rows are correct");
}

void TestAge() {
	Check(FormatSnapshotAge(1000, 0) == L"unknown age", "zero timestamp");
	Check(FormatSnapshotAge(1000, 990) == L"just now", "seconds");
	Check(FormatSnapshotAge(1000, 2000) == L"just now", "clock skew is clamped");
	Check(FormatSnapshotAge(10000, 10000 - 5 * 60) == L"5 min ago", "minutes");
	Check(FormatSnapshotAge(100000, 100000 - 3 * 3600) == L"3 h ago", "hours");
	Check(FormatSnapshotAge(1000000, 1000000 - 86400) == L"1 day ago", "one day");
	Check(FormatSnapshotAge(1000000, 1000000 - 3 * 86400) == L"3 days ago", "days");
}

void TestBaseTypeName() {
	// Examples from live vanilla snapshots
	Check(SnapshotBaseTypeName(L"Monarch Spirit", L"Spirit") == L"Monarch", "runeword base type");
	Check(SnapshotBaseTypeName(L"Shako Harlequin Crest", L"Harlequin Crest") == L"Shako", "unique base type");
	Check(SnapshotBaseTypeName(L"Ancient Armor Milabrega's Robe", L"Milabrega's Robe") == L"Ancient Armor", "multi-word base type");
	Check(SnapshotBaseTypeName(L"Hawk Branded Amulet of Enlightenment", L"Hawk Branded Amulet of Enlightenment").empty(),
		"magic item with no separate type");
	Check(SnapshotBaseTypeName(L"Horadric Cube", L"Horadric Cube").empty(), "full name equals short name");
	Check(SnapshotBaseTypeName(L"Monarch Spirit", L"").empty(), "empty short name");
	Check(SnapshotBaseTypeName(L"MonarchSpirit", L"Spirit").empty(), "suffix must be a separate word");
	Check(SnapshotBaseTypeName(L"Monarch Spirit", L"Enigma").empty(), "unrelated short name");
	Check(SnapshotBaseTypeName(L" Spirit", L"Spirit").empty(), "blank base type");
}

void TestUtf8() {
	Check(CacheUtf8ToWide("\xC3\xA9") == L"\x00E9", "2-byte UTF-8");
	Check(CacheUtf8ToWide("\xF0\x9F\x94\xA5") == L"\U0001F525", "4-byte UTF-8");
	Check(CacheUtf8ToWide("\xFF") == L"\xFFFD", "invalid lead byte");
	Check(CacheUtf8ToWide("\xC0\xAF") == L"\xFFFD\xFFFD", "overlong encoding rejected");
	Check(CacheWideToUtf8(L"\x00E9\U0001F525") == "\xC3\xA9\xF0\x9F\x94\xA5", "wide to UTF-8");
}

} // namespace

int main() {
	TestClassification();
	TestSourceCodes();
	TestNames();
	TestRoundTrip();
	TestRejects();
	TestAge();
	TestBaseTypeName();
	TestUtf8();
	std::puts("pyre-stashsearch character cache tests passed.");
	return 0;
}
