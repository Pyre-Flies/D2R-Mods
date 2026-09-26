# Map Assistance 1.3.1+rev.1

Map Assistance adds configurable area-navigation guidance to the normal Keys
tooltip in Diablo II: Resurrected through D2RLoader.

## Install

1. Close Diablo II: Resurrected and D2RLoader.
2. Extract this archive into the Diablo II Resurrected installation folder.
3. Confirm `d2rloader/plugins/Map Assistance.dll` exists.
4. Start D2RLoader. It creates
   `d2rloader/config/map-assistance.toml` if that file is missing.

Existing configuration files are intentionally preserved. To adopt a newer
default configuration, compare your file with the standalone
`map-assistance.toml` included in this archive.

## Remove

Close the game and loader, then remove
`d2rloader/plugins/Map Assistance.dll`. The configuration may be retained for
a future reinstall or removed separately.

See `README.md` for configuration syntax, `COMPATIBILITY.md` for requirements
and validation limits, and `CREDITS.md` for attribution to Kryszard's PD2 Loot
Filter—the work that made this adaptation exceptionally straightforward.
