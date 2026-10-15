# Item Roll Ranges

**By PyreFly · Version 1.3.1+rev.45**

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
Fixed modifier bounds are compact, such as `[+38]`; Base headers retain both
endpoints. Socketed items also show Max Sockets in the normal header position
while ranges are held. Weapons use Required Level / item-level details / Max Sockets / Attack Speed.
The new `(Ilvl: XX, Qlvl: YY)` line shows generated item level and active base
quality level. Items without Required Level use the remaining header position.
Unrelated triggered skills do not prevent a known header base from displaying.
Unresolved expected ranges show `(?)` and
`(?) Range Unavailable - Please report item affixes`.

Magic/rare/crafted affixes can show `[P] [Name] [Tn]` / `[S] [Name] [Tn]` labels.
T1 is the highest distinct affix level in the applicable family for that item
type/quality; duplicate levels share a tier. Unique properties use `[U]`
and verified inherent contributions use `[Base]`. Ambiguous contributions are
not assigned invented ranges or tiers. Supported overlapping host, socket and
completed corruption contributions show a `[Combined]` total followed by gray
source rows; sockets use `[Socket n]` in insertion order and corruptions `[C]`.
Unknown individual rolls remain `?` when the saved total cannot identify them.
Active rune/gem definitions use the host item's context. Level-scaled bounds
use the active level cap and compatible coefficient/rounding rules. Affix
families retain distinct property/selector identities even when names repeat.
Runeword-owned properties use `[RW]`; rune contributions retain socket labels.
Recipes and ranges come from active compiled tables, including mod changes.
Shared property rolls can resolve resistance overlaps and grouped Attributes.
Random skill/class/tab properties show their bonus bounds, not the selected
skill ID interval; Gnostics can display `[+5]` for its selected skill.
Replenishes Quantity can show its guarded recurring interval.
Enchantments, unsupported transforms, ambiguous conditional groups, set bonuses
and crafting-recipe contributions do not have complete source attribution.

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

All nine automated suites pass. User checks confirm the reviewed socket and
corruption breakdowns, fixed and modified Defense headers, Shadow Mantle's
overlapping ED/flat Thorns and the corrected Max Sockets header placement.
No-clone tooltips also receive verified source bounds. Dedicated live controller,
every item family, localization and plugin combination remain incompletely
validated. Ranges reflect the active item definitions, including mod changes.

## Uninstall

Close the game and loader, remove this DLL from the active plugins folder and
relaunch. Core's native range behavior remains. `SHA256SUMS` records runtime
archive contents; the source archive is supplied separately.

Unique source labels use `[U]` in revision25. Set modifier ranges remain native; the plugin does not currently add set source labels.

## Optional always-show ranges

The default is `display.always_show_ranges = false`: hold Ctrl or RT/R2
(or the Controller QOL override) to view ranges. Set this option to true in
`d2rloader/config/item-roll-ranges.toml` and restart to enable at startup.
`defaults/item-roll-ranges.toml` is a reference; existing settings are preserved.
In game, `/AlwaysShowRolls` toggles for the current session. Arguments `on`,
`off` and `status` are supported. Use lowercase `/alwaysshowrolls` if needed.
Feedback is written to the loader console. Restart restores the saved setting.
The range hint is hidden while always-show is active; normal input stays native.

Rev.40 fixes the plain Sash fixed Base header with always-show off and Ctrl held.
Eight suites pass; user checks confirm the session toggle and fixed Sash repair.
Unknown property formatter replacements/entry detours refuse admission rather
than attempting undocumented composition. Broader localization/controller and
arbitrary-plugin combinations have not been exhaustively validated.

## Revision44 validation

Private active-table replays inspect2,026 affix rows and resolve219/219 enabled
Reimagined recipes;56 random-selector specifications verify fixed bonus bounds.
These counts describe data/model checks, not exhaustive visible coverage.
User tests confirm resistance overlaps, grouped Attributes, Bone, Lightning and
Honor in Reimagined, independent fire endpoints, replenishment display,
Ctrl/session toggles and Wake of Fire. Later vanilla smoke checks also passed.
Captured real charm/amulet metadata is replayed privately and is not distributed.
Unknown transforms and ambiguous contributions retain native text.
See `docs/PROPERTY-ROLLS-REV41.md` and `docs/RANDOM-SKILL-PROPERTIES-REV44.md`.
