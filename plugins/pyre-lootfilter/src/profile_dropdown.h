#pragma once
// Status text formatting, TOML string persistence, and native D2R 3.x LootFilterConfig
// active .fltr check for pyre-lootfilter's Options > Gameplay (D2RL::Settings) rows.

#include "filter_schema.h"
#include "unit_reader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>

namespace PyreLootFilter::ProfileDropdown {

namespace Rva {
// Native D2R 3.x LootFilterConfig singleton & active .fltr profile name
// (D2R 3.3.93847 loader-2026-09-17 / 3.3.94995 loader-1.3.1-beta)
inline constexpr std::uint32_t GetLootFilterConfig = 0x1BF9D0;
inline constexpr std::uint32_t LootFilterConfigSingleton = 0x2358E20;
inline constexpr std::size_t OffsetActiveProfileName = 0x20;
inline constexpr std::size_t OffsetActiveProfileLen = 0x28;
} // namespace Rva

namespace Witnesses {
inline constexpr std::array<std::uint8_t, 16> GetLootFilterConfigEntry{
    0x48, 0x83, 0xEC, 0x28, 0x65, 0x48, 0x8B, 0x04,
    0x25, 0x58, 0x00, 0x00, 0x00, 0x8B, 0x0D, 0xE5};
} // namespace Witnesses

[[nodiscard]] inline bool IsInGameFilterActive(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    std::uint64_t strLen = 0;
    const auto cfgAddr = exeBase + Rva::LootFilterConfigSingleton;
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(cfgAddr + Rva::OffsetActiveProfileLen),
                              &strLen, sizeof(strLen))) {
        return false;
    }
    return strLen > 0;
}

[[nodiscard]] inline std::string ReadActiveInGameFilterName(std::uintptr_t exeBase) {
    if (!exeBase) return {};
    const auto cfgAddr = exeBase + Rva::LootFilterConfigSingleton;
    const char* strPtr = nullptr;
    std::uint64_t strLen = 0;
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(cfgAddr + Rva::OffsetActiveProfileName),
                              &strPtr, sizeof(strPtr)) ||
        !Reader::CopyReadable(reinterpret_cast<const void*>(cfgAddr + Rva::OffsetActiveProfileLen),
                              &strLen, sizeof(strLen)) ||
        !strPtr || strLen == 0) {
        return {};
    }
    const std::size_t clampedLen =
        static_cast<std::size_t>(std::min<std::uint64_t>(strLen, 64ULL));
    char buf[65]{};
    if (!Reader::CopyReadable(strPtr, buf, clampedLen)) {
        return {};
    }
    buf[clampedLen] = '\0';
    return std::string(buf, strnlen_s(buf, clampedLen));
}

[[nodiscard]] inline std::string SummarizeErrorForPanelBox(std::string_view lastError) {
    if (lastError.find("Version mismatch") != std::string_view::npos) {
        return "Filter Error: Version mismatch";
    }
    if (lastError.find("Mod/Season mismatch") != std::string_view::npos) {
        return "Filter Error: Mod or Season mismatch";
    }
    if (lastError.find("Invalid Filter Format") != std::string_view::npos ||
        lastError.find("browser-library") != std::string_view::npos) {
        return "Filter Error: Export single filter JSON";
    }
    if (lastError.find("Invalid JSON syntax") != std::string_view::npos) {
        return "Filter Error: Invalid JSON syntax";
    }
    if (lastError.find("exceeds 4 MB") != std::string_view::npos) {
        return "Filter Error: File exceeds 4 MB limit";
    }
    if (lastError.find("Could not open") != std::string_view::npos) {
        return "Filter Error: Could not open filter file";
    }
    const auto sanitized = Schema::SanitizeDisplayAscii(lastError, 28);
    if (!sanitized.empty()) {
        return "Filter Error: " + sanitized;
    }
    return "Filter Error: Failed to load filter";
}

inline std::string BuildPanelStatusText(
    bool enabled, const Schema::FilterRuleTable* activeTable,
    std::string_view activeShortPath, std::string_view lastError,
    std::string_view /*failedProfileLabel*/,
    std::string_view activeInGameFilter = {}) {
    if (!activeInGameFilter.empty()) {
        const std::string shortInGame = Schema::SanitizeDisplayAscii(activeInGameFilter, 20);
        if (!shortInGame.empty()) {
            return "Disabled (In-Game: " + shortInGame + ")";
        }
        return "Loot Filter Disabled";
    }
    if (!enabled) {
        return "Loot Filter Disabled";
    }

    if (!lastError.empty()) {
        return SummarizeErrorForPanelBox(lastError);
    }

    if (activeTable) {
        const std::string rawName =
            !activeTable->filterName.empty()
                ? activeTable->filterName
                : (!activeShortPath.empty()
                       ? std::filesystem::path(activeShortPath).stem().string()
                       : std::string("filter"));
        std::string shortName = Schema::SanitizeDisplayAscii(rawName, 20);
        if (shortName.empty()) {
            shortName = "filter";
        }
        const auto ruleCount = activeTable->rules.size();
        return "Filter Loaded: " + shortName + " (" + std::to_string(ruleCount) +
               (ruleCount == 1 ? " rule)" : " rules)");
    }

    return "No filter found in config/filter/";
}

[[nodiscard]] inline std::string_view TrimTomlSlice(std::string_view s) noexcept {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1U);
}

inline std::string UpdateTomlString(std::string_view text, std::string_view section,
                                    std::string_view key, std::string_view value) {
    // Sanitize quotes/control chars from value
    std::string cleanVal;
    cleanVal.reserve(value.size());
    for (char c : value) {
        if (c == '"' || c == '\\' || c == '\r' || c == '\n') continue;
        cleanVal.push_back(c);
    }
    const std::string quoted = "\"" + cleanVal + "\"";
    const std::string header = "[" + std::string(section) + "]";
    const std::string_view eol = (text.find("\r\n") != text.npos) ? "\r\n" : "\n";

    bool inside = false;
    bool foundSection = false;
    std::size_t sectionInsertPos = text.size();

    for (std::size_t cursor = 0; cursor < text.size();) {
        const auto nl = text.find('\n', cursor);
        const auto end = (nl == text.npos) ? text.size() : nl;
        auto line = text.substr(cursor, end - cursor);
        const std::size_t bom = (cursor == 0 && line.starts_with("\xEF\xBB\xBF")) ? 3U : 0U;
        line.remove_prefix(bom);
        const auto comment = line.find('#');
        const auto logical = TrimTomlSlice(line.substr(0, comment));
        if (!logical.empty() && logical.front() == '[') {
            if (inside) sectionInsertPos = cursor;
            inside = (logical == header);
            if (inside) {
                foundSection = true;
                sectionInsertPos = text.size();
            }
        } else if (inside) {
            const auto eq = logical.find('=');
            if (eq != logical.npos) {
                const auto k = TrimTomlSlice(logical.substr(0, eq));
                if (k == key) {
                    const auto lineEq = line.find('=');
                    const auto valStart = cursor + bom + lineEq + 1;
                    const auto valEnd = cursor + bom + (comment == line.npos ? line.size() : comment);
                    std::size_t trimEnd = valEnd;
                    while (trimEnd > valStart &&
                           (text[trimEnd - 1] == ' ' || text[trimEnd - 1] == '\t' ||
                            text[trimEnd - 1] == '\r')) {
                        --trimEnd;
                    }
                    std::string out(text);
                    out.replace(valStart, trimEnd - valStart, " " + quoted);
                    return out;
                }
            }
        }
        cursor = (nl == text.npos) ? text.size() : nl + 1;
    }

    std::string out(text);
    std::string added;
    if (sectionInsertPos > 0 && text[sectionInsertPos - 1] != '\n') added += eol;
    if (!foundSection) {
        added += header;
        added += eol;
    }
    added += key;
    added += " = ";
    added += quoted;
    added += eol;
    out.insert(sectionInsertPos, added);
    return out;
}

} // namespace PyreLootFilter::ProfileDropdown
