# D2R Mods - 2026.11.15.1

Updates **Pyre Performance Optimizations** (`1.3.1+rev.3`) with Warlock skill fixes, broader companion plugin compatibility, and native `/pyreperf` chat command registration:

- **Fix Warlock 'Miasma Chains' not tethering or dealing cloud damage:** Spatial collision filtering (Switch `4`) no longer skips valid chain-tether and cloud collision checks on Warlock's 'Miasma Chains'.
- **Fix Warlock demon deaths triggering false cold-shatter bursts:** Frozen enemy shatter restoration (Switch `9`) won't shatter un-frozen Warlock bound or summoned demons when they die anymore unless they're actually frozen, and respects monsters whose death state already suppresses corpses.
- **Cleaner companion plugin & audio-hook compatibility:** Client impact sound budgeting (Switch `5`) now leaves the shared unit-sound function untouched so it works out of the box alongside audio-hooking plugins like **Controller QOL Updates**, **Inventory Deposit**, and drop-alert mods even without **Universal Chain Hook**, and chains cleanly across plugin reloads and third-party detours.
- **Native `/pyreperf` in-game chat command registration:** Registers `/pyreperf` directly with the game's slash-command parser (alongside `pyreperf` in the D2RLoader console) so toggling switches in game chat works cleanly across offline, TCP/IP, and multiplayer sessions.
- **Full runtime suite included:** Bundles **Pyre Performance Optimizations `1.3.1+rev.3`**, **Universal Chain Hook `1.3.1+rev.7`**, **Controller QOL Updates `1.3.1+rev.97`**, **Inventory Deposit `1.3.1+rev.16`**, all remaining PyreFly plugins, **Buff HUD `1.3.1+rev.3`**, and a complete `SHA256SUMS` manifest. Keep the `00-` prefix on `00-d2rl-chain-hook.dll` so D2RLoader loads it first.

Ran through the unit, hook-coexistence, binary, and packaging checks locally, and verified the 'Miasma Chains' collision fix in-game.
