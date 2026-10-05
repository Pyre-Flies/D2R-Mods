# Item Roll Ranges

**By PyreFly · Version 1.3.1+rev.21**

Show item roll ranges beside actual values in light blue:
`[+80 - +120] +118 Defense`.

Hold **Ctrl** on keyboard or **RT/R2** on controller. Controller QOL's active
menu/range remap uses **R1/RB** instead. The plugin follows Core's native input
decision and does not consume button presses or poll physical keys separately.
Display enhancements apply in inventory, stash, Horadric Cube and vendor screens
while the game is in the foreground. Release returns to the usual tooltip.

Defense and physical damage headers keep current values and add their verified
intrinsic base range, including explicit fixed endpoints:

- `Two-Hand Damage: 32 to 69 (Base: 9 - 19)`
- `Defense: 25 (Base: 2 - 2)` on Bloodrune

Base values include ethereal scaling and native Enhanced Defense base generation
(maximum base Defense plus one). They exclude affix percentage/flat additions.
Individual modifier lines retain their own roll ranges and source labels.
Unrelated triggered skills do not prevent a known header base from displaying.
Unresolved expected ranges show `(?)` and
`(?) Range Unavailable - Please report item affixes`.

Magic/rare/crafted affixes can show `[P] [Name] [Tn]` / `[S] [Name] [Tn]` labels.
T1 is the highest distinct affix level in the applicable family for that item
type/quality; duplicate levels share a tier. Unique properties use `[Unique]`
and verified inherent contributions use `[Base]`. Ambiguous contributions are
not assigned invented ranges or tiers. Coverage is not universal, especially
for conditional, socket, set and crafting recipe contributions.

## Install

1. Close Diablo II: Resurrected and D2RLoader and back up the existing DLL.
2. Extract into the game folder for a global install at
   `d2rloader/plugins/Item Roll Ranges.dll`. For a mod-scoped install, extract
   into the active mod folder instead. Keep only one active copy.
3. Enable the following in the effective D2RLoader configuration:

   ```toml
   [d2rcore.items]
   item_stat_ranges = true
   ```

4. Launch through D2RLoader. Use a cold restart after updates.

## Compatibility and validation

Requires Windows x64, D2RLoader plugin ABI4 and the supported D2RCore build in
`COMPATIBILITY.txt`. Controller QOL, Python and SDK installation are optional.
Native queries support the game's selected controller backend; Item Roll Ranges
no longer requires separate XInput polling.

Exact provider and live code guards disable unsupported/conflicting paths.
Other plugins modifying the same tooltip functions can still conflict. Hot
reload is unsupported. Production formatter diagnostic dumps are disabled.

All five automated suites pass. The Base header display was user-confirmed in
game on the tested belts and bows. Revision21's native input/coexistence path
has automated context coverage; dedicated live controller validation remains
pending. Every item family, localization and plugin combination has not been
validated. Ranges reflect the active item definitions, including mod changes.

## Uninstall

Close the game and loader, remove this DLL from the active plugins folder and
relaunch. Core's native range behavior remains. `SHA256SUMS` records runtime
archive contents; the source archive is supplied separately.
