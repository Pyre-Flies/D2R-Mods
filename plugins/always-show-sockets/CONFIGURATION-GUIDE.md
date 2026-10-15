# Always Show Sockets — quick configuration

Configure socket visibility and the appearance of inserted gems, runes and
jewels from game chat. Loose items keep their normal size and opacity.

## Choose when sockets appear

| Command | Behavior |
| --- | --- |
| `/sockets default` | Normal hover/selection visibility, with your appearance settings. **Fresh-install default.** |
| `/sockets always` | Show styled sockets continuously. |
| `/sockets off` | Restore native visibility and appearance. |

Switching modes keeps your appearance settings. Bare `/sockets` toggles the
selected styled mode off/on. `/sockets on` is an alias for `always`.

## Adjust the appearance

| Command and default | Range | Changes |
| --- | --- | --- |
| `/sockets size 65` | 10–100 | Inserted artwork size. Ring and center size follow at +15 points: size 65 gives 80% circles; size 100 gives 115%. |
| `/sockets ring 80` | 0–100 | Ring outline opacity. Zero hides the outline. |
| `/sockets center 40` | 0–100 | Black center opacity. Zero makes the center clear. |
| `/sockets opacity 65` | 10–100 | Inserted artwork opacity, relative to its native fade. 100 keeps native opacity. |
| `/sockets lift 1` | 0–8 | Move inserted artwork upward by whole rendered pixels. Ring and center stay fixed. |

Opacity values mean percent opaque: ring 80 is 80% opaque, and center 40 is
40% black. Ring opacity and ring size are separate properties.
Use `/sockets status` to write the current mode and values to the loader's
debug console. All commands also work there without the leading `/`.

To apply the recommended settings in an existing session:

```text
/sockets default
/sockets size 65
/sockets ring 80
/sockets center 40
```

## Automatic saving

Visibility modes, `size`, `opacity`, `ring`, `center`, and `lift` apply immediately and save
automatically to the active loader's `d2rloader/config/always-show-sockets.toml`.
They update only the changed setting and retain the other values and comments.
Saved appearance values load on the next start.

Visibility modes persist too. Each launch restores your saved choice. Fresh
installs use `/sockets default`. Off/on toggles retain the last styled mode
(default or always). Status commands do not write configuration.

For manual editing, close the game, edit these keys, then restart:

```toml
[always-show-sockets]
enabled = true          # false restores native visibility/appearance
always_visible = false  # true keeps styled sockets visible when enabled
inserted_item_scale_percent = 65
inserted_item_opacity_percent = 65
inserted_item_lift_pixels = 1
ring_opacity_percent = 80
ring_center_opacity_percent = 40
```

The loader creates a missing config from the embedded defaults. The package's
`defaults/` TOML is a reference and does not overwrite the active config.
If saving fails, the current appearance still applies for the session and a
warning appears in the debug console and plugin log. Correct the config problem
and repeat the appearance command to save it.

The appearance controls apply to HD graphics. Restart the game after installing,
updating or removing the DLL; do not hot-reload it.
