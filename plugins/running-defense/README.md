# Running Defense (`Running Defense.dll`)

A D2RLoader plugin for Diablo II: Resurrected that replaces the vanilla 100% Defense loss while running (`PLRMODE_RUN == 3`) with a configurable percentage penalty, displays the actual running Defense value on the Character Sheet instead of `"---"`, and renders a running indicator icon inside the Character Sheet's Defense box.

## Features

- **Configurable Running Defense Penalty (`defense_loss_percent`):**
  - Replaces vanilla D2R's melee running auto-hit and 100% Defense loss while running with any percentage from `0` (lose no Defense while running) to `100` (vanilla behavior).
  - For example, `defense_loss_percent = 30` loses 30% of Defense while running (retaining 70%), and `defense_loss_percent = 75` loses 75% (retaining 25%).
  - Walking (`PLRMODE_WALK == 2`) continues to preserve 100% of Defense and Block.
- **Character Sheet Defense Display & Always-Visible Running Indicator:**
  - Prevents the Character Sheet (`CharacterStatsPanel`) from replacing the Defense number with `"---"` while running when `defense_loss_percent < 100`.
  - Updates the Character Sheet Defense number and "Chance to Hit You" tooltip dynamically when running.
  - Draws a crisp running figure icon next to the Defense number inside the `ArmorClass` box on the Character Sheet regardless of whether the player is currently running or standing still (lighting up in active amber with motion ticks while running).
- **Optional Running Shield Block Override (`override_block_penalty`, `block_loss_percent`):**
  - Optionally replaces vanilla D2R's running Shield Block penalty (`block / 3`, ~67% loss) with a custom percentage penalty.

## Configuration (`running-defense.toml`)

```toml
[running-defense]
enabled = true
defense_loss_percent = 30
override_block_penalty = false
block_loss_percent = 67
show_running_icon_on_sheet = true
highlight_defense_when_running = true
```

## Installation and Console Command

Copy `Running Defense.dll` into your `d2rloader/plugins/` directory (or install the release `.zip` through the D2R Reimagined Launcher's **Install Plugin/Zip** button). Missing `d2rloader/config/running-defense.toml` files are created automatically on first launch.

Use `/runningdefense` or `/runningdefense status` in the D2RLoader console to inspect active penalty settings and native hook admission state.

See [docs/reverse-engineering/running-defense.md](../../docs/reverse-engineering/running-defense.md) for the complete native RVA and byte-guard contract.
