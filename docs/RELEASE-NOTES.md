# D2R Mods - 2026.11.11.2

Updates **Universal Chain Hook** (`1.3.1+rev.7`) and **Pyre Performance Optimizations** (`1.3.1+rev.2`) with broader third-party plugin compatibility, shared hotkey support, and faster startup on macOS CrossOver:

- **Shared plugin hotkeys no longer block each other:** When two plugins bind the same key (like `F10`), the first plugin to handle it won't swallow the keypress and starve the other one anymore - every plugin listening on that key gets notified cleanly without double-firing on a single press.
- **Preserve shared jump-stub space between plugins:** Plugins that look for unused padding space to place their jump stubs won't have an earlier plugin's stub wiped out or overwritten while Chain Hook is coordinating startup.
- **Call-site and lazy UI panel hook chaining:** Extends automatic chaining beyond standard function prologues so plugins that patch individual call sites or wrap UI panels lazily after startup (like opening stash or inventory grids) chain cleanly instead of stepping on each other.
- **Eliminate macOS CrossOver startup stall in Pyre Performance Optimizations:** Skips past host-reserved memory windows immediately during startup under CrossOver / Wine so the game launches right away instead of stalling for over a minute, without changing native Windows behavior.
- **Full runtime suite included:** Bundles **Universal Chain Hook `1.3.1+rev.7`**, **Pyre Performance Optimizations `1.3.1+rev.2`**, all 12 open-source PyreFly plugins, **Buff HUD `1.3.1+rev.3`**, and a complete `SHA256SUMS` manifest. Keep the `00-` prefix on `00-d2rl-chain-hook.dll` so D2RLoader loads it first.

Ran through the full unit, live x64 chaining, runtime artifact, and packaging checks locally - live third-party combo and CrossOver testing in-game are ready for a spin.
