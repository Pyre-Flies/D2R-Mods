# Inventory Deposit All button investigation

Follow-up: beta.2 implements live gold-relative positioning. Its native setter,
geometry, build, installation and validation records are in
[GOLD-ANCHOR.md](GOLD-ANCHOR.md). Fixed-coordinate observations below describe
the beta.1 prototype and remain historical evidence.

Recorded 2026-10-11. Scope: add a mouse button in a separate `inventory-deposit`
plugin to the existing HD player inventory while the stash is open, reusing
the proven Controller QOL deposit operation through the integration selected
for this prototype.
The screenshot's requested location is the unused bottom strip to the right of
the gold display. This record separates existing contracts from proposed
placement and runtime behavior. The standalone prototype is implemented;
automated validation and the subsequent user-confirmed live result are recorded
below. Individual unverified behaviors remain identified separately.

## Supported extension mechanism

`third_party/D2RLoader-PluginSDK/README.md:942-975` documents
`PanelService::registerChildLayout`: it adds a plugin-owned widget to
`StockPanel::PlayerInventory` without replacing the stock inventory JSON.
`include/D2RLPlugin/panels.h:170-183` defines the registration. Register the
resource and child during plugin load, using a unique plugin-local `localId`,
zero `reserved`, and the `KeyboardMouseOnly` flag for this mouse button.

The child layout is a strict JSON resource under
`data/global/ui/layouts/inventory-deposit/`; its root name is the corresponding
`inventory-deposit/<localId>`. It may be an `ImageWidget`, `ButtonWidget`, or another
native widget with children. `ResourceService` can register the JSON from the
DLL, so a separate companion MPQ or modification of Reimagined's inventory
files is not required. `resources.h:39-46` states that resource paths and bytes
are copied by registration and defines the permitted namespace.

Without an input flag, injection covers HD original/expansion inventories and
their controller variants. `KeyboardMouseOnly` excludes controller layouts.
Legacy non-HD layouts are not supported by the documented child-layout API.
Layout changes require restarting the game: unregistering affects future
layout loads, while already-created native children can remain cached.
Shutdown invalidates callbacks and unregisters ownership; it does not make
widget calls from an unknown unload thread or claim to destroy cached widgets.
Only install/remove/reload this candidate with the game closed.

This operation does not register a custom inventory page or controller menu
route. The globally single-owner custom-page/controller-route restrictions
used by Charm Inventory therefore do not themselves exclude an extra inventory
button. A unique namespace avoids a logical-name collision. Simultaneous
rendering still needs live validation.

## Actual installed Charm Inventory arrow

Inspected the Reimagined installation, not an assumed source checkout:

- `<game-root>/mods/Reimagined/d2rloader/plugins/d2rl-charm-inv.dll`
  is 68,952 bytes, SHA-256
  `B14F6A0BD4C9DF0AF9C392EA97DC879AD3760592481517DBDB0983595145D88B`.
- Its companion `d2rl-charm-inv.mpq` in that directory is 9,591,921 bytes,
  SHA-256 `E01163AAC3B0D76C3CBDAD56E19350692864B51484D9AD6DFBC41F28023FB0AC`.
- Read-only extraction of
  `data/global/ui/layouts/charm-inv/InventoryButtonhd.json` produced SHA-256
  `57D4F476FC081415E9F64D1C7904C14423F6F76DA235450AECCEFA6B76C9FECF`.
  The temporary extracted file was removed; no proprietary asset is added to
  this repository.

That layout has root `ImageWidget` named `charm-inv/InventoryButton`, with
parent-local rectangle `(3, 672, 54, 141)` and resource reference
`d2rloader/charm-inv/button_mold`. Its `ButtonWidget` child is named
`charm-inv/ToggleButton`, positioned at `(1, 39)` within that root, using
`d2rloader/charm-inv/charminvbutton`, `hoveredFrame: 3`, tooltip `Charm Inventory`,
and action `PanelManager:TogglePanel:charm-inv/CharmInvPanel`.

This is direct on-disk evidence that the arrow uses a small namespaced child
layout, consistent with the SDK's exact Charm Inventory example. It occupies
the left edge of inventory, separate from the proposed bottom-right button.
Its DLL source/registration implementation was not available in the inspected
checkouts; no claim about an uninspected implementation is needed.

The installed root `D2RCore.dll` inspected on this date is file version
`1.3.1-beta`, 18,938,712 bytes, SHA-256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
These identities are inspection snapshots, not new exact-hash requirements for
the public service APIs. No live module enumeration was performed by this
investigation. Existing deposit compatibility guards remain authoritative for
the action itself.

## Geometry and existing artwork

Both the source checkout
`<reimagined-source-root>/data/global/ui/layouts/playerinventoryoriginallayouthd.json`
and installed
`mods/Reimagined/Reimagined.mpq/data/global/ui/layouts/playerinventoryoriginallayouthd.json`
have the relevant HD structure:

| Widget/field | Observed value |
| --- | --- |
| Root type/name | `PlayerInventoryPanel` / `PlayerInventoryOriginalLayout` |
| Root rectangle/anchor | `$RightPanelRectPI` / `$RightPanelAnchor` |
| Gold button | `(412, 1644)`, `PANEL\\GoldButton`, hovered frame 3 |
| Gold amount | `(492, 1656, 246, 48)` |
| Inventory grid | `(94, 818)`, 10 columns by 8 rows |

The expansion file is based on `PlayerInventoryOriginalLayoutHD.json` and names
its root `PlayerInventoryExpansionLayout`. Existing QOL lookup in
`plugins/controller-qol/src/custom_page_actions.cpp:35-42` uses the expansion name with original-name
fallback. WidgetService rectangles are parent-local (`widgets.h:43`), not
physical screenshot pixels.

The proposed `(800, 1644)` is a placement hypothesis inferred from the user's
highlight and those gold controls. It is not a discovered native constant.
Use a small square button and test its actual hitbox, native sprite dimensions,
scaling, ultrawide placement, gold overlap, and original/expansion panels in
game. Child injection inherits the parent composition; it does not need a
separate screen-anchored overlay.

The mod's `bankexpansionlayouthd.json:1016-1023` already uses the stock
`PANEL\\Horadric_Cube\\TransmuteButton` sprite with `hoveredFrame: 2`; it is a
possible prototype square-button visual. Reusing the sprite does not authorize
reusing its `BankPanelMessage:Convert` action. A deposit button needs its own
tooltip and its own consumed message. Native sprite dimensions and whether its
transmutation symbolism is sufficiently clear remain visual-design checks.

Potential separate-plugin overlap: the local maps prototype in
`<d2rl-plugins-root>/plugins/maps/src/map_stash.h:18-19` defaults its inventory button
to `(770, 1628)`, essentially the same bottom area. Its own custom-page feature
reports a conflict when Charm Inventory owns the only custom page
(`map_stash.cpp:1663-1667`); that does not establish all future installations
will be free of the button. Configuration or a separate placement is needed if
both buttons are active. Do not remove or reposition another plugin's widget.

## Click dispatch and stash visibility

Use an established native message target, for example
`PanelManager:OpenPanel:inventory-deposit/<unique-action-name>`, then have the
SharedEvents listener consume only that exact target, command, and payload
before normal panel handling. The payload is an action token, not an actual
panel that should be opened. Validate the action token before consuming it and
leave unrelated panel messages unchanged. Queue the existing deposit request;
do not perform authoritative item writes in the UI-message callback.

Do not invent an unregistered native message type such as
`ControllerQol:DepositAll`. Previous live Chat Tabs research in
`<d2rl-plugins-root>/docs/reverse-engineering/chatpanel-native-layout.md:46-55`
records native TypeDesc assertions for the unregistered `ChatTabs` message
target. A SharedEvents listener alone did not register a native UI channel.
The working convention is also present in
`<d2rl-plugins-root>/plugins/maps/src/map_stash.cpp:75-78,1403-1450`.

The public APIs required to control the new child are
`WidgetService::findPanel`, `findWidget`, `setWidgetVisible`, and
`setWidgetEnabled` (`widgets.h:56-66`). Widget handles store plugin-owned paths
and are re-resolved each call; they are not raw widget pointers. All widget
operations must run on the UI thread. Visibility/enabled setters are available,
but this API does not expose visibility queries, arbitrary child creation, or
rectangle setters.

Show the child only after guarded state confirms a supported open stash
context; enable it only when the deposit operation is also available. A held
cursor item keeps the child visible but disabled (beta.10). Re-check preconditions
on click and when the queued action actually runs. Hide/disable for ordinary
inventory, vendor, cube-only view, game exit, and unsupported compatibility
state. Disable while deposit is pending so repeated clicks do not create
duplicate requests. Missing UI services or an unresolved child must disable
only this optional plugin, preserving the established Controller QOL path.

## Validation limits and next checks

The above establishes public API capability, current on-disk Charm resources,
local geometry, and prior message-channel constraints. It does not establish
the new button's rendering, pointer routing, game-thread behavior, item
integrity, or save persistence.

The integration should report automated checks separately from live tests.
Required live checks include stash-open-only visibility, clicking with and
without an eligible item, selected stash tab semantics, repeated clicks,
stash close during a pending action, reopen/game re-entry, original/expansion
inventory, input-mode switching, and coexistence with the Charm arrow. Verify
the original controller shortcut and ordinary Ctrl+click remain functional.
No new RVA, address, structure offset, private ABI, or native patch was
introduced by this investigation.

## Standalone native adapter

The prototype compiles the original Controller QOL `src/bulk_stash.cpp` and
`src/belt_compatibility.cpp` into its own DLL. It has no runtime dependency on
the Controller QOL DLL and does not link its controller-input, input-hook, or
material-withdrawal implementation. `src/deposit_native.cpp` supplies only the
two symbols required by that batch source:

- `QolMaterials::SelectedStashTab(context)`, a guarded UI-thread query.
- `QolClientTransfer::BatchDeposit(context, expected)`, the existing remote
  native-request path, retaining its no-retry result semantics.

The adapter admits Item, Inventory, Thread and Lifecycle service ABI versions,
table sizes, and every required function pointer before enabling itself. It
requires the exact existing `QolNativeProfile::CoreHash` shown above; it checks
that the loaded Core base stays the same before later native calls. This is
the reused native adapter's exact-build requirement, distinct from the public
child/widget API's versioned service contracts.

Admission and runtime checks reuse the following existing QOL profiles. These
are D2R.exe-relative RVAs unless explicitly marked Core; exact expected bytes
remain in the cited source headers instead of duplicating them here.

| Existing contract | Reused addresses and byte source |
| --- | --- |
| Complete native transfer/owner bodies | `client_transfer_profile.h::Sites`: `0x15F8B0`, `0x15EEC0`, `0x15A340`, `0x15EDB0` |
| Local context/player | `0x08B2D0`, `0x09A480`; `belt_signatures.h::LocalContext/LocalPlayer` |
| Client unit lookup / native item page | `0x09A5D0`, `0x36CFE0`; `vendor_signatures.h::ClientUnitBytes/ItemPageBytes` |
| Item code, cursor, native mode | `0x36EF50`, `0x14F1E0`, `0x14F210`; `native_identify_profile.h::Code/Cursor/Mode` |
| Native item blocked check | `0x1C7360`; `identify_probe_profile.h::Blocked` |
| UI-mode query | `0x0CE500`; `materials_signatures.h::UiMode` |
| Top-level panel lookup | `0x846170`; `native_identify_profile.h::FindPanel` |
| Selected stash tab | `0x23AF50`; `shared_page_signatures.h::SelectedTab` |
| Shared owner ID | `0x2EF880`; `shared_deposit_signatures.h::SharedOwnerIdBytes` |
| Shared owner resolver | `0x23AD80`; existing `shared_owner_compatibility.h::ValidateOwner`, including its narrowly allowed relocated call to Core `0x1C9E00` |
| Client request dispatch | existing `client_request_profile.h::Guard`: thunk `0x3E2B73C`, slot `0x3E2AE20` to Core `0x822DD0`; existing `plugin_coexistence.h::Send` validates the known forwarding-hook alternative |
| Deposit eligibility | `0x15A0B0`; `storage_focus_profile.h::DepositEligible` |
| Deposit destination, previous season, finish interaction | `0x46DA50`, `0x23B980`, `0x1A0780`; `materials_signatures.h::CurrentOwner/PreviousSeason/FinishInteraction` |

Canonical native evidence remains in Controller QOL's
`docs/REMOTE-ID-TRANSFER-TRACE.md`, `docs/BULK-STASH-REV37.md`, and the referenced
compatibility headers. No new byte window or private ABI is inferred here.

`InventoryDepositNative::Available()` is UI-thread only. Before any native UI
query it repeats the profile checks, then requires stash UI mode `0x18`, a
resolved `BankExpansionLayout` with tab 0 through 4, current season, no vendor
UI mode `0x0B`, no held cursor item, native mode other than 5, and a local player
visible through both SDK and native lookup. It returns false during ordinary
unavailability. A profile mismatch or caught native fault latches the adapter
off until plugin reload; it does not relax a signature or try another route.

Since beta.10, `InventoryDepositNative::Visible()` uses the same guarded checks
but omits the held-item query at `0x14F1E0` and mode-5 rejection from `0x14F210`.
This only controls presentation. The button's enabled predicate still requires
strict `Available()` plus an idle batch. The queue receives strict `Available()`
directly: using the visual predicate would admit held items, while using the
enabled predicate would cancel a batch because its own busy flag is set.
Native batch preflight and each individual transfer retain their cursor checks.
Both public predicates remain UI-thread only and share the existing mutex,
profile guards and fault latch. These are existing build-specific calls, not
new native entry points or relaxed compatibility checks.

The remote adapter re-resolves `expected.handle` through ItemService, checks
the existing `QolClientTransfer::Source` identity/container rule, then checks
native item type, class ID, runtime ID, unit mode, item code, inventory page 0,
blocked state, cursor state, and native interaction mode. Those native identity
fields are the inherited DWORD offsets `+0x00`, `+0x04`, `+0x08`, `+0x0C` from
the QOL request path. It checks eligibility and resolves the destination before
using the existing disengaged 16-byte placement state for inventory page 0 to
stash page 4 and the existing finish call `(3, nullptr, 0, 0, false)`.

As in QOL, `Submitted` includes a false native transfer return: a request may
already have been sent, so the batch owns observation and must not submit it
again. Fault after possible submission returns `Refused`, stops that batch,
and disables this adapter. The reused queue's advanced-counter observation
limits and non-atomic behavior are unchanged.

The adapter has its own mutex and never calls the bulk queue while holding it;
the queue is allowed to call the adapter while holding its queue mutex. It
retains no native widget/item pointer after a callback. The UI-thread restriction
is a caller contract, not a claim that the SDK exposes a current-thread query.
The button is registered `KeyboardMouseOnly`; this adapter does not read the
private controller manager or install input hooks to infer input mode. An
already-started batch retains the inherited queue cancellation rules when the
input device changes. Live input-switch/coexistence behavior remains a required
test. Build/automated validation passed as recorded below; runtime validation
is pending.

Remote coexistence limit: the standalone DLL and Controller QOL have separate
batch queues. `KeyboardMouseOnly` prevents the new button from being added to a
controller layout, but does not cancel a request already started with a mouse.
On a remote client, switching to controller and starting QOL's deposit before
the standalone queue observes the first request could submit the same still-
visible source from both queues. Each queue's internal no-retry rule does not
provide cross-DLL arbitration. No live coexistence result or existing public
SDK input-mode/busy query establishes that this is safe. Do not claim remote
coexistence until cancellation/arbitration is demonstrated; disabling the
standalone remote fallback is a conservative initial scope if that proof is
unavailable. No new controller-manager address read was added to hide this gap.

## Implemented candidate and automated validation - 2026-10-11

Built `Inventory Deposit.dll` version `0.1.0-beta.1` using MSVC 19.44.35220.0,
CMake 4.3.3, Windows SDK 10.0.26100.0, Release/x64 with static CRT. The final
changed sources compile at `/W4` without warnings. The first CMake compiler
probe encountered host PDB-manager error C1902; using CMake's Embedded debug
information mode for Debug/RelWithDebInfo compiler probes resolved it. This
does not relax compiler or artifact validation.

All five CTest suites passed:

- `inventory-deposit-ui`: fake SDK capability/registration rollback, exact
  message routing, bounded refresh/click queues, UI-only widget operations,
  state-write failures, late widget creation, unload/reinitialization and
  session-generation invalidation.
- `inventory-deposit-batch-policy`: reused QOL inventory-only scope, identity,
  overflow, source observation, no-retry and chord policy tests.
- `inventory-deposit-transfer-policy`: reused QOL remote identity/routing tests.
- `inventory-deposit-artifact`: DLL data/image-resource admission, exact
  one-DWORD ABI manifest, normal DLL load, all three exports, plugin metadata,
  shared role and independent embedded defaults.
- `inventory-deposit-settings`: exact section/key parsing, explicit disable,
  coordinate bounds and refusal without partial settings mutation.

The UI fixture exported the actual SDK resource-registration payload for both
default `(800,1644)` and custom `(123,456)` positions. Python strict JSON parsing
validated them, including root type/name, initial hidden state and exact click
message. No speculative JSON `enabled` field is used: native enablement is set
through WidgetService before revealing the initially hidden widget.

PE inspection found only `bcrypt.dll` and `KERNEL32.dll` imports. It exports
`D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`, and `D2RLoaderUnloadPlugin`.
The DLL has no runtime Controller QOL dependency. Its SHA-256 is:

`3DE0007B016DD4936D1A75C51DEDB197D3D9BF054F6E50D062C9798624906E22`

Local candidate output is under `outputs/inventory-deposit-button/`:

- `Inventory-Deposit-0.1.0-beta.1-runtime.zip`
- `SHA256SUMS.txt`
- `build/Inventory Deposit.dll`

The ZIP's integrity, exact entries, text privacy, and packaged/built DLL byte
identity passed. It contains `d2rloader/plugins/Inventory Deposit.dll`, reference
settings under `defaults/`, installation/validation notes, and repository/SDK
licenses. It contains no game assets, live configs, saves, logs or backups.
The ZIP SHA-256 is:

`1DB0189FCC24FC7574F0E319CAD10501E1CAB4B46684C9C5F0B99735A83963A7`

Existing QOL files considered during integration were compared byte-for-byte
with pre-edit snapshots and are unchanged by this task. There was no deployment,
commit, publication, live loader admission, in-game rendering, actual item
mutation or save-persistence test. The remote coexistence limitation above
remains a real validation/design boundary, not something these tests prove.

## User-confirmed operation and alignment adjustment - 2026-10-11

The user reported "That worked really well" and supplied
`screenshot-e63b66b4-593c-4f6e-a672-ae269dd8ea54.png` (2767 by 1439).
It shows the deposit button rendered in the inventory footer alongside the
gold control, with the Charm Inventory arrow still present. This confirms
visible integration; the user report confirms their deposit interaction worked.
It does not independently establish save persistence or remote coexistence.

The global installed `d2rloader/plugins/Inventory Deposit.dll` matches the
beta.1 artifact SHA-256 above. `d2rloader/logs/inventory-deposit.log` records
native profile admission, child registration, and the following two batches
(log timestamps retained verbatim, without inferring timezone):

| Log timestamp | Snapshot | Submitted | Removed from inventory | Skipped |
| --- | ---: | ---: | ---: | ---: |
| 2026-10-11 06:13:15.698 | 16 | 4 | 4 | 12 |
| 2026-10-11 06:13:26.773 | 17 | 5 | 5 | 12 |

These are the authoritative-queue completion summaries from the reused bulk
implementation, rather than independent destination-counter or save evidence.
The loader log identifies this plugin as globally loaded. The verified live
configuration is `d2rloader/config/inventory-deposit.toml` in the same game root.

The old sprite center is about 12 pixels below the gold row in the resized
2048-pixel-wide screenshot. The next position is `(860,1620)`: 60 parent-local
units right and 24 up, approximately 30 pixels right and 12 up at that scale.
This aims to align the button vertically with gold and horizontally center the
free footer region to its right. These coordinates are presentation settings,
not new native offsets. Transparent texture margins remain unmeasured; the
adjusted appearance needs a fresh in-game view.

The source defaults/reference settings and the existing global configuration
were adjusted. The configuration is backed up before the edit; only its two
coordinate values change. Explicit settings remain preserved generally, and
no installed DLL or panel/MPQ asset is replaced. A full game restart is needed
because the embedded resource/child layout is registered during plugin load.

The exact backup is
`d2rloader/backups/inventory-deposit-alignment-20261011-5f24161d/inventory-deposit.toml`.
TOML read-back verified that only `button_x` and `button_y` changed; the installed
DLL still has the beta.1 hash recorded above. A separate Release build under
`outputs/inventory-deposit-button/build-alignment/` passed all five existing
CTest suites after the source-default update. It was not installed or packaged;
the user can see the new placement using their existing DLL after restarting.

## Held-item visibility fix - beta.10, 2026-10-11

The user reported that picking up an inventory item hid the button and supplied
`screenshot-19536af6-d688-4f62-ae85-80be9c67b8ea.png`. The source explains
this directly: the button's visibility callback previously called strict
`Available()`, so either existing cursor check hid it. This was a plugin
presentation bug; no missing panel resource or artwork was implicated.

Beta.10 splits visibility from action availability as documented above. The
existing widget update sets `enabled=false` while retaining `visible=true`,
which was intended to select existing embedded `disabledFrame=3`. Beta.12's
[native investigation](BUTTON-STATE.md) establishes that this SDK flag alone
does not invoke the ButtonWidget visual transition. Putting the item down only
re-enables the button. The UI queue revalidates before click dispatch, and the
action still requires a fresh click. No artwork, layout, feedback, configuration,
native address, byte guard or provider contract changed.

Automated evidence: the Release/x64 `/W4` build completed without compiler
warnings and all ten CTest suites passed. The new `inventory-deposit-wiring`
suite compiles the actual `plugin_main.cpp` with adapter stubs and captures its
real callbacks. It verifies held-item visibility with blocked actions, release
without an automatic request, a busy batch that cannot be clicked but remains
eligible to continue, closed contexts, and two load/unload cycles. Existing UI
tests verify queued-click revalidation. These tests exercise application wiring;
they do not execute the native cursor queries or prove live grey rendering.

The actual DLL artifact verifier passed ABI manifest, exports, plugin identity,
version and embedded artwork checks. Package inspection passed exact sprite
resource identity, layout JSON, imports, archive contents/integrity and text
privacy. The runtime DLL remains independent of Controller QOL and imports only
`KERNEL32.dll` and `bcrypt.dll`.

Built/deployed DLL SHA-256 (616960 bytes):
`DE0249364961D793F09F6E18C005781C4B6833192FA2849E08FFF34D3DCA4CF8`.
Local candidate archive
`outputs/inventory-deposit-button/cursor-disabled/Inventory-Deposit-0.1.0-beta.10-runtime.zip`
SHA-256: `CAAD912E5F92630E465005DA7D19B173820C49E746A77C14D92FF77E1088E549`.

D2R and D2RLoader were closed before deployment to the established global
`d2rloader/plugins/Inventory Deposit.dll`. The previous beta.9 DLL and live
config were backed up under
`d2rloader/backups/inventory-deposit-cursor-disabled-20261011-093622/` before an
atomic file replacement. Built and installed DLL hashes match. The live config
is byte-identical, SHA-256
`154283F7EBD5CCB13D8056D533E8E8F4C487594BB7211040B3723B8241571E2A`.

The user reported the earlier feedback/fit works well. Beta.10 has not yet been
observed in game. On the next launch, hold an inventory item while the stash is
open: the button should remain grey and refuse clicks, then return to normal
after placing the item without depositing anything until a fresh click. No
new live transfer, destination-counter or save-persistence result is claimed.

## Empty-inventory enablement - beta.11, 2026-10-11

The user confirmed beta.10's cursor-state behavior works and requested the same
disabled appearance when there is nothing eligible to deposit. Holding an item
remains intentionally blocked: the supported transfer adapter requires an empty
cursor and invokes the native finish-interaction path after submission. This
change does not establish that a transfer can safely coexist with cursor item
placement, so it preserves that proven precondition.

`InventoryDepositNative::HasEligibleItems()` runs synchronously on the SDK UI
thread under the existing adapter mutex. It repeats strict `AvailableUnsafe(true)`
before and after a positive scan. `deposit_eligibility.h::Any` captures the SDK
local-player handle, snapshots copied Inventory/page-zero items with the shared
`QolBulkStash::Plan` (deduplicated runtime IDs, 256-item limit), then refreshes
each candidate through `ItemService::getItemInfo`. It requires unchanged source
identity, seeds and placement through `QolClientTransfer::Source`, and page zero
through `QolBulkStash::Source`. It stops at the first validated native-positive
candidate, after rechecking the same player handle. An empty/ineligible list,
failed or malformed read, changed source/player, or overflow leaves it disabled.
No SDK read is nested in the inventory enumeration callback.

Thread/API provenance: PluginSDK `include/D2RLPlugin/inventory.h` permits reads
using a player handle captured on the UI thread and states that enumeration is
synchronous with copied `ItemInfo` values. The SDK README documents handle
session/runtime-ID/seed validation. `include/D2RLPlugin/item.h` restricts
`editNativeItem` to the game thread, so the UI scan does not use that API.

`InspectEligibilityUnsafe` reuses the existing admitted client path on D2R
`3.3.93787` with the existing exact D2RCore profile hash. No new RVA is introduced:

| Existing operation | Reused contract |
| --- | --- |
| Client item lookup | `0x09A5D0(runtimeId, 4)`; guard `QolVendor::ClientUnitBytes` |
| Unit identity | DWORD offsets `+0x00/+0x04/+0x08/+0x0C`: type 4, class ID, runtime ID, mode 0 |
| Item code and inventory page | `0x36EF50(item)` and `0x36CFE0(item)`; require matching code and page 0 |
| Blocked item | `0x1C7360(item)`; existing `QolIdentifyProbe::Blocked` guard |
| Native eligibility | `bool 0x15A0B0(item)`; full `QolStorageProfile::DepositEligible` guard |

These are the same client lookup, identity and eligibility calls used by
`BatchDepositUnsafe`; native pointers are local to one synchronous inspection.
The eligibility predicate's existing provenance is Controller QOL
`docs/BULK-STASH-REV37.md` and `docs/REMOTE-ID-TRANSFER-TRACE.md`: stash context,
expansion and advanced-storage registry membership. It is not a destination
capacity probe or proof that the subsequent transfer will succeed. Exceptions
latch the existing adapter off; ordinary SDK read failures simply disable the
current button check. The scan submits nothing and calls neither the transfer
nor finish-interaction routine.

The button enabled predicate is `!Busy() && HasEligibleItems()`. Thus busy batches
skip the scan. The bulk queue still receives strict `Available()` alone: removing
the final eligible item must not cancel verification or once-per-tab feedback.
The existing 100 ms UI refresh picks up inventory changes, and queued clicks
repeat the scan. No cache or raw pointer survives a callback. Actual execution
retains its native eligibility, source and cursor checks.

The user-confirmed cursor presentation is separate from the new empty-state
presentation, which still needs an in-game check. Test equipment-only inventory,
adding one eligible item, depositing the last eligible item and adding another;
the button should toggle grey/normal without triggering an unsolicited deposit.

Release/x64 `/W4` build completed without compiler warnings. All eleven CTest
suites passed. The new eligibility suite exercises the actual SDK scan helper
with fake inventory/item services: empty, all-ineligible and mixed inventories;
non-inventory/page exclusions; duplicate identities; complete snapshots and early
native-positive exit; the 256-item boundary and overflow; malformed/failed reads;
changed source identity, seeds, cell, container and page; and changed players.
The fixtures reject SDK/native reentry during enumeration and provide no item
mutation functions. Production callback tests additionally prove that empty
inventory disables new requests, adding items does not auto-submit, and an
exhausted active batch retains its context availability. These tests do not
execute game code or establish new runtime/native behavior.

Actual DLL ABI/resource, exports, identity/version, layout and package integrity
checks passed. Imports remain `KERNEL32.dll` and `bcrypt.dll`; sprite manifests
and source/output artwork hashes match beta.10 exactly. Built/deployed beta.11
DLL SHA-256 (618496 bytes):
`CAA6E620B74BDFDDF8266EA92BDCD8E801143764819AFF362D230E60BC4D7734`.
Candidate archive
`outputs/inventory-deposit-button/empty-disabled/Inventory-Deposit-0.1.0-beta.11-runtime.zip`
SHA-256: `18E75BC7EA9480CFF2C1DA22321C0032F895416981EE476FC58A1C70ED2590DC`.

After confirming D2R and D2RLoader were closed, the established global DLL was
replaced atomically and its hash verified against the build. The beta.10 DLL
and config are backed up at
`d2rloader/backups/inventory-deposit-empty-disabled-20261011-094447/`.
Live config is byte-identical with SHA-256
`154283F7EBD5CCB13D8056D533E8E8F4C487594BB7211040B3723B8241571E2A`.
No beta.11 live-game or persistence validation, commit or publication was made.
