#include "config.h"
#include "sheet_layout.h"

#include <array>
#include <cstdio>
#include <cstdlib>

namespace {

void Check(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

} // namespace

int main() {
    using namespace RunningDefense;

    // 1. Default settings and scaling math.
    {
        unsigned rejected = 99;
        const auto defaults = ParseConfig("", rejected);
        Check(rejected == 0, "empty config parses without rejections");
        Check(defaults.enabled, "enabled defaults to true");
        Check(defaults.defenseLossPercent == 30, "defense_loss_percent defaults to 30");
        Check(!defaults.overrideBlockPenalty, "override_block_penalty defaults to false");
        Check(defaults.blockLossPercent == 67, "block_loss_percent defaults to 67");
        Check(defaults.showRunningIconOnSheet, "show_running_icon_on_sheet defaults to true");
        Check(defaults.highlightDefenseWhenRunning, "highlight_defense_when_running defaults to true");

        Check(ScaleDefenseForRun(1000, 30) == 700, "30% defense loss retains 70% (700 of 1000)");
        Check(ScaleDefenseForRun(1000, 75) == 250, "75% defense loss retains 25% (250 of 1000)");
        Check(ScaleDefenseForRun(1000, 0) == 1000, "0% defense loss retains 100%");
        Check(ScaleDefenseForRun(1000, 100) == 0, "100% defense loss retains 0%");
        Check(ScaleDefenseForRun(0, 30) == 0, "zero defense stays zero");
        Check(ScaleDefenseForRun(50000, 30) == 35000, "large defense scales without overflow");

        Check(ScaleBlockForRun(75, 67) == 24, "67% block loss retains 33% of 75");
        Check(ScaleBlockForRun(75, 30) == 52, "30% block loss retains 70% of 75");
        Check(ScaleBlockForRun(75, 0) == 75, "0% block loss retains 100% of 75");
        Check(ScaleBlockForRun(75, 100) == 0, "100% block loss retains 0%");
    }

    // 2. Custom TOML configuration parsing.
    {
        constexpr const char* customToml = R"(
            # Comment line
            [running-defense]
            enabled = true
            defense_loss_percent = 75
            override_block_penalty = true
            block_loss_percent = 40
            show_running_icon_on_sheet = true
            highlight_defense_when_running = false
        )";
        unsigned rejected = 0;
        const auto parsed = ParseConfig(customToml, rejected);
        Check(rejected == 0, "valid custom TOML has zero rejections");
        Check(parsed.enabled, "custom enabled parsed");
        Check(parsed.defenseLossPercent == 75, "custom defense_loss_percent=75 parsed");
        Check(parsed.overrideBlockPenalty, "custom override_block_penalty=true parsed");
        Check(parsed.blockLossPercent == 40, "custom block_loss_percent=40 parsed");
        Check(parsed.showRunningIconOnSheet, "custom show_running_icon_on_sheet=true parsed");
        Check(!parsed.highlightDefenseWhenRunning, "custom highlight_defense_when_running=false parsed");
    }

    // 3. Invalid values are rejected while keeping valid values and defaults.
    {
        constexpr const char* invalidToml = R"(
            [running-defense]
            defense_loss_percent = 150
            block_loss_percent = -5
            enabled = maybe
            unknown_key = 42
            show_running_icon_on_sheet = false
            [other-section]
            defense_loss_percent = 10
        )";
        unsigned rejected = 0;
        const auto parsed = ParseConfig(invalidToml, rejected);
        Check(rejected == 6, "all 6 invalid/foreign entries counted as rejected");
        Check(parsed.defenseLossPercent == DefaultDefenseLossPercent, "out-of-range defense_loss_percent falls back to default");
        Check(parsed.blockLossPercent == DefaultBlockLossPercent, "negative block_loss_percent falls back to default");
        Check(parsed.enabled, "invalid bool falls back to default");
        Check(!parsed.showRunningIconOnSheet, "valid key alongside invalid keys still applies");
    }

    // 4. Widget layout bounds and running icon placement inside ArmorClass box.
    {
        // ArmorClass widget (842, 730, 225x86) inside CharacterStatsPanel at 1080p scale (0.5).
        const std::array<LayoutNode, 2> chain{{
            {842.0f, 730.0f, 225.0f, 86.0f, 0.0f, 0.0f, 1.0f, false},
            {160.0f, 120.0f, 1162.0f, 1417.0f, 0.0f, 0.0f, 0.5f, false},
        }};
        ScreenRect box{};
        Check(LayoutBounds(chain, box), "ArmorClass widget chain resolves screen bounds");
        Check(box.width > 110.0f && box.height > 40.0f, "scaled ArmorClass box has expected 1080p dimensions");

        Point centerSmall{}, centerLarge{};
        float scaleSmall{}, scaleLarge{};
        Check(ComputeRunningIconPlacement(box, 120, centerSmall, scaleSmall), "icon placement succeeds for 3-digit defense");
        Check(ComputeRunningIconPlacement(box, 24500, centerLarge, scaleLarge), "icon placement succeeds for 5-digit defense");
        Check(centerSmall.x > box.x + box.width * 0.5f, "icon sits to the right of box center");
        Check(centerLarge.x >= centerSmall.x, "5-digit defense pushes icon further right than 3-digit defense");
        Check(centerLarge.x + scaleLarge <= box.x + box.width, "icon stays inside right border of ArmorClass box");
    }

    return 0;
}
