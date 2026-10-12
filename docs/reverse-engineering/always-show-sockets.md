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
