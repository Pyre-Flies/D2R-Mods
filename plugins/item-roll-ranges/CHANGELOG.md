# Item Roll Ranges changelog

## Distribution documentation

- Keep runtime ZIPs minimal: one DLL, reference defaults and a usage README
  with command examples and required notices. Keep detailed guides, release
  history and engineering reports in the repository. Sanitize public documents.
- Packaging/privacy checks are automated; plugin DLLs and live-tested behavior
  are unchanged by this documentation update.

## [1.3.1+rev.45] - 2026-10-15

- Add `(Ilvl: XX, Qlvl: YY)` while ranges are shown, below Required Level and
  before Max Sockets / weapon Attack Speed. Items without Required Level use
  existing header anchors; header-less and empty bodies are supported.
- Read generated item level and active base quality level directly. Respect
  modded base definitions without a database or calling a repairing level getter.
- Reuse the existing guarded header callback. Two optional field witnesses omit
  only the new line on mismatch; no additional hook or provider slot is added.
- Nine suites and DLL artifact checks pass. The user confirmed Reimagined
  placement and Ctrl release after restart; startup has no plugin guard refusal.
  Unattributed UI/object-pool shutdown assertions are recorded separately.
- Preserve actual values, roll/source behavior, existing settings and the false
  always-show default. See [the level contract](docs/ITEM-LEVELS-REV45.md).

## [1.3.1+rev.44] - 2026-10-14

- Resolve shared property rolls across resistances and grouped unique Attributes.
  Infer overlapping contributions only when owned stats uniquely constrain them.
- Resolve all active runeword recipes from ordered sockets, eligible bases and
  separate word-owned stats. Add `[RW]` sources using current compiled tables,
  respecting vanilla/mod differences without a shipped runeword database.
- Display independently rolled elemental-damage endpoints on a single affix,
  and replenishment intervals from qualified native timing and active parameters.
- Support random skill/class/tab properties: their parameter is bonus strength,
  while min/max select the stat layer. Prove selections alongside fixed skill
  sources; show compact fixed bounds such as `[+5]`. Group tiers within the same
  selector pool and keep class pools separate.
- Keep ambiguous, unsupported or budget-limited attribution native. Add optional
  exact semantic/dispatcher guards; no new hook or arbitrary formatter chaining.
- Nine suites pass. Private replays inspect2,026 affix rows, resolve219/219
  active Reimagined runewords and verify56 random-selector specifications.
  Captured charm/amulet metadata replays pass. User checks confirm Reimagined
  resistances, Attributes, Bone/Lightning/Honor, fire endpoints, replenishment,
  hold/toggle behavior and Wake of Fire; subsequent vanilla smoke checks were
  also user-confirmed. These samples are distinct from catalog/automated coverage.
- Always-show remains disabled by default. Existing user configuration is
  preserved. See [property coverage](docs/PROPERTY-ROLLS-REV41.md) and
  [random selector contracts](docs/RANDOM-SKILL-PROPERTIES-REV44.md).

## Development candidates included in rev.44

The following records describe intermediate validation before this release.
Those candidate versions were not published separately.

## 1.3.1+rev.43 (local candidate)

- Treat ItemStatCost operations8/9 as direct item attribute operands; their
  character-resource conversion metadata does not scale displayed item points.
  This restores grouped Attributes with real Energy/Vitality metadata.
- Add an optional private replay of the current charm's unique/group/property/
  stat tables and owned values. It renders the active5..10 Attributes bounds;
  the previous synthetic fixture incorrectly used operation0 for every member.
- Keep proof-budget exhaustion distinct from a contradiction when resolving
  weighted alternatives; unknown implicit targets refuse the unproved model.
- Nine suites and the exact captured charm replay pass. The user confirmed the
  Attributes prefix, Prismatic overlap, Bone sources, Raven Song fire bounds,
  replenishment display and hold/always-show behavior in standalone rev.43.
  Lightning (Reimagined) and Honor (vanilla-origin) runewords also passed the
  user's live checks. These samples do not prove every recipe's visible output.
  No public release.

## 1.3.1+rev.42 (local candidate)

- Prove selected alternatives in supported exactly-one weighted unique pools
  using owned stat constraints. Keep ambiguous candidates conditional while
  preserving existing witnessed unique marginal bounds. This covers grouped
  All Attributes when it is an alternative to FRW or Faster Hit Recovery.
- Render independently rolled elemental damage endpoints for a single host
  affix, as well as overlapping socket sources. This fixes Raven Song's
  Scorching fire-damage range without changing its actual damage values.
- Nine suites pass, including the weighted-pool and two-endpoint production
  regressions. All2026 active affix rows and219 runeword recipes were inspected;
  that data audit is distinct from complete visible tooltip validation.
- Installed in standalone Reimagined with configuration/companions preserved;
  visible restart confirmation remains pending. No public release.

## 1.3.1+rev.41 (local candidate)

- Interpret shared property rolls across linked stats; infer overlapping
  resistance contributions only when owned values uniquely constrain them.
  Add grouped All Attributes bounds from active unique definitions.
- Resolve all active runeword recipes using ordered sockets, base restrictions
  and separate runeword stat ownership. Add `[RW]` sources and verified
  scalar/class/tab/skill/proc bounds without a runeword database.
- Show quantity-replenishment intervals from active parameters and guarded
  recurring timing conversion. Unsupported operations retain native display.
- Permit direct unique properties with empty PropertyGroups; refuse numeric
  assumptions about ambiguous weighted groups and non-additive operations.
- Nine suites and a private219-recipe replay pass; visible candidate validation
  remains pending. See [coverage and native evidence](docs/PROPERTY-ROLLS-REV41.md).

## 1.3.1+rev.40

- Add optional always-show ranges, disabled by default in embedded and reference
  config. `/AlwaysShowRolls` toggles for the session; on/off/status supported.
- Preserve native Ctrl/controller hold behavior, fixed Base headers and Max
  Sockets placement. Hide only the range hint while always-show is active.
- Require the qualified property formatter target and entry bytes; refuse unknown
  predecessors. Reuse completed actual text after post-render merge failure.
- Fix plain Sash hold-mode keyboard local width to match native byte reads.
- Eight suites pass. User confirmed always-show, chat toggles and the final plain
  Sash repair; runtime registration and dispatch logs show no IRR backend errors.
  Arbitrary formatter composition and exhaustive platform coverage are excluded.

## [1.3.1+rev.34] - 2026-10-12

- Insert socket capacity beside the actual Required Level header line. The
  builder returns the full tooltip including affixes; revision33 incorrectly
  prepended to that entire buffer, displaying capacity below the affixes.
- On weapons, retain Required Level / Max Sockets / Attack Speed order.
  Without a level requirement, use the lowest recognized localized native
  requirement, durability, quantity, Defense or damage header. Preserve native
  text if the anchor is missing or ambiguous. Unsocketed capacity stays native.
- Full-tooltip regression fixtures now include sockets, overlapping affixes,
  attack speed and requirements in native bottom-to-top order, plus translated
  templates, absent-level fallbacks and duplicate refusals. All seven automated
  suites pass. The user confirmed the corrected placement in-game after
  installing revision34. Included in release-2026.10.12.2.
  See [the revision34 correction](docs/SOURCE-CAPTURE-REV32.md#revision34-header-placement-correction).

## [1.3.1+rev.33] - 2026-10-12 (local candidate)

- Move socket capacity into the held-range header, below Required Level,
  matching the native unsocketed layout. Remove revision32's modifier-row
  insertion and incorrect `Required Level: 2` label.
- Use the actual native `d2rloader:D2RLoaderMaximumSockets` localization key;
  the intermediate `ItemStats1p` lookup is Required Level, not Max Sockets.
  Qualify the key and its native caller alongside the capacity helper.
- Preserve capacity through the existing guarded header publication, including
  weapon headers without Defense. Current sockets and socket contributions
  remain unchanged; releasing ranges retains the ordinary native tooltip.
- Seven automated suites pass, with the real key distinction, translated
  templates, header ordering, capacity refusals and publication retention.
  Read-only comparison confirms both new optional witnesses match the loaded
  build. Visible restart rendering remains pending. Not published.
  See [the revision33 correction](docs/SOURCE-CAPTURE-REV32.md#revision33-header-correction).

## [1.3.1+rev.32] - 2026-10-12 (local candidate)

- Admit the observed extended stat-list flags on Bramble Song while retaining
  ownership, parent, vector and socket checks. This restores the Eagleeye/Trump
  scaling breakdown and its level-100 bounds.
- Support Enhanced Defense's actual property handler2 for magic/rare source
  bounds. Godly and Silver-Wrought show combined bounds and separate source
  rows; individual rolls remain unknown when the saved total is ambiguous.
- Include flat Thorns in the scalar source model, removing the false missing
  range warning for Shadow Mantle's known 1-3 roll.
- Show native `Max Sockets` capacity for already socketed items while holding
  ranges. The socket count is built outside the transformed property buffer,
  so revision31's attempted suffix could never match it.
- Seven automated suites pass with fixtures corrected to match the observed
  flags, property handlers and multi-property affix records. Visible restart
  rendering remains pending. See [revision32 evidence](docs/SOURCE-CAPTURE-REV32.md).
  Not published; enchantments remain deferred.

## [1.3.1+rev.31] - 2026-10-12 (local candidate)

- Restore light-blue source labels on fixed modifier rows, including
  Loremaster and of Speed, while preserving the original modifier color.
- Extend the existing native hold request to identified tooltips without a
  range clone. Consume that request only at the qualified property caller,
  with matching source identity; release and the next tooltip clear it.
- Recover missing scalar identities using exact localized native formatting
  and owned source stats. Show complete magic/rare source bounds even when
  Core omitted its range pass; ambiguous identities or unknown sources stay
  native. Include the typed class-skill selector for Shadow Mantle.
- Break down proven overlapping level-scaled sources, including Eagleeye and
  Trump. Require unique actual contributions and compatible rounding; keep
  ambiguous individual rolls unknown and retain the displayed total.
- Show a socketed item's capacity beside its current socket count, such as
  `Socketed (2) (Max: 4)`, using the guarded native Core capacity helper.
  Preserve the existing unsocketed header and refuse invalid/mismatched data.
- Read-only live captures confirmed Shadow Mantle's absent clone and the
  source coefficients/operands. Seven automated suites pass, including
  capacity, color, selector, duplicate-text, wrong-source, repeated-request,
  release and rounding-refusal fixtures. Restart rendering is pending.
  See [revision31 evidence](docs/CLONELESS-TOOLTIPS-REV31.md). Not published.

## [1.3.1+rev.30] - 2026-10-12 (local candidate)

- Include property-function8 scalars in fixed-value prefixes, covering fixed
  Faster Hit Recovery and other additive speed properties under the already
  guarded native handler. Raven Carapace should show `[+10%]` for of Balance.
- Add a fixed proc chance prefix when the packed skill/level, independent
  chance operand, actual text and owned stat lists agree. Keep the skill level
  in the original line; do not treat chance/level as a scalar min/max range.
- Use the loaded Experience MaxLvl header for proven level-based bounds,
  correcting Thorns to 1-100 and Paleocene Defense to 6-600 in this setup.
  Require uniform class caps, matching raw coefficients and supported active
  operation metadata; unfamiliar bases or source totals retain native bounds.
- Compare function17 coefficients as roll strength within a tier family.
  Preserve different property IDs and selectors, so flat and per-level Thorns
  are separate families despite sharing the displayed name. The observed
  coefficient8 family member becomes T2 below coefficient12 at T1.
- New optional byte witnesses gate scaling/proc additions and coefficient
  families; mismatches retain previous behavior. Read-only source/stat/table
  captures establish the reported values. All seven suites pass and all six
  new optional byte windows match the loaded build. Restart rendering remains pending;
  no release is published and no input or gameplay behavior changes.

## [1.3.1+rev.29] - 2026-10-12 (local candidate)

- Compact verified fixed modifier bounds to a singleton, such as `[+38]`
  for Perfect Ruby Life and `[+50%]` for fixed corruption ED. Base headers
  retain explicit endpoints. Add fixed P/S bounds when the captured values
  exactly match the single actual number, including Loremaster's `[+2%]`.
- Recognize crude/cracked/damaged armor's reduced base interval, with integer
  truncation and observed intrinsic agreement; refuse unsupported ethereal or
  custom low-quality bases. Normal and superior behavior stays intact:
  durability-only superior armor varies; generated ED uses a fixed max+1 base.
- Read-only inventory controls confirm normal121, crude87 and durability-only
  superior118 with active Serpentskin bounds111–126. Regression tests cover
  these shapes, refusals and fixed modifier formatting; all seven suites pass.
  Visual confirmation for revision29 remains pending; no release is published.
- Extend the same quality check to weapon Base headers. Live Long Sword
  controls retain normal3-19 and superior3-19 (ED6% produces actual3-20),
  while low quality uses2-14. Require native intrinsic agreement; cover
  one-hand/two-hand/throw paths and minimum-one truncation in tests.

## [1.3.1+rev.28] - 2026-10-12 (local candidate)

- Restore generated Enhanced Defense armor Base headers when jewels/gems are
  socketed. The legacy host operand reader rejected attached extended socket
  siblings before reaching the own ED list. Reuse the guarded owned source
  capture for the host-generation check, excluding socket ED and nested copies.
- Beast Shell stores intrinsic Defense127 and own ED94%; the header should
  retain actual246 and show `(Base: 127 - 127)`. No inverse rounding guess,
  input change, gameplay write or new native hook is introduced.
- Seven automated suites pass, including reversed child order with two socket
  owners, native header colors, ethereal generation, excluding socket-only ED,
  guard mismatch, bad parent/state and cyclic insertion refusal. Live read-only
  capture confirms the failure mechanism and values; the user confirms
  Defense246 with Base127–127 and the preserved Life/socket breakdown.

## [1.3.1+rev.27] - 2026-10-12 (local candidate)

- Replace host-only bounds on supported socket-overlap modifiers with the
  complete combined range and gray source rows. Preserve the native actual
  total; show the separately captured host/jewel values and each source's
  own affix range, name and tier. Identify sockets in native insertion order.
- Support variable IAS/Life/flat maximum damage/Enhanced Damage contributions,
  socket-only scalar properties and paired elemental endpoints. Normalize Life
  using the active stat shift; use leaf operands for consumed ED and computed
  weapon damage rather than treating the aggregate as an additive modifier.
- Resolve rune/gem effects from active compiled definitions and host context
  through the public table service. Add source labels such as `[Socket 1]
  [El Rune]`; fixed fire endpoints stay distinct from a random scalar roll.
- Recover the observed completed preserved-item corruption outcome from its
  saved roll and applicable active recipe. Split only proven fixed additive
  bonuses and label them `[C]`. Pending/invalid states and converted rares gain
  no guessed corruption contribution. Enchantments, runewords, conditional
  lists, unresolved unique choices and unsupported transformations retain the
  prior display; this candidate does not infer `[E]` from enchantment counters.
- Seven automated suites pass, including the full production capture/label/
  merge path and malformed ownership, cycles, capacity, metadata-revision and
  total mismatch controls. Local replay of captured compiled tables confirms
  the corruption ED50/Life Steal8 winner and El's context effects. Four new
  byte witnesses match captured code. User screenshots confirm Stone Fang's
  combined ED bounds/host and socket rows, socket-only Cold Absorb, and the
  preserved Cruel sword's ED and Life Steal corruption labels. User confirms
  releasing Ctrl restores normal tooltips. Converted-rare and other socket
  fixture rendering remain unconfirmed for this candidate; no rev27 release.

## [1.3.1+rev.26] - 2026-10-12 (local candidate)

- Add a guarded fixed-IAS socket breakdown for magic/rare hosts with matching
  fixed affix definitions. Preserve the actual combined total above gray
  source rows; keep the host affix label and identify the inserted contribution
  as `[Socket 1]` (native insertion order).
- Capture socket-owned extended stat lists and verify inventory order, owner,
  parent, identity/count and `host + sockets == total`. Decline partial reads,
  cycles, conditional/timed host lists, disagreement and variable host rolls.
  Existing tooltip behavior remains when the separate socket guards fail.
- Admit property function 8 only for IAS stat 93 in scalar affix bounds. No
  Item SDK dependency, new native hook, input remap or gameplay write.
- Six automated suites pass, including production affix/merge regressions and
  independent ownership/order/refusal fixtures. Live read-only Eagle Barb
  control confirms host IAS 20, inserted jewel IAS 15 and total 35; the new
  displayed breakdown remains pending a restart/visual check. Variable jewel
  ranges and other modifier families are not yet implemented.

## [1.3.1+rev.25] - 2026-10-12

- Shorten the unique source marker from [Unique] to [U], including fixed-value
  modifier lines. Provenance/range calculations are unchanged.
- Set items retain native range annotations without added set source labels.

## [1.3.1+rev.24] - 2026-10-12

- Remove the indirect Chronicle Items dependency for hidden unique/set roll
  definitions. Exact tooltip selectors receive private visibility snapshots;
  shared flags, table rows, Chronicle discovery and save data stay unchanged.
- Preserve original getter calls and returns outside the qualified tooltip
  chain; reject disabled/out-of-bounds definitions and incompatible code.
- Five suites pass, including immutable originals, copied unique/set rows,
  invalid records and caller-chain exclusion. User confirmed standalone Lustwander ranges/labels and Heartbane ED range/Base display.

## [1.3.1+rev.23] - 2026-10-10

- Restore fixed Base headers when the gesture is held but Core creates no range
  clone, including a plain Sash with Defense2 (Base:2-2).
- Observe native controller hold at the guarded header entry; keyboard fallback
  reads the native modifier query. Input slots/events and QOL remain unchanged.
- Five suites pass, including nine-argument ABI forwarding and native keyboard/
  controller hold/release fixtures. User confirmed a plain Sash shows its
  Base range while Ctrl is held and returns to the normal header on release.

## [1.3.1+rev.22] - 2026-10-10

- Fix variable Defense headers reverting to Core's range-only replacement.
  Core restores range TLS before publishing Defense; use the saved receiver/text
  proof from the guarded builder instead of requiring that expired context.
- Keep focus/panel gating and fail open on proof mismatch. Input queries remain
  native. Regression fixture covers restored context and invalid proofs.
- User confirmed the Iceblink Defense header fix in game. All five automated
  suites pass, including restored-context and invalid-proof regressions.

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
