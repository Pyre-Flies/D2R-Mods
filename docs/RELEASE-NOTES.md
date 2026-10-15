# D2R Mods stable suite - 2026.10.15.1

Six independently installable plugins now use stable `1.3.1+rev.N` versions.
Each plugin has its own revision number.

| Plugin | Version |
| --- | --- |
| Controller QOL Updates | 1.3.1+rev.78 |
| Item Roll Ranges | 1.3.1+rev.45 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 1.3.1+rev.13 |
| Floating Damage | 1.3.1+rev.12 |
| Always Show Sockets | 1.3.1+rev.1 |

## Changes

- **Item Roll Ranges:** Adds `(Ilvl: XX, Qlvl: YY)` below Required Level and
  above Max Sockets/weapon Attack Speed, including items without Required Level.
  Uses the existing range controls; always-show remains off by default.
- **Always Show Sockets:** Joins the suite with hover/selection, continuous and
  native display modes. Size, opacity, ring, center and lift commands save your
  appearance settings; each launch starts in default hover/selection mode.
- **Inventory Deposit and Floating Damage:** Move from beta names to the stable
  version format, preserving their existing behavior and settings.
- **All packages:** Contain only the DLL, reference defaults and one usage
  README with command examples. Detailed documentation stays in the repository.
  Controller QOL and Map Assistance retain their released behavior.

## Commands and controls

Enter slash commands in game chat. Feedback appears in the loader console.

| Example | Effect |
| --- | --- |
| Hold Ctrl or RT/R2 | Show ranges and item/base levels. Controller QOL's menu remap uses RB/R1. |
| `/AlwaysShowRolls on` | Show ranges continuously for this session. |
| `/AlwaysShowRolls off` | Return to holding the range control. |
| `/AlwaysShowRolls status` | Report always-show state. |
| `/sockets always` | Keep styled sockets visible for this session. |
| `/sockets default` | Show styled sockets on normal hover/selection. |
| `/sockets ring 80` | Set ring opacity to 80% and save it. |
| `/sockets center 40` | Set black-center opacity to 40% and save it. |
| `/sockets status` | Report socket mode and appearance values. |
| `/damagenumbers off` | Hide floating damage numbers for this session. |
| `/damagenumbers motion off` | Use straight-rise animation for this session. |

See each ZIP's README for all controls, configuration examples and requirements.

## Installation

Close the game, back up the previous DLLs and extract each selected ZIP into
the game directory or active mod directory. DLLs go under `d2rloader/plugins/`.
Use one active copy per plugin, then restart through D2RLoader. The D2RR launcher
can remain open. Managed mod packs should use their normal update process.

Existing settings are preserved. Files under `defaults/` are references;
missing main configs are created automatically. No live config files are
shipped. `SHA256SUMS` alongside the downloads covers all six ZIP files.

## Validation and limits

All 67 plugin suites and the packaging checks passed locally. Live checks
confirmed the new level line's placement and Ctrl release. Prior live coverage
includes the documented item shortcuts, ranges, socket appearance and damage
numbers; stable naming does not extend that coverage.

Socket appearance autosave/restart and the latest Floating Damage 4K/critical
style visuals still need live confirmation. Floating Damage supports local
single player. Not every item, mod, controller or resolution has been tested.
Existing shutdown issues remain under investigation; this release does not
claim to fix them. Native features require the supported builds listed in each
package's compatibility file and disable unsupported paths.
