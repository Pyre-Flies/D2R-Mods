# Map Assistance

**1.3.1+rev.1 · by PyreFly · Windows x64 / D2RLoader plugin ABI 4**

Hover a normal Keys stack outside town for map-reading tips for the current
area. Guidance is static and does not reveal unexplored rooms or the map seed.
Towns and unconfigured areas add no text. There are no chat commands.

## Installation and updates

1. Close the game and back up the previous plugin DLL and active configuration.
   The D2RR launcher can remain open.
2. Extract the ZIP into the game directory for global use, or the active mod
   directory for mod-specific use. The DLL belongs in `d2rloader/plugins/`.
   Keep one active copy of each plugin. Managed mod packs should use their
   normal updater.
3. Start through D2RLoader. Missing main configs are created automatically in
   the active `d2rloader/config/` directory.

Files under `defaults/` are references. Extracting an update preserves existing
settings; editing a reference does not change gameplay. Edit the active file
with the game closed, change existing keys instead of duplicating them, and
restart to apply manual edits. TOML booleans are lowercase `true`/`false`,
strings use quotes, and `#` starts a comment outside a quoted string.

To uninstall or roll back, close the game, remove the DLL or restore its backup,
then restart. Hot reload is unsupported.

## All configuration settings

Edit `d2rloader/config/map-assistance.toml` with the game closed, then restart.

| Section / key | Default or accepted value | Meaning |
| --- | --- | --- |
| `[map-assistance] enabled` | `true`; boolean | Enable area tips. false disables them. |
| `[[zones]] map_id` | Positive Levels-table ID, at most 2147483647 | Identifies the area. Use a unique ID per entry. |
| `[[zones]] name` | Nonempty quoted string, at most 96 bytes | Area name displayed in the tooltip. |
| `[[zones]] layout` | Nonempty quoted string, at most 64 bytes | Display label such as "Tile-based" or "Semi-static". |
| `[[zones]] lines` | Array of 1-8 nonempty quoted strings, at most 256 bytes each | Advice lines displayed beneath the heading. |

The default file includes 127 non-town vanilla/LoD areas across Acts I-V. Modded
layouts can differ, so edit the advice for your pack. A file may contain up to
1024 entries and is limited to 1 MiB; each entry's combined name/layout/line
text is limited to 900 bytes. Malformed/duplicate entries are skipped and logged.
At least one valid entry is needed. Keep the documented table/array syntax;
this plugin accepts a limited TOML subset.

## Editing examples

Find the existing `map_id = 29` entry and change it instead of appending a
duplicate. This example keeps the enabled flag and edits Jail Level 1 advice:

```toml
[map-assistance]
enabled = true

[[zones]]
map_id = 29
name = "Jail Level 1"
layout = "Semi-static"
lines = [
  "From entrance: left finds the waypoint; straight finds Level 2.",
  "From the waypoint, turn left for Level 2."
]
```

For a mod-added area, copy one `[[zones]]` block, replace its ID with the area's
actual Levels-table ID, and replace name/layout/advice. Verify that ID is not
already present. Keep other area blocks intact. Set `enabled = false` under
the existing `[map-assistance]` section to disable all tips, then restart.

The [full reference configuration](map-assistance.toml) includes every shipped
area and its default text.

## Compatibility, credit and validation

Requires item-tooltip/configuration services from a compatible D2RLoader
plugin ABI 4 build. No other plugin is required. See
[compatibility](COMPATIBILITY.md). Two automated suites passed. Visible Keys
tips and configuration loading were confirmed in game; not every area's advice
has been individually checked.

Inspired by the map-reading notes in
[Kryszard's PD2 Loot Filter](https://github.com/Kryszard-POD/Kryszard-s-PD2-Loot-Filter).
This is an independent adaptation, unaffiliated with that project or PD2.
See [CREDITS.md](CREDITS.md) for attribution.

## Support

Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues)
with the plugin/game/mod versions and steps to reproduce. Check logs for
personal information before sharing. Download checksums are alongside the ZIPs.
