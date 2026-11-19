#pragma once
// Read and change one "key = ..." line of a plugin's config (flat or inside a [section]),
// leaving the rest of the file alone. No SDK or Windows state, so it is tested off line.
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace D2RLSettings {

namespace Detail {

inline auto TrimWhitespace(std::string_view text) noexcept -> std::string_view {
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos) return {};
	const size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

inline auto StripInlineComment(std::string_view text) noexcept -> std::string_view {
	bool inDoubleQuotes = false;
	bool inSingleQuotes = false;
	for (size_t i = 0; i < text.size(); ++i) {
		if (inDoubleQuotes && text[i] == '\\') {
			if (i + 1 < text.size()) ++i;
			continue;
		}
		if (text[i] == '"' && !inSingleQuotes) {
			inDoubleQuotes = !inDoubleQuotes;
		} else if (text[i] == '\'' && !inDoubleQuotes) {
			inSingleQuotes = !inSingleQuotes;
		} else if (text[i] == '#' && !inDoubleQuotes && !inSingleQuotes) {
			return TrimWhitespace(text.substr(0, i));
		}
	}
	return TrimWhitespace(text);
}

// Parses "[section]" and "[[array]]" header lines. For "[[array]]" table headers, returns
// true with isArrayTableOut = true and sectionOut empty so array tables leave the current
// section and mark the end of top-level keys without matching any [section].
inline auto ParseSectionHeader(std::string_view line, std::string_view& sectionOut, bool& isArrayTableOut) noexcept -> bool {
	const std::string_view stripped = StripInlineComment(line);
	if (stripped.size() < 3 || stripped.front() != '[' || stripped.back() != ']') return false;
	if (stripped.size() >= 5 && stripped[1] == '[' && stripped[stripped.size() - 2] == ']') {
		const std::string_view inner = TrimWhitespace(stripped.substr(2, stripped.size() - 4));
		if (inner.empty()) return false;
		sectionOut      = {};
		isArrayTableOut = true;
		return true;
	}
	if (stripped[1] == '[' || stripped[stripped.size() - 2] == ']') return false;
	sectionOut      = TrimWhitespace(stripped.substr(1, stripped.size() - 2));
	isArrayTableOut = false;
	return !sectionOut.empty();
}

// Allows either an explicit section parameter or dotted "section.key" notation.
inline void SplitSectionAndKey(std::string_view keyOrPath, std::string_view explicitSection,
                               std::string_view& sectionOut, std::string_view& keyOut) noexcept {
	const std::string_view trimmedSection = TrimWhitespace(explicitSection);
	const std::string_view trimmedKey     = TrimWhitespace(keyOrPath);
	if (!trimmedSection.empty()) {
		sectionOut = trimmedSection;
		keyOut     = trimmedKey;
		return;
	}
	const size_t dot = trimmedKey.rfind('.');
	if (dot != std::string_view::npos && dot > 0 && dot + 1 < trimmedKey.size()) {
		sectionOut = TrimWhitespace(trimmedKey.substr(0, dot));
		keyOut     = TrimWhitespace(trimmedKey.substr(dot + 1));
		return;
	}
	sectionOut = {};
	keyOut     = trimmedKey;
}

inline auto ParseBoolValue(std::string_view line, size_t valueAt, bool& out) noexcept -> bool {
	if (valueAt == std::string_view::npos) return false;
	const std::string_view token = StripInlineComment(line.substr(valueAt));
	if (token == "true") {
		out = true;
		return true;
	}
	if (token == "false") {
		out = false;
		return true;
	}
	return false;
}

inline auto ParseIntValue(std::string_view line, size_t valueAt, int32_t& out) noexcept -> bool {
	if (valueAt == std::string_view::npos) return false;
	std::string_view token = StripInlineComment(line.substr(valueAt));
	if (token.empty()) return false;

	bool negative = false;
	if (token.front() == '+' || token.front() == '-') {
		negative = (token.front() == '-');
		token.remove_prefix(1);
	}
	if (token.empty()) return false;

	int64_t acc          = 0;
	bool    anyDigits    = false;
	bool    inFraction   = false;
	bool    anyFracDigit = false;
	for (const char c : token) {
		if (c == '_') continue;
		if (c == '.' && !inFraction && anyDigits) {
			inFraction = true;
			continue;
		}
		if (c < '0' || c > '9') return false;
		if (inFraction) {
			if (c != '0') return false;
			anyFracDigit = true;
			continue;
		}
		anyDigits = true;
		acc       = acc * 10 + (c - '0');
		if (!negative && acc > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) return false;
		if (negative && -acc < static_cast<int64_t>(std::numeric_limits<int32_t>::min())) return false;
	}
	if (!anyDigits) return false;
	if (inFraction && !anyFracDigit) return false;
	out = static_cast<int32_t>(negative ? -acc : acc);
	return true;
}

inline auto ParseStringValue(std::string_view line, size_t valueAt, std::string& out) -> bool {
	if (valueAt == std::string_view::npos) return false;
	const std::string_view token = StripInlineComment(line.substr(valueAt));
	if (token.empty()) return false;
	if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
		const std::string_view inner = token.substr(1, token.size() - 2);
		std::string            unescaped;
		unescaped.reserve(inner.size());
		for (size_t i = 0; i < inner.size(); ++i) {
			if (inner[i] == '\\' && i + 1 < inner.size()) {
				const char next = inner[++i];
				if (next == 'n') {
					unescaped += '\n';
				} else if (next == 't') {
					unescaped += '\t';
				} else if (next == 'r') {
					unescaped += '\r';
				} else {
					unescaped += next;
				}
			} else {
				unescaped += inner[i];
			}
		}
		out = std::move(unescaped);
		return true;
	}
	if (token.size() >= 2 && token.front() == '\'' && token.back() == '\'') {
		out = std::string(token.substr(1, token.size() - 2));
		return true;
	}
	out = std::string(token);
	return true;
}

inline auto EscapeTomlString(std::string_view text) -> std::string {
	std::string out;
	out.reserve(text.size() + 2);
	out += '"';
	for (const char c : text) {
		if (c == '"' || c == '\\') {
			out += '\\';
			out += c;
		} else if (c == '\n') {
			out += "\\n";
		} else if (c == '\r') {
			out += "\\r";
		} else if (c == '\t') {
			out += "\\t";
		} else {
			out += c;
		}
	}
	out += '"';
	return out;
}

inline auto DetectNewline(std::string_view toml) noexcept -> std::string_view {
	return toml.find("\r\n") != std::string_view::npos ? "\r\n" : "\n";
}

inline auto WithKeyValue(const std::string& toml, std::string_view keyOrPath, std::string_view formattedValue,
                         std::string_view section = {}) -> std::string {
	std::string_view targetSection;
	std::string_view targetKey;
	SplitSectionAndKey(keyOrPath, section, targetSection, targetKey);

	const std::string      wanted  = std::string(targetKey) + " = " + std::string(formattedValue);
	const std::string_view newline = DetectNewline(toml);

	std::string_view currentSection;
	bool             inArrayTable           = false;
	size_t           firstSectionStart      = std::string::npos;
	bool             inTargetSection        = targetSection.empty();
	bool             targetSectionSeen      = targetSection.empty();
	size_t           targetSectionInsertPos = std::string::npos;

	size_t start = 0;
	while (start < toml.size()) {
		const size_t           end      = std::min(toml.find('\n', start), toml.size());
		const size_t           nextLine = std::min(end + 1, toml.size());
		const std::string_view line     = std::string_view(toml).substr(start, end - start);

		std::string_view headerSection;
		bool             isArrayTable = false;
		if (ParseSectionHeader(line, headerSection, isArrayTable)) {
			if (firstSectionStart == std::string::npos) firstSectionStart = start;
			inArrayTable    = isArrayTable;
			currentSection  = headerSection;
			inTargetSection = (!inArrayTable && !targetSection.empty() && currentSection == targetSection);
			if (inTargetSection) {
				targetSectionSeen      = true;
				targetSectionInsertPos = nextLine;
			}
			start = nextLine;
			continue;
		}

		const size_t first = line.find_first_not_of(" \t");
		if (first != std::string_view::npos && line.substr(first, targetKey.size()) == targetKey) {
			const size_t equals = line.find_first_not_of(" \t", first + targetKey.size());
			if (equals != std::string_view::npos && line[equals] == '=') {
				const bool carriageReturn = !line.empty() && line.back() == '\r';
				if (!inArrayTable && currentSection == targetSection) {
					return toml.substr(0, start) + wanted + (carriageReturn ? "\r" : "") + toml.substr(end);
				}
			}
		}

		if (inTargetSection && !TrimWhitespace(line).empty()) {
			targetSectionInsertPos = nextLine;
		}
		start = nextLine;
	}

	if (targetSection.empty()) {
		if (firstSectionStart != std::string::npos) {
			std::string prefix = toml.substr(0, firstSectionStart);
			if (!prefix.empty() && prefix.back() != '\n') prefix += newline;
			return prefix + wanted + std::string(newline) + toml.substr(firstSectionStart);
		}
		std::string updated = toml;
		if (!updated.empty() && updated.back() != '\n') updated += newline;
		return updated + wanted + std::string(newline);
	}

	if (targetSectionSeen && targetSectionInsertPos != std::string::npos) {
		std::string prefix = toml.substr(0, targetSectionInsertPos);
		if (!prefix.empty() && prefix.back() != '\n') prefix += newline;
		return prefix + wanted + std::string(newline) + toml.substr(targetSectionInsertPos);
	}

	std::string updated = toml;
	if (!updated.empty() && updated.back() != '\n') updated += newline;
	if (!updated.empty()) updated += newline;
	updated += "[" + std::string(targetSection) + "]" + std::string(newline);
	updated += wanted + std::string(newline);
	return updated;
}

} // namespace Detail

// [begin, end) of the line's value when the line sets key, else false.
inline auto FindBoolLine(std::string_view line, std::string_view key, size_t& valueAt) noexcept -> bool {
	const size_t first = line.find_first_not_of(" \t");
	if (first == std::string_view::npos || line.substr(first, key.size()) != key) return false;
	const size_t equals = line.find_first_not_of(" \t", first + key.size());
	if (equals == std::string_view::npos || line[equals] != '=') return false;
	valueAt = line.find_first_not_of(" \t", equals + 1);
	return true;
}

// The value of a "key = ..." line (top-level or inside [section]), or fallback.
inline auto ReadBool(std::string_view toml, std::string_view key, bool fallback, std::string_view section = {}) noexcept -> bool {
	std::string_view targetSection;
	std::string_view targetKey;
	Detail::SplitSectionAndKey(key, section, targetSection, targetKey);

	std::string_view currentSection;
	bool             inArrayTable = false;

	size_t start = 0;
	while (start < toml.size()) {
		const size_t           end  = std::min(toml.find('\n', start), toml.size());
		const std::string_view line = toml.substr(start, end - start);
		start                       = end + 1;

		std::string_view headerSection;
		bool             isArrayTable = false;
		if (Detail::ParseSectionHeader(line, headerSection, isArrayTable)) {
			inArrayTable   = isArrayTable;
			currentSection = headerSection;
			continue;
		}

		size_t valueAt = 0;
		if (!FindBoolLine(line, targetKey, valueAt) || valueAt == std::string_view::npos) continue;
		if (!inArrayTable && currentSection == targetSection) {
			bool parsed = fallback;
			return Detail::ParseBoolValue(line, valueAt, parsed) ? parsed : fallback;
		}
	}
	return fallback;
}

// The same text with the boolean value changed (in its [section] if given), or added.
inline auto WithBool(const std::string& toml, std::string_view key, bool value, std::string_view section = {}) -> std::string {
	return Detail::WithKeyValue(toml, key, value ? "true" : "false", section);
}

// The integer value of a "key = ..." line (top-level or inside [section]), or fallback.
inline auto ReadInt(std::string_view toml, std::string_view key, int32_t fallback, std::string_view section = {}) noexcept -> int32_t {
	std::string_view targetSection;
	std::string_view targetKey;
	Detail::SplitSectionAndKey(key, section, targetSection, targetKey);

	std::string_view currentSection;
	bool             inArrayTable = false;

	size_t start = 0;
	while (start < toml.size()) {
		const size_t           end  = std::min(toml.find('\n', start), toml.size());
		const std::string_view line = toml.substr(start, end - start);
		start                       = end + 1;

		std::string_view headerSection;
		bool             isArrayTable = false;
		if (Detail::ParseSectionHeader(line, headerSection, isArrayTable)) {
			inArrayTable   = isArrayTable;
			currentSection = headerSection;
			continue;
		}

		size_t valueAt = 0;
		if (!FindBoolLine(line, targetKey, valueAt) || valueAt == std::string_view::npos) continue;
		if (!inArrayTable && currentSection == targetSection) {
			int32_t parsed = fallback;
			return Detail::ParseIntValue(line, valueAt, parsed) ? parsed : fallback;
		}
	}
	return fallback;
}

// The same text with the integer value changed (in its [section] if given), or added.
inline auto WithInt(const std::string& toml, std::string_view key, int32_t value, std::string_view section = {}) -> std::string {
	return Detail::WithKeyValue(toml, key, std::to_string(value), section);
}

// The string value of a "key = ..." line (top-level or inside [section]), or fallback.
inline auto ReadString(std::string_view toml, std::string_view key, std::string_view fallback, std::string_view section = {}) -> std::string {
	std::string_view targetSection;
	std::string_view targetKey;
	Detail::SplitSectionAndKey(key, section, targetSection, targetKey);

	std::string_view currentSection;
	bool             inArrayTable = false;

	size_t start = 0;
	while (start < toml.size()) {
		const size_t           end  = std::min(toml.find('\n', start), toml.size());
		const std::string_view line = toml.substr(start, end - start);
		start                       = end + 1;

		std::string_view headerSection;
		bool             isArrayTable = false;
		if (Detail::ParseSectionHeader(line, headerSection, isArrayTable)) {
			inArrayTable   = isArrayTable;
			currentSection = headerSection;
			continue;
		}

		size_t valueAt = 0;
		if (!FindBoolLine(line, targetKey, valueAt) || valueAt == std::string_view::npos) continue;
		if (!inArrayTable && currentSection == targetSection) {
			std::string parsed;
			return Detail::ParseStringValue(line, valueAt, parsed) ? parsed : std::string(fallback);
		}
	}
	return std::string(fallback);
}

// The same text with the string value changed (in its [section] if given), or added.
inline auto WithString(const std::string& toml, std::string_view key, std::string_view value, std::string_view section = {}) -> std::string {
	return Detail::WithKeyValue(toml, key, Detail::EscapeTomlString(value), section);
}

} // namespace D2RLSettings
