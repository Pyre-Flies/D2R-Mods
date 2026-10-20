# Always Show Sockets 1.3.1+rev.1

Style sockets without hiding the item artwork. Inserted gems, runes and jewels
can be scaled and faded independently of loose items. Rings follow artwork size
with a +15-point margin, with independent outline and black-center opacity.

- Startup mode is `/sockets default`: native hover/selection visibility with
  appearance settings. `/sockets always` shows styled sockets continuously;
  `/sockets off` restores native rendering. Modes remain session-only.
- Defaults: artwork size 65%, ring opacity 80%, black-center opacity 40%,
  inserted opacity 65%, and lift 1 pixel. Circle size is 80% at the default
  artwork size, reaching 115% at artwork size 100.
- `size`, `opacity`, `ring`, `center` and `lift` apply and save automatically to
  the active config. Other settings and comments are retained. Save failures
  are reported in the console/log while the session appearance remains usable.
- The included configuration guide covers commands and manual TOML editing.
  The runtime ZIP contains one DLL and a reference TOML under `defaults/`.
  Installing the package does not replace the active configuration.

Rendering, modes, controls and the chosen appearance have user-confirmed live
coverage on earlier candidates. Four native suites and eight packaging checks
pass, including autosave round trips and read/write failures through SDK fixtures.
Actual loader-backed saving and restart persistence need a live check on this
staged artifact. Existing native hooks, guards and renderer contracts are unchanged.

Quality Backgrounds worked with beta.2/beta.4. Later ring/mode revisions have no
new combined live coverage. Vendors, trade, controller-specific behavior, classic
graphics and resolution/scaling changes remain unverified.

Requires Windows x64, D2RLoader 1.3.1 / plugin ABI 4 and the exact executable/Core hashes
in `COMPATIBILITY.txt`. Unsupported builds preserve native behavior. Close the
game before installing, updating or removing the DLL; restart rather than hot reload.
