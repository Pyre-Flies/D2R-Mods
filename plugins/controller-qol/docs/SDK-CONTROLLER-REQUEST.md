# D2RLoader controller SDK request and technical evidence

**Author:** PyreFly, Controller QOL Updates  
**Updated:** 2026-09-24 — production native-input release 1.3.1+rev.5  
**Purpose:** One self-contained engineering handoff for maintainer review and AI-assisted investigation.  
**Scope:** Controller input, focused UI context, navigation, prompts, native item actions and world interaction. Item Roll Ranges is not part of this feature request.

## Reading guide

Start with the requested outcome and first milestone. The technical appendix maps current workarounds to concrete entry points, signatures, observed contracts and regression cases. Addresses are evidence for locating relevant behavior, not a proposed public ABI or a demand to preserve our hook design.

**Evidence labels:**
- **Source-verified:** inspected current plugin source or official SDK headers.
- **Binary-verified:** inspected a binary/capture and recorded exact bytes or control flow.
- **User-verified:** observed in gameplay by the plugin author/tester; not an automated compatibility guarantee.
- **Proposed:** requested API semantics, names and acceptance criteria; not existing SDK functionality.
- **Historical/disabled:** retained research that is not admitted on the current build.

The proposed threading, ownership and callback design is a starting point for maintainer discussion. Please adapt it to Core architecture. No patches to the loader or SDK are included, and no upstream message or issue has been submitted automatically.

## Production result and why an SDK service still matters

**User-verified:** Controller QOL Updates **1.3.1+rev.5** works in these three configurations, each with **Stash Search and Potion Auto Pickup** enabled:

| Installation | Controller / translation environment | Result |
|---|---|---|
| Battle.net | DualShock, Steam closed | User-confirmed pass |
| Steam | Steam Controller | User-confirmed pass |
| Steam | DualShock | User-confirmed pass |

This is the author’s reported gameplay matrix, not an exhaustive test of every action, reconnect sequence, plugin version or registration order. No separate Xbox-hardware test is claimed. Eleven automated suites also pass.

The working solution now intercepts the **game’s normalized key events and input resets**, using SDK-managed inline hooks. It leaves shared Core input function-pointer slots untouched and installs no XInput hooks when the native path qualifies. XInput is only a fallback. This fixes the controller-coverage gap observed with a DualShock connected directly to the Battle.net installation without Steam translation.

The remaining burden is not writing another controller driver. It is maintaining private event/reset contracts, original-versus-filtered state, key-edge delivery, game-thread pumping, cancellation and exact-build admission. SDK hook installation makes a patch mechanically manageable; it does not make the intercepted function a supported semantic interface.

| Current cost or risk | SDK capability that would remove it |
|---|---|
| Exact Core hash can reject even an unrelated rebuild; native fallback then loses direct-device coverage | Versioned controller service with feature-level compatibility, implemented and maintained inside Core |
| Two plugins can patch/check the same native entry and conflict by load order | One loader-owned interception point with ordered subscriptions and explicit ownership |
| Suppression removes a key from game-effective state, but QOL still needs to know it is physically held | Immutable original normalized snapshot plus separately identified effective state |
| A tooltip-only reader is not a reliable world-input update source | Pre-action event stream and a frame snapshot independent of tooltip/UI rendering |
| Plugin must reconstruct presses/releases, repeats and reset behavior | Loader-owned gesture lifecycle, cancellation reasons and effective edge bookkeeping |
| A 33 ms SDK game-thread task maintains snapshots and synthetic navigation timing | Stable game-thread frame/sequence callback and finite navigation/action requests |
| A successful hook log does not prove the active device is covered | Provider/device coverage and capability diagnostics, with explicit unsupported status |

An SDK service would move game-version adaptation to the layer already owning the controller backend. It would not eliminate Core’s need to adapt after a game patch, and it should not promise arbitrary future compatibility. It would let a compatible Core update serve existing plugin binaries without every plugin carrying its own new hash/RVA profile.

## 1. Requested outcome
Move interception, game-version knowledge and hook lifetime into D2RLoader; keep button choices and gameplay/UI policy in the plugin. Prefer versioned services and cooperative events over publishing raw manager pointers or fixed RVAs. A read-only state reader alone cannot replace QOL's interception: QOL must prevent conflicting vanilla actions before they execute.

## Priority 1: controller input/action pipeline
Expose a game-normalized, immutable original snapshot and the current effective input after earlier listeners. Include connection-generation-safe device identity, active local player where available, connection/input mode, frame/sequence identity, buttons held/pressed/released, and capability-marked trigger/stick values. A digital-only trigger state must be distinguishable from a measured analog value; do not invent analog precision when only pressed/released is available. Define whether original means pre-plugin normalized input; do not promise literal hardware state across Steam Input or other translation layers.

Provide a game-thread pre-dispatch filter before bindings/actions execute. Let a listener consume a specific button/trigger action or remap a specific navigation action for the current context. An observer notification after the action cannot prevent loot-filter toggles, portal entry or item actions. The service must follow all devices recognized by the game, including direct DualShock and Steam-translated input. It must not require plugins to patch XInput or the same native dispatcher independently.

QOL examples: hold LB to enter looting mode without toggling the vanilla loot filter; LB+A requests the assigned loot item rather than portal/ordinary interaction; LB+X transfer or sell; LB+Y cube transfer; LB+R3 focused-potion belt refill. Neutral A remains available for portal priority. Context defines ownership; plugin must not globally consume LB or A in menus.

Consumption should be monotonic across priority-ordered listeners. Later listeners may inspect original input but cannot accidentally restore already-consumed actions. Original/virtual press and release edges must be paired, with held-gesture ownership defined through release, including releasing LB before the chord button. Define repeats, cancellation, context changes, focus loss and device disconnect. Prefer finite explicit action requests to arbitrary synthetic input; optional analog rewriting can be later.

### Proposed minimum controller contract (names illustrative, not existing APIs)

| Facility | Required behavior |
|---|---|
| `QueryCapabilities` | Version/size negotiation; supported snapshot fields, filter phases, cancellation and action-request features; actionable unavailable reason. SDK/ABI version alone must not imply support. |
| `ReadSnapshot` | Copied original and effective state, device generation, active player, controller/keyboard mode, sequence/timestamp and validity. Explicit staleness and reset state. Safe worker read or a documented game-thread-only contract with a copyable value. |
| `SubscribePreDispatch` | Game-thread callback after device normalization, before held-state/action side effects. Exposes original event and remaining effective input; returns a bounded consume/remap decision for supported controls/actions. |
| `SubscribeFrame` / equivalent existing service | Coherent snapshot publication while idle/held and in world/UI contexts, independent of tooltip draws. Define ordering relative to input dispatch and queued tasks. Reuse an existing suitable frame service if available. |
| `Cancel` / lifecycle events | Disconnect, focus loss, controller-mode switch, context closure and owner unload. Define release delivery, gesture ownership and pending action cancellation. |
| Registration diagnostics | Owner, priority, phase, consumed/remapped controls, active provider/capabilities and conflict reason. Tracing should be opt-in and bounded. |

**Composition rules to agree on:** original means pre-plugin game-normalized input, not guaranteed raw hardware state. Effective means input remaining after earlier listeners. Consumption is monotonic; observing original input grants no right to resurrect another owner’s consumed action. Define deterministic equal-priority ordering, conflict reporting, and whether remapping consumes the source atomically. Unknown controls pass through. Callbacks must have a documented reentrancy policy and execution budget.

**Edge ownership:** if a press reached the game before a plugin took over the gesture, the game must receive an appropriate release/cancel. If the press was consumed, it must not receive an orphan release or a delayed press when the modifier is released first. Preserve permitted native repeats. State reset must clear consumed held controls too; inspecting only the effective held set cannot recover them. Provide owner tokens/generations so reconnecting devices do not inherit an old gesture.

**Origin and actions:** distinguish physical-normalized input, loader repeat, and plugin-requested navigation/actions. Action requests should carry owner/origin and should not recursively re-enter the same filter as fresh physical input. Prefer finite requests such as next shared page or one inventory navigation step, with cancellation/completion, over exposing unrestricted fabricated device state. If synthetic input is supported, document routing and recursion explicitly.

**Lifetime:** unregister must prevent new callbacks and define draining/cancellation of in-flight callbacks and queued game tasks before plugin code can unload. Do not require each plugin to pin trampolines or retain executable pages for process lifetime. A subscription should own its queued action requests as well as its callbacks.

## Priority 2: stable controller UI context and focus
Expose panel/context flags and generation-safe focused-item, container and page handles, selected grid cell, active stash category and shared subpage. Separate item focus from world interaction target. Integrated Materials/Gems/Cube layouts require the focused child container, not just the selected top tab. State callbacks must specify whether context is captured before or after native focus changes. Avoid requiring private widget offsets or UI string parsing.

## Priority 3: navigation and prompt metadata
Allow scoped binding/remapping for main panel tabs, shared stash subpages, skills and quests. QOL uses LT/RT for main tabs, LB+LT/RT for shared subpages, LB/RB for skills/quests. Map actions rather than hardcoded button glyph strings. Prompt API should accept chord components, localized action label, visibility/enabled predicate and placement/available-width constraints. Loader resolves device glyphs and controller family, owns rendering/layout, and supports modded stash layouts. Must support a compact single modifier hint when full chord labels do not fit. Do not require hooking a global text renderer.

## Priority 4: native item action requests
SDK requests should reuse the same native validation/transactions as normal play: transfer the focused item to an explicit destination, cube transfer, vendor sale, and belt refill with explicit potion type/source policy. Inventory/personal/shared/material/gem/cube identities must be unambiguous. Preserve stash ownership, stack/special-tab rules, vendor context, confirmation requirements and multiplayer authority. Return accepted/queued/rejected plus eventual completion/failure, not a boolean implying a queued action succeeded. Revalidate handles at execution. Existing services may cover parts; extend them where possible. Button observation and safe item transactions are separate needs.

## Priority 5: world target selection and ground labels
Offer a scoped candidate-selection event for normal Interact: candidate kind/handle, distance, valid/reachable eligibility and current selection. Permit priority preference or exclusion among eligible candidates without bypassing native visibility/collision/interaction validation. Prefer portal on neutral A within configured distance; prefer assigned eligible loot under LB. If acquisition radius is configurable, apply it before candidates are discarded, with explicit bounds, rather than only rescoring the native shortlist. Ground-loot labels should expose visibility policy and a contribution slot for controller pickup hints without bypassing another loot filter or altering label buffers privately.

## Cross-cutting contract
Version/size-queryable services; feature-level capability checks; explicit thread affinity and callback reentrancy rules; generation-safe handles; copied snapshots with documented lifetime; automatic owner unregister; safe in-flight teardown; actionable conflict/owner diagnostics. Deterministic priority and equal-priority ordering, observer-only mode and narrowly scoped consumption. A plugin must not monopolize all input via a registration intended for one chord. Document whether requested game/UI actions are queued and when they complete. Fail callback/registration safely without disabling unrelated plugins.

## Suggested first milestone
One original/effective controller snapshot plus one cooperative pre-dispatch consumption callback, reset/cancellation notification, a stable frame publication point, context indicating gameplay versus UI, and a two-listener example. Analog rewriting, arbitrary synthetic input and the full item-action catalogue are not prerequisites. Prove LB looting while preserving normal UI confirm, paired press/release behavior, disconnect/context cancellation and unload. Then add focused-container/navigation/prompt support. Later item-action and target-selection services can follow independently.

## Contribution offer
We can provide focused examples, currently qualified RVAs/signatures and calling conventions, regression cases, and test the service against the working plugin. Keep version-specific implementation in Core, stable semantics in SDK, QOL policy in the plugin. Direct exports of unstable private functions would reduce duplicated hooks but not solve ABI/update or multi-plugin ownership problems.


For the complete address/ABI evidence and regression matrix, use [SDK-CONTROLLER-HANDOFF.md](SDK-CONTROLLER-HANDOFF.md), the canonical singular maintainer document.
