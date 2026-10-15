# Map Assistance 1.3.1+rev.1

By PyreFly. Windows x64 and D2RLoader 1.3.1 / plugin ABI 4.

## Install or update

Close the game and back up the old DLL. Extract into the game directory or
active mod directory so the DLL lands in `d2rloader/plugins/`. Keep one active
copy and restart through D2RLoader. The D2RR launcher can remain open.
For managed mod packs, use their normal updater.

The `defaults/` TOMLs are references. Existing `d2rloader/config/` settings
are preserved; missing main configs are created on first launch. Edit active
configs with the game closed and restart after manual changes. To uninstall,
close the game, remove this DLL and restart.

## Use and configure

Hover a normal Keys stack outside town for map-reading advice. No chat
commands. Tips are static; they do not reveal unexplored rooms or the map seed.

Edit existing entries in `map-assistance.toml`. Example:

```toml
[[zones]]
map_id = 29
name = "Jail Level 1"
layout = "Semi-static"
lines = ["From the waypoint, turn left for Level 2."]
```

Replace the existing entry for ID 29 instead of duplicating it. New unique
positive IDs support mod-added areas. Set `[map-assistance] enabled = false`
to disable tips. Modded maps may need different advice.

Map-reading guidance is inspired by
[Kryszard's PD2 Loot Filter](https://github.com/Kryszard-POD/Kryszard-s-PD2-Loot-Filter).
This is an independent adaptation, unaffiliated with that project.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/map-assistance/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
