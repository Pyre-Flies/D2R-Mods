# Always Show Sockets

Always Show Sockets 1.3.1+rev.1 styles empty and filled sockets while keeping
loose gems, runes and jewels at their native size and opacity. Generated ring
artwork is embedded in the DLL; no separate runtime textures are needed.

See the [quick configuration guide](CONFIGURATION-GUIDE.md) for all commands.

## Display modes

- `/sockets default`: native hover/selection visibility with appearance settings.
  This is the startup mode on every launch.
- `/sockets always`: show styled sockets continuously.
- `/sockets off`: restore native visibility and appearance.

Modes are session-only and keep your appearance values. Bare `/sockets` or
`toggle` disables/resumes the selected styled mode. `/sockets on` selects always.

## Appearance and persistence

Defaults: artwork size 65%, ring size 80%, ring opacity 80%, black-center opacity
40%, inserted opacity 65%, and lift 1 rendered pixel. Ring size follows artwork
size +15 points, reaching 115% at artwork size 100. The ring and black center
share geometry; inserted artwork uses paired native sprite centers plus its lift.

`size`, `opacity`, `ring`, `center` and `lift` update immediately and save through
D2RLoader's config API to `d2rloader/config/always-show-sockets.toml`. Only the
changed key is updated; other values, comments, line endings and sections remain.
No visibility mode is written. If reading/writing the config fails, the session
change remains active and the loader console/log reports that it was not saved.

The loader creates a missing config from embedded defaults. Packages put their
reference TOML under `defaults/`, outside the live config directory. Manual edits
should be made with the game closed and are read on restart.

## Validation and compatibility

The user confirms the rendering, size/opacity controls, paired alignment,
1-pixel lift, full-size ring correction and default/always/off modes on the
admitted build. Empty/filled/mixed sockets, inventory/equipment, Cube and personal/
shared stash have prior user-confirmed coverage. Autosave is covered by automated
SDK config fixtures and parsing tests; this artifact still needs live save/restart
validation. Historical Quality Backgrounds combinations worked, but the later
ring/mode revisions have no new combined live coverage.

Existing hooks, guards, shared-loader admission, native eligibility and item data
are unchanged. Classic rendering keeps native appearance. Vendors, trade,
controller-specific behavior and resolution/scaling changes remain unverified.
See [native contracts](../../docs/reverse-engineering/always-show-sockets.md).

Install only with D2R/D2RLoader closed. Restart to install, update or remove;
native chat callbacks retain storage until process exit, so do not hot-reload.
This stable D2RLoader 1.3.1 release is staged locally for the combined release.
The exact executable/Core admission remains required.
