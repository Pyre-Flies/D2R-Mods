# Inventory Deposit

Stable version **1.3.1+rev.13**. This promotes the accepted beta.13 behavior
without changing native operations, artwork or existing configuration. The
older beta sections below record historical validation.

Standalone D2RLoader plugin adding a mouse-clickable **Deposit All** button in
the HD inventory footer while the stash is open. The button follows the current
gold amount's right edge and vertical center, including changes to its layout.

The button uses the same batch implementation as Controller QOL's LB+L3 shortcut.
It deposits eligible **gems, runes, materials and rejuvenation potions** into
advanced storage. Native eligibility decides what qualifies. Equipment and
other ineligible items stay in inventory; the Cube, belt and Charm Inventory
are not sources. This is not a general move-everything-to-Personal-Stash action.

Controller QOL is **not required at runtime**. The layout and original custom
button artwork are embedded in this DLL. The button has an antique-gold deposit
arrow and tray on a dark stone face, with normal, hover, pressed and disabled
states and a worn stone frame with a fine antique-gold inlay. Both standard and low-end sprites are included. It is injected as a
uniquely named inventory child; no stock inventory or Charm Inventory resource
is replaced. No additional MPQ or loose sprite files are needed.

## Prerelease status

`1.3.1+rev.13` shortens the tooltip to **Deposit All Gems, Materials and Runes**.
This wording-only update preserves native eligibility and deposit behavior,
disabled/hover states, artwork, placement, configuration and tab feedback.
All twelve existing suites and DLL/resource/package checks passed. The exact
tooltip was verified in generated layout output, and beta.13 was installed with
matching hashes after backing up beta.12; the live config is unchanged. The
user subsequently confirmed the revised wording looks good in game. This is
the first public Inventory Deposit prerelease; see
[release notes](RELEASE-NOTES.md) and the concise [installation guide](../../README.md).

`0.1.0-beta.12` fixes the missing grey appearance. The SDK enabled flag controls
input separately from D2R's ButtonWidget visual state. A guarded adapter now
applies the native visual transition to this button and verifies the grey frame
before showing it. Eligible items restore the enabled appearance. The existing
artwork and eligibility rules are unchanged. See [button state](../BUTTON-STATE.md)
for the native evidence. The user confirmed in game that the button goes grey
with no eligible items or while holding an item, and brightens on hover when
eligible items are available.

`0.1.0-beta.11` also greys out the button when no eligible items remain in the
main inventory. A fresh read-only scan uses the same native eligibility rule
as Deposit All; adding an eligible item re-enables the button without submitting
anything. Failed inventory reads leave it disabled for that check. Execution
still revalidates each item; eligibility does not guarantee destination capacity
or a successful transfer. The user reported its missing grey appearance, which
beta.12 addresses through the separate native visual state.

`0.1.0-beta.10` keeps the button visible and greyed out when holding an item on
the cursor. Putting the item down re-enables it; a fresh click is still required.
Cursor checks continue to block queued clicks and transfers. Closed or
unsupported stash contexts still hide the button. No config change is needed.
The user confirmed this cursor-state presentation works.

`0.1.0-beta.9` starts the existing supplemental tab animation once per affected
destination per Deposit All batch. Multiple different materials share one
Materials animation; later confirmations cannot restart or extend that batch's
timer. A new batch can trigger the tab again. Duration and fade cycles are
unchanged. The game's own current-tab notification remains native-controlled;
this change does not suppress notifications from ordinary item transactions.

The user confirmed beta.8's button fit works. Its 144-by-136 size, fine gold
inlay, `gold_gap=85` and `gold_offset_y=1` are unchanged. The user also reported
beta.9's feedback update works well. No configuration edit or separate
MPQ/image files are required for this update.

Beta.4 added independent deposit highlights for affected advanced-stash
tabs. Confirmed source removals identify the affected destinations without
slowing the instant batch. The native panel normally remembers only one flashing
tab, which let later destinations overwrite earlier ones. See
[stash feedback](../STASH-FEEDBACK.md) for native contracts, checks and live
validation status. Feedback failure does not disable deposits.
For beta.12, all twelve automated suites passed, including bounded inventory
eligibility reads, production callback wiring, independent input/appearance UI
state, and isolated execution of the guarded native enable/disable bodies. Actual DLL/resource/package
checks passed; beta.12 was installed with matching hashes, a rollback backup and
byte-identical configuration. Native-state read-back does not prove live rendering.
An earlier isolated native-setter ABI/footprint probe also passed.
The user reported the once-per-batch feedback update works well; this
does not independently validate every mixed-destination or remote-play case.

The user confirmed beta.3's embedded custom artwork works and looks good.
Runtime logs show completed batches with 4, 3 and 4 submitted source removals.
Beta.12's grey/hover states are now user-confirmed; pressed appearance and
low-end scaling remain unverified. See
[button sprites](../BUTTON-SPRITES.md) and the original
[artwork/prompts](../../assets/art/PROMPTS.md).

Beta.2 added automatic gold-relative placement. Its runtime log subsequently
confirmed successful position/read-back and a completed six-item deposit.
See [gold anchoring](../GOLD-ANCHOR.md) for the evidence and native guards,
and [the original investigation](../INVENTORY-DEPOSIT-BUTTON.md) for beta.1.
The user confirmed beta.1's button works in game on 2026-10-11. Runtime logs show
two completed batches with all 4 and 5 submitted source items removed and
12 ineligible items skipped in each batch. Visual alignment across layouts, destination
counts, save persistence and remote/controller coexistence still require
validation. These boundaries also apply to the public prerelease.

The native profile is inherited from Controller QOL's reviewed D2R
`3.3.93787` / D2RLoader `1.3.1` family. The required D2RCore SHA-256 is
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`, together with
live code, owner-resolver and request-dispatch guards. A version label alone
does not establish compatibility. An unsupported profile leaves the button
unavailable and does not patch the game.

## Install and remove

1. Close D2R and D2RLoader.
2. Put `Inventory Deposit.dll` in the active pack's `d2rloader/plugins/` directory
   (or the global `d2rloader/plugins/` directory for a global installation).
   Install it in one scope only.
3. Start the game with that pack and open the stash in keyboard/mouse mode.
   Hover the new footer icon to identify **Deposit All**.

The loader owns the `inventory-deposit.toml` configuration. The package includes
a reference under `defaults/`; it deliberately does not overwrite a live config.
To remove the plugin, close the game and remove its DLL. Its configuration can
also be removed if no longer wanted. Restart after layout or configuration
changes; cached native inventory children are not hot-reloadable.

## Settings

```toml
[inventory_deposit]
enabled = true
anchor_to_gold = true
gold_gap = 85
gold_offset_y = 1
# Used only with anchor_to_gold = false:
button_x = 860
button_y = 1620
```

Automatic placement reads the loaded `gold_amount` widget each UI refresh and
before a queued click. `gold_gap` is the space from its right edge to the button;
`gold_offset_y` adjusts the vertically centered position. These are parent-local
UI units, so the inventory panel's screen position and scaling apply together.
Changing the gold display's position or dimensions moves the button with it.

The beta.2 DLL enables anchoring by default, including for older configurations
that omit `anchor_to_gold`. Existing `button_x`/`button_y` values are preserved
and used only with `anchor_to_gold = false`; they are never reinterpreted as
relative offsets. To retain beta.1's fixed placement, explicitly disable anchoring.
Adding the new settings to beta.1 alone does not implement automatic placement.

Coordinates and `gold_gap` accept integers 0 through 4096; `gold_offset_y` accepts
-4096 through 4096. Invalid recognized settings disable the plugin. Set
`enabled = false` to hide this plugin's button. Restart after configuration edits.

Automatic mode requires the gold amount and button to be direct children of the
same inventory panel, with ordinary unscaled, zero-anchor child geometry and
enough room for the button. Unsupported/missing geometry or native guard failure
hides the button and refuses clicks; it does not guess a screen position.
Renaming or nesting the gold widget requires an adapter update or manual mode.
The native setters move this plugin's button only; no panel/MPQ replacement is
needed. Multiple resolutions and the final live hitbox still need testing.

The optional Map Stash button can occupy the same footer area. If both are
active, adjust `gold_gap`/`gold_offset_y` or use manual mode and restart. This
plugin does not reposition another plugin's widgets. Legacy graphics and controller layouts
are outside this button's scope.

## Behavior and testing

- The action refuses while a cursor item is held, the vendor is open, an
  unsupported tab/previous-season page is selected, or native guards fail.
  A held item greys the button out without hiding it in a supported stash.
- No eligible main-inventory items also greys the button out. It refreshes
  with the existing UI polling and rechecks queued clicks; there is no automatic
  deposit when an eligible item appears. A running batch still finishes its
  source-removal verification after the inventory becomes empty of candidates.
- Clicks are queued to the UI thread and rechecked. Repeated clicks are bounded;
  session changes invalidate pending clicks. The button disables during its batch.
- Offline/host batches use authoritative game work. Remote TCP/IP batches reuse
  guarded native requests and observe each source removal before advancing.
  Source removal is not independent proof of destination counters or persistence.
- Partial or unconfirmed transfers stop without automatic retries. The operation
  is not atomic. Existing per-item identity and source checks remain in force.
- Controller QOL and this DLL have separate busy state. Cross-plugin batch
  serialization is not implemented. Avoid switching input modes to start a
  second bulk action while the first is awaiting completion; include this case
  in controlled coexistence testing before considering the candidate stable.

For the first live check, use a backed-up test character with a few low-value
eligible items and an ineligible item. Confirm source items, destination amounts,
the untouched ineligible item, gold and Charm Inventory controls, stash-only
visibility, close/reopen, repeated clicks, and save/reload. Test offline and
remote play separately. The original controller shortcut must still work if
Controller QOL is also installed.

## Build from this repository

Requires Windows x64, MSVC, CMake 3.29+, Python 3.10+ with Pillow, the repository's
PluginSDK checkout, and the sibling `controller-qol` source tree. Python/Pillow
are build dependencies only. From an x64 VS Developer Prompt:

```powershell
cmake -S plugins/inventory-deposit -B plugins/inventory-deposit/build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build plugins/inventory-deposit/build
ctest --test-dir plugins/inventory-deposit/build --output-on-failure
```

The standalone target compiles `controller-qol/src/bulk_stash.cpp` and
`belt_compatibility.cpp` directly and imports their reviewed policy/signature
headers. `deposit_native.cpp` supplies only the two native adapters needed by
the batch. Controller input hooks, navigation, aim and other QOL features are
not linked. Build-time source reuse does not create a runtime DLL dependency.
The original PNGs in `assets/art/` are converted into sprites in the build
directory and embedded as Windows RCDATA. Source assets remain editable; the
runtime archive needs only the DLL and optional reference settings.
