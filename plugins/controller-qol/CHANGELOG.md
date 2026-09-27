# Changelog

All notable changes to Controller QOL Updates are recorded here. Versions before
the GitHub migration are reconstructed from release and validation records.

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
