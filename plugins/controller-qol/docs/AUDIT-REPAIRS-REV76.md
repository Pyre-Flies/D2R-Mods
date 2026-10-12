# Controller QOL audit repairs - rev.76-beta.3

**Live follow-up:** beta.3 subsequently failed Cube self-insertion with LB+X
and an overlapping Shared-trigger gesture. T04 was user-cleared; T01/T03 are
partial. The assessments below describe the original eight repairs, not full
plugin qualification. The user subsequently confirmed beta.4 fixes Cube
self-insertion and Shared escape, with Cube behavior working through rejoins.
See [beta.4 evidence and reassessment](AUDIT-LIVE-FOLLOWUP-BETA4.md).

2026-10-11. Local candidate based on the existing rev.76-beta.2 working tree.
All pre-existing beta changes are retained. At the user's request, the tested
beta.3 DLL is now installed locally in Reimagined after verified beta.2 backup.
All 28 configuration files and 39 other plugin files were unchanged at
installation. A guarded rollback script and private deployment receipt are
retained under `outputs/`. No commit, push or publication was performed; the
game was not launched and live validation remains pending.

## Reassessment after repair

The original audit reproduced eight mechanisms with production functions and
mock SDK/native boundaries. The new regression executable exercises the
repaired production input, lifecycle, pickup filter, UI queues and move executor.
The mechanisms below are corrected in source and automated checks. **Live
controller behavior, hosted authority, server acceptance and persistence are
still unverified for this candidate.** These repairs do not establish a cause or
fix for the separate full-Ladder Save & Exit crash investigation.

Repairs were applied in this order: session/actor ownership (1/2), physical
button lifetime and state serialization (3/7), master disable and Cube context
(4/5), removal of packet-driven scans (8), then Shared Cube destination (6).

| Original issue | Change and reassessment | Remaining qualification |
| --- | --- | --- |
| 1. Ground state survives leaving/rejoining | Removed authoritative player/game caches and the server-wide pending slot. Joined/Left/LocalPlayerReady fence UI requests, visible IDs, slots, permits and placard leases. Old callbacks cannot act on a new game, even with reused IDs. Mechanism fixed. | Leave/rejoin while holding a chord; first pickup without first sending a movement/cast packet. |
| 2. Guest packets claim local pickup | Loot uses the local UI request path in offline, host and joined-client modes. No guest action observes/dispatches a host pickup. Native pickup filtering and the retained Guided Arrow correction are qualified by local lifecycle player ID; peers and unqualified actors pass through. Mechanism fixed; route expansion needs live proof. | Live distinct-peer/competitive-pickup testing is unavailable with one client; actor isolation has automated fixture coverage only. |
| 3. LB-first release leaks native presses/repeats | Consumed buttons/triggers remain suppressed until their own physical release. X/Y/R3 latches rearm only after button release. Removed independent polling X/Y action dispatcher. Synthetic holds cannot resurrect consumed buttons. Mechanism fixed. | Both release orders, modifier repress, UI changes mid-chord, ordinary next press and Shared page triggers. |
| 4. Master disabled still consumes input | Input processing receives the master flag and returns raw buttons/triggers without callbacks when disabled. Ground hooks also pass through. Mechanism fixed. | Restart with `[qol] enabled=false` and compare native controls. |
| 5. Closed-panel Cube action uses stale focus | Capture copied identity at physical Y press; check current source-specific UI/focus on UI dispatch and current UI/local SDK owner/session/age before execution. No remembered-player fallback for Cube. Mechanism fixed. | Closing panels or changing focus around dispatch; world LB+Y after inventory closes. |
| 6. Shared Cube lacks capture/targets Inventory | Capture normal Shared page/season/local SDK owner on UI thread. Submit one existing-item Move with Cube destination. Validate selection/owner/source again; rejected requests never retry another route/destination. Mechanism fixed on the capability-supported SDK route. | Normal page 0/later pages, full Cube and local save/rejoin. Joined-client coverage is optional only on an already matching external server; otherwise unavailable. Older authoritative and previous-season routes refuse direct Cube rather than move to Inventory; unsupported normal Shared hints are omitted. |
| 7. Concurrent fallback corrupts shortcut state | A recursive state mutex serializes per-pad history, gestures, callbacks and shared hold engine. The gate still drains active calls; recursion ownership is shared across callback types for the same gate. Mechanism fixed. | Existing shutdown fixture plus normal operation on native and naturally selected fallback providers. No forced guard bypass. |
| 8. Every packet scans ground graph | Removed the diagnostic enumeration and 18-entry loot observer installation from the plugin path. The existing local Guided Arrow correction alone retains one packet hook. Functional loot refresh uses a bounded client-visible ID set on UI thread. Mechanism fixed; no FPS claim. | Solo item-pile/movement/casting checks are feasible. Multiple-participant load remains unverified; no performance improvement is claimed. |

## Hosted ownership and transaction rules

The tester can run only one client at a time. The current live checklist is
therefore single-client offline, optional solo host, and optional joined-client
play only if an external server already uses the matching candidate. No second
player/client is requested. Peer isolation and competitive pickup remain
automated-fixture evidence only, with live multiplayer qualification open.
The constraint does not reduce the need for safe actor/authority handling.

Every live hosted test requires like-for-like plugin sets, versions and DLL
hashes on all participants. A guest without QOL or with a previous QOL version
cannot join this candidate host. The fixture
simulates native peer actions; it does not qualify a live guest or admission.
Any optional external-server test must use beta.3 on both sides; no such server
is assumed available.

Only lifecycle `LocalPlayerReady.playerId` for the currently joined
`sessionGeneration` qualifies a local actor. Player ID zero is valid;
`UINT32_MAX`, a different type/ID, missing lifecycle registration, missing ready
event or a changed native loot profile cannot qualify local interception.
Unknown/guest pickups call the native predecessor once with its original args.

Explicit loot is an immutable GUID/code/quality request with hold/session epoch
and 500 ms dispatch expiry. It resolves the **current** local player and target
on UI thread, rechecks visibility, distance, collision and native witnesses,
then calls the existing native client interaction. No SDK-handle cast, borrowed
authoritative pointer, custom network packet or direct server pickup is used.
Native/server code owns dispatch, item placement and acceptance.

Queued UI refresh ownership also uses an epoch token. Session cancellation
clears it even if a scheduled callback is abandoned; an old callback cannot
release a newer session's refresh token.

A local explicit request has one GUID-specific, generation-specific allowance
through the host's native pickup filter, consumed once and expiring after one
second. A guest cannot consume it. Outstanding requests are not automatically
resent. Modifier release does not revoke an already submitted request's
allowance; leaving/rejoining, losing/changing the player, or a contract fault
does. Submission is logged as pending, and the slot is retained until client
state observes removal. Delay/rejection is not reported as successful pickup.

Cube requests carry copied item identity, current SDK local owner, action
generation and a one-second age fence. Normal Shared sources additionally carry
UI page and season. `SharedStashWrite` plus the SDK existing-item transaction
contract supplies the direct Shared-to-Cube operation; the SDK README and
`include/D2RLPlugin/item.h` document atomic moves across these containers.
Unavailable authoritative scheduling may use the already guarded native client
transfer path. Busy/rejected/accepted work never triggers a second submission.
Old native Shared withdrawal remains Inventory-only; it refuses an explicit
Cube destination. Previous-season/remove-only direct Cube is not claimed.

## Build-specific evidence and reuse

No new game RVA, hook site or byte profile was introduced. These exact-build
contracts are reused, and their full witnesses remain in source headers:

| Game RVA / offset | Reused role / evidence |
| --- | --- |
| `0xFA180` | `void __fastcall(uint16_t unitType, uint32_t runtimeId)` native UI interaction; type 4 for item requests. Complete interaction body and caller guard in `client_loot_profile.h`; original capture/ABI in [REMOTE-GROUND-LOOT.md](REMOTE-GROUND-LOOT.md). Expanded to offline/host callers here, pending live validation. |
| `0x08B2D0`, `0x09A480`, `0x09A5D0` | Current local data context/player getter and client target lookup. Resolved inside the UI callback only. |
| `0x325140`, `0x350550`, `0x36EF50` | Distance, collision and item-code helpers. Existing client-loot witnesses, including collision mask `0x804`. |
| Unit offsets `+0x00`, `+0x08`, `+0x0C`, `+0x10` | Copied type, runtime ID, mode and immediate item-data pointer. Ground mode 3/type 4. Native server actor uses only type/ID during the pickup hook; it is never cached. |
| `0xCE500`, `0x3440170`, manager `+0xD0`, focus `+0x190` | Existing UI mode and controller widget discovery. `storage_focus.h` reuses native focus/item guards for the actual selected cell instead of accepting a tooltip as focus proof. |
| `0x846170`, `0x23AF50`, `0x23B910`, `0x23AEF9`; bank `+0x168` | Existing normal Shared page/season getters and 64-bit page index. Captured before cross-thread dispatch and compared at execution; never substitute page zero for unknown. |

The normalized input provider retains its existing guards, hook ownership and
native-first selection. State serialization introduces no new native address.
The native mutex precedes the shortcut-state mutex; processing does not acquire
the native mutex. Callback predicates used here expose lock-free admission/UI
state or schedule deferred work. The SRW gate remains a lifetime drain, with a
per-gate thread-local entry chain preventing recursive filtering through
different export callback types.

Guided Arrow is present in `[aim]` and the per-skill catalog. The existing
20-tile legacy correction still exists: `OwnsGuidedArrow()` suppresses it when
installed, enabled, in-session manual aim owns skill 22. This repair does not
remove that fallback or claim complete consolidation. It prevents the host's
legacy correction from using its controller state on a guest's packet. The
older 18-hook inventory/remote scheduling records are historical; this record
supersedes their loot dispatch architecture for beta.3.

## Automated tests

Local x64 Release build and **33/33 CTest suites passed**. The new production
regression executable uses a private allocated native image and mock SDK
boundaries. It does not open a D2R process, send a real packet, mutate a saved
character, replace an installed DLL or disable production compatibility guards.

| Test | Targeted cases |
| --- | --- |
| `qol-audit-regressions` / `tests/audit_regression_tests.cpp` | Actual LB-first X/Y/B/RB/R3 release; held-X deduplication; native next press; master disable; different-lambda reentry; 100 concurrent same-pad trials; different-pad serialization; local/guest native pickup; single-use permit; no resend; host/remote scheduler isolation; reused-ID rejoin cancellation; stale ready event; expired placard buffer disposal; native guard failure; guest Guided Arrow passthrough; closed/mid-close/focus-change/session-change Cube; selected Shared pages 0/3; Cube rejection; page/season/owner/unavailable-owner changes; older route refusal; one retained projection hook and no loot observer enumeration. |
| `qol-client-loot` | Existing stable slots/identity/radius/removal tests plus lifecycle identity including player ID zero, permit actor/GUID/generation mismatch and one-second/backwards-time expiry boundaries. |
| `qol-native-input` | All five consumed button lifetimes, trigger lifetime, release/rearm and preserved Shared LT/RT paging; existing native delivery and A/L3 tests. |
| `qol-shared-sdk` | One automatic existing-item move, page zero/later pages, Cube vs Inventory destination, invalid destination refusal, rejected/zero-commit results with no retry. |
| Existing 29 suites | Hook ownership/draining, profile admission, coexistence, item identity, inventory/belt/material/bulk actions, aim and skill discovery, glyph/navigation, and candidate DLL metadata/resources. |

Reproduce with a configured x64 MSVC environment:

```powershell
python plugins/controller-qol/tools/build_local.py --source plugins/controller-qol --build outputs/controller-qol-repair-20261011-build
```

Candidate DLL, CTest logs, SHA256 receipt, pre-repair source backup and the repair
delta are local under `outputs/controller-qol-repair-20261011*`. The candidate
identifies itself as **1.3.1+rev.76-beta.3**. The detailed user checklist is
[AUDIT-REPAIRS-IN-GAME-TESTS.md](AUDIT-REPAIRS-IN-GAME-TESTS.md).
