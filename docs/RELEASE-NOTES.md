## Controller QOL 1.3.1+rev.63

- Restore target acquisition around the armed reticle before the first snap-skill
  cast. An enemy behind the character can be acquired before the first Fireball
  turns the character, rather than only becoming available for the next shot.
- Preserve active ground-only and disabled-skill targeting gates and remaining
  native candidate checks. Idle native monster ranking changes deliberately.
- Keep bounded candidate/lookup diagnostics for future targeting investigations.

The user confirmed improved Fireball snapping, Teleport remaining ground-targeted,
and disabled aim skills retaining native targeting in offline play.

Rev.62's client UI scheduler update remains included. Remote Ladder validation
is pending; direct ground-loot and SDK item mutations still require host authority.
Signed Ladder packages must include the DLL through their normal revision process.

## Installation

Extract the wanted runtime ZIP into the game directory. DLLs install under
`d2rloader/plugins/`; supplied configs install under `d2rloader/config/`.
Close the game and loader first, back up existing files, and preserve customized
TOML files. Keep one active copy of each plugin and restart after updates.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

## Validation and compatibility

Local Release builds and automated suites: Controller QOL 23/23,
Item Roll Ranges 5/5, Map Assistance 2/2. Live checks cover the reported offline
Fireball sequence, Teleport, and disabled aim skills, not every spell or remote
Ladder session.

Target: Windows x64, D2RLoader 1.3.1 / ABI 4 and the qualified D2RCore/game
build listed in the bundled compatibility notes. Hot reload is unsupported.

Included unchanged: Item Roll Ranges 1.3.1+rev.21 and Map Assistance
1.3.1+rev.1. This release changes Controller QOL only.
