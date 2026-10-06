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
