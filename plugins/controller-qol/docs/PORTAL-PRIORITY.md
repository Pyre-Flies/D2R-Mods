# Neutral interaction: portal priority (1.5.19)

> Rev.43 extends this same guarded pipeline to the exact town Bank class and
> waypoint SubClass. See [STASH-WAYPOINT-PRIORITY-REV43.md](STASH-WAYPOINT-PRIORITY-REV43.md).

## Behavior and scope

An eligible portal within `portal_priority_distance` (default 10 native game units, inclusive; clamped to 1–20) wins an Interact comparison against a ground item. `prioritize_portals = true` is the default; false disables installation of this hook. Enabled/ground_pickup must also be true. Ground-loot modifier gestures retain their original targeting. Dedicated Loot (358), CubeLoot (370), combat skills, NPC comparisons and other object comparisons are unchanged.

This is a change to native candidate ranking, not a new portal-entry implementation. It consumes the game's current primary/secondary candidates. It does not force a portal that the game excludes from its candidate lists, bypass entry permissions or choose portals through walls. Other objects can still compete within the game's object category. No additional world enumeration, polling worker, retained unit pointers, synthetic button event, direct server call, or packet construction is added.

## How the hook was located

1. Existing `GroundLoot::HookPickup` intercepts server-side item pickup at RVA `0x471950`. That is too late to change the client's chosen interaction target. Do not redirect it into a guessed portal packet.
2. The local captured `.rdata` contains `D2Client\src\Game\TargetingController.cpp` at `0x1CD2010`. String cross-references locate the native targeting code around `0x18A650..0x192410`.
3. Native profiler descriptors identify functions precisely: descriptor `0x1CD2728` references GetSelectedTargetForSkill; `0x1CD27F0` references GetInteractionTarget; `0x1CD2890` references GetLootTarget; `0x1CD29B0` references UnitTestSelect. The descriptor stores string pointers; search references to the descriptor, not just references directly to the function-name string.
4. GetInteractionTarget calls GetSelectedTargetForSkill with `EDX=0x165` at `0x18D9FD..0x18DA08`. The active mod's skills.txt identifies 357 as Interact, 358 as Loot, and 370 as CubeLoot.
5. GetSelectedTargetForSkill constructs a stack closure at `0x18DEEE..0x18DF20` and passes it to the same candidate comparison at `0x18E0DB` and `0x18E2C9`. These are primary and secondary candidate paths. The candidate is RDX, the float score is XMM2, the native target-category index is R9D; RCX is the closure. The callback return value is not consumed.
6. The comparison already contains asymmetric portal preference: at `0x18A713..0x18A75E`, an item candidate cannot displace a current object whose ObjectsTxt SubClass has bit 4. But a later portal candidate still has to beat the current item's ordinary score. The new wrapper fixes that ordering case while retaining the original callback's remaining checks.

## ABI and addresses (all D2R.exe-relative RVAs)

| RVA / offset | Meaning and evidence |
|---|---|
| `0x18A650` | Hooked comparison: `void(Selection*, Unit* candidate, float score, int targetCategory)`. Entire reviewed body through `0x18A9A3` is checked before installation; the loader receives the first 16 reviewed bytes for its inline hook. |
| `Selection+0x00` | Pointer to best score (float); native comparison at `0x18A680`, write at `0x18A97E`. |
| `Selection+0x08` | Pointer to skill ID; comparison to 357 at `0x18A68E`. |
| `Selection+0x10` | Pointer to player-unit local. Closure construction at `0x18DF03..0x18DF07`. |
| `Selection+0x18` | Pointer to selected-unit local; native write at `0x18A978`. |
| `Selection+0x20/+0x28` | Native targeting-controller pointer / pointer to skill-settings pointer. Wrapper leaves both untouched. |
| `0x349860` | `uint32(Unit*, const char* sourceFile, uint32 sourceLine)` class getter. Native portal check passes an assertion source and line. |
| `0x34A0E0` | `uint8(Unit*)` game-data version getter. |
| `0x38FD00` | ObjectsTxt lookup `(uint8 gameVersion, uint32 classId) -> record`. |
| `ObjectsTxt+0x127` | SubClass byte, bit `0x04` is portal. Native test at `0x18A753` and active objects.txt agree. IDs 59/60 are Portal, subclass 4, OperateFn 15; IDs are evidence only, not hardcoded detection. |
| `0x325140` | `int(Unit*, Unit*)` distance, shared with existing ground-loot code. Negative results do not qualify. |
| `Unit+0x00` | Native type: 4 item, 2 object. Native getter/caller observations agree; wrapper reads it only from live callback arguments. |
| `0x18DDE0` | GetSelectedTargetForSkill, not hooked. Primary candidates at controller `+0xC20` (12-byte entries); secondary candidates at `+0xCB0`. |
| `0x18D960` | GetInteractionTarget, not hooked. |
| `0x18DB60` | GetLootTarget, not hooked. |
| `0x1919F0` | UnitTestSelect, observed native selection/score updater; do not call it as a pure validator because it updates selection state. |
| `0x18E390` | GetUnitTargetType, observed only. |
| `0x18B350` | Native target scoring, observed only; deliberately not globally boosted because it is also used by skill targeting. |
| `0x19057A` | GetSelectedTargetForSkill call in controller skill targeting. |
| `0x151964B/0x1519664` | Ground-label target display asks for Loot then falls back to Interact. UI highlight can have a distinct path from actual Interact selection. Neither call is patched. |

## Implementation and failure behavior

The wrapper first checks the enabled state, Interact ID, candidate object and released ground-loot modifier. A current item is required only for the ranking boost, not for the range exception. It uses the same native class/version/table helpers as the original comparison to identify portal SubClass. Only distances 0 through the configured radius qualify.

If the qualifying portal's score is no greater than the item's best score, the wrapper submits the next representable finite float above the best score to the **original callback**. The original comparison still decides acceptance; its distance-only predicate receives the scoped exception described below, while other native checks remain in force. If accepted, the wrapper restores the portal's original score so later NPC/object comparisons keep normal scoring. Subsequent items cannot displace it because the original callback already protects selected portals. The original callback is called exactly once, including fallback cases. Native rejection leaves the item selected. Nonfinite scores do not receive an artificial boost.

The native hook is installed and owned by `PluginContext::InstallInlineHook`; no custom trampoline is allocated. `CheckExpectedBytes` admits all twenty recorded sites before installation. Any mismatch leaves vanilla targeting in place and logs one warning. Shutdown deactivates the wrapper; loader lifecycle owns hook cleanup. Optional trace messages are throttled to one per second and record selection changes, not successful portal entry.

## Patch recovery / repeatable validation

- Reviewed decrypted capture: `<private-workspace>/work/runtime-game.exe`. SHA256 is recorded in `portal-evidence.json`. The decrypted artifact is local evidence, not shipped in release ZIPs.
- Exact bytes, RVAs and sizes: `portal-evidence.json`; inspected disassembly: `portal-native-disassembly.txt`; compiled admission data: `src/portal_signatures.h`.
- Run `python tools/audit_portals.py --image <new-decrypted-PE>` to compare every admitted site. This reads PE section mappings; it does not regenerate signatures.
- After a mismatch, re-find TargetingController.cpp and the profiler descriptors, re-prove the Interact ID, both callback ABIs, closure layout, portal SubClass offset, score comparison, post-selection writes and helper contracts. Only then update evidence and admission bytes. Do not substitute current live bytes as expected bytes.
- Eight CTest suites passed, including the added range boundary, modifier/skill restrictions, tie, native rejection, one-call forwarding and score-restoration checks. This is static/automated validation; no in-game test is claimed.

## Runtime acceptance checklist

1. Confirm the loader reports 1.5.17 and `[QOL/Portal] Neutral Interact portal priority and distance gate enabled within 20 game units` with the installed test configuration (release default is 10). Profile mismatch means this feature did not install.
2. Place a usable portal on visible loot. With LB released, press A: portal should win in both candidate ordering cases. Repeat near the ten-unit boundary.
3. Away from portals / beyond range, verify ordinary A pickup still works.
4. Hold LB+A beside that same portal: assigned item pickup should still work. Check the other LB loot buttons too.
5. Test a disabled/closing portal, an obstructed portal, two nearby portals and a nearby NPC/chest; native eligibility and non-item comparisons remain relevant.
6. With debug logging enabled temporarily, comparison logs can distinguish a hook/selection problem from the native entry path rejecting the selected portal. Restore false afterward.

The module changes native selection, so the first in-game check must establish whether this installed controller binding uses Interact in the reported case. A dedicated Loot-bound button is intentionally not redirected to portal operation.

Active data evidence hashes:

- objects.txt: `45851636360723F5E1B3DE98207625748AF23546D40918C1BC8254A263F2747B`
- skills.txt: `818011AD76D0800FFDC6E261C175802607B6979E4A4AB274B93826D6B677498C`

## 1.5.16 configuration change

Portal priority now uses `portal_priority_distance = 10`, independent of loot pickup range. Missing or malformed settings default to 10; integer values clamp to 1–20. Existing `ground_pickup_distance` is unchanged. No new RVAs, hooks, ABI changes or portal-entry distance overrides were introduced. Native eligibility still applies. The user confirmed the original six-unit priority behavior before this range change. Added parser and ten-unit boundary checks pass in the portal suite.

## 1.5.17: native distance gate fix

### Observation and conclusion

The installed TOML had `portal_priority_distance = 20`, `ground_pickup_distance = 6`, and `debug_logging = false`. The September 23 23:10:28 startup log explicitly reported a portal radius of 20. Configuration parsing was therefore working. Version 1.5.16 only widened `ShouldPrefer`; the original comparison could subsequently reject the portal using its independent native interaction range. The prior policy tests modeled native rejection, but did not model an expanded radius crossing that gate.

Following the original comparison's object branch identifies this chain:

| RVA / offset | Reviewed contract / finding |
|---|---|
| `0x18A7BF..0x18A7C9` | Closure +0x28 supplies the skill-settings pointer; usageType 4 activates the subsequent interaction checks. |
| `0x18A94C..0x18A970` | Read native allowance from controller +0x1910, obtain local player, call the distance predicate; false skips the selected-unit write. |
| `0x18AED0..0x18AF2E` | **New loader-owned hook**, `bool(Unit* player, Unit* candidate, int allowance)`. All 95 bytes are checked; hook entry admits the first 16 bytes at complete instruction boundaries. |
| `0x18AEED` | Calls the existing distance helper `0x325140`. |
| `0x18AF0A` | Calls `0x348650(player, candidate, 0, &reachAdjustment)`, initialized output zero. This function's full contract is not inferred; its output is used as a reach adjustment here. No direct call added by the plugin. |
| `0x18AF18..0x18AF26` | Returns `distance < allowance + reachAdjustment` (strict comparison). It contains no visibility, portal operation, or packet handling. |
| `0x325140 -> 0x325200` | Distance uses unit positions and sizes. The latter clamps size-adjusted axis differences and returns `2*max(dx,dy)+min(dx,dy)`; these native units are not Euclidean world-coordinate units. |

### Implementation and isolation

`Qualify` admits only Interact 357, released loot modifier, a live object with portal subclass bit 4, and native distance in `[0, portal_priority_distance]`. It creates a thread-local `RangeOverride` **only for the duration of that original comparison**. The new distance wrapper calls its original exactly once, then returns `nativeAllowed || matchingQualifiedOverride`. Player and candidate identities must both match. Outside that scope, after shutdown, for a nested ordinary comparison, and for all other targets/callers, the original result passes through unchanged. A stack scope restores prior state, including nested calls; there are no persistent unit references or mutations to the game's shared controller state.

This scope also applies when the portal is the first candidate, or the current candidate is another object. Score boosting remains restricted to portal-versus-item comparisons. Other object ranking continues to use the original score. Both hooks must install successfully before activation; a partial install remains inactive and forwards native behavior. D2RLoader owns both trampolines and cleanup.

With debug logging enabled, a throttled message reports `Interact distance gate extended: distance=... configured=... nativeAllowance=...` only when the new scope rescues a native range rejection. Debug remains **off** by default and in the installed configuration. The startup line distinguishes the new distance-gate build from 1.5.16 without verbose logging.

### Earlier candidate scoring: reviewed, not altered in 1.5.17 in 1.5.17

`0x18B350` scores `(controller, player, candidate, classProfile)` and calls `0x18AF30` at `0x18B47C`. The latter reads generic profile `controller+0xD40` for classProfile 8, otherwise `controller+0xE60+classProfile*0x120`; maximum distance is `profile+0x60+targetCategory*4`. It rejects Euclidean center distance / profile maximum outside `[0,1]`, and then applies angle and score curves. UnitTestSelect calls it at `0x191B99` and `0x191ECA` before populating native candidate lists. These profiles serve other skill targeting too: this fix deliberately does not globally widen their ranges or rescore objects. Disassembly is appended for future diagnosis; these functions are not newly admitted or called by this release.

The corrected setting controls portal preference and the **final Interact distance gate**, not unrestricted world acquisition or remote portal entry. A portal must still enter native candidate lists and pass the remaining native comparison checks. If runtime testing still shows a limit, establish whether it is missing from those lists before changing scoring or enumeration. Do not equate the scoring function's Euclidean distance with `0x325140` units. No successful extended-range in-game result is claimed yet.

### Regression and patch recovery

Tests now model an original six-unit range rejection while exercising configured radii 6/10/20 at distances 6/7/10/11/20/21. They verify inclusive configured boundaries, one original comparison, cleanup, nested masking/restoration, invalid distances, distinct units, and shutdown. Existing native-rejection and score-restoration tests remain. The simulated six-unit gate reproduces the reported symptom; it is not a claim that the native formula always equals six for every object.

After a game patch, re-find the original comparison, follow its object branch to the distance call, and prove that the callee is still only a distance predicate before porting this exception. Update its entire-body signature and the existing comparison signature together. Run `tools/audit_portals.py` against a reviewed decrypted PE; never automatically adopt changed bytes. The captured PE hash and exact admission bytes remain in `portal-evidence.json`.

Runtime check: restart the game, confirm version 1.5.17 and configured radius 20 in the log, then compare neutral A with a visible usable portal between the old range and the new radius. Verify LB pickup remains normal and out-of-radius ordinary looting still works. This feature changes target selection; actual travel/entry remains native.

## 1.5.18: earlier candidate scoring and targeted diagnostics

### Runtime evidence and remaining uncertainty

User testing of 1.5.17 still selected loot at greater distances. The log at 23:28:27.882 confirmed the new distance-gate hook was installed with radius 20. The user clarified that A **still picks up items**, rather than selecting a portal and failing to enter. No verbose trace was enabled for that run. This rules out a stale DLL/config and points to acquisition/ranking, but does not prove the earlier score cutoff is the only remaining gate. This release addresses the independently proven earlier cutoff and adds stage diagnostics; runtime success remains unconfirmed.

### Native reuse and exact contracts

| RVA / offset | Use in 1.5.18 |
|---|---|
| `0x18B350..0x18B48E` | New D2RLoader inline hook, `float(controller, player, candidate, int profile)`; XMM0 return. First 16 bytes end on an instruction boundary. Always call original unit scorer once. |
| `0x18AF30..0x18B344` | Native point scorer, `float(controllerView, player, const float xy[2], int category, int profile)`. Called directly only for a qualifying failed portal distance score. Fifth argument is profile; return XMM0. Full body admission-checked. |
| `0x144640..0x1446BA` | Native controller/aim-state singleton getter, no arguments. This is the same getter called at 0x18AF73. |
| `0x1446C0..0x1448B2` | `void(aimState, player, float position[2], float facing[2])`; same helper called at 0x18AF86. Uses dynamic-path fractional positions for players and native aim/facing calculation. Full body admission-checked. |
| `Unit+0x38`, static path `+0x10/+0x14` | Object X/Y uint32 coordinates; copied exactly as the original unit scorer does at 0x18B407..0x18B450. |
| `0x18E54F..0x18E584` | Classification switch: UnitType 2 returns category 2 at 0x18E575. Guarded evidence justifies category 2 for the direct point-scoring retry. |
| profile `+0x68` | Category-2 max center-distance float: +0x60 + 2*4. Generic profile begins controller+0xD40; class profiles 0..7 begin +0xE60 + class*0x120. |

The point scorer's entire reviewed body reads its first argument **only through the chosen 0x120-byte profile**. It obtains player position/facing from the separate singleton; it neither writes to nor calls virtual methods on the supplied controller pointer. A zero-initialized 0x1760-byte stack scratch view contains just the selected profile at its native offset. Every profile byte is copied unchanged except category-2 max distance. This is not a real controller object and must never be passed to other native functions. Re-prove this restriction after a patch.

### Retry conditions and behavior

The unit-score wrapper forwards immediately unless enabled, the loot modifier is released, and the candidate is a native portal (ObjectsTxt SubClass bit 4). It measures distance using 0x325140. Only a negative finite native score **within the configured native radius**, with a valid profile and a positive finite original center-distance limit, can be retried. Its measured center distance must actually exceed that limit; a failure within the limit (such as angle rejection) is left alone.

For the retry, only the scratch profile's distance maximum becomes `centerDistance + 1.0f`. The extra coordinate unit keeps the portal inside the scoring boundary instead of exactly on a zero-score edge; it does **not** extend the configured native-unit radius, which has already been checked. The original point scorer runs with the real portal coordinates and native angle curves, score curves and weights. A negative or nonfinite retry result is discarded. A valid original score is never changed. This retains the native scoring implementation without rewriting its math, fabricating unit positions, scanning the world, persisting unit pointers, or modifying shared controller settings.

Candidate generation is shared by skill targeting; a successfully rescored portal can therefore enter the native object candidate slot before the skill-specific comparison. Only portal candidates are rescored, never items/NPCs/monsters. The existing Interact-only preference and range scope remain. This is a deliberate acquisition change; do not claim that the shared candidate list is byte-for-byte identical for other skill consumers. Native visibility/selectability checks before UnitTestSelect scoring and skill filters afterward remain in place. Nearby chests/other portals can still compete for the object candidate slot.

### Diagnostics and validation

New `portal_diagnostics = false` defaults off independently of `debug_logging`. For this test installation it is set to **true**, while general debug stays false and portal radius remains 20. Set it false after confirming behavior. Restart after editing either setting.

- `Candidate score`: native distance, configured radius, profile, original/result scores, whether the retry ran, and old/new center limit (zero fields can mean no retry preparation was needed).
- `Interact distance gate extended`: final native short-range rejection was overridden for the qualified portal.
- `Interact comparison`: configured/native distance, whether the original comparison selected the portal, and whether its score required a boost.

Candidate messages throttle to one per two seconds; comparison and gate messages to one per second each. No world scan or extra worker is added. If a portal is visible but there are no candidate messages, investigate the earlier enumeration/selectability path. A negative retry with an extended limit implicates other scoring conditions, including angle. A valid score without Interact acceptance implicates candidate competition or later filters. Acceptance with continued item pickup implicates the action path after comparison. A logged native distance above 20 means it is outside this setting even if the screen-space distance appears small: native distance is **not** Euclidean world distance.

Tests cover scratch profile bounds for generic/class profiles, unchanged original memory, preservation of every unrelated profile field, earlier-distance rejection becoming eligible, retained angular rejection, and invalid profiles/positions. Existing range, modifier, native-rejection and nested-scope tests remain. Full DLL build, eight CTest suites, and all fifteen signature checks must pass before deployment. Tests model the native gates; only a game session can confirm the actual path used here.

Patch recovery: re-find TargetingController profiler descriptors, validate unit/point scorer ABIs and the complete scratch-view read contract, class-profile offsets, object-category mapping, static-path coordinates, and aim helpers before updating signatures. Exact bytes and decrypted image hash are in portal-evidence.json; appended disassembly records the new helpers, GetInteractionTarget and candidate population branches.

## 1.5.19: object-contact gate between scoring and candidate insertion

### Evidence from the failed 1.5.18 playtest

The portal-only log (saved in `portal-1.5.18-runtime-evidence.txt`) confirms radius 20 and successful rescoring. At 23:43:12.311 a portal at native distance 6 changes score from -1 to 0.292; the native center limit is 5.20 and the retry limit is 7.32. Later samples also show valid scores at native distances 4/5/6. The sampled Interact comparisons only show distances 0/2. Thus rescoring executes, but the newly eligible farther portal is not observed at the later comparison. The user reports continued item pickup. Sampling alone cannot rule out multiple portals or every other rejection; the next control-flow inspection identifies an additional unconditional object-contact gate matching this symptom.

### Newly reviewed native call chain

After successful scoring, UnitTestSelect reaches `0x192236`. Object/category 2 runs its normal object-mode/collision handling and then calls `0x34BC90(player, object)` at `0x1922CD`. A zero result skips the primary candidate insertion at `0x1922DA..0x192357`. The same contact helper is called at `0x192378` before the function's final success return. The candidate refresh path also calls it at `0x191589` and skips its update on zero. These are the only direct relative calls to 0x34BC90 found in the reviewed targeting span 0x18A000..0x192410.

| RVA / offset | Contract / discovery |
|---|---|
| `0x34BC90..0x34BDD3` | New loader-owned inline hook, `int(Unit* player, Unit* object)`. Enforces UnitType 2, succeeds immediately at native distance zero, otherwise extracts object dimensions and position and calls the rectangle helper. Full 0x144-byte body checked; 16-byte hook prefix ends on an instruction boundary. |
| `0x34BDE0..0x34BF58` | `int(player, objectX, objectY, sizeX, sizeY)` geometry predicate. Full 0x179-byte body checked. Computes object rectangle from half sizes; checks player position against bounds expanded by one/two coordinate units, with corner handling dependent on player size. No portal permissions, collision traversal, object operation, or packet transmission here. |
| `0x191589` / return `0x19158E` | Candidate refresh/update object contact check. Call-site bytes from 0x19157E through 0x191595 checked. |
| `0x1922CD` / return `0x1922D2` | UnitTestSelect primary candidate insertion check. Bytes 0x1922C7..0x1922D9 checked. |
| `0x192378` / return `0x19237D` | UnitTestSelect final object success check. Bytes 0x19236B..0x192384 checked. |

### Narrow implementation

`CandidateContact` records `_ReturnAddress()` at its own non-inlined hook entry and always calls the native contact function exactly once. It can replace zero with one **only** for the three exact return RVAs above, active portal priority, released ground-loot modifier, a portal identified by the existing native ObjectsTxt lookup, and measured native distance within the configured inclusive radius. Nonzero native results pass through unchanged. All unlisted callers retain their original result, including gameplay/server/object-operation callers: this is not a global interaction reach patch. No instruction-address interval is used as an allowlist.

Object-mode/collision checks surrounding the first contact call still execute. The existing point-score retry, native angle checks, Interact-only score preference and final scoped Interact distance fix remain. The shared candidate-list implications documented for 1.5.18 still apply. D2RLoader owns all four inline hooks; all twenty sites are checked before installation and activation happens only after all hooks succeed.

A new throttled `Candidate contact` diagnostic reports the native caller RVA, measured/configured distance, original contact result and whether the configured exception qualifies. This should connect successful rescoring to subsequent Interact acceptance in the next runtime log. General debug remains false; portal-only diagnostics remain enabled for the test.

### Verification and future patches

Regression tests reproduce the logged intermediate condition: a rescued portal score (0.292) at distance 6 must survive candidate contact before it can beat an item score of 0.9. Tests cover all three callers, exact caller matching, configured boundaries 20/21, invalid distance, nonportal targets, modified pickup and disabled state. Existing eight suites and all twenty signature comparisons pass before release. Native in-game success still requires the next playtest; no runtime outcome is asserted from unit tests alone.

After a patch, re-find the candidate insertion writes and follow every object-specific branch between the scorer and those writes. Re-prove that 0x34BC90's successor is purely geometric before porting this exception. Re-find the three CALL instructions and derive their return RVAs from instruction ends; checking only the callee would be insufficient because the caller allowlist is part of the safety boundary. If any call moves, the full guarded call-site bytes must be re-reviewed. Exact bytes are in portal-evidence.json, and both helper bodies plus caller disassembly are appended to portal-native-disassembly.txt.

## 2026-09-26: rev.7 contact CALL migration

Supersedes the historical shared-entry contact hook: QOL now patches calls at 0x191589, 0x1922CD and 0x192378; entry 0x34BC90 remains untouched. Other portal hooks are unchanged. See [evidence, ownership, admission and pending live checks](PORTAL-CONTACT-1.3.1-rev.7.md).
