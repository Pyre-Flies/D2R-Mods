#include "config.h"
#include <cstdio>
#include <cstdlib>

namespace {
void Check(bool value, const char* name) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}
}

int main() {
    using namespace Ultrawide::Config;
    bool invalid = false;
    const auto defaults = Parse("", invalid);
    Check(!invalid && defaults == Settings{}, "empty config yields defaults");

    const char* customToml = R"(
# Ultrawide config test
[ultrawide]
enable_bars_removal = false
enable_mouse_unclamp = true
enable_placard_unclamp = false
enable_automap_unclamp = false
enable_full_length_hud = true
enable_corner_minimap = false
enable_corner_mercenary = false
enable_corner_game_info = false
enable_terrain_expansion = false
enable_monster_pop_in_fix = true
monster_pop_in_radius = 2
enable_monster_ai_range = true
monster_ai_range = 65
scale_friendly_ai_range = true
)";
    const auto custom = Parse(customToml, invalid);
    Check(!invalid, "valid custom config has no errors");
    Check(!custom.enableBarsRemoval && custom.enableMouseUnclamp && !custom.enablePlacardUnclamp &&
        !custom.enableAutomapUnclamp && custom.enableFullLengthHud && !custom.enableCornerMinimap &&
        !custom.enableCornerMercenary && !custom.enableCornerGameInfo &&
        !custom.enableTerrainExpansion && custom.enableMonsterPopInFix && custom.monsterPopInRadius == 2 &&
        custom.enableMonsterAiRange && custom.monsterAiRange == 65 && custom.scaleFriendlyAiRange, "custom values parsed");

    const auto hudModeFull = Parse("[ultrawide]\nhud_mode = \"full\"\n", invalid);
    Check(!invalid && hudModeFull.enableFullLengthHud, "hud_mode = \"full\" parsed");
    const auto hudModeCentered = Parse("[ultrawide]\nhud_mode = \"centered\"\n", invalid);
    Check(!invalid && !hudModeCentered.enableFullLengthHud, "hud_mode = \"centered\" parsed");

    const auto clampedLow = Parse("[ultrawide]\nmonster_ai_range = 10\nmonster_pop_in_radius = 0\n", invalid);
    Check(!invalid && clampedLow.monsterAiRange == MinMonsterAiRange && clampedLow.monsterPopInRadius == MinMonsterPopInRadius,
        "monster_ai_range and monster_pop_in_radius clamp to minimums");

    const auto clampedHigh = Parse("[ultrawide]\nmonster_ai_range = 250\nmonster_pop_in_radius = 9\n", invalid);
    Check(!invalid && clampedHigh.monsterAiRange == MaxMonsterAiRange && clampedHigh.monsterPopInRadius == MaxMonsterPopInRadius,
        "monster_ai_range and monster_pop_in_radius clamp to maximums");

    const auto dupBool = Parse("[ultrawide]\nenable_bars_removal = false\nenable_bars_removal = true\n", invalid);
    Check(invalid && dupBool.enableBarsRemoval, "duplicate boolean key marks invalid and falls back to default");

    const auto badInt = Parse("[ultrawide]\nmonster_ai_range = abc\n", invalid);
    Check(invalid && badInt.monsterAiRange == DefaultMonsterAiRange, "malformed integer marks invalid and falls back to default");

    const auto otherSection = Parse("[other]\nenable_bars_removal = false\nmonster_ai_range = 80\n", invalid);
    Check(!invalid && otherSection == Settings{}, "unrelated section ignored");

    Check(MenuSideWidth(5120, 1440, false) == 1040 && MenuSideWidth(5120, 1440, true) == 0 &&
        MenuSideWidth(1920, 1080, false) == 0, "menu/game aspect helper");

    std::puts("Ultrawide config parser and clamping checks passed.");
}
