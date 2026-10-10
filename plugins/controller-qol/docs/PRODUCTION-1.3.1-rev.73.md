# Production 1.3.1+rev.73

2026-10-09. Stable promotion of rev.72-beta.1 after the user reports the current
features are working well. Version, documentation and release metadata change;
no gameplay logic, native signatures, leading values or configuration defaults
change in this promotion.

The release includes fresh Item Roll Ranges rev.21 and Map Assistance rev.1
builds. Their sources and independent plugin versions are unchanged.

The reviewed native profile remains documented in
[rev.60 production](PRODUCTION-1.3.1-rev.60.md),
[remote item mechanics](REMOTE-ID-TRANSFER-TRACE.md),
[neutral-A recovery](NEUTRAL-A-2026-10-08.md), and
[NPC categories](NPC-INTERACTION-2026-10-08.md).
The [leading baseline record](LEADING-BASELINES-REV71.md) identifies empirical
estimates and variant limitations. `[aim] verbose` defaults false; quiet-mode
regression tests preserve enemy/NPC scoring and suppress detailed log writes.

User confirmation establishes the reported playtesting experience, not every
skill/platform/mod combination. Automatic mod differentiation remains deferred.
No measured Steam Deck frame-time claim is made. Earlier full-Ladder exit-crash
investigations remain separate and are not labelled fixed by this promotion.

Fresh local Release builds pass QOL 28/28, Item Roll Ranges 5/5 and Map
Assistance 2/2 suites. Package checks cover byte-identical local DLLs, stable
Windows version flags, install paths, expected configs and SHA-256 manifests.
GitHub repeats builds, tests and packaging from the release tag. Runtime ZIPs contain no game executables, local logs, dumps or backups.
