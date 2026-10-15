# Always Show Sockets

**1.3.1+rev.2 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Style empty and filled sockets on normal hover/selection, or keep them visible
continuously. Inserted gems, runes and jewels have adjustable appearance;
loose items keep their normal size and opacity. Appearance controls apply to
HD graphics; classic graphics retain native size and opacity.

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

Enter commands in game chat; feedback appears in the loader console. They also
work in that console without `/`. Use the lowercase arguments shown here.

| Command | Effect |
| --- | --- |
| `/sockets default` | Save styled sockets on normal hover/selection. Fresh-install default. |
| `/sockets always` | Save continuous styled socket display. |
| `/sockets on` | Alias for `always`; selects continuous styled display. |
| `/sockets off` | Save native visibility and appearance. |
| `/sockets` | Toggle off/on, retaining and saving the last styled mode. |
| `/sockets toggle` | Explicit form of the bare toggle. |
| `/sockets size 65` | Save inserted artwork size, integer 10-100%. |
| `/sockets opacity 65` | Save inserted artwork opacity relative to native fade, integer 10-100%. |
| `/sockets ring 80` | Save ring opacity, integer 0-100%. |
| `/sockets center 40` | Save black-center opacity, integer 0-100%. |
| `/sockets lift 1` | Save upward artwork lift, integer 0-8 rendered pixels. |
| `/sockets status` | Report mode and all appearance values; no config write. |

Every setting command saves to the active
`d2rloader/config/always-show-sockets.toml` and restores on restart. Modes keep
your appearance values. `off` followed by the bare toggle resumes the last
styled mode; `on` explicitly selects `always`. A save failure is reported and
the change remains temporary until saving succeeds. Status/invalid commands
do not save.

## All configuration settings

All keys belong under `[always-show-sockets]`:

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | false restores native visibility/appearance; true enables the saved styled mode. |
| `always_visible` | `false` | When enabled: true shows sockets continuously; false uses normal hover/selection. Retained while off. |
| `inserted_item_scale_percent` | `65` | Inserted artwork size, integer 10-100%. Circles use this +15 points, up to 115%. |
| `inserted_item_opacity_percent` | `65` | Opacity relative to native fade, integer 10-100%; 100 keeps native opacity. |
| `inserted_item_lift_pixels` | `1` | Move inserted artwork upward, integer 0-8 rendered pixels; ring/center stay fixed. |
| `ring_opacity_percent` | `80` | Ring outline opacity, integer 0-100%. Zero hides it. |
| `ring_center_opacity_percent` | `40` | Black center opacity, integer 0-100%. Zero makes the center clear. |

Ring size and opacity are different properties. Artwork size 65 gives 80%
circles; size 100 gives native-size artwork with 115% circles. Ring/center
size move together, while their opacity controls are independent. Ring 80 is
80% opaque; center 40 is 40% black. Lift moves inserted artwork only.

## Editing examples

Apply the recommended appearance immediately and save it:

```text
/sockets default
/sockets size 65
/sockets opacity 65
/sockets ring 80
/sockets center 40
/sockets lift 1
```

Or close the game and edit the active TOML, then restart:

```toml
[always-show-sockets]
enabled = true
always_visible = false
inserted_item_scale_percent = 65
inserted_item_opacity_percent = 65
inserted_item_lift_pixels = 1
ring_opacity_percent = 80
ring_center_opacity_percent = 40
```

Set `always_visible = true` for continuous styled display. Set `enabled = false`
for native appearance. Edit existing keys rather than appending a second copy.
See the [reference TOML](always-show-sockets.toml) for the shipped values.

## Compatibility and validation

Requires the exact native profile in [COMPATIBILITY.txt](COMPATIBILITY.txt).
Unsupported builds preserve native behavior. Four automated suites passed,
including retained chat dispatch, saved mode reload and save-failure checks.
Prior live checks cover inventory/equipment, Cube, personal/shared stash and
mixed empty/filled sockets. Actual loader-backed save/restart, the latest
Quality Backgrounds combination, vendors, trade and broader controller/display
coverage still need live confirmation.

## Support

Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues)
with the plugin/game/mod versions and steps to reproduce. Check logs for
personal information before sharing. Download checksums are alongside the ZIPs.
