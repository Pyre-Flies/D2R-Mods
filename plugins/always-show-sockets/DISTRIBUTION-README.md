# Always Show Sockets 1.3.1+rev.1

Style empty and filled sockets, either on native hover/selection or continuously.
Loose gems, runes and jewels keep their normal size and opacity.

## Install

Close Diablo II: Resurrected and D2RLoader. Back up an existing
`d2rloader/plugins/Always Show Sockets.dll`, then extract into the active directory
containing `d2rloader/`. The runtime ZIP installs exactly one DLL. The TOML under
`defaults/` is a reference; it does not overwrite the active configuration.

Requires Windows x64, D2RLoader 1.3.1 / plugin ABI 4 and the exact executable/Core hashes
in `COMPATIBILITY.txt`. Unsupported builds preserve native behavior. Restart to
install, update or remove the DLL; native callbacks prevent safe hot reload.

## Configure

See [the quick configuration guide](CONFIGURATION-GUIDE.md) for the full table.

| Command | Behavior |
| --- | --- |
| `/sockets default` | Native hover/selection visibility with your appearance settings. Startup mode. |
| `/sockets always` | Show styled sockets continuously. |
| `/sockets off` | Restore native visibility and appearance. |
| `/sockets size 65` | Artwork size, 10-100; ring size follows at +15 points. |
| `/sockets ring 80` | Ring outline opacity, 0-100. |
| `/sockets center 40` | Black center opacity, 0-100. |
| `/sockets opacity 65` | Inserted artwork opacity relative to native fade, 10-100. |
| `/sockets lift 1` | Upward artwork lift in rendered pixels, 0-8. |
| `/sockets status` | Write the current mode and values to the loader debug console. |

Appearance commands apply immediately and save automatically to
`d2rloader/config/always-show-sockets.toml`. They preserve other settings and
comments. Saved appearance values are loaded on the next start. If saving fails,
the session change remains active and the console/log reports the failure.

Modes are session-only: every launch starts in `default`. Changing mode keeps
appearance values. Bare `/sockets` or `toggle` disables/resumes the selected styled
mode; `/sockets on` selects always. Commands also work in the debug console
without `/`, and operate locally. Manual config edits require a restart.

The default size 65 gives 80% circles; size 100 gives native-size artwork and
115% circles. Ring and center size move together, while their opacity controls
are independent. Ring 80 means 80% opaque, center 40 means 40% black. The lift
moves inserted artwork only. The DLL generates its original ring textures;
no separate runtime assets are installed. Unavailable optional resources/guards
retain native circles and prior artwork-centering fallback.

## Validation and limits

User-confirmed coverage includes empty/filled/mixed sockets, inventory/equipment,
Cube, personal/shared stash, artwork scaling/opacity, normal loose items, paired
alignment, 1-pixel lift, full-size rings and default/always/off mode transitions.
The requested 80/40/65 appearance was exercised in the accepted beta.13 session.

Four native suites and eight packaging checks pass. Autosave tests exercise the
SDK read/write boundary, saved values, comment/section preservation, rejected
commands and save failures. This staged artifact has not been separately installed
or live-tested for actual loader-backed save/restart behavior.

Quality Backgrounds worked with earlier beta.2/beta.4 versions. The established
compatibility guards remain, but later ring/mode revisions have no new combined
live coverage. Vendors, trade, controller-specific behavior, classic graphics and
resolution/scaling changes remain unverified. Classic graphics retain native size
and opacity. No item data or interaction state is edited.

To roll back, close the game and remove this DLL or restore your previous copy.
