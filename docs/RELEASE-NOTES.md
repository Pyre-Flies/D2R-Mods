## Controller QOL 1.3.1+rev.61

- Apply `block_filtered_pickup = true` only while D2R's controller UI is active.
  Mouse and keyboard pickup works with item placards off, including when an idle
  controller remains connected.
- Preserve native pickup when the guarded controller state is unavailable.
  Controller filtering retains its existing visible-label policy.

## Installation

Extract the wanted runtime ZIP into the game directory. DLLs install under
`d2rloader/plugins/`; supplied configs install under `d2rloader/config/`.
Close the game and loader first, back up existing files, and preserve customized
TOML files. Keep one active copy of each plugin and restart after updates.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

## Validation and compatibility

Local Release builds and automated suites passed: Controller QOL 23/23,
Item Roll Ranges 5/5, and Map Assistance 2/2. The user confirmed mouse pickup
with placards off using the installed fix. Controller filtering and switching
between input modes still require live validation.

Target: Windows x64, D2RLoader 1.3.1 / ABI 4 and the qualified D2RCore/game
build listed in the bundled compatibility notes. Hot reload is unsupported.

Included unchanged: Item Roll Ranges 1.3.1+rev.21 and Map Assistance
1.3.1+rev.1. This release changes Controller QOL only.
