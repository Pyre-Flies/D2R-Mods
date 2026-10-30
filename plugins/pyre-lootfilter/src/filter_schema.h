#pragma once
// Filter rule schema (v1, v2, and v3), rule evaluator, RGBA parser, and
// debounced file-stamp watcher for d2rloader/config/filter.json.

#include "filter_tables.h"
#include "json_value.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace PyreLootFilter::Schema {

inline constexpr std::size_t MaximumFilterFileBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t MaximumFilterRules = 4096U;
inline constexpr std::size_t MaximumFilterNameBytes = 79U;

enum class MinimapShape : std::uint8_t {
    Circle = 0,
    Diamond = 1,
    Triangle = 2,
    Star = 3,
    Square = 4,
    TriangleDown = 5,
    Kite = 6,
    Hexagon = 7,
    Spark = 8,
    Cross = 9,
    Chevron = 10,
    Bullseye = 11,
};

inline constexpr float DefaultMinimapSizePx = 12.0f;
inline constexpr float MinMinimapSizePx = 4.0f;
inline constexpr float MaxMinimapSizePx = 128.0f;

struct NumberTest {
    bool enabled{};
    bool hasEq{}, hasMin{}, hasMax{};
    std::uint32_t eq{}, min{}, max{};
    bool minInclusive{true}, maxInclusive{true};

    [[nodiscard]] constexpr bool Matches(std::uint32_t value) const noexcept {
        return !enabled ||
               ((!hasEq || value == eq) &&
                (!hasMin || (minInclusive ? value >= min : value > min)) &&
                (!hasMax || (maxInclusive ? value <= max : value < max)));
    }
};

struct SignedNumberTest {
    bool enabled{};
    bool hasEq{}, hasMin{}, hasMax{};
    std::int32_t eq{}, min{}, max{};
    bool minInclusive{true}, maxInclusive{true};

    [[nodiscard]] constexpr bool Matches(std::int32_t value) const noexcept {
        return !enabled ||
               ((!hasEq || value == eq) &&
                (!hasMin || (minInclusive ? value >= min : value > min)) &&
                (!hasMax || (maxInclusive ? value <= max : value < max)));
    }
};

struct RawStatEntry {
    std::uint16_t statId{};
    std::uint16_t layer{};
    std::int32_t value{};

    constexpr bool operator==(const RawStatEntry&) const noexcept = default;
};
static_assert(sizeof(RawStatEntry) == 8);

inline constexpr std::size_t MaximumItemStats = 64U;

struct ItemFacts {
    std::uint32_t code{};
    bool classIdKnown{};
    std::uint32_t classId{};
    bool quantityKnown{};
    std::uint32_t quantity{};
    bool qualityKnown{};
    std::uint32_t quality{};
    bool itemLevelKnown{};
    std::uint32_t itemLevel{};
    bool socketsKnown{};
    std::uint32_t sockets{};
    bool etherealKnown{};
    bool ethereal{};
    bool identifiedKnown{};
    bool identified{};
    bool sellPriceKnown{};
    std::uint32_t sellPrice{};
    std::uint64_t sellPriceObservedMs{};
    bool statsKnown{};
    std::uint8_t statCount{};
    std::array<RawStatEntry, MaximumItemStats> stats;

    constexpr void ResetScalars() noexcept {
        code = 0;
        classIdKnown = false;
        classId = 0;
        quantityKnown = false;
        quantity = 0;
        qualityKnown = false;
        quality = 0;
        itemLevelKnown = false;
        itemLevel = 0;
        socketsKnown = false;
        sockets = 0;
        etherealKnown = false;
        ethereal = false;
        identifiedKnown = false;
        identified = false;
        sellPriceKnown = false;
        sellPrice = 0;
        sellPriceObservedMs = 0;
        statsKnown = false;
        statCount = 0;
    }

    [[nodiscard]] constexpr std::int32_t GetStat(std::uint16_t statId,
                                                 std::uint16_t layer = 0) const noexcept {
        const std::size_t limit = std::min<std::size_t>(statCount, MaximumItemStats);
        for (std::size_t i = 0; i < limit; ++i) {
            if (stats[i].statId == statId && stats[i].layer == layer) {
                return stats[i].value;
            }
        }
        return 0;
    }

    [[nodiscard]] constexpr bool HasStatKey(std::uint16_t statId,
                                            std::uint16_t layer) const noexcept {
        const std::size_t limit = std::min<std::size_t>(statCount, MaximumItemStats);
        for (std::size_t i = 0; i < limit; ++i) {
            if (stats[i].statId == statId && stats[i].layer == layer) {
                return true;
            }
        }
        return false;
    }

    constexpr bool SetOrAddStat(std::uint16_t statId, std::uint16_t layer, std::int32_t value,
                                bool overwrite = true) noexcept {
        if (value == 0) return true;
        const std::size_t limit = std::min<std::size_t>(statCount, MaximumItemStats);
        for (std::size_t i = 0; i < limit; ++i) {
            if (stats[i].statId == statId && stats[i].layer == layer) {
                if (overwrite) stats[i].value = value;
                return true;
            }
        }
        if (limit >= MaximumItemStats) return false;
        stats[limit] = RawStatEntry{statId, layer, value};
        statCount = static_cast<std::uint8_t>(limit + 1U);
        return true;
    }
};

// Category A vs Category B (xray_mode):
// By default (xrayMode == false), StatListEx rules only match items whose stats are
// legitimately visible on the ground: white/grey runeword bases (quality 1..3:
// inferior/normal/superior) or already-identified Magic/Rare/Set/Unique/Crafted items.
// When xrayMode == true (hidden debug flag in pyre-lootfilter.toml), StatListEx rules
// also match unidentified Magic/Rare/Set/Unique/Crafted items on the ground.
[[nodiscard]] constexpr bool IsStatsEligible(const ItemFacts& item, bool xrayMode) noexcept {
    if (xrayMode || Tables::IsGoldCode(item.code)) return true;
    if (item.identifiedKnown && item.identified) return true;
    if (item.qualityKnown && item.quality >= 1U && item.quality <= 3U) return true;
    return false;
}

[[nodiscard]] constexpr bool IsStatsKnownIneligible(const ItemFacts& item,
                                                    bool xrayMode) noexcept {
    if (xrayMode || Tables::IsGoldCode(item.code)) return false;
    if (item.identifiedKnown && !item.identified &&
        item.qualityKnown && item.quality > 3U) {
        return true;
    }
    return false;
}

using StatKind = Tables::StatKind;

struct StatCondition {
    StatKind kind{StatKind::Standard};
    std::uint16_t statId{};
    bool hasLayerFilter{};
    std::vector<std::uint16_t> layers;
    SignedNumberTest value;

    [[nodiscard]] bool Matches(const ItemFacts& item) const noexcept {
        if (!item.statsKnown) return false;
        switch (kind) {
            case StatKind::AllResist: {
                const auto minRes = std::min({item.GetStat(39, 0), item.GetStat(41, 0),
                                              item.GetStat(43, 0), item.GetStat(45, 0)});
                return value.Matches(minRes);
            }
            case StatKind::TotalResist: {
                const auto sumRes = item.GetStat(39, 0) + item.GetStat(41, 0) +
                                    item.GetStat(43, 0) + item.GetStat(45, 0);
                return value.Matches(sumRes);
            }
            case StatKind::AnyResist: {
                const auto maxRes = std::max({item.GetStat(39, 0), item.GetStat(41, 0),
                                              item.GetStat(43, 0), item.GetStat(45, 0)});
                return value.Matches(maxRes);
            }
            case StatKind::EnhancedDamage: {
                const auto ed = std::max(item.GetStat(17, 0), item.GetStat(18, 0));
                return value.Matches(ed);
            }
            case StatKind::Standard:
            default: {
                if (hasLayerFilter) {
                    for (const auto layer : layers) {
                        if (value.Matches(item.GetStat(statId, layer))) {
                            return true;
                        }
                    }
                    return false;
                }
                const std::size_t limit = std::min<std::size_t>(item.statCount, MaximumItemStats);
                bool foundAny = false;
                for (std::size_t i = 0; i < limit; ++i) {
                    if (item.stats[i].statId == statId) {
                        foundAny = true;
                        if (value.Matches(item.stats[i].value)) {
                            return true;
                        }
                    }
                }
                return !foundAny && value.Matches(0);
            }
        }
    }
};

struct Conditions {
    std::vector<std::uint32_t> codes;
    std::vector<std::uint32_t> baseCodes;
    std::vector<std::uint32_t> typeCodes;
    std::vector<std::uint32_t> tierCodes;
    NumberTest quantity;
    std::vector<std::uint32_t> qualities;
    NumberTest itemLevel;
    NumberTest sockets;
    NumberTest sellPrice;
    bool etherealEnabled{}, etherealExpected{};
    bool identifiedEnabled{}, identifiedExpected{};
    bool superiorEnabled{}, superiorExpected{};
    std::vector<StatCondition> stats;

    [[nodiscard]] static bool ContainsSorted(const std::vector<std::uint32_t>& vec,
                                             std::uint32_t val) noexcept {
        return std::binary_search(vec.begin(), vec.end(), val);
    }

    [[nodiscard]] bool CheapCannotMatch(const ItemFacts& item) const noexcept {
        if (!codes.empty() && !ContainsSorted(codes, item.code)) return true;
        if (!baseCodes.empty() && !ContainsSorted(baseCodes, item.code)) return true;
        if (!typeCodes.empty() && !ContainsSorted(typeCodes, item.code)) return true;
        if (!tierCodes.empty() && !ContainsSorted(tierCodes, item.code)) return true;
        return false;
    }

    [[nodiscard]] bool Matches(const ItemFacts& item, bool xrayMode = false) const noexcept {
        if (CheapCannotMatch(item)) return false;
        if (quantity.enabled && (!item.quantityKnown || !quantity.Matches(item.quantity)))
            return false;
        if (!qualities.empty() && (!item.qualityKnown || !ContainsSorted(qualities, item.quality)))
            return false;
        if (itemLevel.enabled && (!item.itemLevelKnown || !itemLevel.Matches(item.itemLevel)))
            return false;
        if (sockets.enabled && (!item.socketsKnown || !sockets.Matches(item.sockets)))
            return false;
        if (sellPrice.enabled && (!item.sellPriceKnown || !sellPrice.Matches(item.sellPrice)))
            return false;
        if (etherealEnabled && (!item.etherealKnown || item.ethereal != etherealExpected))
            return false;
        if (identifiedEnabled && (!item.identifiedKnown || item.identified != identifiedExpected))
            return false;
        if (superiorEnabled && (!item.qualityKnown || (item.quality == 3U) != superiorExpected))
            return false;
        if (!stats.empty()) {
            if (!IsStatsEligible(item, xrayMode) || !item.statsKnown) return false;
            for (const auto& sc : stats) {
                if (!sc.Matches(item)) return false;
            }
        }
        return true;
    }
};

enum class MatchState : std::uint8_t {
    NoMatch = 0,
    Match = 1,
    Unknown = 2,
};

[[nodiscard]] inline MatchState EvaluateConditionsFailOpen(const Conditions& c,
                                                           const ItemFacts& item,
                                                           bool xrayMode = false) noexcept {
    if (c.CheapCannotMatch(item)) return MatchState::NoMatch;
    if (c.quantity.enabled && item.quantityKnown && !c.quantity.Matches(item.quantity))
        return MatchState::NoMatch;
    if (!c.qualities.empty() && item.qualityKnown &&
        !Conditions::ContainsSorted(c.qualities, item.quality))
        return MatchState::NoMatch;
    if (c.itemLevel.enabled && item.itemLevelKnown && !c.itemLevel.Matches(item.itemLevel))
        return MatchState::NoMatch;
    if (c.sockets.enabled && item.socketsKnown && !c.sockets.Matches(item.sockets))
        return MatchState::NoMatch;
    if (c.sellPrice.enabled && item.sellPriceKnown && !c.sellPrice.Matches(item.sellPrice))
        return MatchState::NoMatch;
    if (c.etherealEnabled && item.etherealKnown && item.ethereal != c.etherealExpected)
        return MatchState::NoMatch;
    if (c.identifiedEnabled && item.identifiedKnown && item.identified != c.identifiedExpected)
        return MatchState::NoMatch;
    if (c.superiorEnabled && item.qualityKnown && (item.quality == 3U) != c.superiorExpected)
        return MatchState::NoMatch;
    if (!c.stats.empty()) {
        if (IsStatsKnownIneligible(item, xrayMode)) return MatchState::NoMatch;
        if (IsStatsEligible(item, xrayMode) && item.statsKnown) {
            for (const auto& sc : c.stats) {
                if (!sc.Matches(item)) return MatchState::NoMatch;
            }
        }
    }

    if (c.quantity.enabled && !item.quantityKnown) return MatchState::Unknown;
    if ((!c.qualities.empty() || c.superiorEnabled) && !item.qualityKnown)
        return MatchState::Unknown;
    if (c.itemLevel.enabled && !item.itemLevelKnown) return MatchState::Unknown;
    if (c.sockets.enabled && !item.socketsKnown) return MatchState::Unknown;
    if (c.sellPrice.enabled && !item.sellPriceKnown) return MatchState::Unknown;
    if (c.etherealEnabled && !item.etherealKnown) return MatchState::Unknown;
    if (c.identifiedEnabled && !item.identifiedKnown) return MatchState::Unknown;
    if (!c.stats.empty() && (!IsStatsEligible(item, xrayMode) || !item.statsKnown))
        return MatchState::Unknown;
    return MatchState::Match;
}

struct FilterRule {
    bool usesConditions{};
    std::uint32_t code{};
    Conditions conditions{};
    bool show{true};
    bool continueEvaluation{};
    bool hasName{};
    std::array<char, 80> name{};
    std::size_t bytes{};
    bool hasBackground{};
    std::array<float, 4> background{};
    bool hasTextColor{};
    std::array<float, 4> textColor{};
    bool hasBorderColor{};
    std::array<float, 4> borderColor{};
    bool hasDropSound{};
    std::array<char, 64> dropSound{};
    bool hasMinimapIcon{};
    MinimapShape minimapShape{MinimapShape::Diamond};
    std::array<float, 4> minimapBorderColor{};
    std::array<float, 4> minimapFillColor{};
    float minimapSizePx{DefaultMinimapSizePx};
};

struct EvaluatedDecision {
    bool matched{};
    bool show{true};
    bool hasName{};
    std::array<char, 80> name{};
    std::size_t bytes{};
    bool hasBackground{};
    std::array<float, 4> background{};
    bool hasTextColor{};
    std::array<float, 4> textColor{};
    bool hasBorderColor{};
    std::array<float, 4> borderColor{};
    bool hasDropSound{};
    std::array<char, 64> dropSound{};
    bool hasMinimapIcon{};
    MinimapShape minimapShape{MinimapShape::Diamond};
    std::array<float, 4> minimapBorderColor{};
    std::array<float, 4> minimapFillColor{};
    float minimapSizePx{DefaultMinimapSizePx};
};

enum class NextProperty : std::uint8_t {
    None = 0,
    QualityLevel = 1,
    Sockets = 2,
    Ethereal = 3,
    Identified = 4,
    SellPrice = 5,
    Stats = 6,
};

struct FilterRuleTable {
    std::uint64_t generation{};
    std::uint32_t schema{1};
    std::size_t backgroundRules{};
    std::size_t textColorRules{};
    std::size_t borderColorRules{};
    std::size_t hiddenRules{};
    std::size_t soundRules{};
    std::size_t minimapIconRules{};
    std::size_t statRules{};
    bool usesQuantity{};
    bool usesQuality{};
    bool usesItemLevel{};
    bool usesSockets{};
    bool usesEthereal{};
    bool usesIdentified{};
    bool usesSellPrice{};
    bool usesItemType{};
    bool usesStats{};
    bool xrayMode{};
    std::vector<FilterRule> rules;
};

[[nodiscard]] inline MatchState EvaluateRuleFailOpen(const FilterRule& rule,
                                                     const ItemFacts& item,
                                                     bool xrayMode = false) noexcept {
    if (!rule.usesConditions) {
        return rule.code == item.code ? MatchState::Match : MatchState::NoMatch;
    }
    return EvaluateConditionsFailOpen(rule.conditions, item, xrayMode);
}

[[nodiscard]] inline NextProperty NextRequiredProperty(const std::vector<FilterRule>& rules,
                                                       const ItemFacts& item,
                                                       bool xrayMode = false) noexcept {
    for (const auto& rule : rules) {
        if (!rule.usesConditions) {
            if (rule.code == item.code && !rule.continueEvaluation) return NextProperty::None;
            continue;
        }
        const auto& c = rule.conditions;
        if (c.CheapCannotMatch(item)) continue;
        if (c.quantity.enabled) {
            if (!item.quantityKnown) return NextProperty::None;
            if (!c.quantity.Matches(item.quantity)) continue;
        }
        if (!c.qualities.empty() && item.qualityKnown &&
            !Conditions::ContainsSorted(c.qualities, item.quality))
            continue;
        if (c.itemLevel.enabled && item.itemLevelKnown && !c.itemLevel.Matches(item.itemLevel))
            continue;
        if (c.sockets.enabled && item.socketsKnown && !c.sockets.Matches(item.sockets))
            continue;
        if (c.sellPrice.enabled && item.sellPriceKnown && !c.sellPrice.Matches(item.sellPrice))
            continue;
        if (c.etherealEnabled && item.etherealKnown && item.ethereal != c.etherealExpected)
            continue;
        if (c.identifiedEnabled && item.identifiedKnown && item.identified != c.identifiedExpected)
            continue;
        if (c.superiorEnabled && item.qualityKnown && (item.quality == 3U) != c.superiorExpected)
            continue;
        if (!c.stats.empty()) {
            if (IsStatsKnownIneligible(item, xrayMode)) continue;
            if (IsStatsEligible(item, xrayMode) && item.statsKnown) {
                bool allStatsMatch = true;
                for (const auto& sc : c.stats) {
                    if (!sc.Matches(item)) {
                        allStatsMatch = false;
                        break;
                    }
                }
                if (!allStatsMatch) continue;
            }
        }

        if ((!c.qualities.empty() && !item.qualityKnown) ||
            (c.itemLevel.enabled && !item.itemLevelKnown) ||
            (c.superiorEnabled && !item.qualityKnown) ||
            (!c.stats.empty() && !xrayMode && !Tables::IsGoldCode(item.code) &&
             (!item.qualityKnown || !item.identifiedKnown)))
            return NextProperty::QualityLevel;
        if (c.sockets.enabled && !item.socketsKnown) return NextProperty::Sockets;
        if (c.etherealEnabled && !item.etherealKnown) return NextProperty::Ethereal;
        if (c.identifiedEnabled && !item.identifiedKnown) return NextProperty::Identified;
        if (!c.stats.empty() && !item.statsKnown) return NextProperty::Stats;
        if (c.sellPrice.enabled && !item.sellPriceKnown) return NextProperty::SellPrice;
        if (c.Matches(item, xrayMode) && !rule.continueEvaluation) return NextProperty::None;
    }
    return NextProperty::None;
}

[[nodiscard]] inline bool ResolveDecision(const FilterRuleTable* table,
                                          const ItemFacts& item,
                                          EvaluatedDecision& out) noexcept {
    out = {};
    if (!table) return false;
    bool matched = false;
    for (const auto& rule : table->rules) {
        const auto state = EvaluateRuleFailOpen(rule, item, table->xrayMode);
        if (state == MatchState::NoMatch) continue;
        if (state == MatchState::Unknown) break;
        matched = true;
        out.matched = true;
        out.show = rule.show;
        if (rule.hasName) {
            out.hasName = true;
            out.name = rule.name;
            out.bytes = rule.bytes;
        }
        if (rule.hasBackground) {
            out.hasBackground = true;
            out.background = rule.background;
        }
        if (rule.hasTextColor) {
            out.hasTextColor = true;
            out.textColor = rule.textColor;
        }
        if (rule.hasBorderColor) {
            out.hasBorderColor = true;
            out.borderColor = rule.borderColor;
        }
        if (rule.show) {
            if (rule.hasDropSound) {
                out.hasDropSound = true;
                out.dropSound = rule.dropSound;
            }
            if (rule.hasMinimapIcon) {
                out.hasMinimapIcon = true;
                out.minimapShape = rule.minimapShape;
                out.minimapBorderColor = rule.minimapBorderColor;
                out.minimapFillColor = rule.minimapFillColor;
                out.minimapSizePx = rule.minimapSizePx;
            }
        }
        if (!rule.continueEvaluation) break;
    }
    if (!out.show) {
        out.hasDropSound = false;
        out.dropSound = {};
        out.hasMinimapIcon = false;
    }
    return matched;
}

[[nodiscard]] inline bool CanEverHideCode(const FilterRuleTable* table,
                                          std::uint32_t canonicalCode) noexcept {
    if (!table || table->hiddenRules == 0) return false;
    ItemFacts probe{};
    probe.code = canonicalCode;
    for (const auto& rule : table->rules) {
        if (!rule.usesConditions) {
            if (rule.code == canonicalCode) {
                if (!rule.show) return true;
                if (!rule.continueEvaluation) return false;
            }
            continue;
        }
        if (rule.conditions.CheapCannotMatch(probe)) continue;
        if (!rule.show) return true;
    }
    return false;
}

[[nodiscard]] inline bool ParseFilterRgba(std::string_view value,
                                          std::array<float, 4>& result) noexcept {
    const auto trim = [](std::string_view input) noexcept {
        while (!input.empty() && (input.front() == ' ' || input.front() == '\t'))
            input.remove_prefix(1);
        while (!input.empty() && (input.back() == ' ' || input.back() == '\t'))
            input.remove_suffix(1);
        return input;
    };
    value = trim(value);
    if (value.empty()) return false;

    const auto toLower = [](char ch) noexcept -> char {
        return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
    };

    // 1. Hex format: #RRGGBB or #RRGGBBAA
    if (value.front() == '#' && (value.size() == 7U || value.size() == 9U)) {
        const auto hexDigit = [](char ch, std::uint8_t& out) noexcept -> bool {
            if (ch >= '0' && ch <= '9') {
                out = static_cast<std::uint8_t>(ch - '0');
                return true;
            }
            if (ch >= 'a' && ch <= 'f') {
                out = static_cast<std::uint8_t>(10 + (ch - 'a'));
                return true;
            }
            if (ch >= 'A' && ch <= 'F') {
                out = static_cast<std::uint8_t>(10 + (ch - 'A'));
                return true;
            }
            return false;
        };
        const auto hexByte = [&](std::size_t pos, std::uint8_t& out) noexcept -> bool {
            std::uint8_t hi{}, lo{};
            if (!hexDigit(value[pos], hi) || !hexDigit(value[pos + 1U], lo)) return false;
            out = static_cast<std::uint8_t>((hi << 4U) | lo);
            return true;
        };
        std::uint8_t r{}, g{}, b{}, a{255U};
        if (!hexByte(1U, r) || !hexByte(3U, g) || !hexByte(5U, b)) return false;
        if (value.size() == 9U && !hexByte(7U, a)) return false;
        result = {
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f,
        };
        return true;
    }

    // 2. Named colors (case-insensitive)
    if (value.size() <= 16U && value.find('(') == std::string_view::npos) {
        std::array<char, 16> lowerBuf{};
        std::size_t len = 0;
        for (char ch : value) {
            if (ch == ' ' || ch == '_' || ch == '-') continue;
            if (len >= lowerBuf.size()) {
                len = 0;
                break;
            }
            lowerBuf[len++] = toLower(ch);
        }
        const std::string_view key(lowerBuf.data(), len);
        struct NamedColor {
            std::string_view name;
            std::uint8_t r;
            std::uint8_t g;
            std::uint8_t b;
            float a;
        };
        constexpr std::array<NamedColor, 31> kNamedColors{{
            {"white", 255, 255, 255, 1.0f},
            {"black", 0, 0, 0, 0.85f},
            {"red", 255, 68, 68, 1.0f},
            {"green", 0, 228, 0, 1.0f},
            {"blue", 80, 140, 255, 1.0f},
            {"yellow", 255, 255, 80, 1.0f},
            {"orange", 255, 145, 0, 1.0f},
            {"purple", 185, 80, 255, 1.0f},
            {"violet", 185, 80, 255, 1.0f},
            {"teal", 0, 210, 190, 1.0f},
            {"cyan", 0, 235, 255, 1.0f},
            {"aqua", 0, 235, 255, 1.0f},
            {"gold", 215, 180, 100, 1.0f},
            {"gray", 150, 150, 150, 1.0f},
            {"grey", 150, 150, 150, 1.0f},
            {"darkgray", 90, 90, 90, 1.0f},
            {"darkgrey", 90, 90, 90, 1.0f},
            {"lightgray", 205, 205, 205, 1.0f},
            {"lightgrey", 205, 205, 205, 1.0f},
            {"silver", 205, 205, 205, 1.0f},
            {"pink", 255, 128, 192, 1.0f},
            {"magenta", 255, 64, 255, 1.0f},
            {"lime", 128, 255, 0, 1.0f},
            {"coral", 255, 127, 80, 1.0f},
            {"crimson", 220, 20, 60, 1.0f},
            {"amber", 255, 191, 0, 1.0f},
            {"brown", 160, 100, 50, 1.0f},
            {"navy", 30, 60, 150, 1.0f},
            {"maroon", 140, 25, 25, 1.0f},
            {"olive", 140, 150, 40, 1.0f},
            {"transparent", 0, 0, 0, 0.0f},
        }};
        for (const auto& nc : kNamedColors) {
            if (key == nc.name) {
                result = {
                    static_cast<float>(nc.r) / 255.0f,
                    static_cast<float>(nc.g) / 255.0f,
                    static_cast<float>(nc.b) / 255.0f,
                    nc.a,
                };
                return true;
            }
        }
    }

    // 3. RGBA(r, g, b, a) / rgba(r, g, b, a)
    if (value.size() < 10 || value.back() != ')' ||
        toLower(value[0]) != 'r' || toLower(value[1]) != 'g' ||
        toLower(value[2]) != 'b' || toLower(value[3]) != 'a' || value[4] != '(') {
        return false;
    }
    value.remove_prefix(5);
    value.remove_suffix(1);
    std::array<std::string_view, 4> parts{};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto comma = value.find(',');
        if (i < 3 && comma == std::string_view::npos) return false;
        if (i == 3 && comma != std::string_view::npos) return false;
        parts[i] = trim(i < 3 ? value.substr(0, comma) : value);
        if (parts[i].empty()) return false;
        if (i < 3) value.remove_prefix(comma + 1);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        unsigned int channel{};
        const auto [end, ec] =
            std::from_chars(parts[i].data(), parts[i].data() + parts[i].size(), channel, 10);
        if (ec != std::errc{} || end != parts[i].data() + parts[i].size() || channel > 255U)
            return false;
        result[i] = static_cast<float>(channel) / 255.0f;
    }
    float alpha{};
    const auto [end, ec] = std::from_chars(
        parts[3].data(), parts[3].data() + parts[3].size(), alpha, std::chars_format::general);
    if (ec != std::errc{} || end != parts[3].data() + parts[3].size() || !std::isfinite(alpha) ||
        alpha < 0.0f || alpha > 1.0f)
        return false;
    result[3] = alpha;
    return true;
}

using LogFn = std::function<void(const char*)>;

struct TableCacheState {
    std::filesystem::path configPath;
    std::filesystem::path excelDir;
    bool baseNamesLoaded{};
    Tables::BaseNameCatalog baseNames;
    bool itemTypesLoaded{};
    Tables::ItemTypeCatalog itemTypes;
    bool statCostsLoaded{};
    Tables::StatCostCatalog statCosts;
    bool skillsLoaded{};
    Tables::SkillCatalog skills;

    bool EnsureExcel(std::string& error) {
        if (!excelDir.empty()) return true;
        excelDir = Tables::DiscoverExcel(configPath);
        if (excelDir.empty()) {
            error = "excel-directory-not-found";
            return false;
        }
        return true;
    }

    bool EnsureBaseNames(std::string& error) {
        if (baseNamesLoaded) return true;
        if (!EnsureExcel(error)) return false;
        if (!Tables::LoadBaseNames(excelDir, baseNames, error)) return false;
        baseNamesLoaded = true;
        return true;
    }

    bool EnsureItemTypes(std::string& error) {
        if (itemTypesLoaded) return true;
        if (!EnsureExcel(error)) return false;
        if (!Tables::LoadItemTypes(excelDir, itemTypes, error)) return false;
        itemTypesLoaded = true;
        return true;
    }

    bool EnsureStatCosts(std::string& error) {
        if (statCostsLoaded) return true;
        if (excelDir.empty()) {
            excelDir = Tables::DiscoverExcel(configPath);
        }
        if (!excelDir.empty()) {
            if (!Tables::LoadStatCosts(excelDir, statCosts, error)) return false;
        }
        statCostsLoaded = true;
        return true;
    }

    bool EnsureSkills(std::string& error) {
        if (skillsLoaded) return true;
        if (excelDir.empty()) {
            excelDir = Tables::DiscoverExcel(configPath);
        }
        if (!excelDir.empty()) {
            if (!Tables::LoadSkills(excelDir, skills, error)) return false;
        }
        skillsLoaded = true;
        return true;
    }
};

inline bool ParseSignedNumberOperator(std::string_view opKey, const Json::Value& opVal,
                                      SignedNumberTest& test, std::string& error) {
    if (!opVal.is_number_integer()) {
        error = "stat-operator-requires-integer:" + std::string(opKey);
        return false;
    }
    std::int64_t val = 0;
    if (opVal.is_number_unsigned()) {
        const auto u = opVal.as_uint64();
        if (u > INT32_MAX) {
            error = "stat-value-out-of-int32-range";
            return false;
        }
        val = static_cast<std::int64_t>(u);
    } else {
        val = opVal.as_int64();
        if (val < INT32_MIN || val > INT32_MAX) {
            error = "stat-value-out-of-int32-range";
            return false;
        }
    }
    const auto i32 = static_cast<std::int32_t>(val);
    if (opKey == "eq") {
        if (test.hasEq) {
            error = "stat-duplicate-eq-bound";
            return false;
        }
        test.hasEq = true;
        test.eq = i32;
        test.enabled = true;
        return true;
    }
    if (opKey == "gt" || opKey == "gte" || opKey == "min") {
        if (test.hasMin) {
            error = "stat-duplicate-min-bound";
            return false;
        }
        test.hasMin = true;
        test.min = i32;
        test.minInclusive = (opKey != "gt");
        test.enabled = true;
        return true;
    }
    if (opKey == "lt" || opKey == "lte" || opKey == "max") {
        if (test.hasMax) {
            error = "stat-duplicate-max-bound";
            return false;
        }
        test.hasMax = true;
        test.max = i32;
        test.maxInclusive = (opKey != "lt");
        test.enabled = true;
        return true;
    }
    error = "unsupported-stat-operator:" + std::string(opKey);
    return false;
}

inline bool ResolveStatLayerToken(const StatCondition& cond, const Json::Value& tokenVal,
                                  TableCacheState& tables, std::vector<std::uint16_t>& outLayers,
                                  std::string& error) {
    if (tokenVal.is_number_integer()) {
        std::int64_t raw = tokenVal.is_number_unsigned()
                               ? static_cast<std::int64_t>(tokenVal.as_uint64())
                               : tokenVal.as_int64();
        if (raw < 0 || raw > UINT16_MAX) {
            error = "stat-param-out-of-range";
            return false;
        }
        outLayers.push_back(static_cast<std::uint16_t>(raw));
        return true;
    }
    if (!tokenVal.is_string()) {
        error = "stat-param-requires-integer-or-string";
        return false;
    }
    const auto& str = tokenVal.as_string();
    if (cond.kind != StatKind::Standard) {
        error = "synthetic-stat-does-not-accept-param";
        return false;
    }
    if (cond.statId == 83U) { // item_addclassskills
        std::uint16_t cls = 0;
        if (!Tables::ResolveClassIdentifier(str, cls)) {
            error = "unknown-class-param:" + str;
            return false;
        }
        outLayers.push_back(cls);
        return true;
    }
    if (cond.statId == 188U) { // item_addskill_tab
        if (!Tables::ResolveSkillTabIdentifier(str, outLayers)) {
            error = "unknown-skillTab-param:" + str;
            return false;
        }
        return true;
    }
    if (!tables.EnsureSkills(error)) return false;
    std::uint16_t skillId = 0;
    if (tables.skills.Resolve(str, skillId)) {
        outLayers.push_back(skillId);
        return true;
    }
    std::uint16_t genericParam = 0;
    if (Tables::ParseUnsigned16(str, UINT16_MAX, genericParam)) {
        outLayers.push_back(genericParam);
        return true;
    }
    error = "unknown-stat-param:" + str;
    return false;
}

inline bool ParseSingleStatCondition(std::string_view statKeyOrEmpty, const Json::Value& spec,
                                     TableCacheState& tables, StatCondition& out,
                                     std::string& error) {
    out = {};
    if (!tables.EnsureStatCosts(error)) return false;

    const auto resolveId = [&](const Json::Value& idVal) -> bool {
        if (idVal.is_number_integer()) {
            const std::int64_t n = idVal.is_number_unsigned()
                                       ? static_cast<std::int64_t>(idVal.as_uint64())
                                       : idVal.as_int64();
            if (n < 0 || n > 1023) {
                error = "stat-id-requires-integer-0..1023";
                return false;
            }
            out.kind = StatKind::Standard;
            out.statId = static_cast<std::uint16_t>(n);
            return true;
        }
        if (idVal.is_string()) {
            if (!tables.statCosts.Resolve(idVal.as_string(), out.kind, out.statId)) {
                error = "unknown-stat-name:" + idVal.as_string();
                return false;
            }
            return true;
        }
        error = "stat-requires-name-or-id";
        return false;
    };

    if (!statKeyOrEmpty.empty()) {
        if (!tables.statCosts.Resolve(statKeyOrEmpty, out.kind, out.statId)) {
            error = "unknown-stat-name:" + std::string(statKeyOrEmpty);
            return false;
        }
        if (spec.is_boolean()) {
            out.value.enabled = true;
            if (spec.as_bool()) {
                out.value.hasMin = true;
                out.value.min = 1;
                out.value.minInclusive = true;
            } else {
                out.value.hasEq = true;
                out.value.eq = 0;
            }
            return true;
        }
    }

    if (!spec.is_object() || spec.empty()) {
        error = "stat-condition-requires-object";
        return false;
    }

    bool statResolved = !statKeyOrEmpty.empty();
    for (const auto& [k, v] : spec.objectValues) {
        if (k == "stat" || k == "id") {
            if (statResolved) {
                error = "duplicate-stat-identifier";
                return false;
            }
            if (!resolveId(v)) return false;
            statResolved = true;
        }
    }
    if (!statResolved) {
        error = "stat-condition-missing-stat-or-id";
        return false;
    }

    for (const auto& [k, v] : spec.objectValues) {
        if (k == "stat" || k == "id") continue;
        if (k == "param" || k == "layer" || k == "skill" || k == "class" || k == "tab") {
            if (out.hasLayerFilter) {
                error = "duplicate-stat-param-filter";
                return false;
            }
            out.hasLayerFilter = true;
            if (v.is_array()) {
                if (v.empty() || v.size() > 32U) {
                    error = "stat-param-array-must-have-1..32-values";
                    return false;
                }
                for (const auto& elem : v.arrayValues) {
                    if (!ResolveStatLayerToken(out, elem, tables, out.layers, error)) return false;
                }
            } else {
                if (!ResolveStatLayerToken(out, v, tables, out.layers, error)) return false;
            }
            std::sort(out.layers.begin(), out.layers.end());
            out.layers.erase(std::unique(out.layers.begin(), out.layers.end()), out.layers.end());
        } else if (k == "value") {
            if (!v.is_object() || v.empty()) {
                error = "stat-value-requires-comparison-object";
                return false;
            }
            for (const auto& [opK, opV] : v.objectValues) {
                if (!ParseSignedNumberOperator(opK, opV, out.value, error)) return false;
            }
        } else {
            if (!ParseSignedNumberOperator(k, v, out.value, error)) return false;
        }
    }

    if (!out.value.enabled) {
        error = "stat-condition-missing-comparison-operator";
        return false;
    }
    return true;
}

inline bool ParseConditions(const Json::Value& value, Conditions& dest,
                            TableCacheState& tables, const LogFn& logWarn,
                            std::string& error) {
    if (!value.is_object() || value.empty()) {
        error = "conditions-must-be-nonempty-object";
        return false;
    }
    const auto parseNatural = [](const Json::Value& v, std::uint32_t& out) noexcept {
        if (!v.is_number_integer()) return false;
        if (v.is_number_unsigned()) {
            const auto u = v.as_uint64();
            if (u > UINT32_MAX) return false;
            out = static_cast<std::uint32_t>(u);
            return true;
        }
        const auto i = v.as_int64();
        if (i < 0 || i > UINT32_MAX) return false;
        out = static_cast<std::uint32_t>(i);
        return true;
    };

    for (const auto& [key, fieldVal] : value.objectValues) {
        if (key == "code" || key == "baseName") {
            if (!fieldVal.is_string() && !fieldVal.is_array()) {
                error = key + "-requires-name-or-array";
                return false;
            }
            std::vector<std::uint32_t> parsed;
            const auto addOne = [&](const Json::Value& part) -> bool {
                if (!part.is_string()) return false;
                const auto& text = part.as_string();
                if (key == "baseName") {
                    if (text.empty() || text.size() > 127 || text.find('\t') != text.npos ||
                        text.find('\r') != text.npos || text.find('\n') != text.npos)
                        return false;
                    if (!tables.EnsureBaseNames(error)) return false;
                    const auto* codes = tables.baseNames.Find(text);
                    if (!codes) {
                        error = "unknown-baseName-in-weapons-or-armor:" + text.substr(0, 75);
                        return false;
                    }
                    parsed.insert(parsed.end(), codes->begin(), codes->end());
                    return true;
                }
                if (text.empty() || text.size() > 4 ||
                    !std::all_of(text.begin(), text.end(), [](unsigned char ch) {
                        return ch >= 0x20U && ch <= 0x7EU;
                    }))
                    return false;
                const auto packed = Tables::PackCode(text);
                if (!Tables::PrintableItemCode(packed)) return false;
                parsed.push_back(packed);
                return true;
            };
            if (fieldVal.is_array()) {
                if (fieldVal.empty() || fieldVal.size() > 64) {
                    error = "condition-array-must-have-1..64-values";
                    return false;
                }
                for (const auto& elem : fieldVal.arrayValues) {
                    if (!addOne(elem)) {
                        if (error.empty()) error = "invalid-" + key + "-value";
                        return false;
                    }
                }
            } else if (!addOne(fieldVal)) {
                if (error.empty()) error = "invalid-" + key + "-value";
                return false;
            }
            std::sort(parsed.begin(), parsed.end());
            parsed.erase(std::unique(parsed.begin(), parsed.end()), parsed.end());
            if (key == "code") dest.codes = std::move(parsed);
            else dest.baseCodes = std::move(parsed);
        } else if (key == "itemType") {
            if (!fieldVal.is_string() && !fieldVal.is_array()) {
                error = "itemType-requires-name-code-or-array";
                return false;
            }
            std::vector<std::uint32_t> parsed;
            const auto addType = [&](const Json::Value& part) -> bool {
                if (!part.is_string()) return false;
                const auto& token = part.as_string();
                if (token.empty() || token.size() > 127 || token.find('\t') != token.npos ||
                    token.find('\r') != token.npos || token.find('\n') != token.npos)
                    return false;
                if (!tables.EnsureItemTypes(error)) return false;
                const auto* codes = tables.itemTypes.Find(token);
                if (!codes || codes->empty()) {
                    if (logWarn) {
                        char warn[240]{};
                        std::snprintf(
                            warn, sizeof(warn),
                            "LOOT_ITEMTYPE_IGNORED reason=%s token=%.*s",
                            !codes ? "unknown-itemType-in-itemtypes.txt" : "itemType-has-no-items",
                            75, token.c_str());
                        logWarn(warn);
                    }
                    return true;
                }
                parsed.insert(parsed.end(), codes->begin(), codes->end());
                return true;
            };
            if (fieldVal.is_array()) {
                if (fieldVal.empty() || fieldVal.size() > 64) {
                    error = "itemType-array-must-have-1..64-values";
                    return false;
                }
                for (const auto& elem : fieldVal.arrayValues) {
                    if (!addType(elem)) {
                        if (error.empty()) error = "invalid-itemType-value";
                        return false;
                    }
                }
            } else if (!addType(fieldVal)) {
                if (error.empty()) error = "invalid-itemType-value";
                return false;
            }
            std::sort(parsed.begin(), parsed.end());
            parsed.erase(std::unique(parsed.begin(), parsed.end()), parsed.end());
            if (parsed.empty()) {
                if (logWarn) {
                    logWarn("LOOT_ITEMTYPE_RULE_INACTIVE reason=itemType-selector-has-no-item-codes rule-disabled=1");
                }
                parsed.push_back(0xFFFFFFFFU);
            }
            dest.typeCodes = std::move(parsed);
        } else if (key == "tier") {
            const auto parseTier = [](const Json::Value& v, Tables::EquipmentTier& out) noexcept {
                if (!v.is_string()) return false;
                std::string lower = v.as_string();
                for (char& ch : lower) {
                    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + ('a' - 'A'));
                }
                if (lower == "normal") {
                    out = Tables::EquipmentTier::Normal;
                    return true;
                }
                if (lower == "exceptional") {
                    out = Tables::EquipmentTier::Exceptional;
                    return true;
                }
                if (lower == "elite") {
                    out = Tables::EquipmentTier::Elite;
                    return true;
                }
                return false;
            };
            if (!fieldVal.is_string() && !fieldVal.is_array()) {
                error = "tier-requires-name-or-names";
                return false;
            }
            if (!tables.EnsureBaseNames(error)) return false;
            std::vector<std::uint32_t> parsed;
            const auto appendTier = [&](const Json::Value& elem) -> bool {
                Tables::EquipmentTier tier{};
                if (!parseTier(elem, tier)) {
                    error = "unsupported-tier-name";
                    return false;
                }
                const auto& codes = tables.baseNames.CodesForTier(tier);
                parsed.insert(parsed.end(), codes.begin(), codes.end());
                return true;
            };
            if (fieldVal.is_array()) {
                if (fieldVal.empty() || fieldVal.size() > 3) {
                    error = "tier-array-must-have-1..3-names";
                    return false;
                }
                for (const auto& elem : fieldVal.arrayValues) {
                    if (!appendTier(elem)) return false;
                }
            } else if (!appendTier(fieldVal)) {
                return false;
            }
            std::sort(parsed.begin(), parsed.end());
            parsed.erase(std::unique(parsed.begin(), parsed.end()), parsed.end());
            dest.tierCodes = std::move(parsed);
        } else if (key == "rarity") {
            const auto qualityValue = [](const Json::Value& v, std::uint32_t& out) noexcept {
                if (!v.is_string()) return false;
                const auto& label = v.as_string();
                constexpr std::array<std::pair<std::string_view, std::uint32_t>, 9> choices{{
                    {"inferior", 1},
                    {"normal", 2},
                    {"superior", 3},
                    {"magic", 4},
                    {"set", 5},
                    {"rare", 6},
                    {"unique", 7},
                    {"crafted", 8},
                    {"tempered", 9},
                }};
                for (const auto& [name, q] : choices) {
                    if (label == name) {
                        out = q;
                        return true;
                    }
                }
                return false;
            };
            if (!fieldVal.is_string() && !fieldVal.is_array()) {
                error = "rarity-requires-name-or-names";
                return false;
            }
            std::vector<std::uint32_t> parsed;
            if (fieldVal.is_array()) {
                if (fieldVal.empty() || fieldVal.size() > 9) {
                    error = "rarity-array-must-have-1..9-names";
                    return false;
                }
                for (const auto& r : fieldVal.arrayValues) {
                    std::uint32_t q{};
                    if (!qualityValue(r, q)) {
                        error = "unsupported-rarity-name";
                        return false;
                    }
                    parsed.push_back(q);
                }
            } else {
                std::uint32_t q{};
                if (!qualityValue(fieldVal, q)) {
                    error = "unsupported-rarity-name";
                    return false;
                }
                parsed.push_back(q);
            }
            std::sort(parsed.begin(), parsed.end());
            parsed.erase(std::unique(parsed.begin(), parsed.end()), parsed.end());
            dest.qualities = std::move(parsed);
        } else if (key == "identified") {
            if (!fieldVal.is_boolean()) {
                error = "identified-requires-boolean-true-or-false";
                return false;
            }
            dest.identifiedEnabled = true;
            dest.identifiedExpected = fieldVal.as_bool();
        } else if (key == "ethereal") {
            if (!fieldVal.is_boolean()) {
                error = "ethereal-requires-boolean-true-or-false";
                return false;
            }
            dest.etherealEnabled = true;
            dest.etherealExpected = fieldVal.as_bool();
        } else if (key == "itemLevel" || key == "quantity" || key == "sockets" ||
                   key == "sellPrice") {
            const bool isLevel = key == "itemLevel";
            const bool isSocket = key == "sockets";
            const bool isSellPrice = key == "sellPrice";
            if (!fieldVal.is_object() || fieldVal.empty()) {
                error = key + "-requires-comparison-object";
                return false;
            }
            auto& numberTest = isLevel ? dest.itemLevel
                                       : (isSocket ? dest.sockets
                                                   : (isSellPrice ? dest.sellPrice : dest.quantity));
            numberTest.enabled = true;
            for (const auto& [opKey, opVal] : fieldVal.objectValues) {
                std::uint32_t number{};
                if (!parseNatural(opVal, number) ||
                    (isLevel ? (number < 1U || number > 99U)
                             : (isSocket ? number > 15U : false))) {
                    error = isLevel
                                ? "itemLevel-requires-integer-1..99"
                                : (isSocket ? "sockets-requires-integer-0..15"
                                            : (isSellPrice ? "sellPrice-requires-integer-0..4294967295"
                                                           : "quantity-requires-integer-0..4294967295"));
                    return false;
                }
                if (opKey == "eq") {
                    if (numberTest.hasEq) return false;
                    numberTest.hasEq = true;
                    numberTest.eq = number;
                } else if (opKey == "gt" || opKey == "gte") {
                    if (numberTest.hasMin) {
                        error = key + "-duplicate-min-bound";
                        return false;
                    }
                    numberTest.hasMin = true;
                    numberTest.min = number;
                    numberTest.minInclusive = (opKey == "gte");
                } else if (opKey == "lt" || opKey == "lte") {
                    if (numberTest.hasMax) {
                        error = key + "-duplicate-max-bound";
                        return false;
                    }
                    numberTest.hasMax = true;
                    numberTest.max = number;
                    numberTest.maxInclusive = (opKey == "lte");
                } else {
                    error = "unsupported-" + key + "-operator:" + opKey;
                    return false;
                }
            }
        } else if (key == "stats") {
            if (!fieldVal.is_object() && !fieldVal.is_array()) {
                error = "stats-requires-object-or-array";
                return false;
            }
            if (fieldVal.empty() || fieldVal.size() > 32U) {
                error = "stats-must-have-1..32-conditions";
                return false;
            }
            if (fieldVal.is_object()) {
                for (const auto& [statName, statSpec] : fieldVal.objectValues) {
                    StatCondition sc{};
                    if (!ParseSingleStatCondition(statName, statSpec, tables, sc, error)) {
                        return false;
                    }
                    dest.stats.push_back(std::move(sc));
                }
            } else {
                for (const auto& statElem : fieldVal.arrayValues) {
                    StatCondition sc{};
                    if (!ParseSingleStatCondition({}, statElem, tables, sc, error)) {
                        return false;
                    }
                    dest.stats.push_back(std::move(sc));
                }
            }
        } else {
            error = "unsupported-ground-condition:" + key;
            return false;
        }
    }
    return true;
}

inline std::shared_ptr<FilterRuleTable> LoadFilterRuleTable(
    const std::filesystem::path& configPath, std::uint64_t nextGeneration,
    const LogFn& logWarn) {
    if (configPath.empty()) {
        if (logWarn) logWarn("LOOT_RULES_REFUSED path-unavailable");
        return nullptr;
    }
    std::error_code ec;
    const auto length = std::filesystem::file_size(configPath, ec);
    if (ec || length == 0 || length > MaximumFilterFileBytes) {
        if (logWarn)
            logWarn("LOOT_RULES_REFUSED config-missing-empty-or-over-4MiB last-valid-rules-preserved=1");
        return nullptr;
    }
    std::ifstream file(configPath, std::ios::binary);
    if (!file) {
        if (logWarn) logWarn("LOOT_RULES_REFUSED config-open-failed last-valid-rules-preserved=1");
        return nullptr;
    }
    std::string raw(static_cast<std::size_t>(length), '\0');
    if (!file.read(raw.data(), static_cast<std::streamsize>(length))) {
        if (logWarn) logWarn("LOOT_RULES_REFUSED config-read-failed last-valid-rules-preserved=1");
        return nullptr;
    }
    Json::Value config;
    std::size_t parseLine{};
    std::string parseError;
    if (!Json::Parse(raw, config, &parseLine, &parseError)) {
        if (logWarn) {
            char msg[280]{};
            std::snprintf(msg, sizeof(msg),
                          "LOOT_RULES_REFUSED json-parse-error line=%zu reason=%s last-valid-rules-preserved=1",
                          parseLine, parseError.c_str());
            logWarn(msg);
        }
        return nullptr;
    }
    if (!config.is_object() || !config.contains("version") ||
        !config["version"].is_number_integer() ||
        (config["version"].as_int64() != 1 && config["version"].as_int64() != 2 &&
         config["version"].as_int64() != 3) ||
        !config.contains("rules") || !config["rules"].is_array()) {
        if (logWarn)
            logWarn("LOOT_RULES_REFUSED required-schema={version:1|2|3,rules:array} last-valid-rules-preserved=1");
        return nullptr;
    }
    const auto& entries = config["rules"].arrayValues;
    if (entries.size() > MaximumFilterRules) {
        if (logWarn) logWarn("LOOT_RULES_REFUSED over-4096-rules last-valid-rules-preserved=1");
        return nullptr;
    }

    auto fresh = std::make_shared<FilterRuleTable>();
    fresh->generation = nextGeneration;
    fresh->schema = static_cast<std::uint32_t>(config["version"].as_int64());
    fresh->rules.reserve(entries.size());

    std::unordered_set<std::uint32_t> seenV1Codes;
    TableCacheState tables{configPath};
    std::size_t entryIdx = 0;

    for (const auto& entry : entries) {
        ++entryIdx;
        if (!entry.is_object()) {
            if (logWarn) {
                char msg[220]{};
                std::snprintf(msg, sizeof(msg),
                              "LOOT_RULES_REFUSED entry=%zu rule-must-be-object last-valid-rules-preserved=1",
                              entryIdx);
                logWarn(msg);
            }
            return nullptr;
        }
        const Json::Value* block = &entry;
        bool wrapperShow = true;
        if (fresh->schema == 3) {
            const bool hasShow = entry.contains("show");
            const bool hasHide = entry.contains("hide");
            if (entry.size() != 1U || hasShow == hasHide ||
                (hasShow && !entry["show"].is_object()) ||
                (hasHide && !entry["hide"].is_object())) {
                if (logWarn) {
                    char msg[280]{};
                    std::snprintf(
                        msg, sizeof(msg),
                        "LOOT_RULES_REFUSED entry=%zu schema3-requires-exactly-one-wrapper={show:object}|{hide:object} last-valid-rules-preserved=1",
                        entryIdx);
                    logWarn(msg);
                }
                return nullptr;
            }
            wrapperShow = hasShow;
            block = &entry[hasShow ? "show" : "hide"];
        }

        if ((fresh->schema == 1 && (!block->contains("code") || !(*block)["code"].is_string())) ||
            (fresh->schema == 2 &&
             (!block->contains("conditions") || !(*block)["conditions"].is_object() ||
              block->contains("code"))) ||
            (fresh->schema == 3 &&
             ((block->contains("conditions") && !(*block)["conditions"].is_object()) ||
              block->contains("code") || block->contains("show") || block->contains("hide"))) ||
            (block->contains("ruleName") && !(*block)["ruleName"].is_string()) ||
            (block->contains("name") && !(*block)["name"].is_string()) ||
            (block->contains("tooltip") && !(*block)["tooltip"].is_object()) ||
            (block->contains("backgroundColor") && !(*block)["backgroundColor"].is_string()) ||
            (block->contains("textColor") && !(*block)["textColor"].is_string()) ||
            (block->contains("borderColor") && !(*block)["borderColor"].is_string()) ||
            (block->contains("dropSound") && !(*block)["dropSound"].is_string()) ||
            (block->contains("minimapIcon") && !(*block)["minimapIcon"].is_object()) ||
            (fresh->schema <= 2 && block->contains("show") && !(*block)["show"].is_boolean()) ||
            (fresh->schema == 3 && block->contains("continue") &&
             !(*block)["continue"].is_boolean()) ||
            (fresh->schema <= 2 && !block->contains("show") && !block->contains("name") &&
             !block->contains("tooltip") && !block->contains("backgroundColor") &&
             !block->contains("textColor") && !block->contains("borderColor") &&
             !block->contains("dropSound") && !block->contains("minimapIcon"))) {
            if (logWarn) {
                char msg[320]{};
                std::snprintf(
                    msg, sizeof(msg),
                    "LOOT_RULES_REFUSED entry=%zu invalid-rule-block last-valid-rules-preserved=1",
                    entryIdx);
                logWarn(msg);
            }
            return nullptr;
        }

        if (fresh->schema == 3 &&
            (block->contains("backgroundColor") || block->contains("textColor") ||
             block->contains("borderColor"))) {
            if (logWarn)
                logWarn("LOOT_RULES_REFUSED schema3-flat-colors-unsupported use-tooltip-object last-valid-rules-preserved=1");
            return nullptr;
        }
        if (block->contains("tooltip")) {
            const auto& tooltip = (*block)["tooltip"];
            if (block->contains("backgroundColor") || block->contains("textColor") ||
                block->contains("borderColor")) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED tooltip-cannot-be-mixed-with-flat-colors last-valid-rules-preserved=1");
                return nullptr;
            }
            if (tooltip.empty() || tooltip.size() > 3U ||
                (tooltip.contains("backgroundColor") && !tooltip["backgroundColor"].is_string()) ||
                (tooltip.contains("textColor") && !tooltip["textColor"].is_string()) ||
                (tooltip.contains("borderColor") && !tooltip["borderColor"].is_string()) ||
                (!tooltip.contains("backgroundColor") && !tooltip.contains("textColor") &&
                 !tooltip.contains("borderColor"))) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-tooltip last-valid-rules-preserved=1");
                return nullptr;
            }
            for (const auto& [k, v] : tooltip.objectValues) {
                if (k != "backgroundColor" && k != "textColor" && k != "borderColor") {
                    if (logWarn)
                        logWarn("LOOT_RULES_REFUSED invalid-tooltip unexpected-field last-valid-rules-preserved=1");
                    return nullptr;
                }
            }
        }

        if (fresh->schema == 2) {
            for (const auto& [k, v] : block->objectValues) {
                if (k != "conditions" && k != "show" && k != "name" && k != "tooltip" &&
                    k != "textColor" && k != "backgroundColor" && k != "borderColor" &&
                    k != "dropSound" && k != "minimapIcon") {
                    if (logWarn)
                        logWarn("LOOT_RULES_REFUSED unexpected-v2-action-or-field last-valid-rules-preserved=1");
                    return nullptr;
                }
            }
        } else if (fresh->schema == 3) {
            for (const auto& [k, v] : block->objectValues) {
                if (k != "ruleName" && k != "conditions" && k != "continue" && k != "name" &&
                    k != "tooltip" && k != "dropSound" && k != "minimapIcon") {
                    if (logWarn)
                        logWarn("LOOT_RULES_REFUSED unexpected-v3-block-field last-valid-rules-preserved=1");
                    return nullptr;
                }
            }
        }

        const auto codeStr =
            fresh->schema == 1 ? (*block)["code"].as_string() : std::string{};
        const bool hasName = block->contains("name");
        const auto nameStr = hasName ? (*block)["name"].as_string() : std::string{};

        if (fresh->schema == 1 &&
            (codeStr.empty() || codeStr.size() > 4 ||
             !std::all_of(codeStr.begin(), codeStr.end(), [](unsigned char ch) {
                 return ch >= 0x20U && ch <= 0x7EU;
             }) ||
             !Tables::PrintableItemCode(Tables::PackCode(codeStr)))) {
            if (logWarn)
                logWarn("LOOT_RULES_REFUSED invalid-v1-code last-valid-rules-preserved=1");
            return nullptr;
        }

        std::size_t lineCount = 1;
        std::size_t currentLineLen = 0;
        bool safeName =
            !hasName || (!nameStr.empty() && nameStr.size() <= MaximumFilterNameBytes);
        for (const unsigned char ch : nameStr) {
            if (ch == '\n') {
                ++lineCount;
                if (currentLineLen == 0) safeName = false;
                currentLineLen = 0;
            } else if (ch >= 0x20U && ch <= 0x7EU) {
                if (++currentLineLen > 55U) safeName = false;
            } else {
                safeName = false;
            }
        }
        if (hasName && (currentLineLen == 0 || lineCount > 3U)) safeName = false;
        if (!safeName) {
            if (logWarn)
                logWarn("LOOT_RULES_REFUSED invalid-rule-name last-valid-rules-preserved=1");
            return nullptr;
        }

        const auto packedCode = Tables::PackCode(codeStr);
        if (fresh->schema == 1 && !seenV1Codes.insert(packedCode).second) {
            if (logWarn)
                logWarn("LOOT_RULES_REFUSED duplicate-v1-code last-valid-rules-preserved=1");
            return nullptr;
        }

        FilterRule rule{};
        rule.usesConditions = fresh->schema >= 2;
        rule.code = packedCode;
        rule.show = fresh->schema == 3
                        ? wrapperShow
                        : (!block->contains("show") || (*block)["show"].as_bool());
        rule.continueEvaluation =
            fresh->schema == 3 && block->contains("continue") && (*block)["continue"].as_bool();
        if (!rule.show) ++fresh->hiddenRules;

        if (rule.usesConditions) {
            std::string condErr;
            if (block->contains("conditions")) {
                if (!ParseConditions((*block)["conditions"], rule.conditions, tables, logWarn,
                                     condErr)) {
                    if (logWarn) {
                        char msg[300]{};
                        std::snprintf(msg, sizeof(msg),
                                      "LOOT_RULES_REFUSED entry=%zu %s last-valid-rules-preserved=1",
                                      entryIdx, condErr.c_str());
                        logWarn(msg);
                    }
                    return nullptr;
                }
            }
            if (rule.conditions.quantity.enabled) fresh->usesQuantity = true;
            if (!rule.conditions.qualities.empty()) fresh->usesQuality = true;
            if (rule.conditions.itemLevel.enabled) fresh->usesItemLevel = true;
            if (rule.conditions.sockets.enabled) fresh->usesSockets = true;
            if (rule.conditions.etherealEnabled) fresh->usesEthereal = true;
            if (rule.conditions.identifiedEnabled) fresh->usesIdentified = true;
            if (rule.conditions.sellPrice.enabled) fresh->usesSellPrice = true;
            if (!rule.conditions.typeCodes.empty()) fresh->usesItemType = true;
            if (!rule.conditions.stats.empty()) {
                fresh->usesStats = true;
                ++fresh->statRules;
            }
        }

        if (hasName) {
            std::memcpy(rule.name.data(), nameStr.data(), nameStr.size());
            rule.name[nameStr.size()] = '\0';
            rule.bytes = nameStr.size() + 1;
            rule.hasName = true;
        }

        const auto* tooltip = block->find_ptr("tooltip");
        const auto* bgVal = tooltip && tooltip->contains("backgroundColor")
                                ? tooltip->find_ptr("backgroundColor")
                                : (fresh->schema <= 2 ? block->find_ptr("backgroundColor") : nullptr);
        const auto* fgVal = tooltip && tooltip->contains("textColor")
                                ? tooltip->find_ptr("textColor")
                                : (fresh->schema <= 2 ? block->find_ptr("textColor") : nullptr);
        const auto* bdVal = tooltip && tooltip->contains("borderColor")
                                ? tooltip->find_ptr("borderColor")
                                : (fresh->schema <= 2 ? block->find_ptr("borderColor") : nullptr);

        if (bgVal) {
            if (!ParseFilterRgba(bgVal->as_string(), rule.background)) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-backgroundColor last-valid-rules-preserved=1");
                return nullptr;
            }
            rule.hasBackground = true;
            ++fresh->backgroundRules;
        }
        if (fgVal) {
            if (!ParseFilterRgba(fgVal->as_string(), rule.textColor)) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-textColor last-valid-rules-preserved=1");
                return nullptr;
            }
            rule.hasTextColor = true;
            ++fresh->textColorRules;
        }
        if (bdVal) {
            if (!ParseFilterRgba(bdVal->as_string(), rule.borderColor)) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-borderColor last-valid-rules-preserved=1");
                return nullptr;
            }
            rule.hasBorderColor = true;
            ++fresh->borderColorRules;
        }

        if (block->contains("dropSound")) {
            const auto& sound = (*block)["dropSound"].as_string();
            const auto validSoundChar = [](unsigned char c) noexcept {
                return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                       (c >= '0' && c <= '9') || c == '_' || c == '-';
            };
            if (sound.empty() || sound.size() >= rule.dropSound.size() ||
                !std::all_of(sound.begin(), sound.end(), validSoundChar)) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-dropSound last-valid-rules-preserved=1");
                return nullptr;
            }
            if (rule.show) {
                bool soundAssetAvailable = true;
                if (sound.size() == 8 && sound.starts_with("Filter")) {
                    const auto soundExcel = !tables.excelDir.empty()
                                                ? tables.excelDir
                                                : Tables::DiscoverExcel(configPath);
                    if (!soundExcel.empty()) {
                        std::error_code sfxEc;
                        const auto flacPath = soundExcel.parent_path().parent_path() / L"hd" /
                                              L"global" / L"sfx" / L"item" / (sound + ".flac");
                        if (!std::filesystem::is_regular_file(flacPath, sfxEc) || sfxEc) {
                            soundAssetAvailable = false;
                            if (logWarn) {
                                char msg[280]{};
                                std::snprintf(
                                    msg, sizeof(msg),
                                    "LOOT_SOUND_ASSET_MISSING entry=%zu name='%s' reason=missing-flac-in-mod-data fallback=native-drop-sound",
                                    entryIdx, sound.c_str());
                                logWarn(msg);
                            }
                        }
                    }
                }
                if (soundAssetAvailable) {
                    std::memcpy(rule.dropSound.data(), sound.c_str(), sound.size() + 1);
                    rule.hasDropSound = true;
                    ++fresh->soundRules;
                }
            }
        }

        if (block->contains("minimapIcon")) {
            const auto& icon = (*block)["minimapIcon"];
            const bool hasSize = icon.contains("size");
            if ((icon.size() != 3U && icon.size() != 4U) || !icon.contains("shape") ||
                !icon.contains("borderColor") || !icon.contains("fillColor") ||
                !icon["shape"].is_string() || !icon["borderColor"].is_string() ||
                !icon["fillColor"].is_string() ||
                (hasSize && !icon["size"].is_number())) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-minimapIcon last-valid-rules-preserved=1");
                return nullptr;
            }
            for (const auto& [k, v] : icon.objectValues) {
                if (k != "shape" && k != "borderColor" && k != "fillColor" && k != "size") {
                    if (logWarn)
                        logWarn("LOOT_RULES_REFUSED invalid-minimapIcon-field last-valid-rules-preserved=1");
                    return nullptr;
                }
            }
            const auto& shape = icon["shape"].as_string();
            if (shape == "circle") rule.minimapShape = MinimapShape::Circle;
            else if (shape == "square") rule.minimapShape = MinimapShape::Square;
            else if (shape == "diamond") rule.minimapShape = MinimapShape::Diamond;
            else if (shape == "triangle") rule.minimapShape = MinimapShape::Triangle;
            else if (shape == "triangleDown" || shape == "invertedTriangle")
                rule.minimapShape = MinimapShape::TriangleDown;
            else if (shape == "star") rule.minimapShape = MinimapShape::Star;
            else if (shape == "spark" || shape == "star4")
                rule.minimapShape = MinimapShape::Spark;
            else if (shape == "kite" || shape == "gem")
                rule.minimapShape = MinimapShape::Kite;
            else if (shape == "hexagon" || shape == "hex")
                rule.minimapShape = MinimapShape::Hexagon;
            else if (shape == "cross" || shape == "plus")
                rule.minimapShape = MinimapShape::Cross;
            else if (shape == "chevron")
                rule.minimapShape = MinimapShape::Chevron;
            else if (shape == "bullseye" || shape == "ring")
                rule.minimapShape = MinimapShape::Bullseye;
            else {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-minimapIcon-shape last-valid-rules-preserved=1");
                return nullptr;
            }
            if (!ParseFilterRgba(icon["borderColor"].as_string(), rule.minimapBorderColor) ||
                !ParseFilterRgba(icon["fillColor"].as_string(), rule.minimapFillColor)) {
                if (logWarn)
                    logWarn("LOOT_RULES_REFUSED invalid-minimapIcon-color last-valid-rules-preserved=1");
                return nullptr;
            }
            if (hasSize) {
                const double rawSize = icon["size"].as_double();
                if (!std::isfinite(rawSize)) {
                    if (logWarn)
                        logWarn("LOOT_RULES_REFUSED invalid-minimapIcon-size last-valid-rules-preserved=1");
                    return nullptr;
                }
                const float clampedSize = std::clamp(
                    static_cast<float>(rawSize), MinMinimapSizePx, MaxMinimapSizePx);
                if (rule.show && static_cast<float>(rawSize) != clampedSize && logWarn) {
                    char msg[240]{};
                    std::snprintf(
                        msg, sizeof(msg),
                        "LOOT_MINIMAP_SIZE_CLAMPED entry=%zu requested=%.1f clamped=%.1f range=%.0f..%.0f",
                        entryIdx, rawSize, static_cast<double>(clampedSize),
                        static_cast<double>(MinMinimapSizePx),
                        static_cast<double>(MaxMinimapSizePx));
                    logWarn(msg);
                }
                rule.minimapSizePx = clampedSize;
            }
            if (rule.show) {
                rule.hasMinimapIcon = true;
                ++fresh->minimapIconRules;
            }
        }

        fresh->rules.push_back(std::move(rule));
    }

    return fresh;
}

struct FileStamp {
    bool exists{};
    std::uintmax_t size{};
    std::filesystem::file_time_type modified{};
    constexpr bool operator==(const FileStamp&) const noexcept = default;
};

[[nodiscard]] inline FileStamp ReadFilterFileStamp(const std::filesystem::path& path) noexcept {
    if (path.empty()) return {};
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec || !std::filesystem::is_regular_file(status)) return {};
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) return {};
    const auto modified = std::filesystem::last_write_time(path, ec);
    if (ec) return {};
    return {true, size, modified};
}

struct LiveReloadWatcher {
    FileStamp applied{};
    FileStamp candidate{};
    std::uint64_t changedAtMs{};
    bool pending{};

    constexpr void Resync(const FileStamp& current) noexcept {
        applied = current;
        candidate = current;
        changedAtMs = 0;
        pending = false;
    }

    [[nodiscard]] constexpr bool Observe(const FileStamp& current, std::uint64_t nowMs,
                                         std::uint64_t quietMs) noexcept {
        if (current == applied) {
            pending = false;
            return false;
        }
        if (!pending || current != candidate) {
            candidate = current;
            changedAtMs = nowMs;
            pending = true;
            return false;
        }
        if (nowMs < changedAtMs || nowMs - changedAtMs < quietMs) return false;
        applied = current;
        pending = false;
        return true;
    }
};

} // namespace PyreLootFilter::Schema
