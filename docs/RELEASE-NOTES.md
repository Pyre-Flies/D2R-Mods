## Controller QOL Updates 1.3.1+rev.48

- Restore Guided Arrow's native acquisition and homing behavior for controller ground casts by projecting only its adjacent target point to 20 tiles. Mouse casts, selected targets, other skills and the original packet remain unchanged.
- Reuse Controller QOL's existing guarded controller action hook; no additional shared hook is installed. The selected-skill guard fails open without disabling other Controller QOL features.
- Treat the Reimagined Runes page like Gems and Materials: inventory LB+X tries native advanced storage first, then sends noneligible items to the embedded Horadric Cube.
- Preserve the rev.46 identification, bulk-stash, filtered-pickup, interaction-priority, navigation, transfer, potion, label and controller-header behavior.

All 21 Controller QOL suites pass locally, including Guided Arrow policy/isolation, Runes routing and DLL ABI/export/version checks. The user confirmed the merged Guided Arrow behavior and the Runes embedded-Cube correction in game. Static and automated coverage does not imply validation on unreviewed game builds.

## Included unchanged

- Item Roll Ranges 1.3.1+rev.13
- Map Assistance 1.3.1+rev.1

## Installation

Close the game and loader, back up existing DLLs, and extract the desired ZIPs into the game directory for a global installation. Preserve existing configuration and keep only one active copy of each plugin. Debug logging defaults remain off. Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

Target: Windows x64, D2RLoader 1.3.1 / ABI4; SDK pinned to v0.3.0. Native features retain build-specific guards. See the bundled rev.48 production record for compatibility limits. Hot reload is unsupported.
