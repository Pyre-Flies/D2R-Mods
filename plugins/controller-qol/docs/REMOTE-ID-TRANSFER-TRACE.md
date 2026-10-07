# Remote identification and transfer paths - 2026-10-06

## Provenance and limits

Read-only inspection of running root D2RLoader client PID 30812, game image
base `0x140000000`, Core base `0xC0DE5000000`. No process injection, native calls,
breakpoints, input simulation, item manipulation, suspension or replacement DLL.
The user confirmed the preceding ground-A/belt correction appears to work.

Loaded-module paths and disk hashes:

| Module | SHA256 |
| --- | --- |
| D2RLoader.exe (1.3.1-beta) | `93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10` |
| D2RCore.dll (1.3.1-beta) | `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2` |
| Root Controller QOL Updates.dll candidate | `e21050f5ec6e4834214ac0729b57592b262980fab7dacb53055d5ef6890d59f7` |

Private artifacts in `outputs/remote-id-transfer-trace/`:

- `game-30812.exe`: PE-shaped read-only `.text` capture, SHA256
  `246dcb55785662194ee1103ef159abd2dd37dce66a7711bdc33cc519ae15fdff`.
  This matches the earlier ground-pickup code capture.
- `core-30812.dll`: `.text` capture, SHA256
  `b8dc77e3df599014213071c9b288b3bf23936bcf0fa2544cc04acabfaf7a3ee7`.
  Loader-generated wrappers outside `.text` require separate live reads.
- `inspect_live.py`, `live-contract.json`: exact game profile comparisons,
  live vtable/dispatch slots, module provenance and current UI/cursor values.
- Bounded disassemblies, `span-evidence.json`, `core-send-822dd0.bin`:
  private research evidence, not distributable game assets.

All ten game witnesses in `native_identify_profile.h` match the live process,
including the 512-byte hold-action witness. Existing log at 19:30:26.900 records
native-ID admission accepted; the custom-page bridge is also admitted. The log's
`gameScheduler=1` means the function pointer exists, not that a remote client can
successfully schedule an authoritative callback.

At the inspection snapshot, inventory/stash/Cube UI modes were all zero; cursor
pointer was null and cursor mode was 7. This is not an observed item operation.
The call chains below are disassembly evidence combined with live slot/byte
checks, not an execution trace or proof of remote server acceptance.

## Identification: existing UI path reaches client request dispatch

The existing implementation already models native tome use. Its remote blockers
are the authoritative preparation and verification steps, not absent client code.

| Location | Contract / observed role |
| --- | --- |
| Game `0x1CF3800` | Inventory-grid vtable; live slots `+0xC8 -> 0x2C49F0`, `+0x118 -> 0x2AA7E0`, `+0x100 -> 0x3E2B274`. |
| Game `0x2AA7E0` | Existing hold action, `void(grid*, item*)`; retain its native item-use/readiness validation. |
| Game `0x14F1E0` / `0x14F210` | Existing cursor/source and mode getters, backed by `0x2A88700` / `0x2A88708`; mode 5 is the identification-targeting branch in the activation routine. |
| Game `0x3E2B274` | Live `FF 25 5E F5 FF FF` thunk reads slot `0x3E2A7D8 -> Core+0x8150A0`. |
| Core `0x8150A0` | Existing item-interaction wrapper, preserving listeners; predecessor `Core+0x70E9C8 -> game+0x2C7540`, lookup `Core+0x70E968 -> game+0x2C49F0` both checked live. |
| Game `0x2C760C..0x2C77DE` | Mode-5 branch inside activation `0x2C7540`. Resolves source cursor and target cell, checks blocked target through `0x1C7360`, owner eligibility and grid page, then request readiness `0x1E3300`. |
| Game `0x2C77D9` | Calls existing generic item-use request builder `0xED1E0` with source/target identities and grid-page context. Source mode 2 additionally supplies native auxiliary data through `0x1C2140`. |
| Game `0xED1E0..0xED2E1` | Constructs a 0x24-byte request with opcode `0x26`; dispatch call at `0xED2CA -> 0x3E2B73C`. This is a subordinate witness, not a new direct QOL entry point. |

Proposed adapter: capture exact source/target copies on the UI scheduler; use
fresh client-side source charge reads instead of authoritative `editNativeItem`;
retain visible-grid lookup, existing hold-A/activation wrapper, cursor ownership
and readiness checks. Confirm the exact target is identified and one source
charge has disappeared in client-observed state. Do not describe that as
authoritative/persistence verification or locally edit charges/identified flags.
Start with one Inventory tome and one Inventory target. Other source/container
combinations and bulk continuation require separate checks.

The older research lead `0x1C7A30` consumes a structured input pointer (including
a 16-bit field at input+1), scans native inventory, then builds another item-use
request. It is not established as a direct `identify(target*)` entry point and
should not replace the existing UI route.

## Transfers: native grid action owns destination resolution and placement

The live inventory-grid vtable slot `+0xD8` points to game `0x2C73D0`.
Observed ABI from callers and body:

```cpp
// Candidate contract; not yet called by a new remote QOL adapter.
void __fastcall TransferGridItem(Grid* source, const Cell* cell, uint8_t destinationPage);
struct Cell { int32_t x, y; };
```

| Location | Contract / observed role |
| --- | --- |
| Game `0x2C73D0..0x2C74C5` | Resolves cell through virtual `+0xC8`, rejects null/blocked item (`0x1C7360`), resolves destination owner, and invokes native transfer. Does not trust an item pointer retained across callbacks. |
| Game `0x2C7B65..0x2C7C24` | Native activation calls `+0xD8` with destination 0, 3 or 4 after Cube/stash/UI and item checks. Cube UI mode `0x19` and stash mode `0x18` participate in routing. These native page IDs are Inventory 0, Cube 3 and stash storage 4; they are not SDK container enum values or Shared sub-page numbers. |
| Game `0x2A9DB2`, `0x2A9F12` | Existing controller item-message branches also call `+0xD8` with pages 3 and 0. Native helper `0x2AA400` uses the same virtual action at `0x2AA571` / `0x2AA694`. Actual physical input execution was not recorded this run. |
| Game `0x2C740C -> 0x15EEC0(true)` | Destination page 4 resolves the current native stash owner; other ordinary destination pages obtain the local player via `0x8B2D0 -> 0x9A480`. Destination `0xFF` takes a different `0x15BDF0` route and is excluded from the proposed ordinary transfer adapter. |
| Game `0x15EEC0..0x15EF6C` | Reads optional owner ID via `0x15EDB0`; resolves type-0 client unit via `0x9A5D0`, with additional `0x9A720 -> 0x38CD60` check for the true argument. If no optional owner, returns local player. Do not replace it with a guessed Shared owner/category index. |
| Game `0x2C7446` | Page-4 item eligibility uses `0x15A340`; native behavior retained by calling the grid action. |
| Game `0x2C7457` | Reads source native page through `0x36CFE0(item*)`. |
| Game `0x2C7479 -> 0x15F8B0` | Native bool transfer `(item, destinationUnit, destPage, sourcePage, true, placement*)`. Local placement has 32-bit X/Y at +0/+4 and engaged byte +8, initially disengaged. Same underlying helper used by existing Shared transfer research. |
| Game `0x15F94F -> 0x3865B0` | Native free-space search populates disengaged placement. Unavailable placement takes failure/finish behavior, rather than selecting another QOL destination. |
| Game `0x16008B..0x160117` | Ordinary true-flag stored-item branch builds opcode `0x54` via `0xEC820` at `0x160112`, with native item ID, source/destination page and placement. Other owner/storage branches use different requests; do not hard-code a universal opcode. |
| Game `0xEC820..0xEC876` | Generic 0x15-byte request builder, dispatch call `0xEC85F -> 0x3E2B73C`. |

This supports a guarded UI-thread adapter using exact source identity/cell and
native destination resolution. It does not yet prove that every Shared/advanced
page or embedded Cube grid uses the same vtable. Preserve the already established
custom-page and advanced-materials routes. Explicit Cube transfer must never
fall back to stash on refusal. Wait for the same item to appear in the intended
container/page; a returned native call is not success.

## Common loader dispatch and byte witnesses

Game thunk `0x3E2B73C` bytes `FF 25 DE F6 FF FF` resolves through live game slot
`0x3E2AE20` to Core `0x822DD0`. Its inspected body checks pointer/length, applies
duplicate/policy handling, and forwards through loader-owned dispatch. Both
identified request builders reach this wrapper. Keep it intact; no packet
construction or direct network API calls are proposed for QOL.

Key entry witnesses retained for later exact-build admission:

```text
game 2C73D0 (32 bytes):
48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 40 48
8b 01 41 0f b6 f0 ff 90 c8 00 00 00 48 8b d8 48

game 15F8B0 (32 bytes):
40 55 53 56 57 41 54 41 55 41 56 41 57 48 8d ac
24 08 ff ff ff 48 81 ec f8 01 00 00 48 8b 05 f5

game 15EEC0 (32 bytes):
40 57 48 83 ec 30 48 8b 05 fb c3 86 02 48 33 c4
48 89 44 24 28 0f b6 f9 48 8d 4c 24 20 e8 ce fe
```

These short entries are locator evidence, not sufficient complete adapter
admission. The private span ledger also retains full action body `[2C73D0,2C74C5)`,
transfer body `[15F8B0,16018C)`, stash resolver `[15EEC0,15EF6C)` and bounded request
witness hashes. Future code must guard the reviewed action/lookup/owner contracts
and current vtable bindings, cancel on session/tab/source changes and fail open
on unsupported builds. All newly documented addresses are build-specific.

| Exact game span | SHA256 of captured bytes |
| --- | --- |
| `[0x2C73D0,0x2C74C5)` | `cf5e604e37b8ca397d443eea13d3d45407f5cb8f50c81f2df8cbe604abba6139` |
| `[0x15F8B0,0x16018C)` | `8bcaed3ffadf5f1bcffe06f876a031441b55313cd8519ae724defff2b38d4910` |
| `[0x15EEC0,0x15EF6C)` | `e5ec69bc2e87ea421258e4ef3b269b50a87ac10eca72b7753cf6dfada9cb1ea5` |
| `[0x2C760C,0x2C77DE)` | `62bff1550bc1d3618568172ab0f5ba531a660300f9a8bf0edab8dfdd1279266d` |
| `[0xED1E0,0xED2E1)` | `f3e0a6de1b41affc475083d15103436d73350e775eac9c50675bd19427b3e286` |
| `[0xEC820,0xEC876)` | `1cbfd32d9351c0f00650a5ce7eff1dbcd7eb6639faf030285a6945a0f526968c` |

## Next validation

No additional discovery-logging DLL is necessary to establish these candidate
paths. A functional test adapter should log source/target IDs, destination,
admission/refusal, one submission and later observed confirmation. That DLL would
require a client restart to install. The server need not be stopped for client
inspection or client DLL replacement.

Before expanding to bulk, validate single ID with exact charge decrement and
single transfers in both directions, full destination, source moved while queued,
tab change, inventory close and session exit/rejoin. No implementation was added
and no new native path was executed in this tracing pass.

## User-performed native baseline, 19:45-19:46 EDT

The user subsequently performed actions with the existing candidate still loaded:

- LB+A on an unidentified cap picked the cap up instead of identifying it.
- Ordinary tome use (hold A on tome, then A on cap) identified the cap.
- Personal Stash deposit and withdrawal both worked.
- Shared Stash deposit and withdrawal both worked.
- No Horadric Cube was available; Cube behavior remains untested.

The saved delta `outputs/remote-id-transfer-trace/native-actions-delta.log`
has SHA256 `adbca0bf42cf4d41587e89333c31240e082e4fcb2ed5ecc52f808aeac0339079`.
At 19:45:10.318-.342 the plugin received controller activation for SDK item
handle 25, Inventory container 4, cell (8,0), packed code `0x20706163` (cap),
quality 4, unidentified=1, modifier active and LB=1. Input capture therefore
worked. At 19:45:28.100-.121 the same cap's activation was logged with modifier
inactive, consistent with the user's subsequent normal identification sequence.
That callback precedes native activation and still reports unidentified=1; it
is not a post-identification confirmation or charge-count measurement.

The active config has `quick_identify=true`, `native_identify=false`,
`require_tome_or_scroll=true`, `consume_tome_or_scroll=true`. The current single-ID
handler submits `ExecuteIdentifyTask` to the authoritative game scheduler; on
rejection it clears pending state and returns `Decision::Continue`. In a remote
client this explains the ordinary inventory pickup after LB+A. That branch does
not log its result code, so the exact rejection value was not captured by this
test; source and SDK ownership rules establish the expected failure path.
Changing the native-ID config alone does not bypass this initial scheduling step.

The trace records Bank/Inventory opening and Shared tab input, but does not log
ordinary native transfers as QOL requests. Successful native identification and
both stash round trips are user-confirmed behavior, not instrumented proof of
every internal call or persistence. No exact tome charge decrement was supplied.
This baseline supports implementing client-request adapters for ID and ordinary
stash moves; it does not qualify the new adapters or Cube support.

## Implemented test candidate: direct ID and ordinary transfers

The following supersedes the proposed tome-cursor adapter above. The user
reported that the old multi-stage tome-use flow felt clunky. Remote single ID
now calls the native request builder directly, supplying the captured tome and
target identity. It never holds the tome, enters targeting mode, activates the
grid or synthesizes input. The existing configured offline native flow remains
available; offline default SDK identification is preserved.

Only `runOnGameThread == Unavailable` admits either client fallback. No fallback
follows a queued authoritative operation, another scheduling error, a failed
mutation or an unconfirmed native submission. Remote ID requires controller UI,
consumption enabled, an Inventory target and a charged Inventory tome. Remote
bulk ID, loose scrolls, stash/Cube ID and no-consumption identification are not
implemented. Native refusal fails open before admission; once a request is
accepted, the triggering item action is consumed to avoid ordinary pickup.

### Direct identification request ABI

Disassembly at `0x2C769D..0x2C77DE`, independently cross-checked against the
second item-target caller `0x2CAE87..0x2CAF39`, establishes the x64 argument order
for `void __fastcall game+0xED1E0(...)`:

| Argument | Provenance / meaning | Inventory-only adapter |
| --- | --- | --- |
| RCX low byte | Native source-use flag from `0x308E80(source)` | Preserve returned flag, never hard-code it. |
| EDX | Source ID (`0x34A330(source)` / checked unit+8) | Captured tome runtime ID. |
| R8D | Source mode (`0x34AB60(source)` / checked unit+0xC) | Require mode 0. |
| R9 low byte | Source native page from `0x36CFE0` | Require page 0. |
| stack +0x20 | Packed source XY from `0x34A110` | Compare with captured SDK cell. |
| stack +0x28 | Target runtime ID | Captured item runtime ID. |
| stack +0x30 | Target unit type | 4 (item), validated before call. |
| stack +0x38 | Target native page (grid+0x630 in caller) | 0, independently checked on target. |
| stack +0x40 | Packed target XY from `0x34A110` | Compare with captured SDK cell. |
| stack +0x48/+0x50/+0x58 | Three optional arrays used only when source-use flag is nonzero AND source mode is 2 | All null; mode 2 is excluded. |

Stack offsets are **caller RSP before CALL**. The callee's pushed-RBP view is
offset by +0x10. `0x34A110` returns X in bits 0..15 and Y in bits 16..31; the
request builder serializes only each coordinate's low byte, so the adapter
rejects SDK cells outside 0..255 or differing native coordinates.

The native caller checks target blockage (`0x1C7360`), allowed owner and page,
then readiness (`0x1E3300`). The adapter additionally requires both source and
target to be ordinary stored Inventory items owned by the local player, checks
both blocked states, exact runtime ID/code/class/seeds/cells and a positive
stat-70 quantity. `0x1E3300` is a native request readiness gate, not a pure
predicate: its failure branch may invoke `0x1EF1C0` after `0x8D4B0` /
`0x1F0A40` checks. Preserve its result and do not retry.

Before submission, `0x1C6AC0(target)` preserves native pending-target/stat-list
bookkeeping (list/state key `0x36`). It does not identify the item or debit the
tome. The adapter calls this existing native function instead of writing its
internal state. Helper `0x308E80` derives the opaque native use flag from active
item data: type tests `0x373890`, context `0x34A0E0`, code/category lookup
`0x36CDE0 -> 0x313C50`, fallback class lookup `0x349860 -> 0x314110`, and a
20-entry table at `0x236FE30` with stride 12 and flag byte +8. These are recorded
as subordinate observations, not additional QOL entry points or copied tables.
The fallback row index is at data row +0x94. Source type 0x12 immediately returns
false; a false native use flag is valid and is passed through.

The native builder owns serialization and passes its request through the intact
loader dispatch at `0x3E2B73C`. Both new adapters check that six-byte thunk,
slot `0x3E2AE20 == Core+0x822DD0`, and the first 0x43 bytes of that generated Core
wrapper, in addition to the exact Core file hash. No direct networking API or
handwritten packet serialization is added. A native call returning is only a
submission: remote ID waits for the same target's identified flag and exactly
one fewer tome charge. This is client-observed confirmation, not a claim of
save persistence or an authoritative SDK transaction.

### Transfer adapter and additional owner evidence

`client_transfer.cpp` mirrors the ordinary grid action's reviewed preflight and
calls `0x15F8B0` directly, avoiding dependence on an unproven Shared/Cube grid
widget layout. It retains `0x1C7360` blocked-item checks; destination page 4 uses
`0x15EEC0(true)` plus `0x15A340(item)` eligibility. The resolved owner must match
the local player for Personal Stash or the existing selected Shared owner
resolver (`0x23AD80 -> 0x2EF880 -> 0x9A5D0`) for Shared Stash. Placement is a
zeroed, aligned 16-byte buffer: native free-space search selects the cell.

Full guarded eligibility body is `[0x15A340,0x15A492)`, bool(item*). It retains
selected-owner and storage restrictions, UI mode 0x18 and subordinate native
eligibility rather than admitting every item from QOL. The full owner-selection
body `[0x15EDB0,0x15EEC0)` fills an optional ID: uint32 at +0, engaged byte +4.
It uses UI registry getter `0x8461C0`, registry hash/index witnesses
`0x5F864EB9001295CD` and `0x8D4D18A675312BD4`, then the resolved panel +0x110
record pointer and ID at record+0. These are native implementation observations;
QOL does not reproduce that hash lookup or dereference those panel fields.

Only normal Personal/Shared pages are supported. Shared selection is captured
before queuing and rechecked with exact SDK page identity; previous-season and
advanced-materials routes are excluded. Explicit Cube transfers require a Cube
in Inventory for deposits and an open Cube for withdrawals. A failed materials
route is never converted into a Cube move. Source SDK identity includes runtime
ID, code, class, generation/item seeds, container, page and cell. Confirmation
allows native automatic placement but requires the same item and requested
destination, including the exact Shared page. SDK handles may be refreshed only
by finding that exact identity, never by matching code/cell alone.

New transfer work owns copied state, has a 750ms initial queue bound, a 2.5s
confirmation bound and a 480-callback cap. Submission is latched before the
native call, including when it returns false. Session events invalidate callback
generations; tab/source/player/UI changes stop work. No pointers survive across
callbacks except comparison-only tokens in the existing native-ID machinery.
Single-ID authoritative requests now also use owned copied state and generation
tokens, eliminating that request's discarded-callback heap leak/busy latch.
Existing legacy move-payload and bulk-stash lifecycle concerns are separate.

### Compatibility witnesses and validation limits

`client_identify_profile.h` records full builder, use-flag, position, readiness,
pending-target and caller spans, each with RVA, size and SHA256. New guarded
spans are `[ED1E0,ED2E1)`, `[308E80,308F77)`, `[34A110,34A197)`,
`[1E3300,1E3329)`, `[1C6AC0,1C6B5B)`, `[2C760C,2C77DE)`.
`client_transfer_profile.h` records full transfer, stash-owner, stash-eligibility
and owner-selection spans. Shared helpers reuse the existing reviewed byte
witnesses. `client_request_profile.h` records the common dispatch guard.
All witnesses derive from the exact PID-30812 capture and Core-wrapper read
identified above. An attempted repeat live profile comparison found that the
client had exited; no new candidate was executed during that inspection.

Release build and all 26 CTest suites pass. Added tests check direct-ID x64
argument order, packed coordinates, exact charge evidence, moved/reused item
identities, destination/Shared page confirmation, one-submission behavior,
queue/confirmation bounds and stale callbacks after cancellation/rejoin.
Artifact tests verify manifest ABI, exports and metadata. These tests do not
establish remote server acceptance or live UI behavior. Required next checks:
single inventory ID (including last tome charge), both Personal/Shared stash
directions, explicit Cube transfer when available, full destination, ordinary
mouse/controller actions, and session exit/rejoin. Cube was unavailable for the
user's native baseline and remains untested live.

## First candidate validation and admission correction, 20:07-20:10 EDT

The installed candidate SHA256 was
`05ebc85d7264049283598037261d1d8de0f3e7a8b93887f9117be124e1627e9f`.
The user confirms that single-item ID worked quickly and efficiently on
**Aldur's Gaze antlers**; the cap belonged to the earlier native baseline test.
The item name is user-provided, not inferred from the numeric confirmation. Log:
20:09:14.521 inspected code `0x20387264` (`dr8 `), quality 5, unidentified=1;
20:09:14.568 direct request target 12 / tome 10 / charges 7, followed at
20:09:14.600 by client-confirmed identification and charges 7 -> 6.
This confirms the direct path in this remote session, not every source/target
combination or persistence across restart. Gems/materials/runes were not tested.

Personal/Shared LB+X did not work because client-transfer initialization failed:
20:07:54.531 reports a byte mismatch at game `0x23AD80`, followed by module
disabled at 20:07:54.541. The new module incorrectly required literal
`SharedOwnerBytes`; the repository already has `QolShared::ValidateOwner` for
the loader's legitimate relocation. No transfer was submitted by this module.

Read-only inspection of root client PID 17892 (same game/Core disk identities)
shows only offsets +0x5F and +0x60 differ from the literal owner witness. The
entire body outside CALL displacement +0x5F..+0x62 matches. The instruction at
`0x23ADDE` resolves to game thunk `0x3E2B310`, bytes `FF 25 92 F5 FF FF`, then
slot `0x3E2A8A8 -> Core+0x1C9E00`. The existing `OwnerVector131` wrapper witness
matches in full. All ten new ID/transfer profile spans also match live memory.
Private read-only evidence: `outputs/remote-id-transfer-trace/shared-owner-current.json`
and `check_shared_owner.py`; no calls or writes to the client were performed.

The correction reuses `ValidateOwner` while retaining the exact Core hash,
surrounding owner body, opcode, resolved wrapper target and wrapper bytes. It
does not accept arbitrary changed CALLs. The shared-owner regression suite
checks permitted displacement changes, rejected opcode/body changes and invalid
base access. The corrected Release build passes all 26 CTest suites. The newly
admitted Personal/Shared transfer path still needs runtime retesting. Advanced-materials smart
deposit remains a separate authoritative route and is not fixed by this guard.

## Empty selection and advanced deposits, subsequent remote candidate

The user confirmed that candidate SHA256
`bbd051da5afb666d5af5982f2364db25a34a978d9d5a6328f81152f99d1a1688`
transfers actual items in both directions through Personal/Shared Stash and that
LB+Y Cube transfers work. They then reproduced LB+X on empty Shared cells moving
the top-left Inventory item, across Shared tabs, and reported rune/gem/material
(rejuvenation) deposits failing with and without smart depositing.

The LB+X target came from the last item-tooltip callback, which does not prove
current controller selection. The remote advanced-tab branch also explicitly
discarded `Unavailable` scheduling without submitting anything; ordinary remote
stash deposits lacked the existing offline smart-deposit branch.

### Selected-cell contract

All addresses below are relative to the same game build/capture recorded above.
Read-only inspection of root client PID 32092 uses the same Core and loader
hashes. No process code was called or modified during these reads.

| RVA / field | Finding and use |
| --- | --- |
| `0x846020`, global `0x3440170`, manager `+0xD0` | Existing guarded UI focus getter and focus-state chain; null-safe direct reads avoid its assertion path |
| `0x876160..0x87617E` | Full 30-byte controller-focus predicate: compares widget against focus-state `+0x190` |
| `0x876180..0x87619E` | Mouse-hover predicate instead compares focus-state `+0x178`; not used as a controller fallback |
| focus-state `+0x188` / `+0x190` | Active parent / actual controller widget; widget can become null when switching to mouse/chat |
| `0x2A89C0..0x2A8AD8` | Full 280-byte native selected-item resolver witness. Controller branch `0x2A8A3B..0x2A8A55` verifies focus and reads widget `+0x544`; later dispatches cell lookup at vtable `+0xC8`. The earlier mouse branch is deliberately not invoked by the new selection check |
| widget `+0x544`, `+0x548` | Controller cell X/Y, signed 32-bit integers. Negative/unreasonable coordinates reject the shortcut |
| InventoryGrid vtable `0x1CF3800`, slot `+0xC8 -> 0x2C49F0` | Read current cell's item using the existing native lookup. Its owner resolver `0x2A7810`, lookup entry and item code/page getters retain byte guards |
| Advanced widget vtable `+0xC8 -> 0x2CE900` | Existing bound/display-item contract at `+0x608` / `+0x600`; accept only the actual controller widget's matching runtime ID and code |

The live sampler observed the Shared `grid` widget with vtable `0x1CF3800` and
cells `(4,4)`, `(4,5)`, `(5,4)` while the user selected empty cells. Switching back
to chat cleared `+0x190`; this explains the first null snapshot. The three new
full witnesses (controller focus, selected-item resolver, deposit predicate)
match PID 32092 byte-for-byte. Private evidence is
`outputs/remote-id-transfer-trace/focus-empty-shared-samples.json` and
`storage-profile-admission.json`. Absolute heap addresses are transient.

LB+X checks the native current cell against the tooltip runtime ID/code/page
before any stash route, including offline and advanced withdrawal routes. A
null/empty/unsupported selection cancels the custom shortcut; it never searches
another grid by code or remembered cell. The remote submit callback checks again,
then retains the existing SDK identity/seeds/source-container checks. Native
lookup handles multi-cell items without assuming the selected cell is the item's
top-left corner. No selection check is applied after submission, when native UI
completion may legitimately change focus. Unsupported custom focus widgets stop
the shortcut; normal native interaction remains available.

### Remote advanced deposit contract

The game controller input method at `0x2A98B0` obtains the selected item through
`0x2A89C0` at `0x2A9C56`. The advanced deposit branch begins with eligibility at
`0x2A9CF1`, checks the resolved cell item with `0x1C7360`, resolves local player
and current advanced owner, invokes `0x15F8B0` at `0x2A9D68` with destination
page 4, actual source page and automatic placement, then finishes interaction
at `0x2A9D7D`. The plugin uses those existing helpers without synthetic input.

| RVA | Contract / bounds |
| --- | --- |
| `0x15A0B0..0x15A10D` | Full 93-byte `bool(item*)` predicate; requires stash UI, native expansion eligibility, and membership in the native advanced-storage registry. New full witness in `storage_focus_profile.h` |
| `0x15F320..0x15F42E` | Registry membership helper reached by the predicate; iterates registry rooted at global `0x22BE698` and native item key. Recorded as a lead, not independently called or reimplemented |
| `0x15F430..0x15F539` | Related category/key registry lookup; recorded only, not used |
| `0x46DA50` | Existing guarded current-season advanced owner resolver `(localPlayer*) -> owner*` |
| `0x23B980` | Existing guarded previous-season selection predicate. Remote Inventory deposits refuse previous-season contexts |
| `0x15F8B0` / `0x1A0780` | Existing guarded transfer helper `(item, advancedOwner, 4, 0, true, disengagedPlacement)` then finish `(3,nullptr,0,0,false)` |

Only `runOnGameThread == Unavailable` enters the new client route. It captures
tab/normal Shared page, requires current Inventory identity and controller
selection, rechecks stash/season/player/cursor/blocked state, and asks the native
eligibility predicate before resolving the advanced owner. Eligible items on
tabs 0..4 use that owner. Ineligible items retain the ordinary destination on
tabs 0/1 and the existing Cube route on tabs 2/3/4, requiring a carried Cube.
Explicit LB+Y remains a Cube request, not a smart deposit. Unknown tabs and
advanced proxies cannot become Inventory deposits. Native eligibility owns mod
category membership; no rune/gem/potion code whitelist is introduced.

The submission fence is latched before the native call regardless of its return
value. No false return or observation timeout causes a Cube/ordinary fallback or
retry after submission. The consumed item need not survive as a counter with
the same runtime identity. Logs therefore distinguish an exact source leaving
its Inventory position plus a same-code Inventory quantity decrease from an
ordinary destination confirmation. The message explicitly says
`advanced-source-decrease-observed-destination-unverified`: this is not a server
acknowledgment, counter increment proof or save-persistence claim. Concurrent
manual same-code changes can confound aggregate quantity observation; they do
not cause another request. Confirmation is bounded by the existing lifecycle
fence, player/tab/season checks and deadline.

Release compilation and all 26 CTest suites pass, including added advanced vs
ordinary/Cube route cases on tabs 0..4, unknown tabs and proxy exclusion. Artifact
manifest/export checks pass. Guard matches and focus reads are live evidence of
layout/admission only. Required behavior retests: empty Shared cell does nothing,
actual-item Personal/Shared and Cube transfers still work, one rune/gem/rejuvenation
deposits to its counter from ordinary and advanced tabs, ineligible/full-storage
cases, and normal mouse/controller actions. These changes are not yet released.

## Remote Identify All and advanced withdrawal destinations

The user confirmed the preceding storage-focus candidate working, then reported
Identify All, Materials LB+A and advanced-tab LB+Y still unavailable. In its
20:35:17 log, Identify All stops at `Game scheduling unavailable` before planning
any targets. At 20:38:24, focused Materials LB+R3 successfully submitted and
observed two full rejuvenations reaching the belt, then stopped at native belt
capacity. The retained log has no ordinary ItemInteraction activation for the
advanced potion shortcut; counter widgets need input handling ahead of that
ordinary-item event. This observation is specific to this tested session.

Identify All now falls back only from an `Unavailable` authoritative scheduler
and only for controller requests with charge consumption enabled, a highlighted
Inventory tome, admitted direct-request guards and lifecycle listeners. The UI
callback verifies current controller selection and snapshots main Inventory.
It excludes Cube contents and does not add items acquired after the snapshot.
Each target and the selected tome retain runtime ID, code, class, seeds and
source location checks. Remote quantities use the already guarded client grid
and native stat getter, not authoritative native-item access. After one existing
`0xED1E0` direct request, the target flag and exact one-charge decrement must
confirm before advancing. Blocked tomes wait within the existing bound; empty
tomes, changed identities, session changes, guard failures or unconfirmed
requests stop the batch. The same tome is retained even if another has charges.
There is no native cursor-use sequence, local edit or request retry. Offline
SDK/native batch routes retain their existing behavior.

Materials now distinguishes three destinations throughout scheduling and
observation. LB+A on a currently focused rejuvenation counter submits one belt
withdrawal through the existing `0x159B30(item, seasonOwner, 3)` path and native
free-belt-slot planner. A recent UI-validated counter focus publishes eligibility
for the normalized controller input callback. An accepted A press is consumed
until A release, including when LB is released/repressed first; ordinary A is
passed through when the counter request is not accepted. The owned request is
rechecked against the actual widget on the UI thread before sending. No heap
payload or native pointer crosses threads. LB+R3 keeps its existing refill loop.

LB+Y now routes advanced proxies before the ordinary Cube mover. It calls the
already fully guarded widget wrapper `0x2CF680(widget, zeroCell, page=3)`;
the previously documented native wrapper maps **page 3 to withdrawal destination
2**, selects season owner and finishes interaction. This is not destination
enum 3 (Belt) and not an ordinary item transfer of the counter proxy. A carried
Cube is required. Confirmation observes the requested code's quantity increase
in Cube, not Inventory. No temporary withdrawal to Inventory is performed.
LB+X continues to pass widget page 0 for Inventory.

Single advanced requests expire before submission after 750 ms, check focus,
stash tab and season, and wait at most 1500 ms for a destination increase.
Session resets and generation tokens invalidate old callbacks. Destination
quantity increases remain client observations; concurrent manual same-code
changes may confound them. A failed/timeout request never retries. A full belt
is rejected by the native planner; Cube capacity/eligibility remains native.

No new RVAs or layout assumptions are introduced in this follow-up: all native
destinations reuse the admitted profiles and contracts above and in
`MATERIALS-NATIVE-CONTRACT.md`. All 26 suites pass, including Cube wrapper
page/destination argument tests, quantity-container selection, accepted/rejected
A behavior, held-A/LB-release deduplication, existing bulk snapshot/charge and
identity cases, and DLL ABI/export checks. These are automated checks; remote
Identify All, single belt withdrawal and direct advanced Cube withdrawal still
need live qualification. Test last tome charge, moved items/session exit, both
rejuvenation sizes, full belt/Cube, each advanced category, and ordinary A,
LB+X and LB+R3 regressions.

## 2026-10-06: remote bulk deposit, Personal ID and ordinary stored potions

The user confirmed the preceding Identify All/Materials belt/advanced Cube
candidate working. That does not cover the new routes below. All RVAs refer to
the same reviewed decrypted game capture SHA256
`246DCB55785662194EE1103EF159ABD2DD37DCE66A7711BDC33CC519AE15FDFF`.
Existing exact Core, dispatcher and relocation-aware owner profiles still apply.

### ID source and target pages

Native caller `0x2C7620..0x2C77DE` retains source R14 and target R15. Target
lookup uses grid virtual `+0xC8`; blocked/readiness/mark helpers are `0x1C7360`,
`0x1E3300`, `0x1C6AC0`. Target widget page byte `+0x630` is read at `0x2C766A`
and `0x2C76AF`; pages 1/2 are rejected in this branch. Calls to `0x34A110` at
`0x2C76A0`/`0x2C76AA` obtain target/source packed cells. `0x36CFE0` at
`0x2C76D4` obtains source page; `0x34AB60` obtains source mode. Mode 2 enters
the equipment auxiliary-array path, excluded by our profile. `0x308E80` at
`0x2C7706` obtains the source use flag. Argument construction at
`0x2C77AF..0x2C77D9` feeds the direct `0xED1E0` builder. Source and target
pages are therefore independent native values, not fixed Inventory constants.

Remote ID now admits Inventory page 0 and Personal page 4 for source and target.
Existing grid/owner, mode, identity, packed-cell, readiness and dispatch guards
remain required. Both grid owners must be the local player; Personal requires
its visible grid on tab 0. Page 4 alone cannot admit Shared. Cube/Shared supplies
and targets remain excluded. Static caller evidence supports page arguments;
it does not prove server acceptance of Personal Stash use.

Single ID prefers charged Inventory then Personal tomes, followed by loose
Inventory then Personal scrolls. Loose quantity 0/1 conventions are admitted;
negative quantities and modded multi-scroll stacks are excluded. The direct
request performs no cursor sequence or SDK edit. Scroll confirmation requires
the exact target's identified flag, absent source SDK handle and absent source
runtime ID in guarded client lookup `0x9A5D0`. Neither identification nor source
disappearance alone confirms. Closed panels, changed identities and timeouts
stop without retry. Tome confirmation retains the exact one-charge check.
Remote Identify All accepts a highlighted Personal tome but snapshots Inventory
targets only. Offline identification routing is unchanged.

### Sequential remote deposit-all

LB+L3 retains authoritative scheduling when available. Only `Unavailable`, with
session listeners registered, permits the UI route. Snapshot at most 256
Inventory items once; reject overflow. Capture current stash tab and normal
Shared page if selected, excluding previous season. Validate selection and
player/session on every continuation; changed source ID/code/class/seeds or
location stops the batch.

UI-only `QolClientTransfer::BatchDeposit` shares existing exact Core/dispatch/
transfer guards. It freshly resolves SDK/client identity, native page 0/mode 0,
cursor and blocked state. Eligibility `0x15A0B0` decides whether to skip;
current advanced owner `0x46DA50` supplies destination. Submit
`0x15F8B0(item, owner, 4, 0, true, zeroPlacement16)`, then finish via
`0x1A0780(3, nullptr, 0, 0, false)`. Focus is not required for each entry:
the explicit batch command covers Inventory, not the currently selected cell.

Only one request may be pending. Successful full Inventory enumeration must
observe its runtime ID gone before advancing. A changed source or 1500 ms
without disappearance stops; overall deadline is 30 s. Enumeration failure
never counts as removal. Once attempted, native return values cannot trigger
resubmission; faults stop the batch including a possibly sent request. No
rollback or destination-counter proof is provided. Logs distinguish source
removal from unverified advanced totals. Offline one-update batching is retained.

### Ordinary stored potion source pages

Native caller reads widget page `+0x630` at `0x2AA7A5`, sets R9B=1 at
`0x2AA7B2`, passes owner R14/item RSI and calls `0x15F660` at `0x2AA7C5`.
In that helper, R8B becomes R15B (source page), R9B becomes BL (stored flag).
Page 4 selects the stash-owner branch at `0x15F70A` even for local-player owner.
`0x15EDB0` at `0x15F710` supplies selection owner ID or -1. Source-cell helper
`0x1601E0` is called at `0x15F754`, item-ID getter `0x34A330` at `0x15F76B`.
The `0xEC820` request at `0x15F782` uses opcode 0x53, R9D source page, R8D
selection owner, EDX item ID, and stack source-cell/target-belt-slot arguments.
Full stored-placement guards retain reviewed auto-belt detour compatibility.

Remote LB+A now passes native pages 0/3/4 for Inventory/Cube/stash. Capacity
uses local-player Inventory. Shared source uses the selected normal stash unit;
Personal/Cube use the local player. Admission checks open Cube mode 0x19 or
stash mode 0x18, tab 0/1, normal Shared page, no previous season, guarded native
unit/code/page, no blocked item/cursor, full `0x15EDB0`, FindPanel `0x846170`,
selected-tab `0x23AF50`, relocated owner `0x23AD80` and owner ID `0x2EF880`
(bytes `8B 01 C3`). Confirm the same item reaching Belt. LB+R3 retains
Inventory then Materials scope. Offline Cube single-potion support uses SDK
Inventory staging then belt placement, requiring staging space.

### Validation

All 26 suites pass: independent ID pages 0/4 and excluded pages; loose-scroll
confirmation; belt source pages 0/3/4; remote deposit wait/removal/timeout/manual
movement; existing regressions; DLL ABI/exports. These are automated checks.
Live qualification remains: mixed deposits with ineligible items across tabs,
counter totals/persistence, loose scroll without a charged eligible tome,
Personal sources/targets and Personal-tome Identify All, ordinary stored
potions, full belt, tab/session cancellation and an offline controls pass.

## 2026-10-06: embedded Cube routing and Shared potion input

The preceding candidate SHA256
`FEDD4AEC0D46EA3371B4A627B2F6C4EFD82CCD6AD4C390F6D51EFFCF981E3D9C`
was user-confirmed for LB+L3, identification, Personal potion placement and
standalone Cube potion placement. The log at 21:10:00 records a bulk snapshot
of 14, three submissions/removals and eleven skipped. At 21:11:54 and 21:12:00,
runtime item 30 was observed in Belt. Five Personal-tome Identify All targets
completed at 21:13:36. These do not independently prove counters/persistence.

Embedded Cube LB+X failed before submission at 21:10:12..25 with source SDK
container 5 and destination Inventory; standalone withdrawal worked at 21:10:44
(`nativePage=3->0`). The dispatcher unconditionally routed Cube sources to
Inventory, while the client mover demanded UI mode 0x19. Potion attempts at
21:12:08 and 21:15:57 also reported Cube container 5 and failed preflight.
The embedded view belongs to stash mode 0x18 on category tabs 2/3/4. The current
live client matches the existing UI getter at `0xCE500`; read-only inspection
confirms mode table RVA `0x2A2ADA0` with stash=1 and Cube=0. The loaded Core hash
is the same reviewed `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
This reuses existing guards/layouts and does not introduce a new UI offset.

`EmbeddedCube` qualifies source Cube plus open stash and tab 2..4. LB+X captures
that context, reuses guarded `0x15A0B0` eligibility and `0x46DA50` advanced
owner, then passes destination 4/source 3 to the already fully guarded
`0x15F8B0` and finishes via `0x1A0780`. Quantity observation scans the source
Cube rather than always scanning Inventory. Native eligibility chooses the
item's counter category. Ineligible Cube items are refused, without Inventory
fallback. Tab/season, exact identity/focus and one-submission guards remain.
Standalone Cube LB+X still requests Inventory. Explicit LB+Y retains
Cube/Inventory semantics, capturing embedded context when applicable. The
authoritative helper also accepts source page 3 for this explicit deposit;
its default Inventory page 0 behavior is unchanged.

Embedded-Cube LB+A admits the stash/tab context plus actual controller focus,
retaining exact native Cube page 3 and item/capacity checks. It does not admit
arbitrary Cube contents from ordinary stash tabs with the Cube closed.

For normal Shared, the user repeated LB+A and observed ordinary pickup. At
21:17:38.883 the log shows ControllerActionBegin with L1 held and a subsequent
end, but no ItemInteraction activation or belt request. The ordinary-item
listener therefore cannot own this input in the tested view. The normalized
controller A handler now also accepts a recently UI-qualified Shared potion.
The tooltip must match the actual selected widget/cell, SDK Shared container,
tab 1 and potion eligibility. A separate 250 ms focus stamp authorizes copying
the existing identity/player into the belt mailbox. The UI callback rechecks
normal Shared page, exact item/focus, owner guards and belt capacity before
submission. Existing AcceptedPress handling consumes accepted A until release,
including LB release/repress, preventing concurrent native pickup. Materials
counters keep their own qualified route; ordinary A and other container input
stay unchanged. No new native hook or offset is introduced.

All 26 suites pass, including embedded/standalone admission, eligible versus
ineligible Cube routing, Shared potion versus advanced/scroll input admission,
existing held-A deduplication, native page contracts and DLL ABI/exports. The
new fixes await live validation. Retest embedded LB+X across all categories,
ineligible items staying in place, standalone LB+X, LB+A from both Cube views
and normal Shared pages, full belt and ordinary A. Belt refusals now log the
reason, runtime ID, SDK container/page and Shared page to distinguish failed
admission from missing input dispatch.

## 2026-10-06: final ownership and Shared first-press cleanup

The user confirmed the embedded Cube/Shared potion candidate (SHA256
`3EE09CCF000FB811226A23B49928D563FBB2E5CA07140BF0A5028EECD561D95E`)
working. The 21:22 session additionally records repeated same-item belt
placements and advanced Cube source removals at 21:23:08/10. Advanced totals
and save persistence are separate observations.

One Shared potion request at 21:22:43.898 was rejected before submission; the
next request at 21:22:44.307 submitted and was observed in Belt at .340.
The log did not include both source snapshots, so it does not prove which
metadata differed. Early normalized Shared LB+A now first binds its runtime
ID/code/class to a fresh inventory enumeration while the actual controller
cell still selects that exact item on the current normal Shared page. This
UI binding expires at 750 ms; missing/changed focus or identity cancels it.
Subsequent source checks still require identical container, inventory/Shared
page and coordinates. Metadata differences during initial binding are logged.
No new native offset or relaxed submission identity check is introduced.

SDK `threads.h` documents dropped game work on session change and dropped work
on unload. Legacy MoveRequest, Cube Pending and PickupTaskArgs previously
relied on callback execution or immediate enqueue failure to free heap memory.
They now reside in bounded queues (32 entries each); userdata contains only a
monotonically increasing nonzero token. Callbacks take exclusive ownership,
while session/unload cancellation frees entries that never execute. Tokens are
not reused, including across reset; integer exhaustion refuses new work.
A predecessor token prevents a cancelled ground request from reappearing when
falling back from the game scheduler to the UI scheduler. Already executing
callbacks retain local ownership until return. No native pointers are retained
by this ownership change. New tests exercise discarded callbacks, queue-full
rejection, single take, late tokens, reset/requeue races and destruction.

All 27 suites and the Release build pass. The new first-press and lifecycle
changes await live validation. Vendor scroll refill is documented in
[BELT-NATIVE-CONTRACT.md](BELT-NATIVE-CONTRACT.md#2026-10-06-vendor-scroll-refill).

## 2026-10-07: full Ladder package coexistence capture

Read-only inspection of client PID 54372 establishes a compatibility refusal
before any stash-layout hypothesis is needed. The client loads QOL from
`mods/ReimaginedLadder/d2rloader/plugins`, SHA256
`EAA3D879BF87939F56789CD262196F4EDCA4E53E426F4EDE76A925A270E434E3`.
Its log is in that package's `d2rloader/logs`, not the root loader log.
Loader disk SHA256 remains
`93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`;
Core disk SHA256 remains
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.

Runtime observed, build-specific contracts:

- Game send thunk `0x3E2B73C` and pointer slot `0x3E2AE20` still lead to
  Core `0x822DD0`. That entry now begins `FF 25 00 00 00 00` plus an absolute
  destination pointer to `d2rl-global-chat.dll+0x538B0`. Global Chat disk hash:
  `0182AE1414544DFF7B20824F7B99EDB4236455F894F7EEC103FA3D8892E864AB`.
  QolClientRequest::Guard requires the original Core entry bytes and therefore
  rejects this runtime state. ClientTransfer checks this guard before admission
  of a request, even though initialization earlier logged an admitted profile.
- Game stat slot `0x3E2A218` now points to `d2rl-maps.dll+0x106D0`, rather
  than Core `0x831DE0`. Maps disk hash:
  `FE4DCBB368A09E85C3C618F8DA68F95156295B94C48DEACBAF04EC4C9E6CF7D1`.
  The package log at 07:30:24.130/.131 explicitly reports this slot mismatch,
  followed by refused remote identification at .133. Vendor-tome stat guards
  also require the original slot; vendor behavior has not been tested here.
- UI getter `0xCE500` matches the existing witness; table `0x2A2ADA0` reports
  Inventory=1, stash=1, Cube=0. Grid vtable `0x1CF3800` slots +C8/+D8/+100/+118
  remain `0x2C49F0` / `0x2C73D0` / `0x3E2B274` / `0x2AA7E0`.
  Controller focus chain `0x3440170` -> +D0 -> +190 had no selected widget
  during this snapshot. Thus this capture does not establish selected stash
  page identity or a Ladder layout difference.

Local evidence: `outputs/remote-id-transfer-trace/ladder-20261007/`
contains `live-contract.json` (module paths/hashes, bounded code windows,
UI/focus observations) and the copied plugin log. The read-only capture script
is `outputs/remote-id-transfer-trace/inspect_ladder_current.py`. No process
memory, package, DLL or configuration was changed; no item action was injected.

Next engineering step: inspect both hook implementations and their forwarding
contracts before admitting them. Do not bypass the hooks, blindly allow any
executable pointer, or remove guard checks. This evidence proves a present
request-guard conflict and a logged ID refusal, not the absence of additional
Ladder stash issues or successful server acceptance after a future correction.

### Same-session acceleration report and correction

The user also reports ID and Cube transfer failures, plus very slow constant
right-stick movement even in town. Cube transfers share the client request
sender guard above. The loaded package configuration and initialization log
both specify deadzone 0.22, initial speed 4, maximum speed 28, ramp 0.65 seconds.
This is not evidence of changed acceleration settings.

Source-established independent defect: UiTick uses GetTickCount64 elapsed time;
CursorMotion::Advance reset its ramp on `seconds <= 0`. Repeated clock values
across render-driven callbacks therefore erase held duration, even with a
continuously held stick. The 07:32:30 aim observations include repeated tick
values across approximately 8 ms log intervals; these observations establish
clock quantization, not a direct recording of the motion integrator state.

A regression simulating 64 positive time steps interleaved with zero-time calls
failed on the previous code. Zero elapsed time now preserves acceleration and
position after validating stick/direction; release or reversal still resets
acceleration at the same timestamp. Equal one-second travel, no zero-time
movement, release and reversal are covered. All 27 suites pass. No new RVA or
hook is involved. The new DLL is staged, not installed into the running Ladder
package, and the reported symptom still needs live confirmation. This does not
resolve the independently captured item-action compatibility guards.

## 2026-10-07: Ladder forwarding compatibility candidate

The user confirmed the preceding acceleration candidate improves the cursor.
The new item-action candidate keeps the original game/Core guards and accepts
one additional reviewed forwarding chain per conflict. It never calls a
plugin continuation directly, patches a hook, or forces the remote route when
the authoritative scheduler is available. Offline and non-Ladder operation
retain their existing request/SDK selection. Neither plugin is a dependency:
the original sender and stat-slot values return through the original guard
branches before attempting to locate a Ladder module.

Artifact identities are those in the capture section above. Profile bytes are
in `src/plugin_coexistence_profile.h`; predicates and runtime checks are in
`plugin_coexistence_policy.h` / `plugin_coexistence.h`. The exact module file
hash, current hook bytes, destination and continuation are checked when a
non-original path is encountered. Missing, changed or unknown hooks refuse.

### Global Chat sender chain

- Core sender `0x822DD0` uses a 14-byte absolute `FF 25 00 00 00 00` jump
  plus address to Global Chat `0x538B0`. Bytes +14 through the original sender
  admission window must remain exact.
- Global Chat `[0x538B0,0x5395A)` (170 bytes) is the reviewed hook. It accepts
  `(packet pointer, int32 byte count)` and returns the original sender result
  unless chat dispatch consumes the request. The hook passes both arguments
  unchanged to its continuation at `0x53928`.
- `[0x53730,0x538A8)` (376 bytes) is the dispatch filter. It returns Continue
  for null/nonpositive input or an opcode other than `0x15`; the non-chat path
  is `0x5377A` comparison -> `0x5377F` zero return. ID opcode `0x26` and the
  reviewed transfer requests therefore continue. The whole filter is guarded,
  not just its prefix. No packet is synthesized by QOL.
- Hook object RVA `0xA75C0` is passed to `[0x17CC0,0x17CCF)` (15 bytes),
  which reads its +8 continuation. Thus the continuation slot is `0xA75C8`.
  Its pointed executable trampoline must contain exactly the original first
  14 Core sender bytes followed by an absolute jump to Core `0x822DDE`.
  Those stolen instructions contain no RIP-relative operands. The original
  trampoline address is process-specific and is never hardcoded.
- Runtime candidate admission preserves this whole hook chain; it does not
  skip Global Chat or claim that an arbitrary executable detour is compatible.

### Maps stat-reader chain

- Game slot `0x3E2A218` may remain Core `0x831DE0`, or point to Maps
  `0x106D0` for the exact reviewed Maps file hash. The entire wrapper
  `[0x106D0,0x10985)` (693 bytes) must match the profile.
- The wrapper loads its original reader from Maps `0xBCD30` at `0x106E6`,
  then calls it with unchanged unit/stat/layer at `0x1070A`. The continuation
  must still be Core `0x831DE0`; original Core wrapper and stat-body guards
  remain mandatory in each caller.
- The wrapper returns the original result for non-player units at
  `0x107C0..0x107CC`. Source `player_penalties.h::Read` corroborates that
  only player penalty stats are changed. QOL reads charges/capacity on exact
  item identities, so it continues through Maps and receives item values.
- The same admission is used by native single/bulk ID, the existing offline
  stat reader, and vendor tome quantity checks. No caller bypasses Maps.
  A newer Maps source export advertises ownership, but the installed contract
  is qualified by exact bytes/hash and original continuation; no export is
  assumed present or called based on a source checkout alone.

### Evidence and remaining validation

Local `ladder-coexistence/live-contract.json` records read-only client PID
50848 observations: all 30 captured checks passed, including the four plugin
code windows, send tail/trampoline, Maps continuation, stat entry/wrapper/body,
and existing ID/transfer profiles. Both plugin hash identities also match.
No action was injected into that client. This is runtime byte/chain admission
evidence, not proof of a successful item operation.

Release build and 28 suites pass. New tests reject unverified owners, changed
Core tails, wrong hooks, altered relocated instructions, wrong/null/recursive
continuations, and unrelated stat replacements. Production guards are also
exercised with isolated original game/Core memory fixtures and neither Ladder
plugin loaded, verifying that the original branch does not require the pack.
These tests do not execute game functions or replace an offline smoke test.

Live qualification: single ID and Identify All; ordinary Personal/Shared and
Cube moves; advanced deposits/withdrawals; vendor tome refill; session changes;
and an offline/non-Ladder pass. The previously validated storage routing and
input semantics remain in place, but this capture cannot exclude additional
pack-specific differences. Current plugin files are deliberately required;
new Global Chat/Maps builds need requalification rather than broad admission.
