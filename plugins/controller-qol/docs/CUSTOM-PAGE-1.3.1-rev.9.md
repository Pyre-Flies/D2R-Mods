# Custom-page quick transfer - 1.3.1+rev.9 candidate

## Scope and evidence

User confirmed rev.8 fixes Charm Inventory controller menu navigation, and mouse
Ctrl+click transfers in both directions. Rev.9 adds LB+X using that loader operation.
It does not hook, patch, or send simulated keyboard input. It does not hard-code a
Charm Inventory panel name, page handle or charm whitelist. This is still a PRIVATE
build-specific bridge, not an SDK guarantee for all custom panels.

Target D2RCore.dll SHA256:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Global installation: `<game>/d2rloader/plugins`.
Charm DLL SHA256 `B14F6A0BD4C9DF0AF9C392EA97DC879AD3760592481517DBDB0983595145D88B`.
Read-only live verification used D2RLoader PID7828, game base 0x140000000 and
Core base 0xC0DE5000000. Addresses below are RVAs unless explicitly stated.
All generated witnesses were compared against live memory; see
src/custom_page_profile.h. Live pointers are
session examples only and must NEVER become constants.

## Public SDK investigation

SDK inventory.h exposes plugin-owned page handles and cursor/page operations,
not a generic quick-transfer action to another plugin's active page. item.h's
ItemDestination can name CustomPage but does not discover the active foreign
page handle. PanelService binds plugin-owned grids. Widget handles are paths,
not native pointers. ItemInteraction is an event, not a transfer command.
PluginSDK README lines 657-660 documents Core-owned Ctrl+click policy checking,
free-space placement and rollback, with no item-pointer/quick-move callback to
page plugins. Therefore reuse the existing loader operation rather than infer
Charm's storage layout or issue homemade item/network transactions.

## Recovery chain and private contracts

| Module/RVA | Meaning and recovered ABI |
|---|---|
| Core 0x8150A0 | Grid activation wrapper: item lookup and ItemInteraction dispatch, then 0x45C630, then native fallback through slot 0x70E9C8. |
| Core 0x45C630 | Custom page click handler, RCX grid, RDX packed x/y. Queries Ctrl through slot 0x6FE398. This wrapper is documented but not called or patched by QOL. |
| Core 0x45C140 | uint8 classify(grid, Binding*). 0 unrelated; 1 custom grid; 2 inventory with visible registered custom grid; 3 protected/unbound page6. |
| Core 0x4617A0 | Copies enabled state2 grid registrations under lock; registry shared_ptr vector begin/end globals 0x7DEDC8/0x7DEDD0. Read-only research only. |
| Core 0x461900 -> 0x461940 | Resolves registration panel/grid, requires panel visibility, immediate child name, parent identity and expected grid vtable. |
| Core 0x41F6C0 | uint32 snapshot(Binding-owner-context*, pageHandle, Snapshot*). Validates page ownership and active registry state; success0. |
| Core 0x461B10 | bool isNormalInventory(grid, player). Validates page0, unit identity/type and grid name. |
| Core 0x45FA00 | uint32 validateCustomGrid(grid, Snapshot*), success0. |
| Core 0x41FF40 | uint32 quickMove(owner-context*, uint64 page, bool fromCustom, int32 x, int32 y). fromCustom=false => operation3; true => operation4. Both branches seen at 0x45C861 and 0x45C7D6. |
| Core 0x41F9C0 | Existing request path selected by 0x41FF40 for its client mode. Not invoked independently. |
| Core 0x31A570 | Existing local operation target from 0x41FF40. Not invoked independently. |
| Core 0x70E968 | Native grid-item lookup pointer, live game+0x2C49F0; admission requires this exact link. |
| Game 0x846170 | FindTopLevelPanelByName; global manager pointer 0x3440170. |
| Game 0x856220 | Find child widget by name. Resolve normal PlayerInventoryExpansionLayout (fallback Original) then grid. |
| Game 0x2C49F0 | Item* lookup(grid, const Cell*). IMPORTANT: its second argument is a POINTER to two int32s, unlike packed-coordinate 0x45C630. |
| Game 0x34A330 / 0x36EF50 | Item runtime ID / code, used to match source cell against fresh SDK ItemInfo. |

Binding is a 0x30-byte caller-owned temporary: owner context bytes0..0x17,
page handle+0x18, custom grid pointer+0x20, usable byte+0x28. Context is copied
from the loader registration; it is not a fabricated QOL-owned page handle.
Snapshot is {player pointer, uint32 width, uint32 height}, size0x10.
Native grid: width+0x628, height+0x62C, inventory page byte+0x630;
normal inventory page0, custom page6. Panel visibility uses bytes+0x50 & +0x51.
All resolution and transfer occur synchronously in one SDK UI-thread task.
No borrowed native context, player or grid pointer is retained across callbacks.

Live registry showed two bindings, both page1, owner identity shared:
`charm-inv/CharmInvPanel` and `charm-inv/CharmInvControllerPanel`, both grid
`charm-inv/InventoryGrid`. Registration fields: state+0x28, panel string+0x50,
page+0x70, grid name+0x78, enabled+0xA0. String fields use MSVC string layout.
Live controller grid 0x349BC7278 was 10x4/page6/playerID1. Normal inventory
0x349B3B588 was 10x8/page0/playerID1. The mouse grid existed too but was not
bound to the same live player state. Do not pick grids by name alone.

Manager's primary lookup lists at +0x88/+0x90 and +0xA0/+0xA8 were empty in
this session; actual panels were in child vector +0x58/count+0x60. Native lookup
0x89F760 falls back to 0x856220, so an empty primary list is not missing UI.
Research enumerated this tree read-only; production uses native lookup.

## Guarding and cooperation

Exact on-disk Core hash plus full live direct-callee Core witnesses, game entry
witnesses and native lookup slot identity are required. Recheck before mutation;
on drift disable this bridge only. No added entry hook, vtable patch, imported
pointer change, physical Ctrl spoof or Charm-specific destination override.
The existing rev.8 menu routing fix remains intact.

The currently bound loader page is authoritative. Stale (>1s) focus refuses an
action; source cell ID/code must match the SDK item. Native page snapshot, local
player association, source bounds and custom grid checks precede quickMove.
Once a custom page claims an action, rejection MUST NOT fall through to personal
stash or another destination. Unknown/unavailable CustomPage sources are consumed.
Core performs its original policy/free-space/local-or-request handling; QOL does
not interpret success as proof of publication, visible output or persistence.

Limit: direct 0x41FF40 reuse does not replay the outer mouse ItemInteraction
notification wrapper at 0x8150A0. It preserves the registered page item policy;
it does not promise that arbitrary third-party mouse event interceptors see
this controller command. An SDK high-level quickMoveFocusedItem/action dispatch
API remains preferable and should include event/policy semantics.

## Rejected paths / update procedure

Do not call cursor move 0x41F810, spoof global Ctrl, or patch the Ctrl query.
0x430860 dispatches interaction callbacks, not a transfer operation; 0x326C10 is
text clipboard handling and 0x431D30 belt activation. These are not substitutes.
A historical captured game CALL at 0x2C4A3C targeted thunk0x3E2B1A6; current
live code targets0x3E2B26E. Loader-generated thunk placement is not stable, so
use entry witnesses and validate the lookup's returned identity; never pin that
historical CALL displacement as an ABI promise.

After Core update: locate custom grid activation and Ctrl branch; recover its
binding classifier, snapshot and quickMove arguments; re-check operation3/4
routing, ownership checks, layouts and current grid lookup slot. Verify bytes
on the active build and document new witnesses before changing the hash guard.

## Validation status

Static ABI and live read-only registry/grid/witness inspection completed.
No live items were mutated during research. Existing regression suites pass;
new absent-provider tests exercise refusal/fallback through the actual bridge.
Candidate runtime transfers, item policy refusal, full-grid behavior and save
persistence remain unverified until user tests after installation.

Test both directions with a charm, reject a non-charm, reject full destination,
close/reopen panel and rejoin the game. Recheck ordinary stash/cube/materials
and Ctrl+click, with other installed plugins enabled. Do not promote to
production merely on a native request result of zero.

User subsequently confirmed the installed rev.9 transfer change worked. Individual rejection/full-grid/persistence scenarios were not separately reported.
