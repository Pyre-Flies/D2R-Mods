# Floating Damage

**1.3.1+rev.12 · by PyreFly**

Displays numbers when nearby hostile enemies lose HP in local single player,
including confirmed killing blows. Numbers reflect observed HP loss from any
attacker. Element colors and Critical/Deadly Strike emphasis appear when the
damage can be matched reliably; uncertain damage uses the default color.

## Chat commands

Enter these in game chat. Feedback appears in the loader console. They also
work there without the leading `/`.

| Command | What it does |
| --- | --- |
| `/damagenumbers` | Toggle numbers for this session. |
| `/damagenumbers on` | Enable numbers. |
| `/damagenumbers off` | Disable numbers and clear current tracking. |
| `/damagenumbers status` | Report enabled state. |
| `/damagenumbers motion on` | Use rise, fall and sideways drift. |
| `/damagenumbers motion off` | Use a straight upward animation. |
| `/damagenumbers motion status` | Report the current animation mode. |

Chat toggles are session-only. Choose startup behavior in
`d2rloader/config/floating-damage.toml`, then restart.

## Configuration examples

Change existing keys under `[floating-damage]`; the reference TOML lists all
available settings:

```toml
[floating-damage]
enabled = true
text_size = 26                 # Base pixels at 1920x1080; accepts 12-64
resolution_scaling = true      # Scale size/motion with resolution; 4K uses 2x
element_colors = true          # Color matched damage by element
text_color = "#D5DAE2"          # Fallback color; all numbers if colors disabled
critical_hits = true
critical_style = "both"        # "suffix" = !!, "size" = larger, "both" = both
critical_size_scale = 1.35      # Size multiplier for matched critical hits
falling_motion = true          # false gives a straight rise
lifetime_ms = 1000             # Animation duration in milliseconds
tick_merge_ms = 120            # Merge rapid small ticks; 0 keeps them separate
```

Critical and Deadly Strike share the same emphasis; the plugin does not
distinguish them or multiply HP loss. Colors accept quoted `#RRGGBB` values.
Outline, shadow and poison backing can also be adjusted in the reference file.

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

Requires Windows x64 and the exact D2RLoader/Core profile in
`COMPATIBILITY.txt`. Remote multiplayer clients are unsupported. Controller QOL
is optional. Unsupported builds leave the feature unavailable.

Live checks cover ordinary/killing numbers, movement, commands, element/poison
colors and critical-marker controls. Seven automated suites passed. The latest
4K scaling and three critical-style modes still need live visual checks;
dense-combat performance and every camera/resolution combination are not fully
validated. Closely spaced damage and healing can combine between observations.

For troubleshooting only, `/damagenumbers capture on` records up to 128
diagnostic entries in the plugin log. `/damagenumbers capture off` stops it;
`/damagenumbers capture status` reports its state. Capture stops at game exit.

For requirements, see the included compatibility file. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` lists the packaged file checksums.
