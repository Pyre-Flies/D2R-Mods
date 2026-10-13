# Hosted Controller QOL investigation - 2026-10-12

## Runtime evidence

Read-only inspection of the user's running Ladder client, PID 62044, established
that the loaded mod-scoped QOL admits its profiles at startup, before Global Chat
and Maps install their forwarding hooks. Later remote requests reject those hooks
because both companion DLL identities have changed since the recorded profile.
The 20:18:51/52 log reports `IdentifyAll: Game scheduling unavailable`; this is a
generic fallback failure message, not evidence that the client UI scheduler failed.

Exact module hashes:

- D2RCore: `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.
- Global Chat: `a2d98893ea1dddaf5fcaf362fe86306b6983baec17fd7edf6b720de9c5e0a0d8`.
- Maps: `717cbd31c29a9d60410fa96fad9d4cebea6f2e21626dde235554e572548cada8`.

All existing game witnesses for client identification, transfer and native
identification matched. Core stat entry/wrapper/body and sender continuation
matched. Chat's hook at module RVA `0x538B0`, dispatcher `0x53730`, original getter
`0x17CC0`, trampoline slot `0xA75C8`, relocated prologue and Core continuation
all matched the existing byte witnesses. The new hash is admitted only with these
unchanged forwarding checks; older supported artifacts remain supported.

Maps' stat hook remains at module RVA `0x106D0`. Its original-reader pointer moved
from `0xBCD30` to `0xBDD30`; the running pointer equals Core+`0x831DE0`.
Disassembly of the complete recorded hook span shows data RIP-relative operands
moved by `0x1000` and its helper CALL at `0x107BA` changed from `0x84E30` to
`0x857B0`. The new exact full-span witness is `MapsCurrentHook` in
`src/plugin_coexistence_profile.h`, paired exclusively with the new file hash and
continuation slot. This is a bounded forwarding-contract admission, not a claim
about every Maps feature or helper ABI. QOL continues calling the installed hook.

Private reproducible evidence: `outputs/hosted-qol-20261012/` contains the original
QOL log, read-only inspection scripts, module/slot reports, disassembly comparison
and coexistence check results. No memory writes, remote function calls or game
input were performed by those scripts. Do not commit these local artifacts.

## Ground snapshot repair

The live log at 20:19:52-58 shows assignments repeatedly disappearing and holes
being filled by other IDs. `Refresh` previously called `Assign` on every poll.
The candidate takes the initial eligible visible set once per LB hold, retaining
the original identities even when placards temporarily disappear or new loot
arrives. Empty initial snapshots remain empty until release/repress. Submission
still checks fresh visibility, identity, ground mode, range, collision and session;
retaining an assignment does not make a stale item actionable.

## Limits and pending validation

Release candidate built locally; all **33/33 CTest suites passed**, including DLL
artifact/ABI validation and integration regressions. The integration fixture now
releases an empty hold before tracking a new item and testing guard cancellation.
Candidate SHA256:
`6f9f045ae58aa6ef2af6b32ae5d8c7108e14d9bcf5e6b25c2125f253926f282b`.
The loaded mod DLL has SHA256
`d05ac0704f1398271a729e75f1d486349af9b6847b98b807a949d4114fd3ff38`;
a rollback copy and candidate are staged under the private evidence directory.
No deployed DLL or configuration was changed. A closed-client replacement and
restart are required for live candidate testing.

The compatibility repair targets the shared remote ID/transfer guard, including
remote batches using those routes. At 20:32:40 the user pressed LB+L3 on Personal
Stash. The running original DLL logged `Remote deposit refused or faulted` with
`snapshot=23 submitted=0 removedFromInventory=0 skipped=0`. Input and client-side
enumeration therefore reached the batch handler; it stopped at `BatchDeposit`.
That path checks the same sender guard already demonstrated to reject the loaded
Chat artifact. This establishes a blocker, not that no additional blocker remains.
Hosted success and inventory persistence remain to be tested with the candidate.

Walking to loot remains unresolved. The existing `game+0xFA180` path reaches an
immediate pickup request; it does not establish a native approach action. See
`AUDIT-LIVE-FOLLOWUP-BETA4.md`. No guessed movement call or position edit was added.

Further static inspection of the prior `runtime-code.exe` capture around
`game+0xFF700..0xFF886` shows the item interaction branch at `0xFF752` loading
type from `[RBX]` and ID from `[RBX+8]`, then reaching the existing `0xFF836`
caller. Earlier branches call `0xF9860` and `0xFBA20`; these are research leads
only, with no complete input-object ABI, thread or approach semantics established.
They were not called or added to runtime admission.
