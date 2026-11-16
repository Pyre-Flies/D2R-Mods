#include "native.h"
#include "stash_search.h"
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

void TestTooltipNormalization() {
	using namespace StashSearch;
	Check(NormalizeNarrowTooltipLine("\xFF" "c4Harlequin Crest\xFF" "c0\nDefense: 120 *ignored") == "Harlequin Crest Defense: 120",
		"narrow D2R color codes, whitespace collapsing, and '*' truncation");
	Check(NormalizeNarrowTooltipLine("\xC3\xBF" "c8 Jah Rune \t\r\n +50 To Life") == "Jah Rune +50 To Life",
		"UTF-8 encoded 0xC3 0xBF cX color sequence stripping");
	Check(NormalizeNarrowTooltipLine("\xEE\x81\xBE" "8Gem Bag") == "Gem Bag",
		"D2R runtime U+E07E (0xEE 0x81 0xBE) color sequence stripping");
	Check(NormalizeWideTooltipLine(L"\x00FF" L"c4Shako\x00FF" L"c0\r\n  +2 To All Skills *sock") == L"Shako +2 To All Skills",
		"wide D2R color codes, whitespace collapsing, and '*' truncation");
	Check(NormalizeWideTooltipLine(L"\x00C3\x00BF" L"c2  Enigma  \n +1 To Teleport ") == L"Enigma +1 To Teleport",
		"wide 0x00C3 0x00BF cX sequence stripping");
	Check(NormalizeWideTooltipLine(L"\uE07E8Gem Bag") == L"Gem Bag",
		"wide U+E07E color sequence stripping");
	Check(ExtractShortItemDisplayName("\xEE\x81\xBE" "8Gem Bag\n\xEE\x81\xBE" "8Gem Bag") == L"Gem Bag",
		"short item display name strips D2R runtime U+E07E color sequence");
	Check(NormalizeNarrowAffixText("\xFF" "c3+2 To All Skills\n+25% Faster Cast Rate *note") == "+2 To All Skills +25% Faster Cast Rate *note",
		"affix text normalization strips color codes and preserves text past '*'");
	Check(ExtractShortItemDisplayName("\xFF" "c4Skull Cap [N]\nDevourer of Dreams (8)\n") == L"Devourer of Dreams",
		"short item display name extracts last line and strips [N] and (8) loot filter tags");
	Check(ExtractShortItemDisplayName("Skull Cap [N] Devourer of Dreams (8)") == L"Devourer of Dreams",
		"short item display name extracts name after [N] when on a single line");
	Check(ExtractShortItemDisplayName("Skull Cap [N]") == L"Skull Cap",
		"short item display name preserves base item name when no unique name follows [N]");
	Check(ExtractShortItemDisplayName("Monarch [E] (4)") == L"Monarch",
		"short item display name strips both [E] and (4) on normal/socketed base item");
}

void TestCaseInsensitiveMatching() {
	using namespace StashSearch;
	Check(ContainsCaseInsensitive(L"shako", L"Harlequin Crest Shako"), "case-insensitive substring match");
	Check(ContainsCaseInsensitive(L"ALL SKILLS", L"+2 To All Skills"), "uppercase needle matches mixed haystack");
	Check(!ContainsCaseInsensitive(L"griffon", L"Harlequin Crest Shako"), "non-matching substring returns false");
	Check(!ContainsCaseInsensitive(L"", L"Harlequin Crest"), "empty needle returns false");
}

void TestBooleanAndShorthandQuery() {
	using namespace StashSearch;
	const std::wstring monarch4os = L"Monarch [E] (4) Defense: 148 Socketed (4) [qual:normal]";
	const std::wstring ethMonarch = L"Monarch [E] (4) Defense: 222 Ethereal (Cannot Be Repaired), Socketed (4) [qual:normal] [flag:ethereal]";
	const std::wstring fcrRing    = L"Ring Dread Loop +10% Faster Cast Rate +15 To All Resistances +80 To Attack Rating [qual:rare]";
	const std::wstring coldFcrRing = L"Ring Havoc Grip +10% Faster Cast Rate Cold Resist +28% [qual:rare]";
	const std::wstring bkRing     = L"Ring Bul-Kathos's Wedding Band +1 To All Skills 5% Life Stolen Per Hit [qual:unique]";
	const std::wstring griffon    = L"Diadem [E] Griffon's Eye -15% To Enemy Lightning Resistance +25% Faster Cast Rate [qual:unique]";
	const std::wstring spiritMon  = L"Monarch [E] Spirit +2 To All Skills +35% Faster Cast Rate Socketed (4) [qual:normal] [flag:runeword]";

	// Shorthands
	Check(EvaluateSearchQuery(L"4os", monarch4os), "'4os' matches Socketed (4)");
	Check(EvaluateSearchQuery(L"os", monarch4os), "'os' matches socketed item");
	Check(EvaluateSearchQuery(L"sockets", monarch4os), "'sockets' matches socketed item");
	Check(!EvaluateSearchQuery(L"3os", monarch4os), "'3os' does not match Socketed (4)");
	Check(EvaluateSearchQuery(L"eth", ethMonarch), "'eth' matches Ethereal");
	Check(!EvaluateSearchQuery(L"eth", monarch4os), "'eth' does not match non-ethereal");
	Check(EvaluateSearchQuery(L"fcr", fcrRing), "'fcr' matches Faster Cast Rate");
	Check(EvaluateSearchQuery(L"allres", fcrRing), "'allres' matches All Resistances");
	Check(EvaluateSearchQuery(L"ar", fcrRing), "'ar' matches Attack Rating");
	Check(!EvaluateSearchQuery(L"ar", monarch4os), "'ar' does not false-match 'Monarch'");
	Check(EvaluateSearchQuery(L"ll", bkRing), "'ll' matches Life Stolen Per Hit");
	Check(EvaluateSearchQuery(L"uni", bkRing), "'uni' matches [qual:unique]");
	Check(!EvaluateSearchQuery(L"uni", monarch4os), "'uni' does not match normal item");
	Check(EvaluateSearchQuery(L"rare", fcrRing), "'rare' matches [qual:rare]");
	Check(EvaluateSearchQuery(L"rw", spiritMon), "'rw' matches [flag:runeword]");

	// Boolean AND (&) and OR (|)
	Check(EvaluateSearchQuery(L"monarch & 4os", monarch4os), "'monarch & 4os' matches 4os Monarch");
	Check(!EvaluateSearchQuery(L"monarch & 4os & eth", monarch4os), "'monarch & 4os & eth' rejects non-eth Monarch");
	Check(EvaluateSearchQuery(L"monarch & 4os & eth", ethMonarch), "'monarch & 4os & eth' matches eth 4os Monarch");
	Check(EvaluateSearchQuery(L"griffon | shako", griffon), "'griffon | shako' matches Griffon's Eye");
	Check(EvaluateSearchQuery(L"ring & fcr & allres", fcrRing), "'ring & fcr & allres' matches FCR all-res ring");
	Check(!EvaluateSearchQuery(L"ring & fcr & allres", coldFcrRing), "'ring & fcr & allres' rejects cold-only FCR ring");

	// Exclusion (- and !) vs Hyphenated Item Names and Negative Numbers
	Check(EvaluateSearchQuery(L"Bul-Kathos", bkRing), "hyphenated 'Bul-Kathos' matches literally and is not treated as exclusion");
	Check(EvaluateSearchQuery(L"bul-kathos & ll", bkRing), "hyphenated 'bul-kathos & ll' matches BK ring");
	Check(EvaluateSearchQuery(L"bul-kathos -cold", bkRing), "hyphenated 'bul-kathos' with inline '-cold' exclusion matches BK ring");
	Check(!EvaluateSearchQuery(L"!Bul-Kathos & ring", bkRing), "'!Bul-Kathos & ring' excludes BK ring");
	Check(EvaluateSearchQuery(L"-15%", griffon), "negative stat '-15%' matches literally and is not treated as exclusion");
	Check(EvaluateSearchQuery(L"ring & fcr & -cold", fcrRing), "'ring & fcr & -cold' matches FCR ring without cold");
	Check(!EvaluateSearchQuery(L"ring & fcr & -cold", coldFcrRing), "'ring & fcr & -cold' excludes FCR ring with cold");
	Check(EvaluateSearchQuery(L"monarch & 4os & -eth", monarch4os), "'monarch & 4os & -eth' matches non-eth 4os Monarch");
	Check(!EvaluateSearchQuery(L"monarch & 4os & -eth", ethMonarch), "'monarch & 4os & -eth' excludes eth 4os Monarch");
	Check(EvaluateSearchQuery(L"monarch 4os -eth", monarch4os), "space-separated inline '-eth' matches non-eth 4os Monarch");
	Check(!EvaluateSearchQuery(L"monarch 4os -eth", ethMonarch), "space-separated inline '-eth' excludes eth 4os Monarch");

	// Word-boundary affix matching (header before '\n', affixes after '\n')
	const std::wstring grandfather =
		L"Colossus Blade [E] The Grandfather [qual:unique] Socketed (0)\n"
		L"+200% Enhanced Damage +220 to Maximum Damage (Based on Character Level) +20 to All Attributes Indestructible";
	const std::wstring arachnidMesh =
		L"Spiderweb Sash [E] Arachnid Mesh [qual:unique] Socketed (0)\n"
		L"+1 to All Skills +20% Faster Cast Rate Slows Target by 10%";
	Check(EvaluateSearchQuery(L"Arac", arachnidMesh), "'Arac' matches Arachnid Mesh in item header");
	Check(!EvaluateSearchQuery(L"Arac", grandfather), "'Arac' does not false-match 'Character' inside affix text on The Grandfather");
	Check(!EvaluateSearchQuery(L"ist", arachnidMesh), "'ist' does not false-match mid-word affix substrings");
	Check(EvaluateSearchQuery(L"maximum damage", grandfather), "word-boundary affix phrase 'maximum damage' matches The Grandfather");
	Check(EvaluateSearchQuery(L"indestructible", grandfather), "word-boundary affix 'indestructible' matches The Grandfather");
}

void TestSlashPageCommands() {
	using namespace StashSearch;
	const auto normal = ParseSlashPageCommand(L"shako");
	Check(normal.state == SlashCommandState::None && normal.pageNumber == 0, "normal query is not a slash command");

	const auto missing = ParseSlashPageCommand(L"  /  ");
	Check(missing.state == SlashCommandState::MissingNumber &&
		FormatSlashPagePrompt(missing) == L"Enter shared page number",
		"'/' prompts for shared page number");

	const auto zeroPage = ParseSlashPageCommand(L"/0");
	Check(zeroPage.state == SlashCommandState::InvalidNumber &&
		FormatSlashPagePrompt(zeroPage) == L"Invalid page number",
		"'/0' is rejected as invalid page number");

	const auto nonDigit = ParseSlashPageCommand(L"/12a");
	Check(nonDigit.state == SlashCommandState::InvalidNumber &&
		FormatSlashPagePrompt(nonDigit) == L"Invalid page number",
		"'/12a' is rejected as invalid page number");

	const auto valid = ParseSlashPageCommand(L" /14 ");
	Check(valid.state == SlashCommandState::ValidPage &&
		valid.pageNumber == 14 &&
		FormatSlashPagePrompt(valid) == L"Press Enter to jump to shared page 14",
		"'/14' parses as valid 1-based shared page command");
}

void TestDropdownScrollMath() {
	using namespace StashSearch;
	size_t sel    = 0;
	size_t scroll = 0;
	AdjustDropdownSelectionAndScroll(10, 4, 5, sel, scroll);
	Check(sel == 5 && scroll == 2, "scrolling down clamps scroll window so selected row is visible at bottom");

	AdjustDropdownSelectionAndScroll(10, 4, 1, sel, scroll);
	Check(sel == 1 && scroll == 1, "scrolling up moves scroll window up to selected row");

	AdjustDropdownSelectionAndScroll(10, 4, 99, sel, scroll);
	Check(sel == 9 && scroll == 6, "selection beyond matchCount clamps to last item and max scroll offset");

	AdjustDropdownSelectionAndScroll(0, 4, 3, sel, scroll);
	Check(sel == 0 && scroll == 0, "empty match list resets selection and scroll to 0");
}

void TestScaleHelpers() {
	using namespace StashSearch;
	Check(ScaleBorderThickness(2, 1.0f) == 2, "border thickness at 1080p");
	Check(ScaleBorderThickness(2, 0.2f) == 1, "border thickness clamps minimum to 1");
	int scaled = 0;
	Check(ScalePositiveDimension(280, 1.5f, &scaled) && scaled == 420, "dimension scaling at 1.5x");
	Check(!ScalePositiveDimension(0, 1.0f, &scaled), "zero dimension rejected");
	Check(Rva::GetLocalizedStringByKey == 0x05F4B90, "GetLocalizedStringByKey RVA matches 0x5F4B90");
	Check(Rva::DrawFilledRect == 0x0657B90, "DrawFilledRect RVA matches 0x657B90");
	Check(Rva::BuildItemStatsString == 0x02DA0E0, "BuildItemStatsString RVA matches 0x2DA0E0");
	Check(Rva::BuildSocketableStatsString == 0x02DA310, "BuildSocketableStatsString RVA matches 0x2DA310");
}

void TestConfigPersistence() {
	Check(!StashSearch::ParseTabFlyoutExpandedConfig("", false), "empty config defaults to false");
	Check(StashSearch::ParseTabFlyoutExpandedConfig("", true), "empty config honors caller default");

	const std::string createdTrue = StashSearch::UpdateTabFlyoutExpandedConfig("", true);
	Check(StashSearch::ParseTabFlyoutExpandedConfig(createdTrue, false), "created config parses as true");

	const std::string updatedFalse = StashSearch::UpdateTabFlyoutExpandedConfig(createdTrue, false);
	Check(!StashSearch::ParseTabFlyoutExpandedConfig(updatedFalse, true), "updated config parses as false");

	const std::string customDoc =
		"# Custom comment\r\n"
		"[stash_search]\r\n"
		"tab_flyout_expanded = false # inline comment\r\n";
	const std::string updatedCustom = StashSearch::UpdateTabFlyoutExpandedConfig(customDoc, true);
	Check(StashSearch::ParseTabFlyoutExpandedConfig(updatedCustom, false), "updated custom doc parses as true");
	Check(updatedCustom.find("# inline comment") != std::string::npos, "inline comment preserved");

	StashSearch::StashSearchConfig cfg{};
	cfg.tabFlyoutExpanded = true;
	cfg.personalTabName   = L"  Main Gear  ";
	cfg.sharedTabNames    = {
		{ 4, L"Runeword Bases" },
		{ 0, L"Uniques & Sets" },
	};
	const std::string serialized = StashSearch::SerializeStashSearchConfig(updatedCustom, cfg);
	const StashSearch::StashSearchConfig parsed = StashSearch::ParseStashSearchConfig(serialized, false);
	Check(parsed.tabFlyoutExpanded, "tabFlyoutExpanded preserved with custom tab names");
	Check(parsed.personalTabName == L"Main Gear", "personalTabName trimmed and roundtripped");
	Check(parsed.sharedTabNames.size() == 2, "two shared tab names roundtripped");
	Check(parsed.sharedTabNames[0].first == 0 && parsed.sharedTabNames[0].second == L"Uniques & Sets", "tab_1 roundtripped and sorted");
	Check(parsed.sharedTabNames[1].first == 4 && parsed.sharedTabNames[1].second == L"Runeword Bases", "tab_5 roundtripped and sorted");

	// Clearing a tab name removes it cleanly on re-serialization
	StashSearch::StashSearchConfig clearedCfg = parsed;
	clearedCfg.personalTabName.clear();
	clearedCfg.sharedTabNames.erase(clearedCfg.sharedTabNames.begin());
	const std::string reserialized = StashSearch::SerializeStashSearchConfig(serialized, clearedCfg);
	const StashSearch::StashSearchConfig reparsed = StashSearch::ParseStashSearchConfig(reserialized, false);
	Check(reparsed.personalTabName.empty(), "cleared personalTabName removed from TOML");
	Check(reparsed.sharedTabNames.size() == 1 && reparsed.sharedTabNames[0].first == 4, "cleared tab_1 removed while tab_5 preserved");
}

void TestIsolatedDropdownMatchFilter() {
	const StashSearch::StashPageTarget sharedPage1{ true, 0 };
	const StashSearch::StashPageTarget sharedPage2{ true, 1 };
	const StashSearch::StashPageTarget personalPage{ false, 0 };

	// Normal query mode (lockedMatchActive == false): any item matching the query highlights
	Check(
		StashSearch::DoesItemMatchActiveFilter(true, L"Harlequin Crest", sharedPage1, false, sharedPage1, L"Harlequin Crest"),
		"unlocked filter highlights matching item");
	Check(
		StashSearch::DoesItemMatchActiveFilter(true, L"Small Charm", sharedPage1, false, sharedPage1, L"Harlequin Crest"),
		"unlocked filter highlights other matching item on same page");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(false, L"Harlequin Crest", sharedPage1, false, sharedPage1, L"Harlequin Crest"),
		"unlocked filter dims non-matching item");

	// Locked dropdown selection mode (lockedMatchActive == true): only the selected item entry on the target page highlights
	Check(
		StashSearch::DoesItemMatchActiveFilter(true, L"Harlequin Crest", sharedPage1, true, sharedPage1, L"Harlequin Crest"),
		"locked filter highlights selected item on target page");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(true, L"Small Charm", sharedPage1, true, sharedPage1, L"Harlequin Crest"),
		"locked filter dims other query-matching items on the same page");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(false, L"Harlequin Crest", sharedPage1, true, sharedPage1, L"Harlequin Crest"),
		"locked filter dims same-named item that does not satisfy query");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(true, L"Harlequin Crest", sharedPage2, true, sharedPage1, L"Harlequin Crest"),
		"locked filter dims same-named item on a different shared page");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(true, L"Harlequin Crest", personalPage, true, sharedPage1, L"Harlequin Crest"),
		"locked filter dims same-named item on personal page when target is shared");

	// Same-named items with different affixes (e.g. Maps or Small Charms on the same page): only the exact selected affix tooltip matches
	Check(
		StashSearch::DoesItemMatchActiveFilter(
			true,
			L"Desert Ruins Map",
			sharedPage1,
			true,
			sharedPage1,
			L"Desert Ruins Map",
			L"Desert Ruins Map +45% Monster Density +80% Magic Find",
			L"Desert Ruins Map +45% Monster Density +80% Magic Find"),
		"locked filter highlights exact map with matching affix tooltip");
	Check(
		!StashSearch::DoesItemMatchActiveFilter(
			true,
			L"Desert Ruins Map",
			sharedPage1,
			true,
			sharedPage1,
			L"Desert Ruins Map",
			L"Desert Ruins Map +20% Monster Density Players have -30% Fire Resist",
			L"Desert Ruins Map +45% Monster Density +80% Magic Find"),
		"locked filter dims another map with the same shortName but different affixes on the same page");
}

void TestBuildItemTooltipLines() {
	size_t headerCount = 0;
	const auto lines = StashSearch::BuildItemTooltipLines(
		"Skull Cap [N]\n\xFF" "c4Devourer of Dreams (8)*internal",
		"Skull Cap",
		"\xFF" "c3+1 to All Skills\n\xFF" "c3+25% Faster Cast Rate\n+40 to Life\n",
		"",
		L" - Shared 5",
		&headerCount);
	Check(headerCount == 2, "multi-line item description produces 2 header lines without duplicating Skull Cap");
	Check(lines.size() == 5, "header lines + 3 affix lines produce 5 total tooltip lines");
	if (lines.size() == 5) {
		Check(lines[0] == L"Devourer of Dreams (8) - Shared 5", "line 0 is reversed top title with tab suffix");
		Check(lines[1] == L"Skull Cap [N]", "line 1 is base item line from description");
		Check(lines[2] == L"+1 to All Skills", "line 2 is first affix line in top-down order");
		Check(lines[3] == L"+25% Faster Cast Rate", "line 3 is second affix line in top-down order");
		Check(lines[4] == L"+40 to Life", "line 4 is third affix line in top-down order");
	}

	// Single-line item description with distinct base item name and socketable stats
	size_t runeHeaderCount = 0;
	const auto runeLines = StashSearch::BuildItemTooltipLines(
		"\xFF" "c8Jah Rune",
		"Jah Rune",
		"",
		"Weapons: Ignore Target's Defense\nArmor: Increase Maximum Life 5%\nHelms: Increase Maximum Life 5%\nShields: +50 to Life",
		L" - Shared 2 (Runes)",
		&runeHeaderCount);
	Check(runeHeaderCount == 1, "identical base name is not duplicated in header");
	Check(runeLines.size() == 5, "rune header + 4 socketable lines produce 5 total tooltip lines");
	if (runeLines.size() == 5) {
		Check(runeLines[0] == L"Jah Rune - Shared 2 (Runes)", "rune header line includes custom tab suffix");
		Check(runeLines[1] == L"Weapons: Ignore Target's Defense", "rune socketable line 1 preserved");
		Check(runeLines[4] == L"Shields: +50 to Life", "rune socketable line 4 preserved");
	}
}

void TestActiveSearchOverlayHitTest() {
	StashSearch::SearchBarLayout layout{};
	layout.valid           = true;
	layout.scale           = 1.0f;
	layout.searchBox       = { 100, 50, 300, 36 };
	layout.dropdownBox     = { 100, 88, 300, 240 };
	layout.visibleRowCount = 6;
	layout.hasScrollbar    = true;
	layout.scrollbarTrack  = { 386, 90, 12, 236 };
	layout.pagePickerOpen  = true;
	layout.pageCellCount   = 5;
	layout.pagePickerBox   = { 420, 50, 180, 300 };

	Check(
		StashSearch::IsPointInActiveSearchOverlay(layout, true, 150, 65),
		"cursor inside search box is over overlay");
	Check(
		StashSearch::IsPointInActiveSearchOverlay(layout, true, 200, 180),
		"cursor inside open dropdown is over overlay");
	Check(
		!StashSearch::IsPointInActiveSearchOverlay(layout, false, 200, 180),
		"cursor inside dropdown area when dropdown is hidden is not over overlay");
	Check(
		StashSearch::IsPointInActiveSearchOverlay(layout, false, 460, 120),
		"cursor inside open side tab flyout is over overlay");
	Check(
		!StashSearch::IsPointInActiveSearchOverlay(layout, true, 200, 360),
		"cursor on uncovered stash grid below dropdown is not over overlay");

	// Controller collapsed button-only layout
	StashSearch::SearchBarLayout controllerLayout{};
	controllerLayout.valid                 = true;
	controllerLayout.collapsedToButtonOnly = true;
	controllerLayout.scale                 = 1.0f;
	controllerLayout.pageButtonRect        = { 364, 50, 36, 36 };
	controllerLayout.searchBox             = controllerLayout.pageButtonRect;
	controllerLayout.pagePickerOpen        = true;
	controllerLayout.pageCellCount         = 5;
	controllerLayout.pagePickerBox         = { 410, 50, 180, 300 };

	Check(
		!StashSearch::IsPointInActiveSearchOverlay(controllerLayout, false, 150, 65),
		"controller layout does not cover LB+LT/RT area to the left of the collapsed arrow button");
	Check(
		StashSearch::IsPointInActiveSearchOverlay(controllerLayout, false, 380, 65),
		"controller layout covers collapsed arrow button");
	Check(
		StashSearch::IsPointInActiveSearchOverlay(controllerLayout, false, 450, 120),
		"controller layout covers right-docked tab flyout");

	layout.valid = false;
	Check(
		!StashSearch::IsPointInActiveSearchOverlay(layout, true, 200, 180),
		"invalid layout never reports cursor over overlay");
}

void TestFlyoutChevronDirection() {
	// Both KB/M and Controller layouts dock the flyout to the right of the button:
	// '>' (pointLeft=false) when closed (expand right), '<' (pointLeft=true) when open (collapse left)
	Check(
		!StashSearch::ShouldPointFlyoutChevronLeft(false, false),
		"KB/M chevron points right ('>') when flyout is closed");
	Check(
		StashSearch::ShouldPointFlyoutChevronLeft(true, false),
		"KB/M chevron points left ('<') when flyout is open");
	Check(
		!StashSearch::ShouldPointFlyoutChevronLeft(false, true),
		"controller chevron points right ('>') when flyout is closed");
	Check(
		StashSearch::ShouldPointFlyoutChevronLeft(true, true),
		"controller chevron points left ('<') when flyout is open");
}

void TestControllerFlyoutCursorSnapAndRestore() {
	StashSearch::SearchBarLayout layout{};
	layout.valid                 = true;
	layout.collapsedToButtonOnly = true;
	layout.scale                 = 1.0f;
	layout.pageButtonRect        = { 364, 50, 36, 36 };
	layout.searchBox             = layout.pageButtonRect;
	layout.pagePickerOpen        = false;
	layout.pageCellCount         = 3;
	layout.pagePickerBox         = { 410, 50, 180, 180 };
	layout.pageCellRects[0]      = { 416, 56, 168, 48 };
	layout.pageCellIndices[0]    = 0;
	layout.pageCellIsShared[0]   = false; // Personal
	layout.pageCellRects[1]      = { 416, 112, 168, 48 };
	layout.pageCellIndices[1]    = 0;
	layout.pageCellIsShared[1]   = true;  // Shared 1
	layout.pageCellRects[2]      = { 416, 168, 168, 48 };
	layout.pageCellIndices[2]    = 1;
	layout.pageCellIsShared[2]   = true;  // Shared 2

	StashSearch::ControllerFlyoutCursorState state{};

	// 1. Track stash grid cursor before opening the flyout, then open with 'A' on the toggle button
	StashSearch::TrackControllerPreFlyoutCursor(state, layout, POINT{ 220, 260 });
	Check(
		state.savedPreFlyoutCursorPt.x == 220 && state.savedPreFlyoutCursorPt.y == 260,
		"tracks stash grid cursor position before flyout opens");

	// Moving into the top header bar on the way to the toggle button does not overwrite the stash grid position
	StashSearch::TrackControllerPreFlyoutCursor(state, layout, POINT{ 350, 60 });
	Check(
		state.savedPreFlyoutCursorPt.x == 220 && state.savedPreFlyoutCursorPt.y == 260,
		"header transit does not overwrite saved stash grid position");

	// Pressing 'A' on the toggle button opens the flyout, preserves savedPreFlyoutCursorPt, and queues flyout focus snap
	StashSearch::OnControllerFlyoutToggled(state, layout, true, POINT{ 380, 65 });
	Check(
		state.pendingFlyoutFocus && !state.pendingCursorRestore,
		"opening flyout via controller toggle queues focus snap into flyout");
	Check(
		state.savedPreFlyoutCursorPt.x == 220 && state.savedPreFlyoutCursorPt.y == 260,
		"opening flyout via controller toggle preserves pre-flyout stash cursor position");

	// On the next frame once pagePickerOpen is true, ConsumeControllerFlyoutFocusTarget snaps to the active tab cell
	layout.pagePickerOpen = true;
	POINT focusPt{ -1, -1 };
	Check(
		StashSearch::ConsumeControllerFlyoutFocusTarget(state, layout, 1, 1, &focusPt),
		"consumes pending flyout focus when flyout is open");
	Check(
		focusPt.x == (416 + 168 / 2) && focusPt.y == (168 + 48 / 2),
		"snaps controller cursor to center of active shared tab cell in flyout");
	Check(
		!state.pendingFlyoutFocus,
		"clears pendingFlyoutFocus after snapping cursor");

	// 2. Closing the flyout via toggle restores the cursor back to the saved pre-flyout position and resets it
	StashSearch::OnControllerFlyoutToggled(state, layout, false, POINT{ 380, 65 });
	Check(
		state.pendingCursorRestore && !state.pendingFlyoutFocus,
		"closing flyout in controller mode queues cursor restore");

	POINT restorePt{ -1, -1 };
	Check(
		StashSearch::ConsumeControllerFlyoutCursorRestore(state, layout, &restorePt),
		"consumes pending cursor restore on close");
	Check(
		restorePt.x == 220 && restorePt.y == 260,
		"restores controller cursor to saved pre-flyout stash position");
	Check(
		state.savedPreFlyoutCursorPt.x == -1 && state.savedPreFlyoutCursorPt.y == -1,
		"resets savedPreFlyoutCursorPt after restoring cursor");

	// 3. Opening directly via toggle when no prior stash position was tracked saves the toggle cursor position
	layout.pagePickerOpen = false;
	StashSearch::OnControllerFlyoutToggled(state, layout, true, POINT{ 382, 68 });
	Check(
		state.savedPreFlyoutCursorPt.x == 382 && state.savedPreFlyoutCursorPt.y == 68 && state.pendingFlyoutFocus,
		"opening without prior stash tracking saves current cursor position and sets pendingFlyoutFocus");
	StashSearch::OnControllerFlyoutToggled(state, layout, false, POINT{ 382, 68 });
	POINT fallbackRestorePt{ -1, -1 };
	Check(
		StashSearch::ConsumeControllerFlyoutCursorRestore(state, layout, &fallbackRestorePt) &&
			fallbackRestorePt.x == 382 && fallbackRestorePt.y == 68,
		"closing restores to the saved toggle cursor position");

	// 4. Latching an overlay 'A' press defers cursor restoration until 'A' is released (+ 1 post-release frame)
	StashSearch::ResetControllerFlyoutCursorState(state);
	StashSearch::TrackControllerPreFlyoutCursor(state, layout, POINT{ 180, 260 });
	layout.pagePickerOpen = true;
	StashSearch::OnControllerFlyoutToggled(state, layout, true, POINT{ 382, 68 });
	POINT dummyFocusPt{ -1, -1 };
	(void)StashSearch::ConsumeControllerFlyoutFocusTarget(state, layout, 1, 2, &dummyFocusPt);

	StashSearch::LatchControllerOverlayAPress(state);
	StashSearch::OnControllerFlyoutToggled(state, layout, false, POINT{ 382, 68 });
	layout.pagePickerOpen = false;
	Check(
		StashSearch::UpdateControllerOverlayAPressLatch(state, true) && state.overlayAPressLatched,
		"overlay 'A' press latch stays active while 'A' is held down");
	POINT deferredRestorePt{ -1, -1 };
	Check(
		!StashSearch::ConsumeControllerFlyoutCursorRestore(state, layout, &deferredRestorePt) &&
			state.pendingCursorRestore,
		"cursor restore is deferred while overlay 'A' press latch is active");
	Check(
		StashSearch::UpdateControllerOverlayAPressLatch(state, false) && state.overlayAPressLatched,
		"overlay 'A' press latch stays active for one post-release frame");
	Check(
		!StashSearch::ConsumeControllerFlyoutCursorRestore(state, layout, &deferredRestorePt) &&
			state.pendingCursorRestore,
		"cursor restore remains deferred on the release frame itself");
	Check(
		!StashSearch::UpdateControllerOverlayAPressLatch(state, false) && !state.overlayAPressLatched,
		"overlay 'A' press latch clears on the frame after release");
	Check(
		StashSearch::ConsumeControllerFlyoutCursorRestore(state, layout, &deferredRestorePt) &&
			deferredRestorePt.x == 180 && deferredRestorePt.y == 260,
		"cursor restore succeeds once the overlay 'A' press latch has cleared");
}

namespace {

struct FakeXInputEnv {
	uint32_t                        callCount = 0;
	uint32_t                        slotCalls[StashSearch::kXInputMaxControllers]{};
	bool                            connected[StashSearch::kXInputMaxControllers]{};
	StashSearch::XInputControllerState states[StashSearch::kXInputMaxControllers]{};
};

FakeXInputEnv g_FakeXInput{};

uint32_t WINAPI FakeXInputGetState(uint32_t dwUserIndex, StashSearch::XInputControllerState* pState) {
	++g_FakeXInput.callCount;
	if (dwUserIndex >= StashSearch::kXInputMaxControllers) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	++g_FakeXInput.slotCalls[dwUserIndex];
	if (!g_FakeXInput.connected[dwUserIndex]) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	if (pState != nullptr) {
		*pState = g_FakeXInput.states[dwUserIndex];
	}
	return ERROR_SUCCESS;
}

} // namespace

void TestXInputPollCacheAndActivePad() {
	// 1. All 4 slots disconnected: probes once at t=1000ms, then makes 0 XInputGetState calls until t=2000ms
	g_FakeXInput = FakeXInputEnv{};
	StashSearch::XInputPollCache       cache{};
	StashSearch::XInputControllerState out{};

	Check(
		!StashSearch::PollActiveXInputState(&FakeXInputGetState, 1000, cache, &out),
		"returns false when no XInput controller is connected");
	Check(
		g_FakeXInput.callCount == 4,
		"initial probe checks all 4 slots once");

	Check(
		!StashSearch::PollActiveXInputState(&FakeXInputGetState, 1016, cache, &out) &&
			!StashSearch::PollActiveXInputState(&FakeXInputGetState, 1500, cache, &out) &&
			!StashSearch::PollActiveXInputState(&FakeXInputGetState, 1999, cache, &out),
		"intermediate frames within 1s window return false");
	Check(
		g_FakeXInput.callCount == 4,
		"disconnected slots are not re-probed before 1000ms has elapsed (0 calls on intermediate frames)");

	Check(
		!StashSearch::PollActiveXInputState(&FakeXInputGetState, 2000, cache, &out),
		"re-probes disconnected slots after 1000ms");
	Check(
		g_FakeXInput.callCount == 8,
		"exactly 4 additional calls after 1000ms interval");

	// 2. Single connected slot (slot 2): intermediate frames only query slot 2 (1 call/frame, not 4)
	g_FakeXInput              = FakeXInputEnv{};
	g_FakeXInput.connected[2] = true;
	g_FakeXInput.states[2].dwPacketNumber = 10;
	cache                     = StashSearch::XInputPollCache{};

	Check(
		StashSearch::PollActiveXInputState(&FakeXInputGetState, 5000, cache, &out) && cache.activeSlot == 2,
		"detects connected controller on slot 2");
	Check(
		g_FakeXInput.callCount == 4,
		"first probe checks all 4 slots");

	g_FakeXInput.callCount = 0;
	Check(
		StashSearch::PollActiveXInputState(&FakeXInputGetState, 5016, cache, &out) && cache.activeSlot == 2,
		"polls connected slot 2 on next frame");
	Check(
		g_FakeXInput.callCount == 1 && g_FakeXInput.slotCalls[2] == 2,
		"only queries the cached connected slot on intermediate frames");

	// 3. Multiple connected pads (slot 0 idle, slot 1 active): prefers the pad actually in use
	g_FakeXInput              = FakeXInputEnv{};
	g_FakeXInput.connected[0] = true;
	g_FakeXInput.connected[1] = true;
	g_FakeXInput.states[0].dwPacketNumber   = 1;
	g_FakeXInput.states[1].dwPacketNumber   = 50;
	g_FakeXInput.states[1].Gamepad.wButtons = StashSearch::kXInputGamepadA;
	cache                     = StashSearch::XInputPollCache{};

	Check(
		StashSearch::PollActiveXInputState(&FakeXInputGetState, 8000, cache, &out),
		"returns true when multiple controllers are connected");
	Check(
		cache.activeSlot == 1 && (out.Gamepad.wButtons & StashSearch::kXInputGamepadA) != 0,
		"prefers connected controller with live input (slot 1) over idle slot 0");

	// Releasing button on slot 1 keeps slot 1 as the active pad rather than snapping back to idle slot 0
	g_FakeXInput.states[1].dwPacketNumber   = 51;
	g_FakeXInput.states[1].Gamepad.wButtons = 0;
	Check(
		StashSearch::PollActiveXInputState(&FakeXInputGetState, 8016, cache, &out) && cache.activeSlot == 1,
		"retains active controller slot after input release");
}

void TestDuplicateSearchAndGrouping() {
	using namespace StashSearch;
	constexpr uint32_t kFlagIdentified = 0x10u;
	constexpr uint32_t kFlagRuneword   = 1u << 26;

	// 1. InspectDuplicateQueryTerms detects positive vs negated duplicate tokens without false-matching normal queries
	const auto normalFlags = InspectDuplicateQueryTerms(L"monarch 4os -eth");
	Check(!normalFlags.usesDuplicateTerm && !normalFlags.hasPositiveDuplicateTerm, "normal query does not enable duplicate scan");

	const auto posDupe = InspectDuplicateQueryTerms(L"dupe uni -ring -amulet");
	Check(posDupe.usesDuplicateTerm && posDupe.hasPositiveDuplicateTerm, "'dupe uni -ring -amulet' enables duplicate scan and grouping");

	const auto posDupesOr = InspectDuplicateQueryTerms(L"dupes uni | dupes set");
	Check(posDupesOr.usesDuplicateTerm && posDupesOr.hasPositiveDuplicateTerm, "'dupes uni | dupes set' enables duplicate scan and grouping");

	const auto negDupe = InspectDuplicateQueryTerms(L"uni -dupe");
	Check(negDupe.usesDuplicateTerm && !negDupe.hasPositiveDuplicateTerm, "'uni -dupe' enables duplicate scan without duplicate grouping");

	const auto bangNegDupe = InspectDuplicateQueryTerms(L"!duplicate & set");
	Check(bangNegDupe.usesDuplicateTerm && !bangNegDupe.hasPositiveDuplicateTerm, "'!duplicate & set' enables duplicate scan without duplicate grouping");

	// 2. EvaluateSearchQuery with isDuplicate composes cleanly with AND, OR, and NOT
	const std::wstring shako   = L"Shako [E] Harlequin Crest [qual:unique] Socketed (0)\n+2 To All Skills +50% Better Chance of Getting Magic Items";
	const std::wstring bkRing  = L"Ring Bul-Kathos's Wedding Band [qual:unique] Socketed (0)\n+1 To All Skills";
	const std::wstring sigon   = L"Gothic Plate [N] Sigon's Shelter [qual:set] Socketed (0)\n+25% Enhanced Defense";
	const std::wstring monarch = L"Monarch [E] [qual:normal] Socketed (4)";

	Check(EvaluateSearchQuery(L"dupe", shako, true), "'dupe' matches duplicate item");
	Check(!EvaluateSearchQuery(L"dupe", shako, false), "'dupe' rejects singleton item");
	Check(EvaluateSearchQuery(L"dupes", shako, true), "'dupes' alias matches duplicate item");
	Check(EvaluateSearchQuery(L"duplicate", shako, true), "'duplicate' alias matches duplicate item");
	Check(EvaluateSearchQuery(L"duplicates", shako, true), "'duplicates' alias matches duplicate item");

	Check(EvaluateSearchQuery(L"dupe uni", shako, true), "'dupe uni' matches duplicate unique");
	Check(!EvaluateSearchQuery(L"dupe uni", shako, false), "'dupe uni' rejects singleton unique");
	Check(!EvaluateSearchQuery(L"dupe uni", monarch, true), "'dupe uni' rejects duplicate normal item");

	Check(EvaluateSearchQuery(L"dupe uni | dupe set", shako, true), "'dupe uni | dupe set' matches duplicate unique");
	Check(EvaluateSearchQuery(L"dupe uni | dupe set", sigon, true), "'dupe uni | dupe set' matches duplicate set");
	Check(!EvaluateSearchQuery(L"dupe uni | dupe set", sigon, false), "'dupe uni | dupe set' rejects singleton set");

	Check(EvaluateSearchQuery(L"dupe uni -ring -amulet", shako, true), "'dupe uni -ring -amulet' matches duplicate unique armor");
	Check(!EvaluateSearchQuery(L"dupe uni -ring -amulet", bkRing, true), "'dupe uni -ring -amulet' excludes duplicate unique ring");
	Check(EvaluateSearchQuery(L"uni -dupe", shako, false), "'uni -dupe' matches singleton unique");
	Check(!EvaluateSearchQuery(L"uni -dupe", shako, true), "'uni -dupe' excludes duplicate unique");

	// 3. BuildItemDuplicateKey rules across item qualities and identification states
	const auto shakoNormTier  = BuildItemDuplicateKey(7, kFlagIdentified, 111, 249, L"Harlequin Crest");
	const auto shakoEliteTier = BuildItemDuplicateKey(7, kFlagIdentified, 250, 249, L"Harlequin Crest");
	const auto unidShako      = BuildItemDuplicateKey(7, 0, 250, 249, L"Shako");
	const auto nagelring      = BuildItemDuplicateKey(7, kFlagIdentified, 522, 120, L"Nagelring");
	const auto soj            = BuildItemDuplicateKey(7, kFlagIdentified, 522, 122, L"The Stone of Jordan");
	Check(shakoNormTier.IsValid() && shakoNormTier == shakoEliteTier, "identified uniques with same fileIndex match even across base tier upgrades");
	Check(!unidShako.IsValid(), "unidentified unique is never treated as a duplicate");
	Check(nagelring != soj, "distinct unique rings with different fileIndex do not collide");

	const auto setPieceA = BuildItemDuplicateKey(5, kFlagIdentified, 80, 42, L"Sigon's Shelter");
	const auto setPieceB = BuildItemDuplicateKey(5, kFlagIdentified, 80, 42, L"Sigon's Shelter");
	const auto unidSet   = BuildItemDuplicateKey(5, 0, 80, 42, L"Gothic Plate");
	Check(setPieceA.IsValid() && setPieceA == setPieceB, "identified set items with same fileIndex match");
	Check(!unidSet.IsValid(), "unidentified set item is never treated as a duplicate");

	const auto normMonarch   = BuildItemDuplicateKey(2, 0, 447, 0, L"Monarch");
	const auto supMonarch    = BuildItemDuplicateKey(3, kFlagIdentified, 447, 0, L"Superior Monarch");
	const auto gemmedMonarch = BuildItemDuplicateKey(2, 0, 447, 0, L"Gemmed Monarch", "uit ", true);
	Check(normMonarch.IsValid() && normMonarch == supMonarch, "normal and superior bases with same classId match");
	Check(!gemmedMonarch.IsValid(), "socket-filled (Gemmed) white/superior base does not collide with empty bases");

	const auto spiritSword  = BuildItemDuplicateKey(2, kFlagIdentified | kFlagRuneword, 29, 0, L"Spirit", "crs ", true);
	const auto spiritShield = BuildItemDuplicateKey(3, kFlagIdentified | kFlagRuneword, 447, 0, L"spirit", "uit ", true);
	Check(spiritSword.IsValid() && spiritSword == spiritShield, "runewords with the same name match case-insensitively even when socketed");
	Check(spiritSword != normMonarch, "runeword Monarch does not collide with plain base Monarch");

	const auto rareRing1 = BuildItemDuplicateKey(6, kFlagIdentified, 522, 0, L"Havoc Grip");
	const auto rareRing2 = BuildItemDuplicateKey(6, kFlagIdentified, 522, 0, L"Beast Turn");
	Check(rareRing1.IsValid() && rareRing2.IsValid() && rareRing1 != rareRing2, "distinct rare rings do not collide");

	const auto magicRingA = BuildItemDuplicateKey(4, kFlagIdentified, 522, 0, L"Bahamut's Ring of the Apprentice", "rin ");
	const auto magicRingB = BuildItemDuplicateKey(4, kFlagIdentified, 522, 0, L"Bahamut's Ring of the Apprentice", "rin ");
	Check(magicRingA.IsValid() && magicRingA == magicRingB, "magic rings with identical base and name match");

	// Charms and jewels are excluded from duplicate keys across all qualities (magic, rare, crafted, unique)
	for (const std::string_view code : { "cm1 ", "cm2 ", "cm3 ", "cm4 ", "cm5 ", "cs1 ", "cs2 ", "jew ", "cjw " }) {
		Check(IsCharmOrJewelItemCode(code), "space-padded charm/jewel code recognized");
	}
	Check(IsCharmOrJewelItemCode(std::string_view("jew\0", 4)), "NUL-padded jewel code recognized");
	Check(IsCharmOrJewelItemCode("CM3 "), "uppercase charm code recognized");
	Check(!IsCharmOrJewelItemCode("rin ") && !IsCharmOrJewelItemCode("amu ") && !IsCharmOrJewelItemCode("r30 ") && !IsCharmOrJewelItemCode("ceh ") && !IsCharmOrJewelItemCode("jwp "),
		"non-charm/jewel item codes are not excluded");
	Check(!BuildItemDuplicateKey(4, kFlagIdentified, 605, 0, L"Harpoonist's Grand Charm", "cm3 ").IsValid(), "magic Grand Charm excluded from duplicate key");
	Check(!BuildItemDuplicateKey(4, kFlagIdentified, 603, 0, L"Small Charm of Vita", "cm1 ").IsValid(), "magic Small Charm excluded from duplicate key");
	Check(!BuildItemDuplicateKey(7, kFlagIdentified, 605, 160, L"Gheed's Fortune", "cm3 ").IsValid(), "unique Grand Charm excluded from duplicate key");
	Check(!BuildItemDuplicateKey(7, kFlagIdentified, 701, 410, L"Bone Break", "cs1 ").IsValid(), "sunder charm excluded from duplicate key");
	Check(!BuildItemDuplicateKey(4, kFlagIdentified, 643, 0, L"Ruby Jewel of Fervor", "jew ").IsValid(), "magic Jewel excluded from duplicate key");
	Check(!BuildItemDuplicateKey(6, kFlagIdentified, 643, 0, L"Plague Whorl", "jew ").IsValid(), "rare Jewel excluded from duplicate key");
	Check(!BuildItemDuplicateKey(7, kFlagIdentified, 643, 390, L"Rainbow Facet", "jew ").IsValid(), "unique Jewel excluded from duplicate key");
	Check(!BuildItemDuplicateKey(7, kFlagIdentified, 700, 420, L"Defender's Fire", "cjw ").IsValid(), "Colossal Jewel excluded from duplicate key");

	// 4. SortDuplicateMatchEntries groups duplicates alphabetically while preserving stash page order within each group
	std::vector<MatchEntry> entries;
	MatchEntry              shakoSh2{};
	shakoSh2.shortName    = L"Harlequin Crest";
	shakoSh2.displayText  = L"Harlequin Crest - Sh.2";
	shakoSh2.target       = { true, 1 };
	shakoSh2.duplicateKey = shakoEliteTier;

	MatchEntry andySh5{};
	andySh5.shortName    = L"Andariel's Visage";
	andySh5.displayText  = L"Andariel's Visage - Sh.5";
	andySh5.target       = { true, 4 };
	andySh5.duplicateKey = BuildItemDuplicateKey(7, kFlagIdentified, 310, 320, L"Andariel's Visage");

	MatchEntry shakoSh9{};
	shakoSh9.shortName    = L"Harlequin Crest";
	shakoSh9.displayText  = L"Harlequin Crest - Sh.9";
	shakoSh9.target       = { true, 8 };
	shakoSh9.duplicateKey = shakoEliteTier;

	MatchEntry andySh1{};
	andySh1.shortName    = L"Andariel's Visage";
	andySh1.displayText  = L"Andariel's Visage - P";
	andySh1.target       = { false, 0 };
	andySh1.duplicateKey = andySh5.duplicateKey;

	// Push in stash scan order: Personal (Andy), Sh.2 (Shako), Sh.5 (Andy), Sh.9 (Shako)
	entries.push_back(andySh1);
	entries.push_back(shakoSh2);
	entries.push_back(andySh5);
	entries.push_back(shakoSh9);

	SortDuplicateMatchEntries(entries);
	Check(entries.size() == 4, "SortDuplicateMatchEntries preserves entry count");
	if (entries.size() == 4) {
		Check(entries[0].displayText == L"Andariel's Visage - P", "group 1 first item is Personal page");
		Check(entries[1].displayText == L"Andariel's Visage - Sh.5", "group 1 second item preserves page order Sh.5");
		Check(entries[2].displayText == L"Harlequin Crest - Sh.2", "group 2 first item is Sh.2");
		Check(entries[3].displayText == L"Harlequin Crest - Sh.9", "group 2 second item preserves page order Sh.9");
	}

	// 5. Multi-name duplicate groups ("Superior Monarch", "Monarch", "Cracked Monarch") stay contiguous
	std::vector<MatchEntry> baseEntries;
	MatchEntry              supMonP{};
	supMonP.shortName    = L"Superior Monarch";
	supMonP.displayText  = L"Superior Monarch - P";
	supMonP.duplicateKey = supMonarch;

	MatchEntry shakoMid{};
	shakoMid.shortName    = L"Harlequin Crest";
	shakoMid.displayText  = L"Harlequin Crest - Sh.2";
	shakoMid.duplicateKey = shakoEliteTier;

	MatchEntry normMonSh3{};
	normMonSh3.shortName    = L"Monarch";
	normMonSh3.displayText  = L"Monarch - Sh.3";
	normMonSh3.duplicateKey = normMonarch;

	MatchEntry crackedMonSh6{};
	crackedMonSh6.shortName    = L"Cracked Monarch";
	crackedMonSh6.displayText  = L"Cracked Monarch - Sh.6";
	crackedMonSh6.duplicateKey = BuildItemDuplicateKey(1, 0, 447, 0, L"Cracked Monarch", "uit ");

	MatchEntry shakoEnd{};
	shakoEnd.shortName    = L"Harlequin Crest";
	shakoEnd.displayText  = L"Harlequin Crest - Sh.8";
	shakoEnd.duplicateKey = shakoEliteTier;

	baseEntries.push_back(supMonP);
	baseEntries.push_back(shakoMid);
	baseEntries.push_back(normMonSh3);
	baseEntries.push_back(crackedMonSh6);
	baseEntries.push_back(shakoEnd);

	SortDuplicateMatchEntries(baseEntries);
	Check(baseEntries.size() == 5, "SortDuplicateMatchEntries preserves multi-name base count");
	if (baseEntries.size() == 5) {
		Check(baseEntries[0].displayText == L"Superior Monarch - P", "Monarch group sorts together under 'cracked monarch' while keeping Personal first");
		Check(baseEntries[1].displayText == L"Monarch - Sh.3", "Monarch group second item is Sh.3");
		Check(baseEntries[2].displayText == L"Cracked Monarch - Sh.6", "Monarch group third item is Sh.6");
		Check(baseEntries[3].displayText == L"Harlequin Crest - Sh.2", "Harlequin Crest group follows Monarch group cleanly");
		Check(baseEntries[4].displayText == L"Harlequin Crest - Sh.8", "Harlequin Crest group second item is Sh.8");
	}

	// 6. PruneSingletonDuplicateMatches removes lone 'dupe <modifier>' rows while keeping >=2 groups and OR-branch matches
	const auto negDupeFlags = InspectDuplicateQueryTerms(L"uni -dupe");
	Check(negDupeFlags.usesDuplicateTerm && !negDupeFlags.hasPositiveDuplicateTerm && negDupeFlags.hasNegativeDuplicateTerm,
		"'uni -dupe' sets hasNegativeDuplicateTerm without hasPositiveDuplicateTerm");

	std::vector<MatchEntry> pruneEntries;
	MatchEntry              lone4osMonarch{};
	lone4osMonarch.displayText             = L"Monarch - Sh.2";
	lone4osMonarch.count                   = 1;
	lone4osMonarch.duplicateKey            = normMonarch;
	lone4osMonarch.matchedWithoutDuplicate = false;
	lone4osMonarch.matchedUnits            = { 9001u };

	MatchEntry shakoDup1                   = shakoSh2;
	shakoDup1.count                        = 1;
	shakoDup1.matchedWithoutDuplicate      = false;
	shakoDup1.matchedUnits                 = { 9002u };

	MatchEntry shakoDup2                   = shakoSh9;
	shakoDup2.count                        = 1;
	shakoDup2.matchedWithoutDuplicate      = false;
	shakoDup2.matchedUnits                 = { 9003u };

	MatchEntry orBranchJah{};
	orBranchJah.displayText                = L"Jah Rune - P";
	orBranchJah.count                      = 1;
	orBranchJah.duplicateKey               = BuildItemDuplicateKey(2, 0, 639, 0, L"Jah Rune", "r31 ");
	orBranchJah.matchedWithoutDuplicate    = true;
	orBranchJah.matchedUnits               = { 9004u };

	pruneEntries.push_back(lone4osMonarch);
	pruneEntries.push_back(shakoDup1);
	pruneEntries.push_back(shakoDup2);
	pruneEntries.push_back(orBranchJah);

	std::vector<uintptr_t> prunedUnits;
	PruneSingletonDuplicateMatches(pruneEntries, &prunedUnits);
	Check(pruneEntries.size() == 3, "PruneSingletonDuplicateMatches drops singleton 'dupe 4os' match and keeps true duplicates + OR match");
	Check(prunedUnits.size() == 1 && prunedUnits[0] == 9001u, "PruneSingletonDuplicateMatches reports pruned unit pointer for grid dimming sync");
	if (pruneEntries.size() == 3) {
		Check(pruneEntries[0].displayText == L"Harlequin Crest - Sh.2", "kept duplicate #1");
		Check(pruneEntries[1].displayText == L"Harlequin Crest - Sh.9", "kept duplicate #2");
		Check(pruneEntries[2].displayText == L"Jah Rune - P", "kept OR-branch non-duplicate match");
	}
}

void TestOccludedCursorSuppressionPanelScoping() {
	using namespace StashSearch;
	constexpr uintptr_t kBankPanel      = 100u;
	constexpr uintptr_t kInventoryPanel = 200u;
	constexpr uintptr_t kPausePanel     = 300u;

	const auto noneActive = ComputeOccludedCursorSuppression(0, kInventoryPanel, true, kBankPanel, kBankPanel);
	Check(!noneActive.clearHoveredWidget && !noneActive.clearFocusedWidget, "inactive BankPanel never clears hover or focus");

	const auto bothBank = ComputeOccludedCursorSuppression(kBankPanel, 0, false, kBankPanel, kBankPanel);
	Check(bothBank.clearHoveredWidget && bothBank.clearFocusedWidget, "occluded BankPanel clears BankPanel hover and focus");

	const auto invFocusedOverSearch = ComputeOccludedCursorSuppression(kBankPanel, kInventoryPanel, false, kBankPanel, kInventoryPanel);
	Check(invFocusedOverSearch.clearHoveredWidget && !invFocusedOverSearch.clearFocusedWidget, "PlayerInventoryPanel controller focus is preserved when cursor rests on BankPanel search bar");

	const auto invOccludedByFlyout = ComputeOccludedCursorSuppression(kBankPanel, kInventoryPanel, true, kInventoryPanel, kInventoryPanel);
	Check(invOccludedByFlyout.clearHoveredWidget && invOccludedByFlyout.clearFocusedWidget, "PlayerInventoryPanel hover and controller focus are suppressed when occluded by expanded flyout or A-latch");

	const auto pauseFocused = ComputeOccludedCursorSuppression(kBankPanel, 0, true, kPausePanel, kPausePanel);
	Check(!pauseFocused.clearHoveredWidget && !pauseFocused.clearFocusedWidget, "PausePanel hover and controller focus are never cleared by Stash Search");
}

} // namespace

int main() {
	TestTooltipNormalization();
	TestCaseInsensitiveMatching();
	TestBooleanAndShorthandQuery();
	TestSlashPageCommands();
	TestDropdownScrollMath();
	TestScaleHelpers();
	TestConfigPersistence();
	TestIsolatedDropdownMatchFilter();
	TestBuildItemTooltipLines();
	TestActiveSearchOverlayHitTest();
	TestFlyoutChevronDirection();
	TestControllerFlyoutCursorSnapAndRestore();
	TestXInputPollCacheAndActivePad();
	TestDuplicateSearchAndGrouping();
	TestOccludedCursorSuppressionPanelScoping();
	std::puts("PASS: Stash Search unit tests (tooltip normalization, case folding, boolean/shorthand queries, slash commands, scroll math, scaling, config persistence, isolated dropdown match filter, multi-line item tooltip builder, overlay hit test, flyout chevron direction, controller cursor snap/restore, XInput poll cache, duplicate search & grouping, panel-scoped cursor suppression).");
	return 0;
}
