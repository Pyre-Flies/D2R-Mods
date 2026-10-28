#pragma once
// O(1) per-unit evaluation cache keyed by (unitId, code, classId, filterGeneration)
// plus stack-quantity label formatting ("12x Item Name").

#include "filter_schema.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string_view>

namespace PyreLootFilter::Cache {

inline constexpr std::size_t Capacity = 2048U;
inline constexpr std::size_t ProbeWindow = 8U;
inline constexpr std::uint64_t DefaultTtlMs = 2500U;
inline constexpr std::uint64_t SellPriceTtlMs = 1000U;

struct CachedFacts {
    std::uint32_t code{};
    std::uint32_t classId{};
    std::uint32_t quantity{};
    std::uint32_t quality{};
    std::uint32_t itemLevel{};
    std::uint32_t sockets{};
    std::uint32_t sellPrice{};
    std::uint64_t sellPriceObservedMs{};
    bool classIdKnown{};
    bool quantityKnown{};
    bool qualityKnown{};
    bool itemLevelKnown{};
    bool socketsKnown{};
    bool etherealKnown{};
    bool ethereal{};
    bool identifiedKnown{};
    bool identified{};
    bool sellPriceKnown{};

    static constexpr CachedFacts FromItemFacts(const Schema::ItemFacts& f) noexcept {
        return CachedFacts{
            .code = f.code,
            .classId = f.classId,
            .quantity = f.quantity,
            .quality = f.quality,
            .itemLevel = f.itemLevel,
            .sockets = f.sockets,
            .sellPrice = f.sellPrice,
            .sellPriceObservedMs = f.sellPriceObservedMs,
            .classIdKnown = f.classIdKnown,
            .quantityKnown = f.quantityKnown,
            .qualityKnown = f.qualityKnown,
            .itemLevelKnown = f.itemLevelKnown,
            .socketsKnown = f.socketsKnown,
            .etherealKnown = f.etherealKnown,
            .ethereal = f.ethereal,
            .identifiedKnown = f.identifiedKnown,
            .identified = f.identified,
            .sellPriceKnown = f.sellPriceKnown,
        };
    }

    constexpr void ToItemFacts(Schema::ItemFacts& out) const noexcept {
        out.code = code;
        out.classIdKnown = classIdKnown;
        out.classId = classId;
        out.quantityKnown = quantityKnown;
        out.quantity = quantity;
        out.qualityKnown = qualityKnown;
        out.quality = quality;
        out.itemLevelKnown = itemLevelKnown;
        out.itemLevel = itemLevel;
        out.socketsKnown = socketsKnown;
        out.sockets = sockets;
        out.etherealKnown = etherealKnown;
        out.ethereal = ethereal;
        out.identifiedKnown = identifiedKnown;
        out.identified = identified;
        out.sellPriceKnown = sellPriceKnown;
        out.sellPrice = sellPrice;
        out.sellPriceObservedMs = sellPriceObservedMs;
        out.statsKnown = false;
        out.statCount = 0;
    }
};

struct Entry {
    std::uint32_t unitId{};
    std::uint32_t code{};
    std::uint32_t classId{};
    std::uint64_t generation{};
    std::uint64_t observedMs{};
    CachedFacts facts{};
    Schema::EvaluatedDecision decision{};
};

class UnitEvaluationCache {
public:
    void Clear() noexcept {
        std::lock_guard lock(mutex_);
        entries_.fill({});
    }

    void ForgetUnit(std::uint32_t unitId) noexcept {
        if (!unitId) return;
        std::lock_guard lock(mutex_);
        const auto base = Hash(unitId);
        for (std::size_t i = 0; i < ProbeWindow; ++i) {
            auto& slot = entries_[(base + i) & (Capacity - 1U)];
            if (slot.unitId == unitId) {
                slot = {};
            }
        }
    }

    [[nodiscard]] bool Lookup(std::uint32_t unitId, std::uint32_t code, std::uint32_t classId,
                              std::uint64_t generation, std::uint64_t nowMs,
                              bool ruleTableUsesSellPrice,
                              Schema::ItemFacts& outFacts,
                              Schema::EvaluatedDecision& outDecision) noexcept {
        if (!unitId || !generation) return false;
        if (!mutex_.try_lock()) return false;
        const auto base = Hash(unitId);
        bool hit = false;
        for (std::size_t i = 0; i < ProbeWindow; ++i) {
            const auto& slot = entries_[(base + i) & (Capacity - 1U)];
            if (slot.unitId != unitId) continue;
            if ((code == 0 || slot.code == code) && slot.classId == classId &&
                slot.generation == generation && nowMs >= slot.observedMs &&
                (nowMs - slot.observedMs) <= DefaultTtlMs) {
                if (ruleTableUsesSellPrice && slot.facts.sellPriceKnown) {
                    if (!slot.facts.sellPriceObservedMs || nowMs < slot.facts.sellPriceObservedMs ||
                        (nowMs - slot.facts.sellPriceObservedMs) > SellPriceTtlMs) {
                        break;
                    }
                }
                slot.facts.ToItemFacts(outFacts);
                outDecision = slot.decision;
                hit = true;
            }
            break;
        }
        mutex_.unlock();
        return hit;
    }

    [[nodiscard]] bool LookupByUnitId(std::uint32_t unitId, std::uint64_t generation,
                                      std::uint64_t nowMs, bool ruleTableUsesSellPrice,
                                      Schema::ItemFacts& outFacts,
                                      Schema::EvaluatedDecision& outDecision) noexcept {
        if (!unitId || !generation) return false;
        if (!mutex_.try_lock()) return false;
        const auto base = Hash(unitId);
        bool hit = false;
        for (std::size_t i = 0; i < ProbeWindow; ++i) {
            const auto& slot = entries_[(base + i) & (Capacity - 1U)];
            if (slot.unitId != unitId) continue;
            if (slot.generation == generation && nowMs >= slot.observedMs &&
                (nowMs - slot.observedMs) <= DefaultTtlMs) {
                if (ruleTableUsesSellPrice && slot.facts.sellPriceKnown) {
                    if (!slot.facts.sellPriceObservedMs || nowMs < slot.facts.sellPriceObservedMs ||
                        (nowMs - slot.facts.sellPriceObservedMs) > SellPriceTtlMs) {
                        break;
                    }
                }
                slot.facts.ToItemFacts(outFacts);
                outDecision = slot.decision;
                hit = true;
            }
            break;
        }
        mutex_.unlock();
        return hit;
    }

    void Store(std::uint32_t unitId, std::uint32_t code, std::uint32_t classId,
               std::uint64_t generation, std::uint64_t nowMs,
               const Schema::ItemFacts& facts,
               const Schema::EvaluatedDecision& decision) noexcept {
        if (!unitId || !generation) return;
        if (!mutex_.try_lock()) return;
        const auto base = Hash(unitId);
        std::size_t target = base;
        std::uint64_t oldestMs = UINT64_MAX;
        for (std::size_t i = 0; i < ProbeWindow; ++i) {
            const auto idx = (base + i) & (Capacity - 1U);
            const auto& slot = entries_[idx];
            if (slot.unitId == unitId || slot.unitId == 0 || slot.generation != generation ||
                nowMs < slot.observedMs || (nowMs - slot.observedMs) > DefaultTtlMs) {
                target = idx;
                break;
            }
            if (slot.observedMs < oldestMs) {
                oldestMs = slot.observedMs;
                target = idx;
            }
        }
        entries_[target] = Entry{
            .unitId = unitId,
            .code = code,
            .classId = classId,
            .generation = generation,
            .observedMs = nowMs,
            .facts = CachedFacts::FromItemFacts(facts),
            .decision = decision,
        };
        mutex_.unlock();
    }

private:
    [[nodiscard]] static constexpr std::size_t Hash(std::uint32_t unitId) noexcept {
        return static_cast<std::size_t>((unitId * 2654435761U) & (Capacity - 1U));
    }

    std::mutex mutex_;
    std::array<Entry, Capacity> entries_{};
};

namespace QuantityFormat {

[[nodiscard]] inline bool HasCountPrefix(std::string_view text) noexcept {
    std::size_t n{};
    while (n < text.size() && text[n] >= '0' && text[n] <= '9') ++n;
    return n > 0 && n + 2 < text.size() && text[n] == 'x' && text[n + 1] == ' ';
}

// Expands "{name}" (case-insensitive) in a custom rule name using the item's native name
// with zero heap allocations. Returns false if no "{name}" token is present or if the
// expanded string would exceed capacity.
inline bool ExpandNameTemplate(std::string_view ruleName, std::string_view nativeName,
                               char* output, std::size_t capacity,
                               std::size_t& outBytes) noexcept {
    outBytes = 0;
    if (!output || capacity == 0 || ruleName.empty() || nativeName.empty()) return false;
    if (ruleName.find('{') == std::string_view::npos) return false;

    const auto isNameToken = [](std::string_view s, std::size_t pos) noexcept -> bool {
        if (pos + 6U > s.size() || s[pos] != '{' || s[pos + 5U] != '}') return false;
        const auto lower = [](char c) noexcept -> char {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        };
        return lower(s[pos + 1U]) == 'n' && lower(s[pos + 2U]) == 'a' &&
               lower(s[pos + 3U]) == 'm' && lower(s[pos + 4U]) == 'e';
    };

    bool replaced = false;
    std::size_t cursor = 0;
    for (std::size_t i = 0; i < ruleName.size();) {
        if (isNameToken(ruleName, i)) {
            if (cursor + nativeName.size() >= capacity) {
                outBytes = 0;
                return false;
            }
            std::memcpy(output + cursor, nativeName.data(), nativeName.size());
            cursor += nativeName.size();
            i += 6U;
            replaced = true;
        } else {
            if (cursor + 1U >= capacity) {
                outBytes = 0;
                return false;
            }
            output[cursor++] = ruleName[i++];
        }
    }
    if (!replaced) {
        outBytes = 0;
        return false;
    }
    output[cursor] = '\0';
    outBytes = cursor;
    return true;
}

inline bool Append(std::string_view name, std::uint32_t quantity,
                   char* output, std::size_t capacity, std::size_t& bytes) noexcept {
    bytes = 0;
    if (!output || quantity <= 1U || quantity > 65535U || name.empty() ||
        HasCountPrefix(name) || name.find('\0') != name.npos ||
        name.find('\r') != name.npos || name.find('\n') != name.npos) {
        return false;
    }
    char number[6]{};
    const auto [end, ec] = std::to_chars(number, number + sizeof(number), quantity);
    if (ec != std::errc{}) return false;
    const auto digits = static_cast<std::size_t>(end - number);
    if (name.size() > capacity || capacity - name.size() < digits + 3U) return false;
    std::memcpy(output, number, digits);
    bytes = digits;
    output[bytes++] = 'x';
    output[bytes++] = ' ';
    std::memcpy(output + bytes, name.data(), name.size());
    bytes += name.size();
    output[bytes] = '\0';
    return true;
}

[[nodiscard]] inline bool MatchesRuleName(std::string_view displayed,
                                          std::string_view configured,
                                          std::uint32_t quantity) noexcept {
    if (displayed.empty() || configured.empty()) return false;
    if (displayed == configured) return true;
    std::array<char, 256> expected{};
    std::size_t size{};
    return Append(configured, quantity, expected.data(), expected.size(), size) &&
           displayed == std::string_view(expected.data(), size);
}

} // namespace QuantityFormat

} // namespace PyreLootFilter::Cache
