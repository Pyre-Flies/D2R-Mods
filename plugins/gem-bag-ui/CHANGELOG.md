# Changelog

## 1.3.1+rev.2 - 2026-10-22

- Swap controller Gem Bag shortcuts so `LB+A` opens the gem picker and `LB+B` deposits compatible gems and clusters from the bag's current storage page.
- Show `LB+A: Choose gem` and `LB+B: Deposit gems + clusters` in the controller tooltip footer.

## 1.3.1+rev.1 - 2026-10-21

- Withdraw gems through a mouse, keyboard or controller picker.
- Deposit compatible gems and clusters with Shift + right-click, including normal shared-stash pages.
- Matching client and host versions are required for hosted games.

Promotes beta.19 behavior. Hosted loose-gem deposit and save/rejoin are live-tested; shared clusters and full-inventory refusal remain automated-only.

## 0.1.0-beta.19 - 2026-10-21

Shared-stash deposits now use a supported inventory transfer before consuming
each gem or cluster. Leave room in normal inventory for one stack. A clean
failure returns the gem to its original stash cell and restores the credits.
Automated checks pass. Hosted Skull deposit and Ruby withdrawal/redeposit
passed with matching client state and host logs. The user also confirmed
the shared bag persisted after hosted save/rejoin.

## 0.1.0-beta.18 - 2026-10-21

Clarify deposit refusal messages and record the failed transaction stage and
SDK result for the shared-stash investigation. Mutation guards are unchanged;
shared-stash deposits are not yet fixed. Drag-to-bag is no longer planned.

## 0.1.0-beta.17 - 2026-10-21

- Shift + right-click a Gem Bag to deposit compatible gems and gem clusters
  from its current storage page, without adding a picker button.
- Discover deposit values from active compiled recipes. Roll cluster credits
  on the server and leave stacks in place if their maximum value cannot fit.
- Keep deposits server-owned with bag leases, replay protection and guarded
  recovery. A failure can stop a batch after earlier stacks have completed.

Automated checks and recorded-table replay are complete. One offline Onyx
deposit is verified on client/server (1491 to1492, only that item removed, bag
unmoved). A separate cluster deposit is verified (1492 to1513, only the cluster
removed). Hosted and persistence validation are pending. Drag support
remains next.
See [deposit contract](DEPOSITS.md) for bounds, empty-bag limitations and evidence.

## 0.1.0-beta.16 - 2026-10-21

- Read the correct compiled item field for localized gem names. Keep name
  failures optional and report the exact lookup that failed.
- Keep client name references out of the host compatibility fingerprint, so
  presentation differences do not block matching withdrawal definitions.
- Preserve server-owned space checks, credit deductions, identified creation,
  replay protection and exact local definition checks.

Owned tests cover a real captured gem row, different client/host name IDs,
UTF-8 client labels, headless discovery without localization and continued
refusal of changed gem effects. The user confirms offline labels; the loaded
global beta.16 DLL and successful name-application log are verified. A logged
Diamond withdrawal agrees with the current client/local-server state, without
a controlled before snapshot. Separate-host regression and persistence remain
pending.

## 0.1.0-beta.15 - 2026-10-21

- Discover compatible Gem Bags from the game's loaded recipes and item data,
  across global installs and active mod profiles. No Reimagined name, loose
  TXT files or package-wide table hashes are required.
- Resolve bag classes, credit stats, properties, gem codes and localized names
  from compiled definitions. Show only supported gems, with stock-art fallback.
- Keep host-owned space/credit checks, identified creation, replay protection
  and compensation. Changed or mismatched definitions refuse withdrawals.
- Preserve native engine/provider and ownership guards. Installs without
  compatible bags remain inactive; no items or recipes are added.

Eleven automated suites and a replay of the current non-Ladder compiled tables
pass. The user subsequently confirmed normal-package UI and withdrawals;
logs show successful requests, but labels were missing. Controlled state,
persistence and separate-host validation were not repeated on beta.15.

## 0.1.0-beta.14 - 2026-10-21

- Explain startup compatibility refusals: name the executable, provider, mod,
  data table or native check that failed.
- Include expected/actual SHA-256 hashes for changed files and the failing
  stage/Windows error for unreadable files. Report all failures in each group.
- Keep every existing compatibility guard and withdrawal rule.
- Allow installation in the global plugin folder for packages without a
  mod-local loader. The active Reimagined mod and exact data are still required.

Nine automated suites pass, including real hashing, missing/exclusively locked
files, newline-only changes, native-byte diagnostics and retained SDK refusal.
Installed and hash-verified globally, with the prior DLL, configuration and
saves backed up. The real global loader accepted beta.14, then the normal
package failed all four table hashes. Relevant bag/gem definitions match the
tested files, but native checks and transactions remain unqualified. Beta.13 is the prior
hosted/offline baseline. The current local Reimagined 3.0.12 data differs from
the pinned profile in all four table hashes; no non-Ladder profile is admitted
by this diagnostic update.

## 0.1.0-beta.13 - 2026-10-20

- Run server requests in Core's existing per-game callback when ordinary SDK
  scheduling is unavailable on a dedicated host.
- Keep peer ownership, bag leases, space and credit checks on the server, and
  preserve the original Core callback. The supplied D2RDS DLL is unchanged.

Eight automated suites pass. The new relay suite executes publication,
forwarding and concurrent teardown in an owned executable fixture. RPC checks
cover wrong-game deferral, expiry, one-credit withdrawal and cached duplicates
with SDK/backend substitutes. Installed in both client and server plugin
folders with saves/configuration and both previous DLLs backed up. A matching
Reimagined dedicated host and client passed inspection and a controlled mouse
withdrawal: one identified Ruby, 87 -> 86 credits in both processes, bag unmoved
and all other sampled items unchanged. Six earlier mouse/controller withdrawals
were logged successfully; all six outputs' identification flags match both
inventories. The final offline regression also passes: one identified Ruby,
86 -> 85 credits, bag unmoved and all other sampled items unchanged. The hosted
86-credit bag and its storage cell contents/identification survived Save and
Exit followed by offline loading. Repeated hosted rejoin, other hosted storage
types and multi-game isolation remain unqualified.

## 0.1.0-beta.12 - 2026-10-20

- Keep the host request queue moving after a session change discards callbacks.

Seven automated suites pass, including the new discarded-callback regression.
Beta.11's offline host inspection matched the 95-credit client/server snapshot.
The exact loaded beta.12 passed an offline mouse withdrawal: one identified Ruby,
94 -> 93 credits on client/server, bag unmoved and other sampled items unchanged.
All 47 active mod plugin files and the selected October 15 D2RDS DLL are copied
and hash-verified in the root server plugins folder. Saves/configuration are
backed up and configuration preserved. Separate-host operation and persistence
remain unqualified. A subsequent dedicated-host test returns Unavailable before
inspection because host game-work scheduling refuses the request. No dedicated
withdrawal is accepted; investigation continues in the Gem Bag plugin.

## 0.1.0-beta.11 - 2026-10-20

- Send gem requests to the host, which checks the sending player's bag, free
  storage and credits before creating an identified gem and spending a credit.
- Use that same request path offline. Add a host-only startup path for D2RDS.
- Reject stale bags and duplicate requests; stop after an uncertain result.

Seven automated suites pass, including real protocol/RPC code with separate
client/host SDK contexts, offline loopback, bounded replay/session handling,
authority/refusal cases and headless loading without UI services. The client
candidate was installed with beta.10, saves and configuration backed up. Its
offline host inspection matched the client/server baseline. Withdrawal testing
continues on beta.12; separate-host operation and persistence have not yet been
tested live. Root-server copying follows the final offline check.

## 0.1.0-beta.10 - 2026-10-20

- Create withdrawn gems identified. Reject an output with the wrong quality or
  missing identification state, remove it and restore its credit.
- Keep the working compact picker, localized names and controller placement.

All five suites pass. The real creation/verification path is exercised for all
eight gems in four stores with owned SDK/setter/publisher substitutes, including
removal/refund of an unidentified result. The new test failed before this fix.
The installed build's fresh Ruby is visibly identified and read-only snapshots
confirm that state on client/server, one output, 97 -> 96 credits and the bag
unmoved. Other sampled items, including old test gems' flags, are unchanged.
This does not automatically repair earlier test outputs or qualify save/rejoin.

## 0.1.0-beta.9 - 2026-10-20

- Restore the icon picker by publishing its layouts during startup. Missing
  gem names can no longer prevent it from opening.
- Fill the localized hover names after opening, removing item color markers
  while keeping translated text. Keep controller controls and placement.

All five automated suites pass, including startup resource timing, optional
labels and guarded reserved-buffer writes. Installed with saves/configuration
and the previous DLL backed up. The exact loaded build opened and applied all
eight labels; the user confirmed the picker and plain names look correct.
Three controller inventory withdrawals logged 100 -> 97 credits. The user then
reported unidentified output gems; the omitted SDK identification state is
fixed in beta.10. Earlier audits did not cover that flag.

## 0.1.0-beta.8 - 2026-10-20

- Restore the picker when gem names are unavailable during startup. Prepare the
  localized labels when the picker is first opened, then reuse its layout.
- Keep the color-marker removal, controller controls and accepted position.

All five automated suites pass. A new callback lifecycle regression reproduces
beta.7's startup failure and verifies retry and layout setup before opening.
The live check resolved all eight names and registered resources successfully,
but native Open returned NotFound (5). The user saw no picker; the bag remained
at 100 credits on client and server. The fake provider's late-resource behavior
did not reproduce this native availability failure; beta.9 corrects that model.

## 0.1.0-beta.7 - 2026-10-20

- Remove the visible `*U` color marker from localized gem labels while retaining
  the translated names. Keep the accepted controller position and controls.

All four automated suites pass, including copied localization, color-token
removal, Unicode preservation and JSON escaping. The live startup check failed:
the names were unavailable before controls were registered, so no picker opened.
The new beta.8 lifecycle regression catches this gap. Beta.6 controller controls
were user-confirmed; its log records three
inventory withdrawals and 103 -> 100 credits. Short status messages remain
English; restart after changing game language to rebuild the picker labels.

## 0.1.0-beta.6 - 2026-10-20

- Use the game's localized item names for all eight gem hover labels, including
  Reimagined name overrides. Keep the same compact icon picker and controls.

All four automated suites pass. Its subsequent runtime check resolved all eight
keys; the user screenshot showed their color instructions as a `*U` prefix.
Controller controls were user-confirmed and the centered position accepted.
Short plugin status messages remain English. Beta.5's mouse withdrawals were
user-confirmed in inventory, Cube, personal stash and normal shared page 1;
logs and final client/server reads agree at 103 credits.

## 0.1.0-beta.5 - 2026-10-20

- Fix the picker closing immediately after a right-click. Its full-size parent
  panel stores a zero rectangle but inherits the real screen dimensions.
- Keep native parent scaling when positioning and selecting icons. Anchor the
  popup to the actual click point, with the fresh-hover check unchanged.

All four automated suites pass. The geometry regression now reproduces the
live fill-parent tree at ⅓ scale. Beta.4's exact loaded DLL and 109-credit bag
were verified read-only; its UI was refused before placement. Beta.5 visible
appearance and two mouse withdrawals were subsequently confirmed: Amethyst
and Skull, with matching client/server 107 credits and Cube outputs. The user
then tested Ruby in Cube, Diamond in inventory, Amethyst in personal stash and
Skull in normal shared page 1. Logs and a same-process snapshot audit agree
107 -> 103. See the native contract for the shared-owner capture limit.

## 0.1.0-beta.4 - 2026-10-20

- Use a small grid of the game's eight gem icons, with a subtle selection
  highlight and names on hover. Keep the surrounding game UI visible.
- Open beside the mouse cursor and clamp inside the current window. Use the
  native widget geometry for mouse selection instead of a fixed screen scale.
- Correct native text colors and use native widgets for short failure labels.

Automated build, layout and policy checks pass. Live UI and mouse validation is
pending. Beta.3's user confirmed another keyboard withdrawal, but reported
blank labels and failed mouse selection. The eight beta.2 gems and 110 credits
were present after restart; beta.3's Amethyst log then reported 109 credits.

## 0.1.0-beta.3 - 2026-10-20

- Replace the large dimmed dialog with a compact picker using muted colors,
  a thin border and simple gem rows. Keep the surrounding game UI visible.
- Route mouse choices through the same geometry as the displayed rows. Add
  outside-click dismissal while retaining Close, Escape and controller B.
- Keep presentation and request generations separate so closing/navigating does
  not depend on the current withdrawal request.

Beta.2's eight keyboard withdrawals in the Cube were confirmed by the user and
matching client/server snapshots: 118 to 110 credits, bag unmoved, all eight
expected gem codes and positions. Compact mouse UI and other surfaces are
still awaiting live checks.

## 0.1.0-beta.2 - 2026-10-20

- Open directly from a fresh hovered-bag right-click instead of depending on a
  normal item activation being emitted for the bag. Own its matching release
  and refuse stale or moved mouse targets.
- Resolve the selected bag again in authoritative storage before accessing its
  native copy. Add concise input, panel and inspection diagnostics.

Beta.1 loaded in the offline game, but the first user right-click showed no
selector. No withdrawal was attempted. Beta.2's eight keyboard withdrawals were
subsequently verified as recorded above; its mouse path failed.

## 0.1.0-beta.1 - 2026-10-20

- Add an eight-gem selector opened by right-clicking a Gem Bag or pressing LB+B
  while focused on it.
- Withdraw one gem into free space in the bag's current inventory, Cube,
  personal stash or normal shared-stash page, spending one bag credit.
- Add credit and destination labels, keyboard/controller navigation, space and
  stale-item checks, and refusal after an uncertain native result.

Development candidate. Automated checks pass; live behavior and persistence
remain unverified.
