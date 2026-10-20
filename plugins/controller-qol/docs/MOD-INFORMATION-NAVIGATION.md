# Mod Information controller navigation

Date: 2026-10-11. Candidate: `1.3.1+rev.76-beta.2`. Live validation pending.

## Behavior and scope

The user reports LT/RT changing the game panels behind an open Mod Information
window and requested L1/R1 for the information tabs. The custom shell is
`modinfo_aa`, with direct child `OptionsTabs`. Its seven content pages are
separate panels. The prior softmod work is recorded in
`controller-mod-information-softmod.md`, preserved with the external workspace's
unpublished mod-data research records.
No softmod files are changed by this plugin update.

Controller QOL already maps LT/RT actions 7/8 to the main UISwitcher's cycle
actions 19/20, and maps L1/R1 actions 19/20 to supported inner tabs' actions
7/8. The custom panel was missing from that scoped inner-tab policy, and the
underlying UISwitcher was still allowed to act.

This candidate adds:

- Exact `modinfo_aa` lifecycle tracking in its own bit128. Child-page opens and
  closes do not clear it. ClosePanel/UnloadPanel for the shell, HUD teardown,
  plugin cleanup, and initialization clear its state.
- Exact visible `OptionsTabs`/direct-parent `modinfo_aa` admission, requiring
  tab switching enabled and the existing native left/right bindings 7/8.
  L1/R1 is translated through a borrowed synchronous message copy. LT/RT is
  skipped only by this tab widget so it cannot change information tabs.
- While that shell is tracked and the existing navigation hooks are admitted,
  actions 7/8/19/20 do not reach the underlying UISwitcher. Other actions and
  messages, including A, B, and ControllerActionEnd, pass through unchanged.
- Matching bumper prompts only for the exact ancestry
  `TabLeftIndicator` or `TabRightIndicator` -> `OptionsTabs` -> `modinfo_aa`.
  No stored asset, binding, or widget text is modified.

The guard does not globally consume trigger input. ScrollView and the active
page's handlers remain native. Normal LT/RT main-panel navigation resumes
after closing the shell. Tracking also excludes ground-loot shortcuts while
Mod Information owns the foreground, so L1 is available for its tabs.

The softmod independently provides the pause-menu entry, actual Close button,
native ButtonWidget Back handling, scrollbar focus/navigation, and tab-indicator
positions. None of those require this plugin addition. Disabling the plugin
removes the bumper remapping, matching prompt substitution, and underlying-panel
navigation guard; it does not remove the JSON controls. A plugin-disabled live
test has not yet been performed. The requested wider prompt spacing and
scrollbar-first focus follow-up are JSON-only changes, not new QOL hooks.

`qol menu off`, `qol input off`, disabled plugin state, dedicated-panel
contexts, or missing existing hook admission preserve the prior native/remap
fallback. No new configuration key or migration is needed. A `qol status`
line reports Mod Information state and blocked underlying navigation count;
`qol trace` adds scoped routing diagnostics.

## Existing native contracts and limits

Target environment: D2R `3.3.93787`, D2RLoader `1.3.1-beta`. No new RVA,
structure offset, byte guard, provider hash, ABI, or hook is introduced. The
existing guarded contracts remain authoritative:

| Existing path | Contract reused |
|---|---|
| `qol_navigation.cpp`, `native_signatures.h` | TabBar handler D2R RVA `0x878D30`; tab-switch flag `+0x16A0`, left/right actions `+0x16A4`/`+0x16A8` |
| `menu_signatures.h`, `menu_route_profile.h` | Original UISwitcher RVA `0x27DF80`, or the already-qualified loader dispatcher route; complete existing route/byte checks retained |
| Existing synchronous message decoder | Message size `0x120`, payload size `0x20`, payload pointer `+0x110`, action `+0x10`; only InputMessage/ControllerActionBegin is translated/blocked |
| Existing scoped widget/glyph reader | Name pointer `+0x08`, direct parent `+0x30`, visible/enabled bytes `+0x50`/`+0x51`, names bounded to fewer than 64 bytes |
| Public SharedEventService | PanelManager OpenPanel/OpenExclusivePanel, ClosePanel/UnloadPanel lifecycle notifications |

See [menu restoration](MENU-LOADER-1.3.1.md) and the
[native contract index](../../../docs/reverse-engineering/controller-qol-native-contracts.md)
for prior admission evidence. Signature failures remain fail-open. Panel
state follows lifecycle requests, not a new live visibility accessor; failed
opens or missing close notifications remain a limitation of the existing
scheme. Exact softmod shell names and cleanup/HUD resets bound this candidate.

## Validation and deployment

Focused regression coverage includes exact/prefix-name rejection, all seven
content-page transitions, shell close and HUD reset, both native/loader route
models, action IDs 0 through 255, disabled passthrough, exact glyph ancestry,
and existing Settings/Chronicle/stash/filter prompt behavior. Automated route
models do not prove real game dispatch ordering or visual controller behavior.

The active installed rev.76-beta.1 DLL was matched to its prior build/source
checkout before building current workspace changes. A source comparison found
only this navigation/version work and the separately staged ordinary-chest
default; the current Reimagined config already explicitly enables that option.
Existing configuration and skill-profile files must be retained byte-for-byte.
The candidate includes the previous stash-feedback work unchanged.

Release build passed all 32 Controller QOL CTest suites, including DLL exports,
ABI manifest, embedded defaults, and the new routing/glyph regression cases.
Independent source review and whitespace checks passed. The initial restricted
Windows compiler check failed with MSVC C1902; the same build/test command
completed outside that sandbox. No code workaround or guard change was needed.

The tested DLL reports `1.3.1+rev.76-beta.2`, is 609,280 bytes, and has SHA-256
`BF57CA86EC23E943C86AB362061263A300AFBF0EF0F2DBAF4D323BBC72209985`.
This verifies an artifact, not loader admission or live controller behavior.

The candidate was initially staged at the user's request while another game
test was running. After the user closed D2R and authorized local installation,
the active Reimagined DLL was replaced with the tested rev.76-beta.2 artifact.
Its installed SHA-256 matches the build hash above. The prior rev.76-beta.1 DLL
(`25C03D583A3E511DBE708797E13F447F9A837BFC0045DC0E51A044549E1A801C`)
has a verified rollback backup and guarded restore script. All four existing
configuration/profile/catalog/backup files were preserved byte-for-byte.

D2R/D2RLoader closure was checked outside the restricted process view before
and during deployment. The separate older Ladder plugin install was not
updated. All ten required Reimagined JSON files already matched the reviewed
source/hash manifests: eight shared shell/content layouts and two controller
pause layouts. All parse; the seven fixed footers contain the native Back
field and direct close action, with no temporary top buttons. Shell tab routes
are unchanged and the retired controller shell override is absent. No JSON
recopy was necessary. Private logs, build output,
deployment receipts, and backups must not be committed. No game was launched
by the agent and nothing was published; live behavior remains unverified.

The user subsequently confirmed that Mod Information opens, the bottom Close
control is available, and L1/R1 rotates through the pages. Screenshots exposed
two remaining layout/input issues: both bumper indicators occupy the left edge,
and controller scrolling cannot reach content below the initial viewport.
This confirms page switching, not every input/lifecycle check below. The
indicator placement and scrollbar focus follow-up is documented in that separate
softmod investigation. Its original evidence remains preserved outside this plugin checkout.

Remaining live checks after the layout follow-up:

1. Open Mod Information and use L1/R1 through all seven tabs; verify prompts
   show the matching bumpers and each press changes only the information tab.
2. Press and hold LT/RT; verify the underlying Quest/Character/Inventory/etc.
   selection stays unchanged. Verify content scrolling retains native behavior.
3. Test A/B closing, reopen, and switch keyboard/controller input modes.
4. Close Mod Information and verify normal LT/RT main-panel navigation resumes.
5. Leave/rejoin a game and verify no stale panel lock; check stock Settings,
   Quest, Skills, Chronicle, and stash navigation for regressions.
