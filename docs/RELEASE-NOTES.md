# D2R Mods release 2026.10.09.1

This stable release contains fresh runtime packages for:

- **Controller QOL Updates 1.3.1+rev.73**
- **Item Roll Ranges 1.3.1+rev.21**
- **Map Assistance 1.3.1+rev.1**

## Controller QOL

Promotes the tested rev.72 beta behavior without gameplay changes. Since the
previous stable QOL release, this includes remote-client LB item actions and
Ladder hook coexistence, vendor scroll purchases that fill tomes, improved
Telekinesis/Fire Blast targeting, neutral-A object priority, and first-press NPC
interaction after snap aiming.

Projectile leading uses observed enemy motion and bounded per-skill estimates.
The config supplies 42 projectile baselines and explicit zero-lead choices for
the remaining 198 catalog skills. Tuned ice limits and Shock Web, Holy/Miasma Bolt
and Blade Fury values are retained. Reimagined replacement skills are labelled;
automatic vanilla/Reimagined behavior detection remains deferred.

Detailed aim logging defaults off with `[aim] verbose = false`, including for
existing configs that omit the flag. Quiet mode skips diagnostic formatting and
logging-only work; warnings remain visible. Set true and restart for diagnostics.

Aim defaults: deadzone 0.22, speeds 8.0/48, acceleration 0.35 seconds, ground
reticle `#C2B596`, lock reticle `#CC9C52F2`, thickness 2.0.

Item Roll Ranges and Map Assistance receive fresh builds with no source changes.

## Validation and scope

The user reports the current QOL features are working well following beta
playtesting, including earlier confirmations of projectile tuning and interaction
fixes. Individual new skill estimates still benefit from gameplay feedback.
Handheld frame-time improvement has not been benchmarked; earlier full-Ladder
exit-crash investigations do not become confirmed fixes through this promotion.

Fresh local builds pass Controller QOL 28/28, Item Roll Ranges 5/5 and Map
Assistance 2/2 suites, including DLL artifact checks. Runtime ZIP layout and
checksums are verified before publication. Native hooks remain guarded to reviewed builds.
Offline and non-Ladder routes remain included; remote Identify All targets are
limited to Inventory, and Cube/Shared identification supplies are unsupported.

## Installation

Close the game and back up the existing plugin/config files. Extract each wanted
ZIP into the game directory; DLLs install under `d2rloader/plugins/` and configs
under `d2rloader/config/`. Merge new defaults into customized TOML files rather
than overwriting personal toggles. Restart after installation. Item Roll Ranges
requires `d2rcore.items.item_stat_ranges = true`.

Pack-managed Ladder installations should use the pack author's normal revision
process. Each ZIP contains file checksums; the release `SHA256SUMS` covers all
three ZIPs. Source archives are provided by GitHub for this tag.
