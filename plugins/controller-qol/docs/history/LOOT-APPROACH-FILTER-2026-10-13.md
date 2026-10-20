# Loot approach, filtered pickup and rule reordering (2026-10-13)

## Provenance

Build-specific static witnesses: private decrypted capture
`outputs/remote-ground-loot/runtime-code.exe`, SHA-256
`246dcb55785662194ee1103ef159abd2dd37dce66a7711bdc33cc519ae15fdff`.
All 17 new profile windows matched running self-hosted client PID 51756.
Loaded D2RLoader.exe SHA-256:
`93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10`;
Core SHA-256: `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.
Running QOL was rev.77-beta.1, not this candidate. Private evidence is under
`outputs/qol-approach-filter-20261013/`; do not commit process captures.
Exact byte windows are recorded in `loot_approach_profile.h`,
`filter_reorder_profile.h` and `gold_pickup_profile.h`.
Static ABI witnesses and runtime byte/layout observations do not establish
live acceptance of the new behavior.

## Native approach

Existing `game+0xFA180(uint16_t type, uint32_t id)` sends final interaction.
Native item-click branch `0x1019FE..0x101AE9` compares distance to four and
collision mask `0x804`, then submits action `0x16`, or moves to the target and
stores action `0x16` for later. Command dispatcher
`0xFABE0(action, player, type, id)` creates its own transient payload (type+0,
ID+8); switch tables begin `0xFB0E4`/`0xFB118`. `0xFB1D0` forwards player
commands to `0xFF010`, whose 31-entry RVA switch table begins `0xFF888`.

`0xFBEF0` has a bounded complete entry/exit witness for
`void __fastcall(void* player, uint32_t pendingAction, uint32_t type, uint32_t id)`.
Native run/walk getter `0x8AFD0` selects movement command 2/4 for `0xFABE0`.
If actor mode is not neutral (1/5), it stores deferred action in native player
data (`unit+0x10`): +0x1C8 pending, +0x1CC action, +0x1D0 type, +0x1D4 ID,
+0x1E0 zero. QOL calls the function; it never writes those fields or packets.
`0xF9680` is the equivalent native setter with a fifth integer argument.

Pending consumer `0xF9BC0` resolves the current target and clears prior pending
state. Item branch `0xF9E66` uses `0xFBDB0`, whose type4/5 predicate is distance
>4 via `0x325140`. It continues approach through `0xFBEF0`/`0xF9680`, or dispatches
final action at `0xFA10A..0xFA119`. Target movement `0xFB2E0` and dispatchers are
guarded. QOL keeps fresh identity, visibility and collision checks. The nearby
radius applies to acquisition; when approach is admitted, a frozen assignment
has no QOL submission distance cap. Unsupported approach retains the old radius.
Distance0..4 retains immediate pickup; >4 uses native approach if admitted.
Mismatch preserves immediate pickup. Local one-use server permits stay 1 second
for immediate pickup and use 5 seconds for approach, bound to actor/item/session.
They cause no automatic retries. Explicit button retries have a separate 250 ms
debounce (same last target/session); they renew rather than duplicate a live
completion permit. Consuming that permit does not bypass the debounce. Native
movement owns arrival and cancellation.

Rejected leads: `0x892F0`/`0x89400` are object/portal paths. `0xFDEB0` has a
unit-target branch, but inspected callers `0x12AE79`/`0x12C7D2` pass null targets.
Installed disk-PE exception boundaries differ from the runtime code capture and
must not be used as its unwind evidence.

## Filtered pickup and gold

The former filter ran only in seven server pickup-call wrappers. Joined clients
cannot apply local controller/placard state there; dedicated servers lack that
local UI session. Add an SDK inline hook at `0xFA180`, 15 complete prologue bytes,
before submission. Only ground items in controller UI mode with the option on
are filtered. Objects, keyboard/mouse, disabled settings, invalid identities and
unsupported profiles retain the original. Unknown/stale visible-label state is
treated as hidden, matching existing option policy. Original function tail and
captured installed entry bytes remain guarded; later entry changes are rejected.
Visibility callback executes outside the loot lock to preserve lock ordering.

Gold codes `gld ` and `gld\0` are exempt on client and local-server paths,
including modifier holds. Installed Reimagined misc.txt Gold row confirms `gld`.
Native gold capacity and auto-pickup settings still apply.
Server detection mirrors pickup witness `0x47197E..0x471A0F` on its native actor
and thread. `0x34B440` resolves a room, **not a Game pointer** despite the historical
GetGame label: player path `unit+0x38` -> `0x341C30` -> path+0x20.
`0x2EFDE0(room, roomsOut, countOut)` reads room+0 and +0x40;
`0x2EFD90` reads room+0xA8 first unit; `0x34B4A0` reads unit+0x160 next.
Traversal is bounded to 256 rooms/8192 units; type/ID/ground mode precede guarded
`0x36EF50` item code. No client pointer crosses threads. Unknown profiles retain
existing behavior; peer actors remain untouched by local filtering.

## Rule-list modifier and hints

Live controller focus: `LootFilterProfilePanel` vtable `0x1FD47D8`, handler
`0x14A21F0`; `RuleSummaryList` vtable `0x1D70CD0`, handler `0x8CF870`;
ListItem vtable `0x1FD4460`, handler `0x149F140`. Existing focus layout:
manager slot `0x3440170`, manager+0xD0 state, state+0x188 owner/+0x190 widget.
Existing widget fields: name+8, parent+0x30, visibility+0x50/51,
children pointer/count+0x58/+0x60.

Row handler actions12/13 (Up/Down) call `0x149E460` at `0x149F8DE`/`0x149F92E`.
Predicate ABI: bool(input-state pointer, device index0..7); input bucket stride
0x1C8, configured held-modifier key at bucket+0x68, active-key map in bucket.
SDK 15-byte entry hook overrides only return addresses `0x149F8E3`/`0x149F933`,
in the visible focused profile panel and controller mode, using fresh physical
LB. Other callers/panels, disabled navigation and unavailable physical state
pass through. Native reorder, repeats and persistence remain unchanged.

ControllerOverlay Legend vtable `0x1D76550`, entries+0x88/count+0x90, stride0x40.
Live reorder entries contain glyphs E00D and E001/E002, count2 (entry+0/+4/+8).
Dynamic text widgets use legend+0x680+index*0x230. Existing scoped text draw
replaces leading E00D with bumper E027, preserving direction/localized text.
No global binding, native text buffer, layout or Equipment/Items tab is modified.

Existing widget draw reinspection at `0x86D474..0x86D489` selects text pointer
offset +0x88 or +0x200 from flags+0x1F6/+0x1F7; `0x86D69D` loads that pointer
for the existing scoped draw call. A later text-pointer read found the process
had exited; it supplies no additional live rendering evidence.

## Validation

- All 17 new native byte windows matched the running client; both reorder legend
  entries and the profile-panel layout were observed read-only.
- Initial Release build: 33/33 CTest suites passed. Added production-handler
  cases cover neutral hidden pickup, gold padding, keyboard/mouse, disabled
  filtering, visible items, authoritative gold lookup and approach routing.
- Final working-tree and isolated rev.77-based rev.78 builds: 33/33 CTest suites
  passed in each. Rule modifier tests cover LB, neutral directions, both callers
  and unrelated caller/panel fallback. The isolated production fixture passed
  457 checks, and all 11 package-layout tests passed. The focused ZIP passed
  internal/external checksum, one-DLL, reference-config and privacy audits.
- Candidate live behavior: pending a restarted client/server with matching
  candidate packages. Running rev.77 was only read, not modified.

## Rev.78 hosted follow-up: range and repeat responsiveness

The restarted rev.78-beta.1 client admitted approach and logged submissions at
distances 5 through 9, plus immediate pickup at 3. These are submitted requests,
not authoritative pickup confirmations. The user reported that mapping a nearby
item, walking away while holding LB, then pressing its shortcut did not travel
until returning closer. Both Refresh and Submit used max(configured radius,10);
Submit silently rejected the still-assigned item outside that radius.

Follow-up removes only the submission radius when the existing approach guards
are admitted, retaining nearby acquisition and fresh visibility, ground mode,
identity, collision and session checks. There are no new native addresses or
ABI assumptions. The 5-second approach completion permit previously also blocked
repeat inputs; a separate 250 ms same-target debounce now allows explicit retries
without changing the permit's actor/item/session scope or one-use semantics.
The completion permit still expires after 5 seconds; a longer offline travel
while LB remains held needs separate arrival validation.

Validation: isolated rev.78-beta.2 Release build passed all 33 CTest suites,
including the production-handler and ABI/resource/export artifact checks.
Controller QOL production audit regressions: 464 checks passed.
Live distance/retry acceptance remains pending. Existing rev.78-beta.1 hosted
logs do not validate this follow-up.

### Restarted beta.2 observations and acquisition-radius trial

Client PID 52952 loaded rev.78-beta.2 with configured distance 6 (effective
acquisition radius 10). On 2026-10-13 at 05:51:10 and 05:51:25 local time,
GUID 39 submitted native approach at distances 29 and 68 respectively. This
confirms submission beyond acquisition range; the log does not establish final
pickup or retry timing. The user described movement as working and the nearby
mapping radius as restrictive.

For the next local Reimagined test, only the client setting
`ground_pickup_distance` was changed from 6 to the supported maximum 20, with a
byte-preserving backup and parsed comparison verifying no other setting changed.
It loads on client restart. No server setting or DLL change is required, and
this is a local trial, not a shipped default change. Wider acquisition acceptance
is pending the restarted client.

### Wider acquisition and movement-speed observation

Restarted beta.2 client PID 62240 logged configured distance 20 at 05:54:40
on 2026-10-13. Approach requests were submitted at distances 8, 21, 30 and 10;
no approach fault appears in this session excerpt. The user confirmed wider LB
mapping feels better, but sometimes approach feels like walking/light stick input.
No speed or walk/run telemetry exists in these log messages.

Static reinspection of the same private capture confirms `0xFBF0F` calls
`0x8AFD0`; `0xFBF14` shifts AX by 8, and `0xFBF18..0xFBF30` selects action 4
for a nonzero high byte, action 2 otherwise, before `0xFABE0`. This selection
uses no item-distance comparison or analog magnitude supplied by QOL. The
getter at `0x8AFD0..0x8B030` returns packed low/high flags in AX, not a simple
bool: the high byte reads native globals and conditionally calls `0x77E10`.
The low byte is independent and is not used for this movement branch.
RIP-relative slot references (instruction -> slot RVA): 0x8afdd -> 0x2a23758, 0x8aff1 -> 0x2a2375c, 0x8b007 -> 0x2a236eb, 0x8b018 -> 0x2a23760.
Exact getter bytes in this capture: `40534883ec2033c0bb01000000833d748799020066894424300fb64424310f45c3833d6487990200884424317415e80dcefeff84c075083805de8699027504885c24310fb6442430833d41879902000f45c3884424300fb74424304883c4205bc3`.
The slots and helper semantics have not been correlated with controller
input, run-toggle or stamina in a live test; do not label them as analog-speed
fields or patch them based on this evidence. No movement-speed code was changed.
A diagnosis of the intermittent walk/run choice needs observation during the
actual approach, separately from the confirmed absence of QOL distance scaling.

### Explicit LB loot run request (beta.3 candidate)

The user requested that shortcut pickup always request running after a slow
Horadric Cube approach. The 05:56 log records submission, not selected movement
mode or latency, so it cannot establish the cause of the observed slowdown.

Same-capture static witness: approach CALL `0xFBF30` has exact bytes
`E8 AB EC FF FF`, targeting `0xFABE0`. Its ignored-return x64 call ABI is
`(uint32 action, void* player, uint32 type, uint32 guid)`. Action 2 maps through
tables `0xFB118`/`0xFB0E4` to `0xFAC8E` (internal action 0); action 4 maps to
`0xFAC9F` (internal action 0x18). Internal 0x18 reaches `0xFF3D6`, uses the same
`0xFB2E0` target pathfinder, then requests mode 3 at `0xFF405` through the native
mode dispatcher. Existing full approach/dispatch/player-action guards cover
this chain. There is no position, speed, stamina or global run-toggle write.

Implementation redirects only that five-byte CALL through a pinned near relay
and SDK-owned PatchBytes; shared dispatcher and run/walk getter entries remain
original. It upgrades action 2 to action 4 only for the latest explicit LB
approach item, matching the local player identity and a current one-use
actor/item/session pickup permit. Native continuations for the same request
receive the same run preference while that permit is current (5 seconds).
Keyboard/mouse mode, other items, actors, objects, actions, expired/consumed
permits, unavailable
hooks and changed guards retain native behavior. Final interaction, lifecycle
reset or another selected item clears ownership. Player input can still cancel
native travel. Full-profile validation accounts for our own installed CALL
bytes without relaxing any other byte window.

The callback logs the native walk/run choice and the requested run action for
each owned movement dispatch. This diagnoses whether a future slow approach
was actually requested as run; native/server restrictions still apply.
Beta.3 final Release build: all 33 CTest suites passed, including DLL artifact
admission and production routing tests. Controller QOL production audit regressions: 476 checks passed.
Live run-choice/speed validation remains pending.

Additional static ABI clarification (not used by this patch): setter
`0xF9680..0xF9742` receives `(uint32 action, void* player, uint32 type,
uint32 guid, uint32 auxiliary)` in RCX/RDX/R8/R9/stack respectively; the
fifth argument is copied to player-data+0x1E0. It does not issue movement.
The caller ignores its return. No replacement implementation or setter call
was introduced.

A beta.3 read-only byte check attempted after the build began could no longer
read PID 62240 (partial-copy failure; no game process appeared
in the subsequent full process listing). This supplies no new live admission
evidence. Static capture guards and runtime startup checks remain required.

## Beta.3 did not resolve intermittent movement speed (2026-10-13)

Restarted client PID 25528 admitted the run CALL override. At 06:08:29 and
06:08:33, both pickups logged `movement requested=run native=run`, distances
23 and 8. Later attempts at 06:10:19/20 (29/28), 06:10:41/44 (47/18), and
06:10:51 (19) also selected native run. The user reported ordinary full-stick
movement normal, LB movement occasionally slow, and a recent failure to start.
This rejects the hypothesis that simply selecting native run resolves the
issue; submission messages still do not prove path progression or pickup.
The 06:17:51 attempt (GUID 38, distance 8) was specifically reported as slow,
and also logged native run. No latency conclusion follows from those logs.

A read-only external observer resolved local context via global RVA 0x2A23704
(`0x8B2D0` getter), local player IDs at 0x2A238F0 and client bucket table at
0x2A23910 (derived from `0x9A480`). Bucket index is GUID & 0x7F; observed bucket
head matched type 0 / GUID 1. The known `0x9F270` chain follows unit+0x158;
its capture entry is redirected to 0x3E2DB60, so this observation does not
claim the entire loader replacement's ABI. Observer traversal is bounded to
128 entries and checks type/ID each sample; it never executes remote code.
It samples unit mode+0xC, dynamic path pointer+0x38, established 16.16 X/Y
at path+0/+4, and the established pending fields at player data+0x1C8.
Opaque path bytes through +0x7F are retained only in private local outputs,
not assigned speculative meanings. The first 75-second trace ended at
06:17:03, before the specifically reported slow event; a longer follow-up
trace was started. Do not claim that the first trace captured the slow event.

Static same-capture follow-up: native click-to-item `0x101A87..0x101ABB` uses
`0xFABE0`, `0x8D9D0(true)`, `0xF9680(0x16,player,type,id,0)`, then `0x915A0`.
Our `0xFBEF0` helper omits both state helpers and only records pending pickup
when the post-dispatch actor mode differs from 1/5. This is a concrete route
difference, not yet a proven speed fix. `0x8D9D0` bytes
`88 0D 6F 5D 99 02 C3` write byte RVA 0x2A23745; getter `0x8AE20` reads it,
with callers `0xFE991`/`0xFEA96`. `0x915A0` bytes
`C7 05 D2 1E 99 02 01 00 00 00 C3` write dword RVA 0x2A2347C.
Input branches at `0x8A9C0`, `0x8B9A9`, and `0x8BB3D` test that dword and
skip portions of their processing when nonzero. The global's lifecycle and
controller-motion meaning are not established; no write/patch was made.

Additional rejected/unfinished static leads: `0xF9860` invokes mode preparation
`0x9B660` then `0x1060D0`; `0x9B660` includes local-player path handling via
`0xFD770`, `0x343240` (path+0x48 mask 0x20) and `0x3AC9F0` (path+0x388
auxiliary structure). `0xFC000` performs movement dispatch follow-up and
`0xED860` constructs a native target movement message with optional path data
from `0x3AC900`, then uses the existing network forwarding thunk 0x3E2B73C.
These witnesses do not establish analog-speed controls, a safe setter ABI, or
a packet modification contract. Native packets and speed fields remain untouched.
Private reproduction scripts and observations: outputs/qol-speed-diagnosis-20261013.

### Captured slow approach and expanded speed observation

The follow-up trace captured the user's final slow attempt at 06:22:24.251,
GUID 39, logged distance 15. Mode was 3 and pending fields remained
`[1,22,4,39]` throughout movement. Between 06:22:24.274 and .678, sampled
path coordinates travelled at 8.78 units/second; the preceding same-item
approach at 06:22:18.169 through .573 measured 13.86 units/second with the
same mode/pending fields. These are summed Euclidean client-position deltas
over elapsed wall time, not authoritative server speed measurements. Movement
was already visible 23 ms after the slow submission. Most of the slow segment
was consistently near 8.4 units/second, rather than only a delayed first frame.
Pending cleared and mode became 5 at 06:22:24.728. This proves the observed
slowdown can coexist with native run mode, but does not establish its cause.

Both approaches used path type +0x50 = 7. Flag +0x48 bit 0x8 alternates within
both runs and is not a stable fast/slow discriminator. Path+0x4C changed from
15 to 0 early in the slow approach, while remaining 15 in the fast approach.
Its meaning is not inferred from this correlation. Static `0x3411FB` clears
it during path construction; `0x342AE0..0x342B3A` sets it to 15 when changing
path+0xA0, then copies the requested value to +0xA0 and +0xA8, subject to a
unit/skill predicate. Getter `0x341D30` returns +0xA0 (`8B 81 A0 00 00 00 C3`).
Call `0x3516B2` supplies a value calculated from native movement/stat data;
`0x350EE1` supplies 0x1000 on a different mode branch. This is evidence for
observing the speed-related fields, not authorization for an arbitrary setter.

Additional bounded witnesses in the same capture: `0x343260..0x3432FB`
interpolates float +0x80 toward +0x84 using +0x88 and an incoming time factor,
with a 64-direction bound; `0x3433D0..0x343528` consumes correction floats
at +0x2A8/+0x2AC when +0x260 is set, adding them to coordinates and decaying
them. `0x3AC9F0` copies +0xA0/+0xA8/+0xAC into its auxiliary history entry
at +0x10/+0x14/+0x18; `0x3AD7AB..0x3AD7BD` copies those fields back to a
path. These paths have not been proved to cause the reported slowdown.
The next read-only observer expands its path read from 0x80 to 0x2B4 bytes
to retain speed, direction and correction state together. No native state
or plugin behavior is changed by this diagnosis.

Direction cross-check: `0x341520` converts a byte direction to float and
branches to `0x341530`; the latter writes desired direction +0x84 and chooses
the turn increment +0x88 from a signed 64-entry delta table, or directly writes
current direction +0x80 on its immediate branch. `0x3416C0` derives direction
from coordinates and either writes both direction fields or calls `0x341530`.
These witnesses support treating +0x80/+0x84/+0x88 as direction state, not
movement speed. They are observation candidates only; no hooks were added.

### Expanded capture and client-update candidate (beta.4)

The five-minute expanded trace expired at 06:32:35 before the 06:40:07 test;
it provides no speed evidence for that test. A subsequent 30-minute read-only
trace captured 06:41:18.903 and later tests and was explicitly stopped after
sufficient samples. At 06:41:18.940..19.445, measured speed was 9.09 units/sec,
mode 3, pending `[1,22,4,38]`, path+0xA0/+0xA8 = 1474. Both current and saved
speed (+0x28C) were already 1474 before submission and stayed there during
the approach. Direction +0x80 converged to +0x84 = 3 by 06:41:19.042, while
travel remained slow. Correction floats +0x2A8/+0x2AC were zero during this
approach. Later full-stick segments showed +0xA0/+0xA8 = 2304 and roughly
14 units/sec. Another slow approach at 06:42:11.410..11.815 had +0xA0 = 1274,
+0xA8 = 2304 and measured 7.99 units/sec. A reduced native speed can therefore
persist after turning finishes; this does not prove how it became reduced.

The user additionally reported invisible blocked ground after pickup/failure,
especially when turning from facing away; changing zones cleared it. This is
user-observed behavior, not yet a captured collision-grid delta. Do not claim
that the following candidate has repaired it until retested.

Static timing witness in the same capture: `0x94C87` calls `0x9C300`, which
iterates client units and calls `0x99B90` (e.g. call `0x9C557`). `0x99B90`
restores saved unit animation fields when unit+0x18C is set and calls
`0x343610` at `0x99C1C`. That function restores path coordinates, direction,
speed and related state from its saved fields; `0x343720` is the corresponding
save routine. After client processing (`0x94FD8 -> 0x9D5A0`), `0x95033..0x95045`
gets the current local player and calls existing pending-action consumer
`0xF9BC0`. Immediately afterward, `0x95046 -> 0x9C740` saves client units
and paths (e.g. `0x9C849 -> 0x343720`) for subsequent presentation updates.
The SDK `threads.h` documents UI callbacks as later root-widget updates;
it does not promise this native restore/process/save boundary.

Beta.4 redirects only CALL `0x95041`, exact bytes `E8 7A 4B 06 00`, through
a pinned relay with SDK-owned restoration. Ignored-return call ABI is
`void(void* localPlayer)`; the wrapper always calls the original pending-action
consumer first, then drains at most one queued explicit LB request before
the native save. Guards include `0x94C87` and the complete `0x95033..0x9504A`
local-player/call/save window, recorded in `loot_update_profile.h`.
No shared entry, authoritative scheduler, speed field, position, collision
grid or network packet is patched. UI submission queues copied identity,
epoch, time and radius only. Execution rechecks feature/held/controller state,
player identity, age (500 ms), visibility, ground mode, item identity, native
guards and collision, before issuing the existing native approach. Release,
lifecycle transitions, stale requests and incompatible guards cancel it.
Unsupported update admission disables approach and retains immediate pickup.
This is a candidate repair for dispatch timing, not proof that timing caused
all reported speed/collision symptoms. At build time, offline and hosted live
coverage remained pending; see the subsequent hosted retest below. No claim of
a speed multiplier or forced coordinates is made.

Other bounded findings/rejected leads: `0x350550` can temporarily remove and
restore unit collision, but only when requested mask intersects that unit's
mask. `0x349890` returns item mask 0x200 and obtains player mask through
`0x341860` (path+0x60, observed 0x80); neither intersects QOL's mask 0x804.
Thus its footprint-removal branches are not supported as the cause in these
observations. `0x3A9F91` restores speed from the auxiliary history entry,
similarly to `0x3AD7AB`. Native mode-change chain `0x34E5F0 -> 0x34C880`
calls movement/stat calculation `0x350B40` at `0x34CC19`; this routine is
not invoked directly by QOL. Native click preprocessing `0x101020` derives
movement actions from flag 0x40; wrapper `0x1013B0` compares current path
speed (call `0x101498 -> 0x341D30`) with native/stat-derived speed before
calling `0x102490`. These are static leads, not complete public call contracts.

Beta.4 validation: all 33 CTest suites passed, including 483 production audit
checks and the DLL ABI/resource/export/version artifact check. Before shutdown,
read-only inspection of the running beta.3 client confirmed the exact restore
CALL at `0x94C87`, 24-byte update window at `0x95033`, and 18-byte pending-action
entry at `0xF9BC0` against this capture. These checks establish admission
evidence, not live validation of the beta.4 movement change.

### User-confirmed beta.4 hosted retest

Following installation in Reimagined on 2026-10-13, the user reported:
"This seems to be working well. I don't seem to be able to reproduce the issue."
This is a positive hosted retest of the reported intermittent LB approach and
blocked-ground symptoms. It supports the dispatch-timing correction, but does
not prove a collision-grid mechanism or establish exhaustive coverage. A fresh
offline retest and successful shutdown after beta.4 remain unconfirmed. The
preceding beta.3 shutdown crash is not marked resolved by this gameplay result.

### Shutdown crash before beta.4 installation

At 06:56:59 local time on 2026-10-13, the beta.3 client exited its hosted
session, reported `Stats.cpp:1285`, `BC_ASSERT: s_StatsPool.IsEmpty()`
(leaked stats after unit deletion), caller `D2RLoader.exe+0x78EF6`, and then
crashed. The report's final exception is a read access violation, 0xC0000005,
in `nvwgf2umx.dll+0x78E389`; its recorded stack contains NVIDIA driver frames
followed by Windows thread startup. The faulting instruction reads
`[rax+0xCD48]` with RAX `0x80E28B80E28B80E2`. This is crash evidence for the
installed driver instance, not a portable RVA or a diagnosed driver defect.
The assertion occurred on a different thread from the final exception.
Neither stack attributes the cause to Controller QOL, and earlier corruption
or a connection to the reported collision symptoms cannot be excluded from
this report alone. Beta.4 had not been installed or loaded. The full report
and loader log are retained privately with the candidate's deployment evidence.
