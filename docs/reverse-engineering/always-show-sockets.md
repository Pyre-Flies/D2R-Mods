# Native socket display — 2026-10-12 candidate

## Provenance

Read-only inspection of running D2RLoader process 62480, mapped game image at
0x140000000, PE timestamp 1790146604. Addresses below are game-image RVAs,
not Core RVAs. Session addresses are observations, not reusable constants.

- Loaded executable: D2RLoader.exe SHA-256
  `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`.
- Loaded D2RCore.dll SHA-256
  `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
- Source string `D2Client\src\UI\Widgets\InventoryItemWidget.cpp` at
  RVA 0x1cf1360; `gemSocketFilename` at 0x1cf1540. Property-registration
  references at 0x2a8bed and 0x2a8d9d.

## Static witnesses, not live visual validation

| RVA | Interpretation and evidence |
| --- | --- |
| 0x2a72d0 | Item icon drawing, RCX widget / RDX item. Null-item early return; computes widget scale and dimensions, dispatches widget vtable +0xc0 and +0xf8, then native image renderer 0x15bb80. Caller ignores return registers. |
| 0x2a73c0 | Socket artwork candidate, same two arguments. Native predicates at 0x373610 / 0x36ead0 and item flags 0x800 / 0x10 must pass; computes dimensions and native placement table; draws socket sprite and attached item artwork. Caller ignores return registers. |
| 0x36c750 | Socket-count candidate, repeatedly supplies loop bound and placement-table index inside socket renderer. Meaning inferred from consumer; not separately runtime validated. |
| 0x388c10 | Native inventory first-item iterator, used on the host item's inventory to enumerate socket contents. |
| 0x38aab0 / 0x38aba0 | Current inserted item / next socket inventory entry candidates inside rendering loop. |
| 0x858510 | Socket sprite drawing call, widget+0x5a8 passed as RCX. |
| 0x2a8e60 | Widget resource setup; filename string at +0x580/count +0x588, sprite object +0x5a8; resource helper 0x858990. Clears byte +0x459 before base lifecycle dispatch. |
| 0x2c3731 | Grid path calls socket renderer immediately after icon call 0x2c3726. Selection item equality at 0x2c36e8 plus additional native conditions gate this branch. Do not bypass the whole branch: it also draws selection feedback. |
| 0x2c9a82 | Slot path calls socket renderer immediately after icon call 0x2c9a77. Native interaction/selection predicates gate this branch. |

Socket loop draws all socket positions and advances attached-item iterator only
when an entry exists, which supports empty and filled socket visuals. Widget
fields +0x5b8/+0x5bc contribute positioning dimensions. Field semantics and full
function ABI remain bounded static interpretations pending live confirmation.

Entry guards:

- 0x2a72d0: `48 85 D2 0F 84 D7 00 00 00 56 57 48 83 EC 68`.
- 0x2a73c0: `40 55 53 41 54 48 8D AC 24 70 FF FF FF 48 81 EC 90 01 00 00`.

## Probe strategy and limits

Hook icon drawing, preserve original call, then invoke original socket renderer
on the same thread with the same widget/item. Hook the socket renderer separately
to consume an immediately matching duplicate from existing selected paths.
Thread-local matching clears on each icon call. Both hooks must succeed before
enabling; partial installation preserves native calls. No hover state changes,
native item writes, extracted assets or new sprite dependencies.

Native draw order/clipping, sprite readiness, supported panels, double rendering,
classic mode and lifecycle are not established by disassembly. Runtime hooks and
visible results must be checked before promoting this candidate. Local survey and
disassembly outputs are temporary evidence under outputs/ and are not committed.

## Automated validation

MSVC 19.44 Release build completed with /W4 /WX. Artifact test passed (1/1):
one-DWORD SDK ABI resource, normal DLL load, three lifecycle exports, metadata,
client/native-hook role, local debug command, unsupported fixture remains inert
and cannot enable rendering. This does not execute native drawing or establish
live hook admission.

## First live result

After installing 0.1.0-probe.1, the user reports automatic socket display and
provides an equipment/inventory screenshot showing two filled socket visuals on
equipped armor without a tooltip. This confirms that specific visible result.
Empty and filled sockets together on one item, vendors, trade,
controller, classic mode and scaling remain unverified. Feature defaults to ON;
the user does not need to enter a console command to activate it.
Active mod log `d2rloader/logs/always-show-sockets.log` at 17:00:09.505 on
2026-10-12 confirms native icon/socket hooks admitted and always-visible ON.
The subsequent user-confirmed screenshot shows three empty socket circles on an
unhovered inventory sword. Empty sockets in inventory and filled sockets on
equipped armor therefore both have accepted live visual coverage. This does not
establish empty and filled sockets together on one item or other input modes.
The user subsequently identifies the armor's inserted items as a gem and a jewel,
confirming mixed inserted item types. The user also confirms socket display in
the Cube, personal stash and shared stash. These are user-confirmed live results;
the exact item/socket combinations used in those containers were not specified.

## probe.2 native slash command

Reuse the vendored native chat backend from Floating Damage without altering
its signatures, record ABI, deferred registration or callback lifetime contracts.
Canonical getter/registration/caller RVAs, guards, definition offsets, pinning and
live backend evidence are in [the chat command contract](floating-damage-chat-command.md).
No new native addresses were introduced. The backend is copied into this plugin
to keep its build standalone and requires exact artifact admission before Start.

Register local command `sockets`; bound borrowed arguments to 16 bytes and route
them through the same handler as the SDK console. Empty arguments or `toggle`
flip the session flag; `on`/`off` set it; `status` leaves it unchanged. No network
chat packet is sent. Existing console output reports state and logs record changes.
Stop drains native callbacks before disabling plugin state. Restart is required
to update/remove because native parser entries cannot be unregistered.

MSVC Release build and two suites pass: artifact admission/unsupported refusal,
and the actual slash-to-console adapter's toggle/on/off/status, invalid/overlong
arguments, unavailable build and malformed borrowed input. Rendering hooks are
unchanged. Socket-specific native registration and slash dispatch await live test.

## Quality Backgrounds compatibility audit — 2026-10-13

Installed global plugin `d2rloader/plugins/d2rl-item-quality-backgrounds.dll`,
SHA-256 `F3B6C2D532D22FD7BF32F1AE0EC8782B7735D1D028EEE52539E44D761B9861BA`.
Bounded static DLL disassembly, not a combined live test:

- Plugin RVA 0x130a7 supplies game RVA 0x2a72d0 to its renderer guard helper.
- Plugin RVA 0x13389 sets the inline-hook registration's game RVA to 0x2a72d0;
  expected size 15 at 0x1339d, hook target plugin RVA 0xa980 at 0x133a8,
  registration helper call at 0x13414. Expected bytes at plugin RVA 0x187f0 are
  `48 85 D2 0F 84 D7 00 00 00 56 57 48 83 EC 68`, identical to our icon guard.
- Its icon hook at 0xa980 reads the caller return address at 0xa9c7 and compares
  game-relative return RVAs 0x2c9a7c, 0x2c9b82 and 0x2c9c1b at
  0xa9d6/0xa9de/0xa9e6, using the result as an equipment-path discriminator.
  A wrapper that changes that return address may therefore alter its behavior.
- Separate sprite-resource hook registration uses game RVA 0x858990 at plugin
  RVA 0x130d9. Game RVA 0x858510 is bound as a drawing helper at 0x131b3.

Both installed plugins request the same icon entry and pristine byte guard.
Whichever patches it first can cause the other's guard/admission to fail;
directory separation does not isolate patches in the shared game image. No
current combined-run log was available to establish the exact observed failure.
Do not weaken guards or make load order the compatibility contract. Candidate
repair: move socket augmentation to a distinct post-icon rendering point while
preserving the original native icon call and its caller return address. This is
a design direction, not a validated alternative RVA/ABI. A shared loader item-
render callback is another option if both features must use the same consumer.

Existing socket log additionally confirms native `/sockets` registration result
0 at 2026-10-12 17:14:52.876 and OFF/ON dispatch at 17:15:06.155/17:15:08.915.
This establishes registration/state dispatch, not a separately reported visible
toggle test or combined Quality Backgrounds coverage.

## beta.2 post-icon compatibility candidate — 2026-10-13

Same executable/Core hashes as the original profile. Replace only our icon-entry
hook with an interior hook at game RVA 0x2a73a0, after the icon renderer call at
0x2a739b. The call and function entry 0x2a72d0 remain unmodified by our plugin.
Quality Backgrounds can therefore retain its entry hook and inspect the same
original caller return address regardless of plugin load order.

New guard: `0F 28 74 24 50 48 8B 5C 24 60 48 83 C4 68` at 0x2a73a0.
Native instructions restore XMM6 from RSP+0x50, RBX from RSP+0x60, and add 0x68
to RSP; remaining epilogue pops RDI/RSI and returns. Earlier frame witnesses:
0x2a72d9/0x2a72da push RSI/RDI; 0x2a72db subtracts 0x68; 0x2a72df sets RDI
from RCX (widget), 0x2a72eb sets RSI from RDX (item). Thus RDI/RSI retain those
arguments and RSP is 16-byte aligned at the post-draw site.

The MASM adapter enters via JMP, allocates 0xc0 with LEA to preserve flags,
reserves Win64 shadow space, saves RAX/RCX/RDX/R8-R11, XMM0-5 and RFLAGS, calls
the bounded socket adapter with RDI/RSI, restores state, and jumps through the
loader trampoline to replay the displaced native epilogue. Nonvolatile registers
and XMM6-15 are preserved by the Win64 callee ABI. Existing socket renderer entry
hook remains at 0x2a73c0, outside Quality Backgrounds' mutation ranges.

Executable assembly fixture enters with native-equivalent stack alignment,
sentinel GPR/SIMD values and carry flag, invokes the real adapter and verifies
preservation, widget/item forwarding, selected-path duplicate suppression and
disabled-mode native socket passthrough. This caught and corrected flag changes
from an initial SUB allocation. Release build and both test suites now pass.
Static non-overlap and fixture checks do not prove combined live rendering or
real loader admission of the interior detour. Test both features together before
publishing beta.2. The published beta.1 remains unchanged during this candidate.

### Accepted combined live result

On 2026-10-13, after deploying beta.2 with the global Quality Backgrounds DLL
unchanged, the user confirms both quality backgrounds and sockets work and
`/sockets` toggles correctly. This upgrades the combined visual and slash-toggle
checks from pending to user-confirmed live coverage. It does not establish all
load orders, other Quality Backgrounds builds, or previously untested panels,
input modes, classic graphics and scaling changes.

## Inserted-item scale/opacity feasibility — 2026-10-15

Bounded reinspection of the retained exact-build disassembly, not a new runtime
test: socket circle draw is a separate call to 0x858510 at 0x2a76ea. Inserted
item artwork later calls the shared item image renderer 0x15bb80 at 0x2a776c.
At 0x2a7752 XMM2 is assigned from the computed scalar in XMM0; this follows
widget scale/parent scale calculations at 0x2a7707..0x2a7736. RCX is the inserted
item (R15), RDX packs calculated coordinates, and R9 points to local options at
RSP+0x50. The ordinary host icon has a distinct call at 0x2a739b to the same
shared image renderer. These witnesses support a narrowly scoped inserted-art
scale candidate while leaving loose/host icon draws alone.

Do not globally change 0x15bb80 or scale the entire host widget. Reduced inserted
art must be recentered within the native socket position; the size/pivot details
still require validation. Opacity/alpha is not established by the observed
options pointer or its byte initialized at RSP+0x54. No alpha field or ABI is
claimed. Size changes appear lower-risk than opacity work; neither is implemented
or live validated. Keep Quality Backgrounds' icon-entry hook and caller semantics
intact when selecting any additional hook site.

## beta.3 centered HD scaling candidate — 2026-10-15

Provenance: retained analysis image
`runtime-game-auto-deposit-20260927.exe` SHA-256
`02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D`.
Installed executable/Core still match the original exact hashes. These new
contracts are static witnesses in the analysis image; current mapped byte guards
are checked at plugin load. No claim of beta.3 live behavior yet.

### Scoped image consumer and ABI

- At game RVA 0xc71e3..0xc71f8 the item-image builder forwards its resolved sprite
  through native sprite draw 0x858510. R15 is the sprite wrapper, XMM6 the UI
  scale, and native RSP+0x28 the two-int32 pixel anchor. Native RSP is aligned
  to 16 at this site. Displaced preparation instructions are `movaps xmm3,xmm6`,
  `lea r8,[rsp+0x60]`, `lea rdx,[rsp+0x28]`, `mov rcx,r15`.
- New site guard at 0xc71e3:
  `0F 28 DE 4C 8D 44 24 60 48 8D 54 24 28 49 8B CF`.
- HD mode query 0x846210 reads a mode-manager byte at +0xb9, falling back to
  0x650210 if unavailable. Entry guard:
  `48 8B 05 59 9F BF 02 48 85 C0 74 08 0F B6 80 B9 00 00 00`.
- Sprite wrapper +0 is the HD sprite pointer (0x858880 returns it). The HD
  renderer 0x79da50 reads width at +0xd0 (instruction 0x79daf1), height +0xc0
  (0x79dbb2 and 0x79df35), pivotX +0xc4 (0x79db60), pivotY +0xc8
  (0x79db8a). Width reader guard: `41 8B 86 D0 00 00 00` at 0x79daf1.
  Native renderer subtracts pivot*scale from the supplied anchor and adds
  width/height*scale to build its draw rectangle. No writes to sprite metadata.

Scope each call to the existing socket renderer with a thread-local RAII depth.
Only item-image sprite calls inside that scope may be resized; loose/host icons
outside it pass through unchanged. Socket rings draw directly through 0x858510,
so they bypass the modified item-image preparation site. The existing socket
and post-icon hooks at 0x2a73c0/0x2a73a0 remain in place. Quality Backgrounds'
icon entry 0x2a72d0 and its resource hook 0x858990 remain untouched. No general
hook chaining is assumed; unknown occupied scale-site bytes refuse only scaling.

MASM gateway saves volatile GPRs, XMM0–6 and flags with 0xd0 aligned stack
storage, including Win64 shadow space. C++ receives R15, original RSP+0x28,
and a pointer to saved XMM6's low float. After bounded metadata reads and mode/
scope checks, it updates only that anchor and scale. The gateway restores state
and replays the original preparation instructions through the loader trampoline.
The native image function later restores the caller's original XMM6 itself.

Center formula: newAnchor = oldAnchor + (oldScale-newScale)*(size/2-pivot),
rounded once to a pixel. Percent is 10–100, defaults to 65, 100 preserves exact
native inputs. Invalid bounds, nonfinite/overflowing geometry, unsupported mode
or missing optional guards keep native dimensions. File setting is embedded;
chat size adjustments are session-only. No item data, gem ownership, saved game,
Quality Backgrounds state or sprite resources are modified.

Automated results: three suites pass, including real assembly gateways with
native-equivalent stack alignment, sentinel GPR/SIMD/flags, scoped inserted vs
unscoped loose draws, classic fallback, command bounds, config defaults, and
centering within half a pixel at 0.5/1/2/4 UI scales with varied pivots. Actual
loader admission and visible centering/coexistence remain pending.

### Ethereal opacity lead (not implemented)

The user's ethereal observation has a static witness: item-image builder
0xc7188..0xc71c5 tests item flag 0x400000 via 0x36e2d0, then chooses a float
from UI manager +0x4b0 instead of the ordinary fallback. It writes the value
to local sprite options at native RSP+0x70 (options +0x10). A separate optional
override at RDI+0 (float) with RDI+4 (valid byte) can replace that float at
0xc71cd..0xc71dd. The higher image wrapper 0x15bb80 forwards the borrowed
override through its own local record and clears the validity byte afterward.
These are opacity candidates, not a fully validated public override ABI. Do not
fake an ethereal flag or modify the shared UI-manager opacity to implement them.
This scaling candidate leaves all opacity inputs and item flags unchanged.

### Accepted beta.3 scaling result

On 2026-10-15 the user confirms 65% looks good and loose items remain normal
size. The active socket log records beta.3 native admission, native `/sockets`
registration, the first centered inserted draw, and size changes ending at 65%.
This establishes the scoped size path in game. It does not claim beta.4 opacity
validation or new Quality Backgrounds combination coverage.

## beta.4 native opacity candidate — 2026-10-15

Reuse beta.3's admitted game RVA 0xc71e3 adapter; no new hook. Its fourth C++
argument is original native RSP+0x70, computed as adapter RSP+0x140 after its
0xd0 allocation. That float is options+0x10 in the native sprite settings at
original RSP+0x60. It has already been populated by native ethereal selection or
an existing per-draw override when the adapter runs. Native RGB/settings bytes,
override validity, shared UI alpha and actual item flags are never modified.

Independent opacity producer guard at 0xc71cd:
`80 7F 04 00 F3 0F 11 44 24 70 74 0A F3 0F 10 07 F3 0F 11 44 24 70`.
HD settings consumer guard at 0x79e062:
`F3 44 0F 10 66 10` (load options+0x10 into XMM12).
Provenance remains the named analysis image above plus exact installed artifact
admission; these additional byte windows are checked in current mapped code at
load. Opacity guard mismatch leaves native opacity while retaining size support.

Only inside synchronous socket-render scope, with HD mode and all feature
guards admitted, multiply a finite native alpha in [0,1] by the configured
10–100 percent. Default is 65. At 100 leave the original float unchanged;
existing ethereal/override fades are therefore retained instead of replaced.
Host and loose-item draws occur outside the scope, and socket rings bypass the
item-image stage, so neither gets this additional opacity. No new overlap with
Quality Backgrounds' icon-entry or resource-load hook is introduced.

File setting `inserted_item_opacity_percent` is independent from size. Missing
keys in prior size-only configs get the 65% opacity default without overwriting
the file. `/sockets opacity N` adjusts session state only. Classic mode, disabled
feature, unadmitted opacity guards and invalid native alpha keep native values.

Three Release suites pass: actual assembly adapter forwarding of the opacity
stack slot while preserving sentinel registers/flags; inserted-only vs loose
draw scope; 0.5 native alpha becoming 0.325 at 65%, 100 preserving 0.5; disabled,
classic and guard-refusal paths; command/config bounds and embedded resources.
The scaled geometry tests still pass. Native opacity guard admission and visible
beta.4 rendering await the user's live test; no publication is requested here.

### Accepted beta.4 result and coexistence limits

On 2026-10-15 the user confirms transparency looks good, loose items remain
normal, and the previously tested Quality Backgrounds combination works with
this candidate. This is accepted live appearance/coexistence coverage for that
combination; it does not cover arbitrary companion versions or load orders.

Current mutation entries are 0x2a73a0 (post-icon), 0x2a73c0 (socket renderer),
and 0xc71e3 (item-image sprite preparation). The opacity addition reused the
last site and introduced no fourth hook. Another plugin occupying overlapping
bytes can cause byte-guard or hook admission refusal. A plugin replacing a
broader renderer without forwarding may bypass these native paths even with
non-overlapping entry bytes. A plugin altering scale, pivots or alpha can produce
visual composition differences; our alpha multiplies the current native value.
Our startup guards cannot guarantee behavior if an unguarded later patch
overwrites our detour. A shared native `/sockets` name can also collide separately
from rendering. The backend has no general native hook-chain/unregister contract.

No new source change is made for this assessment. The accepted Quality
Backgrounds combination is stronger evidence than a static non-overlap check,
but newly encountered native renderers still need specific compatibility review.

## Socket-circle size feasibility — 2026-10-15

Reinspection of the retained exact-build socket renderer confirms the separate
circle sprite draw at game RVA 0x2a76ea calls 0x858510 with RCX=widget+0x5a8,
RDX pointing to native RSP+0x40 (pixel anchor), R8 pointing to RSP+0x60 (sprite
settings), and XMM3 copied from the parent/widget scale in XMM6 at 0x2a76cf.
This call bypasses beta.4's item-image stage at 0xc71e3, so existing size and
opacity settings do not affect circles.

A candidate extension could adjust that circle call's scale/anchor using the
same HD sprite bounds and center-preserving math, without scaling the entire
widget or changing native socket placement spacing. It would apply to both
empty and filled socket circles and could have a separate circle-size control.
It needs an additional guarded integration point scoped to this call, preserving
native argument/register state and Quality Backgrounds' existing entry/resource
hooks. No circle scaling is implemented or live tested by this feasibility check.

## beta.5 linked circle size and empty-slot emphasis — 2026-10-15

Same exact executable/Core profile and retained analysis image as above. New
optional mutation: circle-call site 0x2a76ea. Expected bytes, including native
occupied-slot decision after the call:
`E8 21 0E 5B 00 4D 85 ED 0F 84 84 00 00 00`.
Circle alpha setup guard at 0x2a75f2:
`C7 44 24 70 00 00 80 3E`, initializing native RSP+0x70 to 0.25f.
HD mode/sprite-width and alpha-consumer guards remain required. Unsupported or
occupied circle sites decline only this extension and retain native circles.

At the circle call, RCX is widget+0x5a8 (sprite wrapper); RDX points to the native
two-int32 anchor at RSP+0x40; R8 points to settings at RSP+0x60; XMM3 contains
parent/widget scale; RSP+0x70 is settings+0x10 opacity. RSP is 16-byte aligned.
R13 is the native current inserted-entry iterator, initialized from 0x388c10 at
0x2a7484/0x2a74a5. Immediately after drawing the circle, the native code tests
R13 at 0x2a76ef and skips inserted artwork if zero. The occupied branch obtains
the item through 0x38aab0 at 0x2a76fb, then advances R13 through 0x38aba0 at
0x2a7774/0x2a7779. The adapter mirrors that exact zero/nonzero predicate instead
of inferring occupancy from colors, socket counts, item quality or asset names.
It does not dereference or retain the iterator.

The circle gateway saves volatile GPRs, XMM0-5 and flags in a 0xd0 frame with
Win64 shadow space. It passes saved XMM3 by reference and a 16-byte local
CircleState record (opacity pointer at +0, borrowed iterator value at +8).
The native opacity pointer is adapter RSP+0x140 (original RSP+0x70). After the
helper, saved registers/flags and the native argument pointers are restored,
then the loader trampoline executes the original draw call. Nonvolatile R13
and the native occupied/empty branch are preserved.

Within admitted HD socket scope, apply the existing scale percentage and same
center formula to circles. Empty (R13==0) circles receive temporary opacity 1;
filled circles retain the native float, normally 0.25. Inserted art still uses
its independent relative opacity. Size 100 preserves exact native circle size
and anchor, while empty circles remain highlighted until the feature is turned
off. Classic/disabled/unavailable paths retain the complete native appearance.
No new config key or circle-size command is introduced. The prior saved
`inserted_item_scale_percent` now controls both; native positions/spacing and
loose items are unaffected.

Current mutation entries are 0x2a73a0, 0x2a73c0, 0xc71e3 and 0x2a76ea. The
tested Quality Backgrounds artifact's 0x2a72d0/0x858990 hooks remain untouched.
This extra call-site hook still needs specific review against other socket
renderers; no hook-chaining assumption is added.

Release build and three suites pass, including the real circle gateway with
native-equivalent alignment, sentinel GPR/SIMD/flags, original sprite/settings
pointers and preserved R13; shared `/sockets size` changing ring scale; filled
vs empty alpha; 100% size; disabled/classic/refused-feature passthrough. The
inserted scale/opacity and loose-item suites still pass. Native circle detour
admission, visual centers and full-opacity empty emphasis await a live check.

## beta.6 empty-outline correction — 2026-10-15

The user reports beta.5 makes empty sockets look like solid black holes and
clarifies that only the outline should be opaque. Whole-sprite alpha is therefore
a rejected implementation: it affects the native sprite's interior as well as
its edge. Beta.6 never increases opacity on the original native circle. Filled
circles retain it; empty circles also retain it if the optional replacement fails.

Use an original procedural thin warm-gray ring with fully transparent center and
exterior. No extracted game art is used or replaced. Generate 128-square HD and
64-square low-end SpA1 format-31 RGBA8 files in memory and register them through
ResourceService v1. The 40-byte, one-frame header follows the independently
constructed format already documented in Inventory Deposit's BUTTON-SPRITES.md.
The two owned virtual paths are:

- `data/hd/global/ui/d2rloader/always-show-sockets/empty-outline.sprite`
- `data/hd/global/ui/d2rloader/always-show-sockets/empty-outline.lowend.sprite`

The entry passes extensionless `d2rloader/always-show-sockets/empty-outline` to
the native sprite wrapper loader at game RVA 0x858990, once on the drawing thread
at its first empty-circle draw. Its adopted ABI is
`void(NativeSprite* wrapper, const char* filename, bool option)`, where the
16-byte wrapper holds HD resource at +0 and classic resource at +8. The known
widget lifecycle witness at 0x2a8e7a..0x2a8e8b passes widget+0x5a8, its filename
and false. Loader body 0x8589ad..0x858a41 saves those inputs, releases prior
resources through 0x79e310/0x79d200, clears both pointers, selects HD/classic via
0x846210 and stores the loaded resource into +0/+8. These are static witnesses
from the same retained analysis image/hash as above; beta.6 loading remains
live-unverified. The lower resource-loading branch contains a capture-specific
redirect; no new direct call to that lower branch is adopted.

New loader admission windows, within the already exact executable/Core profile:

- Game 0x858990 entry: `40 53 56 57 48 81 EC 40 01 00 00`.
- Game 0x8589ba body, beyond Quality Backgrounds' displaced window:
  `E8 51 59 F4 FF 48 8B 4B 08 E8 38 48 F4 FF 33 C0 48 89 03 48 89 43 08`.

The body guard must pass. The entry must either be pristine or resolve through
at most three E9 rel32 / FF25 absolute-inline jump stubs to the known installed
Quality Backgrounds DLL's RVA 0xc8a0. The latter requires that module's exact
previously recorded F3B6...61BA file SHA and its mapped entry bytes
`40 53 48 83 EC 20 45 0F B6 D8 4C 8B D2 48 8B D9`. Bounded reads only inspect
the stubs; no unknown destination is invoked. Unknown companions or patch shapes
refuse the optional ring and keep the dim native sprite. No pristine bytes are
written, no predecessor is bypassed, and no hook is installed at this loader.

Additional static Quality Backgrounds witnesses from that exact installed DLL:
registration at 0x130d9 uses game 0x858990 and 0x130f8 binds target 0xc8a0;
0xc8a0..0xc917 preserves wrapper/name and forwards through its original-function
slot at plugin 0x6c178. It changes the boolean option only for a filename
containing `quality-background` (literal at plugin 0x18838). Our unique filename
does not match, so it forwards the supplied false value and original arguments.
This is an adopted bounded predecessor contract, not general hook chaining or a
claim of live beta.6 coexistence. Existing Quality icon caller semantics remain
unchanged. Other entry changes are feature-local refusal.

The existing circle gateway now accepts C++'s returned sprite pointer in saved
RCX. It still restores the original RAX, other volatile GPR/SIMD registers,
flags, R8 settings pointer and R13 occupancy predicate before the original CALL.
Only empty, admitted HD socket draws can select the owned outline wrapper and
set that draw's alpha to 1. The original ring is never made opaque. Read both
sprites' native dimensions and pivots; require square bounded dimensions and
fit the owned ring to the original circle width times the shared size percent,
preserving the original center within half a pixel. Non-square/invalid geometry
falls back to ordinary centered native-circle scaling. No separate size setting.

Resource registrations roll back on partial failure and unregister on plugin
unload. One lazy native wrapper/reference per drawing thread is intentionally
retained to process exit so queued renderer commands never see a freed texture;
there is no per-item/per-frame resource allocation. This plugin already requires
restart for replacement due to retained native chat callbacks. No new hot-reload
or native resource-release/thread-affinity contract is claimed.

Release build and four automated suites pass. Pixel checks establish an entirely
clear inner disk, opaque rim, antialiased edge and exact sprite header/payload;
fitting checks preserve centers at 10/65/100 percent and UI scales 0.5/1/2/4;
service fixtures cover owned paths, both variants and partial-registration
rollback. The executable circle fixture checks intentional RCX replacement,
retained native arguments/registers/flags and unavailable-asset fallback without
raising native alpha. Existing inserted-art, loose-item, command and artifact
checks pass. Visible native loading, rim styling, clipping and combined beta.6
Quality Backgrounds behavior await the user's live test. This is local only.

## beta.7 original D2R overlay and circle-size margin — 2026-10-15

The user screenshots show beta.6's procedural empty ring and inserted artwork
covering the filled circle. They explicitly choose the original D2R overlay with
a faint center. Read-only inspection of the active D2RLoader process 8880 finds
the global beta.6 socket DLL loaded. Its log records admission at 06:21:45.373,
owned-outline execution at 06:22:00.682, circle/art scaling and inserted opacity
immediately afterward, and a session size change to 75% at 06:23:20.760. This
establishes that the custom asset loaded/displayed, not acceptance of its look
or comprehensive Quality Backgrounds coverage.

Beta.7 removes the procedural sprite generator, both resource registrations,
per-thread native wrapper, loader calls and Quality loader-detour recognition.
The historical beta.6 loader/path/ABI witnesses above remain research records;
none of those new asset paths or loader contracts are required by beta.7.
Every circle draw uses the original native sprite argument again. The gateway
restores original RCX rather than substituting C++'s return value, while keeping
its existing R13 native occupancy predicate, scale/anchor references and settings
pointer. The four existing mutation sites/guards are unchanged; no new RVA,
offset, native call, asset override or hook is introduced.

Inside the admitted enabled HD socket scope, an empty native circle with finite
alpha in [0,1] gets 0.35 draw opacity. This value applies to the whole original
sprite, including both rim and dark center; the two cannot be assigned separate
alpha values by this scalar input. It is an explicit visual compromise selected
by the user after the opaque-black beta.5 and clear-center beta.6 experiments.
Filled circles retain the current native alpha (normally 0.25). Inserted opacity
remains independent, and disabled/classic/unadmitted paths preserve native alpha.

Circle percent is min(artwork percent+10,100), using the existing 10–100 artwork
setting and bounded arithmetic. Thus artwork 65/75/90/100 gives circles
75/85/100/100. The existing center-preserving geometry applies to the original
sprite's native dimensions and pivots, including non-square bounds. Socket
positions/spacing are not moved. Status reports both derived percentages; no
additional configuration key or size command is added. Invalid internal percent
fails open to 100 without unsigned overflow.

Release build and three suites pass. Real assembly fixtures check that both
filled and empty draws forward the original sprite/settings pointers and retain
the native registers/flags/occupancy predicate; empty alpha is 0.35, filled alpha
is unchanged, invalid alpha passes through, and the circle margin/cap preserve
centers. Existing inserted-only size/opacity, loose-item, config/command and DLL
admission/refusal checks still pass. The custom sprite pixel/resource suite is
retired with its implementation. Beta.7's actual appearance, room around jewels
and combined Quality Backgrounds behavior require a new live test. Local only.

The local beta.7 install was completed with the game closed, built/deployed
hashes matching and a verified beta.6 rollback. Configuration and companion
inventories were unchanged. At this install Quality Backgrounds was already
renamed to `.disabled`; that state was preserved, so this deployment does not
provide combined beta.7 runtime coverage.

## beta.8 shaded custom rings and paired sprite centers — 2026-10-15

The user supplies beta.7 screenshots of a filled helm, empty/filled armor and a
six-rune weapon, reporting circles above the rune bodies and no sufficiently
visible dark center. They request the earlier procedural socket design with a
black center at 20% opacity. Process 30292 was observed loading the global socket
DLL and Core; the log records beta.7 admission at 06:37:00.576 and empty-circle,
circle/art scaling and opacity execution at 06:38:37.747..760. This establishes
execution, not accepted visual alignment. Quality Backgrounds was disabled.

Beta.8 reuses beta.6's two owned resource paths, RGBA8 SpA1 construction, native
sprite-wrapper ABI, loader body/entry guards and exact known Quality Backgrounds
forwarding contract. No guard is weakened or new native hook added. The custom
ring now replaces both empty and filled circle draws. The draw's scalar alpha
is 1 while the generated texture itself has opaque rim pixels, black inner-disk
pixels RGBA(0,0,0,51) (exactly 20% alpha), antialiased transitions and a clear
exterior. This keeps center shading independent of inserted-art opacity. A
failed resource registration/load, guard, mode, alpha or geometry check retains
the original native sprite/alpha and existing centered-circle size behavior.
The beta.7 native-empty-alpha override is removed. Texture fitting and its
failure paths mutate anchor/scale atomically.

Additional static route witness from the same retained analysis image/hash:
game 0x15bcd0..0x15bce8 forwards the selected inserted sprite in RDX, packed
anchor from RBX in R8 and scale in XMM3 into 0xc7020. The builder stores the
packed anchor at its RSP+0x28 at 0xc7084, then forwards that point to 0x858510
at the existing 0xc71e3 adapter. Thus its center depends on its own dimensions
and pivots, separately from the preceding circle's rectangle. Static inspection
does not establish the actual rune-specific offset or visible ink/transparent
padding; no per-rune offset is guessed.

Use the already-proven synchronous order in 0x2a76ea..0x2a7779: draw the circle,
test R13, and only for a filled slot call the inserted image path before drawing
the next circle. After a custom ring is successfully fitted, copy its actual
rendered geometric center into a thread-local pending record only when R13 is
nonzero. The following admitted item-image adapter consumes that record once,
scales the inserted sprite by the existing artwork percent, and computes its
anchor from the copied circle center and its own dimensions/pivots. It rounds
only the final anchor. Circle sizing remains artwork+10 points, capped at 100.
Native socket positions/spacing and circle centers remain fixed.

The pending record contains only two doubles and validity, with no native item,
sprite or iterator pointer retained. Each circle clears old pairing; empty and
fallback circles never establish it. SocketDrawScope saves/restores the prior
record and clears it on entry, isolating nested draws; scope exit prevents a
later loose draw from consuming it. Missing/invalid pairs use the prior
independent center-preserving artwork scale. Up to eight geometry-only log
samples report native sprite dimensions and the measured center correction to
support the live check without per-frame log growth. Actual visible art padding
may still require further evidence after testing; matching sprite rectangles
does not itself prove every rune's visible outline is centered.

The circle gateway again accepts the owned sprite returned in RAX as its RCX
argument, restores original RAX and the other native state, then uses the
unchanged circle trampoline. Resource rollback/unload and process-lifetime
native-wrapper retention follow the beta.6 contract. Quality Backgrounds' icon
caller semantics and its hooks are preserved; no general chaining is assumed.

Release build and four suites pass. The actual C++-generated sprite was decoded
for visual inspection, with center RGBA(0,0,0,51); both raster variants have an
opaque rim and transparent exterior. Assembly fixtures check filled/empty owned
sprite forwarding, exact argument/register/flag preservation, a non-square rune
matching its circle center, no target reuse, nested isolation, disabled/classic
and failed-asset passthrough. Geometry tests cover varied pivots, several UI
scales, size caps and atomic failure. Existing loose-item, relative opacity,
config/command and DLL admission checks pass. Beta.8 native loading, visible rune
alignment and combined Quality Backgrounds behavior still need live validation.

The armor screenshot additionally shows occupied and empty positions together on
one item. Beta.8 was installed locally with the game closed and built/deployed
hashes matching; the prior beta.7 DLL/log were retained for rollback. Configuration
and companion inventories were unchanged, including Quality Backgrounds' existing
disabled state. No new release was published.

## beta.9 independent native-layer opacity and +15 size margin — 2026-10-15

The user confirms beta.8 alignment looks good, requests a +15-point circle-size
margin and 35% black center, and asks for independent rim/center opacity. The
beta.8 log records native admission at 06:49:56.330 and paired-center execution
at 06:50:10.998..06:50:11.019. Measured geometric corrections include
100x100 sprites (-1.62,-0.62 pixels) and 98x98 sprites (-1.12,-0.12). This,
together with the user's visible confirmation, accepts the paired alignment;
it does not establish a specific rune type from dimensions or new companion
coverage. No alignment algorithm or insertion-order contract is changed.

Circle percent is now min(artwork+15,100): artwork 65/75/85/100 gives circles
80/90/100/100. Default rim opacity is 100 and center opacity is 35. Add session
commands `/sockets rim N` and `/sockets center N` (0–100), independent of each
other and existing inserted-art opacity. Saved TOML keys are
`ring_rim_opacity_percent` and `ring_center_opacity_percent`. Old files missing
the keys receive defaults without rewriting the user's configuration.

Separate the generated texture into a rim-only layer (clear center) and an
opaque black disk. Keep the rim's two established virtual filenames; add owned
center paths:

- `data/hd/global/ui/d2rloader/always-show-sockets/socket-center.sprite`
- `data/hd/global/ui/d2rloader/always-show-sockets/socket-center.lowend.sprite`

Four resource registrations and two lazy native wrappers are bounded per load/
drawing thread. All registrations roll back on partial failure. Both wrappers
retain the established process-lifetime reference contract; no new texture or
resource registration occurs on opacity changes. Both native bounds/pivots must
match before using the layered path.

The added center pass directly calls the existing sprite wrapper renderer at
game 0x858510, with adopted Win64 ABI
`void(spriteWrapper*, int32Point*, spriteSettings*, floatScale)` (RCX, RDX, R8,
XMM3). The original intercepted circle CALL remains the rim pass, then the native
loop draws inserted artwork as before. This adds no native hook and does not
use a late UI overlay. Entry guard at 0x858510:
`40 55 53 57 41 56 48 8D 6C 24 D8 48 81 EC 28 01 00 00`.
The exact executable/Core profile, HD mode and previous guards remain required.
An unknown entry detour refuses the optional layered drawing.

New settings-bound witness from the same retained analysis image/hash: HD
0x79da50 uses settings fields through +0x77, including the qword load at
0x79dc3e (`F2 44 0F 10 56 70`), which is an additional admission guard. Other
reads include RGB/tint +0..0x0c, opacity +0x10, scalar/frame fields +0x18/0x1d/
0x1e, optional records +0x30..0x57, flags +0x58/0x60 and parameters +0x5c/
0x60..0x77. The zero-record path in helper 0x79edd0 examines four dwords of
the nested record at settings+0x20. The adopted 0x78-byte copied range covers
these HD reads; it is not a claim about the full native settings structure or
classic-mode ABI. At the circle site the existing alpha pointer is original
RSP+0x70; settings begins 0x10 bytes before it, at original RSP+0x60.

Read that bounded range into 16-byte-aligned owned stack storage and change
only copied +0x10 to centerPercent/100. Call 0x858510 with that copy and a private
copy of the fitted point. Skip the extra draw at center zero. The pending native
rim CALL keeps the original settings pointer and uses rimPercent/100. Its
sprite/point/scale and the center layer have the same fitted geometry. The
inserted sprite still pairs to the rim's copied center, even if both layer
opacities are zero. The global opacity values are bounded atomics; neither
command modifies assets, item flags, pivots, shared opacity state or resources.
Native sprites/alpha and independent artwork-centering fallback remain available
if layer resources, bounds, alpha, settings reads or direct-call guards fail.

Release build and four suites pass. The real circle assembly fixture includes
the additional call, native-equivalent alignment and a complete settings input
range; it verifies GPR/SIMD/flags, sprite/settings pointers and original tint
survive. The fake center renderer deliberately mutates its private point/settings
copy to prove isolation. Tests verify independent 40% rim/70% center, default
100%/35%, 0/100 commands, zero-draw skip with retained alignment, four-resource
rollback and old/new/invalid config parsing. Existing loose-item, geometry,
opacity and DLL-admission checks pass. Native beta.9 layered rendering, visible
opacity adjustments and combined Quality Backgrounds behavior require a new
live test. The current companion remained disabled at initial inspection.

The generated rim/disk layers were decoded and composited at the requested
100%/35% defaults for visual inspection. Beta.9 was installed locally with the
game closed and matching built/deployed hashes; the accepted beta.8 DLL and log
were backed up. Existing configuration and companion/disabled-plugin inventories
were unchanged. The old size-only config receives the new opacity defaults
without rewriting the file. No release was published.

## beta.10 ring terminology and adjustable optical lift — 2026-10-15

The user accepts beta.9's appearance and requests `ring` instead of `rim` in
the controls. Canonical session command is now `/sockets ring N`, with saved
`ring_opacity_percent`; the old command/key remain compatibility aliases.
Canonical saved spelling wins regardless of key order. Status, help and new
logs use `ring`. Historical native/resource names remain unchanged.

The user explicitly chooses a slight upward adjustment of inserted artwork,
leaving the ring and black center fixed. Screenshot inspection suggests about
1–2 rendered pixels; this is an optical estimate, not a newly discovered native
offset or proof for every gem/rune/jewel. Default `inserted_item_lift_pixels`
is 1; `/sockets lift N` accepts whole pixels 0–8 for direct visual comparison.
Zero restores beta.9's geometric pairing. Commands apply for the session;
the saved key is read on restart and old configurations receive the default
without being rewritten.

After consuming a valid pending circle center, subtract the lift from its copied
Y coordinate before the existing ScaleAtCenter calculation. Preserve X, current
native sprite scale and metadata contracts; round only the final int32 anchor.
This is a rendered-pixel correction, independent of artwork percentage/UI scale.
Only the paired inserted sprite receives it. Ring/center geometry and opacity,
loose items, unpaired fallback, nested-scope isolation and native admission are
unchanged. No new address, byte guard, hook, resource or item-type rule is added.

Release build and four suites pass. The executable native adapters verify
command-driven lifts of 0/1/2/8 pixels on non-square paired artwork while both
circle layers remain stationary; existing loose/unpaired/classic/disabled
passthrough still passes. Tests cover bounded saved/session values, missing-key
defaults, canonical/legacy naming and geometry across UI scales. The beta.9 log
records native layered passes and opacity controls, and the user accepts their
appearance. The beta.10 optical correction still needs visible live comparison.
Quality Backgrounds is still disabled, so no new combined live coverage is
claimed. Its known loader admission and icon hook contracts are unchanged.

Beta.10 was installed locally with the game closed after the four suites passed;
built and deployed hashes match. The beta.9 DLL and log were backed up, and
configuration, other plugins and disabled companions were unchanged. The local
receipt is `outputs/always-show-sockets-ring-alignment-deployment.json`. No
release was published and no beta.10 live appearance confirmation is claimed.

## beta.11 slash-command registration repair — 2026-10-15

The user reports no visible difference between lift 0 and 8. The beta.10 runtime
log at 07:20:01.820 records successful chat getter/registration/caller resolution
(0x2E6550/0xA281D0/0x2E87C5), immediately followed by `native chat registration
unavailable` at 07:20:01.822. There are native paired-art draws, but no command
state-change records or native `/sockets` registration result. The installed DLL
is beta.10. This points to registration refusal rather than invisible geometry.

The exact beta.10 description is 72 ASCII bytes. The vendored backend's
NativeCommands::Register rejects descriptions above 63 bytes before reserving a
slot or queuing native registration. The already-recorded native Definition ABI
has description[64] at +0x44, including the NUL terminator. This is an existing
bounded contract, not a new RVA or a reason to enlarge the native record.
The earlier socket test called ChatCommand directly and therefore missed it.

Beta.11 uses a shared shorter help literal, with sizeof including NUL asserted
at compile time to be <=64. Log the backend Reason() on setup refusal. Preserve
native admission, UI scheduling, callback lifetime, render hooks and lift math.
No vendored backend behavior or native patch is changed.

The command fixture now includes the actual vendored backend implementation
with test-only resolved getter/registration entries, following the existing
Floating Damage fixture pattern. It calls the same socket registration helper
as production, verifies pending UI work and native result 0, then invokes the
retained native execute thunk with lift 0/1/2/8 through to the actual scale and
circle assembly adapters. Artwork moves by the requested rendered pixels while
ring/center stay fixed. The backend still rejects a 64-byte description.
Production still resolves all native entries normally; no test injection API is
added to the shipped DLL. Release build and all four suites pass. Real beta.11
game registration, command dispatch and appearance require a fresh live test;
the simulated registration result is not claimed as runtime game evidence.

Beta.11 was installed with the game closed and matching built/deployed hashes.
The prior beta.10 DLL and log were backed up; configuration and other/disabled
plugin inventories were unchanged. Deployment receipt:
`outputs/always-show-sockets-chat-registration-deployment.json`. No release
was published; the next game start is the required live registration/lift test.

### beta.11 user confirmation — 2026-10-15

The user confirms "Lift is working" and "1 looks the best." This establishes
user-confirmed live lift-command behavior and acceptance of the 1-pixel optical
correction. Retain DefaultInsertedLiftPixels=1 and the embedded
inserted_item_lift_pixels=1; no code, default, DLL or configuration change is
needed. This confirmation adds no new panel, resolution or Quality Backgrounds
coexistence coverage beyond the previously recorded checks.

## beta.12 full-range ring-size margin — 2026-10-15

The user reports rune artwork spilling outside the ring at `/sockets size 100`.
The beta.11 log records artwork 100/circles 100 at 07:27:25.392, artwork 90/
circles 100 at 07:28:32.915, and artwork 85/circles 100 at 07:28:36.498.
This confirms the old min(artwork+15,100) policy stops ring growth at artwork
size 85. The startup settings remain artwork 65/circles 80; the user has not
requested a default change. Native size is each sprite's own 100% baseline,
not a promise that different sprites have equal visible widths.

Use artwork+15 for all admitted artwork percentages 10–100, giving custom
circles 25–115%. Bound the owned-layer fitter to 115, multiply its fitted native
scale by the ring percentage, then use the existing center placement at 100%.
Keep ordinary inserted-art percentage limits unchanged and validate the final
native scale against the existing finite/positive/16 bound before mutation.
The native-circle fallback keeps its former at-most-100% sizing. Ring/center
share fitted geometry; existing paired-art placement and the accepted 1-pixel
lift are retained. No new native address, byte guard, asset path, ABI or hook is
introduced, and no Quality Backgrounds admission contract is changed.

Release build and all four suites pass. Through actual retained chat dispatch,
size 85/90/100 produces circle 100/105/115 in both empty/filled assembly paths;
the two layers share scale/anchor and preserve the native center. Filled rune
art scales only to its requested percentage and retains the 1-pixel lift.
Geometry checks include 105/110/115 across UI scales, >115 refusal, native-scale
overflow refusal and unchanged ordinary-art bounds. Native chat registration
and lift are already user-confirmed in beta.11; beta.12's larger-ring appearance
still needs live validation. Quality Backgrounds remains disabled at inspection.

Beta.12 was installed after the game closed; built and deployed hashes match.
The beta.11 DLL/log were backed up, and all existing configuration and other/
disabled plugin inventories were unchanged. Receipt:
`outputs/always-show-sockets-full-size-ring-deployment.json`. No release was
published. Check `/sockets size 100` after restarting for visible ring clearance.

## beta.13 default/always/off appearance modes — 2026-10-15

The user confirms beta.12 works great, then requests the same appearance controls
without forcing sockets visible. They select mode commands `/sockets always`
and `/sockets default` and explicitly request `default` as the startup mode.
The beta.12 log at 07:33:29.028 records artwork 100/circles 115, followed by
65/80 at 07:33:35.870. This adds user-confirmed acceptance of the large ring,
not new Quality Backgrounds coverage.

Keep appearance enablement separate from forced visibility. Default mode enables
styling but disables post-icon augmentation; the original selected socket paths
at game 0x2C3731 and 0x2C9A82 still enter the existing socket hook at 0x2A73C0.
DrawSockets scopes the original renderer in both native and forced paths, so
the existing 0xC71E3 artwork adapter and 0x2A76EA circle adapter see the same
appearance enablement and copied-center pairing. Native hover/selection gates
are not bypassed in default mode. Always mode additionally draws from existing
post-icon 0x2A73A0 and keeps matching-duplicate suppression. Off disables all
appearance changes and augmentation while forwarding native draws normally.
These addresses/contracts were already recorded; no new RVA, ABI, byte guard,
native state field, hook or asset is introduced. Quality Backgrounds' established
non-overlapping icon contract and guarded loader admission remain unchanged.

`always` and `default` are idempotent selections; `off` restores fully native
rendering. Existing `on` aliases always. Bare/toggle disables or resumes the
previously selected styled policy, and status reports the current mode.
Appearance values survive mode changes; each process starts in default mode.
No saved mode key is added or existing config rewritten. All size/opacity/ring/
center/lift settings, defaults and bounds remain as in beta.12.

Release build and four suites pass. Retained native chat dispatch selects each
mode, then a test original socket renderer invokes the real circle/image
assembly adapters inside SocketHook/DrawSockets. Empty and filled cases verify
no forced draw in default/off, one forced draw plus duplicate suppression in
always, shared circle/center/art size and opacity, paired lift, original native
geometry/alpha in off, unchanged loose draws, scope/pair cleanup and setting
retention. Tests cover the startup policy, idempotence, toggle resumption, legacy
on and unsupported-build refusal. These are simulated native paths; beta.13's
visible default-hover behavior and live mode transitions still need validation.

Beta.13 was installed after the game closed, with matching built/deployed
hashes and a verified beta.12 DLL/log backup. Configuration and all other/
disabled plugin inventories were unchanged. Receipt:
`outputs/always-show-sockets-display-modes-deployment.json`. Startup mode is
default; no release was published and live mode validation remains pending.

## beta.14 accepted modes, requested defaults and local staging — 2026-10-15

The user confirms beta.13 works and requests ring opacity 80, center opacity 40
and artwork size 65. The log records off/always/default transitions at
08:11:08.298..08:11:44.632, ring 80/center 40 at 08:12:31.932 and final artwork
65/circles 80 with those opacity values at 08:13:51.331. This establishes live
mode/control behavior and the chosen appearance on beta.13. Beta.14 changes only
the embedded/default fallback values, version and distribution documentation;
no native address, byte guard, resource, hook, ABI or rendering calculation changes.

Keep startup default mode, inserted opacity 65 and lift 1. Explicit saved values
remain authoritative; old size-only configs receive the new missing-key defaults
without file rewriting. Add a public quick configuration guide and ship the
reference TOML under defaults/ only, never under d2rloader/config/. The runtime
archive remains limited to this plugin's DLL and public documentation/licenses.

Four native suites and six packaging regressions pass. The new DLL/embedded
defaults are checked automatically; beta.14 is not claimed as separately
live-tested. Stage runtime/source artifacts, post-ready guide, release notes and
checksums under outputs/release-staging/always-show-sockets/0.1.0-beta.14/ for the
user's later combined release. The staging manifest records exact artifact and
source hashes plus validation limits. Do not publish, tag, commit unrelated work
or replace the running test DLL as part of this staging request.

## beta.15 automatic appearance persistence — 2026-10-15

The user requests automatic saving for all appearance commands, removes the
obsolete outline alias/config compatibility wording and keeps default as the
startup visibility mode. Save size, inserted opacity, ring opacity, center
opacity and lift through the existing SDK config API. Source contract is
third_party/D2RLoader-PluginSDK/include/D2RLPlugin/context.h: ReadConfigFn is
bool(__cdecl*)(const PluginContext*,char*,uint32_t,uint32_t*) noexcept;
WriteConfigFn is bool(__cdecl*)(const PluginContext*,const char*) noexcept.
The context selects this plugin's own config; no game path or native RVA is
hardcoded for saving, and no SDK/provider table is patched.

After a validated supported appearance command updates its atomic session value,
SaveAppearance serializes this plugin's read/update/write operations with a mutex.
Read at most 4096 bytes including the terminator, reject unread/oversized or
unterminated input and oversized output, and update only the requested key in
the always-show-sockets section. Preserve other text, comments, BOM, CRLF/LF,
other keys and other sections. Insert a missing key inside the section or append
the section if absent. Call WriteConfig with owned NUL-terminated text; log saved
only on SDK success. Catch allocation/API failures, leave the current session
value active, warn in the console/log and return command Failed. No unread or
oversized file is replaced as a fallback.

Mode/status/invalid/unsupported commands perform no config IO. Startup still
selects styled default mode each process, independently of saved appearance.
Only ring is accepted for the outline command/key; the previous aliases are
removed from parsing and public configuration guidance. Native hook sites,
bytes/ABI guards, resources, rendering math and Quality Backgrounds contracts
remain unchanged. The default values stay 65/80/40 for size/ring/center,
65 inserted opacity and lift 1.

Four native suites pass, including real retained chat dispatch to SDK read/write
fixtures, all five persisted values reparsed by the startup parser, rejected
outline alias, no mode/status/invalid/unsupported writes, round-trip BOM/comment/
newline/section preservation and read/write/oversized-config failure behavior.
These fixtures establish SDK call/text behavior, not live physical-disk saving
by the actual loader. Stage beta.15 runtime/source/guide/notes/checksums for the
combined release; keep the installed beta.13 test DLL untouched and make no tag,
commit or publication. Exact artifact/source hashes are in the new staging
manifest under outputs/release-staging/always-show-sockets/0.1.0-beta.15/.

## Stable 1.3.1+rev.1 promotion — 2026-10-15

The user requests promotion out of beta and specifies the repository's
D2RLoader-version + plugin-revision naming scheme. This is the first stable
Always Show Sockets revision: 1.3.1+rev.1, with Windows numeric file/product
versions 1,3,1,1. PluginInfo, resource strings, CMake version, logs, public docs
and runtime/source archive names agree. The revision belongs to this plugin;
other plugins' revision counters are unchanged.

Keep the beta.15 implementation unchanged: all five appearance controls save,
the outline alias is removed, and the user's explicit default-startup mode is
retained. The package accepts stable loader+revision versions and historical
beta versions but rejects version/tag/channel mismatches. The compatibility
document identifies D2RLoader 1.3.1 in addition to ABI 4 and the same exact
executable/Core hashes. A matching version label does not replace native hash,
byte, ABI or resource guards. No native addresses/contracts or runtime behavior
change in this promotion.

Stage the stable package for the pending combined release, superseding beta.15;
do not publish a separate standalone release or alter the installed test DLL.
The stable staging manifest records build/package/source checks and hashes under
outputs/release-staging/always-show-sockets/1.3.1+rev.1/. Earlier rendering/mode
live confirmations remain valid at their documented scope; actual loader-backed
autosave/restart validation and later Quality Backgrounds coverage remain pending.
