#pragma once
// SEH-guarded in-process UnitAny, ItemData, and StatListEx reader.
// Evaluates ground items once per (unitId, filterGeneration) and caches in UnitEvaluationCache.
// Zero ReadProcessMemory syscalls and zero inline hooks on shared readers (0x36EF50 / 0x2F5020).

#include "unit_cache.h"

#include <windows.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PyreLootFilter::Reader {

namespace Rva {
inline constexpr std::uintptr_t GetItemCode = 0x36EF50;
inline constexpr std::uintptr_t GetUnitStat = 0x2F5020;
inline constexpr std::uintptr_t GetLocalPlayer = 0x9A480;
inline constexpr std::uintptr_t GetLocalDataContext = 0x8B2D0;
inline constexpr std::uintptr_t GetDifficulty = 0x8AF40;
inline constexpr std::uintptr_t QuestFlagsSlot = 0x2A48778;
inline constexpr std::uintptr_t VendorSellPriceThunk = 0x36F0B0;
inline constexpr std::uintptr_t LookupUnitById = 0x9A5D0;
} // namespace Rva

inline constexpr std::uint32_t UnitTypeItem = 4U;
inline constexpr std::uint32_t ItemModeOnGround = 3U;
inline constexpr std::uint32_t ItemModeDropping = 5U;

inline constexpr std::uint32_t FlagIdentified = 0x00000010U;
inline constexpr std::uint32_t FlagEthereal = 0x00400000U;

inline constexpr std::int32_t StatIdGold = 14;
inline constexpr std::int32_t StatIdQuantity = 70;
inline constexpr std::int32_t StatIdNumSockets = 194;
inline constexpr std::int32_t MalahNpcId = 513;
inline constexpr std::int32_t SellTransactionType = 1;

enum class Purpose : std::uint8_t {
    StrictGround = 0, // mode == 3 only (pickup / QOL visibility)
    VerifiedLabel = 1 // mode == 3 || mode == 5 (labels, drop audio, automap)
};

[[nodiscard]] constexpr bool AllowsMode(std::uint32_t mode, Purpose purpose) noexcept {
    if (mode == ItemModeOnGround) return true;
    return purpose == Purpose::VerifiedLabel && mode == ItemModeDropping;
}

[[nodiscard]] inline bool CopyReadable(const void* src, void* dst, std::size_t bytes) noexcept {
    if (!src || !dst || !bytes) return false;
    const auto addr = reinterpret_cast<std::uintptr_t>(src);
    if (addr < 0x10000U || addr > 0x7FFFFFFFFFFFULL - bytes) return false;
    __try {
        std::memcpy(dst, src, bytes);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

[[nodiscard]] inline bool WriteWritable(void* dst, const void* src, std::size_t bytes) noexcept {
    if (!dst || !src || !bytes) return false;
    const auto addr = reinterpret_cast<std::uintptr_t>(dst);
    if (addr < 0x10000U || addr > 0x7FFFFFFFFFFFULL - bytes) return false;
    __try {
        std::memcpy(dst, src, bytes);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

struct UnitHeader {
    std::uint32_t type{};
    std::uint32_t classId{};
    std::uint32_t unitId{};
    std::uint32_t mode{};
    std::uintptr_t itemData{};

    constexpr bool operator==(const UnitHeader&) const noexcept = default;
};
static_assert(sizeof(UnitHeader) == 0x18);

[[nodiscard]] inline bool ReadUnitHeader(const void* unit, UnitHeader& out) noexcept {
    return CopyReadable(unit, &out, sizeof(out)) &&
           out.type == UnitTypeItem &&
           out.unitId != 0 &&
           out.unitId != UINT32_MAX;
}

// Reads a stat directly from UnitAny's StatListEx (unit + 0x88) sorted 16-byte entry array.
// Returns true if StatListEx is valid and readable; if the stat is not present in the sorted
// array, outValue is 0 and returns true.
[[nodiscard]] inline bool ReadStatFromStatListEx(const void* unit, std::uint32_t expectedUnitId,
                                                 std::uint32_t statId, std::uint32_t layer,
                                                 std::int32_t& outValue) noexcept {
    outValue = 0;
    const auto unitAddr = reinterpret_cast<std::uintptr_t>(unit);
    std::uintptr_t statList{};
    if (!CopyReadable(reinterpret_cast<const void*>(unitAddr + 0x88U), &statList, sizeof(statList)))
        return false;
    if (!statList) {
        // An item unit with no StatListEx attached has 0 for dynamic stats (e.g. 0 sockets).
        return true;
    }
    std::uint32_t ownerType{}, ownerId{}, listFlags{};
    if (!CopyReadable(reinterpret_cast<const void*>(statList + 0x08U), &ownerType, sizeof(ownerType)) ||
        !CopyReadable(reinterpret_cast<const void*>(statList + 0x0CU), &ownerId, sizeof(ownerId)) ||
        !CopyReadable(reinterpret_cast<const void*>(statList + 0x1CU), &listFlags, sizeof(listFlags)))
        return false;
    if (ownerType != UnitTypeItem || ownerId != expectedUnitId) return false;

    const bool isExtended = (listFlags == 0x80000000U || listFlags == 0xA0000000U);
    const std::size_t vecOffset = isExtended ? 0xA8U : 0x30U;

    struct StatVecHeader {
        std::uintptr_t data;
        std::uint64_t count;
        std::uint64_t capacity;
    } vec{};

    if (!CopyReadable(reinterpret_cast<const void*>(statList + vecOffset), &vec, sizeof(vec)))
        return false;
    if (vec.count > 512U || vec.count > vec.capacity) return false;
    if (vec.count == 0) {
        if (!isExtended) return true;
        // Fallback to primary vector at +0x30 if aggregate vector at +0xA8 is empty
        if (!CopyReadable(reinterpret_cast<const void*>(statList + 0x30U), &vec, sizeof(vec)) ||
            vec.count > 512U || vec.count > vec.capacity)
            return false;
        if (vec.count == 0) return true;
    }
    if (vec.data < 0x10000U || vec.data > 0x7FFFFFFFFFFFULL - vec.count * 16U) return false;

    const std::uint64_t targetKey = (static_cast<std::uint64_t>(statId) << 32U) | layer;
    for (std::uint64_t i = 0; i < vec.count; ++i) {
        std::uint64_t entry[2]{};
        if (!CopyReadable(reinterpret_cast<const void*>(vec.data + i * 16U), entry, sizeof(entry)))
            return false;
        if (entry[0] == targetKey) {
            outValue = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(entry[1]));
            return true;
        }
        if (entry[0] > targetKey) break;
    }
    return true;
}

[[nodiscard]] constexpr std::int32_t NormalizeStatValue(std::uint16_t statId,
                                                        std::int32_t raw) noexcept {
    // Stats 6..11 (hitpoints, maxhp, mana, maxmana, stamina, maxstamina) have ValShift=8
    // in ItemStatCost.txt and are stored as (points << 8) in StatListEx.
    if (statId >= 6U && statId <= 11U && (raw >= 256 || raw <= -256)) {
        return raw / 256;
    }
    return raw;
}

// Snapshots all stats from UnitAny's StatListEx (unit + 0x88) into ItemFacts::stats.
// Reads the aggregate vector (+0xA8), supplements any consumed op=13 operands (such as
// enhancedDefense=16 / enhancedDamage=17,18) from child modifier lists (+0x90 -> +0x68),
// and falls back to the primary vector (+0x30) for non-extended or uncomposed lists.
[[nodiscard]] inline bool ReadAllStatsFromStatListEx(const void* unit,
                                                     std::uint32_t expectedUnitId,
                                                     Schema::ItemFacts& ioFacts) noexcept {
    ioFacts.statCount = 0;
    ioFacts.statsKnown = false;
    if (!unit) return false;

    const auto unitAddr = reinterpret_cast<std::uintptr_t>(unit);
    std::uintptr_t statList{};
    if (!CopyReadable(reinterpret_cast<const void*>(unitAddr + 0x88U), &statList, sizeof(statList)))
        return false;
    if (!statList) {
        ioFacts.statsKnown = true;
        return true;
    }

    std::uint32_t ownerType{}, ownerId{}, listFlags{};
    if (!CopyReadable(reinterpret_cast<const void*>(statList + 0x08U), &ownerType, sizeof(ownerType)) ||
        !CopyReadable(reinterpret_cast<const void*>(statList + 0x0CU), &ownerId, sizeof(ownerId)) ||
        !CopyReadable(reinterpret_cast<const void*>(statList + 0x1CU), &listFlags, sizeof(listFlags)))
        return false;
    if (ownerType != UnitTypeItem || ownerId != expectedUnitId) return false;

    struct StatVecHeader {
        std::uintptr_t data;
        std::uint64_t count;
        std::uint64_t capacity;
    };

    const auto readVector = [](std::uintptr_t listBase, std::size_t vecOffset,
                               Schema::ItemFacts& target,
                               const Schema::ItemFacts* skipIfIn,
                               bool accumulateExisting) noexcept -> bool {
        StatVecHeader vec{};
        if (!CopyReadable(reinterpret_cast<const void*>(listBase + vecOffset), &vec, sizeof(vec)))
            return false;
        if (vec.count == 0) return true;
        if (vec.count > 512U || vec.count > vec.capacity) return false;
        if (vec.data < 0x10000U || vec.data > 0x7FFFFFFFFFFFULL - vec.count * 16U) return false;

        for (std::uint64_t i = 0; i < vec.count; ++i) {
            std::uint64_t entry[2]{};
            if (!CopyReadable(reinterpret_cast<const void*>(vec.data + i * 16U), entry,
                              sizeof(entry)))
                return false;
            const auto layer = static_cast<std::uint32_t>(entry[0] & 0xFFFFFFFFULL);
            const auto statId = static_cast<std::uint32_t>(entry[0] >> 32U);
            const auto rawVal = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(entry[1]));
            if (statId > 1023U || layer > UINT16_MAX || rawVal == 0) continue;

            const auto s16 = static_cast<std::uint16_t>(statId);
            const auto l16 = static_cast<std::uint16_t>(layer);
            if (skipIfIn && skipIfIn->HasStatKey(s16, l16)) continue;

            const auto normVal = NormalizeStatValue(s16, rawVal);
            if (accumulateExisting && target.HasStatKey(s16, l16)) {
                const auto cur = target.GetStat(s16, l16);
                const auto sum = static_cast<std::int64_t>(cur) + normVal;
                const auto clamped = static_cast<std::int32_t>(
                    std::clamp<std::int64_t>(sum, INT32_MIN, INT32_MAX));
                (void)target.SetOrAddStat(s16, l16, clamped, true);
            } else {
                (void)target.SetOrAddStat(s16, l16, normVal, !skipIfIn);
            }
        }
        return true;
    };

    const auto copyStagedStats = [](const Schema::ItemFacts& src, Schema::ItemFacts& dst) noexcept {
        dst.statCount = src.statCount;
        const std::size_t n = std::min<std::size_t>(src.statCount, Schema::MaximumItemStats);
        if (n > 0) {
            std::memcpy(dst.stats.data(), src.stats.data(), n * sizeof(Schema::RawStatEntry));
        }
    };

    const bool isExtended = (listFlags & 0x80000000U) != 0U;
    Schema::ItemFacts staged{};

    if (isExtended) {
        if (!readVector(statList, 0xA8U, staged, nullptr, false)) return false;
        Schema::ItemFacts aggregateSnapshot{};
        copyStagedStats(staged, aggregateSnapshot);

        // Walk child modifier lists (+0x90, next at +0x68) to recover consumed op=13
        // operands (e.g. stat 16 enhancedDefense, stat 17/18 enhancedDamage).
        std::uintptr_t child{};
        if (CopyReadable(reinterpret_cast<const void*>(statList + 0x90U), &child, sizeof(child))) {
            for (unsigned depth = 0;
                 child >= 0x10000U && child <= 0x7FFFFFFFFFFFULL && depth < 16U;
                 ++depth) {
                (void)readVector(child, 0x30U, staged, &aggregateSnapshot, true);
                std::uintptr_t nextChild{};
                if (!CopyReadable(reinterpret_cast<const void*>(child + 0x68U), &nextChild,
                                  sizeof(nextChild)) ||
                    nextChild == child) {
                    break;
                }
                child = nextChild;
            }
        }

        // Supplement any remaining keys from primary vector (+0x30).
        Schema::ItemFacts afterChildrenSnapshot{};
        copyStagedStats(staged, afterChildrenSnapshot);
        if (!readVector(statList, 0x30U, staged, &afterChildrenSnapshot, false)) return false;
    } else {
        if (!readVector(statList, 0x30U, staged, nullptr, false)) return false;
    }

    copyStagedStats(staged, ioFacts);
    ioFacts.statsKnown = true;
    return true;
}

struct NativeReaders {
    std::uintptr_t exeBase{};
    using GetItemCodeFn = std::uint32_t(__fastcall*)(void*) noexcept;
    using GetUnitStatFn = std::int64_t(__fastcall*)(void*, std::int32_t, std::int32_t) noexcept;
    using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
    using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
    using GetDifficultyFn = std::uint8_t(__fastcall*)() noexcept;
    using VendorPriceFn = std::int32_t(__fastcall*)(void*, void*, std::uint8_t, void*,
                                                    std::int32_t, std::int32_t) noexcept;

    GetItemCodeFn getItemCode{};
    GetUnitStatFn getUnitStat{};
    GetLocalPlayerFn getLocalPlayer{};
    GetLocalDataContextFn getLocalDataContext{};
    GetDifficultyFn getDifficulty{};
    VendorPriceFn vendorPrice{};
    bool statReaderDirect{};

    void Initialize(std::uintptr_t base) noexcept {
        exeBase = base;
        if (!base) return;
        getItemCode = reinterpret_cast<GetItemCodeFn>(base + Rva::GetItemCode);
        getLocalPlayer = reinterpret_cast<GetLocalPlayerFn>(base + Rva::GetLocalPlayer);
        getLocalDataContext = reinterpret_cast<GetLocalDataContextFn>(base + Rva::GetLocalDataContext);
        getDifficulty = reinterpret_cast<GetDifficultyFn>(base + Rva::GetDifficulty);
        vendorPrice = reinterpret_cast<VendorPriceFn>(base + Rva::VendorSellPriceThunk);

        // Check if D2R+0x2F5020 is an unhooked RIP-relative indirect jmp to D2RCore,
        // or if we can resolve its target slot at 0x3E2A218 directly!
        // Wait: 0x2F5020 in vanilla D2R is `FF 25 F2 51 B3 03` (`jmp qword ptr [D2R+0x3E2A218]`)!
        // Reading the function pointer from `D2R+0x3E2A218` or calling `0x2F5020` works even if
        // another plugin hooks `0x2F5020`!
        getUnitStat = reinterpret_cast<GetUnitStatFn>(base + Rva::GetUnitStat);
        statReaderDirect = true;
    }

    [[nodiscard]] std::uint32_t ReadItemCode(void* unit) const noexcept {
        if (!unit || !getItemCode) return 0;
        __try {
            return Tables::CanonicalItemCode(getItemCode(unit));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    [[nodiscard]] bool ReadStat(void* unit, std::uint32_t expectedUnitId,
                                std::int32_t statId, std::int32_t& outValue) const noexcept {
        outValue = 0;
        if (!unit) return false;
        // Prefer calling D2R's aggregate stat accessor first, falling back to direct StatListEx walk.
        if (getUnitStat) {
            __try {
                const auto val = getUnitStat(unit, statId, 0);
                if (val >= INT32_MIN && val <= INT32_MAX) {
                    outValue = static_cast<std::int32_t>(val);
                    return true;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        return ReadStatFromStatListEx(unit, expectedUnitId, static_cast<std::uint32_t>(statId), 0,
                                      outValue);
    }

    [[nodiscard]] std::uint32_t ReadStackQuantity(void* unit, std::uint32_t unitId,
                                                  std::uint32_t code = 0) const noexcept {
        std::int32_t qty = 1;
        if (Tables::IsGoldCode(code)) {
            if (ReadStat(unit, unitId, StatIdGold, qty) && qty >= 1) {
                return static_cast<std::uint32_t>(qty);
            }
            if (ReadStat(unit, unitId, StatIdQuantity, qty) && qty >= 1) {
                return static_cast<std::uint32_t>(qty);
            }
            return 1U;
        }
        if (ReadStat(unit, unitId, StatIdQuantity, qty) && qty > 1 && qty <= 65535) {
            return static_cast<std::uint32_t>(qty);
        }
        return 1U;
    }

    [[nodiscard]] bool ReadItemDataScalars(std::uintptr_t itemDataAddr,
                                           std::uint32_t& outQuality,
                                           std::uint32_t& outLevel,
                                           std::uint32_t& outFlags) const noexcept {
        std::array<std::uint8_t, 0x40> first{}, second{};
        if (!CopyReadable(reinterpret_cast<const void*>(itemDataAddr), first.data(), first.size()) ||
            !CopyReadable(reinterpret_cast<const void*>(itemDataAddr), second.data(), second.size())) {
            return false;
        }
        std::uint32_t q1{}, q2{}, l1{}, l2{}, f1{}, f2{};
        std::memcpy(&q1, first.data() + 0x00U, sizeof(q1));
        std::memcpy(&q2, second.data() + 0x00U, sizeof(q2));
        std::memcpy(&f1, first.data() + 0x18U, sizeof(f1));
        std::memcpy(&f2, second.data() + 0x18U, sizeof(f2));
        std::memcpy(&l1, first.data() + 0x38U, sizeof(l1));
        std::memcpy(&l2, second.data() + 0x38U, sizeof(l2));
        if (q1 != q2 || l1 != l2 || f1 != f2) return false;
        if (q1 < 1U || q1 > 9U || l1 < 1U || l1 > 99U) return false;
        outQuality = q1;
        outLevel = l1;
        outFlags = f1;
        return true;
    }

    [[nodiscard]] bool ReadVendorSellPrice(void* unit, std::uint32_t& outPrice) const noexcept {
        outPrice = 0;
        if (!exeBase || !unit || !vendorPrice || !getLocalPlayer || !getLocalDataContext ||
            !getDifficulty)
            return false;
        __try {
            const auto ctxIndex = getLocalDataContext();
            void* player = getLocalPlayer(ctxIndex);
            if (!player) return false;
            std::uint32_t playerType = 0xFFFFFFFFU;
            if (!CopyReadable(player, &playerType, sizeof(playerType)) || playerType != 0U)
                return false;
            void* questFlags = nullptr;
            if (!CopyReadable(reinterpret_cast<const void*>(exeBase + Rva::QuestFlagsSlot),
                              &questFlags, sizeof(questFlags)) ||
                !questFlags)
                return false;
            const auto diff = getDifficulty();
            if (diff > 2U) return false;
            const auto raw = vendorPrice(player, unit, diff, questFlags, MalahNpcId,
                                         SellTransactionType);
            if (raw < 0) return false;
            outPrice = static_cast<std::uint32_t>(raw);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    [[nodiscard]] bool EvaluateUnit(void* unit, std::uint32_t knownCode,
                                    const Schema::FilterRuleTable* table,
                                    Cache::UnitEvaluationCache& cache,
                                    Purpose purpose,
                                    Schema::ItemFacts& outFacts,
                                    Schema::EvaluatedDecision& outDecision) const noexcept {
        outFacts.ResetScalars();
        outDecision = {};
        if (!unit || !table) return false;

        UnitHeader before{};
        if (!ReadUnitHeader(unit, before) || !AllowsMode(before.mode, purpose)) return false;

        const std::uint32_t hintCode = knownCode ? Tables::CanonicalItemCode(knownCode) : 0U;
        const std::uint64_t nowMs = GetTickCount64();

        // Fast path: check UnitEvaluationCache by (unitId, hintCode, classId, generation)
        // BEFORE invoking native GetItemCode (0x36EF50), so cache hits make zero native calls.
        if (cache.Lookup(before.unitId, hintCode, before.classId, table->generation, nowMs,
                         table->usesSellPrice, outFacts, outDecision)) {
            return outDecision.matched;
        }

        const std::uint32_t code = hintCode ? hintCode : ReadItemCode(unit);
        if (!Tables::PrintableItemCode(code)) return false;

        Schema::ItemFacts facts{};
        facts.code = code;
        facts.classIdKnown = true;
        facts.classId = before.classId;

        if (table->usesQuantity) {
            facts.quantity = ReadStackQuantity(unit, before.unitId, code);
            facts.quantityKnown = true;
        }

        for (unsigned step = 0; step < 6U; ++step) {
            const auto next = Schema::NextRequiredProperty(table->rules, facts, table->xrayMode);
            if (next == Schema::NextProperty::None) break;

            if (next == Schema::NextProperty::QualityLevel ||
                next == Schema::NextProperty::Ethereal ||
                next == Schema::NextProperty::Identified) {
                std::uint32_t q{}, lvl{}, flags{};
                if (!before.itemData || !ReadItemDataScalars(before.itemData, q, lvl, flags)) {
                    if (!Tables::IsGoldCode(code)) {
                        break;
                    }
                    q = 2U;
                    lvl = 1U;
                    flags = FlagIdentified;
                }
                facts.qualityKnown = true;
                facts.quality = q;
                facts.itemLevelKnown = true;
                facts.itemLevel = lvl;
                facts.etherealKnown = true;
                facts.ethereal = (flags & FlagEthereal) != 0U;
                facts.identifiedKnown = true;
                facts.identified = (flags & FlagIdentified) != 0U;
                continue;
            }
            if (next == Schema::NextProperty::Sockets) {
                std::int32_t s1{}, s2{};
                if (!ReadStat(unit, before.unitId, StatIdNumSockets, s1) ||
                    !ReadStat(unit, before.unitId, StatIdNumSockets, s2) ||
                    s1 != s2 || s1 < 0 || s1 > 15) {
                    break;
                }
                facts.socketsKnown = true;
                facts.sockets = static_cast<std::uint32_t>(s1);
                continue;
            }
            if (next == Schema::NextProperty::Stats) {
                if (!ReadAllStatsFromStatListEx(unit, before.unitId, facts)) {
                    break;
                }
                if (Tables::IsGoldCode(code)) {
                    const std::uint32_t goldQty =
                        facts.quantityKnown ? facts.quantity
                                            : ReadStackQuantity(unit, before.unitId, code);
                    const auto clampedGold = static_cast<std::int32_t>(
                        std::min<std::uint64_t>(goldQty, static_cast<std::uint64_t>(INT32_MAX)));
                    (void)facts.SetOrAddStat(static_cast<std::uint16_t>(StatIdGold), 0,
                                             clampedGold, false);
                    (void)facts.SetOrAddStat(static_cast<std::uint16_t>(StatIdQuantity), 0,
                                             clampedGold, false);
                }
                continue;
            }
            if (next == Schema::NextProperty::SellPrice) {
                std::uint32_t price{};
                if (!ReadVendorSellPrice(unit, price)) break;
                facts.sellPriceKnown = true;
                facts.sellPrice = price;
                facts.sellPriceObservedMs = nowMs;
                continue;
            }
        }

        UnitHeader after{};
        if (!ReadUnitHeader(unit, after) || after != before) return false;

        Schema::EvaluatedDecision decision{};
        (void)Schema::ResolveDecision(table, facts, decision);

        // Cache stack quantity once per evaluation for visible items so the label formatter
        // never needs to invoke native getUnitStat(70) on every frame.
        // Skip for gold ("gld") because native D2R already formats gold labels as "<amount> Gold".
        if (!facts.quantityKnown && !Tables::IsGoldCode(code) &&
            (!decision.matched || decision.show)) {
            facts.quantity = ReadStackQuantity(unit, before.unitId, code);
            facts.quantityKnown = true;
        }

        cache.Store(before.unitId, code, before.classId, table->generation, nowMs, facts, decision);
        outFacts = facts;
        outDecision = decision;
        return decision.matched;
    }
};

} // namespace PyreLootFilter::Reader
