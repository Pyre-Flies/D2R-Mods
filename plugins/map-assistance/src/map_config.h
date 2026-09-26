#pragma once

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace MapAssistance {

struct Zone {
    std::int32_t mapId{};
    std::string name;
    std::string layout;
    std::vector<std::string> lines;
};

struct Configuration {
    bool enabled{true};
    std::vector<Zone> zones;
    std::size_t rejectedEntries{};
};

namespace Detail {

inline auto Trim(std::string_view value) noexcept -> std::string_view {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
    return value;
}

inline auto StripComment(std::string_view value) noexcept -> std::string_view {
    bool quoted = false;
    bool escaped = false;
    for (std::size_t index = 0; index < value.size(); ++index) {
        const char ch = value[index];
        if (escaped) { escaped = false; continue; }
        if (quoted && ch == '\\') { escaped = true; continue; }
        if (ch == '"') quoted = !quoted;
        else if (ch == '#' && !quoted) return value.substr(0, index);
    }
    return value;
}

inline auto ParseString(std::string_view value, std::string& output) -> bool {
    value = Trim(value);
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') return false;
    output.clear();
    for (std::size_t index = 1; index + 1 < value.size(); ++index) {
        char ch = value[index];
        if (ch != '\\') { output.push_back(ch); continue; }
        if (++index + 1 >= value.size()) return false;
        switch (value[index]) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: return false;
        }
    }
    return true;
}

inline auto ParseLines(std::string_view value, std::vector<std::string>& output) -> bool {
    value = Trim(value);
    if (value.size() < 2 || value.front() != '[' || value.back() != ']') return false;
    value.remove_prefix(1);
    value.remove_suffix(1);
    output.clear();
    while (true) {
        value = Trim(value);
        if (value.empty()) return !output.empty();
        bool escaped = false;
        std::size_t end = 1;
        if (value.front() != '"') return false;
        for (; end < value.size(); ++end) {
            if (escaped) { escaped = false; continue; }
            if (value[end] == '\\') { escaped = true; continue; }
            if (value[end] == '"') break;
        }
        if (end >= value.size()) return false;
        std::string line;
        if (!ParseString(value.substr(0, end + 1), line) || line.empty() || line.size() > 256) return false;
        output.push_back(std::move(line));
        value.remove_prefix(end + 1);
        value = Trim(value);
        if (value.empty()) return true;
        if (value.front() != ',') return false;
        value.remove_prefix(1);
    }
}

inline auto Complete(Zone& zone, bool haveId, Configuration& result) -> void {
    if (!haveId && zone.name.empty() && zone.layout.empty() && zone.lines.empty()) return;
    const bool duplicate = std::any_of(result.zones.begin(), result.zones.end(), [&](const Zone& found) { return found.mapId == zone.mapId; });
    std::size_t total = zone.name.size() + zone.layout.size();
    for (const auto& line : zone.lines) total += line.size() + 1;
    if (!haveId || zone.mapId <= 0 || zone.name.empty() || zone.name.size() > 96 || zone.layout.empty() ||
        zone.layout.size() > 64 || zone.lines.empty() || zone.lines.size() > 8 || total > 900 || duplicate || result.zones.size() >= 1024) {
        ++result.rejectedEntries;
        return;
    }
    result.zones.push_back(std::move(zone));
}

} // namespace Detail

inline auto ParseConfiguration(std::string_view text) -> Configuration {
    Configuration result;
    Zone current;
    bool inZone = false;
    bool inRoot = false;
    bool haveId = false;
    std::string pendingLines;
    bool collectingLines = false;

    for (std::size_t offset = 0; offset <= text.size();) {
        const auto end = text.find('\n', offset);
        auto line = Detail::Trim(Detail::StripComment(text.substr(offset, end == std::string_view::npos ? text.size() - offset : end - offset)));
        offset = end == std::string_view::npos ? text.size() + 1 : end + 1;
        if (collectingLines) {
            pendingLines.append(line);
            if (line.find(']') != std::string_view::npos) {
                if (!Detail::ParseLines(pendingLines, current.lines)) ++result.rejectedEntries;
                collectingLines = false;
                pendingLines.clear();
            }
            continue;
        }
        if (line.empty()) continue;
        if (line == "[[zones]]") {
            if (inZone) Detail::Complete(current, haveId, result);
            current = {};
            haveId = false;
            inZone = true;
            inRoot = false;
            continue;
        }
        if (line == "[map-assistance]") {
            if (inZone) Detail::Complete(current, haveId, result);
            current = {};
            haveId = false;
            inZone = false;
            inRoot = true;
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) continue;
        const auto key = Detail::Trim(line.substr(0, equals));
        auto value = Detail::Trim(line.substr(equals + 1));
        if (inRoot && key == "enabled") {
            if (value == "true") result.enabled = true;
            else if (value == "false") result.enabled = false;
            else ++result.rejectedEntries;
        } else if (inZone && key == "map_id") {
            std::int64_t parsed{};
            const auto conversion = std::from_chars(value.data(), value.data() + value.size(), parsed);
            haveId = conversion.ec == std::errc{} && conversion.ptr == value.data() + value.size() &&
                parsed > 0 && parsed <= std::numeric_limits<std::int32_t>::max();
            if (haveId) current.mapId = static_cast<std::int32_t>(parsed);
        } else if (inZone && key == "name") {
            if (!Detail::ParseString(value, current.name)) ++result.rejectedEntries;
        } else if (inZone && key == "layout") {
            if (!Detail::ParseString(value, current.layout)) ++result.rejectedEntries;
        } else if (inZone && key == "lines") {
            if (value.find(']') == std::string_view::npos) {
                collectingLines = true;
                pendingLines.assign(value);
            } else if (!Detail::ParseLines(value, current.lines)) ++result.rejectedEntries;
        }
    }
    if (inZone) Detail::Complete(current, haveId, result);
    if (collectingLines) ++result.rejectedEntries;
    std::sort(result.zones.begin(), result.zones.end(), [](const Zone& left, const Zone& right) { return left.mapId < right.mapId; });
    return result;
}

inline auto FindZone(const Configuration& config, std::int32_t mapId) noexcept -> const Zone* {
    const auto found = std::lower_bound(config.zones.begin(), config.zones.end(), mapId,
        [](const Zone& zone, std::int32_t value) { return zone.mapId < value; });
    return found != config.zones.end() && found->mapId == mapId ? &*found : nullptr;
}

inline auto JoinLines(const Zone& zone) -> std::string {
    std::string result;
    for (const auto& line : zone.lines) {
        if (!result.empty()) result.push_back('\n');
        result.append(line);
    }
    return result;
}

} // namespace MapAssistance
