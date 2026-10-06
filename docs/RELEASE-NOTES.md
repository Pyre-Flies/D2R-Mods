## Controller QOL 1.3.1+rev.62

- Refresh normalized controller input and ground-label display on the client UI
  scheduler, which is available to remote TCP/IP clients. Previously these
  refreshes depended on the authoritative game scheduler, unavailable remotely.
- Keep the controller-only label filter separate from ground-modifier handling.
  Mouse pickup remains unaffected by filtered-label blocking.
- The updated DLL was tested offline and reported working by the user.

Remote Ladder validation is pending. This update may improve LB input detection
and labels; it does not add remote support for direct ground-loot or SDK item
mutations that require host authority. Signed Ladder packages must include this
DLL through their normal revision process.

## Installation

Extract the wanted runtime ZIP into the game directory. DLLs install under
`d2rloader/plugins/`; supplied configs install under `d2rloader/config/`.
Close the game and loader first, back up existing files, and preserve customized
TOML files. Keep one active copy of each plugin and restart after updates.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

## Validation and compatibility

Local Release builds and automated suites: Controller QOL 23/23,
Item Roll Ranges 5/5, Map Assistance 2/2. Offline behavior was user-confirmed
with the UI scheduler update; remote Ladder behavior remains unverified.

Target: Windows x64, D2RLoader 1.3.1 / ABI 4 and the qualified D2RCore/game
build listed in the bundled compatibility notes. Hot reload is unsupported.

Included unchanged: Item Roll Ranges 1.3.1+rev.21 and Map Assistance
1.3.1+rev.1. This release changes Controller QOL only.
