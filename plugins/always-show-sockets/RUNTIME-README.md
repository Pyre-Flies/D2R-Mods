# Always Show Sockets 1.3.1+rev.1

By PyreFly. Windows x64 and D2RLoader 1.3.1 / plugin ABI 4. Native features require the supported game/Core build; unsupported paths retain native behavior.

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

Style sockets on hover/selection or keep them visible continuously. Loose
gems, runes and jewels retain normal size and opacity. Appearance controls
apply to HD graphics.

| Chat command | Effect |
| --- | --- |
| `/sockets default` | Styled sockets on normal hover/selection; startup mode. |
| `/sockets always` | Keep styled sockets visible for this session. |
| `/sockets off` | Restore native visibility and appearance. |
| `/sockets size 65` | Artwork size, 10-100%; circles follow at +15 points. |
| `/sockets opacity 65` | Artwork opacity relative to native fade, 10-100%. |
| `/sockets ring 80` | Ring opacity, 0-100%. |
| `/sockets center 40` | Black-center opacity, 0-100%. |
| `/sockets lift 1` | Move artwork upward 0-8 rendered pixels. |
| `/sockets status` | Report mode and appearance values. |

Appearance changes save to `always-show-sockets.toml`. Modes are session-only;
each launch starts in default mode. A save warning means the appearance change
applies only to the session until the config problem is corrected.

Slash commands are entered in game chat; feedback appears in the loader console.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/always-show-sockets/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
