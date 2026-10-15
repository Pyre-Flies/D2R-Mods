# Map Assistance

**1.3.1+rev.1 · by PyreFly**

Hover a normal **Keys** stack outside town to see map-reading tips for the
current area. Guidance is static: it does not reveal unexplored rooms or the
current map seed. Towns and unconfigured areas add no text.

## Customize the tips

There are no chat commands. Edit `d2rloader/config/map-assistance.toml` with the
game closed, then restart. Change the existing `[[zones]]` entry for an area;
do not add a second entry with the same ID. Example:

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

`map_id` is the area's Levels-table ID. `name` and `layout` label the tip;
`lines` contains the displayed advice. Add a unique positive ID to support a
mod-added area. Set `enabled = false` to disable guidance. The included reference
TOML covers 127 non-town vanilla/LoD areas across Acts I-V; modded layouts can
differ, so adapt the text to your pack.

## Install, update or remove

1. Close the game. The D2RR launcher may remain open.
2. Back up the previous DLL, then extract the ZIP into the game directory for
   a global install, or the active mod directory for a mod-specific install.
   The DLL belongs in `d2rloader/plugins/`. Keep one active copy per plugin.
3. Start the game through D2RLoader.

For a launcher-managed mod pack, use its normal update process. The `defaults/`
files are references: updates preserve your active `d2rloader/config/` settings.
The loader creates a missing main configuration on first launch. Edit active
configuration files with the game closed, then restart to apply manual changes.

To remove or roll back, close the game and remove the plugin DLL or restore its
backup. Restart after any DLL change; hot reload is unsupported.

## Compatibility and limits

Requires Windows x64 and D2RLoader plugin ABI 4 with item-tooltip/configuration
services. No other plugin is required. Two automated suites passed. Visible
Keys tips and configuration loading were confirmed in game; the remaining
area tips have not each been individually checked. See `COMPATIBILITY.md`.

Based on Kryszard's PD2 Loot Filter map-reading notes. Attribution and the
original project link are in [CREDITS.md](CREDITS.md).

For requirements, see the repository compatibility record. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` is provided alongside the download ZIPs.

Runtime ZIPs contain only the DLL, reference defaults and a usage README.
Detailed guides and compatibility records stay in this repository.
