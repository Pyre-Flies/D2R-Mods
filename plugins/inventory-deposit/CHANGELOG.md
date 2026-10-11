# Inventory Deposit changelog

## 0.1.0-beta.13 - 2026-10-11

- Shorten the button tooltip to exactly "Deposit All Gems, Materials and Runes".
  Eligibility, transfers, disabled/hover states, artwork, placement, configuration
  and tab feedback are unchanged.
- The user confirmed beta.12 greys out the button with no eligible items or
  while holding an item, and brightens it on hover when eligible items are
  available. This is live confirmation of those visual states, separate from
  beta.12's automated and artifact checks.
- All twelve existing automated suites and actual DLL/resource/package checks
  passed. Verified the exact tooltip in generated layout output. Installed
  beta.13 with matching built/deployed hashes after backing up beta.12 and the
  byte-identical live config. The user subsequently confirmed the revised
  wording looks good in game.
- Package the first public prerelease with its own CI/tag workflow, reference
  configuration, installation guide and checksums. Preserve the existing
  stable suite release and publish the original editable artwork with source.

## 0.1.0-beta.12 - 2026-10-11

- Fix unavailable buttons retaining gold artwork. The SDK enabled setter only
  changes the native input flag; ButtonWidget has a separate visual-disabled
  state. Apply D2R's guarded native enable/disable methods to this plugin's own
  button, selecting the existing grey frame and restoring normal interaction
  artwork when items are eligible again.
- Synchronize and read back the visual state after SDK input enablement, before
  showing the button or allowing queued clicks. Refuse unsupported native
  profiles, unexpected widget types/frames or failed read-back without moving
  items. Record new native evidence in `docs/BUTTON-STATE.md`.
- Keep the beta.11 inventory eligibility check, cursor/busy blocking, button
  placement, source artwork and tab feedback unchanged. This corrects the
  earlier assumption that SDK enablement alone selected the disabled sprite.
  Live appearance still requires confirmation.
- All twelve automated suites passed, including isolated execution of the
  guarded native enable/disable bodies and independent input/appearance UI
  regressions. Actual DLL/resource/package checks passed. Installed beta.12
  after backing up beta.11 and the live config; built/deployed hashes match
  and configuration is byte-identical. No live rendering result is claimed.

## 0.1.0-beta.11 - 2026-10-11

- Grey out the visible Deposit All button when ordinary inventory has no
  eligible items. It re-enables after eligible items are added, with no
  automatic request. Held-item and busy-batch blocking remain intentional.
- Use a bounded read-only scan and the existing native advanced-storage
  eligibility rule. Preserve Inventory/page-zero scope, identity checks and
  the 256-item batch limit. Read failures disable the button for that check.
- Keep eligibility separate from ongoing batch availability so removing the
  final eligible item does not cancel source-removal verification or feedback.
  Execution still rechecks items and handles native refusal normally.
- The user confirmed beta.10's held-item fix works. No new native address,
  hook, artwork, alignment, configuration or feedback change. The empty-state
  appearance needs live verification.
- All eleven automated suites and actual DLL/package checks passed, including
  SDK scan scope, bounds, errors, identity/player changes and production button
  wiring. Installed beta.11 after backing up the previous DLL and live config;
  built/deployed hashes match and configuration is byte-identical.

## 0.1.0-beta.10 - 2026-10-11

- Fix the button disappearing while an inventory item is held on the cursor.
  Keep it visible in the supported stash context and use its existing grey
  disabled frame until the cursor is free. Releasing the item only re-enables
  the button; it does not automatically request a deposit.
- Separate visual availability from strict action preflight. The click path,
  active batch queue and each native transfer retain both cursor checks.
  Busy batches still disable the button without cancelling themselves.
- Keep unsupported/closed stash contexts hidden, and preserve native profile,
  selected-tab, vendor, previous-season and player guards. No new native
  address, hook, asset or config setting is introduced.
- The user reported beta.9's feedback update works well. Button fit, artwork,
  configuration and once-per-batch tab feedback are unchanged. Live cursor-state
  appearance still needs verification.
- All ten automated suites and actual DLL/package checks passed. Added a test
  of production plugin callbacks for held items, cursor release, busy batches,
  closed contexts and reload cleanup. Installed the verified beta.10 DLL after
  backing up beta.9 and the byte-identical live config.

## 0.1.0-beta.9 - 2026-10-11

- Start supplemental stash-tab feedback once per affected destination per
  Deposit All batch. Six different materials now share one Materials animation;
  additional confirmed items do not restart or extend its timer, including
  after that timer expires. Gems and Runes are tracked independently.
- Preserve the existing animation duration and fade cycles, as requested.
  Each later Deposit All batch can trigger those destinations again.
- Carry batch identity with queued confirmations so rapid successive batches
  retain separate notification state. Session/queue cancellation clears it.
- Keep the game's own current-tab notification handling and all deposit
  semantics unchanged. No new native addresses or hooks are introduced.
- The user confirmed beta.8's button fit works. Its geometry, artwork and live
  config remain unchanged; the new feedback behavior needs live verification.
- All nine automated suites and actual DLL/package checks passed, including
  six distinct materials across updates/expiry and a later batch restarting
  the destination. Installed after backing up beta.8; live config is unchanged.

## 0.1.0-beta.8 - 2026-10-11

- Correct remaining top-lip overlap after beta.7: set the native button and HD
  sprite to 144-by-136, low-end72-by-68, and default `gold_offset_y=1`.
  This lowers only the top edge by 4 UI units (about 2.7 screenshot pixels),
  preserving the bottom edge, width and gold gap 85.
- Account for the translucent artwork fringe above the opaque stone frame.
  The beta.7 screenshot and runtime logs confirm the previous update loaded,
  but its centered height reduction did not provide enough top clearance.
- Preserve the gold-inlay artwork, native anchor, deposit behavior and feedback.
  Existing explicit settings are preserved; use offset 1 to apply the new fit.
  Final visual alignment still requires an in-game check.
- All nine automated suites and actual DLL/package checks passed. Installed
  the verified DLL and offset 1 after backing up beta.7 and its config.

## 0.1.0-beta.7 - 2026-10-11

- Trim button height from 144 to 140 UI units, preserving width144 and the
  existing gold anchor settings. This reveals approximately one screen pixel
  at each footer lip at the user's screenshot scale. Match the click rectangle
  and embedded sprites: HD144-by-140 and low-end72-by-70.
- Make the fine antique-gold inlay in the stone frame more visible across all
  four states, retaining the dark stone surround and deposit symbol design.
- Keep live configuration, deposit behavior and tab feedback unchanged.
  Record the original edited assets and prompts in `assets/art/PROMPTS.md`.
  Final fit and interaction-state appearance still require live-game validation.
- Release build, all nine automated suites, rectangular sprite round-trip and
  actual DLL/package checks passed. Installed the verified DLL after backing
  up beta.6; the live configuration is byte-identical.

## 0.1.0-beta.6 - 2026-10-11

- Enlarge the button from 132 to 144 UI units, with matching HD144/low-end72
  sprites and click rectangle. Preserve the approved square stone artwork.
- Set default gold spacing to `gold_gap=85`, `gold_offset_y=-1`. This moves
  the button 25 UI units left and extends its visible frame toward both footer
  lips, accounting for the artwork's transparent margin. The user's annotated
  screenshot is the fitting reference; final alignment needs an in-game check.
- Existing explicit configuration values remain preserved. Applying this fit
  to an existing install requires updating both spacing settings after backup.
- Gold anchoring, embedded assets, deposit behavior and tab feedback are
  unchanged. No new native addresses, layouts or MPQ resources are required.
- All nine automated suites and DLL/package checks pass. Installed the tested
  DLL and requested spacing after backing up beta.5; live beta.6 fit is pending.

## 0.1.0-beta.5 - 2026-10-11

- Replace the button's metallic rim with dark, worn, stepped stone molding to
  match the gold display's surround. Preserve the gold arrow/tray symbol and
  regenerate normal, hover, pressed and disabled artwork with the same frame.
- Enlarge the native button and HD sprite from 120 to 132 units (10%); update
  the low-end sprite from 60 to 66. The click rectangle matches the new size.
- Reduce the default gold gap from 122 to 110 units. Together with the larger
  button this extends the left edge over the background seam while keeping the
  right edge fixed; gold centering raises the top by 6 units. Manual x/y settings
  retain their existing meaning. Explicit config values remain preserved unless
  edited; this user's requested fit also updates their live `gold_gap` to 110.
- Keep the sprites embedded in the DLL. No native addresses, transfer behavior,
  tab feedback, panel replacement or additional runtime asset is introduced.
- The user approved the revised stone border. Final in-game fit still needs
  verification; build/deployment evidence is recorded in `docs/BUTTON-SPRITES.md`.
- All nine automated suites and actual DLL/package checks pass. The local DLL
  and requested gap110 configuration were installed after backing up beta.4.

## 0.1.0-beta.4 - 2026-10-11

- Give affected advanced-stash tabs independent deposit highlights. The native
  panel keeps one flashing destination, so a mixed instant batch previously
  replaced earlier tab notifications with its last destination.
- Observe each submitted item's removal from inventory once, after a successful
  inventory scan, and refresh the corresponding tab's feedback on the UI thread.
  Preserve instant batch submission, item routing, eligibility and no-retry rules.
- Use guarded native tab visuals without replacing the stash panel or adding
  an MPQ. An unsupported feedback profile leaves deposits available.
- Add optional bulk-queue observers; Controller QOL does not opt into them and
  retains its existing deposit behavior.
- User confirmed beta.3's custom button works and looks good. Its runtime log
  records completed batches with 4, 3 and 4 submitted source items removed.
  This does not independently verify destination counts or save persistence.
- New multi-tab feedback requires live-game validation; automated results and
  exact native contracts are recorded in `docs/STASH-FEEDBACK.md`.
- All nine automated suites and an isolated execution probe of the two native
  tab setters pass. The installed DLL matches the tested artifact; the previous
  beta.3 DLL is backed up and the configuration is unchanged.

## 0.1.0-beta.3 - 2026-10-11

- Replace the borrowed Transmute artwork with original dark-stone/antique-gold
  Deposit All art: a downward arrow entering a tray, with normal, hover, pressed
  and disabled states. Keep the button's 120-by-120 logical size and gold anchor.
- Embed standard and low-end native sprites in the DLL and register both under
  this plugin's resource namespace before the layout. No separate MPQ, loose
  asset files, stock-sprite replacement or Charm Inventory change is needed.
- Preserve registration rollback and reverse-order resource cleanup if artwork
  or layout setup fails. Build directly from original PNG sources using Python
  and Pillow; no game artwork is copied into the sprites.
- Seven automated suites pass, including sprite encoding, exact embedded DLL
  resource checks, binary registration/rollback, and existing UI/action policy.
  Decoded native-size states were visually inspected. New-art rendering,
  hover/press behavior and low-end sizing still require an in-game check.
- Beta.2 runtime logs now confirm gold-anchor placement/read-back and a completed
  batch with 6 submitted source items removed and 12 ineligible items skipped.
  This does not establish the new art's rendering or save persistence.

## 0.1.0-beta.2 - 2026-10-11

- Anchor the button to the live gold amount rectangle: right edge plus spacing,
  vertically centered with an optional offset. Refresh before showing/clicking
  so ordinary inventory layout changes are followed automatically.
- Add `anchor_to_gold`, `gold_gap` and `gold_offset_y`. Automatic mode defaults
  on, including for older configs. Preserve `button_x`/`button_y` for manual
  mode (`anchor_to_gold = false`); their meaning is unchanged.
- Qualify native X/Y setters and the actual button's hit-test method with exact
  build/byte guards. Unsupported geometry hides the child and refuses clicks.
  No inventory panel replacement, new MPQ, executable patch or hook is added.
- Pass the isolated native setter/getter probe and six automated suites,
  including shifted/resized gold layouts, invalid geometry, missing widgets,
  placement failure/recovery and queued-click revalidation. Live automatic
  placement, inherited hit-testing and resolution/UI-scale checks are pending.

- Align the footer button with the gold row and center it in the space to the
  right: default position changes from `(800,1644)` to `(860,1620)`.
  These remain the manual-mode defaults. Beta.1 can apply this fixed alignment
  through its existing coordinates; gold anchoring requires the beta.2 DLL.
- User-confirmed live button operation on 2026-10-11. Runtime logs record two
  completed batches with 4/4 and 5/5 source removals and 12 ineligible items
  skipped per batch. Revised placement and save persistence remain unverified.

## 0.1.0-beta.1 - 2026-10-11

- Add a standalone mouse Deposit All button in the HD inventory footer, visible
  while the stash is open and the guarded deposit context is available.
- Reuse Controller QOL's batch implementation at build time. Controller QOL is
  not required at runtime; its files, settings and shortcuts are unchanged.
- Use a namespaced inventory child layout alongside the Charm Inventory arrow.
  Embed the layout and reference existing game artwork; no MPQ is required.
- Configure enablement and parent-local button position in `inventory-deposit.toml`.
- Automated checks and live validation are recorded in the feature research
  document. This initial candidate requires in-game validation before release.
