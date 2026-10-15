# Map Assistance

Map Assistance is a small D2RLoader proof-of-concept inspired by the Keys-stack
map-reading notes in Kryszard's Project Diablo 2 loot filter. Hover a normal
`Keys` stack while outside town to see a short tip for the current campaign
area. All zone text is editable in `d2rloader/config/map-assistance.toml`.

The plugin does not reveal the generated map, inspect unexplored rooms, change
drops, or implement a loot filter. It displays static LoD tile-reading guidance
for selected areas using public D2RLoader services only.

## Current scope

- Normal `key` items in any supported inventory tooltip surface.
- All 127 non-town vanilla/LoD level ids from 2 through 132 across Acts I-V.
- Towns intentionally add no text; unknown levels fail open unless configured.
- No native hooks, RVAs, external runtime, or item mutation.

Directional guidance is included where a stable tile-reading rule exists.
Static and genuinely random zones are labeled without inventing a route.
Item-mod highlighting remains future work.

## Credit

Map Assistance exists because of the excellent map-reading work assembled by
Kryszard for [Kryszard's PD2 Loot Filter](https://github.com/Kryszard-POD/Kryszard-s-PD2-Loot-Filter).
That project made identifying, organizing, and presenting this guidance
exceptionally straightforward. This plugin is an independent D2RLoader
adaptation and is not affiliated with or endorsed by Kryszard or Project
Diablo 2. See [CREDITS.md](CREDITS.md) for the full attribution.

## Configuration

D2RLoader creates `d2rloader/config/map-assistance.toml` from the default
embedded in the DLL when the file is missing. It does not replace an existing
file, so local edits survive plugin updates. Restart D2RLoader after editing it.

Current runtime ZIPs contain `defaults/map-assistance.toml` as a reference only,
with no live `d2rloader/config/` paths. Extracting an update preserves your file;
compare and copy individual new settings if wanted instead of replacing it.

Each `[[zones]]` entry uses the numeric Levels.txt id for matching while keeping
the display name human-readable:

```toml
[map-assistance]
enabled = true

[[zones]]
map_id = 29
name = "Jail Level 1"
layout = "Semi-static"
lines = [
  "From entrance: left finds the waypoint; straight finds level 2.",
  "From the waypoint, turn left for level 2."
]
```

New map ids can be appended with the same fields. Duplicate or malformed
entries are skipped and reported in the loader log. At least one valid zone is
required; limits are 1 MiB per file, 1,024 zones, eight lines per zone, and 256
characters per line. The parser intentionally supports this documented TOML
subset rather than depending on an external TOML library.

## Build

```powershell
cmake -S plugins/map-assistance -B build/map-assistance -A x64
cmake --build build/map-assistance --config Release
ctest --test-dir build/map-assistance -C Release --output-on-failure
```

Copy `build/map-assistance/Release/Map Assistance.dll` into
`d2rloader/plugins/`.

## Live validation

1. Start a game with a Keys stack in the player inventory.
2. Enter Tower Cellar Level 1 and hover Keys. Confirm the tooltip names the
   area and says to turn left for the next level.
3. Take the waypoint to Durance of Hate Level 2 and hover Keys. Confirm the
   waypoint-specific note appears.
4. Return to town or enter an unsupported area. Confirm no stale area tip is
   shown.
5. Confirm the original name, quantity, and all normal item interactions remain
   unchanged.

Loader admission and visible Keys tips in Tower Cellar Level 1 and Jail Level 1
were user-confirmed on 2026-09-26. Other areas remain unvalidated in game.

Version 1.3.1+rev.1 includes the embedded D2RLoader ABI manifest required for loader
admission. The artifact test verifies that resource against the SDK ABI before
the DLL is shipped and also verifies the embedded default TOML resource.
