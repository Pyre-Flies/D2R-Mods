# Item Roll Ranges 1.3.1+rev.46

By PyreFly. Windows x64 and D2RLoader 1.3.1 / plugin ABI 4. Native features require the supported game/Core build; unsupported paths retain native behavior.

## Install or update

Close the game and back up the old DLL. Extract into the game directory or
active mod directory so the DLL lands in `d2rloader/plugins/`. Keep one active
copy and restart through D2RLoader. The D2RR launcher can remain open.
For managed mod packs, use their normal updater.

The `defaults/` TOMLs are references. Existing `d2rloader/config/` settings
are preserved; missing main configs are created on first launch. Edit active
configs with the game closed and restart after manual changes. To uninstall,
close the game, remove this DLL and restart.

## Use and configure

Shows modifier ranges/source breakdowns, Base damage/Defense and item/base
quality levels. Uses the active game's item definitions.

Hold Ctrl or RT/R2; with Controller QOL's menu remap, use RB/R1. Release to
restore the normal tooltip. Example: `Defense: 246 (Base: 127 - 127)`.

| Chat command | Effect |
| --- | --- |
| `/AlwaysShowRolls on` | Show ranges continuously and save the setting. |
| `/AlwaysShowRolls off` | Return to hold-to-show. |
| `/AlwaysShowRolls status` | Report the current setting. |

Lowercase `/alwaysshowrolls` also works. Always-show defaults off. Setting
commands save automatically and survive restart; status is read-only. A save
failure leaves the previous state active and reports a warning. You can also set `[display] always_show_ranges = true` or `false` in the active
`item-roll-ranges.toml`. Enable `[d2rcore.items] item_stat_ranges = true` in
the effective D2RLoader configuration. `(?)` means a bound is unresolved;
enchantment and set-bonus attribution is incomplete.

Slash commands are entered in game chat; feedback appears in the loader console.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/item-roll-ranges/docs/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
