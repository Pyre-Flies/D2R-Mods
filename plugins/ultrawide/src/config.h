#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace Ultrawide::Config {

inline constexpr uint32_t MinMonsterAiRange = 35U;
inline constexpr uint32_t MaxMonsterAiRange = 100U;
inline constexpr uint32_t DefaultMonsterAiRange = 65U;

inline constexpr uint32_t MinMonsterPopInRadius = 1U;
inline constexpr uint32_t MaxMonsterPopInRadius = 3U;
inline constexpr uint32_t DefaultMonsterPopInRadius = 3U;

struct Settings {
    bool enableBarsRemoval{true};
    bool enableMouseUnclamp{true};
    bool enablePlacardUnclamp{true};
    bool enableAutomapUnclamp{true};
    bool enableFullLengthHud{false};
    bool enableCornerMinimap{true};
    bool enableCornerMercenary{true};
    bool enableCornerGameInfo{true};
    bool enableTerrainExpansion{true};
    bool enableMonsterPopInFix{true};
    uint32_t monsterPopInRadius{DefaultMonsterPopInRadius};
    bool enableMonsterAiRange{true};
    uint32_t monsterAiRange{DefaultMonsterAiRange};
    bool scaleFriendlyAiRange{false};
    bool operator==(const Settings&) const = default;
};

inline std::string_view Trim(std::string_view text) noexcept {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == text.npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

inline bool ParseBoolValue(std::string_view setting, bool& out) noexcept {
    if (setting == "true") {
        out = true;
        return true;
    }
    if (setting == "false") {
        out = false;
        return true;
    }
    return false;
}

inline bool ParseHudModeValue(std::string_view setting, bool& outFullLength) noexcept {
    if (setting.size() >= 2 && setting.front() == '"' && setting.back() == '"') {
        setting = setting.substr(1, setting.size() - 2);
    }
    if (setting == "centered" || setting == "Centered" || setting == "CENTERED") {
        outFullLength = false;
        return true;
    }
    if (setting == "full" || setting == "full_length" || setting == "Full" || setting == "FullLength") {
        outFullLength = true;
        return true;
    }
    return false;
}

inline bool ParseUintClamped(std::string_view setting, uint32_t minVal, uint32_t maxVal, uint32_t& out) noexcept {
    if (setting.empty() || setting.size() > 6) return false;
    uint32_t value = 0;
    for (char ch : setting) {
        if (ch < '0' || ch > '9') return false;
        value = value * 10U + static_cast<uint32_t>(ch - '0');
    }
    out = std::clamp(value, minVal, maxVal);
    return true;
}

inline Settings Parse(std::string_view text, bool& invalid) noexcept {
    invalid = false;
    Settings result{};
    bool section = false;
    bool seenBars = false, seenMouse = false, seenPlacard = false, seenAutomap = false, seenTerrain = false;
    bool seenHudMode = false, seenCornerMinimap = false, seenCornerMercenary = false, seenCornerGameInfo = false;
    bool seenPopIn = false, seenPopInRadius = false, seenAiEnable = false, seenAiRange = false, seenFriendly = false;

    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = Trim(text.substr(0, end));
        text = end == text.npos ? std::string_view{} : text.substr(end + 1);
        line = Trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        if (line.front() == '[') {
            section = (line == "[ultrawide]");
            continue;
        }
        if (!section) continue;
        const auto eq = line.find('=');
        const auto key = Trim(line.substr(0, eq));
        const auto val = eq == line.npos ? std::string_view{} : Trim(line.substr(eq + 1));

        if (key == "enable_bars_removal") {
            bool parsed{};
            if (seenBars || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableBarsRemoval = true;
            } else {
                seenBars = true;
                result.enableBarsRemoval = parsed;
            }
        } else if (key == "enable_mouse_unclamp") {
            bool parsed{};
            if (seenMouse || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableMouseUnclamp = true;
            } else {
                seenMouse = true;
                result.enableMouseUnclamp = parsed;
            }
        } else if (key == "enable_placard_unclamp") {
            bool parsed{};
            if (seenPlacard || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enablePlacardUnclamp = true;
            } else {
                seenPlacard = true;
                result.enablePlacardUnclamp = parsed;
            }
        } else if (key == "enable_automap_unclamp") {
            bool parsed{};
            if (seenAutomap || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableAutomapUnclamp = true;
            } else {
                seenAutomap = true;
                result.enableAutomapUnclamp = parsed;
            }
        } else if (key == "enable_full_length_hud") {
            bool parsed{};
            if (seenHudMode || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableFullLengthHud = false;
            } else {
                seenHudMode = true;
                result.enableFullLengthHud = parsed;
            }
        } else if (key == "hud_mode") {
            bool parsed{};
            if (seenHudMode || eq == line.npos || !ParseHudModeValue(val, parsed)) {
                invalid = true;
                result.enableFullLengthHud = false;
            } else {
                seenHudMode = true;
                result.enableFullLengthHud = parsed;
            }
        } else if (key == "enable_corner_minimap") {
            bool parsed{};
            if (seenCornerMinimap || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableCornerMinimap = true;
            } else {
                seenCornerMinimap = true;
                result.enableCornerMinimap = parsed;
            }
        } else if (key == "enable_corner_mercenary") {
            bool parsed{};
            if (seenCornerMercenary || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableCornerMercenary = true;
            } else {
                seenCornerMercenary = true;
                result.enableCornerMercenary = parsed;
            }
        } else if (key == "enable_corner_game_info") {
            bool parsed{};
            if (seenCornerGameInfo || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableCornerGameInfo = true;
            } else {
                seenCornerGameInfo = true;
                result.enableCornerGameInfo = parsed;
            }
        } else if (key == "enable_terrain_expansion") {
            bool parsed{};
            if (seenTerrain || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableTerrainExpansion = true;
            } else {
                seenTerrain = true;
                result.enableTerrainExpansion = parsed;
            }
        } else if (key == "enable_monster_pop_in_fix") {
            bool parsed{};
            if (seenPopIn || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableMonsterPopInFix = true;
            } else {
                seenPopIn = true;
                result.enableMonsterPopInFix = parsed;
            }
        } else if (key == "monster_pop_in_radius") {
            uint32_t parsed{};
            if (seenPopInRadius || eq == line.npos || !ParseUintClamped(val, MinMonsterPopInRadius, MaxMonsterPopInRadius, parsed)) {
                invalid = true;
                result.monsterPopInRadius = DefaultMonsterPopInRadius;
            } else {
                seenPopInRadius = true;
                result.monsterPopInRadius = parsed;
            }
        } else if (key == "enable_monster_ai_range") {
            bool parsed{};
            if (seenAiEnable || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.enableMonsterAiRange = true;
            } else {
                seenAiEnable = true;
                result.enableMonsterAiRange = parsed;
            }
        } else if (key == "monster_ai_range") {
            uint32_t parsed{};
            if (seenAiRange || eq == line.npos || !ParseUintClamped(val, MinMonsterAiRange, MaxMonsterAiRange, parsed)) {
                invalid = true;
                result.monsterAiRange = DefaultMonsterAiRange;
            } else {
                seenAiRange = true;
                result.monsterAiRange = parsed;
            }
        } else if (key == "scale_friendly_ai_range") {
            bool parsed{};
            if (seenFriendly || eq == line.npos || !ParseBoolValue(val, parsed)) {
                invalid = true;
                result.scaleFriendlyAiRange = false;
            } else {
                seenFriendly = true;
                result.scaleFriendlyAiRange = parsed;
            }
        } else {
            invalid = true;
        }
    }
    return result;
}

inline int MenuSideWidth(int width, int height, bool inSession) noexcept {
    if (inSession) return 0;
    const int capped = static_cast<int>(static_cast<float>(height) * 2.1111112f);
    return width > capped ? (width - capped) / 2 : 0;
}

} // namespace Ultrawide::Config
