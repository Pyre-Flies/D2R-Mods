# Controller QOL changelog

## 1.3.1+rev.73 - 2026-10-09

- Promote the rev.72 beta behavior to a stable release after the user reports
  the current features are working well. No gameplay or aim-tuning changes.
- Retain quiet aim logging by default, Reimagined-oriented leading baselines,
  native NPC/object interactions and offline/remote item-action paths.
- Package with fresh Item Roll Ranges rev.21 and Map Assistance rev.1 builds.
- Fresh Release builds pass Controller QOL 28/28, Item Roll Ranges 5/5 and Map
  Assistance 2/2 suites, including DLL artifact checks. Runtime ZIPs are audited
  for layout, checksums and privacy. Broad user confirmation is not a measured
  handheld benchmark or proof that earlier full-Ladder exit crashes are fixed.

## 1.3.1+rev.72-beta.1 - 2026-10-08

- Add `[aim] verbose = false`. Omitted settings also default false, so existing
  configs become quiet without replacement. `true` restores bounded detailed
  traces after restart, independently of the debug HUD and other QOL logging.
- Skip logging-only counters, native identity reads, comparisons and formatting
  in candidate/lookup/scoring/cast/UI diagnostics when quiet. Cast messages are
  also formatted only within their output limit. Preserve functional category
  validation, target selection, cast-state updates and HUD measurements.
- Keep concise startup status and compatibility/save-failure warnings visible.
  No native hook, address, aim tuning or input behavior changes.
- All 28 automated suites pass. Quiet-mode scoring emits zero traces and leaves
  logging counters untouched; verbose mode retains its eight-message score cap.
  Enemy/NPC scoring, strict flag parsing and packaged defaults pass. Handheld
  frame-time benefit still needs live testing.

## 1.3.1+rev.71-beta.1 - 2026-10-08

- Package Reimagined-oriented leading baselines for all 240 catalog IDs: 42
  projectile estimates and 198 explicit zero-lead choices. Preserve 33 prior
  estimates, add nine reviewed projectile families, and retain tested ice caps.
  New additions remain gameplay estimates; variant detection is deferred.
- Make Reimagined active replacements 18/134/136/141 R3-toggleable but off by
  default; 136/141 use explicit snap overrides. Keep ten enabled defaults.
- Package deadzone 0.22, speeds 8.0/48, acceleration 0.35, ground color #C2B596,
  lock color #CC9C52F2, and thickness 2.0; compiled style fallbacks match.
- User confirms rev.70 NPC first-press interaction and Shock Web improvement.
- All 28 automated suites pass, including embedded baseline/default parsing,
  R3 eligibility, prediction limits, native guards and DLL ABI/exports. Local
  runtime ZIP checks pass; new estimates await gameplay feedback.

## 1.3.1+rev.70-beta.1 - 2026-10-08 (local candidate)

- Preserve native scoring for NPC/noncombat target categories after snap aiming;
  only ordinary monster categories enter the enemy snap/leading cache. Keep
  Telekinesis object/item preparation. Guard the native category classifier and
  score argument path; no additional hooks or native calls. This addresses a
  likely cause of NPC interaction requiring a second A press; live confirmation
  remains pending.
- Remove Shock Wave from local leading trials without changing its R3 setting;
  increase Shock Web from 100 to 110 ms/tile (+10%), retaining default caps.
  User reports Tornado, Flame Wave, Charged Bolt and Double Throw feel good;
  Twister coverage makes leading hard to assess. Other tuning is unchanged.
- All 28 automated suites pass, including category isolation, snap-cache
  invalidation, aim gating and DLL ABI/version checks. Live NPC interaction
  and revised Shock Web tuning remain pending.

## 1.3.1+rev.69-beta.1 - 2026-10-08 (local candidate)

- Add local leading trials for Double Throw, Shock Wave, Flame Wave, Shock Web,
  Charged Bolt, Twister and Tornado (34 configured IDs total). Use installed
  primary/child missile and weapon mappings for estimates; retain default limits
  and all skill-enable states. Wandering/spread skills remain exploratory; defer
  Frozen Orb burst and Molten Boulder startup timing. Configuration validation
  passes; new trials need gameplay checks and require no DLL replacement.
- Tune local Holy Bolt/Miasma Bolt leading from 60 to 68 ms/tile (+13.33%) and
  Blade Fury from 55 to 69 (+25.45%) following user tests. Update commented stock
  examples; preserve existing caps and other settings. Config validation passed;
  the revised estimates await live confirmation. No DLL change required.
- Extend local leading configuration to Lightning, Chain Lightning, Teeth, Holy
  Bolt, Blade Fury and Miasma Bolt using inspected missile-speed estimates. Group
  leading entries by class comments with readable names; keep enable/targeting
  and ice limits unchanged. Add commented stock examples. Configuration checks
  pass; these six additions await live testing and require no DLL replacement.
- User confirms improved ice leading and excludes Slow Missiles compensation
  from scope.
- Add optional per-skill `[aim.leading_max_ms]` (100..1500 milliseconds) and
  `[aim.leading_max_tiles]` (1..8 world tiles). Missing entries preserve the
  original 600 ms / three-tile limits. Limits do not enable a skill or leading;
  R3 preserves them alongside the existing travel estimates.
- Local trial raises only Ice Bolt/Ice Blast to 1200 ms / six tiles after the
  user reported under-leading and the Ice Bolt trace reached the three-tile cap.
  Keep their 100 ms/tile estimates and other skills' tuning unchanged.
- All 28 suites pass, including per-skill isolation, cap/range enforcement,
  invalid configuration, R3 preservation and observed slow/frozen enemy motion.
  Ice tuning remains pending live confirmation. Slow Missiles projectile-speed
  compensation is not implemented; record verified data and remaining runtime
  questions in `docs/PROJECTILE-LEADING.md`.

## 1.3.1+rev.68-beta.1 - 2026-10-08 (local candidate)

- Preserve enabled nearby priority objects when native controller interaction
  arbitration would discard them in favor of a bound combat skill. The previous
  comparison hooks only handled object-versus-loot ranking, which did not cover
  opening a chest surrounded by enemies.
- A separately guarded GetInteractionTarget wrapper runs the original once and
  only recovers a null result through the current native Selected(Interact)
  entry. Only an enabled supported object within the configured priority radius
  qualifies. Existing native results, mouse UI, modified pickup, ordinary loot
  and unsupported/out-of-range objects retain native behavior. No object-use
  packet, scheduler, shared table mutation or persistent unit pointer is added.
- Add policy coverage for combat-null recovery, each supported object family,
  boundary/invalid distances, modifier and mouse isolation, shutdown, retained
  native results and native selection rejection. Live enemy-surrounded chest
  behavior remains pending. Record the reviewed path and byte guards in
  `docs/NEUTRAL-A-2026-10-08.md` and `interaction-priority-evidence.json`.
- User reports rev.67 projectile leading feels better; tuning remains empirical.
- Extend the local leading trial to Fire Bolt, Ice Bolt, Ice Blast, Glacial Spike,
  Poison/Plague Javelin, Lightning Bolt and Lightning Fury. Scale initial estimates
  by the inspected missile speeds and preserve existing skill enable states.
  Add commented configuration examples; no new DLL required for this expansion.
  New skills' leading accuracy remains unverified in gameplay.

## 1.3.1+rev.67-beta.1 - 2026-10-08 (local candidate)

- Add optional per-skill projectile leading through `[aim.leading]`: integer
  travel-time estimates in milliseconds per world tile, 0..200; missing/zero
  leaves current-position snapping unchanged. Leading does not enable a skill.
- Estimate movement from existing copied monster observations, require two
  agreeing sample windows, and fall back on stale samples, abrupt turns, stops,
  large jumps or invalid/out-of-range predictions. Cap prediction at 600 ms and
  three tiles. Select the target using its actual position, then adjust only an
  active snapped cast's destination. Keep the enemy lock marker on the enemy.
- Telekinesis's native unit route, ground-only casts and Whirlwind are excluded.
  No native hook, direct function call, persistent unit pointer or scheduler was
  added. R3 preserves leading preferences independently of enable state.
- All 28 automated suites pass, including motion discontinuities, stale samples,
  distance scaling, lead/range caps, target identity resets, configuration bounds
  and R3 rewrites. Live accuracy and tuning remain pending; no crash fix claimed.

## 1.3.1+rev.66-beta.1 - 2026-10-08 (local candidate)

- Prepare Telekinesis object/item scores while the armed cursor is idle after
  any enabled aim skill, so the first Telekinesis press after Teleport or another
  spell need not warm up object selection. Different active skills still retain
  their scoring behavior; disabled Telekinesis and snapping-off stay native.
- Reduce repeated selected-target diagnostics and include the previous preview
  skill. Label the native candidate return `nativeResult`: false is not proof of
  object ineligibility, as rev.65 logs show those objects subsequently selected.
- User confirms rev.65 Telekinesis interaction, with a two-tap issue. Record the
  separate leave-game access violation in `docs/CRASH-TRIAGE-2026-10-08.md`;
  neither attribution nor a crash fix is established.
- All 28 automated suites pass, including first-press preparation policy and DLL
  ABI/version checks. Deployed to Ladder with matching SHA256 and a rev.65 backup;
  the user subsequently reported that both Telekinesis and Fire Blast feel much
  better. This validates the tested targeting experience, not every item/object,
  other placement skill, offline path or leave-game stability.

## 1.3.1+rev.65-beta.1 - 2026-10-08 (local candidate)

- Default enabled ground attacks/traps to enemy snapping: Fire Blast, Shock Web,
  Blade Sentinel, Charged Bolt Sentry, Wake of Fire, Lightning Sentry, Wake of
  Inferno, Death Sentry, Fissure, Volcano and the three Warlock sigils. These skills
  still start disabled; R3 enables the chosen targeting mode. Explicit per-skill
  ground overrides remain effective. Movement, wall and minion placement defaults
  are unchanged.
- Preserve Telekinesis's native selected-unit and Lookup results instead of
  converting its item/object operations into coordinate casts. Apply reticle
  geometry only to naturally enumerated enemy/item/object candidates while this
  enabled snap skill is active or previewed; native eligibility and dispatch
  remain in control. Bounded `QOL/AimUnit` diagnostics support live validation.
  Custom monster lock markers are not presented as Telekinesis object locks.
- Set cursor defaults to initial speed 8, maximum speed 48 and acceleration time
  0.35 seconds. Existing explicit settings remain authoritative.
- Installed Ladder data and logs confirm Ice Barrage uses catalog ID 253
  (vanilla Psychic Hammer), independently of Shock Web 256. Record the numeric
  mappings and special-skill investigation in `docs/SKILL-CATALOG.md`.
- User reports Fire Blast remained at the cursor with R3 enabled in an earlier
  test, and Telekinesis failed for items/objects; enemy Telekinesis was not tested.
  Candidate behavior still requires live Fire Blast and chest/shrine/item checks.
- All 28 automated suites pass, including special-skill routing policy, explicit
  targeting overrides, disabled-skill isolation, existing hook admission, and DLL
  ABI/version/exports. These checks do not establish native object interaction.

## 1.3.1+rev.64-beta.1 - 2026-10-07

- Publish as a beta prerelease while full-Ladder crashes remain under investigation
  and final offline/persistence qualification is pending. The user confirmed the
  current candidate's improvements and vendor tome refill after supplying gold;
  the log records charge growth 91 -> 100. Earlier validation notes below describe
  the sequence of candidates, not additional claims of final-build coverage.

- Admit the reviewed Ladder Global Chat sender hook and Maps stat-reader hook
  through exact artifact, code and forwarding-chain checks. Restore compatibility
  admission for ID, stash/Cube requests and tome charge reads without bypassing
  either plugin. Unknown hooks still refuse safely; original unhooked paths and
  authoritative/offline scheduling are preserved. Includes vendor tome reads.
- All 28 automated suites pass, including production unhooked guard checks
  without Ladder plugins, forwarding-chain corruption cases and DLL ABI/exports.
  Read-only Ladder inspection matches the new profiles and continuations; actual
  ID/transfer behavior and final-build offline use still need live validation.
- The user confirmed improved cursor behavior with the acceleration candidate.

- Preserve right-stick acceleration across consecutive UI callbacks sharing the
  same clock timestamp. Zero elapsed time causes no movement; release and large
  direction changes still reset fine control. A regression test failed on the
  previous implementation and passes after the correction; all 27 suites pass.
  The user reports very slow constant motion in Ladder, including town. This
  timing defect is reproduced automatically; live symptom resolution is pending.

- Add vendor LB+R3 on Identify and Town Portal scrolls to request native
  Shift-buy into matching carried tomes. Show "Fill Tome", require a non-full
  Inventory tome, and observe charge growth without automatic retries. Scrolls
  remain excluded from belt placement. Native pricing and capacity rules apply.
- Own legacy move, Cube and ground-pickup payloads outside scheduler callbacks.
  Session/unload cancellation frees discarded work; non-reused tokens prevent
  late callbacks from claiming a new request, including remote fallback races.
- Bind early Shared potion LB+A to a fresh inventory snapshot of the exact
  controller-selected item before scheduling. Retain strict source checks after
  binding and log metadata differences. The observed first-press refusal's
  precise cause is not proven; this correction still needs live qualification.
- The user confirmed the preceding embedded Cube/Shared potion candidate works.
  The new cleanup and vendor-tome candidate passes all 27 automated suites,
  including ownership/cancellation, exact Shared focus, scroll/tome policy and
  DLL ABI/export checks. Vendor purchasing and first-press behavior remain
  pending live validation.

- Fix LB+X on the Cube grid embedded beside Gems/Materials/Runes: submit an
  eligible item directly to advanced storage using native source page 3, with
  Cube source observation and no Inventory fallback for ineligible items.
  Standalone Cube LB+X retains its move-to-Inventory behavior.
- Admit the embedded Cube view for LB+A potion placement without requiring the
  standalone Cube UI flag. Handle normal Shared Stash potion LB+A before native
  input because that view can pick up the item without emitting ItemInteraction.
  Recheck the real selected item/page before submission; consume accepted A
  until release. Add detailed belt refusal reasons for further qualification.
  These fixes await live validation; all 26 automated suites pass.
- User validation of the preceding candidate: LB+L3 deposits, identification,
  Personal Stash potions and standalone Cube potions work. Embedded Cube actions
  and normal Shared potion handling required the follow-up above.
- Add remote LB+L3 deposit-all with an Inventory snapshot, sequential native
  requests and source-removal observation. Stop on changed page/session/source
  or timeout; never retry an uncertain deposit. Retain the offline batch path.
- Extend direct remote ID to loose Identify scrolls and open Personal Stash
  sources/targets. Identify All also accepts a Personal Stash tome, with remote
  targets still limited to Inventory. Confirm loose-scroll disappearance
  separately from tome charge consumption. Cube/Shared supplies and modded
  scroll stacks remain excluded.
- Add remote LB+A belt placement from ordinary Personal/normal Shared Stash and
  open Cube slots, with native source-page and owner guards. Offline Cube uses
  existing Inventory staging; existing offline stash/Inventory routes and LB+R3
  scope are retained. All 26 automated suites pass, including new page ABI,
  scroll confirmation and deposit cancellation checks. Live validation pending.
- The user confirmed the preceding candidate's remote Identify All, Materials
  single-potion belt withdrawal and advanced-tab Cube withdrawal working.
- Add remote Identify All on an Inventory Identify Tome. Snapshot Inventory
  targets, send one direct native request at a time, and advance only after the
  target is identified and exactly one charge is consumed. Stop on changed
  identities, empty tome, session change or timeout. Remote Cube contents remain
  excluded; offline routes are unchanged.
- Route LB+A on a Materials rejuvenation to a single native belt withdrawal,
  including the counter widgets that do not emit ordinary item interactions.
  Consume accepted A presses until release; LB+R3 keeps its refill behavior.
- Route LB+Y on Rune/Gem/Materials counters through the native Cube withdrawal
  wrapper. Require a carried Cube, current controller selection, unchanged
  tab/season and destination quantity observation. No inventory staging or
  resubmission after timeout. New single-withdraw callbacks expire and cancel
  across sessions. These three additions await live validation.
- The user confirmed the preceding candidate's advanced deposits and empty
  Shared-cell protection working in the private-server session.
- Add the missing remote LB+X deposit route for eligible runes, gems and
  materials, including rejuvenations, from Inventory. Use the game's eligibility,
  current advanced owner and single native transfer request on ordinary and
  advanced tabs. Preserve the existing Cube route for ineligible items on an
  advanced tab; never retry a submitted deposit. Observe source consumption
  separately from destination confirmation, since counters change item identity.
- Reject stash shortcuts when the controller's selected widget/cell does not
  contain the tooltip target. This prevents an empty Shared cell from moving a
  remembered Inventory item. Remote transfers recheck selection before sending.
  All 26 automated suites pass and new guards match the live client; these two
  fixes await live behavior testing. The preceding candidate's ordinary
  Personal/Shared transfers and LB+Y Cube transfers are now user-confirmed.
- Correct remote-transfer admission to recognize the already reviewed Core
  relocation inside the Shared owner resolver. The previous exact-byte check
  disabled the entire client-transfer module, including Personal Stash, before
  any request could run. Live read-only inspection matches the existing guarded
  wrapper contract; transfer behavior still needs retesting. Direct single-item
  ID is now user-confirmed, with a logged target flag and tome charge 7 -> 6.
- Add guarded remote single-item identification through the native request
  builder, without entering tome targeting mode or activating the item grid.
  Use one charged Inventory tome and one Inventory target; wait for the target
  flag and exactly one consumed charge. Preserve offline SDK behavior and the
  configured offline native route. Remote bulk ID, loose scrolls and other ID
  containers remain unsupported by this candidate.
- Add `Unavailable`-only native client requests for ordinary Personal/Shared
  Stash transfers and explicit Cube transfers. Bind exact item identity and
  selected normal Shared page, preserve native eligibility and placement, and
  confirm the destination in client state. Cube deposits require a carried
  Cube; advanced materials, previous-season pages and bulk moves keep their
  existing routes. Never retry a submitted request or guess another destination.
- Cancel identification and new client-transfer queues on session changes;
  replace the single-ID heap payload with owned state and generation tokens so
  discarded authoritative callbacks cannot leak the request or keep ID busy.
  Release build and all 26 automated suites pass, including native ID argument,
  quantity, transfer identity/page and callback lifetime cases. These new paths
  await live validation; see [remote contracts](docs/REMOTE-ID-TRANSFER-TRACE.md).
- Fix competing native A pickup during a controller ground-loot chord. Preserve
  raw A for slot selection, consume game A until release, and exclude inventory,
  stash and menu contexts. Mouse input and ordinary A remain unchanged. Remove
  identify/portal scrolls from potion-to-belt and refill candidates.
- Review corrections: keep smart-deposit native pointers inside their SDK
  callback, stop sticky-slot log appends after truncation, and replace the
  unguarded Core UI-state fallback with the existing guarded game getter.
  All 24 automated suites pass; the new fixes await live validation. See
  [review findings](docs/REVIEW-2026-10-06.md) for remaining remote scheduler and
  session-cancellation issues.
- Add an `Unavailable`-only client route for inventory potion-to-belt placement
  and inventory belt refill. Use UI-thread SDK snapshots, guarded client unit
  lookup and the existing native stored-item request; wait for the same item to
  appear in the belt. Remote stash-source belt withdrawals remain unsupported.
  The private-server log reproduced rejected belt scheduling; 24 automated
  suites pass after this correction. The user confirmed inventory potion
  placement, but also reproduced scroll refill; its correction awaits retest.
- Add a guarded client ground-loot route when the authoritative game scheduler
  returns `Unavailable`: populate A/X/Y/B/R1/R2/L2 from visible client placards
  and submit the native client interaction request. Preserve offline scheduling,
  rarity/distance ranking and stable assignments; revalidate item identity,
  range, collision, visibility, hold/session and request age before submission.
  Keep labels until client state reflects removal; do not claim pickup success
  from native return or automatically retry. Inventory management is unchanged.
  All 24 automated suites pass. The user reports LB ground labels/pickup now
  appear to work; the 18:40 trace records two submitted requests followed by
  their items leaving the assigned slots. Broader ground regression tests remain.
- Add rate-limited diagnostics for remote controller snapshots, label UI refresh,
  and controller UI scheduling failures. Live private-server traces confirm LB
  capture and label UI refresh, but reject authoritative loot scheduling.
- Log failed ground-loot scheduling when diagnostics are enabled and free the
  pickup request when scheduling fails. Release build and all 23 suites passed.

## 1.3.1+rev.63 - 2026-10-05

- Restore armed idle reticle-circle acquisition before the first snap-skill
  lookup, addressing the reproduced behind-character Fireball fallback. Preserve
  active ground/disabled skill gates and native candidate checks. Idle native
  monster ranking changes deliberately. The user confirmed improved Fireball
  snapping, ground-targeted Teleport and native targeting for disabled aim skills.

- Add bounded candidate/preview/lookup diagnostics for facing-dependent snap
  reports. The trace confirmed active circle acceptance occurred after the first
  lookup; the correction restores pre-cast acquisition. All 23 suites passed.

## 1.3.1+rev.62 - 2026-10-05

- Refresh normalized controller input and label display on the client UI
  scheduler. Remote TCP/IP clients have no authoritative game scheduler, so the
  previous refresh could leave LB snapshots invalid and labels unrecovered.
  Direct ground-loot and SDK item mutation paths still require local authority;
  this change does not make those paths supported on remote Ladder clients.
  The user confirmed the updated DLL works offline. Live Ladder validation
  remains pending; signed packages are unchanged.

- Narrow the controller UI check to filtered-label blocking, preserving the
  existing ground-modifier suppression path independently. Mouse pickup remains
  unaffected by filtered-label blocking. Release build and 23 automated suites
  passed; offline behavior was user-confirmed.

## 1.3.1+rev.61 - 2026-10-05

- Apply native filtered-item pickup blocking only while D2R's controller UI is
  active. Mouse and keyboard pickup bypasses the guard even with
  `block_filtered_pickup = true`; a connected idle controller does not enable it.
- Release build and all 23 existing automated suites passed. The user confirmed
  mouse pickup works with placards off after installing the fix. Controller
  filtering and input-switching validation remain outstanding.

## 1.3.1+rev.60 - 2026-10-04

- Package the default TOML at `d2rloader/config/` beside `d2rloader/plugins/`
  so runtime ZIPs extract directly into the game directory.

- Use `true`, `false`, and `"disabled"` for custom skill enable settings. Enabled
  custom IDs default to snap targeting; legacy ground/snap strings remain readable.
- Add optional `[aim.targeting]` ground/snap overrides for catalog and declared
  custom IDs. Overrides retain enable/lock state and survive R3 off/on cycles.
- Preserve targeting entries during R3 config saves; reject duplicate, malformed
  and undeclared targeting IDs. No native hook or skill-tree admission changes.
- Automated validation covers parser compatibility, override independence and
  R3 persistence; live testing remains separate.

## 1.3.1+rev.59 - 2026-10-04

- Accept `"disabled"` in class skill sections to lock a skill's aim off and ignore
  R3. `false` remains off but toggleable; `true` remains enabled. Locked skills
  display `Auto-aim: DISABLED` and have no icon marker or R3 hint.
- Lock the 30 cataloged passive skills off in the default config. Existing configs
  retain their choices; local migration converts only passive entries set false.
- Validate locked-state parsing, native routing, refusal of R3 document changes,
  runtime override protection and preservation during other skill toggles.
  Automated checks and live-game validation are reported separately.

## 1.3.1+rev.58 - 2026-10-04

- Fix first R3 presses being discarded when entering/re-entering skill focus.
  Observe rising R3 events in the existing admitted normalized-input hook, with
  a monotonic sequence consumed once on the UI thread. Polling fallback retains
  button history across focus gaps and does not treat stale input as release.
- Brighten and slightly strengthen the skill-tree crosshair markers. Add quoted
  hex `ground_reticle_color`, `lock_reticle_color` and a 0.5..4
  `reticle_thickness` multiplier for gameplay reticle strokes and outlines.
- Preserve hashes inside quoted config values while stripping actual comments,
  including during R3 saves. Add edge/focus, color, thickness and preservation
  checks; live first-press and appearance validation remains separate.

## 1.3.1+rev.57 - 2026-10-04

- Show a small brass crosshair inside the upper-right corner of each enabled
  visible skill icon. Disabled skills have no marker. Use actual icon bounds,
  ancestor scale and visibility; hidden tabs are excluded rather than using a
  fixed class/grid layout.
- Move highlighted-skill Auto-aim ON/OFF and R3 toggle instructions into the
  existing control strip. Draw through the SDK overlay without new hooks or
  native icon/text edits. Layout mismatch disables these indicators only.
- Record the read-only layout contracts and native-observed geometry fixture.
  Automated geometry/build checks and live visual validation are separate.

## 1.3.1+rev.56 - 2026-10-04

- Add R3 per-skill aim toggling while a catalog skill is highlighted in the
  controller skill tree. Show an Aim ON/OFF hint and save the numeric setting
  through the SDK, preserving comments and other configuration choices.
- Apply saved toggles immediately through atomic per-skill modes. Reject unknown
  mod IDs and ambiguous configuration; holding R3 never repeats the toggle.
  Add `skill_tree_toggle_enabled=false` to disable the shortcut.
- Guard the newly inspected focus path independently; mismatch disables only
  this shortcut. Live ID tracking confirmed for Magic Arrow and Multi Shot;
  UI toggle/persistence behavior remains pending a test of the new build.

## 1.3.1+rev.55 - 2026-10-04

- Fix idle Selected target queries clearing right-stick intent and hiding the
  reticle. Query IDs are no longer treated as evidence of an unsupported cast.
  Active unsupported skills and the optional cast observer retain their resets;
  disabled/mismatched queries continue returning the original native target.
- Add regression coverage across disabled catalog entries. Automated validation
  and deployment are recorded in the cast observer compatibility document; live
  reticle recovery and rapid skill transitions still require confirmation.

## 1.3.1+rev.54 - 2026-10-04

- Make the cast observer at `0x4FDB40` optional. Keep it enabled by default;
  changed bytes or hook ownership reject only the observer, retaining cursor,
  snapping and reticles. Add `cast_observer_enabled=false` to leave the site free
  regardless of load order. Preserve Whirlwind and other skill settings.
- Extend disabled-skill intent reset through the existing Selected target-request
  path while idle, alongside active-skill/UI checks. The original getter still
  runs once; native casts remain unchanged. Observer fallback omits actual-cast
  diagnostics, LAST CAST and Teleport displacement samples.
- Test real installer control flow against copied signature fixtures: clean cast
  site, foreign prefix, explicit opt-out, SDK hook refusal and essential guard
  failure. Full 23-suite validation and local installation recorded separately;
  live interaction with the Whirlwind rework remains unverified.

## 1.3.1+rev.53 - 2026-10-03

- Expand numeric class sections to all 240 class skills, including Warlock.
  Preserve the ten tested default enables; the remaining 230 default false for
  review, including Corpse Explosion, Nova and Poison Nova. Add readable names
  and preliminary behavior comments without claiming new runtime compatibility.
- Class toggles accept all catalog IDs, with provisional ground/snap modes when
  explicitly enabled. Keep class grouping cosmetic, duplicate-ID rejection,
  legacy ten-skill name migration and 32 additional custom-mode entries.
- Increase both configuration read buffers to 64 KiB so the complete commented
  catalog does not exceed the former 16 KiB limit. No new native sites or ABI.
- Full-catalog/defaults, extended-buffer and duplicate/mode policy tests added;
  live behavior of newly enabled skills still requires individual validation.

## 1.3.1+rev.52 - 2026-10-03

- Activate manual aim through deliberate right-stick input. Preserve the cursor
  after release; disabled/unlisted skill casts release custom locks and restore
  native targeting, with release/retilt required to resume manual aiming.
- Add class-organized numeric skill toggles with name comments, without character-class restrictions,
  and explicit numeric custom skill entries with ground/snap/disabled modes.
  Keep existing ten-skill defaults; unknown skills are never enabled implicitly.
  Legacy name keys remain readable for migration; additional IDs require explicit
  targeting modes in the custom section.
- Remove idle native score overrides. Idle lock markers use native observations
  only; native-excluded candidates may acquire a marker when the cast begins.
  Keep the existing Guided Arrow correction until manual aiming actually owns it.
- Automated policy/configuration and build validation recorded in the rev.52
  implementation note. Live aiming and custom skill compatibility remain pending.

## 1.3.1+rev.50 - 2026-09-30

- Integrate aim.20 into QOL as an opt-in `[aim]` module: right-stick cursor,
  ten explicit skills, circular snapping, Whirlwind pass-through, subtle reticles
  and idle lock preview after Teleport/Leap. All reviewed tuning keys are retained.
- Default aim off; the global QOL switch also gates initialization. Invalid aim
  configuration disables only aim. Separate TOML sections and a larger config
  buffer prevent aim settings from affecting QOL's existing settings.
- Replace inter-DLL Guided Arrow ownership with an internal query; validate
  existing contact relays against QOL's exact wrapper address. No new native sites.
- Preserve disabled/failed-init pass-through and SDK-owned callback/hook cleanup.
  Refuse aim initialization if the standalone prototype is loaded.
- All 23 suites pass, including merged artifact/defaults, section isolation and
  disabled/invalid-config zero-service-registration checks. Standalone aim.20's
  post-Teleport marker was user-confirmed; merged runtime remains unverified.


- Let the enabled, installed Controller Aim Test prototype own Guided Arrow's
  coordinate distance. The existing adjacent-point-to-20-tile correction remains
  active when the prototype is absent, disabled, outside a session or uninstalled.
- Query its private versioned ownership export while holding a temporary module
  reference; no cached function pointer, new native hook, address or game-data edit.
- All 21 automated suites pass. Active coexistence and short-range Guided Arrow
  placement require live validation with aim.14.

# Rev.48 Runes embedded-Cube routing

- Treat selected stash tab 4 (Runes) like the existing Gems and Materials pages for inventory LB+X: try native advanced-storage deposit first, then send noneligible items to the embedded Horadric Cube.
- Preserve native Rune eligibility, Cube-source withdrawal, ordinary Personal/Shared stash routing, and the explicit LB+Y Cube action. No hook, RVA, packet, or native ABI changes.
- All 21 automated suites pass. The user confirmed the byte-identical installed build behaves correctly in game.

# Rev.47 Guided Arrow controller targeting

- Project Guided Arrow's controller-only adjacent ground coordinate to 20 tiles while preserving its direction, allowing the native acquisition and homing logic to search at a useful distance.
- Reuse Controller QOL's existing guarded action `0x05` hook. Other skills, mouse casts, target-selected casts, already-distant points and the original packet remain unchanged.
- Guard the selected-skill accessor independently. A mismatch disables only this projection and retains all other Controller QOL behavior.
- All 21 automated suites pass, including projection policy, action-hook isolation and DLL artifact verification. The user confirmed the merged controller ground-shot acquisition and homing behavior in game.

# Rev.46 well-priority classification candidate

- Extend the existing default-on `prioritize_shrines` family to the 14 environmental Fountain/Well classes that use OperateFn 22 but do not carry the shrine SubClass bit. The allowlist is exact to the inspected active ObjectsTxt table and avoids promoting unrelated SubClass-0 or SubClass-32 objects.
- Keep the same priority hooks, native eligibility checks and `portal_priority_distance`; no new hook or RVA. Automated classification coverage added; visible well selection remains pending live validation.

# Rev.45 shared class-getter compatibility candidate

- Admit the native object-class getter at game `0x349860` when it is original or begins with a five-byte `E9` whose target is committed executable memory, while requiring the remaining reviewed 27-byte function body at `0x349865` to match exactly.
- Do not patch or bypass the existing owner. Priority classification calls the current shared entry, so the predecessor detour remains in the chain. Unknown detour forms, non-executable targets, or changed function tails disable priority and preserve native targeting.
- The captured Auto Deposit runtime image now passes all 20 portal/priority evidence checks. Unit tests cover original, accepted E9, non-executable E9, and unknown-prefix refusal; live priority behavior remains pending.

# Rev.44 shrine/chest priority and batch-feature gates candidate

- Add `prioritize_shrines = true` using the active ObjectsTxt shrine SubClass bit and `prioritize_chests = false` using an exact active-table normal-chest class allowlist. Quest chests, hidden stashes, scenery, bodies, urns and other generic treasure objects are not promoted.
- Add `identify_all = true` and `quick_deposit = true`. Disabling either removes its controller execution path and its header/tooltip hint while retaining single-item Identify and ordinary LB+X transfer behavior.
- Reuse the existing priority hooks and range setting; no new RVA or interception site. Automated classification and artifact validation added; live shrine/chest selection remains pending.

# Rev.43 stash and waypoint interaction-priority candidate

- Extend neutral A/Interact priority from portals to the actual town stash (`ObjectsTxt` Bank class 267) and waypoints (SubClass bit `0x40`). Hidden stashes, treasure chests and other interactable objects remain native.
- Preserve the selected stash/waypoint against later ground-item candidates, while enabled LB direct-loot gestures retain ownership of loot selection. The priority system is now independent of `ground_pickup`; disabling LB shortcuts no longer disables neutral A priority.
- Reuse the existing guarded contact, score, comparison and Interact-range paths with no new hooks or RVAs. Add independent `prioritize_stash_boxes` and `prioritize_waypoints` switches; all three object types share `portal_priority_distance` for backward compatibility.

# Rev.42 Auto Deposit pickup compatibility candidate

- Leave the shared native pickup entry at game `0x471950` untouched. Redirect seven guarded direct game CALL sites through the filtered-pickup wrapper so allowed pickups still traverse an earlier Auto Deposit hook and QOL no longer requests the same MinHook target.
- Keep the wrapper inert unless all seven call sites publish successfully. Exact five-byte witnesses and automated target-resolution checks fail open on a mismatched game build. Live A-button filtering with Auto Deposit remains pending.

# Rev.41 filtered-A / direct-loot gate candidate

- Activate the native filtered-item pickup guard when `block_filtered_pickup = true` even if optional direct ground pickup is disabled. Bare A remains native for visible items; LB loot shortcuts remain disabled when `ground_pickup = false`.
- Restore the independent `ground_pickup` execution gate throughout modifier suppression, placard shortcuts, observer scheduling and queued pickup work. Filtered-A-only mode no longer activates LB ground-loot shortcuts.

# Rev.40 header candidate

- Keep Stash All and the two-row header layout visible over empty stash slots. Bulk hint availability now comes from open-stash context rather than expiring item-tooltip snapshots; stale item-specific hints still disappear. No gameplay changes.

# Rev.39 header candidate

- Add modifier+L3 Stash All beneath Open Cube while stash is open. Align recognized native controls to a consistent top row with contextual QOL shortcuts below; tighten chord spacing and add column gutters. Compact the English native Drop/Hold-to-Move label while retaining hold semantics and localized text.
- Rev.38 instant full-inventory stash confirmed working and retained. Header visual validation pending.

# Rev.38 experimental candidate

- Submit all eligible bulk-stash inventory items in one authoritative game update, then verify submitted identities together. No per-item UI round trips, automatic retries, or new hooks. Stop further submissions on refusal/changed identity and still verify earlier submissions.
- Rev.37 sequential deposit confirmed working. Rev.38 whole-inventory timing and destination behavior require live testing.

# Rev.37 candidate

- Add LB+L3 while stash is open to deposit advanced-storage eligible inventory items using the existing LB+X native helpers. Sequential submissions, exact identity checks and source-removal observation; no ordinary stash/Cube fallback.
- Capture L3 only for the accepted stash chord, preserving normal Open Cube elsewhere. Automated checks recorded in docs/BULK-STASH-REV37.md; live verification pending.
- User confirmed rev.36 Cube bulk identification works.

# Rev.36 candidate

- Include unidentified Horadric Cube items in SDK Identify All. Tome sources remain inventory/open Personal Stash; Cube and Shared tomes remain excluded. Preserve exact target identity, per-item charge verification and stop-on-failure behavior. Native compatibility mode retains inventory-only targets.
- Rev.35 SDK identification and normal L1 navigation confirmed working by the user. Rev.36 live Cube verification pending.

# Rev.35 candidate

- Restore SDK-first single and bulk identification; share stat-70 debit verification and rejected-identify compensation. Bulk stays bound to the highlighted inventory/Personal tome and yields between items. Explicit `native_identify = true` retains native compatibility mode; no automatic retry after SDK mutations.
- Remove L1 spatial inventory leaps and synthetic directional pulses, preserving normal navigation and focused-item shortcut tracking.
- Automated validation and deployment recorded in docs/IDENTIFY-NATIVE-REV26.md; live rev.35 validation pending.

# Rev.34 candidate

- Wait for the native tome-block check to clear before each bulk use, replacing the unsuccessful fixed pause. Bound readiness waiting and preserve one-charge confirmation. All 19 suites pass; live batch validation pending.

# Rev.33 diagnostic candidate

- Log guarded native tome eligibility and cursor state around each bulk tome-use attempt to locate the second-item refusal. No eligibility bypass; all 19 suites pass, live diagnosis pending.

# Rev.32 candidate

- Add bounded UI-callback pacing between bulk identifications and target/stage diagnostics for the second-item timeout. All 19 suites pass; pacing hypothesis requires live testing.

# Rev.31 candidate

- LB+A on an eligible Identify Tome identifies main-inventory items sequentially using that tome, one native charge per confirmed item. Adds Identify All controller hints and stops on empty charges, changed identities or interrupted native work. All 19 suites pass; live validation pending.

# Rev.30 candidate

- Exclude Cube and Shared Stash tomes; prefer charged inventory tomes, with open Personal Stash as a fallback. Extend native tome-use to Personal Stash source/target grids with fresh identity and tab checks. All 19 suites pass; user confirmed QOL Personal Stash identification works.

# Rev.29 diagnostic candidate

- Explain native identification admission/refusal with provider hashes, precise byte/slot checks, scheduling results and route/container summaries. Native rejection no longer masquerades as an SDK edit failure. Compatibility and consumption behavior unchanged; all 19 suites pass, remote testing pending.

# Rev.28 candidate

- Defer missing-focus recovery after loot-filter inner tab changes to SDK UI callbacks, covering both Rarity/Quality-to-Items and expanded Misc-to-Equipment transitions. Preserve healthy focus and native entry behavior. All 19 suites pass; live validation pending.

# Rev.27 candidate

- Recover missing child focus after Equipment-to-Items bumper switching in the loot-filter editor, using the existing guarded native list entry. Pending live validation.
- Native inventory tome identification and lower filter navigation confirmed locally in rev.26.

# Rev.26 candidate

- Main-inventory LB+A with a tome uses native hold-A and target activation, with charge/identity confirmation and no SDK quantity edit. Live candidate validation pending; other containers retain existing behavior.

# Rev.25 candidate

- After the legacy filter section route fails, resolve the loader-owned merged item panel and use native list focus entry. Live navigation validation pending.

# Rev.24 candidate

- Scoped Down-to-native-section handoff for the focused loot-filter rule tab. No raw focus writes or additional hooks. Lower-list editing remains pending live validation.

# Changelog

## 1.3.1+rev.51 - 2026-09-30

- Enable controller aim by default in embedded configuration and when the aim
  section/key is absent. Explicit aim.enabled=false and qol.enabled=false still
  disable it. Invalid aim configuration continues to fail closed for aim only.
- No targeting or visual behavior changes. Updated configuration/default tests.


All notable changes to Controller QOL Updates are recorded here. Versions before
the GitHub migration are reconstructed from release and validation records.

## [1.3.1+rev.23] - candidate

- Remap the loot-filter rule editor's Equipment/Items tabs to LB/RB with matching glyphs through the existing scoped TabBar hook.
- Exclude loot-filter editing panels from world-loot shortcut scheduling.
- All 19 suites pass; installation and visible behavior pending. Lower-panel Gold/Potions controller focus is a separate unresolved issue reproduced with QOL disabled. See docs/LOOT-FILTER-EDITOR-INVESTIGATION.md.

## [1.3.1+rev.22] - 2026-09-27

- Map the native tooltip range query from RT to RB independently of Item Roll Ranges. Preserve its exact caller, shared slots and detailed-formatting interception.
- Update the scoped English Show Ranges header to RB when the remap is active. Keep unrelated button queries and submenu actions unchanged.
- All 19 suites pass; live QOL-only and combined-plugin validation pending. See docs/NATIVE-RANGES-REV22.md for the new guarded native query hook and compatibility limits.

## [1.3.1+rev.21] - candidate

- Mirror available item-tooltip QOL shortcuts beneath corresponding controller header controls, reusing the existing scoped glyph hook. Preserve native controls and device-specific glyph artwork.
- Fit copied draw rectangles with guarded native font metrics; expire item-specific hints and retain keyboard/mouse behavior. No new hooks or input changes.
- All 19 suites pass; visual spacing and coverage remain pending. See docs/CONTROLLER-HEADER-REV21.md.
- User confirmed rev.20 resolved the reported Chronicle-to-Quest crash reproduction.

## [1.3.1+rev.20] - candidate

- Remove three invalid conversions of opaque SDK player handles into native pointers in ground-loot callbacks. Skip when native player context is unavailable.
- Prevent tracked menu navigation from queuing or consuming world-loot shortcuts; retain Chronicle and Quest bumper navigation.
- All 19 suites pass. Live Chronicle-to-Quest crash retest pending; see docs/GROUND-CALLBACK-CRASH-REV20.md for evidence and limits.

## [1.3.1+rev.19] - candidate

- Chronicle inner tabs use LB/RB and matching indicators, while LT/RT remain outer-menu navigation. Extend the existing scoped TabBar hook; no additional hooks.
- Live widget/binding inspection completed; in-game remap validation pending. See docs/CHRONICLE-NAVIGATION-REV19.md.
- User reports rev.18 Shared Stash transfers are working.

## [1.3.1+rev.18] - candidate

- Pin the repository SDK to released v0.3.0 (ABI 4). Use capability-gated SDK transactions for normal Shared Stash LB+X transfers with explicit zero-based page selection.
- Capture/recheck selected page and player; reject stale or unknown pages and do not retry an SDK failure through native transfers. Preserve older-loader and remove-only seasonal paths.
- All 19 QOL suites, 5 Item Roll Ranges suites and 2 Map Assistance suites pass. Live page-4 selection and runtime capability inspected read-only; candidate transfer validation pending. See docs/SHARED-SDK-REV18.md.
- User confirmed rev.17 vendor refill works.

## [1.3.1+rev.17] - candidate

- Vendor potions gain LB+R3 Refill Belt / Buy Missing: first move matching inventory potions, then submit one native Shift-buy if compatible belt space remains. Revalidate player, merchant grid stock identity, native contracts and request age before purchase; never retry an unconfirmed buy.
- Suppress Transfer, To Cube, Identify and To Belt hints on merchant stock; retain LB+X Sell for player inventory.
- Add queued-task generation checks across cancellation/session changes. No additional hooks or packet implementation.
- All 18 automated suites pass, including DLL ABI/exports/defaults checks. Native Shift-buy behavior confirmed by the user; plugin-driven purchase and tooltip validation pending. See docs/VENDOR-BELT-REV17.md.
- User confirmed rev.16 potion compatibility works.

## [1.3.1+rev.16] - candidate

- Admit the reviewed Auto Belt Refill 0.35.9 stored-placement wrapper while preserving its hook and original trampoline. Original native path remains supported without that plugin.
- Use quiet read-only belt preflight checks; unsupported code produces a compatibility warning instead of a misleading SDK patch error.
- Seventeen automated suites pass. Combined live potion behavior pending. See docs/AUTO-BELT-COMPATIBILITY-REV16.md.

## [1.3.1+rev.15] - candidate

- Read tome charges from guarded native stat 70 instead of rejecting tomes from ItemInfo.quantity alone.
- Use SDK quantity edits with native readback before identification and checked charge restoration if identification fails.
- Add bounded charge comparison diagnostics and mismatch/rollback regression tests. All 16 suites pass; live charge and reporter validation pending.
- User confirmed rev.14 controller/keyboard label behavior works.

## [1.3.1+rev.14] - candidate

- Restrict always-on filtered labels and label-action suppression to the game's active controller mode. Keyboard/mouse Alt actions pass through natively.
- Restore controller labels after switching back from keyboard/mouse with labels OFF; retain rev.13 stale-display recovery.
- Rev.13 recovery was accepted by the user. Mixed-input validation for rev.14 pending.

## [1.3.1+rev.13] - candidate

- Recover stale controller ground-label display state through the guarded native refresh, after live confirmation that LB+A identification can leave labels enabled but their display flag off.
- Check on the SDK game thread at most four times per second; skip healthy labels, keyboard mode and intentional menu suppression.
- Sixteen automated suites pass; automatic live recovery pending. See docs/LABEL-REFRESH-REV13.md.

## [1.3.1+rev.12] - candidate

- Resolve stash/Cube transfer targets by exact identity in their source container; remove coordinate/code fallbacks that could select Charm Inventory items.
- Give open stash, Cube and vendor panels priority over visible custom-page bindings.
- Map Options Video/Audio/Gameplay tabs to LB/RB through the existing TabBar hook, with matching button indicators; retain LT/RT outer navigation.
- Sixteen automated suites pass. Live validation pending; label investigation remains open. See docs/LADDER-CONFLICTS-REV12.md.

## [1.3.1+rev.11] - 2026-09-26

- Remove unconditional per-item identification logging and the duplicate UI-thread consumable scan.
- Resolve the exact target and consumables in one authoritative SDK inventory walk.
- Distinguish unavailable enumeration, empty tomes, missing supply and mutation failures in a bounded diagnostic summary.
- Combine identify and charge debit where supported; handle final-charge boundaries with checked SDK operations and compensating edits.
- Remove unsafe same-code target fallback and raw native flag mutation fallback.
- Fifteen automated suites pass; live FPS, charge-boundary and reporter-mod validation pending. See docs/IDENTIFY-REV11.md.

## [1.3.1+rev.10] - 2026-09-26

- Hide controller item shortcuts while the game is in keyboard/mouse mode.
- Reuse the existing native input-mode query; no additional hooks.
- Fourteen automated suites pass. User accepted the update before packaging;
  a separate detailed input-switching test matrix was not recorded.

## [1.3.1+rev.9] - 2026-09-26

- LB+X transfers to/from the visible registered custom inventory page using
  D2RCore's existing quick-transfer operation and item policy.
- Validate active grid, player and focused item identity; refused moves cannot
  fall through to the Personal Stash. No additional hook or Ctrl spoof.
- User confirmed Charm Inventory transfers work.

## [1.3.1+rev.8] - 2026-09-26

- Route controller menu remapping before D2RCore's custom-page route consumes
  shoulder inputs, fixing coexistence with Charm Inventory.
- Guard the exact dispatcher, forwarding links and vtable slot.
- User confirmed menu switching works with Charm Inventory installed.

## Repository migration

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
