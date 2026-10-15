# D2R Mods stable suite - 2026.10.15.1

Six independently installable plugins now use stable `1.3.1+rev.N` versions.
Each plugin has its own revision number.

| Plugin | Version |
| --- | --- |
| Controller QOL Updates | 1.3.1+rev.78 |
| Item Roll Ranges | 1.3.1+rev.46 |
| Floating Damage | 1.3.1+rev.13 |
| Always Show Sockets | 1.3.1+rev.2 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 1.3.1+rev.13 |

## Changes

- **Persistent chat settings:** Floating Damage, Item Roll Ranges and socket
  visibility commands now save to their active TOML and survive restart.
  Status commands are read-only. Save failures are reported explicitly.
- **Item Roll Ranges:** Adds `(Ilvl: XX, Qlvl: YY)` below Required Level and
  above Max Sockets/weapon Attack Speed, including items without Required Level.
  Uses the existing range controls; always-show remains off by default.
- **Always Show Sockets:** Joins the suite with hover/selection, continuous and
  native display modes. Size, opacity, ring, center and lift commands save your
  appearance settings; subsequent launches restore the saved mode.
- **Inventory Deposit and Floating Damage:** Move from beta names to the stable
  version format. Their transfer and rendering behavior remains the same.
- **All packages:** Contain only the DLL, reference defaults and one complete
  user README with controls, commands, settings and editing examples. Engineering
  records stay in the repository. Controller QOL and Map Assistance retain their
  released behavior.

## Commands and controls by plugin

Enter slash commands in game chat. Feedback appears in the loader console.
Setting commands save automatically to the active plugin TOML and survive
restart. Status commands only report the current state.

### Controller QOL Updates

See the [complete Controller QOL README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/controller-qol/README.md)
for every default button combination, shared settings, per-mod skill profiles,
custom skills, targeting, leading and editing examples.

### Item Roll Ranges

Hold Ctrl or RT/R2 for ranges. Controller QOL's menu remap uses RB/R1.
Always-show defaults off for new installs.

| Command | What it does |
| --- | --- |
| `/AlwaysShowRolls on` | Show ranges continuously and save this preference. |
| `/AlwaysShowRolls off` | Return to hold-to-show and save this preference. |
| `/AlwaysShowRolls` | Toggle always-show and save the new state. |
| `/AlwaysShowRolls status` | Report the current state without saving. |

[Full Item Roll Ranges README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/item-roll-ranges/README.md)

### Floating Damage

| Command | What it does |
| --- | --- |
| `/damagenumbers on` | Enable numbers and save `enabled = true`. |
| `/damagenumbers off` | Hide numbers and save `enabled = false`. |
| `/damagenumbers` | Toggle numbers and save the new state. |
| `/damagenumbers status` | Report enabled state without saving. |
| `/damagenumbers motion on` | Save rise/fall/sideways-drift animation. |
| `/damagenumbers motion off` | Save straight-rise animation. |
| `/damagenumbers motion status` | Report the animation mode without saving. |

Numbers support local single player. Diagnostic capture is a bounded recording
action, rather than a startup preference.

[Full Floating Damage README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/floating-damage/README.md)

### Always Show Sockets

| Command | What it does |
| --- | --- |
| `/sockets default` | Save styled sockets on normal hover/selection. Fresh-install default. |
| `/sockets always` | Save continuous styled socket visibility. |
| `/sockets off` | Save native visibility and appearance. |
| `/sockets` | Toggle off/on and save it, retaining the last styled mode. |
| `/sockets size 65` | Save 65% inserted artwork size; circles follow at 80%. |
| `/sockets opacity 65` | Save artwork opacity at 65% of its native fade. |
| `/sockets ring 80` | Save ring opacity at 80%. |
| `/sockets center 40` | Save black-center opacity at 40%. |
| `/sockets lift 1` | Save a one-pixel upward artwork shift. |
| `/sockets status` | Report visibility and appearance without saving. |

Modes and appearance now restore after restart. A save warning means a socket
change is temporary until saving succeeds.

[Full Always Show Sockets README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/always-show-sockets/README.md)

### Map Assistance

No chat commands. Hover a normal Keys stack outside town to read the current
area's static map-reading tip. Edit existing area entries in the active
`map-assistance.toml` with the game closed, then restart.

[Full Map Assistance README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/map-assistance/README.md)

### Inventory Deposit

No chat commands. Open the advanced stash in keyboard/mouse mode, then click
**Deposit All** beside inventory gold to store eligible carried materials,
gems, runes and rejuvenation potions. Equipment, belt, Cube and Charm Inventory
contents are excluded.

[Full Inventory Deposit README](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/inventory-deposit/README.md)

## Installation

Close the game, back up the previous DLLs and extract each selected ZIP into
the game directory or active mod directory. DLLs go under `d2rloader/plugins/`.
Use one active copy per plugin, then restart through D2RLoader. The D2RR launcher
can remain open. Managed mod packs should use their normal update process.

Existing settings are preserved. Files under `defaults/` are references;
missing main configs are created automatically. No live config files are
shipped. `SHA256SUMS` alongside the downloads covers all six ZIP files.

## Validation and limits

All 68 plugin suites and 34 packaging tests passed locally. Live checks
confirmed the new level line's placement and Ctrl release. Prior live coverage
includes the documented item shortcuts, ranges, socket appearance and damage
numbers; stable naming does not extend that coverage.

Chat-setting save/restart and the latest Floating Damage 4K/critical
style visuals still need live confirmation. Floating Damage supports local
single player. Not every item, mod, controller or resolution has been tested.
Existing shutdown issues remain under investigation; this release does not
claim to fix them. Each plugin's README describes its requirements and links
to repository compatibility records. Native features disable unsupported paths.
