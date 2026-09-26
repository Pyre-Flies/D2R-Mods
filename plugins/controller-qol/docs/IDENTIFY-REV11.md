# LB+A identify investigation and rev.11

## Evidence

User reproduced a visible identification FPS dip (reported 120 to 9). Local
rev.10 logs with debug_logging=false contained an unconditional game-thread
per-item dump. Five attempts ran 460, 440, 360, 377 and 396 ms from the start
message to charge-consumption message, with 39, 39, 29, 35 and 37 item lines.
Per-line timestamps were typically 8-10 ms apart. This demonstrates a substantial
synchronous hot-path cost; it is not a GPU/CPU frame profiler or proof of every
millisecond of the reported dip. The dump was NOT guarded by debug_logging.

The external report for yupgoolg132 / game 3.3.93847 says the UI-side Inventory|Cube
scan finds no usable tome/scroll, preventing the game-thread task from running.
Its disassembly accurately identifies the quantity>0 gate and unchecked scan
result. It does not include live ItemInfo quantities or enumeration status, so
stat70 mismatch and other-plugin interference remain hypotheses. Portal RVA
0x349860 refusal is independent. No unsupported native stat getter was added.

## Changes

- One authoritative SDK enumeration in the game-thread task. No consumable
  preflight in the UI interaction callback; client-side handles/copies cannot
  abort before authoritative validation.
- Source filter covers only the target container plus Inventory and Cube;
  consumables still come only from Inventory/Cube. No stash/custom-page supply
  expansion is hidden in this fix.
- Match runtime ID, code, container, page and top-left coordinates (and shared
  page when relevant). Never identify another same-code item or whichever item
  happens to occupy coordinates in another container. Fresh game-side handles
  are used; an already identified target costs no charge.
- Recognize ibk/isc with space or NUL padding, but no other arbitrary high byte.
- No per-item log calls. One bounded result summary on failure, debug, or work
  >=50ms; it distinguishes player/session, enumeration, missing supply, empty
  authoritative tome, target identity and SDK mutation failures. Includes
  scan count, tome/scroll counts, maximum positive tome quantity, queue/work ms.
- One pending identify request, expiration after 2s, with queue failure cleanup.
- Combine identify Edit and Debit in one existing-item transaction for quantity
  >1. Do not retry failed transactions through raw native flag writes.
- SDK Debit cannot reach zero. For the final tome charge use SDK Quantity=0
  (preserve book), then identify; restore quantity if identification fails.
  For a final loose scroll identify then destroy; restore unidentified state if
  destruction fails. These two boundary routes use compensating edits, NOT an
  atomic transaction. A failed compensation is explicitly reported.

## Public contracts / native inventory

No new RVA, hook, patch or raw layout dependency. SDK sources:
third_party/D2RLoader-PluginSDK/include/D2RLPlugin/{inventory.h,item.h} and README
section on existing-item transactions. ItemInfo is a copy; UI and game-thread
handles may differ. ExistingItemOperation Debit explicitly requires a positive
remainder. editItem supports Quantity and Identified fields. The old raw
editNativeItem flag fallback was removed from identification.

Do not infer that SDK ItemInfo.quantity equals book stat70 on every mod/build.
If the candidate reports tome-seen-but-no-positive-authoritative-quantity for a
visibly charged book, capture that reporter's loaded provider/build and compare
SDK copy against the native book stat through a separately guarded read-only
probe. Never treat unknown quantity as free charges or re-use another build's
unverified native address. Record new evidence before enabling any fallback.

## Validation

Fifteen suites pass. New SDK-mock workflow tests execute the real identify
algorithm: stale UI handle resolution, same-code/cell different identity rejection,
failed partial enumeration, zero charges vs no supply, padding, already-identified
race, failed atomic transaction, final charge/scroll compensation and settings.
These tests do NOT prove game publication, persistence, last-charge rendering,
mod compatibility or measured FPS improvement. Promoted for production at the user's request; that does not establish additional live validation.

Live checklist: charged inventory tome, charged Cube tome, empty tome, last tome
charge (empty book remains), loose scroll, already identified item, repeated
button press, same-code unidentified items on different pages; verify exactly one
charge, correct target, UI update, persistence and FPS. Retest reporter separately.
Keep debug_logging=false for the performance comparison; failures still have one
[QOL/Identify] summary. Short successful actions are silent by default.
