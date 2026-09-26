# Native controller input — 1.3.1+rev.5

Date: 2026-09-24. Author: PyreFly. Status: production release. Eleven automated suites passed; user confirmed Battle.net DualShock (Steam closed), Steam Controller on Steam, and DualShock on Steam, all with Stash Search and Potion Auto Pickup enabled. See PRODUCTION-1.3.1-rev.5.md for validation scope. The DLL is unchanged from the tested candidate. Earlier production snapshots remain preserved.

## Problem and approach

The Battle.net installation, with a DualShock connected and Steam closed, ran native menu remaps but missed L1 loot and Shared-page chords. The 17:38 log showed the historical core bridge rejected its provider; XInput hook installation succeeded but that did not establish device detection. The historical bridge only supported named alternate-action state, not every button. Simply admitting the new core hash would call unrelated code and is unsafe.

The new primary provider observes normalized key transitions **before** the game mutates its held-key set or dispatches actions. Both the transition handler and the controller-reset handler use PluginContext::InstallInlineHook. A game-thread task scheduled through the SDK refreshes the snapshot and drives timed navigation pulses; it does not depend on tooltips being visible. Worker readers consume one atomic snapshot and never walk the game's mutable hash tables.

The original native event function performs the actual set updates and action/UI dispatch for keys QOL allows. QOL tracks physical normalized transitions separately from delivered keys, releases keys that become suppressed, and forwards allowed press/release edges and repeat presses. The native reset hook clears tracking on the game's own device/input reset, including consumed buttons missing from the game set. The game selects the active controller. Unknown keys, inactive controller mode and shutdown pass through.

**Review finding that changed the implementation:** the normalized query function `0x13CA70` is suitable for reading held keys, but its only direct game callers are D-pad helpers; the reviewed Core indirect call is the tooltip path. A query-only hook could miss world input and would not intercept the actual action dispatcher. The first local candidate built against that idea was never deployed. The final implementation leaves that reader unmodified and hooks the verified event/reset path below instead.
On successful native installation QOL installs **no XInput hooks**. If admission or SDK hook installation fails, the hardened XInput fallback remains available and logs its limited device coverage. An idle native state is authoritative and does not get merged with a phantom XInput device. Native LT/RT are digital pressed states: the game's trigger threshold applies, not QOL's raw XInput analog threshold. Synthetic navigation emits native key transitions through the same original dispatcher. Cross/A remains visible to native interaction/identify; modifier+B/X/Y/RB/R3 and trigger suppression retain the existing policy, with Shared-page triggers passed through.

## Qualified binaries and paths

Both installations have identical relevant binaries:

- Battle.net: `<battle-net-game-root>\D2RLoader.exe`, `D2RCore.dll`.
- Steam: `<game-root>\D2RLoader.exe`, `D2RCore.dll`.
- Core SHA-256: `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
- Loader SHA-256: `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`.
- Core/loader file version: `1.3.1-beta`.
- Live investigation PID: 25828; game base `0x140000000`, Core base `0xC0DE5000000`. These are observations, not hard-coded runtime absolute addresses.

## Address inventory and evidence chain

All game addresses below are RVAs relative to PluginContext::exeBase. Core RVAs are relative to the loaded D2RCore.dll.

| Address | Meaning and evidence |
|---|---|
| Core `0x8195E4` | Tooltip controller-mode check loads Core slot `0x701BB0`, dereferences the resulting game singleton slot, compares manager DWORD `+0xDC` against 1. |
| Core `0x701BB0` | Live pointer equals game `0x3440170`; that game slot points to the controller manager. Two dereferences from Core slot to manager object. |
| Core `0x819604` / slot `0x6FE440` | Calls game `0x13CE90` to obtain the normalized input singleton. |
| Game `0x13CE90` | Getter's initialized branch returns `game+0x2A4DC20` using RIP-relative LEA at `0x13CEB9`. The getter also has TLS/static initialization code; the new hook never calls it from the worker. |
| Core `0x81960E` / slot `0x7004A8` | Calls game `0x8B2D0` to read active controller index. |
| Game `0x8B2D0` | `mov eax,[rip+0x299842e]; ret` returns DWORD at game `0x2A23704`. Core checks index < 8. |
| Core `0x819949`–`0x81995A` / slot `0x6FE470` | Calls the button reader with RCX=input, EDX=index, R8D=`0x800`. This is a normalized **RT** key, not RB. Item Roll Ranges may already own this Core slot; QOL leaves it untouched and does not assume its current target is native. |
| Game `0x13CA70` | `bool __fastcall(void* input, unsigned controllerIndex, unsigned key)`. Native function checks membership in a per-controller hash set, stride `0x1C8`: bucket count +0, bucket pointer +8; linked nodes next +0/key DWORD +8. Index assertion is < 8. This is the read-only seed helper, not a hook entry. |
| Game `0x13CAAB` | `imul r8,[rsp+0x30],0x1c8`, independently establishes per-controller stride. |
| Game `0x13B350` | Normalized OS-button message adapter: device identity DWORD +0xC, key DWORD +0x10, pressed byte +0x14. It verifies active controller, normalizes the key through `0x13D080`, then calls `0x13EDD0` at `0x13B3A2`. |
| Game `0x13EDD0` | `void __fastcall(unsigned key, bool pressed)`; active index from `0x8B2D0`. Saves ECX/DL in its prologue. Normal branch at `0x13F217` gets input; `0x13F243` computes controller stride; `0x13F24F` tests pressed; `0x13F260` inserts key, release branch erases it. `0x13F337` invokes action manager `0x1452E0`; `0x13F34E` dispatches the bound key callback through `0x2366B0`. New primary event hook. |
| Game `0x13DEF0` | `void __fastcall(void* perControllerState)`; clears held-key buckets and axes. Called by device handlers at `0x13B6EB` and `0x13B7CA`, and all-controller reset in `0x13B603`. New reset-observation hook; pointer must match singleton + tracked index * `0x1C8`. |
| Game `0x13CDD0` | Native D-pad helper queries `0x13CA70` with keys 4/8 (horizontal) and 1/2 (vertical), confirming key interpretation. |

Event replacement length is **21 bytes**, ending at a whole instruction: `40 55 53 57 41 55 48 8D AC 24 A8 FD FF FF 48 81 EC 58 03 00 00`. Reset replacement length is **14 bytes**: `40 56 41 56 48 83 EC 28 45 33 F6 48 8B F1`. Admission checks the event body (0x6A0 bytes), reset body (0xC2), OS event caller (0x64), held-key query (240), singleton-getter prefix (54), index getter (7), exact Core file hash, and manager/input/index slot values. Machine-readable witnesses are in `src/native_input_profile.h`. Do not change the hash without re-deriving the associated addresses and semantics. If Reset installs but Event fails, the reset callback remains a passthrough; SDK owns cleanup and XInput fallback remains available.

## DualShock observations (Steam closed)

User pressed the requested controls individually for about two seconds, released between presses. ReadProcessMemory inspected the game-owned normalized set without writes, injected calls, or suspension. Controller index remained 0. Observed releases emptied the set.

| Control | Native key |
|---|---|
| L1 | `0x0100` (17:52:26) |
| R1 | `0x0200` (17:52:29) |
| L2 | `0x0400` (repeat capture 17:54:07–12) |
| R2 | `0x0800` (repeat capture 17:54:15–21) |
| Cross / A | `0x1000` (17:52:37) |
| Circle / B | `0x2000` (17:52:40) |
| Square / X | `0x4000` (17:52:43) |
| Triangle / Y | `0x8000` (17:52:45) |
| L3 | `0x0040` (17:52:48) |
| R3 | `0x0080` (17:52:51) |

Face/shoulder/stick keys match XInput bit positions. `0x400` and `0x800` are additional native digital trigger keys, stripped from the internal button field and represented as trigger values 255/0. These names describe positions, not a requirement to use an Xbox device. Read-only tools: `tools/audit_native_controller.py`, `tools/monitor_native_controller.py`. The latter is a bounded observational tool, not runtime plugin code; captures can straddle presses, so repeat missed controls rather than guessing.

## Plugin cooperation and remaining limitations

- SDK-managed original trampoline; no Core controller-query slot, shared glyph renderer, or other plugin's binary is overwritten.
- A pre-existing change to these qualified functions causes full-function admission to fail before any native mutation. Do not accept arbitrary executable predecessors or silently skip the code guards. XInput fallback logs its device limitation.
- A later plugin may still conflict if it insists on pristine bytes at either intercepted game function. This is a new shared interception point, not a claim of universal compatibility. A loader-owned ordered input filter/observer API remains the proper long-term answer.
- Native events and the SDK game-thread pump are synchronized through the input processing gate and a native processing mutex, with recursion passing through. Shutdown drains input processing before clearing callback dependencies. The SDK retains responsibility for its inline-hook lifetime.
- Unknown builds require profile revalidation. The old historical core bridge is still guarded by its old hash and is not re-enabled by this change.
- Existing Stash Search glyph-call workaround and potion-auto-pickup action-table cooperation remain intact.
- Native digital trigger threshold is owned by the game. Raw analog trigger custom thresholds remain a fallback-only behavior.

## Validation / patch recovery checklist

`tools/verify_native_profile.py 25828` verified every final code witness and all three slots against the running Battle.net process before deployment (no writes).

Automated: existing ten suites plus native state/chord isolation, all 65,536 normalized key conversions, Shared-trigger ownership, stale/invalid-time rejection, unknown-key pass-through policy, native press/release ordering, consumed-key isolation and repeat preservation. These do not substitute for executing the hook in-game.

After installation, require the `[QOL/NativeInput] Native normalized controller input active` startup line. With Steam closed and DualShock: L1 loot labels; L1+each loot key without casting/dropping; neutral Cross portal priority versus L1+Cross loot; Shared L1+L2/R2; inventory L1+Square/Triangle/R3; vendor sale; native menu and quest/skill navigation; release/reconnect; exit/re-enter. Then repeat core shortcuts on Xbox/Steam and load alongside Stash Search and potion pickup. Use this as the detailed regression checklist after patches; the user-confirmed matrix is recorded in the production release note.

For a patch: recover the Core tooltip controller branch first, follow manager/input/index calls, inspect reader ABI and set semantics, verify individual button values live, regenerate code witnesses, run suites, then repeat the controller matrix. Keep source and runtime production snapshots until that succeeds.
