## Item Roll Ranges 1.3.1+rev.21

- Keep actual Defense and physical damage values and show the underlying base:
  `Two-Hand Damage: 32 to 69 (Base: 9 - 19)` and Bloodrune
  `Defense: 25 (Base: 2 - 2)`. Armor and belts use the same display.
- Make fixed ranges explicit. Account for ethereal base scaling and native
  Enhanced Defense base generation. Keep modifier roll ranges on their own lines.
- Show `(?) Range Unavailable - Please report item affixes` for unresolved
  expected ranges. Unrelated proc/per-level modifiers do not block a known base.
- Follow Core's native input decision without keyboard/controller query
  overrides or physical input polling: Ctrl and RT/R2 normally; Controller QOL's
  existing active menu/range remap selects R1/RB. No QOL update is required.
- Preserve exact compatibility guards, original formatter values and slot
  ownership checks. Other plugins changing the same tooltip functions can
  still conflict; unsupported paths fail open.

## Installation

Extract the wanted runtime ZIP into the game directory. DLLs install under
`d2rloader/plugins/`; supplied configs install under `d2rloader/config/`.
Close the game and loader first, back up existing files, and preserve customized
TOML files. Keep one active copy of each plugin and restart after updates.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

## Validation and compatibility

Local automated suites: Controller QOL23/23, Item Roll Ranges5/5 and Map
Assistance2/2. The Base display was user-confirmed on tested belts and bows.
Revision21's native input/coexistence path passed automated context tests;
dedicated live controller checks remain pending. These checks do not establish
compatibility with every item family, localization or plugin combination.

Target: Windows x64, D2RLoader1.3.1 / ABI4 and the qualified D2RCore/game build
listed in the bundled compatibility notes. Hot reload is unsupported.

Included unchanged source versions: Controller QOL1.3.1+rev.60 and Map
Assistance1.3.1+rev.1. This release changes Item Roll Ranges only.
