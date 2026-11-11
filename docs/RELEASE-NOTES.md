# D2R Mods - 2026.11.11.1

Adds **Pyre Performance Optimizations** and **Universal Chain Hook** to the public runtime release alongside the full D2RLoader plugin suite.

- **Pyre Performance Optimizations 1.3.1+rev.1 (`pyre-perf.dll`):** Improves combat frame pacing and simulation throughput during dense monster packs and heavy projectile builds across nine live-toggleable switches (`/pyreperf`) - missile damage stat caching, fast `NextDelay` hit-lock lookups, spatial collision filtering, client impact sound and sub-missile VFX budgeting, unit collision size caching, radial aura/AoE pre-filtering, monster AI room pre-filtering, and frozen enemy shatter VFX restoration (plus optional same-tick missile damage batching, off by default). **Highly recommended to pair with Universal Chain Hook**, since it hooks across many engine subsystems.
- **Universal Chain Hook 1.3.1+rev.6 (`00-d2rl-chain-hook.dll`):** Lets multiple D2RLoader plugins that hook the same underlying game functions or dispatch tables run together in any load order without conflicts or extra hot-path overhead, with `/chainhook status`, `/chainhook list`, and `/chainhook test` inspection commands in the D2RLoader console.
- **Full Runtime Suite Included:** Bundles all 14 PyreFly D2RLoader plugins plus the **Buff HUD 1.3.1+rev.3** companion package and a complete `SHA256SUMS` manifest in one release.

**Upgrading & installation notes:**
- Both **Pyre Performance Optimizations** and **Universal Chain Hook** are released as standalone runtime DLLs (`pyre-perf.dll` and `00-d2rl-chain-hook.dll`) with player guides in [`plugins/pyre-perf/README.md`](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/pyre-perf/README.md) and [`plugins/chain-hook/README.md`](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/chain-hook/README.md).
- Keep the `00-` prefix on `00-d2rl-chain-hook.dll` intact so D2RLoader discovers it first.

Automated build, unit, runtime hook, binary audit, and packaging checks pass across all included packages, alongside prior live gameplay testing of both new plugins.
