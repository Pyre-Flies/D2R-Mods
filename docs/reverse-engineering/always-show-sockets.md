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
