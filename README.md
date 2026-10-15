# D2RLoader Mods

Quality-of-life plugins for Diablo II: Resurrected by PyreFly.
Download individual runtime ZIPs from the
[latest stable release](https://github.com/Pyre-Flies/D2R-Mods/releases/latest).

| Plugin | Stable version | What it adds |
| --- | --- | --- |
| [Controller QOL Updates](plugins/controller-qol/README.md) | 1.3.1+rev.78 | Controller item shortcuts, menu navigation and right-stick aiming. |
| [Item Roll Ranges](plugins/item-roll-ranges/README.md) | 1.3.1+rev.46 | Roll/source breakdowns, Base damage/Defense, item and base quality levels. |
| [Floating Damage](plugins/floating-damage/README.md) | 1.3.1+rev.13 | Local enemy HP-loss numbers with element colors and critical emphasis. |
| [Always Show Sockets](plugins/always-show-sockets/README.md) | 1.3.1+rev.2 | Configurable socket visibility, size and opacity. |
| [Map Assistance](plugins/map-assistance/README.md) | 1.3.1+rev.1 | Editable map-reading advice on Keys tooltips. |
| [Inventory Deposit](plugins/inventory-deposit/README.md) | 1.3.1+rev.13 | Mouse Deposit All button for eligible advanced-storage items. |

## Install and update

Close the game, back up the previous DLLs, and extract each selected ZIP into
the game directory or active mod directory. DLLs go under `d2rloader/plugins/`.
Keep one active copy of each plugin. The D2RR launcher can remain open.
For launcher-managed mod packs, use the pack's normal update process.

Start through D2RLoader. Missing main configs are created automatically under
`d2rloader/config/`. The ZIP's `defaults/` files are references; updates preserve
your settings. Edit active config files with the game closed and restart after
manual changes. Restart after installing, removing or replacing a DLL.

## Common commands and controls

Controller QOL's complete button combinations and skill-customization instructions
are in its [full README](plugins/controller-qol/README.md).

Enter slash commands in game chat; feedback appears in the loader console.

| Example | What it does |
| --- | --- |
| Hold Ctrl or RT/R2 | Show item ranges. Controller QOL's menu remap uses RB/R1. |
| `/AlwaysShowRolls on` | Show ranges continuously and save the setting. Defaults off. |
| `/AlwaysShowRolls off` | Return to the hold-to-show control. |
| `/damagenumbers off` | Hide floating numbers and save the setting. |
| `/damagenumbers motion off` | Use straight-rise animation and save the setting. |
| `/sockets always` | Show styled sockets continuously and save the mode. |
| `/sockets default` | Return to styled sockets on normal hover/selection. |
| `/sockets size 65` | Set inserted artwork size to 65% and save the appearance setting. |
| Hover Keys outside town | Read Map Assistance's tip for the current area. |
| Click Deposit All with stash open | Store eligible inventory materials, gems, runes and rejuvenation potions. |

Each guide explains its full controls, settings, requirements and validation
limits. Floating Damage supports local single player. Native integrations are
build-specific and disable unsupported features when compatibility checks fail.
Stable version numbers do not imply every mod, controller or item is tested.

## Support

Report bugs at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
Include plugin/game/mod versions and reproduction steps. Review logs for
personal information before sharing. The release includes `SHA256SUMS` alongside the ZIPs for verification.

If you enjoy these plugins, you can support their development on Ko-fi.

[![Support on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D3J128U0X2)

Source, tests and technical compatibility records are included in this repository.
Contributor and release procedures are in [CONTRIBUTING.md](CONTRIBUTING.md)
and [docs/RELEASING.md](docs/RELEASING.md).
