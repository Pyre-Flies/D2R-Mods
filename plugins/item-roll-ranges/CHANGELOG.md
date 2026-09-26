# Changelog

All notable changes to Item Roll Ranges are recorded here. Versions before the
GitHub migration are reconstructed from release and validation records.

## [1.3.1+rev.13] - 2026-09-26

### Changed

- Replaced player-facing `[Automagic]` with `[Base]` for inherent base-item
  modifiers.
- Shortened rolled-affix source tags to `[P]` and `[S]` and added each verified
  internal affix name, for example `[P] [Sharp] [T13]`, so multiple property
  lines from the same affix are visibly linked.
- Bumped the plugin identity to `1.3.1+rev.2`.
- Added conservative source attribution for combined poison-damage lines that
  bypass the ordinary single-property formatter. The fallback requires a
  verified loaded `dmg-pois` affix and exactly one unidentified tooltip line.
- Bumped the plugin identity to `1.3.1+rev.3`.
- Generalized the guarded paired-formatter identity observer from Enhanced
  Damage to all native damage-pair families.
- Added bounded PropertyGroups traversal for a single fixed, positive-weight
  choice; randomized, malformed, cyclic, and out-of-range groups fail open.
- Bumped the plugin identity to `1.3.1+rev.4`.
- Extended the sole-unidentified-line fallback from poison to all composite
  damage families after live tests showed fire, lightning, cold, magic, and
  multi-element lines bypassing both native identity observers.
- Deduplicate candidate stats by their complete loaded source label, allowing a
  single multi-element affix to resolve while refusing competing sources.
- Bumped the plugin identity to `1.3.1+rev.5`.
- Supported the three separate native damage lines emitted by one `Elemental1`
  affix, requiring an exact line-to-family count and one identical loaded
  source label across every line.
- Bumped the plugin identity to `1.3.1+rev.6`.
- Reconciled different unresolved damage sources one-to-one using the active
  loaded `ItemStatCost.descpriority`, while refusing count mismatches and ties.
- Bumped the plugin identity to `1.3.1+rev.7`.
- Added gray per-source unknown-contribution rows beneath stacked composite
  damage totals, preserving the native combined range without inventing rolls.
- Bumped the plugin identity to `1.3.1+rev.8`.
- Corrected mixed-family attribution to follow Core's ascending-priority buffer
  order before its bottom-to-top tooltip layout.
- Fixed the separator before a second colored damage span being misread as a
  unary minus and collapsed paired bounds to an overall minimum/maximum range.
- Bumped the plugin identity to `1.3.1+rev.9`.
- Allowed gray stacked-source rows when the native provider exposes no range
  span, preserving the actual combined value without synthesizing bounds.
- Bumped the plugin identity to `1.3.1+rev.10`.
- Allowed verified stacked damage sources to expand when Core's partial ranged
  line has a different numeric shape/key from the combined actual line.
- Omitted the incomplete provider interval rather than presenting it as the
  combined range.
- Bumped the plugin identity to `1.3.1+rev.11`.
- Distinguished range separators after an existing number from unary negative
  signs before a native range span.
- Displayed negative intervals in ascending signed order, for example
  `[-20 - -11]` instead of `[-(+11 - +20)]`.
- Bumped the plugin identity to `1.3.1+rev.12`.
- Added guarded endpoint decomposition for stacked fire, lightning, and cold
  damage using the active loaded property definitions.
- Replaced `?` child values with exact contributions only when both combined
  endpoints have one unique in-range solution.
- Bumped the plugin identity to `1.3.1+rev.13`.

### Repository

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
