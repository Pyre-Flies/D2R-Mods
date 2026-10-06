# Controller QOL changelog

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
