# Native ButtonWidget disabled artwork

## Why the SDK enabled flag did not grey the button

The beta.11 eligibility gate can block clicks while the button still draws its
normal artwork. D2R's `ButtonWidget` has two separate states: the generic widget
input-enabled byte and a button-specific disabled appearance flag. The pinned
SDK changes only the first. Providing `disabledFrame` in the layout does not
connect those states automatically in this build.

The beta.12 adapter retains SDK input blocking, then invokes the game's native
button appearance transition on this plugin's own child. An empty eligible
inventory, held item, or busy batch uses embedded frame 3. Returning to an
available state uses the native enable transition. Both automatic gold-relative
placement and explicit manual placement use this adapter. Artwork, dimensions,
input/transaction safety rules, and configuration remain unchanged.

The instruction-level root cause was established before live confirmation;
the user-confirmed beta.12 visual results are recorded at the end of this file.
The earlier documentation that
`setWidgetEnabled(false)` itself selected frame 3 described an unverified
assumption. The observed SDK implementation does not perform that selection.

## Exact artifacts and source paths

All addresses below are RVAs for the identified artifact only.

| Artifact | SHA-256 |
| --- | --- |
| D2RCore.dll, installed 1.3.1-beta | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Private full D2R 3.3.93787 capture | `A09EA269F8FC10C538DC9A05E4E81E004EC6DFC84AA0F685D3527A974A542A87` |
| Earlier private code-only capture | `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D` |

The installed Core path is
`<game-root>/D2RCore.dll`.
The full capture is
`<private-captures>/runtime-game-auto-deposit-full-20260927.exe`.
The code-only capture is
`<research-root>/inputs/captures/runtime-game-auto-deposit-20260927.exe`.
The full capture includes the type metadata and vtables missing from the
code-only capture; both were inspected read-only. No capture, game binary,
proprietary asset, or generated build artifact belongs in source control.

Public API provenance is `third_party/D2RLoader-PluginSDK/include/D2RLPlugin/widgets.h`:
WidgetService ABI 1, 64 bytes; `setWidgetEnabled` is the function pointer at
service offset 40 and `setWidgetVisible` at offset 32. These use plugin-owned
path handles and must run on the UI thread. No public button appearance setter
is present in this SDK.

## SDK dispatch and the rejected ordering hypothesis

The exact installed Core's WidgetService table is at Core RVA `0x637F70`.
Its seven pointers are `0x495270`, `0x4952B0`, `0x4952F0`, `0x495330`,
`0x495370`, `0x4953B0`, and `0x4953F0`, matching panel lookup, child lookup,
rectangle, visibility, enabled, action dispatch, and input text respectively.
The rectangle implementation at Core `0x4969F0` copies four integers from
native widget `+0x70`, independently supporting this identification.

- Core `0x495330` passes `r9b=1` to shared setter `0x496D60`, which resolves the
  native widget and calls vtable slot `+0x50` for visibility.
- Core `0x495370` passes `r9d=0`; the same helper calls slot `+0x48` for enabled.
- The shared helper preserves the requested boolean from `r8d` and passes it
  to the native setter in `edx`; the native widget is in `rcx`.

Native ButtonWidget construction at game `0x86E920..0x86ED53` installs vtable
`0x1CE4148` at `0x86E948..0x86E951`. This table's enabled slot `+0x48` points to
game `0x837D0` (`88 51 50 C3`: write byte `+0x50`, return). Its visibility slot
`+0x50` points to game `0xF02A0` (`88 51 51 C3`: write byte `+0x51`, return).
Neither setter changes sprite frame, tint, or the button disabled flag.

Therefore reversing the calls to set-visible and set-enabled cannot fix this
button's disabled artwork. The previously documented generic visibility helper
`0x8580D0`, which invokes show/hide callbacks on changed state, is not the
implementation in this particular ButtonWidget vtable. The adapter does not
patch or replace any SDK setter.

## Layout descriptors and rendering state

The full capture's `disabledFrame` string is at `0x1D756A0`; its descriptor's
name pointer is at `0x2535610` and its object offset is `+0x7F4`.
`disabledTint` is at `0x1CF6250`; its ButtonWidget descriptor's name pointer is
`0x2535640` and object offset is `+0x7F8`. Type registration at
`0x86F0B0..0x86F1A9` identifies ButtonWidget; descriptor factory `0x86EE60`
allocates `0xB88` bytes and enters constructor `0x86E920`.

| Field on ButtonWidget | Observed meaning |
| --- | --- |
| `+0x00` | Exact admitted vtable pointer |
| `+0x08`, `+0x30` | Child name pointer and parent pointer, from existing lookup witnesses |
| `+0x50`, `+0x51` | SDK input-enabled and visible bytes |
| `+0x52` | Fill-parent flag, excluded by adapter |
| `+0x67A` | Button-specific disabled appearance flag |
| `+0x678/+0x679` | Cached mouse/press state consumed by native transitions; no writes by adapter |
| `+0x680` | Embedded ImageWidget object |
| `+0x710` | Embedded image frame (`ImageWidget +0x90`) |
| `+0x714`, `+0x71C` | Embedded image alpha and four-float tint |
| `+0x7E8/+0x7EC/+0x7F0/+0x7F4` | Normal, pressed, hovered, disabled frame indices |
| `+0x7F8`, `+0x808` | Disabled tint and normal tint, four floats each |
| `+0x818` | Embedded text widget; native transitions adjust its local position |
| `+0xA48/+0xA4C`, `+0xA50/+0xA54`, `+0xAC0/+0xAC4` | Text offsets consumed by native transition helpers |

The constructor initializes disabled tint to `(1,1,1,0.5)` and normal tint to
`(1,1,1,1)`. When an explicit disabled frame exists, native disable selects that
frame and uses the normal tint. It falls back to normal frame plus disabled
tint only if disabledFrame is negative. Adding `disabledTint` to this plugin's
layout would therefore not address the missing transition.

Native update `0x870710..0x87090A` consumes the disabled flag at `+0x67A`;
it does not copy the SDK input-enabled byte into that flag. Examples of native
callers at `0x239A74` and `0x239A7E` explicitly invoke disable and enable.

## Adopted native contracts and guards

The complete byte witnesses are in `src/button_state_profile.h`. Methods use
Windows x64 ABI: the ButtonWidget pointer is `rcx`; no return value is consumed.
The adapter calls only native enable/disable. Other entries below guard their
supporting code paths and establish the field meanings.

| Game RVA | Span | Contract |
| --- | --- | --- |
| `0x86EEC0` | 120 bytes through `0x86EF37` | Set disabled flag, choose configured frame/tint, tail-call text adjustment |
| `0x86EFB0` | 239 bytes through `0x86F09E` | Clear disabled flag, restore appropriate normal/pressed frame when currently disabled, restore normal tint |
| `0x859700` | 7 bytes | `void SetFrame(ImageWidget*, int)`, writes `+0x90` only |
| `0x837D0` | 4 bytes | Generic enabled-byte setter |
| `0xF02A0` | 4 bytes | Generic visibility-byte setter |
| `0x870460` | 137 bytes | Adjust embedded text position for the selected image frame |
| `0x856FB0` | 66 bytes | Set widget local x/y; fill-parent diagnostic branch is excluded |
| `0x876180` | 30 bytes | Compare widget with panel-manager pressed-widget pointer at manager `+0x178` |
| `0x846020` | 54 bytes | Read manager-owner global `0x3440170`, then return its nested manager pointer at `+0xD0` |
| `0x86E948` | 10 bytes | Constructor's exact ButtonWidget vtable assignment witness |

The enable method crosses a split `.pdata` unwind range. Guarding only the
first range would miss its normal execution tail; the profile guards all 239
bytes. The pressed-widget helper calls the already established manager getter
`0x846020`. The adapter separately requires nonnull manager-owner global
`0x3440170` and its nested manager at `+0xD0`, excluding the getter's null-owner
diagnostic path through `0x8438C0` and subsequent null nested-manager access.

Existing full witnesses are reused for FindPanel `0x846170`, FindChild
`0x856220`, and GetRect `0x8562A0`. Before every appearance application the
adapter checks the instruction profile and admitted Core module identity,
resolves the SDK path and native inventory panel afresh, bounds the native
direct-child vector to 1..256 nonnull entries, and requires exactly one child
named `inventory-deposit/DepositAllButton`. It verifies parent linkage, exact
ButtonWidget vtable and setter slots, matching SDK/native rectangles, the
144-by-136 dimensions, frame configuration `0/2/1/3`, and the input-enabled byte
already applied by the SDK. It also excludes fill-parent on the embedded text
widget to avoid the native position setter's diagnostic branch.

Only a differing disabled flag invokes a visual transition. Read-back must
confirm the requested flag and input state; disabled means frame 3, enabled
means a normal/hover/pressed frame in 0..2. Native pointers are never retained.
An unsupported profile, unresolved binding, or inconsistent read-back returns
false so the caller hides/disables this optional button. A native fault or
changed instruction profile prevents further native calls until reload.
Success logs occur on state transitions only and are capped at 32 per load;
failure logs are bounded per inventory panel.

## Validation scope

`tests/button_state_tests.cpp` executes the exact guarded enable/disable bodies
in a private read/execute allocation using a synthetic `0xB88`-byte widget.
All external calls are relocated to bounded test callbacks, except the exact
seven-byte frame setter, which also operates only on the synthetic widget.
The test checks RCX/subobject argument binding, disabled frame and fallback
tint selection, normal and pressed restoration, an already-hovered frame,
and byte-for-byte preservation outside the expected mutation footprint.
It neither loads game modules nor reads/writes a running game process.

The probe does not establish live object lifetime, native text layout behavior,
texture rendering, callback scheduling, or mouse interaction. The adapter's
type/path/geometry guards and the separate UI/wiring tests cover additional
contracts, but exact-DLL live verification remains required. The simulated
setters used by the earlier UI tests did not model the game's separate input
and appearance states; those tests could not prove greying.

### Beta.12 local validation and deployment - 2026-10-11

- All twelve CTest suites passed, including this isolated native-state probe,
  separate SDK input/native appearance mocks, failed-state read-back handling,
  queued-click refusal and production callback wiring. Independent review found
  no substantive ABI, guard, mutation-footprint or shutdown-order issue.
- Actual DLL ABI, exports/imports, embedded sprite bytes, generated layout,
  runtime package integrity and package privacy checks passed. Source artwork,
  eligibility, transfer, placement and feedback implementations are unchanged.
- Built and installed DLL: `0.1.0-beta.12`, 623616 bytes, SHA-256
  `1A2EEA1962DC88E7F841CA952867D7DD4678BC93AC33E45DD29A3DDFAE2A8562`.
  Active installation is the game's global
  `d2rloader/plugins/Inventory Deposit.dll`; D2R/loader were closed during replacement.
- Previous beta.11 DLL and live configuration were backed up under
  `d2rloader/backups/inventory-deposit-disabled-render-20261011-101913/`.
  The live configuration stayed byte-identical, SHA-256
  `154283F7EBD5CCB13D8056D533E8E8F4C487594BB7211040B3723B8241571E2A`.
- Local runtime ZIP SHA-256:
  `35CE17310AB3171E4D835478BB245E5431013AA2697A8839CBFEE32D9C94E55C`.
  Build, package and deployment records remain local and uncommitted under
  `outputs/inventory-deposit-button/disabled-render/` (build in the adjacent
  `build-disabled-render/` directory). No public release or commit was made.
- Live appearance, hover/click behavior and restoration after adding eligible
  items remain pending on this exact DLL. Native read-back and isolated probes
  cannot establish that the grey texture was rendered in game.

### Beta.12 user-confirmed live appearance - 2026-10-11

After the beta.12 deployment, the user confirmed that the button goes grey
when no eligible items are available and while picking up/holding an inventory
item. With eligible items available, the button brightens when hovered. This
supersedes the pending live appearance status above for those reported states.

The installed DLL hash still matches the beta.12 deployment record above.
The 2026-10-11 runtime log records beta.12 loading at 10:23:00, disabled frame 3
at 10:23:27 and subsequent enabled/disabled transitions. A batch at 10:23:39
reported 18 snapshot items, one submitted source removal confirmed, and 17
ineligible items skipped. These logs corroborate native state and batch activity;
the user's report supplies the visible grey/hover confirmation.

This is user-observed rendering evidence, separate from the automated probe,
read-back and package checks. It does not independently establish every pressed
state, low-end scaling, layout variant or transfer outcome. The subsequent
beta.13 request only shortens the tooltip to "Deposit All Gems, Materials and
Runes"; it introduces no new native contract or visual-state behavior.
