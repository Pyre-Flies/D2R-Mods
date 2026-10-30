# Item Roll Ranges

**1.3.1+rev.56 · by PyreFly**

Shows possible modifier rolls and their sources beside the item's actual
values. Ranges use the active game's definitions, including supported modded
affixes, uniques and runewords.

## Controls and display examples

Hold **Ctrl** on keyboard or **RT/R2** on controller to show ranges. With
Controller QOL's menu remap active, use **RB/R1** instead. Release to restore
the normal tooltip. Inventory, stash, Cube and vendor tooltips are supported.

| Example | Meaning |
| --- | --- |
| `[+80 - +120] +118 Defense` | Actual roll 118; possible rolls 80 through 120. |
| `[+38] +38 to Life` | This source has a fixed value. |
| `Two-Hand Damage: 32 to 69 (Base: 9 - 19)` | Final physical damage, followed by intrinsic base damage. |
| `Defense: 246 (Base: 127 - 127)` | Final Defense, followed by its fixed pre-affix base. Enhanced Defense can cause the game's maximum-base-plus-one rule. |
| `(Ilvl: 99, Qlvl: 33)` | Generated item level and the base type's quality level. |

Level details appear below Required Level, or in the available header position
when no required level exists. Max Sockets appears in the header, before weapon
Attack Speed. Base values account for applicable quality/ethereal rules and
exclude added affix damage/Defense. Fixed Base bounds retain both endpoints.

## Sources and combined modifiers

| Label | Meaning |
| --- | --- |
| `[P]`, `[S]` | Prefix or suffix; name and tier follow. T1 is the strongest applicable tier in that family. |
| `[U]`, `[Set]` | Unique-item or permanent set-item property. |
| `[RW]` | Runeword-owned property. |
| `[Craft]` | Fixed or ranged crafted-recipe property. |
| `[Base]` | Inherent property of the base item. |
| `[Socket 1]` | Contribution from the first socketed item; subsequent sockets are numbered. |
| `[C]`, `[E]` | Identified corruption or enchantment contribution. |
| `[Combined]` | Total of overlapping sources; gray rows show their individual contributions. |

When the saved total cannot uniquely identify an individual roll, that source
can remain `?`. A `(?)` range warning means the plugin could not establish a
reliable bound. Activated multi-piece set bonuses retain their native
presentation. Keep screenshots of uncertain cases with ranges shown when
reporting them.

## Commands and configuration

Enter these in game chat. Feedback appears in the loader console.

| Command | What it does |
| --- | --- |
| `/AlwaysShowRolls` | Toggle always-show and save the setting. |
| `/AlwaysShowRolls toggle` | Explicit form of the bare toggle; saves the new state. |
| `/AlwaysShowRolls on` | Show ranges without holding Ctrl/RT. |
| `/AlwaysShowRolls off` | Return to holding Ctrl/RT for ranges. |
| `/AlwaysShowRolls status` | Report the current state. |

Lowercase `/alwaysshowrolls` also works. Setting commands save automatically
to the active `d2rloader/config/item-roll-ranges.toml` and survive restart.
Status is read-only. A failed save leaves the prior state active and reports
a warning. You can also edit this file with the game closed, then restart:

```toml
[display]
always_show_ranges = false
```

This is the plugin's complete configuration schema:

| Section / key | Default | Meaning |
| --- | --- | --- |
| `[display] always_show_ranges` | `false` | false uses the hold control; true shows ranges whenever the supported tooltip is open. |

Example: to start with ranges always visible, close the game, open the active
`d2rloader/config/item-roll-ranges.toml`, change the existing value to `true`,
save, and restart:

```toml
[display]
always_show_ranges = true
```

Use the chat commands for immediate changes with automatic saving. The native
range-enable setting below belongs to the loader configuration, not this file.

`false` is the default; `true` enables always-show on startup.

## Install, update or remove

1. Close the game. The D2RR launcher may remain open.
2. Back up the previous DLL, then extract the ZIP into the game directory for
   a global install, or the active mod directory for a mod-specific install.
   The DLL belongs in `d2rloader/plugins/`. Keep one active copy per plugin.
3. Start the game through D2RLoader.

For a launcher-managed mod pack, use its normal update process. The `defaults/`
files are references: updates preserve your active `d2rloader/config/` settings.
The loader creates a missing main configuration on first launch. Edit active
configuration files with the game closed, then restart to apply manual changes.

To remove or roll back, close the game and remove the plugin DLL or restore its
backup. Restart after any DLL change; hot reload is unsupported.

## Enable native ranges

In your effective D2RLoader configuration, ensure:

```toml
[d2rcore.items]
item_stat_ranges = true
```

Requires Windows x64, plugin ABI 4 and the D2RCore build listed in
the repository [compatibility record](docs/COMPATIBILITY.txt). Controller QOL is optional. Unsupported or conflicting
formatter paths retain native behavior.

Nine automated suites passed. Live checks cover header placement and release,
fixed Defense, resistance/Attributes combinations, socket/corruption breakdowns
and vanilla/Reimagined runewords. Every item, language and plugin combination
has not been individually tested.

For requirements, see the repository compatibility record. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` is provided alongside the download ZIPs.

Runtime ZIPs contain only the DLL, reference defaults and a usage README.
Compatibility records and release history stay in this repository.

Charged skills show their source label (affix type, name and tier where applicable).
Their displayed skill level and remaining/maximum charges stay unchanged; charges
do not receive numeric roll ranges or missing-range markers.
