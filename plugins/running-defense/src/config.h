#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string_view>

namespace RunningDefense {

inline constexpr int DefaultDefenseLossPercent = 30;
inline constexpr int DefaultBlockLossPercent = 67;
inline constexpr std::size_t ConfigCapacity = 16384;

struct Settings {
    bool enabled = true;
    int defenseLossPercent = DefaultDefenseLossPercent;
    bool overrideBlockPenalty = false;
    int blockLossPercent = DefaultBlockLossPercent;
    bool showRunningIconOnSheet = true;
    bool highlightDefenseWhenRunning = true;
};

constexpr int ClampPercent(int value) noexcept {
    return std::clamp(value, 0, 100);
}

constexpr int ScaleDefenseForRun(int baseDefense, int defenseLossPercent) noexcept {
    if (baseDefense <= 0) return baseDefense;
    const int loss = ClampPercent(defenseLossPercent);
    const int keepPercent = 100 - loss;
    return static_cast<int>((static_cast<std::int64_t>(baseDefense) * keepPercent) / 100);
}

constexpr int ScaleBlockForRun(int baseBlock, int blockLossPercent) noexcept {
    if (baseBlock <= 0) return 0;
    const int loss = ClampPercent(blockLossPercent);
    const int keepPercent = 100 - loss;
    return static_cast<int>((static_cast<std::int64_t>(baseBlock) * keepPercent) / 100);
}

namespace Detail {

constexpr std::string_view Trim(std::string_view text) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r' || text.front() == '\n')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n')) {
        text.remove_suffix(1);
    }
    return text;
}

constexpr std::string_view StripComment(std::string_view line) noexcept {
    const auto hashPos = line.find('#');
    return hashPos == std::string_view::npos ? line : line.substr(0, hashPos);
}

constexpr bool ParseBool(std::string_view raw, bool& out) noexcept {
    if (raw == "true") {
        out = true;
        return true;
    }
    if (raw == "false") {
        out = false;
        return true;
    }
    return false;
}

constexpr bool ParsePercentInt(std::string_view raw, int& out) noexcept {
    if (raw.empty()) return false;
    std::int64_t acc = 0;
    for (char c : raw) {
        if (c < '0' || c > '9') return false;
        acc = acc * 10 + (c - '0');
        if (acc > 100) return false;
    }
    out = static_cast<int>(acc);
    return true;
}

} // namespace Detail

constexpr Settings ParseConfig(std::string_view input, unsigned& rejected) noexcept {
    Settings settings{};
    rejected = 0;
    bool inTargetSection = true;

    while (!input.empty()) {
        const auto newline = input.find('\n');
        const auto rawLine = newline == std::string_view::npos ? input : input.substr(0, newline);
        input = newline == std::string_view::npos ? std::string_view{} : input.substr(newline + 1);

        const auto line = Detail::Trim(Detail::StripComment(rawLine));
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            const auto section = Detail::Trim(line.substr(1, line.size() - 2));
            inTargetSection = (section == "running-defense");
            if (!inTargetSection) {
                ++rejected;
            }
            continue;
        }

        if (!inTargetSection) {
            ++rejected;
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            ++rejected;
            continue;
        }

        const auto key = Detail::Trim(line.substr(0, eq));
        const auto value = Detail::Trim(line.substr(eq + 1));
        if (key.empty() || value.empty()) {
            ++rejected;
            continue;
        }

        if (key == "enabled") {
            bool parsed{};
            if (Detail::ParseBool(value, parsed)) settings.enabled = parsed;
            else ++rejected;
        } else if (key == "defense_loss_percent") {
            int parsed{};
            if (Detail::ParsePercentInt(value, parsed)) settings.defenseLossPercent = parsed;
            else ++rejected;
        } else if (key == "override_block_penalty") {
            bool parsed{};
            if (Detail::ParseBool(value, parsed)) settings.overrideBlockPenalty = parsed;
            else ++rejected;
        } else if (key == "block_loss_percent") {
            int parsed{};
            if (Detail::ParsePercentInt(value, parsed)) settings.blockLossPercent = parsed;
            else ++rejected;
        } else if (key == "show_running_icon_on_sheet") {
            bool parsed{};
            if (Detail::ParseBool(value, parsed)) settings.showRunningIconOnSheet = parsed;
            else ++rejected;
        } else if (key == "highlight_defense_when_running") {
            bool parsed{};
            if (Detail::ParseBool(value, parsed)) settings.highlightDefenseWhenRunning = parsed;
            else ++rejected;
        } else {
            ++rejected;
        }
    }

    return settings;
}

} // namespace RunningDefense
