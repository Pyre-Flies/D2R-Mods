# Pyre Performance Optimizations

**1.3.1+rev.2 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Improves frame pacing, combat simulation throughput, and audio/VFX stability during dense monster packs and heavy multi-projectile gameplay across nine individually toggleable optimization switches.

> [!IMPORTANT]
> **Highly Recommended - Pair with [Universal Chain Hook](../chain-hook/README.md):**
> **Pyre Performance Optimizations** hooks many native engine paths across projectile combat, hit-delay tracking, room collision, aura/area scans, monster AI, and client state updates. Installing **[Universal Chain Hook](../chain-hook/README.md)** (`00-d2rl-chain-hook.dll`) alongside `pyre-perf.dll` is **highly recommended** so all overlapping hooks chain cleanly with your other D2RLoader plugins in any load order.

## What each optimization switch does

Switches `2` through `9` are enabled by default; Switch `1` is optional and disabled by default:

- **Switch 1 - Same-Tick Missile Damage Batching (`batch_damage`, default `false`)**: Coalesces repeated non-lethal elemental missile hits on the same target within a single server tick while passing lethal hits through immediately. Kept off by default to preserve per-hit proc and Crushing Blow behavior unless you explicitly enable it.
- **Switch 2 - Missile Damage Stat Cache (`cache_damage`, default `true`)**: Caches attacker missile damage calculations within the same server tick instead of re-reading full stat lists on every projectile impact.
- **Switch 3 - Fast NextDelay / Hit-Lock Lookup (`fast_nextdelay`, default `true`)**: Replaces repeated per-target `NextDelay` list scans with a fast lookup table during dense multi-projectile barrages, and cooperates automatically with [Universal Chain Hook](../chain-hook/README.md) when other plugins hook hit-delay checks.
- **Switch 4 - Spatial Room Collision Filter (`spatial_collision`, default `true`)**: Tightens broad collision masks and skips non-overlapping rooms and non-collidable units during projectile and unit collision checks.
- **Switch 5 - Client Impact Sound & Explosion VFX Budgeter (`throttle_client_vfx`, default `true`)**: Caps simultaneous duplicate missile impact sounds and stacked cosmetic sub-missile bursts within a short frame window to prevent audio/particle hitching while leaving inventory and UI sounds untouched.
- **Switch 6 - Unit Collision Size & Pattern Cache (`collide_size_cache`, default `true`)**: Caches per-unit collision size and footprint lookups across pathfinding, melee range checks, and projectile collision.
- **Switch 7 - Radial Aura & Area Scan Pre-Filter (`radial_unit_filter`, default `true`)**: Skips non-matching ground items, missiles, corpses, and objects early during aura and radial area-of-effect scans before full distance checks run.
- **Switch 8 - Monster AI Room Scan Pre-Filter (`ai_room_filter`, default `true`)**: Pre-filters adjacent room unit lists during monster AI target, leash, corpse, and object scans so AI ticks ignore irrelevant ground loot and projectiles.
- **Switch 9 - Frozen Enemy Shatter VFX Fix (`freeze_shatter_fix`, default `true`)**: Restores missing ice-shatter burst visuals and audio when frozen monsters die and their freeze/death state updates arrive out of order.

## Installation and updates

1. **Install via D2R Reimagined Launcher or manual extraction:**
   - **Launcher:** Use **Install Plugin/Zip** on `Pyre-Performance-Optimizations-1.3.1+rev.2-PyreFly.zip` (and `Universal-Chain-Hook-1.3.1+rev.7-PyreFly.zip` if not already installed).
   - **Manual extraction:** Close the game and extract `Pyre-Performance-Optimizations-1.3.1+rev.2-PyreFly.zip` into your game directory (for global/Vanilla use) or your active mod directory (for example `mods/Reimagined/`), placing `pyre-perf.dll` inside `d2rloader/plugins/`.
2. Start the game through D2RLoader. On first launch, D2RLoader creates a minimal `d2rloader/config/pyre-perf.toml` (`enabled = true`); individual optimization switches use the built-in defaults below and are written automatically whenever you toggle them with `/pyreperf` (or you can add any key manually).

## Chat and console commands (`/pyreperf`)

Enter commands in normal game chat or in the D2RLoader console:

| Command | What it does |
| --- | --- |
| `/pyreperf` or `/pyreperf status` | Show whether Pyre Performance Optimizations is enabled and display the active state (`ON`/`OFF`) and readiness (`ok`/`pending`) of switches `1` through `9`. |
| `/pyreperf on` · `/pyreperf off` | Enable or disable all optimizations and save the setting to `pyre-perf.toml`. |
| `/pyreperf all on` · `/pyreperf all off` | Turn the standard switch set (`2`–`9`) on or off at once while keeping optional Switch `1` off. |
| `/pyreperf <1..9> [on\|off]` | Toggle an individual switch by number or alias (`1`/`batch`, `2`/`dmgcache`, `3`/`nextdelay`, `4`/`spatial`, `5`/`cltvfx`, `6`/`collide`, `7`/`radial`, `8`/`ai`, `9`/`shatter`) and save it to `pyre-perf.toml`. |
| `/pyreperf reset` | Clear active in-memory optimization caches. |

## Configuration (`pyre-perf.toml`)

All keys belong under `[pyre-perf]`:

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Master toggle (`true` or `false`) for all optimizations. |
| `batch_damage` | `false` | Switch 1: Same-tick non-lethal elemental missile damage batching (`true` or `false`). |
| `cache_damage` | `true` | Switch 2: Per-tick missile damage stat cache (`true` or `false`). |
| `fast_nextdelay` | `true` | Switch 3: Fast `NextDelay` hit-lock lookup table (`true` or `false`). |
| `spatial_collision` | `true` | Switch 4: Spatial room collision and mask filter (`true` or `false`). |
| `throttle_client_vfx` | `true` | Switch 5: Client missile impact sound and explosion VFX frame budgeter (`true` or `false`). |
| `collide_size_cache` | `true` | Switch 6: Unit collision size and footprint cache (`true` or `false`). |
| `radial_unit_filter` | `true` | Switch 7: Radial aura and area-of-effect unit pre-filter (`true` or `false`). |
| `ai_room_filter` | `true` | Switch 8: Monster AI adjacent-room unit pre-filter (`true` or `false`). |
| `freeze_shatter_fix` | `true` | Switch 9: Frozen enemy death ice-shatter VFX restoration (`true` or `false`). |

### Recommended configuration example

```toml
[pyre-perf]
enabled = true
batch_damage = false
cache_damage = true
fast_nextdelay = true
spatial_collision = true
throttle_client_vfx = true
collide_size_cache = true
radial_unit_filter = true
ai_room_filter = true
freeze_shatter_fix = true
```

## Multiplayer and compatibility notes

- **Closed-Source Runtime Release**: Distributed as a standalone runtime DLL (`pyre-perf.dll`) with this player guide.
- **Client vs. Host Scope**: Client-side optimizations (Switch 5 sound/VFX budgeting and Switch 9 frozen shatter restoration) apply in all modes including server-hosted multiplayer. Simulation-side optimizations (Switches 1–4 and 6–8) run wherever the `D2Game` simulation runs (offline single-player, local TCP/IP host, or dedicated server).
- **Safe Build Guard**: Validates the active game build at startup and remains safely inert on unsupported versions.
