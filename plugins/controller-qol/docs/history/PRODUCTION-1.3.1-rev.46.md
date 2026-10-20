# Controller QOL Updates 1.3.1+rev.46

Promoted for production at the user's request on 2026-09-27. This release
includes revisions 23 through 46 on top of the rev.22 production baseline; see
`CHANGELOG.md` and the focused records included in the runtime archive. Debug
logging and ordinary-chest priority remain off by default.

Target: Windows x64, D2RLoader 1.3.1 / plugin ABI4, SDK v0.3.0. Current guarded
D2RCore SHA-256:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Native game RVAs, exact byte witnesses, shared-entry ownership rules and the
active ObjectsTxt-derived class sets are recorded in the plugin documents and
`docs/reverse-engineering/controller-qol-native-contracts.md`.

The local rev.46 Release build passed all 20 automated suites, including the
ABI resource, exports, plugin identity/version, compatibility policies and
priority classifier. The user confirmed the later SDK Identify All and bulk
stash iterations and reported materially improved object priority behavior.
The rev.46 environmental-well correction still awaits its focused visible
in-game retest. Other candidate-specific validation boundaries remain as
stated in the changelog and revision records; automated checks are not live
validation and do not qualify unreviewed game builds.

Close D2R and D2RLoader before replacing the DLL, preserve the existing TOML,
and keep only one active plugin copy. Hot reload is unsupported. Native
features validate their reviewed profiles and fail open when an artifact,
signature, owner or layout does not match.

The GitHub release workflow performs clean builds and tests for all three
standard plugins, creates deterministic ready-to-install runtime ZIPs and
publishes SHA-256 checksums. Game binaries, captures, logs, build outputs and
other proprietary or local artifacts are not part of the repository release.
