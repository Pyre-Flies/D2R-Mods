# Changelog

All notable changes to Item Roll Ranges are recorded here. Versions before the
GitHub migration are reconstructed from release and validation records.

## [Unreleased]

### Changed

- Moved the source into the D2R Mods monorepo and switched to the shared,
  pinned Plugin SDK submodule.

## [1.3.1+rev.1] - 2026-09-24

### Added

- Native Ctrl and R1/RB hold behavior for roll-range display.
- Prefix, suffix, tier, unique, and automagic source annotations where source
  provenance is unambiguous.
- Native-text reconciliation that preserves actual values and localized text.
- Provider, formatter, table-layout, artifact, and affix regression coverage.

### Validated

- Five automated suites passed.
- Magic and rare affix labels were confirmed in game.

### Known limitations

- Unsupported or ambiguous grouped properties keep native text without a
  guessed label.
- Set/unique and broader crafted/stat-combination coverage is intentionally not
  claimed beyond the documented evidence.
- The D2RCore compatibility profile is build-specific.

See `docs/VERSIONING.md` and the versioned audit documents for the 0.1.x through
0.4.x experimental history.
