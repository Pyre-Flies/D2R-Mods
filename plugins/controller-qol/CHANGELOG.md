# Changelog

All notable changes to Controller QOL Updates are recorded here. Versions before
the GitHub migration are reconstructed from release and validation records.

## [Unreleased]

### Changed

- Moved the source into the D2R Mods monorepo and switched to the shared,
  pinned Plugin SDK submodule.

## [1.3.1+rev.7] - 2026-09-26

### Added

- Native portal-priority behavior and contact-gate integration.
- Build-specific guarded native input, ground-label, glyph, storage, belt,
  materials, shared-page, and vendor integrations documented under `docs/`.

### Validated

- Twelve automated suites passed.
- Chronicle Ground Flag coexistence and portal behavior were confirmed in game.
- Earlier native-controller validation covered Battle.net DualShock and Steam
  controller paths with reviewed plugin coexistence.

### Known limitations

- Native profiles are specific to the tested game/module builds.
- Hot reload is not a supported update path.

See `README-HISTORICAL.md` and the versioned production documents for earlier
experimental 1.5.x checkpoints and detailed provenance.
