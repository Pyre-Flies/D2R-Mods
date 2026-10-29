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
	Check(NormalizeWideTooltipLine(L"\x00FF" L"c4Shako\x00FF" L"c0\r\n  +2 To All Skills *sock") == L"Shako +2 To All Skills",
		"wide D2R color codes, whitespace collapsing, and '*' truncation");
	Check(NormalizeWideTooltipLine(L"\x00C3\x00BF" L"c2  Enigma  \n +1 To Teleport ") == L"Enigma +1 To Teleport",
		"wide 0x00C3 0x00BF cX sequence stripping");
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

} // namespace

int main() {
	TestTooltipNormalization();
	TestCaseInsensitiveMatching();
	TestBooleanAndShorthandQuery();
	TestSlashPageCommands();
	TestDropdownScrollMath();
	TestScaleHelpers();
	TestConfigPersistence();
	std::puts("PASS: Stash Search unit tests (tooltip normalization, case folding, boolean/shorthand queries, slash commands, scroll math, scaling, config persistence).");
	return 0;
}
