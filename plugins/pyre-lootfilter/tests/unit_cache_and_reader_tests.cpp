#include "audio_and_pickup.h"
#include "build_admission.h"
#include "label_pipeline.h"
#include "minimap_pipeline.h"
#include "qol_interop.h"
#include "unit_cache.h"
#include "unit_reader.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

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

        // Even if a decision has hasDropSound=true and hasMinimapIcon=true while show=false,
        // EvaluateDropSound and MinimapState must suppress both.
        dIn.hasDropSound = true;
        std::memcpy(dIn.dropSound.data(), "Filter01", 9);
        dIn.hasMinimapIcon = true;
        cache.Store(777, fIn.code, 310, 5, GetTickCount64(), fIn, dIn);
        table.soundRules = 1;
        table.minimapIconRules = 1;

        static bool customSoundCalled = false;
        customSoundCalled = false;
        const auto dropActionWithSound = AudioAndPickup::EvaluateDropSound(
            &fakeUnit, AudioAndPickup::Rva::DropCallerReturn1, 0x10000, true, &table, readers, cache);
        Check(dropActionWithSound == AudioAndPickup::DropSoundAction::SuppressHidden &&
                  !customSoundCalled,
              "EvaluateDropSound suppresses custom dropSound when decision.show is false");

        Minimap::MinimapState minimap{};
        Check(!minimap.ObserveEvaluatedGroundUnit(&fakeUnit, fakeUnit, fIn, dIn, 5) &&
                  !minimap.ObserveGroundUnit(&fakeUnit, &table, readers, cache),
              "MinimapState refuses to observe or track hidden items even if hasMinimapIcon is set");

        Check(AudioAndPickup::ShouldBlockEvaluatedGroundPickup(fOut, dOut),
              "ShouldBlockEvaluatedGroundPickup blocks hidden non-gold items");
    }

    // 3b2. Verify EvaluateDropSound passes nullptr unit by default (dropSound3dAttenuation = false)
    // and passes unit when dropSound3dAttenuation = true.
    {
        static Cache::UnitEvaluationCache cache{};
        cache.Clear();
        Schema::ItemFacts fIn{};
        fIn.code = Tables::PackCode("rin");
        Schema::EvaluatedDecision dIn{};
        dIn.matched = true;
        dIn.show = true;
        dIn.hasDropSound = true;
        std::memcpy(dIn.dropSound.data(), "Filter06", 9);
        cache.Store(778, fIn.code, 311, 6, GetTickCount64(), fIn, dIn);

        alignas(16) Reader::UnitHeader fakeUnit{
            .type = Reader::UnitTypeItem,
            .classId = 311,
            .unitId = 778,
            .mode = Reader::ItemModeDropping,
            .itemData = 0x10000,
        };
        Schema::FilterRuleTable table{};
        table.generation = 6;
        table.soundRules = 1;
        Reader::NativeReaders readers{};

        static void* s_lastSoundUnit = reinterpret_cast<void*>(0xDEAD);
        static const char* s_lastSoundName = nullptr;
        const AudioAndPickup::PlayNamedSoundFn mockPlay =
            +[](const char* name, void* u, int, int) noexcept -> void* {
            s_lastSoundName = name;
            s_lastSoundUnit = u;
            return reinterpret_cast<void*>(1);
        };
        const auto mockLoaderBase =
            reinterpret_cast<std::uintptr_t>(mockPlay) - AudioAndPickup::Rva::LoaderPlayNamedSound;

        s_lastSoundUnit = reinterpret_cast<void*>(0xDEAD);
        s_lastSoundName = nullptr;
        Check(AudioAndPickup::EvaluateDropSound(
                  &fakeUnit, AudioAndPickup::Rva::DropCallerReturn1, mockLoaderBase, true,
                  &table, readers, cache, false) == AudioAndPickup::DropSoundAction::PlayedCustom &&
                  s_lastSoundUnit == nullptr && s_lastSoundName != nullptr &&
                  std::strcmp(s_lastSoundName, "Filter06") == 0,
              "EvaluateDropSound with dropSound3dAttenuation=false passes nullptr unit (full non-positional volume)");

        s_lastSoundUnit = nullptr;
        Check(AudioAndPickup::EvaluateDropSound(
                  &fakeUnit, AudioAndPickup::Rva::DropCallerReturn1, mockLoaderBase, true,
                  &table, readers, cache, true) == AudioAndPickup::DropSoundAction::PlayedCustom &&
                  s_lastSoundUnit == &fakeUnit,
              "EvaluateDropSound with dropSound3dAttenuation=true forwards unit pointer for 3D positional falloff");
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

    // 7. ShowItemsJoinSwapTracker & synthetic exeBase label placard state machine
    {
        static std::uint8_t s_states[2]{0U, 0U};
        static bool s_isController = false;
        static bool s_isLoading = false;
        static bool s_refusePanelOpen = false;
        static std::uint8_t* s_uiStateTable = nullptr;
        static std::array<std::uint8_t, 0x80> s_fakePanel{};
        static int s_setShowItemsCalls = 0;
        static int s_refreshShowItemsCalls = 0;

        constexpr std::size_t kReserveBytes = 48ULL * 1024ULL * 1024ULL;
        void* const baseMem = VirtualAlloc(nullptr, kReserveBytes, MEM_RESERVE, PAGE_NOACCESS);
        Check(baseMem != nullptr, "VirtualAlloc reserves synthetic exeBase region");
        if (baseMem) {
            const auto exeBase = reinterpret_cast<std::uintptr_t>(baseMem);
            const auto commitPage = [&](std::uintptr_t rva, DWORD prot) {
                const std::uintptr_t pageBase = (exeBase + rva) & ~static_cast<std::uintptr_t>(0xFFFU);
                Check(VirtualAlloc(reinterpret_cast<void*>(pageBase), 4096, MEM_COMMIT, prot) != nullptr,
                      "VirtualAlloc commits synthetic RVA page");
            };
            const auto writeAbsJump = [&](std::uintptr_t rva, const void* targetFn) {
                commitPage(rva, PAGE_EXECUTE_READWRITE);
                auto* const dst = reinterpret_cast<std::uint8_t*>(exeBase + rva);
                dst[0] = 0xFFU;
                dst[1] = 0x25U;
                dst[2] = 0x00U;
                dst[3] = 0x00U;
                dst[4] = 0x00U;
                dst[5] = 0x00U;
                std::memcpy(dst + 6, &targetFn, sizeof(targetFn));
            };

            commitPage(Labels::Rva::ShowItemsStatesPtr, PAGE_READWRITE);
            commitPage(Labels::Rva::NativeUiStateTable, PAGE_READWRITE);
            s_uiStateTable = reinterpret_cast<std::uint8_t*>(exeBase + Labels::Rva::NativeUiStateTable);

            auto* const statesPtrSlot =
                reinterpret_cast<std::uint8_t**>(exeBase + Labels::Rva::ShowItemsStatesPtr);
            auto* const statesCountSlot =
                reinterpret_cast<std::uintptr_t*>(exeBase + Labels::Rva::ShowItemsStatesCount);
            *statesPtrSlot = s_states;
            *statesCountSlot = 2U;

            const Labels::SetShowItemsStateFn fnSetState =
                +[](std::uint8_t idx, bool val) noexcept {
                    ++s_setShowItemsCalls;
                    if (idx < 2U) {
                        s_states[idx] = val ? 1U : 0U;
                    }
                };
            const Labels::RefreshShowItemsUiFn fnRefreshUi = +[]() noexcept {
                ++s_refreshShowItemsCalls;
                if (s_isController || s_isLoading) {
                    return;
                }
                if (s_states[0] != 0U) {
                    if (!s_refusePanelOpen) {
                        s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 1U;
                        s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 1U;
                    }
                } else {
                    s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
                    s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
                }
            };
            const Labels::IsControllerModeFn fnIsController = +[]() noexcept -> bool {
                return s_isController;
            };
            const Labels::IsLoadingScreenActiveFn fnIsLoading = +[]() noexcept -> bool {
                return s_isLoading;
            };
            const Labels::FindUiStatePanelFn fnFindPanel = +[](int idx) noexcept -> void* {
                if (idx == static_cast<int>(Labels::Rva::NativeUiItemLabelPanelIndex) &&
                    s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] != 0U) {
                    return s_fakePanel.data();
                }
                return nullptr;
            };

            writeAbsJump(Labels::Rva::SetShowItemsState, reinterpret_cast<const void*>(fnSetState));
            writeAbsJump(Labels::Rva::RefreshShowItemsUi, reinterpret_cast<const void*>(fnRefreshUi));
            writeAbsJump(Labels::Rva::IsControllerMode, reinterpret_cast<const void*>(fnIsController));
            writeAbsJump(Labels::Rva::IsLoadingScreenActive, reinterpret_cast<const void*>(fnIsLoading));
            writeAbsJump(Labels::Rva::FindUiStatePanel, reinterpret_cast<const void*>(fnFindPanel));

            // 7a. Loading screen gate & normal join settle with native post-load state+panel reset
            Labels::ShowItemsJoinSwapTracker tracker{};
            s_states[0] = 0U;
            s_states[1] = 0U;
            s_isController = false;
            s_isLoading = true;
            s_refusePanelOpen = false;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            s_setShowItemsCalls = 0;
            s_refreshShowItemsCalls = 0;

            Check(tracker.StepOnUiThread(exeBase, true, true, true, 15000ULL, false) == 0U &&
                      s_refreshShowItemsCalls == 0 && tracker.pendingJoin.load(),
                  "StepOnUiThread waits while IsLoadingScreenActive is true without starting timer");

            s_isLoading = false;
            const std::uint8_t evJoin =
                tracker.StepOnUiThread(exeBase, true, true, true, 16000ULL, false);
            Check((evJoin & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::EnabledOnJoin)) != 0U &&
                      Labels::IsItemLabelDisplayActive(exeBase) && tracker.pendingJoin.load(),
                  "StepOnUiThread enables ShowItemsPanel once loading clears and enters settle window");

            // Simulate native post-join HUD initialization resetting BOTH ShowItemsState[0] = 0
            // and closing ShowItemsPanel at +80ms while Alt is NOT held (altKeyHeld = false)
            s_states[0] = 0U;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            const std::uint8_t evReopen =
                tracker.StepOnUiThread(exeBase, true, true, true, 16080ULL, false);
            Check(Labels::IsItemLabelDisplayActive(exeBase) &&
                      Labels::ReadFilteredLabelState(exeBase) == 1 &&
                      tracker.pendingJoin.load() &&
                      (evReopen & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::EnabledOnJoin)) == 0U,
                  "StepOnUiThread immediately recovers when native post-join init resets ShowItemsState[0] and panel");

            // Still settling at +900ms after recovery (16080 + 900 = 16980 < 16080 + 1000)
            (void)tracker.StepOnUiThread(exeBase, true, true, true, 16980ULL, false);
            Check(tracker.pendingJoin.load() && Labels::IsItemLabelDisplayActive(exeBase),
                  "StepOnUiThread keeps pendingJoin active until full 1000ms settle window after recovery");

            (void)tracker.StepOnUiThread(exeBase, true, true, true, 17080ULL, false);
            Check(!tracker.pendingJoin.load() && Labels::IsItemLabelDisplayActive(exeBase),
                  "StepOnUiThread clears pendingJoin after 1000ms continuous settle window completes");

            // 7b. Manual Alt key press during settle window is respected immediately
            tracker.ResetForGameJoin();
            s_states[0] = 0U;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            (void)tracker.StepOnUiThread(exeBase, true, true, true, 20000ULL, false);
            Check(Labels::IsItemLabelDisplayActive(exeBase) && tracker.pendingJoin.load(),
                  "Join enables labels at start of settle window");

            // Player manually presses Alt (altKeyHeld = true) to toggle labels off at +100ms
            s_states[0] = 0U;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            (void)tracker.StepOnUiThread(exeBase, true, true, true, 20100ULL, true);
            Check(!tracker.pendingJoin.load() && Labels::ReadFilteredLabelState(exeBase) == 0 &&
                      !Labels::IsItemLabelDisplayActive(exeBase),
                  "Manual Alt press during settle window immediately cancels pendingJoin without re-enabling");

            // Subsequent ticks with Alt released do not re-enable
            (void)tracker.StepOnUiThread(exeBase, true, true, true, 20400ULL, false);
            Check(!tracker.pendingJoin.load() && !Labels::IsItemLabelDisplayActive(exeBase),
                  "Cancelled pendingJoin remains cleared after Alt release");

            // 7c. Bounded retry spacing (250ms) and 10s deadline when panel refuses to open
            tracker.ResetForGameJoin();
            s_states[0] = 0U;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            s_refusePanelOpen = true;
            s_refreshShowItemsCalls = 0;

            std::uint8_t timeoutEvents = 0U;
            for (std::uint64_t t = 30000ULL; t <= 40000ULL; t += 50ULL) {
                timeoutEvents |= tracker.StepOnUiThread(exeBase, true, true, true, t);
            }
            // 40 attempts across 10,000ms (2 calls to RefreshShowItemsUi per attempt = 80 calls)
            Check((timeoutEvents & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::JoinTimedOut)) != 0U &&
                      !tracker.pendingJoin.load() && s_refreshShowItemsCalls == 80,
                  "Unbounded retry guard spaces attempts by 250ms and stops with JoinTimedOut at 10s");

            const int callsAfterTimeout = s_refreshShowItemsCalls;
            (void)tracker.StepOnUiThread(exeBase, true, true, true, 41000ULL);
            Check(s_refreshShowItemsCalls == callsAfterTimeout,
                  "No further EnableShowItemsPlacards attempts occur after 10s deadline expires");

            // 7d. Controller -> Keyboard+Mouse swap reopens ShowItemsPanel even when shouldAutoShow is false
            s_refusePanelOpen = false;
            s_isController = true;
            s_states[0] = 1U;
            s_uiStateTable[Labels::Rva::NativeUiItemLabelPanelIndex] = 0U;
            s_fakePanel[Labels::Rva::OffsetWidgetIsVisible] = 0U;
            tracker.ObserveControllerMode(exeBase, true);
            Check(!tracker.pendingControllerSwap.load(),
                  "Observing controller mode records controller state without queuing swap");

            // Switch to Keyboard+Mouse while ShowItemsState[0] == 1 and ShowItemsPanel is closed
            s_isController = false;
            tracker.ObserveControllerMode(exeBase, true);
            Check(tracker.pendingControllerSwap.load() && !tracker.pendingJoin.load(),
                  "Controller -> KBM transition arms pendingControllerSwap independently of pendingJoin");

            const std::uint8_t evSwap = tracker.StepOnUiThread(exeBase, true, true, false, 50000ULL);
            Check(!tracker.pendingControllerSwap.load() &&
                      Labels::IsItemLabelDisplayActive(exeBase) &&
                      (evSwap & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::EnabledOnJoin)) == 0U,
                  "Controller -> KBM swap reopens ShowItemsPanel even when shouldAutoShow is false and does not log join");

            (void)VirtualFree(baseMem, 0, MEM_RELEASE);
        }
    }

    // 8. Extended vs standard StatListEx flags & O(log N) binary search edge cases
    {
        struct SyntheticStatEntry {
            std::uint64_t key;
            std::int32_t value;
            std::uint32_t pad;
        };
        const auto makeKey = [](std::uint32_t statId, std::uint32_t layer) noexcept {
            return (static_cast<std::uint64_t>(statId) << 32U) | layer;
        };

        const std::array<SyntheticStatEntry, 6> primaryStats{{
            {makeKey(6U, 0U), 100, 0},
            {makeKey(14U, 0U), 777, 0},
            {makeKey(70U, 0U), 3, 0},
            {makeKey(107U, 0U), 1, 0},
            {makeKey(107U, 50U), 2, 0},
            {makeKey(107U, 112U), -5, 0},
        }};
        const std::array<SyntheticStatEntry, 7> extendedStats{{
            {makeKey(0U, 0U), 15, 0},
            {makeKey(14U, 0U), 9999, 0},
            {makeKey(39U, 0U), 45, 0},
            {makeKey(107U, 0U), 4, 0},
            {makeKey(107U, 50U), 7, 0},
            {makeKey(107U, 112U), 9, 0},
            {makeKey(194U, 0U), 6, 0},
        }};

        alignas(16) std::array<std::uint8_t, 0x100> statList{};
        alignas(16) std::array<std::uint8_t, 0x100> unit{};
        const std::uint32_t ownerType = Reader::UnitTypeItem;
        const std::uint32_t ownerId = 4242U;
        std::memcpy(statList.data() + 0x08U, &ownerType, 4);
        std::memcpy(statList.data() + 0x0CU, &ownerId, 4);

        const auto setVec = [&](std::size_t off, const SyntheticStatEntry* ptr, std::uint64_t count) {
            const auto addr = reinterpret_cast<std::uintptr_t>(ptr);
            std::memcpy(statList.data() + off, &addr, 8);
            std::memcpy(statList.data() + off + 8U, &count, 8);
            std::memcpy(statList.data() + off + 16U, &count, 8);
        };
        setVec(0x30U, primaryStats.data(), primaryStats.size());
        setVec(0xA8U, extendedStats.data(), extendedStats.size());
        const auto statListPtr = reinterpret_cast<std::uintptr_t>(statList.data());
        std::memcpy(unit.data() + 0x88U, &statListPtr, 8);

        // 8a. Extended flags with extra low/high bits read +0xA8 instead of +0x30
        for (const std::uint32_t extFlags :
             {0x80000001U, 0x80000040U, 0xC0000000U, 0xA0000000U}) {
            std::memcpy(statList.data() + 0x1CU, &extFlags, 4);
            std::int32_t val = -1;
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 0U, 0U, val) && val == 15,
                  "extended StatListEx binary search finds first element");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 107U, 50U, val) && val == 7,
                  "extended StatListEx binary search finds middle multi-layer element");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 194U, 0U, val) && val == 6,
                  "extended StatListEx binary search finds last element");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 14U, 0U, val) && val == 9999,
                  "extended StatListEx reads +0xA8 rather than +0x30");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 107U, 25U, val) && val == 0,
                  "extended StatListEx returns 0 for missing middle layer");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 500U, 0U, val) && val == 0,
                  "extended StatListEx returns 0 for missing high key");
        }

        // 8b. Extended flag falls back to +0x30 when +0xA8 is empty
        {
            const std::uint32_t extFlags = 0x80000001U;
            std::memcpy(statList.data() + 0x1CU, &extFlags, 4);
            setVec(0xA8U, extendedStats.data(), 0U);
            std::int32_t val = 0;
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 14U, 0U, val) && val == 777,
                  "extended StatListEx with empty +0xA8 falls back to +0x30");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 107U, 112U, val) &&
                      val == -5,
                  "primary StatListEx preserves negative int32 stat values");
            setVec(0xA8U, extendedStats.data(), extendedStats.size());
        }

        // 8c. Standard flags (bit 31 clear) read +0x30 instead of +0xA8
        for (const std::uint32_t stdFlags : {0x00000000U, 0x00000040U}) {
            std::memcpy(statList.data() + 0x1CU, &stdFlags, 4);
            std::int32_t val = -1;
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 14U, 0U, val) && val == 777,
                  "standard StatListEx reads +0x30 rather than +0xA8");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 194U, 0U, val) && val == 0,
                  "standard StatListEx ignores entries present only in +0xA8");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1U, 0U, val) && val == 0,
                  "standard StatListEx returns 0 for missing low key");
        }

        // 8d. Single-element and empty vector edge cases
        {
            const std::uint32_t stdFlags = 0x00000000U;
            std::memcpy(statList.data() + 0x1CU, &stdFlags, 4);
            setVec(0x30U, primaryStats.data(), 0U);
            std::int32_t val = 99;
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 14U, 0U, val) && val == 0,
                  "empty StatListEx vector returns true with outValue == 0");

            const std::array<SyntheticStatEntry, 1> singleStat{{
                {makeKey(41U, 3U), 33, 0},
            }};
            setVec(0x30U, singleStat.data(), 1U);
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 41U, 3U, val) && val == 33,
                  "single-element StatListEx binary search matches exact key");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 41U, 2U, val) && val == 0,
                  "single-element StatListEx binary search misses key before");
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 41U, 4U, val) && val == 0,
                  "single-element StatListEx binary search misses key after");
        }

        // 8e. Boundary count == 512 (stack bulk copy), count == 600 (SEH binary fallback), count == 5000 (rejected)
        {
            const std::uint32_t extFlags = 0x80000001U;
            std::memcpy(statList.data() + 0x1CU, &extFlags, 4);
            std::vector<SyntheticStatEntry> largeStats(600U);
            for (std::uint32_t i = 0; i < 600U; ++i) {
                largeStats[i] = {makeKey(i * 2U, 1U), static_cast<std::int32_t>(i + 100U), 0};
            }
            setVec(0xA8U, largeStats.data(), 512U);
            std::int32_t val = -1;
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 0U, 1U, val) && val == 100 &&
                      Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1022U, 1U, val) &&
                      val == 611 &&
                      Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1021U, 1U, val) &&
                      val == 0,
                  "512-entry stack-buffered StatListEx binary search hits first/last and misses absent");

            setVec(0xA8U, largeStats.data(), 600U);
            Check(Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1198U, 1U, val) &&
                      val == 699 &&
                      Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1024U, 1U, val) &&
                      val == 612 &&
                      Reader::ReadStatFromStatListEx(unit.data(), ownerId, 1199U, 1U, val) &&
                      val == 0,
                  "600-entry (> kMaxStackStats) StatListEx falls back to bounded SEH O(log N) binary search");

            setVec(0xA8U, largeStats.data(), 5000U);
            Check(!Reader::ReadStatFromStatListEx(unit.data(), ownerId, 0U, 1U, val),
                  "oversized (> kMaxStatListEntries) StatListEx rejects corrupt count");
        }
    }

    // 9. ClampOverlayColor NaN and [0.0f, 1.0f] channel clamping
    {
        const float qnan = std::numeric_limits<float>::quiet_NaN();
        const float pinf = std::numeric_limits<float>::infinity();
        const float ninf = -std::numeric_limits<float>::infinity();

        const auto clamped = Minimap::OverlayRender::ClampOverlayColor(
            D2RL::Overlay::Color{qnan, -0.25f, 1.75f, 0.5f});
        Check(clamped.red == 0.0f && clamped.green == 0.0f && clamped.blue == 1.0f &&
                  clamped.alpha == 0.5f && !std::isnan(clamped.red),
              "ClampOverlayColor clamps NaN, negative, and >1.0 channels to [0.0f, 1.0f]");

        const auto clampedInf = Minimap::OverlayRender::ClampOverlayColor(
            D2RL::Overlay::Color{ninf, pinf, -0.0f, 255.0f});
        Check(clampedInf.red == 0.0f && !std::signbit(clampedInf.red) &&
                  clampedInf.green == 1.0f && clampedInf.blue == 0.0f &&
                  !std::signbit(clampedInf.blue) && clampedInf.alpha == 1.0f,
              "ClampOverlayColor clamps infinities and normalizes -0.0f to +0.0f");

        static int s_drawCalls = 0;
        static D2RL::Overlay::Color s_lastColor{};
        s_drawCalls = 0;
        s_lastColor = {};
        D2RL::OverlayService mockOverlay{};
        mockOverlay.drawLine = +[](const D2RL::PluginContext*, const D2RL::Overlay::LineRequest* req) noexcept {
            ++s_drawCalls;
            s_lastColor = req->color;
            return D2RL::Overlay::Result::Success;
        };
        mockOverlay.drawFilledRectangle =
            +[](const D2RL::PluginContext*, const D2RL::Overlay::FilledRectangleRequest* req) noexcept {
            ++s_drawCalls;
            s_lastColor = req->color;
            return D2RL::Overlay::Result::Success;
        };
        mockOverlay.drawRectangle =
            +[](const D2RL::PluginContext*, const D2RL::Overlay::RectangleRequest* req) noexcept {
            ++s_drawCalls;
            s_lastColor = req->color;
            return D2RL::Overlay::Result::Success;
        };

        Minimap::ScreenMarker badMarker{
            .x = 200.0f,
            .y = 200.0f,
            .unitId = 1U,
            .shape = Schema::MinimapShape::Square,
            .borderColor = {qnan, -2.0f, 3.0f, 1.5f},
            .fillColor = {0.25f, 1.2f, -0.1f, 0.8f},
            .sizePx = 16.0f,
        };
        Minimap::OverlayRender::DrawMarker(nullptr, &mockOverlay, {}, badMarker);
        Check(s_drawCalls == 2 && s_lastColor.red == 0.0f && s_lastColor.green == 0.0f &&
                  s_lastColor.blue == 1.0f && s_lastColor.alpha == 1.0f,
              "DrawMarker clamps Square border/fill colors before OverlayService dispatch");

        Minimap::OverlayRender::DrawLine(
            nullptr, &mockOverlay, {}, 0.0f, 0.0f, 10.0f, 10.0f,
            D2RL::Overlay::Color{qnan, -5.0f, 5.0f, 2.0f}, 2.0f);
        Check(s_drawCalls == 3 && s_lastColor.red == 0.0f && s_lastColor.green == 0.0f &&
                  s_lastColor.blue == 1.0f && s_lastColor.alpha == 1.0f,
              "DrawLine clamps direct Overlay::Color inputs before OverlayService::drawLine");
    }

    // 10. Build admission ("3.3"/"3.4" boundary check) & Steam D2RCore 1.3.1-beta PE/code guard
    {
        Check(IsSupportedBuildVersion("3.3"), "admits '3.3'");
        Check(IsSupportedBuildVersion("3.3.93847"), "admits '3.3.93847'");
        Check(IsSupportedBuildVersion("3.3.94995"), "admits '3.3.94995'");
        Check(IsSupportedBuildVersion("3.4"), "admits '3.4'");
        Check(IsSupportedBuildVersion("3.4.95032"), "admits '3.4.95032'");
        Check(!IsSupportedBuildVersion(nullptr), "rejects nullptr buildVersion");
        Check(!IsSupportedBuildVersion(""), "rejects empty buildVersion");
        Check(!IsSupportedBuildVersion("3"), "rejects truncated '3'");
        Check(!IsSupportedBuildVersion("3."), "rejects '3.'");
        Check(!IsSupportedBuildVersion("3.30"), "rejects '3.30' boundary");
        Check(!IsSupportedBuildVersion("3.30.94995"), "rejects '3.30.94995' boundary");
        Check(!IsSupportedBuildVersion("3.40"), "rejects '3.40' boundary");
        Check(!IsSupportedBuildVersion("3.3-beta"), "rejects '3.3-beta' boundary");
        Check(!IsSupportedBuildVersion("3.2.92777"), "rejects older '3.2.92777'");
        Check(!IsSupportedBuildVersion("3.5.0"), "rejects newer '3.5.0'");

        void* const coreMem = VirtualAlloc(
            nullptr, D2RCore131Beta::SizeOfImage, MEM_RESERVE, PAGE_NOACCESS);
        Check(coreMem != nullptr, "VirtualAlloc reserves synthetic D2RCore image");
        if (coreMem) {
            const auto coreBase = reinterpret_cast<std::uintptr_t>(coreMem);
            const auto commitRvaPage = [&](std::uintptr_t rva) {
                const std::uintptr_t pageBase =
                    (coreBase + rva) & ~static_cast<std::uintptr_t>(0xFFFU);
                Check(VirtualAlloc(reinterpret_cast<void*>(pageBase), 4096U, MEM_COMMIT,
                                   PAGE_READWRITE) != nullptr,
                      "VirtualAlloc commits synthetic D2RCore page");
            };
            commitRvaPage(0U);
            commitRvaPage(D2RCore131Beta::CompilerManifestCheckRva);
            commitRvaPage(D2RCore131Beta::VersionBlockRva);

            auto* const dos = reinterpret_cast<IMAGE_DOS_HEADER*>(coreBase);
            dos->e_magic = IMAGE_DOS_SIGNATURE;
            dos->e_lfanew = 0x100L;
            auto* const nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(coreBase + 0x100U);
            nt->Signature = IMAGE_NT_SIGNATURE;
            nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
            nt->FileHeader.TimeDateStamp = D2RCore131Beta::TimeDateStamp;
            nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
            nt->OptionalHeader.SizeOfImage = D2RCore131Beta::SizeOfImage;

            std::memcpy(reinterpret_cast<void*>(coreBase + D2RCore131Beta::CompilerManifestCheckRva),
                        D2RCore131Beta::CompilerManifestCheckBytes.data(),
                        D2RCore131Beta::CompilerManifestCheckBytes.size());

            const auto resetVersionBlock = [&](const char* verStr) {
                const auto block = coreBase + D2RCore131Beta::VersionBlockRva;
                std::memset(reinterpret_cast<void*>(block), 0, D2RCore131Beta::VersionBlockBytes);
                const std::uint64_t verLen = std::strlen(verStr);
                const std::uint64_t narrowCap = D2RCore131Beta::NarrowSsoCapacity;
                const std::uint64_t wideCap = D2RCore131Beta::WideSsoCapacity;
                std::memcpy(reinterpret_cast<void*>(block + D2RCore131Beta::NarrowVerBufOffset),
                            verStr, verLen + 1U);
                std::memcpy(reinterpret_cast<void*>(block + D2RCore131Beta::NarrowVerLenOffset),
                            &verLen, sizeof(verLen));
                std::memcpy(reinterpret_cast<void*>(block + D2RCore131Beta::NarrowVerCapOffset),
                            &narrowCap, sizeof(narrowCap));
                std::memcpy(reinterpret_cast<void*>(block + D2RCore131Beta::WideBuildCapOffset),
                            &wideCap, sizeof(wideCap));
                std::memcpy(reinterpret_cast<void*>(block + D2RCore131Beta::NarrowBuildCapOffset),
                            &narrowCap, sizeof(narrowCap));
            };

            // Wrong SizeOfImage refuses without writing .data
            resetVersionBlock("3.3.94995");
            nt->OptionalHeader.SizeOfImage = 0xB9D000U;
            Check(!TryApplySteamGameBuildHotfix(coreBase) &&
                      *reinterpret_cast<const std::uint64_t*>(
                          coreBase + D2RCore131Beta::VersionBlockRva +
                          D2RCore131Beta::NarrowBuildLenOffset) == 0ULL,
                  "Steam hotfix refuses mismatched SizeOfImage without modifying .data");
            nt->OptionalHeader.SizeOfImage = D2RCore131Beta::SizeOfImage;

            // Wrong TimeDateStamp refuses without writing .data
            nt->FileHeader.TimeDateStamp = 0x12345678U;
            Check(!TryApplySteamGameBuildHotfix(coreBase),
                  "Steam hotfix refuses mismatched TimeDateStamp");
            nt->FileHeader.TimeDateStamp = D2RCore131Beta::TimeDateStamp;

            // Wrong D2RCore+0x75270 byte witness refuses without writing .data
            auto* const witnessPtr = reinterpret_cast<std::uint8_t*>(
                coreBase + D2RCore131Beta::CompilerManifestCheckRva);
            witnessPtr[5] ^= 0xFFU;
            Check(!TryApplySteamGameBuildHotfix(coreBase),
                  "Steam hotfix refuses mismatched D2RCore+0x75270 code bytes");
            witnessPtr[5] = D2RCore131Beta::CompilerManifestCheckBytes[5];

            // Matching D2RCore 1.3.1-beta PE + code witness populates Steam 3.3.94995 gameBuild
            SteamHotfixResult report{};
            Check(TryApplySteamGameBuildHotfix(coreBase, &report) &&
                      std::strcmp(report.gameBuild.data(), "94995") == 0 &&
                      std::strcmp(reinterpret_cast<const char*>(
                                      coreBase + D2RCore131Beta::VersionBlockRva +
                                      D2RCore131Beta::NarrowBuildBufOffset),
                                  "94995") == 0 &&
                      std::wcscmp(reinterpret_cast<const wchar_t*>(
                                      coreBase + D2RCore131Beta::VersionBlockRva +
                                      D2RCore131Beta::WideBuildBufOffset),
                                  L"94995") == 0 &&
                      *reinterpret_cast<const std::uint64_t*>(
                          coreBase + D2RCore131Beta::VersionBlockRva +
                          D2RCore131Beta::NarrowBuildLenOffset) == 5ULL,
                  "Steam hotfix populates 3.3.94995 gameBuild when PE and +0x75270 witnesses match");

            // Already-populated gameBuild is left untouched
            Check(!TryApplySteamGameBuildHotfix(coreBase),
                  "Steam hotfix is idempotent when gameBuild is already populated");

            // Matching 3.4.95041 populates "95041"
            resetVersionBlock("3.4.95041");
            Check(TryApplySteamGameBuildHotfix(coreBase, &report) &&
                      std::strcmp(report.gameBuild.data(), "95041") == 0,
                  "Steam hotfix also populates 3.4.95041 gameBuild on matching D2RCore 1.3.1-beta");

            (void)VirtualFree(coreMem, 0, MEM_RELEASE);
        }
    }

    std::puts("pyre-lootfilter cache, StatListEx reader, label, pickup, minimap, and admission tests pass.");
    return 0;
}
