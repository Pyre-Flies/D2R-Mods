# Supported controller options review — 2026-09-24

Scope: local plugin/SDK source plus current official SDK 0.3.0 public headers. This is a public-contract review, not a live runtime service probe or review of the loader's internal hook manager. No DLLs, native hooks or production archives changed.

## Version evidence
Both local SDK copies report D2RL_SDK_VERSION 0.2.0, ABI 4. Current official master version.h reports 0.3.0, ABI 4. The web search index still showed the old README; conclusions below were checked against current raw headers. SDK update alone will not provide raw controller input interception.

Official base: https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/
Reviewed: version.h, input.h, hooks.h, item_interactions.h, panels.h, shared_events.h, diagnostics.h, core_exports.h, reimplementation_exports.h; current README https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/README.md
These are mutable master references, reviewed on the date above; recheck before implementation.

## Capability map
- input.h: InputService v1 registers named keyboard actions. Binding contains Key and Modifier; ActionEvent carries Pressed/Released. No controller button/axis state, controller binding, pre-poll filter or controller-device ownership is exposed.
- panels.h: controller layouts and registerControllerRoute support plugin panels, not rebinding stock controller actions. The documented route contract is constrained (plugin-owned staged panel, AfterPlayerInventory, PairWithPlayerInventory; one global controller route).
- item_interactions.h: Activate event can identify Controller input and provides item/container/cell. Modifiers are keyboard Shift/Control/Alt. Explicitly excludes vendor, trade, corpse, ground, equipment, cursor, belt and shared stash. Therefore not a complete LB+X/refill/loot replacement.
- shared_events.h: copied UI messages may be consumed with ordered listeners. QOL already uses these in qol_navigation.cpp. No guarantee that every controller action reaches this stage before native effects. Tooltip contributions add text; they do not expose or replace the full native stat-line buffer, so not a drop-in replacement for Item Roll Ranges inline labels/splits.
- hooks.h: InlineHookRegistration has rva, expected bytes, target and original. No module handle/name or arbitrary source-address field; no public chain negotiation/priority contract. Use for verified game-RVA hooks as QOL already does; do not synthesize an out-of-image RVA to hook XInput exports.
- diagnostics.h: ownership reporting for tracked game patches helps identify conflicts; not a general external XInput hook broker.
- core_exports.h / reimplementation_exports.h: no public controller-state/filter function found. Private ordinals are explicitly outside this public contract.

## Existing QOL requirements and sites
controller_input.cpp reads physical state, detects button edges/chords and filters wButtons and both triggers before the game sees them. Reading with XInputGetState alone cannot suppress vanilla actions. XInput entry points are resolved by export name XInputGetState and ordinal 100 (in XInput modules; unrelated to Core private ordinal 100).
qol_navigation.cpp already uses SDK inline hooks at game RVAs 0xC66A0 / 0xC6E90 (label press/release), 0x23CAA0 (shared page handler), 0x14C6810 (skills), 0x27DF80 (menu), and 0x878D30 (tab message). These are existing source constants, not newly live-verified addresses; their fingerprint/signature guards remain required. Shared UI callbacks cover panel tracking and selected messages, but native hooks remain in the working implementation.

## Recommendation
Keep the current verified behavior. Harden manual XInput ownership, instruction validation and code lifetime first rather than migrate blindly to incomplete UI events. Investigate a narrower game-side interception point managed by the SDK as a separate experiment; it still requires verified RVAs/ABI and coexistence tests. Adopt public services incrementally where they demonstrably cover the full action.

Potential upstream request (not sent): versioned controller snapshots and prioritized pre-dispatch button/trigger consumption, scoped to the game, with original-vs-filtered state, active device and automatic listener teardown. A read-only controller reader alone does not replace QOL interception.
