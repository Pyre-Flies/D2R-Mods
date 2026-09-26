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

# Technical appendix

## A. Exact build identity and provenance

| Component | Identity | Evidence |
|---|---|---|
| D2RLoader.exe | File/ProductVersion1.3.1-beta | Read from installed PE version resource on preparation date |
| D2RCore.dll | File/ProductVersion1.3.1-beta | Read from installed PE version resource |
| Controller QOL Updates |1.3.1+rev.5| Production native-input build; user-verified controller/plugin matrix above |
| Public SDK reviewed |0.3.0, plugin ABI4| Official master headers reviewed during this investigation |
| SDK vendored in QOL build |0.2.0, plugin ABI4| Local version.h; newer services require an explicit header update and runtime availability checks |
| Native game profile | Prior migration notes identify3.3.93787 | Historical qualification context; not a new measurement of a stock game executable in this handoff |

Qualified installed-file SHA-256 values, verified during the 2026-09-24 investigation in both Battle.net and Steam installations:

```text
D2RLoader.exe  93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10
D2RCore.dll  2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2
Controller QOL Updates.dll  E21E894B0B32BC37B9BB3F495737B7F032260FDF8B59741F3FAFD43B3CD3ECC0
```

**Address rules:** Game RVAs below are relative to the mapped D2RLoader game executable; Core RVAs are relative to D2RCore.dll. VA=module base+RVA. Do not reuse ASLR virtual addresses. A disk-file hash identifies the provider, but does not imply runtime bytes are unmodified: Core and plugins patch the image during initialization. Admission checks must examine the actual running code as appropriate.

Native signatures below come from source profiles used by the working plugin. They are localization witnesses, not sufficient standalone proof of a whole function's semantics. Some profiles originated before the current loader update and remained applicable; they are not presented as newly captured this turn. ABI descriptions use our current wrappers and observations, not original game symbols.

## B. Public SDK coverage and remaining gap

| Existing public contract reviewed | Useful coverage | What it does not replace |
|---|---|---|
|InputService/input.h|Named keyboard actions, bindings and press/release callbacks|No public controller snapshot or pre-dispatch button/trigger filter found|
|PanelService/panels.h|Controller layouts and constrained routes for plugin panels|General remapping of stock controller navigation|
|ItemInteractionService/item_interactions.h|Controller-origin Activate on supported item grids|Controller chord state; vendor, shared-stash, belt and ground paths are explicitly excluded in the reviewed V1 contract|
|SharedEventService/shared_events.h|Ordered UI message consumption and tooltip contributions|Guaranteed interception of every native controller action before effects; arbitrary native tooltip text replacement|
|SDK inline hooks/hooks.h|Expected-byte-checked game-RVA hooks|A cooperative controller event pipeline or documented arbitrary external-module XInput hook target|
|DiagnosticsService|Tracked executable-patch ownership information|General arbitration of every external input hook|

Official source references (mutable master; reviewed2026-09-24):
- https://github.com/D2RLoader/PluginSDK
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/input.h
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/panels.h
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/item_interactions.h
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/shared_events.h
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/hooks.h
- https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/diagnostics.h

This records the earlier SDK 0.3.0 public-header review; this native-input addendum does not claim a new upstream revision audit. This is a public-contract review, not a review of the maintainer's internal implementation or a request to duplicate a service already available elsewhere.

## C. Current native controller interception and recovery evidence

**Binary-verified and source-verified:** these are the qualified entry points for the working 1.3.1+rev.5 build. Game RVAs use the mapped game base; Core RVAs use D2RCore.dll. The names below describe observed contracts, not original symbols or proposed public ABI.

| Module / RVA | Observed contract and relevance |
|---|---|
| Game `0x13B350` | OS-button message adapter. Reads device identity DWORD +0xC, normalized key DWORD +0x10, pressed byte +0x14. Checks active controller, passes key through `0x13D080`, calls event dispatcher at `0x13B3A2`. |
| Game `0x13EDD0` | **New SDK-managed inline hook:** `void __fastcall(unsigned key, bool pressed)`. Original function updates held state and dispatches native actions/UI. This is early enough to prevent consumed buttons from executing vanilla effects. |
| Game `0x13DEF0` | **New SDK-managed inline hook:** `void __fastcall(void* perControllerState)`. Native reset clears held-key buckets and axes. QOL clears its independently captured state for the tracked controller, then calls original. |
| Game `0x13CA70` | **Unmodified reader:** `bool __fastcall(void* input, unsigned index, unsigned key)`. Per-controller held-key hash set, stride `0x1C8`. Used only to seed tracking on the game thread; workers never walk the mutable table. |
| Game `0x13CE90` | Input singleton getter; initialized branch at `0x13CEB9` returns game + `0x2A4DC20`. Has TLS/static initialization behavior; not invoked arbitrarily from a worker. |
| Game `0x8B2D0` | Active-controller index getter; reads DWORD at game + `0x2A23704`; qualified range < 8. |
| Game `0x3440170` | Controller-manager pointer slot. Manager DWORD +0xDC equal to 1 denotes controller mode in the reviewed path. |
| Core `0x701BB0` | Points to the game manager slot above; two dereferences from Core slot to manager object. |
| Core `0x6FE440` / `0x7004A8` | Points to game singleton getter / active-index getter. Validated, not replaced. |
| Core `0x6FE470` | Controller-button reader slot used at Core `0x819954`. May be owned by another plugin; QOL neither replaces it nor requires it to point directly to the game reader. |

### Why event interception, not just a state-reader hook

Core’s tooltip branch at `0x8195E4` checks manager mode, calls singleton getter at `0x819604`, active-index getter at `0x81960E`, then invokes the reader at `0x819954`. R8D is `0x800`, observed as **RT**, not RB. Direct game calls to the reader in the reviewed capture were D-pad helpers. These references establish a readable normalized set but do not establish an always-running world-input interception point.

A query-only prototype was rejected before deployment for that reason. The event path gives direct evidence: `0x13EDD0` saves ECX/DL as key/pressed; `0x13F217` obtains the input object; `0x13F243` selects index * `0x1C8`; `0x13F24F` tests pressed; `0x13F260` inserts the held key, while the release branch removes it. Calls at `0x13F337` and `0x13F34E` enter action manager `0x1452E0` and bound-key dispatch `0x2366B0`. Reset callers include `0x13B6EB`, `0x13B7CA` and the all-controller reset loop at `0x13B603`.

### Exact hook prefixes and admission

```text
Game 0x13EDD0 — 21-byte whole-instruction prefix
40 55 53 57 41 55 48 8D AC 24 A8 FD FF FF 48 81 EC 58 03 00 00
Game 0x13DEF0 — 14-byte whole-instruction prefix
40 56 41 56 48 83 EC 28 45 33 F6 48 8B F1
```

The plugin checks substantially more than these prefixes: exact Core hash; event body 0x6A0 bytes; reset body 0xC2; event caller 0x64; reader 240 bytes; singleton-getter prefix 54 bytes; index getter 7 bytes; and the manager/input/index slot targets. `src/native_input_profile.h` holds machine-readable witnesses. `tools/verify_native_profile.py` checked all witnesses and slots against the live Battle.net process before installation. These local source filenames are reproduction aids; the contracts and locating evidence are included here so the request stands alone.

If only the reset hook installs, it remains passthrough until the complete native path activates. SDK owns hook cleanup. A mismatched profile or incomplete native installation selects the existing XInput fallback with a coverage warning. The safeguards reduce unsafe calls; they do not prove every semantic assumption or prevent every possible crash.

### Observed device-normalized key mapping

Read-only live capture with DualShock, Steam closed, one button at a time, active index 0:

| Control position | Game key |
|---|---|
| L1 / LB | `0x0100` |
| R1 / RB | `0x0200` |
| L2 / LT | `0x0400` |
| R2 / RT | `0x0800` |
| Cross / A | `0x1000` |
| Circle / B | `0x2000` |
| Square / X | `0x4000` |
| Triangle / Y | `0x8000` |
| L3 / R3 | `0x0040` / `0x0080` |

D-pad 1/2/4/8 comes from the native helper at `0x13CDD0`. Names describe positions, not a hardware-brand requirement. The current bridge has digital trigger state, encoded internally as 255/0; it does **not** recover analog pressure. An SDK should expose stable named controls and availability flags, not require plugins to adopt these private numeric keys.

### Current plugin responsibilities that an SDK should absorb

QOL preserves original normalized key transitions separately from game-delivered keys. It applies shortcut policy, reconciles releases before new presses, forwards allowed events/repeats to the original dispatcher, and preserves Cross/A for native item activation/identify. Reset clears consumed held keys as well as delivered keys. SDK game-thread tasks every approximately 33 ms refresh a copied snapshot and drive timed navigation; worker readers reject snapshots older than 250 ms. Processing is serialized and recursion passes through. Shutdown drains input processing before clearing feature callbacks.

Those intervals are implementation choices, not requested SDK constants. A coherent loader-owned event/frame contract would remove this local scheduling and state-reconstruction burden. A public pre-dispatch service would retire both private event/reset hooks, their private profile, and eventually external XInput interception, while preserving plugin-defined chord policy.

### XInput fallback — historical primary path

The hardened fallback resolves XInputGetState and ordinal 100 from system XInput modules. Five entries were observed across xinput1_4, xinput1_3 and xinput9_1_0. It accepts a verified prologue or supported module-backed predecessor and retains published relay code for process lifetime on shutdown. It is **not installed when native admission succeeds**. Installed hooks do not establish that a DualShock is exposed through XInput; this distinction caused the original coverage regression. A supported SDK should return explicit coverage/capability failure rather than silently encourage a device-incomplete fallback.

## D. Native navigation, label and prompt entry points

All addresses in this section are **game RVAs**. Current QOL uses SDK-managed inline hooks except the two explicitly listed CALL redirects.

| RVA | Observed purpose | Current wrapper ABI | Desired SDK replacement |
|---|---|---|---|
|0xC66A0|Label press|void(unsigned char channel)|Scoped consumption of vanilla label action|
|0xC6E90|Label release|void(unsigned char channel)|Paired release/cancellation semantics|
|0x27DF80|Main menu/UI switcher messages|void(void* panel,void* message)|Main-panel navigation actions|
|0x878D30|Tab-bar message handler before selection mutation|void(void* widget,void* message)|Scoped tab navigation interception|
|0x14C6810|Skill menu messages|void(void* panel,void* message)|Skill sub-tab actions|
|0x23CAA0|BankPanel/shared-page messages|void(void* bank,void* message)|Explicit stash category/subpage navigation|
|0x86D410|Prompt widget draw|void(void* widget)|Controller prompt metadata/rendering|
|0x86D6A9|First widget CALL to text renderer|CALL site, not function entry|Prompt metadata/rendering|
|0x86D6F3|Second widget CALL to text renderer|CALL site, not function entry|Prompt metadata/rendering|
|0x902E20|Shared text renderer, now unmodified by QOL|void(const char*,void* rect,void* style,float scale)|Keep general rendering outside plugin ownership|

All native wrappers above use the Windows x64 calling convention (spelled __fastcall in our source). Exact parameter meaning remains an observed contract; opaque widget/message objects are not safe SDK structs.

Relevant source symbols: HookLabelPress/Release, HookMenuMessage, HookTabMessage, HookSkillsMessage, HookBankMessage in src/qol_navigation.cpp; HookWidget/HookText in src/qol_glyphs.cpp.

### Byte witnesses

The following are exact leading bytes from the current source profiles. Hex strings are complete for the stated prefix length; do not treat a short prefix as an entire validation strategy.

| RVA | Bytes | Source witness |
|---|---|---|
|0xC66A0|`48 89 5C 24 10 48 89 7C 24 18 55 48 8D AC 24 A0 FB FF FF`|label_signatures.h:LabelBytes0; 19 bytes|
|0xC6E90|`40 53 48 83 EC 20 0F B6 D9 84 C9 75 07 E8 DE A8 01 00 EB 05`|label_signatures.h:LabelBytes1; 20 bytes|
|0x27DF80|`48 89 5C 24 18 48 89 74 24 20 55 57 41 54 41 56 41 57`|menu_signatures.h:MenuBytes0; 18 bytes|
|0x878D30|`4C 8B DC 49 89 5B 18 49 89 73 20 57 48 81 EC E0 00 00`|native_signatures.h:NativeBytes0; 18 bytes|
|0x23CAA0|`48 89 5C 24 10 57 48 83 EC 20 48 8B 02 48 8B D9`|shared_page_signatures.h:Handler; 16 bytes|
|0x86D410|`40 55 57 48 8D AC 24 38 FF FF FF 48 81 EC C8 01 00 00`|glyph_signatures.h:GlyphBytes0; 18 bytes|
|0x902E20|`48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 70`|glyph_signatures.h:GlyphBytes1; 15 bytes|

### Rendering ABI and conflict evidence

The widget draw calls shared renderer0x902E20 at:

```text
0x86D6A9: E8 72 57 09 00 ; return0x86D6AE
0x86D6F3: E8 28 57 09 00 ; return0x86D6F8
```

At each call: RCX=text; RDX=rectangle; R8=style; XMM3=scale. The first rectangle is prepared at[rsp+0x30], style at[rbp]. The second rectangle comes from helper0x1FACD0, style at[rsp+0x50]. QOL redirects only these calls through a near relay, supplies scoped replacement glyph text/rectangle/scale, then calls the unchanged shared renderer. Widget name at+8 and parent pointer at+0x30 are used for bounded ancestor classification. These are private offsets we would prefer not to maintain.

**Concrete third-party collision, binary-verified and user-confirmed:** Stash Search1.10.12 checks32 original bytes at0x902E20 during startup. Our previous global renderer detour failed that check. Moving to the two widget calls restored startup; user reported success. Some compact modded stash layouts still leave compound glyphs cramped, motivating SDK layout-aware prompts.

StashSearch.dll SHA256:
`D2305982248980583C54E4DB73C9CF7973DD7DCD1C71E2EF7EB9CA8F794E1A20`

Stash Search plugin-relative evidence: initializer0xC8B7 calls validator0x10500; validator0x109F9..0x10A0F checks game0x902E20 using helper0x1290 and32-byte witness at plugin0x14650. Failure returns false without a useful log. Expected renderer bytes:

```text
48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 70
0F 29 74 24 60 49 8B F0 0F 28 F3 48 8B FA 48 8B D9
```

Third-party reference: https://github.com/yinyin333333/D2RL-yin-Junk-Room/tree/main/for%20d2rloader/StashSearch . That inspected folder distributes binaries/layout assets, not the C++ implementation; the above is disassembly evidence, not attribution from source.

## E. Interaction selection and ground loot

| Game RVA | Observed contract / use |
|---|---|
|0x34BC90|int(void* player,void* candidate); contact eligibility extension only at whitelisted candidate callers|
|0x18B350|float(void* controller,void* player,void* candidate,int profile); candidate scoring|
|0x18AED0|bool(void* player,void* candidate,int allowance); distance gate within qualified portal comparison|
|0x18A650|void(Selection*,void* candidate,float score,int targetType); comparison/selection|
|0xCBEB0|Ground-item placard builder; controller loot hints|
|0x471950|Native item pickup; suppression/filtering for QOL-owned pickup gestures|

Candidate-contact return sites used for scoping:0x19158E,0x1922D2,0x19237D. Interact skill id357. Selection closure observed fields: best-score pointer+0, skill pointer+8, player-pointer pointer+0x10, selected-pointer pointer+0x18. Object-record portal flag+0x127 bit0x04. Distance helper0x325140 returns native distance units; do not assume meters.

Current portal behavior: expand candidate acquisition/scoring within configured native distance, prefer portal over loot on neutral Interact, and exclude portal candidates from the modified loot interaction. This required addressing earlier candidate rejection as well as final priority. An SDK event emitted only after the native shortlist is built may miss the very candidates plugins need to prioritize.

Additional observation hooks cover18 native action entries; they call predecessor once and observe afterward, leaving the potion plugin's packet-pointer table untouched. This is another case where shared semantic events would be preferable to a collection of native hooks. Entries:

0x4ac050, 0x4ace20, 0x4ace40, 0x4ace60, 0x4ace80, 0x4acf80, 0x4ad030, 0x4ad0e0, 0x4ad100, 0x4ad120, 0x4ad140, 0x4ad230, 0x4ad330, 0x4ad3e0, 0x4ad490, 0x4ad4b0, 0x4ad4d0, 0x4ad4f0

## F. Historical controller bridge — NOT current entry points

QOL retains private Core controller-reader code, but its admission guard expects the older Core SHA256:
`AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0`

That is different from the installed Core hash in sectionA. The old query witness is at Core RVA0x456440; the current working plugin rejects that historical bridge. Release 1.3.1+rev.5 instead uses the separately qualified native event/reset path in section C, with physical XInput only if that new path is unavailable. **Do not enable it by replacing the hash alone.** Recover/review the implementation and offsets first, or replace it with an SDK service. This disabled bridge is evidence of maintenance cost, not a recommended location for the new API.

## G. Reproduction and acceptance matrix

| Scenario | Desired result / reason |
|---|---|
|Hold LB in world, then release|Loot modifier works; vanilla loot filter does not toggle|
|Portal beside eligible loot; neutral A vs LB+A|Neutral prioritizes portal; modified gesture picks assigned loot|
|LB+X in inventory/personal/shared/material/gem/cube contexts|Explicit correct source and destination, never fallback to wrong page|
|Shared page1 then page105|Focused-item identity includes actual subpage|
|LB+R3 over small rejuvenation with both types stocked|Selected potion type wins; refill hint appears only on potions|
|Materials/Gems with integrated cube|Focused child container correctly distinguished from surrounding tab|
|Vendor LB+X|Native sale with gold/item state validated by normal authority|
|Main tabs vs skill/quest vs shared subpages|Correct context-specific navigation and prompts|
|Release LB before LT/RT|Owned chord cannot become an unintended normal tab action|
|Focus loss, panel closure, disconnect while held|No stuck button, delayed unintended action or leaked ownership|
|Direct DualShock with Steam closed, plus Steam-translated controllers|Same supported input pipeline without plugin-owned backend selection|
|No tooltip open; idle held button in world|Frame/snapshot service continues updating independently of rendering|
|Compatible Core rebuild with unrelated changes|Service negotiation succeeds without a plugin hash/profile update|
|SDK controller capability unavailable|Explicit coverage failure; no false claim of complete controller support|
|Two listeners in both registration orders|Deterministic consumption, observable original input, no action resurrection|
|Unload while callback/in-flight request exists|Safe completion/cancellation with no freed function-pointer targets|
|Stash Search and potion plugin alongside QOL|No fingerprint conflict or suppression of another plugin's unrelated action|
|Keyboard/mouse control after controller use|Normal Ctrl-click and other native actions unaffected|

Current automated baseline: **11 QOL suites**, including native key conversion, consumed-key isolation, press/release ordering, repeat preservation, stale snapshot policy, Shared gesture ownership, XInput ownership/lifetime, and glyph-call target/relocation tests. The three user-confirmed controller configurations at the beginning of this document all included Stash Search and Potion Auto Pickup. This does not prove every matrix case, arbitrary plugin compatibility or every registration order. Broader matrix execution would accompany an SDK prototype.

## H. Suggested collaboration plan and open decisions

1. Maintainer chooses earliest appropriate normalized controller interception point and thread contract.
2. Agree on original/effective snapshot semantics, gesture lifetime and consumption ordering.
3. Implement minimum service plus a small two-listener example. Migrate LB loot-modifier behavior first, verifying original/effective state, cancellation and action suppression before retiring either native hook.
4. Run focused regressions, then add UI focus/navigation/prompts incrementally.
5. Extend existing item/interaction services where possible rather than create parallel transaction systems.
6. Once the SDK covers all input processing used by QOL, remove the private event/reset hooks and Core-hash admission for that input feature. Select by service capability; keep unrelated navigation/item guards until their own SDK replacements exist. Avoid running old and new filters simultaneously. Document limited fallback support on older loaders.

Open decisions: Should consumption operate on physical controls, semantic actions, or both? When are UI contexts sampled? How are devices/local players keyed? What is the allowed callback execution budget? Which native operations must be queued rather than executed within input dispatch? What errors/conflicts can a plugin inspect without depending on private objects?

We can contribute verified signatures, current wrapper code, reproduction cases and plugin integration testing. Implementation ownership, service names and ABI design remain the maintainer's choice. No guarantee is made that our current interception points are optimal for Core itself.
