# Shared Stash SDK migration - rev.18 candidate

## Scope

Normal Shared Stash LB+X deposit/withdrawal uses the public ItemService
`executeExistingItemTransaction` when `SharedStashWrite` is advertised.
Pin the upstream SDK submodule to released v0.3.0, peeled commit
`bb0b48e080c51c37fc8296ee415d9daaba7ee33d` (ABI remains 4), replacing v0.2.0
`6fbabfc84a72b83ed812ce7f45f79a519fdd0877`.

SDK reference: `third_party/D2RLoader-PluginSDK/include/D2RLPlugin/item.h`
and README sections on shared-stash writes and existing-item transactions.
The service supports normal active pages, not remove-only seasonal storage.
The normal page index is zero-based. Full current destination/operation sizes
are required: ItemDestination 40 bytes, ExistingItemOperation 64 bytes.

## Behavior

The UI request captures the player and normal Shared page. The game-thread
executor rechecks the player, selected page and a three-second request age.
Item resolution keeps the existing runtime ID/code/class/page safeguards;
unknown or mismatched Shared source pages are not converted to page zero.
The one-operation Move uses the existing item handle, Automatic placement and
an explicit destination sharedStashPage for deposits. Withdrawal uses the
captured source page to validate the item and destinations Inventory.
Success requires SDK Success AND committedOperationCount == 1.

SDK rejection, including a full destination, ends the request. There is no
native retry after an SDK attempt. Missing runtime capability retains the
previous native route; previous-season storage also retains that route.
Missing/changed page admission cancels instead of selecting Personal Stash.
The existing smart advanced-storage routing remains first: Materials/Gems,
Cube, belts, vendor actions and custom pages are outside this migration.

## Remaining native selection dependency

The SDK moves the item, but does not expose the active stock Shared UI page.
`shared_sdk_selection.h` therefore uses read-only, guarded selection helpers.
No new hook or patch is installed.

| Game RVA / offset | Contract / evidence |
|---|---|
| 0x846170 | Find top-level BankExpansionLayout; existing complete 22-byte witness |
| widget +0x50/+0x51 | Active/visible; both must be 1 |
| 0x23AF50 | Selected category; require Shared (1), existing 130-byte witness |
| 0x23B910 | bool(bank): previous-season toggle state; full 0x68 bytes checked |
| 0x23AEF9 | Native normal-season resolver reads qword [bank+0x168]; exact seven-byte witness |
| Bank +0x168 | Normal zero-based selected page; UINT32_MAX/overflow rejected |
| Bank +0x170 | Previous-season index in historical owner resolver; not used by new SDK route |

The existing owner resolver 0x23AD80 calls 0x23B910 at 0x23ADA8; true uses
+0x170, false reaches the +0x168 read. The new route does not need to call
that owner resolver or reconstruct its storage owner. The previous-season
helper resolves its toggle via 0x855C90, checks its native type and reads
+0xB88. These internal calls remain native; only the helper entry is called.

2026-09-27 read-only live verification: the user left Shared page 4 open.
Bank +0x168 was 3, +0x170 was 0. All four selection witnesses above matched
the live game. Current Core SHA256 remains
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.

A read-only runtime service inspection found Core+0x633460 holding the
88-byte ItemService v1, capabilities 0x3, with two references cached in
loaded QOL data. Its executeExistingItemTransaction pointer is Core+0x422500.
This corroborates availability of SharedStashWrite (bit 0). These addresses
are research evidence ONLY: production resolves the service through QueryService
and its public capability check, never through a Core RVA.

## Validation

- QOL builds and all 19 suites pass, including DLL ABI/resource/default checks.
- Shared SDK tests cover old/truncated services, absent capability, pages 0/3/104,
  wrong/unknown source pages, wrong containers, rejected transactions, zero
  committed operations, and changed page/season selection.
- Item Roll Ranges: rebuild plus all 5 suites pass with the new pinned SDK.
- Map Assistance: rebuild plus both suites pass with the new pinned SDK.
- No native item mutation was performed during live inspection.
- Candidate DLL SHA256:
  `4C213480EB24DA24C1C1DF36327D398025D55FFEE2ABCE9355F8C2D3DA177AF4`.

Pending candidate live tests: ordinary item deposit and withdrawal on page 1
and a later page; full destination; change pages between actions; Personal,
Materials/Gems and Cube regressions. Confirm the startup capability log and
[QOL/SharedSDK] commit logs. Visible behavior and save/reload persistence are
not proven by automated tests or service capability alone.

After a patch, prefer public SDK capability checks; only requalify the remaining
page-selection witnesses if they fail. Do not replace page numbers with native
storage-page/category values: those are distinct namespaces.

## Installation

Installed rev.18 in the global plugins folder on 2026-09-27 after the user
closed the game and loader; process absence was checked before replacement.
Rev.17 was backed up. Built and installed SHA256 match the candidate hash
recorded above. In-game transfer and persistence verification remains pending.

