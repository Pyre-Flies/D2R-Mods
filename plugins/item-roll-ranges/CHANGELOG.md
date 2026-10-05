# Changelog

All notable changes to Item Roll Ranges are recorded here. Versions before the
GitHub migration are reconstructed from release and validation records.

## [1.3.1+rev.21] - 2026-10-05

- Remove keyboard/controller query adapters and physical key/XInput polling.
  Follow Core's native range-clone context for display enhancements, preserving
  native Ctrl and RT/R2 and Controller QOL's existing active R1/RB override.
- Keep focused panel gating and native formatter passthrough. No changes to
  Controller QOL, no input consumption and no new interoperability export.
- User confirmed revision20 Base displays look good. Revision21 input behavior
  requires live validation before publication.

## [1.3.1+rev.20] - 2026-10-05

- Preserve current native Defense/damage and label the underlying intrinsic
  range `Base:` instead of displaying calculated total roll intervals. For
  example, `32 to 69 (Base: 9 - 19)` and Bloodrune `25 (Base: 2 - 2)`.
- Apply the same display to armor, belts, and one-hand/two-hand/throw damage.
  Keep fixed endpoints explicit and account for ethereal scaling and the native
  Enhanced Defense maximum-plus-one generation rule.
- Verify base table bounds against primary item stats independently of unrelated
  proc, per-level, and other affix sources. Those sources no longer cause an
  unavailable header marker when the base is known. Individual affix ranges
  retain their existing reporting behavior.
- Five automated suites pass. Live revision20 tooltip validation remains pending.

## [1.3.1+rev.19] - 2026-10-05

- Fix Enhanced Defense/Damage operands being consumed from the aggregate stat
  vector. Read only validated permanent, state-zero child modifier lists and
  compare computed endpoints against both the aggregate total and native header.
- Recognize native property function2 for Enhanced Defense and unrelated copied
  resistance/elemental endpoint functions. Their presence no longer blocks an
  otherwise qualified total. Keep unknown/conditional/timed/source contexts closed.
- User reported revision18 Sturdy Sash remained unavailable; live inspection
  confirmed child ED18 but no aggregate ED16. Bloodrune remains user-confirmed.
  The reported bow has a per-level maximum-damage modifier, which remains
  unsupported and correctly displays the unavailable marker.
- Five suites pass with consumed-operand, Sturdy/Balance, ethereal Four Seasons,
  raw-total agreement, cycle and conditional-state regressions. Live revision19
  display and deployment pending.

## [1.3.1+rev.18] - 2026-10-05

- Calculate total Defense intervals for identified, socket-free normal/magic/
  rare/direct-unique armor with fully resolved positive Enhanced Defense and
  independently verified intrinsic base and native displayed value. Truncate
  ethereal scaling before applying the percentage. A Sturdy Sash with 10-20%
  ED now shows its actual Defense beside `(Total: 3-3)`.
- Preserve native actual one-hand, two-hand and throw damage headers and append
  `(Total: minLow-minHigh / maxLow-maxHigh)` when loaded source bounds and raw
  operands completely explain the native endpoints. Ethereal scaling, on-weapon
  ED and flat min/max additions use separate integer stages.
- Keep `(?)` and the report footer for unresolved sources, sockets/runewords,
  unsupported qualities/functions, contextual damage, clamp-crossing intervals
  or mismatched intrinsic/current values. Combined ED plus flat Defense remains
  unqualified; Bloodrune's existing additive Defense path is retained.
- Five automated suites pass, including arithmetic, source/value guards, native
  adapter delegation and artifact admission. Live revision18 validation pending.

## [1.3.1+rev.17] - 2026-10-05

- Label independently verified additive Defense header intervals `Total:`, for
  example `Defense: 25 (Total: 17-27)`, to distinguish them from affix bounds.
  Core-provided intervals retain their existing label pending arithmetic
  qualification; fixed and unavailable indicators retain their current display.
- User confirmed Bloodrune's revision16 total interval and revision17 label in game.

## [1.3.1+rev.16] - 2026-10-05

- Recover total Defense header intervals for fixed-base, non-ethereal unique
  armor with independently witnessed base/aggregate Defense and direct additive
  flat Defense sources. Bloodrune's base2 plus +15-25 yields total17-27.
- Refuse guessed totals for ED/contextual functions, property groups, sockets,
  runewords, variable bases, ethereal items or mismatched native/display values.
  Those retain the unavailable indicator when Core supplies no interval.
- User confirmed revision15's fixed/unavailable header display works. Revision16
  arithmetic passes automated regression checks; live confirmation is pending.

## [1.3.1+rev.15] - 2026-10-05

- Corrected the Defense localization resource group from `eng` to the native
  `d2r` group. The wrong group caused revision14 to skip fixed intervals and
  unavailable-header markers; variable-header value preservation was unaffected.
- Added a byte witness for the native group literal and a regression that
  verifies the resource group, key and required-lookup flag.
- User confirmed actual Defense stays visible beside variable ranges in
  revision14. Fixed ranges and unavailable-header markers still need a
  revision15 in-game check.

## [1.3.1+rev.14] - 2026-10-05

- Preserve actual Defense beside Core's variable header interval while Ctrl or
  RB is held, for example `Defense: 5 (3-5)`.
- Show equal endpoints for verified fixed Defense, including ordinary and
  ethereal sashes. Incomplete source/socket evidence produces `(?)` instead of
  a guessed fixed interval.
- Mark unresolved expected variable modifier ranges with a gray `(?)` and
  show `(?) Range Unavailable - Please report item affixes` once per tooltip.
  Non-rollable lines do not trigger the hint.
- Added exact-build header caller/function guards and ownership-checked slot
  teardown. Input remains query-result substitution; no key/button is consumed.
- Automated regression/build/artifact checks and read-only native qualification
  are separate from visible in-game validation, which remains pending.

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
