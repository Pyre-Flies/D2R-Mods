#include "filter_schema.h"
#include "profile_dropdown.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* name) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}

void WriteTextFile(const std::filesystem::path& path, std::string_view content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}
} // namespace

int main() {
    using namespace PyreLootFilter;

    // 1. JSON parser line tracking & RFC 8259
    {
        Json::Value val;
        std::size_t line = 0;
        std::string err;
        Check(!Json::Parse("{\n  \"version\": 3,\n  \"rules\": [\n    bad\n  ]\n}", val, &line,
                           &err) &&
                  line == 4,
              "JSON parser reports exact 1-based line on syntax error");
    }

    // 2. Build a synthetic mod workspace with excel tables and sfx
    const auto tempRoot = std::filesystem::temp_directory_path() / L"pyre_lootfilter_schema_test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);

    const auto configDir = tempRoot / L"d2rloader" / L"config";
    const auto excelDir = tempRoot / L"data" / L"global" / L"excel";
    const auto sfxDir = tempRoot / L"data" / L"hd" / L"global" / L"sfx" / L"item";
    std::filesystem::create_directories(configDir);
    std::filesystem::create_directories(excelDir);
    std::filesystem::create_directories(sfxDir);

    WriteTextFile(
        excelDir / L"itemtypes.txt",
        "ItemType\tCode\tEquiv1\tEquiv2\r\n"
        "Any\t\t\t\r\n"
        "Misc\tmisc\t\t\r\n"
        "Gold\tgold\tmisc\t\r\n"
        "Quest\tques\tmisc\t\r\n"
        "Rune\trune\tmisc\t\r\n"
        "Scroll\tscro\tmisc\t\r\n"
        "Weapon\tweap\t\t\r\n"
        "Sword\tswor\tweap\t\r\n"
        "Mace\tmace\tweap\t\r\n"
        "Shield\tshie\t\t\r\n"
        "Unused Category\tunus\t\t\r\n");

    WriteTextFile(
        excelDir / L"weapons.txt",
        "name\tcode\tnormcode\tubercode\tultracode\ttype\ttype2\tquest\r\n"
        "Crystal Sword\tcrs\tcrs\t9cr\t7cr\tswor\t\t0\r\n"
        "Dimensional Blade\t9cr\tcrs\t9cr\t7cr\tswor\t\t0\r\n"
        "Phase Blade\t7cr\tcrs\t9cr\t7cr\tswor\t\t0\r\n"
        "Khalim's Flail\tqf1\tqf1\t\t\tmace\t\t1\r\n");

    WriteTextFile(
        excelDir / L"armor.txt",
        "name\tcode\tnormcode\tubercode\tultracode\ttype\ttype2\tquest\r\n"
        "Kite Shield\tkit\tkit\txit\tuit\tshie\t\t0\r\n"
        "Dragon Shield\txit\tkit\txit\tuit\tshie\t\t0\r\n"
        "Monarch\tuit\tkit\txit\tuit\tshie\t\t0\r\n");

    WriteTextFile(
        excelDir / L"misc.txt",
        "name\tcode\ttype\ttype2\tquest\r\n"
        "Gold\tgld\tgold\t\t0\r\n"
        "Ber Rune\tr30\trune\t\t0\r\n"
        "Scroll of Inifuss\tbks\tscro\t\t4\r\n"
        "Mephisto's Soulstone\tmss\tques\t\t21\r\n");

    // Create Filter01.flac, leave Filter02.flac missing to test fallback warning
    WriteTextFile(sfxDir / L"Filter01.flac", "fLaC");

    // 3. Test Schema v3 with baseName, itemType (including Quest items & unknown itemType warning),
    //    continue:true, tooltip colors, minimapIcon, and dropSound
    const auto filterPath = configDir / L"filter.json";
    WriteTextFile(
        filterPath,
        R"json({
  "version": 3,
  "rules": [
    {
      "hide": {
        "ruleName": "Hide small gold piles",
        "conditions": {
          "itemType": "Gold",
          "quantity": { "lt": 5000 }
        }
      }
    },
    {
      "show": {
        "ruleName": "Quest items always visible",
        "conditions": {
          "itemType": "Quest"
        },
        "tooltip": {
          "textColor": "RGBA(255, 215, 0, 1.0)"
        }
      }
    },
    {
      "show": {
        "ruleName": "Base highlight for Swords",
        "conditions": {
          "itemType": "Sword"
        },
        "continue": true,
        "tooltip": {
          "backgroundColor": "RGBA(20, 40, 80, 0.85)"
        }
      }
    },
    {
      "show": {
        "ruleName": "4os Ethereal Phase Blade",
        "conditions": {
          "baseName": "Phase Blade",
          "sockets": { "eq": 4 },
          "ethereal": true
        },
        "name": "Eth 4os Phase Blade",
        "dropSound": "Filter01",
        "minimapIcon": {
          "shape": "star",
          "borderColor": "RGBA(255, 255, 255, 1.0)",
          "fillColor": "RGBA(255, 128, 0, 0.9)",
          "size": 16
        }
      }
    },
    {
      "show": {
        "ruleName": "Missing sound asset fallback",
        "conditions": {
          "code": "r30"
        },
        "dropSound": "Filter02"
      }
    },
    {
      "hide": {
        "ruleName": "Unknown itemType should warn and disable only this rule",
        "conditions": {
          "itemType": ["NonExistentModType", "Unused Category"]
        }
      }
    }
  ]
})json");

    std::vector<std::string> warnings;
    const auto table = Schema::LoadFilterRuleTable(
        filterPath, 42, [&](const char* msg) { warnings.emplace_back(msg); });
    Check(table != nullptr, "Schema v3 filter.json loads even with unknown/empty itemType and missing .flac");
    Check(table->generation == 42 && table->schema == 3, "table generation and schema v3");
    Check(table->rules.size() == 6, "all 6 rules parsed");
    Check(!warnings.empty(), "warnings logged for unknown itemType and missing Filter02.flac");

    // Verify small gold pile is hidden
    {
        Schema::ItemFacts gold{};
        gold.code = Tables::PackCode("gld");
        gold.quantityKnown = true;
        gold.quantity = 1200;
        Schema::EvaluatedDecision dec{};
        Check(Schema::ResolveDecision(table.get(), gold, dec) && !dec.show,
              "small gold pile (<5000) resolves to hide");

        gold.quantity = 8500;
        Check(!Schema::ResolveDecision(table.get(), gold, dec) && dec.show,
              "large gold pile (>=5000) remains visible");
    }

    // Verify quest items (both explicit type 'ques' and quest > 0 column in weapons/misc) match "Quest"
    for (std::string_view qCode : {"mss", "bks", "qf1"}) {
        Schema::ItemFacts questItem{};
        questItem.code = Tables::PackCode(qCode);
        Schema::EvaluatedDecision dec{};
        Check(Schema::ResolveDecision(table.get(), questItem, dec) && dec.show && dec.hasTextColor,
              "quest item matches itemType 'Quest'");
    }

    // Verify continue:true merges backgroundColor from Sword rule with name/dropSound/minimapIcon from Phase Blade rule
    {
        Schema::ItemFacts pb{};
        pb.code = Tables::PackCode("7cr");
        pb.socketsKnown = true;
        pb.sockets = 4;
        pb.etherealKnown = true;
        pb.ethereal = true;
        Schema::EvaluatedDecision dec{};
        Check(Schema::ResolveDecision(table.get(), pb, dec) && dec.show &&
                  dec.hasBackground && dec.hasName &&
                  std::string_view(dec.name.data()) == "Eth 4os Phase Blade" &&
                  dec.hasDropSound && std::string_view(dec.dropSound.data()) == "Filter01" &&
                  dec.hasMinimapIcon && dec.minimapShape == Schema::MinimapShape::Star &&
                  dec.minimapSizePx == 16.0f,
              "continue:true rule composition merges background, name, sound, and minimapIcon");
    }

    // Verify missing Filter02.flac fell back cleanly (hasDropSound == false)
    {
        Schema::ItemFacts ber{};
        ber.code = Tables::PackCode("r30");
        Schema::EvaluatedDecision dec{};
        Check(Schema::ResolveDecision(table.get(), ber, dec) && dec.show && !dec.hasDropSound,
              "missing Filter02.flac falls back to native drop sound without rejecting rule");
    }

    // Verify CanEverHideCode
    Check(Schema::CanEverHideCode(table.get(), Tables::PackCode("gld")),
          "gld can be hidden by rule 1");
    Check(!Schema::CanEverHideCode(table.get(), Tables::PackCode("r30")),
          "r30 cannot be hidden by any active rule (disabled empty-itemType hide rule ignored)");

    // 4. Test LiveReloadWatcher debounce
    {
        Schema::LiveReloadWatcher watcher{};
        const auto s1 = Schema::ReadFilterFileStamp(filterPath);
        watcher.Resync(s1);
        Check(!watcher.Observe(s1, 1000, 500), "unchanged stamp does not trigger reload");

        Schema::FileStamp s2 = s1;
        s2.size += 10;
        Check(!watcher.Observe(s2, 1100, 500), "first observation starts quiet window");
        Check(!watcher.Observe(s2, 1400, 500), "before quiet window elapses");
        Check(watcher.Observe(s2, 1650, 500), "after quiet window elapses, reload triggers once");
        Check(!watcher.Observe(s2, 2000, 500), "subsequent poll does not re-trigger");
    }

    // 5. Category A (Standard/Identified & White/Grey bases) + Category B (xrayMode) Stat Filtering
    {
        WriteTextFile(
            filterPath,
            R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "GG Paladin Shield Base (40+ All Res, 15% ED)",
        "conditions": {
          "baseName": "Monarch",
          "rarity": ["normal", "superior"],
          "stats": {
            "allResist": { "gte": 40 },
            "enhancedDefense": { "gte": 15 }
          }
        },
        "name": "GG 45@ 15ED Base"
      }
    },
    {
      "show": {
        "ruleName": "Dual-Trap Staffmod Claw (+3 LS, +2 DS)",
        "conditions": {
          "code": "7cr",
          "rarity": ["normal", "superior"],
          "stats": [
            { "stat": "singleSkill", "skill": "Lightning Sentry", "gte": 3 },
            { "stat": "singleSkill", "skill": ["Death Sentry", "Mind Blast"], "gte": 2 }
          ]
        },
        "name": "GG Trap Base"
      }
    },
    {
      "show": {
        "ruleName": "GG Caster Rare (10 FCR, 15+ All Res, 30+ Life)",
        "conditions": {
          "code": "uit",
          "rarity": "rare",
          "stats": {
            "fasterCastRate": { "gte": 10 },
            "allResist": { "gte": 15 },
            "life": { "gte": 30 }
          }
        },
        "name": "GG Caster Rare"
      }
    },
    {
      "show": {
        "ruleName": "Fallback Rare Item",
        "conditions": {
          "code": "uit",
          "rarity": "rare"
        },
        "name": "Ordinary Rare"
      }
    }
  ]
})json");

        const auto statTable = Schema::LoadFilterRuleTable(filterPath, 99, nullptr);
        Check(statTable != nullptr && statTable->usesStats && statTable->statRules == 3 &&
                  !statTable->xrayMode,
              "stat rules load with xrayMode defaulting to false");

        // Category A.1: Superior white/grey base (quality=3, identified=false) with 45 all res and 15% ED
        {
            Schema::ItemFacts shield{};
            shield.code = Tables::PackCode("uit");
            shield.qualityKnown = true;
            shield.quality = 3U; // superior (never drops unidentified)
            shield.identifiedKnown = true;
            shield.identified = false;
            shield.statsKnown = true;
            (void)shield.SetOrAddStat(39, 0, 45);
            (void)shield.SetOrAddStat(41, 0, 45);
            (void)shield.SetOrAddStat(43, 0, 45);
            (void)shield.SetOrAddStat(45, 0, 45);
            (void)shield.SetOrAddStat(16, 0, 15);

            Schema::EvaluatedDecision dec{};
            Check(Schema::ResolveDecision(statTable.get(), shield, dec) && dec.hasName &&
                      std::string_view(dec.name.data()) == "GG 45@ 15ED Base",
                  "Category A matches Superior base with 45 allResist and 15% ED");

            // Drop cold resist to 35 -> min resist is 35 (< 40) -> no match
            (void)shield.SetOrAddStat(43, 0, 35, true);
            Check(!Schema::ResolveDecision(statTable.get(), shield, dec),
                  "Category A rejects Superior base when one elemental resist is below 40");
        }

        // Category A.2: Multi-staffmod array syntax (+3 Lightning Sentry [271], +2 Death Sentry [276])
        {
            Schema::ItemFacts baseWeapon{};
            baseWeapon.code = Tables::PackCode("7cr");
            baseWeapon.qualityKnown = true;
            baseWeapon.quality = 2U; // normal
            baseWeapon.identifiedKnown = true;
            baseWeapon.identified = false;
            baseWeapon.statsKnown = true;
            (void)baseWeapon.SetOrAddStat(107, 271, 3); // +3 Lightning Sentry
            (void)baseWeapon.SetOrAddStat(107, 276, 2); // +2 Death Sentry

            Schema::EvaluatedDecision dec{};
            Check(Schema::ResolveDecision(statTable.get(), baseWeapon, dec) && dec.hasName &&
                      std::string_view(dec.name.data()) == "GG Trap Base",
                  "Category A matches multi-staffmod array condition (+3 LS, +2 DS)");
        }

        // Category A.3 vs Category B: Rare item (quality=6) when unidentified vs identified vs xrayMode
        {
            Schema::ItemFacts rareItem{};
            rareItem.code = Tables::PackCode("uit");
            rareItem.qualityKnown = true;
            rareItem.quality = 6U; // rare
            rareItem.identifiedKnown = true;
            rareItem.identified = false; // unidentified on ground!
            rareItem.statsKnown = true;
            (void)rareItem.SetOrAddStat(105, 0, 10); // 10 FCR
            (void)rareItem.SetOrAddStat(39, 0, 20);
            (void)rareItem.SetOrAddStat(41, 0, 20);
            (void)rareItem.SetOrAddStat(43, 0, 20);
            (void)rareItem.SetOrAddStat(45, 0, 20);
            (void)rareItem.SetOrAddStat(7, 0, 40); // +40 Life

            Schema::EvaluatedDecision dec{};
            // With xrayMode == false (default), unidentified Rare MUST skip the stat rule and
            // fall through to "Fallback Rare Item" ("Ordinary Rare")!
            Check(Schema::ResolveDecision(statTable.get(), rareItem, dec) && dec.hasName &&
                      std::string_view(dec.name.data()) == "Ordinary Rare",
                  "Category A skips stat rule on unidentified Rare and falls through to general Rare rule");

            // Once identified (identified = true), Category A matches "GG Caster Rare"!
            rareItem.identified = true;
            Check(Schema::ResolveDecision(statTable.get(), rareItem, dec) && dec.hasName &&
                      std::string_view(dec.name.data()) == "GG Caster Rare",
                  "Category A matches stat rule when Rare item is identified");

            // Category B: With hidden xrayMode = true, even unidentified (identified = false) Rare
            // matches "GG Caster Rare" on the ground!
            rareItem.identified = false;
            statTable->xrayMode = true;
            Check(Schema::ResolveDecision(statTable.get(), rareItem, dec) && dec.hasName &&
                      std::string_view(dec.name.data()) == "GG Caster Rare",
                  "Category B (xrayMode=true) matches stat rule on unidentified Rare item");
        }

        // Category C: Named colors, Hex colors (#RRGGBB / #RRGGBBAA), lowercase rgba(), and maxLife/maxMp aliases
        {
            std::array<float, 4> c{};
            Check(Schema::ParseFilterRgba("Green", c) && c[1] > 0.8f && c[3] == 1.0f,
                  "ParseFilterRgba accepts named color 'Green'");
            Check(Schema::ParseFilterRgba("gold", c) && c[0] > 0.8f && c[3] == 1.0f,
                  "ParseFilterRgba accepts case-insensitive named color 'gold'");
            Check(Schema::ParseFilterRgba("#FF8000", c) && c[0] > 0.99f && c[1] > 0.49f &&
                      c[2] == 0.0f && c[3] == 1.0f,
                  "ParseFilterRgba accepts #RRGGBB hex color");
            Check(Schema::ParseFilterRgba("#00FF0080", c) && c[0] == 0.0f && c[1] > 0.99f &&
                      c[3] > 0.49f && c[3] < 0.51f,
                  "ParseFilterRgba accepts #RRGGBBAA hex color with alpha");
            Check(Schema::ParseFilterRgba("rgba(10, 20, 30, 0.5)", c) && c[3] == 0.5f,
                  "ParseFilterRgba accepts lowercase rgba(...)");

            Tables::StatCostCatalog cat{};
            Tables::StatKind kind{};
            std::uint16_t sid{};
            Check(cat.Resolve("maxLife", kind, sid) && sid == 7U,
                  "StatCostCatalog resolves 'maxLife' alias to stat 7 (maxhp)");
            Check(cat.Resolve("maxMp", kind, sid) && sid == 9U,
                  "StatCostCatalog resolves 'maxMp' alias to stat 9 (maxmana)");
            Check(cat.Resolve("gold", kind, sid) && sid == 14U,
                  "StatCostCatalog resolves 'gold' alias to stat 14 (STAT_GOLD)");
        }

        // Category D: Gold filtering with large quantities (> 65535), case-insensitive itemType, and stats
        {
            WriteTextFile(
                filterPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "Jackpot Gold Pile (100k+)",
        "conditions": {
          "itemType": "GOLD",
          "quantity": { "gte": 100000 }
        },
        "name": "** {name} **",
        "tooltip": {
          "backgroundColor": "#1A1408DC",
          "textColor": "gold",
          "borderColor": "#FFD700FF"
        },
        "minimapIcon": {
          "shape": "square",
          "borderColor": "white",
          "fillColor": "gold",
          "size": 64
        }
      }
    },
    {
      "show": {
        "ruleName": "Medium Gold via stats.gold",
        "conditions": {
          "code": "gld",
          "stats": {
            "gold": { "gte": 10000 }
          }
        },
        "name": "{name}",
        "tooltip": {
          "borderColor": "yellow"
        },
        "minimapIcon": {
          "shape": "circle",
          "borderColor": "white",
          "fillColor": "yellow",
          "size": 250
        }
      }
    },
    {
      "hide": {
        "ruleName": "Hide small gold",
        "conditions": {
          "code": "gld",
          "quantity": { "lt": 10000 }
        }
      }
    }
  ]
})json");

            const auto goldTable = Schema::LoadFilterRuleTable(filterPath, 100, nullptr);
            Check(goldTable != nullptr && goldTable->rules.size() == 3 &&
                      goldTable->borderColorRules == 2,
                  "Gold filter rules with quantity >= 100000, case-insensitive itemType GOLD, stats.gold, tooltip.borderColor, and square/circle shapes load");

            Schema::ItemFacts bigGold{};
            bigGold.code = Tables::GoldItemCode;
            bigGold.quantityKnown = true;
            bigGold.quantity = 125000;
            Schema::EvaluatedDecision dec{};
            Check(Schema::ResolveDecision(goldTable.get(), bigGold, dec) && dec.show &&
                      dec.hasName && std::string_view(dec.name.data()) == "** {name} **" &&
                      dec.hasBackground && dec.hasTextColor && dec.hasBorderColor &&
                      dec.hasMinimapIcon && dec.minimapShape == Schema::MinimapShape::Square &&
                      dec.minimapSizePx == 64.0f,
                  "125000 Gold pile matches quantity >= 100000 rule with tooltip borderColor and 64px square minimapIcon");

            Schema::ItemFacts medGold{};
            medGold.code = Tables::GoldItemCode;
            medGold.quantityKnown = true;
            medGold.quantity = 25000;
            medGold.statsKnown = true;
            (void)medGold.SetOrAddStat(14, 0, 25000);
            Check(Schema::ResolveDecision(goldTable.get(), medGold, dec) && dec.show &&
                      dec.hasName && std::string_view(dec.name.data()) == "{name}" &&
                      !dec.hasBackground && !dec.hasTextColor && dec.hasBorderColor &&
                      dec.hasMinimapIcon && dec.minimapShape == Schema::MinimapShape::Circle &&
                      dec.minimapSizePx == Schema::MaxMinimapSizePx,
                  "25000 Gold pile matches stats.gold >= 10000 with border-only tooltip, circle minimapIcon, and size 250 -> 128 clamp");

            WriteTextFile(
                filterPath,
                R"json({
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "wa7" }, "minimapIcon": { "shape": "triangleDown", "borderColor": "white", "fillColor": "red" } } },
    { "show": { "conditions": { "code": "ulg" }, "minimapIcon": { "shape": "spark", "borderColor": "white", "fillColor": "cyan" } } },
    { "show": { "conditions": { "code": "ukp" }, "minimapIcon": { "shape": "kite", "borderColor": "white", "fillColor": "purple" } } },
    { "show": { "conditions": { "code": "9pi" }, "minimapIcon": { "shape": "hexagon", "borderColor": "white", "fillColor": "gold" } } },
    { "show": { "conditions": { "code": "amu" }, "minimapIcon": { "shape": "cross", "borderColor": "white", "fillColor": "green" } } },
    { "show": { "conditions": { "code": "hbl" }, "minimapIcon": { "shape": "chevron", "borderColor": "white", "fillColor": "blue" } } },
    { "show": { "conditions": { "code": "umg" }, "minimapIcon": { "shape": "bullseye", "borderColor": "white", "fillColor": "teal" } } }
  ]
})json");
            const auto shapesTable = Schema::LoadFilterRuleTable(filterPath, 101, nullptr);
            Check(shapesTable != nullptr && shapesTable->rules.size() == 7 &&
                      shapesTable->rules[0].minimapShape == Schema::MinimapShape::TriangleDown &&
                      shapesTable->rules[1].minimapShape == Schema::MinimapShape::Spark &&
                      shapesTable->rules[2].minimapShape == Schema::MinimapShape::Kite &&
                      shapesTable->rules[3].minimapShape == Schema::MinimapShape::Hexagon &&
                      shapesTable->rules[4].minimapShape == Schema::MinimapShape::Cross &&
                      shapesTable->rules[5].minimapShape == Schema::MinimapShape::Chevron &&
                      shapesTable->rules[6].minimapShape == Schema::MinimapShape::Bullseye,
                  "All extended minimapIcon shapes parse and map to their enum values");

            WriteTextFile(
                filterPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "Elite bases",
        "conditions": {
          "tier": "elite"
        },
        "name": "[Elite] {name}"
      }
    },
    {
      "hide": {
        "ruleName": "Hide Normal and Exceptional bases",
        "conditions": {
          "tier": ["normal", "exceptional"]
        }
      }
    }
  ]
})json");
            const auto tierTable = Schema::LoadFilterRuleTable(filterPath, 102, nullptr);
            Check(tierTable != nullptr && tierTable->rules.size() == 2,
                  "tier condition ('elite' and ['normal', 'exceptional']) loads from weapons.txt and armor.txt");

            Schema::ItemFacts eliteSword{};
            eliteSword.code = Tables::PackCode("7cr");
            Check(Schema::ResolveDecision(tierTable.get(), eliteSword, dec) && dec.show &&
                      dec.hasName && std::string_view(dec.name.data()) == "[Elite] {name}",
                  "Phase Blade (7cr) matches tier: 'elite'");

            Schema::ItemFacts excShield{};
            excShield.code = Tables::PackCode("xit");
            Check(Schema::ResolveDecision(tierTable.get(), excShield, dec) && !dec.show,
                  "Dragon Shield (xit) matches tier: ['normal', 'exceptional'] hide rule");

            Schema::ItemFacts normSword{};
            normSword.code = Tables::PackCode("crs");
            Check(Schema::ResolveDecision(tierTable.get(), normSword, dec) && !dec.show,
                  "Crystal Sword (crs) matches tier: ['normal', 'exceptional'] hide rule");

            Schema::ItemFacts runeItem{};
            runeItem.code = Tables::PackCode("r30");
            Check(!Schema::ResolveDecision(tierTable.get(), runeItem, dec),
                  "Non-equipment misc item (Ber Rune r30) does not match weapon/armor tier rules");

            // Category E: Hide rules with preserved Show actions (dropSound / minimapIcon) or
            // continue:true chains ending in Hide must never activate dropSound or minimapIcon.
            WriteTextFile(
                filterPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "hide": {
        "ruleName": "Toggled-to-Hide rule retaining Show actions from builder",
        "conditions": {
          "code": "crs"
        },
        "dropSound": "Filter01",
        "minimapIcon": {
          "shape": "star",
          "borderColor": "RGBA(255, 255, 255, 1)",
          "fillColor": "RGBA(210, 120, 255, 1)",
          "size": 24
        }
      }
    },
    {
      "show": {
        "ruleName": "Continue highlight before later Hide rule",
        "conditions": {
          "itemType": "Sword"
        },
        "continue": true,
        "dropSound": "Filter01",
        "minimapIcon": {
          "shape": "diamond",
          "borderColor": "white",
          "fillColor": "gold"
        }
      }
    },
    {
      "hide": {
        "ruleName": "Hide Dimensional Blade after continue highlight",
        "conditions": {
          "code": "9cr"
        }
      }
    },
    {
      "hide": {
        "ruleName": "Hide Shield with continue and preserved actions",
        "conditions": {
          "itemType": "Shield"
        },
        "continue": true,
        "dropSound": "Filter01",
        "minimapIcon": {
          "shape": "circle",
          "borderColor": "white",
          "fillColor": "red"
        }
      }
    },
    {
      "show": {
        "ruleName": "Re-show Monarch without actions",
        "conditions": {
          "code": "uit"
        }
      }
    }
  ]
})json");
            const auto hideActionTable = Schema::LoadFilterRuleTable(filterPath, 103, nullptr);
            Check(hideActionTable != nullptr && hideActionTable->rules.size() == 5 &&
                      hideActionTable->hiddenRules == 3 &&
                      hideActionTable->soundRules == 1 &&
                      hideActionTable->minimapIconRules == 1,
                  "Hide rules with preserved dropSound/minimapIcon do not increment active soundRules or minimapIconRules");

            Schema::ItemFacts hiddenCrs{};
            hiddenCrs.code = Tables::PackCode("crs");
            Check(Schema::ResolveDecision(hideActionTable.get(), hiddenCrs, dec) &&
                      !dec.show && !dec.hasDropSound && !dec.hasMinimapIcon,
                  "Hide rule with preserved dropSound and minimapIcon resolves with neither sound nor minimapIcon");

            Schema::ItemFacts hidden9cr{};
            hidden9cr.code = Tables::PackCode("9cr");
            Check(Schema::ResolveDecision(hideActionTable.get(), hidden9cr, dec) &&
                      !dec.show && !dec.hasDropSound && !dec.hasMinimapIcon,
                  "Later Hide rule clears dropSound and minimapIcon inherited from earlier continue:true Show rule");

            Schema::ItemFacts reshownMonarch{};
            reshownMonarch.code = Tables::PackCode("uit");
            Check(Schema::ResolveDecision(hideActionTable.get(), reshownMonarch, dec) &&
                      dec.show && !dec.hasDropSound && !dec.hasMinimapIcon,
                  "Earlier continue:true Hide rule does not leak preserved dropSound or minimapIcon into later Show rule");
        }

        // Category F: Global vs Mod folder placement for both pyre-lootfilter.dll and filter.json.
        {
            const auto gameRoot = tempRoot / L"game_install";
            const auto globalLoader = gameRoot / L"d2rloader";
            const auto globalConfig = globalLoader / L"config";
            const auto modRoot = gameRoot / L"mods" / L"Reimagined";
            const auto modLoader = modRoot / L"d2rloader";
            const auto modConfig = modLoader / L"config";
            const auto modMpqExcel = modRoot / L"Reimagined.mpq" / L"data" / L"global" / L"excel";
            std::filesystem::create_directories(globalConfig);
            std::filesystem::create_directories(modConfig);
            std::filesystem::create_directories(modMpqExcel);

            for (const auto* tbl : {L"itemtypes.txt", L"weapons.txt", L"armor.txt", L"misc.txt"}) {
                std::filesystem::copy_file(
                    excelDir / tbl, modMpqExcel / tbl,
                    std::filesystem::copy_options::overwrite_existing, ec);
            }
            WriteTextFile(
                modRoot / L"Reimagined.mpq" / L"data" / L"hd" / L"global" / L"sfx" / L"item" / L"Filter01.flac",
                "fLaC");
            WriteTextFile(
                modMpqExcel / L"sounds.txt",
                "Sound\t*Index\tRedirect\tChannel\tFileName\r\n"
                "shockwavedust_hd3\t11991\t\tmaster\tmonster\\golemclay\\spawnclay3_hd.flac\r\n"
                "Filter01\t11992\t\tsfx/items_hd\titem\\Filter01.flac\r\n");

            // Global plugin context: scopeRootDirectory = <GameDir>/d2rloader, activeMod = "Reimagined"
            const Schema::FilterPathInputs globalPluginInputs{
                .modDirectory = modRoot / L"Reimagined.mpq",
                .modSupportDirectory = modLoader,
                .pluginConfigPath = globalConfig / L"pyre-lootfilter.toml",
                .scopeRootDirectory = globalLoader,
                .gameDirectory = gameRoot,
                .activeMod = "Reimagined",
            };

            Check(Schema::ResolveActiveModRoot(globalPluginInputs) == modRoot.lexically_normal(),
                  "ResolveActiveModRoot resolves mods/Reimagined from global plugin context");

            const auto candidates = Schema::BuildFilterJsonCandidates(globalPluginInputs);
            Check(candidates.size() == 2 &&
                      candidates[0] == (modConfig / L"filter.json").lexically_normal() &&
                      candidates[1] == (globalConfig / L"filter.json").lexically_normal(),
                  "BuildFilterJsonCandidates includes both Mod and Global config/filter.json paths");

            // F.1: Global DLL + Mod filter.json (with baseName, itemType, tier, and migrated Filter01 dropSound)
            const auto modFilterPath = modConfig / L"filter.json";
            const auto globalFilterPath = globalConfig / L"filter.json";
            WriteTextFile(
                modFilterPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "Mod-folder filter loaded from Global DLL",
        "conditions": {
          "baseName": "Phase Blade",
          "itemType": "Sword",
          "tier": "elite"
        },
        "name": "Mod Scope PB",
        "dropSound": "Filter01"
      }
    }
  ]
})json");

            const auto orderedModOnly = Schema::OrderFilterCandidatesForLoad(candidates);
            Check(!orderedModOnly.empty() && orderedModOnly.front() == modFilterPath.lexically_normal(),
                  "OrderFilterCandidatesForLoad selects existing Mod filter.json when Global filter.json is absent");
            const auto modLoaded = Schema::LoadFilterRuleTable(
                orderedModOnly.front(), 201, nullptr, Schema::ResolveActiveModRoot(globalPluginInputs));
            Check(modLoaded != nullptr && modLoaded->rules.size() == 1 && modLoaded->soundRules == 1,
                  "Global DLL loads Mod filter.json with baseName/itemType/tier and migrated Filter01.flac");

            // F.2: Global filter.json only (both with and without modRootHint)
            std::filesystem::remove(modFilterPath, ec);
            WriteTextFile(
                globalFilterPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "Global-folder filter discovering Mod Excel tables",
        "conditions": {
          "baseName": "Monarch",
          "itemType": "Shield",
          "tier": "elite"
        },
        "name": "Global Scope Monarch",
        "dropSound": "Filter01"
      }
    }
  ]
})json");

            const auto orderedGlobalOnly = Schema::OrderFilterCandidatesForLoad(candidates);
            Check(!orderedGlobalOnly.empty() &&
                      orderedGlobalOnly.front() == globalFilterPath.lexically_normal(),
                  "OrderFilterCandidatesForLoad falls back to Global filter.json when Mod filter.json is absent");
            const auto globalLoadedWithHint = Schema::LoadFilterRuleTable(
                orderedGlobalOnly.front(), 202, nullptr, Schema::ResolveActiveModRoot(globalPluginInputs));
            const auto globalLoadedWithoutHint = Schema::LoadFilterRuleTable(
                orderedGlobalOnly.front(), 203, nullptr);
            Check(globalLoadedWithHint != nullptr && globalLoadedWithHint->rules.size() == 1 &&
                      globalLoadedWithHint->soundRules == 1 &&
                      globalLoadedWithoutHint != nullptr && globalLoadedWithoutHint->rules.size() == 1 &&
                      globalLoadedWithoutHint->soundRules == 1,
                  "Global filter.json discovers mods/Reimagined Excel tables and sound assets with or without modRootHint");

            // F.3: Both Mod and Global filter.json exist -> most recently modified wins
            WriteTextFile(
                modFilterPath,
                R"json({
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "7cr" }, "name": "Newer Mod Filter" } }
  ]
})json");
            const auto nowTime = std::filesystem::last_write_time(modFilterPath, ec);
            std::filesystem::last_write_time(
                globalFilterPath, nowTime - std::chrono::seconds(60), ec);
            const auto orderedModNewer = Schema::OrderFilterCandidatesForLoad(candidates);
            Check(orderedModNewer.front() == modFilterPath.lexically_normal(),
                  "When both exist, newer Mod filter.json is ordered before older Global filter.json");

            std::filesystem::last_write_time(
                globalFilterPath, nowTime + std::chrono::seconds(60), ec);
            const auto orderedGlobalNewer = Schema::OrderFilterCandidatesForLoad(candidates);
            Check(orderedGlobalNewer.front() == globalFilterPath.lexically_normal(),
                  "When both exist, newer Global filter.json is ordered before older Mod filter.json");

            // G. Dedicated config/filter/ directory discovery & human-readable error diagnostics
            Schema::EnsureFilterDirectoriesExist(globalPluginInputs);
            Check(std::filesystem::is_directory(modConfig / L"filter", ec) &&
                      std::filesystem::is_directory(globalConfig / L"filter", ec),
                  "EnsureFilterDirectoriesExist creates d2rloader/config/filter in Mod and Global folders");

            const auto modStrictPath = modConfig / L"filter" / L"strict.json";
            const auto globalLevelingPath = globalConfig / L"filter" / L"leveling.json";
            WriteTextFile(
                modStrictPath,
                R"json({
  "name": "Reimagined Strict S2",
  "season": "2",
  "modVersion": "2.1.0",
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "7cr" }, "name": "Strict PB" } }
  ]
})json");
            WriteTextFile(
                globalLevelingPath,
                R"json({
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "hpo" }, "name": "HP" } }
  ]
})json");

            const auto profiles = Schema::DiscoverFilterProfiles(globalPluginInputs);
            Check(profiles.size() == 4,
                  "DiscoverFilterProfiles finds config/filter/*.json and legacy config/filter.json across Mod and Global");
            Check(profiles[0].path == modStrictPath.lexically_normal() &&
                      profiles[0].shortPath == "mods/Reimagined/d2rloader/config/filter/strict.json" &&
                      !profiles[0].isGlobalScope,
                  "First discovered profile is Mod config/filter/strict.json");
            Check(profiles[2].displayName == "filter" &&
                      profiles[3].displayName == "filter (Global)",
                  "DiscoverFilterProfiles disambiguates Mod vs Global filter.json cleanly in a single pass");

            // G.0b: Identical config/filter/filter.json skips redundant legacy config/filter.json
            const auto modFolderFilterPath = modConfig / L"filter" / L"filter.json";
            std::filesystem::copy_file(modFilterPath, modFolderFilterPath,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            const auto dedupedProfiles = Schema::DiscoverFilterProfiles(globalPluginInputs);
            Check(dedupedProfiles.size() == 4 &&
                      dedupedProfiles[0].path == modFolderFilterPath.lexically_normal() &&
                      dedupedProfiles[0].displayName == "filter",
                  "DiscoverFilterProfiles skips redundant legacy config/filter.json when identical to config/filter/filter.json");

            // G.0c: Distinct config/filter/filter.json vs legacy config/filter.json labels legacy as 'filter (Legacy)'
            WriteTextFile(
                modFolderFilterPath,
                R"json({
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "7cr" }, "name": "Distinct Folder Filter" } },
    { "show": { "conditions": { "code": "hpo" }, "name": "Second Rule" } }
  ]
})json");
            const auto distinctProfiles = Schema::DiscoverFilterProfiles(globalPluginInputs);
            Check(distinctProfiles.size() == 5 &&
                      distinctProfiles[0].displayName == "filter" &&
                      distinctProfiles[3].displayName == "filter (Legacy)" &&
                      distinctProfiles[4].displayName == "filter (Global)",
                  "DiscoverFilterProfiles labels distinct legacy config/filter.json as 'filter (Legacy)' without repeating suffixes");
            std::filesystem::remove(modFolderFilterPath, ec);

            std::string strictErr;
            const auto strictTable = Schema::LoadFilterRuleTable(
                modStrictPath, 301, nullptr, Schema::ResolveActiveModRoot(globalPluginInputs),
                &strictErr);
            Check(strictTable != nullptr && strictErr.empty() &&
                      strictTable->filterName == "Reimagined Strict S2" &&
                      strictTable->season == "2" &&
                      strictTable->modVersion == "2.1.0",
                  "LoadFilterRuleTable parses optional name, season, and modVersion metadata");

            // G.1: Version mismatch human error
            const auto badVerPath = modConfig / L"filter" / L"bad_version.json";
            WriteTextFile(badVerPath, R"json({ "version": 99, "rules": [] })json");
            std::string humanErr;
            Check(Schema::LoadFilterRuleTable(badVerPath, 302, nullptr, {}, &humanErr) == nullptr &&
                      humanErr.find("Version mismatch") != std::string::npos &&
                      humanErr.find("v99") != std::string::npos,
                  "LoadFilterRuleTable reports human-readable Version mismatch error");

            // G.2: Browser library backup human error
            const auto backupPath = modConfig / L"filter" / L"library_backup.json";
            WriteTextFile(backupPath,
                          R"json({ "format": "pyre-lootfilter-library", "presets": [] })json");
            humanErr.clear();
            Check(Schema::LoadFilterRuleTable(backupPath, 303, nullptr, {}, &humanErr) == nullptr &&
                      humanErr.find("browser-library backup") != std::string::npos,
                  "LoadFilterRuleTable reports human-readable browser-library backup error");

            // G.3: Wrong season / mod version unknown baseName human error
            const auto wrongSeasonPath = modConfig / L"filter" / L"wrong_season.json";
            WriteTextFile(
                wrongSeasonPath,
                R"json({
  "version": 3,
  "rules": [
    {
      "show": {
        "ruleName": "Season 99 Mythic Rule",
        "conditions": { "baseName": "Season99 Mythic Blade" }
      }
    }
  ]
})json");
            humanErr.clear();
            Check(Schema::LoadFilterRuleTable(
                      wrongSeasonPath, 304, nullptr,
                      Schema::ResolveActiveModRoot(globalPluginInputs), &humanErr) == nullptr &&
                      humanErr.find("Season99 Mythic Blade") != std::string::npos &&
                      humanErr.find("Mod/Season mismatch") != std::string::npos,
                  "LoadFilterRuleTable explains when baseName is missing due to wrong season or mod version");

            // G.4: JSON syntax error human error
            const auto syntaxErrPath = modConfig / L"filter" / L"syntax_error.json";
            WriteTextFile(syntaxErrPath, "{\n  \"version\": 3,\n  \"rules\": [\n    broken\n  ]\n}");
            humanErr.clear();
            Check(Schema::LoadFilterRuleTable(syntaxErrPath, 305, nullptr, {}, &humanErr) == nullptr &&
                      humanErr.find("Invalid JSON syntax on line 4") != std::string::npos,
                  "LoadFilterRuleTable reports human-readable JSON syntax error with line number");

            // G.5: Reloading a selected/active filter with a syntax error targets ONLY the selected
            // filter (never falling back to other candidates) and preserves the last valid rules.
            std::filesystem::remove(badVerPath, ec);
            std::filesystem::remove(backupPath, ec);
            std::filesystem::remove(wrongSeasonPath, ec);
            std::filesystem::remove(syntaxErrPath, ec);
            const auto refreshedProfiles = Schema::DiscoverFilterProfiles(globalPluginInputs);
            const std::string selectedFilter = "mods/Reimagined/d2rloader/config/filter/strict.json";
            const auto selectedTargets = Schema::SelectReloadTargetCandidates(
                globalPluginInputs, refreshedProfiles, selectedFilter,
                modStrictPath, modStrictPath);
            Check(selectedTargets.size() == 1U &&
                      selectedTargets.front() == modStrictPath.lexically_normal(),
                  "SelectReloadTargetCandidates targets only the selected profile and does not include fallback candidates");

            // Introduce a syntax error into the active profile (modStrictPath) while other valid
            // candidate profiles (globalLevelingPath, modFilterPath) exist on disk:
            auto activeRules = strictTable;
            WriteTextFile(modStrictPath, "{\n  \"name\": \"Reimagined Strict S2\",\n  \"version\": 3,\n  \"rules\": [\n    typo_here\n  ]\n}");
            const auto targetsAfterTypo = Schema::SelectReloadTargetCandidates(
                globalPluginInputs, Schema::DiscoverFilterProfiles(globalPluginInputs),
                selectedFilter, modStrictPath, modStrictPath);
            Check(targetsAfterTypo.size() == 1U &&
                      targetsAfterTypo.front() == modStrictPath.lexically_normal(),
                  "After saving a syntax error in the selected filter, reload still targets only that filter");
            std::string reloadErr;
            const auto failedReloadTable = Schema::LoadFilterRuleTable(
                targetsAfterTypo.front(), 306, nullptr,
                Schema::ResolveActiveModRoot(globalPluginInputs), &reloadErr);
            if (failedReloadTable) {
                activeRules = failedReloadTable;
            }
            Check(failedReloadTable == nullptr &&
                      !reloadErr.empty() &&
                      reloadErr.find("Invalid JSON syntax") != std::string::npos &&
                      activeRules == strictTable &&
                      activeRules->filterName == "Reimagined Strict S2" &&
                      activeRules->rules.size() == 1U,
                  "Syntax error on reload of selected profile preserves previous rules and records error without switching to another candidate");

            // G.6: Modifying or creating a non-selected candidate profile does not switch the active profile
            const auto nonSelectedPath = modConfig / L"filter" / L"other_downloaded.json";
            WriteTextFile(
                nonSelectedPath,
                R"json({
  "name": "Downloaded Other Filter",
  "version": 3,
  "rules": [
    { "show": { "conditions": { "code": "mp5" }, "name": "Other Rule" } }
  ]
})json");
            const auto profilesWithOther = Schema::DiscoverFilterProfiles(globalPluginInputs);
            const auto targetsAfterOtherEdit = Schema::SelectReloadTargetCandidates(
                globalPluginInputs, profilesWithOther, selectedFilter,
                modStrictPath, modStrictPath);
            Check(targetsAfterOtherEdit.size() == 1U &&
                      targetsAfterOtherEdit.front() == modStrictPath.lexically_normal() &&
                      targetsAfterOtherEdit.front() != nonSelectedPath.lexically_normal(),
                  "Modifying or adding a non-selected candidate file does not switch the active profile");
            std::filesystem::remove(nonSelectedPath, ec);

            // G.7: Cross-mod selected_filter path (e.g. ReimaginedLadder vs Reimagined) resolves by filename fallback
            const auto crossModResolved = Schema::ResolveConfiguredProfilePath(
                "mods/ReimaginedLadder/d2rloader/config/filter/strict.json", refreshedProfiles);
            Check(crossModResolved == modStrictPath.lexically_normal(),
                  "ResolveConfiguredProfilePath falls back to matching profile filename across mod folders");

            // G.8: BuildPanelStatusText reports active in-game loot filter when disabled
            Check(ProfileDropdown::BuildPanelStatusText(false, nullptr, "", "", "") ==
                      "Loot Filter Disabled",
                  "BuildPanelStatusText reports Loot Filter Disabled when no in-game filter is active");
            Check(ProfileDropdown::BuildPanelStatusText(
                      false, nullptr, "", "", "", "My Built-In Filter") ==
                      "Disabled (In-Game: My Built-In Filter)",
                  "BuildPanelStatusText reports active in-game loot filter name when disabled");
        }
    }

    std::filesystem::remove_all(tempRoot, ec);
    std::puts("pyre-lootfilter schema, tables, quest itemType, stats, xrayMode, and watcher tests pass.");
    return 0;
}
