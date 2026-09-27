## Controller QOL Updates 1.3.1+rev.46

- Add SDK-first Identify All for inventory and Cube items, with verified tome-charge accounting, bounded sequencing, exact item identity checks, and an optional native compatibility mode.
- Add LB+L3 Stash All for eligible advanced-storage transfers and keep its header hint visible over empty stash slots. `quick_deposit` and `identify_all` independently gate the batch features.
- Keep filtered items blocked from bare-A pickup independently of the optional LB ground-pickup shortcuts, and avoid competing for Auto Deposit's shared pickup hook.
- Prefer portals, the personal stash, waypoints, shrines, and wells over nearby ground loot when using neutral A. Ordinary chests remain opt-in. Each family remains configurable and uses the shared priority distance.
- Admit a reviewed pre-existing executable detour on the shared object-class getter without patching or bypassing its owner; unknown owners and changed tails still fail open.
- Preserve the rev.22 controller header, native range-query, menu-navigation, transfer, potion, label, and crash fixes.

All 20 Controller QOL suites pass locally, including DLL ABI/export/version checks and policy coverage. The user has confirmed the later identification and bulk-stash iterations and reported that the expanded interaction priority works substantially better; rev.46's environmental-well correction still needs a focused live retest. Static and automated coverage does not imply validation on unreviewed game builds.

## Included unchanged

- Item Roll Ranges 1.3.1+rev.13
- Map Assistance 1.3.1+rev.1

## Installation

Close the game and loader, back up existing DLLs, and extract the desired ZIPs into the game directory for a global installation. Preserve existing configuration and keep only one active copy of each plugin. Debug logging defaults remain off. Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

Target: Windows x64, D2RLoader 1.3.1 / ABI4; SDK pinned to v0.3.0. Native features retain build-specific guards. See the bundled rev.46 production record for compatibility limits. Hot reload is unsupported.
