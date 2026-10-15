# Floating Damage 1.3.1+rev.12

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

Displays nearby hostile enemies' observed HP loss in local single player.
Numbers include damage from any attacker. Matched elements use colors;
Critical/Deadly Strike can use larger text, `!!`, or both. Remote multiplayer
clients are unsupported.

| Chat command | Effect |
| --- | --- |
| `/damagenumbers on` | Enable numbers for this session. |
| `/damagenumbers off` | Hide numbers and clear tracking. |
| `/damagenumbers status` | Report enabled state. |
| `/damagenumbers motion on` | Use rise, fall and sideways drift. |
| `/damagenumbers motion off` | Use straight-rise animation. |

Chat changes are session-only. For startup behavior and styling, edit existing
keys in `floating-damage.toml`:

```toml
[floating-damage]
enabled = true
text_size = 26                 # Base size at 1920x1080
resolution_scaling = true      # 4K uses 2x scaling
critical_style = "both"        # "suffix", "size", or "both"
critical_size_scale = 1.35      # Critical text multiplier
falling_motion = true          # false gives straight rise
```

Slash commands are entered in game chat; feedback appears in the loader console.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/floating-damage/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
