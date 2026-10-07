# Remote client ground-loot candidate - 2026-10-06

## Evidence and scope

The private TCP/IP run captures LB and LB+X, but both slot refresh and pickup
scheduling return SDK `Threads::Unavailable` (2). The normal item labels work;
the missing labels are the assigned A/X/Y/B shortcut prefixes. See
[LADDER-CONTROLLER-SCHEDULING.md](LADDER-CONTROLLER-SCHEDULING.md).

The candidate uses the existing filtered-tooltip observation to collect visible
IDs, resolves client units on the UI thread, and invokes the native client
interaction routine. No host game pointer, SDK handle cast, host enumeration,
direct host pickup, raw packet construction, or new hook is used. The server
retains normal validation. A returned native call means only request submission,
not server receipt, successful pickup or persistence.

Fallback occurs only for `Unavailable`, never `Busy`, `OwnerInactive`, an
accepted callback, or another scheduler error. Offline ground pickup retains its
existing path. This candidate addresses ground shortcuts only; inventory
move/identify authority restrictions are unchanged. The signed Ladder package
has not been edited.

## Build-specific native contract

Evidence is a read-only `.text` capture from the current root D2RLoader PID 14848,
module base `0x140000000`, on 2026-10-06. Private artifact:
`outputs/remote-ground-loot/runtime-code.exe`, SHA256
`246dcb55785662194ee1103ef159abd2dd37dce66a7711bdc33cc519ae15fdff`.
It is a PE-shaped disassembly artifact, not a runnable game. The interaction body
and caller witness also match the private 2026-09-27 auto-deposit capture.
RVAs below are relative to the loaded game image, not D2RCore.
Exact admission bytes live in `src/client_loot_profile.h`; these are not
wildcards or regenerated at runtime.

| RVA / offset | Contract and evidence |
| --- | --- |
| `0xFA180..0xFA924` | `void(uint16_t unitType, uint32_t runtimeId)` client interaction; complete 0x7A4-byte body guarded. Entry saves CX/type in R15D and EDX/ID in R14D, resolves the local player and client target, rejects missing target, dispatches type 4 to `0xFA223`. No useful success return is assumed. |
| `0xFF836..0xFF841` | Guarded native caller: EDX=ESI (target ID), ECX=zero-extended R14W (type), CALL `0xFA180`. |
| `0xFA405..0xFA48C` | Ordinary ground-item branch: query inventory UI mode 1, resolve item coordinates, native opcode `0x16` sender call at `0xFA482`; inventory-closed flag goes in the fifth argument. |
| `0xEC7D0..0xEC81E` | Native serializer accepts opcode plus four DWORDs and sends 17 bytes. Here fields are item ID, X, Y, inventory-closed destination flag. Not called or reimplemented by the plugin. The capture's transport CALL at `0xEC807` targets a process-local loader relay at game-base + `0x3E2B73C`; that relay address is not a portable RVA or admission signature. |
| `0xFA2B4..0xFA405` | Native controller CubeLoot (skill 370 / 0x172) branch chooses cube capacity/coordinates and opcode `0x5F`. Calling the full interaction routine preserves this selection and native inventory-full handling; the plugin does not force its own destination. |
| `0x08B2D0` / `0x09A480` | Existing local-context index and local-player resolver; guard 7 and 176 bytes respectively. |
| `0x09A5D0` | Existing client lookup `void*(uint32_t id, uint32_t type)`, called with type 4. Guard complete 33-byte wrapper. Uses table `0x2A23910` and tail-calls `0x09F270`. |
| `0x09F270` | Native lookup implementation is loader-detoured in this capture; its remaining native chain checks unit ID +8/type +0 and follows +0x158. Preserve the admitted wrapper and its normal loader-owned lookup; do not traverse the table manually or copy the detour target. |
| `0x325140..0x3251F9` | Existing `int(player*,item*)` native distance helper, full 185-byte wrapper guarded; tail calls `0x325200`, which uses native unit sizes and distance tables. Distance is not Euclidean. |
| `0x350550` | Existing `int(player*,item*,uint32_t mask)` collision helper; original 32-byte entry guard, mask `0x804`, require zero. Reads client paths/rooms, not the host's game-unit list. |
| `0x36EF50` | Existing `uint32_t(item*)` packed-code getter; original 32-byte entry guard. |
| Unit +0/+8/+0xC/+0x10 | Existing type / runtime ID / mode / ItemData pointer layout. Require type 4, matching ID, mode 3. ItemData +0 is quality. Only read during the UI callback. |
| Unit +0x38, item path +0x10/+0x14 | Native ordinary pickup branch reads static-path X/Y here. Recorded as ABI evidence only; the plugin lets the native routine read coordinates. |
| D2RCore `IsInGame` | Reuse the exported `bool __cdecl() noexcept` predicate already used by label recovery. Missing export disables the client profile. |

Admission checks the full interaction routine, caller witness, client/local
lookup wrappers and existing helper entries. Submission checks these again in
case another plugin changed them after startup. On mismatch, no client request
is made. The correction below reuses the existing normalized input hook; mouse
pickup is unchanged.

## Lifetime and label behavior

The existing placard callback supplies filtered visible IDs; blank/removed or
reused text buffers invalidate them. Entries expire after 1500 ms, matching the
existing active-placard policy, and the map is bounded at 2048 IDs. No native unit
or text-buffer pointer is retained by the client module. The remembered local
player address is compared as an identity token only, never dereferenced later.

At most one UI refresh is queued, at 100 ms intervals while the route is active.
The first refresh and subsequent client-state changes assign seven slots using
the existing rarity/quality rank and native distance. Existing identities keep
their slot; GUID breaks ties deterministically. The existing owned prefix leases
render/clear hints and preserve other mods' label text. Release, menu opening,
input-mode change, no game, or a changed local player clears client assignments.

A button edge copies the displayed item's GUID/code/quality plus a hold/session
epoch and timestamp. The callback rejects empty or stale requests (over 500 ms),
released modifier, changed session, hidden/removed/out-of-range/blocked items,
or reused identities. It does not select a replacement if the original slot
changed while queued. LB and a face button pressed before an initial mapping
exists produce no pickup; wait for the assigned hints before choosing a slot.

Submission does not clear a slot or placard optimistically and does not retry.
The next client refresh removes an item only when it no longer qualifies, such
as after a server update changes its ground mode. The game's rejection/full
inventory behavior remains observable. ClientLoot logs include PID to distinguish
client/server processes sharing the same log file.

## Validation

Release configuration builds and all 24 CTest suites pass, including DLL ABI /
manifest / export checks. New policy checks cover priority and stable assignment,
removal, changed/reused item identity, queued-request age/epoch/release rejection,
distance/collision/type/mode eligibility, deterministic ties and seven-slot limit.
Disassembly establishes the native argument and dispatch contract; it does not
establish live server acceptance or visible hint correctness.

The 2026-10-06 18:40 test loaded candidate SHA256
`36FE4114AD291ACA3D87BAEF52E81D0373B78B522069C1221E240286FC612741`.
Client PID 52536 assigned IDs 9/10 and submitted native requests for ID 10 at
18:40:48.746 and ID 9 at 18:40:49.416. Their mappings disappeared at .938 and
18:40:49.512 respectively. The user reports this appears to fix LB label pickup.
This is initial client-observed/user-reported support, not persistence validation.

Remaining live checks: on the private server, hold LB beside several visible items;
confirm A/X/Y/B hints and each assigned pickup, removal after server update,
release cleanup, another player taking an assigned item, full inventory, filtered
items and leaving/rejoining. Repeat offline ground pickup to confirm fallback
is not selected. Inventory-management shortcuts require a separate investigation.

## Competing native A correction

The later PID 23096 run of candidate
`AF78830BDFD0B1E44E80FA156CD1C18746F176016C0BC1EDB6809769805DEF61`
corroborates the user's double-pickup report: A=8 and X=9 were assigned at
19:15:42.108; the plugin logged one native request for ID 8 at 19:15:46.522,
while ID 9 disappeared first at .658 and ID 8 at .784. The original normalized
input filter deliberately forwarded A for inventory-grid interactions. Remote
ground pickup also received that A, so native highlighted pickup could compete
with the plugin's assigned-item request. The host pickup hook cannot read the
remote client's held controller modifier.

The corrected gate keeps raw A in the controller snapshot, consumes only the
game-facing A during an admitted controller ground chord, and retains that
consumption until A is released (including when LB releases first). It requires
native input installation, admitted client-loot profile, feature/controller
state, the configured ground modifier and a published world UI context. The
predicate reads atomics/input snapshots and never acquires the navigation mutex
from inside the native input gate. Profile mismatch leaves the input unchanged.

World context requires an open HUD and no tracked dedicated/submenu panel,
`PlayerInventoryExpansionLayout`, `PlayerInventoryOriginalLayout`,
`CharacterStatsPanel`, `NpcDialogPanel` or `HireMenuPanel`. These names are existing
UI message contracts; no new game RVA is introduced. The UI callback publishes
the context atomically, and HUD close clears it. The same context gates the
worker and delayed pickup callbacks so inventory LB+A does not queue world loot.
Tests cover consumption/release ordering, preserved raw A, inventory/menu
exclusion and missing session/HUD. Live simultaneous LB+A, release ordering,
inventory potion action, neutral A, mouse pickup and offline pickup remain to test.
