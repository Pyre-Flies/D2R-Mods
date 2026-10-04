# Production 1.3.1+rev.60

2026-10-04. Consolidates rev.52 through rev.60 skill isolation, class configuration,
optional cast observer, reticle persistence, R3 skill toggles/indicators, reticle
styling, locked settings and independent targeting overrides. Detailed evidence:
[skill-tree contracts](SKILL-TREE-AIM-TOGGLE.md), [catalog](SKILL-CATALOG.md),
[hook coexistence](AIM-CAST-OBSERVER-COMPATIBILITY.md) and
[skill isolation](IMPLEMENTATION-1.3.1-rev.52.md).

Uses the existing qualified Windows x64 game/D2RCore profile and SDK ABI4.
Game disk SHA256: 1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7.
D2RCore SHA256: 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
All native contracts are build-specific and guarded. Optional observer conflicts
omit that observer while required aim site failures preserve native behavior.

User reports the current runtime works well and R3 feels improved. Catalog
comments are preliminary classifications, not runtime metadata discovery.
Automated checks do not establish live correctness of all 240 skills, custom
skills, every tree/resolution, or arbitrary hook owners. Individual new skill
opt-ins/overrides still need live validation.

Runtime packaging places DLLs under d2rloader/plugins and TOML defaults under
 d2rloader/config. Extract into the game directory with game/loader closed.
Preserve customized configs on upgrades. Packaging verifies ZIP integrity, exact
file lists and SHA256 checksums; layouts are audited before release. Source,
compatibility records and reproducible build metadata remain in Git; binaries,
local logs, backups and proprietary inputs are excluded.

Pre-release local validation passed: Controller QOL 23/23, Item Roll Ranges 5/5,
Map Assistance 2/2. All three runtime ZIPs passed integrity/checksum audits;
packaged DLL bytes match tested builds. QOL/Map configs have exact installation
paths, and misplaced config/DLL regression checks passed. GitHub builds and
release asset verification are performed after the release tag is pushed.
