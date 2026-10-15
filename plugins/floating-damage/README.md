# Floating Damage

**1.3.1+rev.13 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Displays observed HP loss above nearby hostile enemies in local single player,
including confirmed killing blows. Numbers include damage from any attacker.
They show HP actually lost, rather than advertised attack damage or overkill.
Remote multiplayer clients are unsupported.

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

## All chat commands

Enter commands in game chat. Feedback appears in the loader console; the same
commands work there without the leading `/`. Command arguments are case-insensitive.

| Command | Effect |
| --- | --- |
| `/damagenumbers` | Toggle numbers and save the new state. |
| `/damagenumbers toggle` | Explicit toggle, also saved. |
| `/damagenumbers on` | Save `enabled = true`. |
| `/damagenumbers off` | Save `enabled = false` and clear current tracking. |
| `/damagenumbers status` | Report enabled state; no config write. |
| `/damagenumbers motion` | Toggle animation mode and save it. |
| `/damagenumbers motion toggle` | Explicit animation toggle, also saved. |
| `/damagenumbers motion on` | Save rise/fall/sideways-drift animation. |
| `/damagenumbers motion off` | Save straight-rise animation. |
| `/damagenumbers motion status` | Report animation mode; no config write. |
| `/damagenumbers capture` | Toggle a bounded diagnostic recording. |
| `/damagenumbers capture toggle` | Explicit recording toggle. |
| `/damagenumbers capture on` | Start up to 128 diagnostic log records. |
| `/damagenumbers capture off` | Stop recording. |
| `/damagenumbers capture status` | Report recording state. |

Enabled/motion settings save to the active
`d2rloader/config/floating-damage.toml` and restore on restart. A failed save
leaves the previous setting active and reports a warning. Diagnostic capture is
a recording action: it stops at its limit or game exit and is not a startup
preference. Invalid/status commands do not write configuration.

## All configuration settings

Every key below belongs under `[floating-damage]` in the active TOML. Manual
edits require a restart. Colors accept quoted `"#RRGGBB"`; legacy RGB arrays
with three values from 0 to 1 remain readable. Defaults are shown below.

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Show numbers; changed and saved by the main chat command. |
| `debug` | `false` | Enable extra diagnostics; leave false for normal play. |
| `resolution_scaling` | `true` | Scale text/geometry/motion from 1920x1080; 4K is 2x. False keeps fixed pixel sizes. |
| `text_size` | `26` | Base font size, 12-64 pixels. |
| `critical_hits` | `true` | Emphasize matched Critical/Deadly Strike events. |
| `critical_style` | `"both"` | "suffix" adds !!; "size" enlarges; "both" does both. |
| `critical_size_scale` | `1.35` | Critical text multiplier, 1-2; affects size/both styles. |
| `text_color` | `"#D5DAE2"` | Fallback color, or every number when element_colors is false. |
| `element_colors` | `true` | Color reliably matched damage by element. |
| `physical_color` | `"#F1E3CA"` | Physical damage color. |
| `fire_color` | `"#FF8B68"` | Fire damage color. |
| `cold_color` | `"#8DD8FF"` | Cold damage color. |
| `lightning_color` | `"#F6DC78"` | Lightning damage color. |
| `magic_color` | `"#B6A1F5"` | Magic damage color. |
| `poison_color` | `"#C6F69B"` | Poison damage color. |
| `mixed_color` | `"#F58CB6"` | Color for matched damage containing several elements. |
| `outline_color` | `"#101215"` | Outline color. |
| `outline_width` | `2` | Base outline thickness, integer 0-3; 0 disables. |
| `fade_start` | `0.8` | Fraction of lifetime before fading begins, 0-0.95. |
| `lifetime_ms` | `1000` | Number lifetime, integer 250-3000 milliseconds. |
| `rise_pixels` | `64` | Upward distance, 0-300 base pixels. |
| `falling_motion` | `true` | Rise/fall/sideways drift when true; straight rise when false. Saved by motion commands. |
| `side_pixels` | `48` | Sideways drift distance, 0-200 base pixels. |
| `fall_pixels` | `96` | Downward distance, 0-300 base pixels. |
| `shadow` | `true` | Enable the text shadow. |
| `shadow_color` | `"#000000"` | Shadow color. |
| `shadow_offset_x` | `4` | Horizontal shadow offset, -20 to 20 base pixels. |
| `shadow_offset_y` | `4` | Vertical shadow offset, -20 to 20 base pixels. |
| `shadow_opacity` | `0.75` | Shadow opacity, 0-1. |
| `shadow_softness` | `1` | Shadow softness, integer 0-2. |
| `backing` | `true` | Draw a translucent backing behind eligible numbers. |
| `backing_poison_only` | `true` | Limit backing to poison and mixed numbers containing poison; false backs all numbers. |
| `backing_color` | `"#0A0D10"` | Backing color. |
| `backing_opacity` | `0.68` | Backing opacity, 0-1. |
| `backing_padding_x` | `4` | Horizontal padding, 0-12 base pixels. |
| `backing_padding_y` | `1` | Vertical padding, 0-12 base pixels. |
| `backing_rounding` | `3` | Corner radius, 0-12 base pixels. |
| `anchor_offset_pixels` | `46` | Offset above the projected target, 0-200 base pixels. |
| `lane_spacing_pixels` | `22` | Spacing between number lanes, 10-80 base pixels. |
| `tick_merge_ms` | `120` | Merge rapid decreases of at most 2 HP, integer 0-300 ms; 0 separates them. |
| `decimal_places` | `0` | Integer 0-2 decimal places. |
| `max_numbers` | `128` | Maximum simultaneous numbers, integer 1-128. |

Resolution scaling changes font size, outline/shadow/backing, offsets and
motion together. Timing, opacity and HP values do not scale. The uniform factor
uses the smaller width/height ratio relative to 1920x1080, bounded to 0.25-4.
Set `resolution_scaling = false` for fixed pixel sizes.

Critical and Deadly Strike share one emphasis; the plugin does not distinguish
them or multiply the HP loss. Reliable element matches use the configured
colors; uncertain damage uses `text_color`. Tiny positive amounts display `<1`
or the corresponding decimal unit when below the selected precision.

## Editing examples

Example: smaller numbers, straight-rise motion, no backing and suffix-only
critical emphasis. Change these existing keys in the active file:

```toml
[floating-damage]
enabled = true
text_size = 22
falling_motion = false
backing = false
critical_hits = true
critical_style = "suffix"
```

Example: make all numbers white and separate rapid small ticks:

```toml
[floating-damage]
element_colors = false
text_color = "#FFFFFF"
tick_merge_ms = 0
decimal_places = 1
```

Merge examples into the existing section; do not add duplicate headers/keys.
The [complete reference TOML](floating-damage.toml) contains all defaults.

## Compatibility and validation

Requires the supported native profile in [COMPATIBILITY.txt](COMPATIBILITY.txt).
Controller QOL is optional. Unsupported builds leave the feature unavailable.
Closely spaced damage/healing can combine between observations. No attacker
identification, DPS meter or live reload of manual edits is provided.

Eight automated suites passed, including saved TOML/reload and production
command/error checks. Prior live coverage includes numbers, movement, element
colors, poison and critical-marker controls. The latest 4K/style visuals and
actual loader-backed command-save/restart still need live confirmation.

## Support

Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues)
with the plugin/game/mod versions and steps to reproduce. Check logs for
personal information before sharing. Download checksums are alongside the ZIPs.
