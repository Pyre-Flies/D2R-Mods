#include "character_cache.h"

#include <charconv>

namespace StashSearch {

namespace {

constexpr std::string_view kSnapshotHeader = "# pyre-stashsearch character snapshot";

void AppendUtf8CodePoint(std::string& out, uint32_t cp) {
	if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) {
		cp = 0xFFFDu;
	}
	if (cp < 0x80u) {
		out.push_back(static_cast<char>(cp));
	} else if (cp < 0x800u) {
		out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
		out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
	} else if (cp < 0x10000u) {
		out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
		out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
		out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
	} else {
		out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
		out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
		out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
		out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
	}
}

void AppendWideCodePoint(std::wstring& out, uint32_t cp) {
	if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) {
		cp = 0xFFFDu;
	}
	if constexpr (sizeof(wchar_t) == 2) {
		if (cp >= 0x10000u) {
			cp -= 0x10000u;
			out.push_back(static_cast<wchar_t>(0xD800u + (cp >> 10)));
			out.push_back(static_cast<wchar_t>(0xDC00u + (cp & 0x3FFu)));
			return;
		}
	}
	out.push_back(static_cast<wchar_t>(cp));
}

// Escapes '\\', '\t', '\r' and '\n' so one item fits on one tab-separated line.
void AppendEscapedField(std::string& out, std::wstring_view field) {
	if (field.size() > kMaxSnapshotFieldChars) {
		field = field.substr(0, kMaxSnapshotFieldChars);
	}
	for (const char ch : CacheWideToUtf8(field)) {
		switch (ch) {
		case '\\': out.append("\\\\"); break;
		case '\t': out.append("\\t"); break;
		case '\r': out.append("\\r"); break;
		case '\n': out.append("\\n"); break;
		default:   out.push_back(ch); break;
		}
	}
}

auto UnescapeField(std::string_view field, std::wstring& out) -> bool {
	std::string raw;
	raw.reserve(field.size());
	for (size_t i = 0; i < field.size(); ++i) {
		const char ch = field[i];
		if (ch != '\\') {
			raw.push_back(ch);
			continue;
		}
		if (i + 1 >= field.size()) {
			return false;
		}
		const char next = field[++i];
		switch (next) {
		case '\\': raw.push_back('\\'); break;
		case 't':  raw.push_back('\t'); break;
		case 'r':  raw.push_back('\r'); break;
		case 'n':  raw.push_back('\n'); break;
		default:   return false;
		}
	}
	out = CacheUtf8ToWide(raw);
	if (out.size() > kMaxSnapshotFieldChars) {
		out.resize(kMaxSnapshotFieldChars);
	}
	return true;
}

auto SplitTabs(std::string_view line) -> std::vector<std::string_view> {
	std::vector<std::string_view> parts;
	size_t                        start = 0;
	while (true) {
		const size_t tab = line.find('\t', start);
		if (tab == std::string_view::npos) {
			parts.push_back(line.substr(start));
			break;
		}
		parts.push_back(line.substr(start, tab - start));
		start = tab + 1;
	}
	return parts;
}

auto ParseU64(std::string_view text, uint64_t* out) noexcept -> bool {
	if (text.empty()) {
		return false;
	}
	uint64_t   value = 0;
	const auto res   = std::from_chars(text.data(), text.data() + text.size(), value);
	if (res.ec != std::errc{} || res.ptr != text.data() + text.size()) {
		return false;
	}
	*out = value;
	return true;
}

} // namespace

auto ClassifyCharacterItem(uint8_t nativePage, uint32_t itemMode, ItemSource* outSource) noexcept -> bool {
	if (outSource == nullptr) {
		return false;
	}
	switch (itemMode) {
	case kItemModeStored:
		if (nativePage == kNativePageInventory) {
			*outSource = ItemSource::Inventory;
			return true;
		}
		if (nativePage == kNativePageCube) {
			*outSource = ItemSource::Cube;
			return true;
		}
		if (nativePage == kNativePageStash) {
			*outSource = ItemSource::PersonalStash;
			return true;
		}
		// Pages 1 (equip) and 2 (trade) are never "stored" storage; 255 is no page.
		if (nativePage > kNativePageStash && nativePage != kNativePageNone) {
			*outSource = ItemSource::ExtraPage;
			return true;
		}
		return false;
	case kItemModeEquipped:
		*outSource = ItemSource::Equipped;
		return true;
	case kItemModeBelt:
		*outSource = ItemSource::Belt;
		return true;
	case kItemModeCursor:
		*outSource = ItemSource::Cursor;
		return true;
	default: // ground, dropping, socketed, unknown
		return false;
	}
}

auto ItemSourceCode(ItemSource source) noexcept -> std::string_view {
	switch (source) {
	case ItemSource::PersonalStash: return "stash";
	case ItemSource::Inventory:     return "inv";
	case ItemSource::Cube:          return "cube";
	case ItemSource::Equipped:      return "worn";
	case ItemSource::Belt:          return "belt";
	case ItemSource::Cursor:        return "cursor";
	case ItemSource::ExtraPage:     return "page";
	}
	return "inv";
}

auto ParseItemSourceCode(std::string_view code, ItemSource* outSource) noexcept -> bool {
	constexpr ItemSource kAll[] = {
		ItemSource::PersonalStash, ItemSource::Inventory, ItemSource::Cube,     ItemSource::Equipped,
		ItemSource::Belt,          ItemSource::Cursor,    ItemSource::ExtraPage,
	};
	for (const ItemSource s : kAll) {
		if (ItemSourceCode(s) == code) {
			*outSource = s;
			return true;
		}
	}
	return false;
}

auto ItemSourceShortLabel(ItemSource source) noexcept -> std::wstring_view {
	switch (source) {
	case ItemSource::PersonalStash: return L"P";
	case ItemSource::Inventory:     return L"Inv";
	case ItemSource::Cube:          return L"Cube";
	case ItemSource::Equipped:      return L"Worn";
	case ItemSource::Belt:          return L"Belt";
	case ItemSource::Cursor:        return L"Cursor";
	case ItemSource::ExtraPage:     return L"Page";
	}
	return L"?";
}

auto ItemSourceFullLabel(ItemSource source) noexcept -> std::wstring_view {
	switch (source) {
	case ItemSource::PersonalStash: return L"Personal Stash";
	case ItemSource::Inventory:     return L"Inventory";
	case ItemSource::Cube:          return L"Horadric Cube";
	case ItemSource::Equipped:      return L"Equipped";
	case ItemSource::Belt:          return L"Belt";
	case ItemSource::Cursor:        return L"Cursor";
	case ItemSource::ExtraPage:     return L"Extra Page";
	}
	return L"Unknown";
}

auto IsValidCharacterName(std::wstring_view name) noexcept -> bool {
	if (name.size() < 2 || name.size() > kMaxCharacterNameChars) {
		return false;
	}
	if (name.front() == L' ' || name.back() == L' ' || name.back() == L'.') {
		return false;
	}
	for (const wchar_t ch : name) {
		const auto u = static_cast<uint32_t>(ch);
		if (u < 0x20u || u == 0x7Fu) {
			return false;
		}
		switch (ch) {
		case L'\\': case L'/': case L':': case L'*': case L'?':
		case L'"':  case L'<': case L'>': case L'|': case L'.':
			return false;
		default:
			break;
		}
		if (u >= 0xD800u && u <= 0xDFFFu) {
			return false;
		}
	}
	return true;
}

auto SerializeCharacterSnapshot(const CharacterSnapshot& snapshot) -> std::string {
	std::string out;
	out.reserve(256 + snapshot.items.size() * 256);
	out.append(kSnapshotHeader);
	out.push_back('\n');
	out.append("version\t");
	out.append(std::to_string(kCharacterSnapshotFormatVersion));
	out.push_back('\n');
	out.append("name\t");
	AppendEscapedField(out, snapshot.name);
	out.push_back('\n');
	out.append("saved\t");
	out.append(std::to_string(snapshot.savedUnixSeconds));
	out.push_back('\n');
	size_t written = 0;
	for (const auto& item : snapshot.items) {
		if (written >= kMaxSnapshotItems) {
			break;
		}
		out.append("item\t");
		out.append(ItemSourceCode(item.source));
		out.push_back('\t');
		AppendEscapedField(out, item.shortName);
		out.push_back('\t');
		AppendEscapedField(out, item.fullName);
		out.push_back('\t');
		AppendEscapedField(out, item.searchText);
		out.push_back('\n');
		++written;
	}
	return out;
}

auto ParseCharacterSnapshot(std::string_view text, CharacterSnapshot& outSnapshot) -> bool {
	outSnapshot = CharacterSnapshot{};
	if (text.size() > kMaxSnapshotFileBytes) {
		return false;
	}
	if (text.size() >= 3 &&
		static_cast<uint8_t>(text[0]) == 0xEF &&
		static_cast<uint8_t>(text[1]) == 0xBB &&
		static_cast<uint8_t>(text[2]) == 0xBF) {
		text.remove_prefix(3);
	}
	bool sawVersion = false;
	bool sawName    = false;
	for (size_t cursor = 0; cursor < text.size();) {
		const size_t     newline = text.find('\n', cursor);
		const size_t     end     = (newline == std::string_view::npos) ? text.size() : newline;
		std::string_view line    = text.substr(cursor, end - cursor);
		cursor                   = (newline == std::string_view::npos) ? text.size() : newline + 1;
		if (!line.empty() && line.back() == '\r') {
			line.remove_suffix(1);
		}
		if (line.empty() || line.front() == '#') {
			continue;
		}
		const auto parts = SplitTabs(line);
		const auto key   = parts[0];
		if (key == "version") {
			uint64_t version = 0;
			if (parts.size() != 2 || !ParseU64(parts[1], &version) || version != kCharacterSnapshotFormatVersion) {
				return false; // Unknown future format: ignore the whole file.
			}
			sawVersion = true;
		} else if (key == "name") {
			if (parts.size() != 2 || !UnescapeField(parts[1], outSnapshot.name)) {
				return false;
			}
			sawName = true;
		} else if (key == "saved") {
			if (parts.size() != 2 || !ParseU64(parts[1], &outSnapshot.savedUnixSeconds)) {
				return false;
			}
		} else if (key == "item") {
			if (!sawVersion || parts.size() != 5) {
				continue; // Skip malformed rows rather than dropping the whole character.
			}
			CachedCharacterItem item{};
			if (!ParseItemSourceCode(parts[1], &item.source) ||
				!UnescapeField(parts[2], item.shortName) ||
				!UnescapeField(parts[3], item.fullName) ||
				!UnescapeField(parts[4], item.searchText) ||
				item.searchText.empty()) {
				continue;
			}
			if (outSnapshot.items.size() < kMaxSnapshotItems) {
				outSnapshot.items.push_back(std::move(item));
			}
		}
		// Unknown keys are ignored so newer writers can add optional metadata.
	}
	return sawVersion && sawName && IsValidCharacterName(outSnapshot.name);
}

auto FormatSnapshotAge(uint64_t nowUnixSeconds, uint64_t savedUnixSeconds) -> std::wstring {
	if (savedUnixSeconds == 0) {
		return L"unknown age";
	}
	const uint64_t delta = nowUnixSeconds > savedUnixSeconds ? nowUnixSeconds - savedUnixSeconds : 0;
	if (delta < 60) {
		return L"just now";
	}
	if (delta < 3600) {
		return std::to_wstring(delta / 60) + L" min ago";
	}
	if (delta < 86400) {
		return std::to_wstring(delta / 3600) + L" h ago";
	}
	const uint64_t days = delta / 86400;
	return std::to_wstring(days) + (days == 1 ? L" day ago" : L" days ago");
}

auto CharacterNamesEqual(std::wstring_view a, std::wstring_view b) noexcept -> bool {
	if (a.size() != b.size()) {
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i) {
		wchar_t ca = a[i];
		wchar_t cb = b[i];
		if (ca >= L'A' && ca <= L'Z') {
			ca = static_cast<wchar_t>(ca - L'A' + L'a');
		}
		if (cb >= L'A' && cb <= L'Z') {
			cb = static_cast<wchar_t>(cb - L'A' + L'a');
		}
		if (ca != cb) {
			return false;
		}
	}
	return true;
}

auto CacheUtf8ToWide(std::string_view utf8) -> std::wstring {
	std::wstring out;
	out.reserve(utf8.size());
	size_t i = 0;
	while (i < utf8.size()) {
		const auto b0 = static_cast<uint8_t>(utf8[i]);
		uint32_t   cp = 0;
		size_t     len = 0;
		if (b0 < 0x80u) {
			cp  = b0;
			len = 1;
		} else if ((b0 & 0xE0u) == 0xC0u) {
			cp  = b0 & 0x1Fu;
			len = 2;
		} else if ((b0 & 0xF0u) == 0xE0u) {
			cp  = b0 & 0x0Fu;
			len = 3;
		} else if ((b0 & 0xF8u) == 0xF0u) {
			cp  = b0 & 0x07u;
			len = 4;
		} else {
			AppendWideCodePoint(out, 0xFFFDu);
			++i;
			continue;
		}
		if (i + len > utf8.size()) {
			AppendWideCodePoint(out, 0xFFFDu);
			break;
		}
		bool ok = true;
		for (size_t k = 1; k < len; ++k) {
			const auto b = static_cast<uint8_t>(utf8[i + k]);
			if ((b & 0xC0u) != 0x80u) {
				ok = false;
				break;
			}
			cp = (cp << 6) | (b & 0x3Fu);
		}
		const bool overlong = (len == 2 && cp < 0x80u) || (len == 3 && cp < 0x800u) || (len == 4 && cp < 0x10000u);
		if (!ok || overlong) {
			AppendWideCodePoint(out, 0xFFFDu);
			++i;
			continue;
		}
		AppendWideCodePoint(out, cp);
		i += len;
	}
	return out;
}

auto CacheWideToUtf8(std::wstring_view wide) -> std::string {
	std::string out;
	out.reserve(wide.size());
	for (size_t i = 0; i < wide.size(); ++i) {
		auto cp = static_cast<uint32_t>(wide[i]);
		if constexpr (sizeof(wchar_t) == 2) {
			if (cp >= 0xD800u && cp <= 0xDBFFu && i + 1 < wide.size()) {
				const auto lo = static_cast<uint32_t>(wide[i + 1]);
				if (lo >= 0xDC00u && lo <= 0xDFFFu) {
					cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
					++i;
				}
			}
		}
		AppendUtf8CodePoint(out, cp);
	}
	return out;
}

} // namespace StashSearch
