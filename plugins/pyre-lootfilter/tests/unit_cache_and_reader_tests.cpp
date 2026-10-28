#include "audio_and_pickup.h"
#include "label_pipeline.h"
#include "minimap_pipeline.h"
#include "qol_interop.h"
#include "unit_cache.h"
#include "unit_reader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {
void Check(bool ok, const char* name) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}
} // namespace

int main() {
    using namespace PyreLootFilter;

    // 1. UnitEvaluationCache hit, miss, TTL, sellPrice TTL, ForgetUnit, and generation invalidation
    {
        static Cache::UnitEvaluationCache cache{};
        cache.Clear();
        Schema::ItemFacts fIn{};
        fIn.code = Tables::PackCode("7cr");
        fIn.socketsKnown = true;
        fIn.sockets = 4;
        fIn.sellPriceKnown = true;
        fIn.sellPrice = 35000;
        fIn.sellPriceObservedMs = 1000;

        Schema::EvaluatedDecision dIn{};
        dIn.matched = true;
        dIn.show = true;
        dIn.hasName = true;
        std::memcpy(dIn.name.data(), "Test Blade", 11);
        dIn.bytes = 11;

        cache.Store(101, fIn.code, 200, 7, 1000, fIn, dIn);

        Schema::ItemFacts fOut{};
        Schema::EvaluatedDecision dOut{};
        Check(cache.Lookup(101, fIn.code, 200, 7, 1500, true, fOut, dOut) &&
                  dOut.matched && fOut.sockets == 4,
              "cache hit within sellPrice TTL (500ms <= 1000ms)");
        Check(!cache.Lookup(101, fIn.code, 200, 7, 2100, true, fOut, dOut),
              "cache miss when sellPrice TTL expired (1100ms > 1000ms)");
        Check(cache.Lookup(101, fIn.code, 200, 7, 2100, false, fOut, dOut),
              "cache hit when ruleTableUsesSellPrice is false (1100ms <= 2500ms)");
        Check(!cache.Lookup(101, fIn.code, 200, 8, 1500, false, fOut, dOut),
              "cache miss on newer filter generation");

        Check(cache.LookupByUnitId(101, 7, 1500, false, fOut, dOut),
              "LookupByUnitId finds cached entry in O(1)");
        cache.ForgetUnit(101);
        Check(!cache.LookupByUnitId(101, 7, 1500, false, fOut, dOut),
              "ForgetUnit invalidates unit immediately");
    }

    // 2. ReadStatFromStatListEx on synthetic UnitAny + StatListEx
    {
        struct SyntheticStatEntry {
            std::uint64_t key;
            std::int32_t value;
            std::uint32_t pad;
        };
        const std::array<SyntheticStatEntry, 2> stats{{
            {(static_cast<std::uint64_t>(Reader::StatIdQuantity) << 32U) | 0U, 12, 0},
            {(static_cast<std::uint64_t>(Reader::StatIdNumSockets) << 32U) | 0U, 4, 0},
        }};

        alignas(16) std::array<std::uint8_t, 0x100> statList{};
        const std::uint32_t ownerType = Reader::UnitTypeItem;
        const std::uint32_t ownerId = 555U;
        const std::uint32_t listFlags = 0x80000000U; // extended StatListEx (uses +0xA8)
        std::memcpy(statList.data() + 0x08U, &ownerType, 4);
        std::memcpy(statList.data() + 0x0CU, &ownerId, 4);
        std::memcpy(statList.data() + 0x1CU, &listFlags, 4);

        const auto dataPtr = reinterpret_cast<std::uintptr_t>(stats.data());
        const std::uint64_t count = stats.size();
        const std::uint64_t cap = stats.size();
        std::memcpy(statList.data() + 0xA8U, &dataPtr, 8);
        std::memcpy(statList.data() + 0xB0U, &count, 8);
        std::memcpy(statList.data() + 0xB8U, &cap, 8);

        alignas(16) std::array<std::uint8_t, 0x100> unit{};
        const auto statListPtr = reinterpret_cast<std::uintptr_t>(statList.data());
        std::memcpy(unit.data() + 0x88U, &statListPtr, 8);

        std::int32_t sockets = -1, qty = -1, missing = -1;
        Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, Reader::StatIdNumSockets, 0,
                                             sockets) &&
                  sockets == 4,
              "ReadStatFromStatListEx reads STAT_ITEM_NUMSOCKETS = 4");
        Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, Reader::StatIdQuantity, 0,
                                             qty) &&
                  qty == 12,
              "ReadStatFromStatListEx reads STAT_QUANTITY = 12");
        Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 999U, 0, missing) &&
                  missing == 0,
              "ReadStatFromStatListEx returns 0 for absent stat");
        Check(!Reader::ReadStatFromStatListEx(unit.data(), 999U, Reader::StatIdNumSockets, 0,
                                              sockets),
              "ReadStatFromStatListEx rejects mismatched ownerId");

        // Test ReadAllStatsFromStatListEx with aggregate (+0xA8), ValShift=8 maxhp (7),
        // layered singleSkill (107, layer 112 = Blessed Hammer), and child list (+0x90 -> +0x30)
        // consumed op=13 enhancedDefense (16).
        const std::array<SyntheticStatEntry, 6> aggStats{{
            {(7ULL << 32U) | 0U, 40 * 256, 0},   // +40 Life (ValShift=8 -> 10240)
            {(39ULL << 32U) | 0U, 45, 0},        // +45 Fire Resist
            {(41ULL << 32U) | 0U, 45, 0},        // +45 Light Resist
            {(43ULL << 32U) | 0U, 45, 0},        // +45 Cold Resist
            {(45ULL << 32U) | 0U, 45, 0},        // +45 Poison Resist
            {(107ULL << 32U) | 112U, 3, 0},      // +3 Blessed Hammer (skill 112)
        }};
        const std::array<SyntheticStatEntry, 1> childStats{{
            {(16ULL << 32U) | 0U, 15, 0},        // +15% Enhanced Defense (op=13 in child list)
        }};

        alignas(16) std::array<std::uint8_t, 0x100> childList{};
        const auto childDataPtr = reinterpret_cast<std::uintptr_t>(childStats.data());
        const std::uint64_t childCount = childStats.size();
        const std::uint64_t childCap = childStats.size();
        std::memcpy(childList.data() + 0x30U, &childDataPtr, 8);
        std::memcpy(childList.data() + 0x38U, &childCount, 8);
        std::memcpy(childList.data() + 0x40U, &childCap, 8);

        const auto aggDataPtr = reinterpret_cast<std::uintptr_t>(aggStats.data());
        const std::uint64_t aggCount = aggStats.size();
        const std::uint64_t aggCap = aggStats.size();
        const auto childListPtr = reinterpret_cast<std::uintptr_t>(childList.data());
        std::memcpy(statList.data() + 0x90U, &childListPtr, 8);
        std::memcpy(statList.data() + 0xA8U, &aggDataPtr, 8);
        std::memcpy(statList.data() + 0xB0U, &aggCount, 8);
        std::memcpy(statList.data() + 0xB8U, &aggCap, 8);

        Schema::ItemFacts snap{};
        Check(Reader::ReadAllStatsFromStatListEx(unit.data(), ownerId, snap) && snap.statsKnown,
              "ReadAllStatsFromStatListEx snapshots aggregate + child StatListEx");
        Check(snap.GetStat(7, 0) == 40,
              "ReadAllStatsFromStatListEx normalizes ValShift=8 maxhp (40*256 -> 40)");
        Check(snap.GetStat(39, 0) == 45 && snap.GetStat(41, 0) == 45 &&
                  snap.GetStat(43, 0) == 45 && snap.GetStat(45, 0) == 45,
              "ReadAllStatsFromStatListEx reads all four elemental resistances");
        Check(snap.GetStat(107, 112) == 3,
              "ReadAllStatsFromStatListEx reads layered singleSkill(112) = 3");
        Check(snap.GetStat(16, 0) == 15,
              "ReadAllStatsFromStatListEx recovers op=13 enhancedDefense(16) = 15 from child list");
    }

    // 3. QuantityFormat::Append, ExpandNameTemplate, and MatchesRuleName
    {
        std::array<char, 64> out{};
        std::size_t len = 0;
        Check(Cache::QuantityFormat::Append("Flawless Amethyst", 12, out.data(), out.size(), len) &&
                  std::string_view(out.data(), len) == "12x Flawless Amethyst",
              "QuantityFormat::Append formats stack count");
        Check(!Cache::QuantityFormat::Append("12x Flawless Amethyst", 12, out.data(), out.size(),
                                             len),
              "QuantityFormat::Append refuses double prefixing");
        Check(Cache::QuantityFormat::MatchesRuleName("12x Flawless Amethyst", "Flawless Amethyst",
                                                     12),
              "MatchesRuleName accepts quantity-prefixed custom name");

        std::array<char, 64> expanded{};
        std::size_t expLen = 0;
        Check(Cache::QuantityFormat::ExpandNameTemplate("** {name} [GG] **", "Shako",
                                                        expanded.data(), expanded.size(), expLen) &&
                  std::string_view(expanded.data(), expLen) == "** Shako [GG] **",
              "ExpandNameTemplate expands {name} token with native item name");
        Check(Cache::QuantityFormat::ExpandNameTemplate("{NAME}!", "Ber Rune",
                                                        expanded.data(), expanded.size(), expLen) &&
                  std::string_view(expanded.data(), expLen) == "Ber Rune!",
              "ExpandNameTemplate is case-insensitive ({NAME})");
        Check(!Cache::QuantityFormat::ExpandNameTemplate("Static Name", "Ber Rune",
                                                         expanded.data(), expanded.size(), expLen),
              "ExpandNameTemplate returns false when no {name} token is present");
    }

    // 3b. Zero-native-call cache hit with wildcard code (knownCode == 0) & DropSoundAction::SuppressHidden
    {
        static Cache::UnitEvaluationCache cache{};
        cache.Clear();
        Schema::ItemFacts fIn{};
        fIn.code = Tables::PackCode("uap");
        fIn.quantityKnown = true;
        fIn.quantity = 1;
        Schema::EvaluatedDecision dIn{};
        dIn.matched = true;
        dIn.show = false; // Hidden item!
        cache.Store(777, fIn.code, 310, 5, GetTickCount64(), fIn, dIn);

        Schema::ItemFacts fOut{};
        Schema::EvaluatedDecision dOut{};
        Check(cache.Lookup(777, 0, 310, 5, GetTickCount64(), false, fOut, dOut) &&
                  dOut.matched && !dOut.show && fOut.code == Tables::PackCode("uap"),
              "cache.Lookup with code=0 matches by (unitId, classId, generation) with zero native calls");

        alignas(16) Reader::UnitHeader fakeUnit{
            .type = Reader::UnitTypeItem,
            .classId = 310,
            .unitId = 777,
            .mode = Reader::ItemModeDropping,
            .itemData = 0x10000,
        };
        Schema::FilterRuleTable table{};
        table.generation = 5;
        table.hiddenRules = 1;
        Reader::NativeReaders readers{}; // getItemCode is null -> proves cache hit avoids calling getItemCode!
        const auto dropAction = AudioAndPickup::EvaluateDropSound(
            &fakeUnit, AudioAndPickup::Rva::DropCallerReturn1, 0, true, &table, readers, cache);
        Check(dropAction == AudioAndPickup::DropSoundAction::SuppressHidden,
              "EvaluateDropSound returns SuppressHidden for hidden items so vanilla drop sound is silenced");
        Check(AudioAndPickup::ShouldBlockEvaluatedGroundPickup(fOut, dOut),
              "ShouldBlockEvaluatedGroundPickup blocks hidden non-gold items");
    }

    // 3c. Gold (gld) STAT_GOLD (stat 14) quantity evaluation & never-block-pickup invariant
    {
        struct SyntheticStatEntry {
            std::uint64_t key;
            std::int32_t value;
            std::uint32_t pad;
        };
        const std::array<SyntheticStatEntry, 1> goldStats{{
            {(static_cast<std::uint64_t>(Reader::StatIdGold) << 32U) | 0U, 2400, 0},
        }};

        alignas(16) std::array<std::uint8_t, 0x100> statList{};
        const std::uint32_t ownerType = Reader::UnitTypeItem;
        const std::uint32_t ownerId = 888U;
        const std::uint32_t listFlags = 0x80000000U;
        std::memcpy(statList.data() + 0x08U, &ownerType, 4);
        std::memcpy(statList.data() + 0x0CU, &ownerId, 4);
        std::memcpy(statList.data() + 0x1CU, &listFlags, 4);

        const auto dataPtr = reinterpret_cast<std::uintptr_t>(goldStats.data());
        const std::uint64_t count = goldStats.size();
        const std::uint64_t cap = goldStats.size();
        std::memcpy(statList.data() + 0xA8U, &dataPtr, 8);
        std::memcpy(statList.data() + 0xB0U, &count, 8);
        std::memcpy(statList.data() + 0xB8U, &cap, 8);

        // Player-dropped gold can have quality=0, itemLevel=0 in ItemData; verify an earlier
        // rarity-conditioned rule does not abort evaluation before reaching the Gold rule.
        alignas(16) std::array<std::uint8_t, 0x40> zeroItemData{};
        alignas(16) std::array<std::uint8_t, 0x100> goldUnit{};
        Reader::UnitHeader hdr{
            .type = Reader::UnitTypeItem,
            .classId = 523U,
            .unitId = ownerId,
            .mode = Reader::ItemModeOnGround,
            .itemData = reinterpret_cast<std::uintptr_t>(zeroItemData.data()),
        };
        std::memcpy(goldUnit.data(), &hdr, sizeof(hdr));
        const auto statListPtr = reinterpret_cast<std::uintptr_t>(statList.data());
        std::memcpy(goldUnit.data() + 0x88U, &statListPtr, 8);

        Schema::FilterRule rareRule{};
        rareRule.usesConditions = true;
        rareRule.conditions.qualities = {6U}; // rare
        rareRule.show = true;

        Schema::FilterRule hideSmallGold{};
        hideSmallGold.usesConditions = true;
        hideSmallGold.conditions.codes = {Tables::GoldItemCode};
        hideSmallGold.conditions.quantity.enabled = true;
        hideSmallGold.conditions.quantity.hasMax = true;
        hideSmallGold.conditions.quantity.max = 5000U;
        hideSmallGold.conditions.quantity.maxInclusive = false;
        hideSmallGold.show = false;

        Schema::FilterRuleTable table{};
        table.generation = 12;
        table.hiddenRules = 1;
        table.usesQuantity = true;
        table.usesQuality = true;
        table.rules = {rareRule, hideSmallGold};

        static Cache::UnitEvaluationCache cache{};
        cache.Clear();
        Reader::NativeReaders readers{};
        Schema::ItemFacts facts{};
        Schema::EvaluatedDecision decision{};

        Check(readers.EvaluateUnit(goldUnit.data(), Tables::GoldItemCode, &table, cache,
                                   Reader::Purpose::StrictGround, facts, decision) &&
                  decision.matched && !decision.show && facts.quantityKnown &&
                  facts.quantity == 2400U,
              "EvaluateUnit reads STAT_GOLD (14) = 2400 for gold and resolves hide rule (< 5000)");
        Check(!AudioAndPickup::ShouldBlockEvaluatedGroundPickup(facts, decision),
              "Gold pickup is NEVER blocked even when hidden by filter rules (decision.show == false)");

        // Also verify a large gold pile (125000 > 65535) is read from STAT_GOLD (14) and stays visible
        const std::array<SyntheticStatEntry, 1> bigGoldStats{{
            {(static_cast<std::uint64_t>(Reader::StatIdGold) << 32U) | 0U, 125000, 0},
        }};
        const auto bigDataPtr = reinterpret_cast<std::uintptr_t>(bigGoldStats.data());
        std::memcpy(statList.data() + 0xA8U, &bigDataPtr, 8);
        cache.Clear();

        Check(!readers.EvaluateUnit(goldUnit.data(), Tables::GoldItemCode, &table, cache,
                                    Reader::Purpose::StrictGround, facts, decision) &&
                  decision.show && facts.quantityKnown && facts.quantity == 125000U,
              "EvaluateUnit reads STAT_GOLD (14) = 125000 (> 65535) for gold and keeps it visible");
    }

    // 4. Labels::IsKnownNativeBlackBackground & ResolveEffectiveFormatterRecord
    {
        Check(Labels::IsKnownNativeBlackBackground({0.0f, 0.0f, 0.0f, 0.68f}),
              "accepts 0.68 native black alpha");
        Check(Labels::IsKnownNativeBlackBackground({0.0f, 0.0f, 0.0f, 0.85f}),
              "accepts 0.85 player-configured black alpha");
        Check(!Labels::IsKnownNativeBlackBackground({0.0f, 0.0f, 0.0f, 0.0f}),
              "rejects 0.0 invisible alpha");
        Check(!Labels::IsKnownNativeBlackBackground({0.2f, 0.0f, 0.0f, 0.8f}),
              "rejects non-black tint");

        alignas(16) std::array<std::uint8_t, 0x100> dummyRecord{};
        void* rec = dummyRecord.data();
        void* dest = dummyRecord.data() + Labels::Rva::GroundTextOffset;
        bool paired = false;
        Check(Labels::ResolveEffectiveFormatterRecord(true, false, false, dest, rec, paired) ==
                  rec &&
              paired,
              "callsite 1 resolves paired record");
        Check(Labels::ResolveEffectiveFormatterRecord(false, true, false, dest, rec, paired) ==
                  rec &&
              paired,
              "callsite 2 resolves record from dest - 0x24");
        Check(Labels::ResolveEffectiveFormatterRecord(false, false, true, dest, nullptr, paired) ==
                  rec &&
              paired,
              "controller callsite resolves record from dest - 0x24 even when r8 is null");

        const std::uint32_t nonZeroDim[2]{120U, 24U};
        const std::uint32_t validId = 4242U;
        std::memcpy(dummyRecord.data() + 0x08U, nonZeroDim, sizeof(nonZeroDim));
        std::memcpy(dummyRecord.data() + Labels::Rva::GroundRecordIdOffset, &validId,
                    sizeof(validId));
        std::memcpy(dest, "Hidden Item", 12);
        Labels::ResetRejectedFormatterRecord(rec, dest, true);
        std::uint32_t afterW = 1, afterH = 1, afterId = 0;
        std::memcpy(&afterW, dummyRecord.data() + 0x08U, 4);
        std::memcpy(&afterH, dummyRecord.data() + 0x0CU, 4);
        std::memcpy(&afterId, dummyRecord.data() + Labels::Rva::GroundRecordIdOffset, 4);
        Check(*static_cast<const char*>(dest) == '\0' && afterW == 0U && afterH == 0U &&
                  afterId == 0xFFFFFFFFU,
              "ResetRejectedFormatterRecord clears dest and restores unallocated record header");

        static int borderRectCalls = 0;
        static std::array<std::array<std::int32_t, 4>, 4> capturedEdges{};
        borderRectCalls = 0;
        const std::int32_t labelBox[4]{100, 50, 80, 24};
        const std::array<float, 4> borderRgba{1.0f, 0.84f, 0.0f, 1.0f};
        Labels::DrawLabelBorder(
            labelBox, borderRgba,
            +[]() noexcept -> bool { return true; },
            +[](std::uint64_t pt) noexcept -> std::uint64_t { return pt; },
            +[](std::int32_t l, std::int32_t t, std::int32_t r, std::int32_t b,
                const float*, std::int32_t filled) noexcept {
                if (filled == 1 && borderRectCalls < 4) {
                    capturedEdges[static_cast<std::size_t>(borderRectCalls)] = {l, t, r, b};
                }
                ++borderRectCalls;
            });
        Check(borderRectCalls == 4 &&
                  capturedEdges[0] == std::array<std::int32_t, 4>{99, 49, 181, 51} &&
                  capturedEdges[1] == std::array<std::int32_t, 4>{99, 73, 181, 75} &&
                  capturedEdges[2] == std::array<std::int32_t, 4>{99, 51, 101, 73} &&
                  capturedEdges[3] == std::array<std::int32_t, 4>{179, 51, 181, 73},
              "DrawLabelBorder emits 4 non-overlapping 2px perimeter strips around label rect");
    }

    // 5. AudioAndPickup::IsKeyboardAltHeld
    {
        Check(!AudioAndPickup::IsKeyboardAltHeld(0, 0), "Alt not held when both 0");
        Check(!AudioAndPickup::IsKeyboardAltHeld(1, 1),
              "Alt low bit (transition history) ignored when key is currently up");
        Check(AudioAndPickup::IsKeyboardAltHeld(static_cast<short>(0x8000), 0),
              "Left Alt high bit detected");
        Check(AudioAndPickup::IsKeyboardAltHeld(0, static_cast<short>(0x8000)),
              "Right Alt high bit detected");
    }

    // 6. Minimap coordinate projection helpers
    {
        Minimap::Point client{};
        Check(Minimap::WorldSubtileToClient(100, 50, client) && client.x == 800 &&
                  client.y == 1200,
              "WorldSubtileToClient isometric transform");
        const Minimap::ClipRect clip{100, 100, 400, 300};
        Check(Minimap::Contains(clip, {250, 250}) && !Minimap::Contains(clip, {50, 250}),
              "Minimap ClipRect bounds check");
    }

    std::puts("pyre-lootfilter cache, StatListEx reader, label, pickup, and minimap tests pass.");
    return 0;
}
