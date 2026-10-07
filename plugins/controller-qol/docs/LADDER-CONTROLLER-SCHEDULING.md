# Remote Ladder controller scheduling investigation (2026-10-05)

## Verified observations

The root installed Controller QOL DLL and the GitHub release-2026.10.05.2
rev.61 DLL share SHA256
`1BC004A9040FCBA4CBC4B97CC643F358B0F329E4D2E951011E31EA4E052050A2`.
The user reports LB works in the normal installation but not on the remote
Reimagined Ladder client using a Steam Controller; menu navigation still works.
The Ladder plugin log at 22:07:19 reports native input hooks admitted, while
22:07:32 reports `RAW modifier=L1 held=0 L1=0 valid=0`.
This is one snapshot, not proof that every input event is lost.

## Scheduler contract and affected paths

PluginSDK `include/D2RLPlugin/threads.h` states that `runOnGameThread` returns
Unavailable on remote TCP/IP clients without a local authoritative game thread.
`controller_input.cpp::PumpNativeInput` queued snapshot refresh and chord
processing on that scheduler; `qol_navigation.cpp::PumpLabels` queued display
recovery there too. Both are client input/UI work, now scheduled on
`runOnUiThread`. Existing native profile guards remain unchanged.
No new native addresses, byte signatures or structure layouts were discovered.

Separately, `plugin_main.cpp` queues RefreshStickySlotsTask, PickupGroundTask,
ExecuteMoveTask and ExecuteIdentifyTask on the authoritative game scheduler.
Ground slot enumeration uses captured native player/game state, and item
mutation requires the SDK's authoritative contract. Moving those tasks to UI
would violate that contract. They remain unchanged and are not claimed to
support remote Ladder sessions. A remote-compatible implementation needs
verified client transaction/request paths and server confirmation.

## Validation and next steps

Static source and SDK contract establish the scheduling mismatch. They do not
establish a conflicting plugin or prove the UI scheduler fixes all symptoms.
Live validation must check LB snapshot validity and labels on a remote session,
then separately exercise each item action. The signed Ladder package must remain
intact; any changed DLL needs inclusion through its accepted package process.
The earlier narrowed pickup-gate test is not the DLL the user tested.

The user subsequently confirmed the UI-scheduler test DLL works offline.
This supports offline regression validation, not remote Ladder item support.

## Private-server follow-up (2026-10-06)

Root DLL SHA256 BC2CCB4EBBCBF49A16065A346515DA232018807ECB28140673F5BB5C09C90442
is the tested first-cast snap correction with a rev.62 version string.
The 18:04-18:10 run admits native input and labels, joins TCP/IP at 18:09:13,
and the user confirms right-stick snapping works while LB/labels remain broken.
QOL debug_logging was false; no LB-state evidence was recorded. This reproduces
the symptom outside the signed Ladder bundle; a package conflict is unproven.

Add five-second RemoteTrace records gated by QOL debug_logging for native
provider validity, cached age/buttons, controller UI and modifier state, and label
UI-callback execution/in-game/filter-state/ensure result. Input UI scheduling
failures are rate-limited warnings. These diagnose capture versus UI work versus
authoritative item-action limits; they do not change runtime targeting or actions.
No new native RVAs, layouts, or contracts are introduced.


### Private-server diagnostic result (2026-10-06 18:20)

Diagnostic DLL DAC7697E879F0B4BDCA1C5056D6E04A517E1F19B0A343FAD55F9E70E9611B46D.
Joined TCP/IP at 18:20:06.978. At 18:20:19.103 LB is captured as modifier held,
provider 0x100/buttons 0x0100; the native label press is consumed. At .117 ground
slot refresh scheduling returns 2 (Threads::Result::Unavailable). At 18:20:18.443
and 18:20:23.460 label callbacks execute with inGame=1/controllerUi=1,
filteredState=1/ensured=1/recoveryReady=1. LB release is captured as well.
This proves successful client input capture and the UI label enable path; it does
not prove labels are rendered visibly. Slot shortcut overlays depend on the
rejected authoritative slot refresh and cannot be claimed working remotely.

The log also has regular controllerUi=0 snapshots interleaved with controllerUi=1
snapshots. Host and client share the root log location; do not attribute these
to client state loss without process-specific attribution. A rejected refresh
requires a verified client-side item enumeration/request path, not UI scheduling
of the server-side native player/game traversal. If plain item names remain
invisible, investigate label render recovery separately from shortcut overlays.

### Client ground-loot implementation (2026-10-06, candidate)

The user clarified that plain names display, while the assigned A/X/Y/B hints
and actions are missing. Add an `Unavailable`-only client fallback for those
ground shortcuts. This does not move the host traversal/mutation callbacks onto
UI. The independently guarded client path and new native interaction contract
are recorded in [REMOTE-GROUND-LOOT.md](REMOTE-GROUND-LOOT.md). Offline requests
still use the original game scheduler. Inventory move/identify still have their
existing authority requirements and are not addressed by this candidate.
