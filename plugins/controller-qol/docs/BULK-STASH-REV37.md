# LB+L3 bulk advanced-storage deposit - rev.37 candidate

## Behavior and ownership

With stash open in controller mode, hold the configured modifier (LB default) and
tap L3. Snapshot ordinary inventory (page 0), then revalidate and offer each item
to the same eligibility/deposit helpers used by LB+X. Native eligibility decides
which materials, gems, runes and rejuvenation potions can enter advanced storage.
No custom code list, Cube source, belt source, normal-stash fallback, or item creation.
Noneligible inventory items stay in place. The exact normal LB+X path is unchanged.

SDK InventoryService enumerates; ThreadService alternates UI availability checks
and authoritative game work. ItemService::editNativeItem scopes both the eligibility
and deposit calls. Native pointers are never retained. One submission at a time;
subsequent inventory enumeration must observe that runtime ID gone before advancing.
This is source-removal evidence, not independent proof of destination counters.
Full/refused/unconfirmed transfers stop without retry. Changing a snapshot item's
cell/container/code/runtime ID cancels; enumeration errors never imply removal.
256-item snapshot overflow refuses the batch before mutation. Transfer wait is 1s,
batch deadline 30s. Session/player changes cancel; SDK drops game work on session
change and unload. A new request replaces an expired batch with a fresh generation.

Input callback serves native and XInput paths. It consumes L3 only when the stash
accepts the chord, until stick release, preventing native Open Cube on modifier
release. Held chords cannot repeat. Normal L3 outside this context is unchanged.
QOL LB+X/LB+R3 requests refuse while the batch owns transfers. No new hook installed.

## Reused native contract and patch recovery

Module-relative RVAs are inherited from include/native_d2r.h and
MATERIALS-CUBE-ROUTING.md, not newly chosen interception points:

| RVA | Role |
| --- | --- |
| 0x15A0B0 | bool eligibility(item); requires stash mode and native item category |
| 0x08B2D0 | local data context |
| 0x09A480 | local player unit(context) |
| 0x46DA50 | advanced stash destination(player) |
| 0x15F8B0 | transfer(item,destination,4,0,true,placement) |
| 0x1A0780 | finish interaction(3,null,0,0,false) |

Bulk admission compares memory read-only; no safety-check patch is attempted.
Existing belt signatures guard context/player/transfer/finish; Materials signature
guards destination. Full 0x5D-byte eligibility body is from the earlier runtime PE
capture SHA256 81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5.
Static disassembly: stash UI test at 0x15A0BE calls 0xCE500 with 0x18; item eligibility
at 0x15A0F4 calls 0x15F320. These calls are provenance, not new callable entry points.
Installed on-disk D2R.exe code is encrypted; do not derive witnesses from it.
After patches, compare this chain to the native quick-stash action and renew the
reviewed signatures; do not accept an arbitrary executable predecessor. A mismatch
disables only bulk deposit, with a warning. Existing single-item functionality stays.

## Validation

Automated scope/identity/overflow and input-chord regression tests added. Game-side
storage counters, supported categories, visual refresh, and save persistence need
live verification. Test mixed eligible items plus equipment, from Personal, Shared
and advanced tabs; confirm L3 alone still opens Cube and LB+L3 does not. Repeat with
an empty inventory and with the stash closed. Logging remains off by default;
only a bounded completion/refusal summary is emitted.

Release build and all 20 CTest suites passed. No diff whitespace errors. Candidate and deployed DLL SHA256: 3D142EFF7FD57999F86B8F82E73454B3532B1425B04DA1910B5869F5FD34E4E5. Installed ProductVersion 1.3.1+rev.37 in global plugins after confirming D2R and D2RLoader closed. Prior DLL/configuration backed up under before-rev37; configuration unchanged. Live transfer verification pending.


## Rev.38 whole-inventory experiment

User requests all eligible inventory items in one update rather than four-item
batches. Rev.37 is the confirmed rollback point. Rev.38 keeps the same bounded
snapshot and native contracts but loops through it in one authoritative callback.
Each identity is re-resolved after earlier mutations, then eligibility and deposit
reuse the original helpers. No UI round-trip or removal wait between submissions.
A refusal or changed identity stops further submissions. Successfully submitted IDs
are tracked separately and verified together in later inventory scans, including
when a later submission failed. Enumeration failure is never interpreted as removal.
No unconfirmed item is retried. A partial submission is not an atomic transaction
and has no batch rollback. The 256-item snapshot bound remains; normal inventories
are processed in full. One-update work may take longer on large inventories; this
tradeoff is intentional for the requested experiment. No new RVAs/ABI/guards.
Regression tests cover full/partial removal, duplicate sightings and unsubmitted IDs.
Live speed, counters, visual refresh and persistence remain pending.

Rev.38 Release build and all 20 CTest suites passed, including whole-batch/partial verification regressions. Installed after checking game/loader closed; previous DLL/config backed up under before-rev38. Built and installed SHA256 A3708B687BF3CFB37FEFC568D9A75880C522C9B88241364228A044A32E23E7A3, ProductVersion 1.3.1+rev.38. Configuration unchanged. Live full-inventory speed and transfer verification pending.
