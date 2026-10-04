# Controller QOL Updates

**Made by PyreFly for D2RLoader**  
Version **1.3.1+rev.48** | Windows x64 | Diablo II: Resurrected

Controller QOL Updates adds direct controller looting, contextual item actions,
stash and Cube transfers, belt management, clearer controller prompts, and
controller-friendly menu navigation. It preserves the game's native item,
storage, vendor, and interaction rules.

Xbox button names are used below. On a PlayStation-style controller, LB/RB are
L1/R1, LT/RT are L2/R2, and A/X/Y/B are Cross/Square/Triangle/Circle.

## Shortcuts

The default shortcut modifier is **LB**. Change `modifier` for item-management
actions or `ground_pickup_button` for direct ground looting.

### World and ground-loot controls

| Shortcut | Context | Action |
|---|---|---|
| Hold LB | Near labeled ground items | Show and refresh up to seven direct-loot assignments |
| LB + A | Ground-loot assignments visible | Pick up the item labeled A |
| LB + X | Ground-loot assignments visible | Pick up the item labeled X |
| LB + Y | Ground-loot assignments visible | Pick up the item labeled Y |
| LB + B | Ground-loot assignments visible | Pick up the item labeled B |
| LB + RB | Ground-loot assignments visible | Pick up the item labeled RB |
| LB + RT | Ground-loot assignments visible | Pick up the item labeled RT |
| LB + LT | Ground-loot assignments visible | Pick up the item labeled LT |
| A | Normal world interaction | Prefer an eligible nearby portal, stash, waypoint, shrine/well, or enabled chest over ground loot |

Direct-loot assignments are stable while the modifier is held. Empty slots are
refilled as items are collected or the player moves. Only nearby, reachable
items with an active visible ground label are assigned. Releasing the modifier
removes the shortcut overlays and does not pick anything up automatically.

Bare **A** remains the normal native interaction button. It is not an LB
shortcut: when no enabled priority object wins, native item pickup and world
interaction continue normally. With `block_filtered_pickup = true`, items whose
labels are filtered out cannot be collected accidentally with bare A.

### Item, storage, Cube, belt, and shop controls

| Shortcut | Focus/context | Action |
|---|---|---|
| LB + A | Unidentified item | Identify the focused item using an allowed Identify Tome or scroll |
| LB + A | Identify Tome | Identify all unidentified items in main inventory and the Horadric Cube when `identify_all` is enabled |
| LB + A | Supported potion | Move the focused potion to the belt |
| LB + X | Inventory with storage open | Transfer the focused item to the active storage context |
| LB + X | Personal, Shared, custom, or Cube item | Transfer the focused item back to inventory |
| LB + X | Gems, Materials, or Runes page | Store eligible items in advanced storage; send other inventory items to the embedded Cube |
| LB + X | Inventory item with NPC shop open | Sell through the game's native vendor path |
| LB + Y | Eligible carried or stored item | Transfer directly to the Horadric Cube |
| LB + R3 | Potion in inventory, Cube, or supported open storage | Fill available belt slots |
| LB + R3 | Merchant potion | Refill the belt and buy missing potions where native rules allow |
| LB + L3 | Stash open | Deposit all eligible materials, gems, runes, and rejuvenation potions from inventory when `quick_deposit` is enabled |

The focused-item tooltip and controller header show only actions currently
available for that item and screen. Shared-stash transfers retain the selected
page and exact item identity. A failed advanced-storage deposit does not spill
the item into another container.

### Menu and stash navigation

| Shortcut | Screen | Action |
|---|---|---|
| LT / RT | Main menu pages | Previous / next main page |
| LB / RB | Skill Tree | Previous / next skill tab |
| LB / RB | Quest Log | Previous / next act tab |
| LB / RB | Chronicle | Previous / next inner Chronicle tab |
| LB / RB | Options | Previous / next inner Options tab |
| LB / RB | Loot-filter rule editor | Switch Equipment / Items inner tabs |
| LB + LT / LB + RT | Shared stash | Previous / next Shared stash page |
| RB | Item tooltip | Show native roll ranges while the remap is available |
| Down | Focused loot-filter rule tab | Enter the lower rule list when native focus is missing |

Main-page triggers remain available while the inner Skill, Quest, Chronicle,
Options, and loot-filter tabs use bumpers. Ground-loot chords are suppressed on
these menus so navigation cannot queue a world pickup.

## Other behavior

- Guided Arrow controller ground casts use a farther target point so the
  skill's native target acquisition and homing can operate like keyboard and
  mouse casting. Target-selected casts, other skills, and mouse input are not
  changed.
- With integrated controller aim enabled in a session, its cursor controls
  Guided Arrow distance. Turning aim off restores QOL's usual correction.
- Filtered ground labels remain visible in controller mode while the direct-loot
  system is active.
- Controller glyphs follow the device artwork selected by the game.
- Item transfers, identification, selling, belt filling, and object interaction
  continue through native game or D2RLoader transaction paths.

## Configuration

Edit `d2rloader/config/controller-qol-updates.toml` while the game is closed.
All settings live under `[qol]` and are read when the plugin loads.

| Variable | Default | Purpose |
|---|---:|---|
| `enabled` | `true` | Master switch for Controller QOL actions and input handling |
| `quick_identify` | `true` | Enable focused-item identification with the configured modifier + A |
| `identify_all` | `true` | Enable modifier + A on an Identify Tome to identify all eligible inventory/Cube items |
| `native_identify` | `false` | Use the slower native hold-A identification sequence instead of SDK-first edits |
| `quick_move` | `true` | Enable contextual transfer, Cube, belt, vendor, and bulk-deposit shortcuts |
| `quick_deposit` | `true` | Enable modifier + L3 bulk advanced-storage deposit while the stash is open |
| `require_tome_or_scroll` | `true` | Require an eligible Identify Tome or scroll before identifying an item |
| `consume_tome_or_scroll` | `true` | Consume and verify one identify charge for each identified item |
| `require_modifier` | `true` | Require `modifier` for A-button identify/item-interaction actions; X/Y/R3 and L3 remain explicit chords |
| `modifier` | `"bumper"` | Item-action modifier. Common values: `bumper`/`lb`/`l1`, `trigger`/`lt`/`l2`, `rb`, `rt`, `l3`, `any`, or a supported key/paddle alias |
| `trigger_threshold` | `30` | Analog trigger threshold reserved by the configuration; the current build uses 30 |
| `ground_pickup` | `true` | Enable direct ground-label assignments and pickup chords |
| `ground_pickup_button` | `"bumper"` | Direct-loot modifier. Supports `lb`, `rb`, `lt`, `rt`, `l3`, `r3`, bracket/paddle aliases, or a single keyboard letter/number |
| `ground_pickup_distance` | `6` | Direct-loot search distance, clamped to 1â€“20 game units |
| `block_filtered_pickup` | `true` | Prevent native pickup of ground items without an active visible label |
| `prioritize_portals` | `true` | Prefer eligible portals over ground loot for neutral A |
| `prioritize_stash_boxes` | `true` | Prefer the town stash over ground loot for neutral A |
| `prioritize_waypoints` | `true` | Prefer eligible waypoints over ground loot for neutral A |
| `prioritize_shrines` | `true` | Prefer shrines and supported wells/fountains over ground loot for neutral A |
| `prioritize_chests` | `false` | Prefer supported ordinary chests over ground loot for neutral A |
| `portal_priority_distance` | `10` | Shared priority-object range, clamped to 1â€“20 game units |
| `debug_logging` | `false` | Enable general diagnostic logging |
| `portal_diagnostics` | `false` | Enable targeted, throttled object-priority diagnostics independently of general debug logging |

`modifier` controls item actions; `ground_pickup_button` independently controls
the seven world-loot chords. Keeping both at LB gives the default layout shown
above.

## Controller aim

Controller aim defaults on and also requires `[qol] enabled = true`. Set
`[aim] enabled = false` in `d2rloader/config/controller-qol-updates.toml` and
restart to disable it. Explicit existing false values remain respected.
It adds a right-stick cursor within 30 world tiles. Tilt controls speed, holding
accelerates, and release stops cursor movement. Unsupported skills keep native
controller targeting; mouse/keyboard casts retain their normal route.

| Class | Skills | Targeting |
| --- | --- | --- |
| Sorceress | Teleport | Ground only |
| Sorceress | Meteor, Blizzard, Hydra, Fire Wall | Enemy snap with ground fallback |
| Amazon | Guided Arrow, Multi Shot | Enemy snap with ground fallback |
| Necromancer | Teeth | Enemy snap with ground fallback |
| Barbarian | Leap | Ground only; Leap Attack is excluded |
| Barbarian | Whirlwind | Retained enemy snap, optional pass-through |

Move the right stick beyond the configured dead zone to activate manual aim.
Releasing it retains the cursor without a timeout. Casting an unsupported or
configuration-disabled skill releases the lock and restores native targeting;
release and tilt the right stick again to resume manual aim. No controller toggle
button is required. Circular snapping uses fresh native-eligible candidates near
the cursor. Native character turning, landing and collision checks remain.
Idle markers only copy native candidate observations: they never change native
targeting scores or selection caches. This means an enemy excluded by native idle
targeting may acquire its marker only when the snap-capable cast begins. Teleport
and Leap themselves never snap.
Whirlwind retains an eligible enemy across a spin so the next cast can pass back
through it. Moving the stick releases retention. Pass-through extends beyond the
enemy by the configured distance, with a 30-tile total cap. Disabling pass-through
keeps direct enemy snapping; disabling snapping uses only the ground cursor.

| Aim control | Action |
| --- | --- |
| Right stick | Activate manual aim and move cursor; release to retain position |
| F8 | Toggle aim for this session (only when enabled in configuration) |
| F9 | Recenter 20 tiles along character facing and release target |
| F10 | Invert right-stick Y for this session |

Bindings appear under Controller QOL Aim in the loader controls menu. The normal
bone reticle marks the cursor, brass corners mark the enemy, and a separate bone
ring marks a Whirlwind pass-through endpoint. Debug mode restores diagnostic
lines/circles/text. These are aim indicators, not skill area-of-effect outlines.

### Aim configuration

All keys below belong in `[aim]`, separately from `[qol]`. Restart after edits.
Missing values use defaults; an invalid aim section disables aim and logs a warning
without disabling other QOL features. Overlay settings do not affect targeting.

| Key | Default | Purpose / allowed values |
| --- | --- | --- |
| `enabled` | `true` | Enable aim hooks, controls and overlay |
| `deadzone` | `0.22` | Radial stick dead zone, 0-0.9 |
| `initial_speed` | `4.0` | Initial full-tilt speed, 0.1-100 tiles/sec |
| `maximum_speed` | `28.0` | Held speed, initial_speed-100 tiles/sec |
| `acceleration_seconds` | `0.65` | Time to maximum speed, 0-5; zero is immediate |
| `snapping_enabled` | `true` | Snap attack skills; false uses ground cursor |
| `snap_radius` | `6.0` | Acquisition radius around cursor, 0.5-15 tiles |
| `switch_advantage` | `1.5` | Competitor must be this much closer, 0-15 tiles |
| `overlay_enabled` | `true` | Show aiming graphics |
| `debug_overlay` | `false` | Show detailed HUD, lines and submitted-cast marker |
| `cast_observer_enabled` | `true` | Optional cast diagnostics/reset observer; false leaves its hook site free |
| `projection_hz` | `60.0` | Ground projection sampling, 10-120; limited by UI rate |
| `overlay_smoothing_ms` | `35.0` | Display-only smoothing, 0-150 ms |
| `whirlwind_pass_through_enabled` | `true` | Extend Whirlwind beyond retained enemy |
| `whirlwind_pass_through_distance` | `1.5` | Extension, 0-15 tiles; zero targets enemy directly |

Disable/remove the standalone `Controller Aim Test.dll` before enabling this
feature. Its old TOML is not read by QOL. For migration, copy its `[aim]` values
into QOL's config and add `enabled = true`; retain both old DLLs/configs for rollback.
Do not load the integrated and standalone aim implementations together.

The observer at `0x4FDB40` remains enabled by default. If another plugin owns or
changes that entry, aim skips only this optional hook and logs a warning; cursor,
snapping and reticles remain active. Set `[aim] cast_observer_enabled = false`
and restart to leave the entry free regardless of plugin load order. Essential
aim hook guards remain required. In observer fallback, actual-cast diagnostics,
the debug LAST CAST marker and Teleport displacement measurements are unavailable.
Disabled-skill resets use admitted active-skill checks and native target requests;
quick transitions that bypass both still need live validation. This does not
guarantee compatibility between two plugins changing Whirlwind movement. QOL's
Whirlwind toggle and pass-through settings are preserved; choose which plugin
controls that skill if their behaviors compete.

### Skills and mod extensions

The config includes `[aim.amazon]`, `[aim.sorceress]`, `[aim.necromancer]`,
`[aim.barbarian]`, `[aim.paladin]`, `[aim.druid]`, `[aim.assassin]` and
`[aim.warlock]` headings, listing all 240 class skills (30 per class).
They organize settings for readability only: an Amazon skill granted to another
class still uses the same setting. The ten previously tested skill IDs default true;
The 30 cataloged passive skills default to `"disabled"`; the other 200 newly
cataloged skills default false for review. Corpse Explosion, Nova,
Poison Nova, passives, auras and self-casts retain native behavior by default.
Comments give a preliminary behavior/review category; they are not compatibility
guarantees. Enabling a newly listed skill opts into its provisional ground or
enemy-snap coordinate mode and requires testing. The list covers player class
skills, including Warlock, not monster-only actions or item/utility internals.
Set any skill to false to keep native targeting while allowing R3 to enable it.
Set it to `"disabled"` to lock aim off and ignore the R3 shortcut for that skill.
For example, `[aim.amazon]` with `"9" = "disabled"` locks Critical Strike off.
Locked entries show `Auto-aim: DISABLED` with no toggle hint. Numeric IDs are quoted TOML keys;
comments label their names. Legacy name keys remain readable for migration.
Unlisted IDs in class sections, duplicate skill entries or malformed aim settings
disable only the aim module with a warning. Restart after editing.

See [the catalog provenance and validation notes](docs/SKILL-CATALOG.md) for
ID sources, review categories and the distinction between listed and tested skills.

```toml
[aim.amazon]
# Guided Arrow
"22" = true
# Multi Shot
"12" = false

[aim.custom]
# Examples only: obtain the actual IDs from the active mod's skills table.
"357" = true
"358" = false
"359" = "disabled"

[aim.targeting]
"357" = "ground"
# Optional override for a built-in skill too:
# "56" = "ground"
```

`[aim.custom]` accepts up to 32 additional unique numeric skill IDs (1..65534),
including mod-added skills or otherwise unlisted base skills. Values are `true`,
`false`, or `"disabled"`, as in class sections. Enabled custom skills default to
circular enemy snapping with ground fallback. Catalog IDs belong in class
sections and cannot be redefined here. Optional `[aim.targeting]` numeric entries
select `"ground"` (cursor coordinates) or `"snap"` (enemy snapping with ground
fallback) for either built-in or declared custom IDs. Overrides never enable a
skill, unlock `"disabled"`, or bypass global `snapping_enabled = false`. They remain
in effect when R3 disables/re-enables a catalog skill. Unknown custom IDs must be
declared under `[aim.custom]` first. Legacy custom `"ground"` and `"snap"` values
remain readable with their existing behavior; explicit targeting overrides win.
Custom R3 toggling remains limited to the verified built-in skill-tree catalog.
Names alone do not identify
custom skills, and IDs must be checked against the active mod rather than an
unrelated installation. Configuration does not prove compatibility with every
native skill implementation: self-cast, aura, summon or unusual targeting paths
may not consume these coordinates. Custom entries are explicit experiments;
test each skill's landing, targeting and collision behavior in game.

## Install or update

1. Close Diablo II: Resurrected and D2RLoader.
2. Back up the existing DLL and configuration.
3. Remove or disable any older `QOL.dll` so only one Controller QOL plugin is
   active.
4. Copy `d2rloader/plugins/Controller QOL Updates.dll` into the loader's active
   plugins directory.
5. Launch through D2RLoader. A missing configuration file is created from the
   embedded defaults.

The plugin ID is `controller-qol-updates`, so its configuration file is
`controller-qol-updates.toml`. If upgrading from the older `qol` identity,
rename `qol.toml` and update any explicit loader ordering entry that names it.
Do not overwrite an existing customized configuration with the loose reference
copy included in the release ZIP.

## Requirements and support information

- D2RLoader plugin ABI 4
- Windows x64 Diablo II: Resurrected client
- Restart the game after changing the DLL or configuration; hot reload is not
  supported

Detailed validation boundaries, native contracts, and engineering records are
kept under [`docs/`](docs/README.md). See
[`PRODUCTION-1.3.1-rev.60.md`](docs/PRODUCTION-1.3.1-rev.60.md) for the current
integration record and `SHA256SUMS` in the release archive for file verification.

## Build from source

Use an x64 Visual Studio Developer Prompt with CMake 3.29+ and the Windows SDK:

```powershell
cmake -S plugins/controller-qol -B build/controller-qol -A x64
cmake --build build/controller-qol --config Release
ctest --test-dir build/controller-qol -C Release --output-on-failure
```

The output is `Controller QOL Updates.dll`. The repository pins the D2RLoader
PluginSDK as a submodule; clone with submodules or initialize it before building.

### Skill-tree aim shortcut (rev.56)

Highlight a skill in the controller skill tree and click R3 to toggle its aim
setting. A control-strip `Auto-aim: ON/OFF` hint shows the highlighted skill's setting.
The change takes effect immediately and is saved to its numeric config entry;
comments, tuning and other skills remain intact. These are shared plugin settings,
not character-specific preferences. Hold does not repeat; release before pressing
again. A save failure leaves the runtime setting unchanged and shows an error.

`[aim] skill_tree_toggle_enabled = true` enables this shortcut by default.
Set it false to disable the shortcut. Catalog skill IDs are accepted regardless
of character class; unknown mod IDs still require explicit `[aim.custom]` modes
and cannot be toggled from the tree in this iteration. Enabling an untested skill
does not establish that its provisional ground/snap behavior is appropriate.
The shortcut reads the existing controller input snapshot on the UI thread and
leaves native button delivery intact; R3 is currently blocked by the native tree.
See [skill-tree compatibility evidence](docs/SKILL-TREE-AIM-TOGGLE.md).

Rev.57 displays a small brass crosshair inside the upper-right corner of each
enabled visible catalog skill icon. Disabled icons have no crosshair. The control
strip shows the highlighted skill's `Auto-aim: ON/OFF | R3 to toggle` status.
Indicators describe the saved per-skill configuration, not whether the right
stick currently owns a cast. Icon bounds/visibility and ancestor scale determine
placement independently of class or skill layout. No new hooks or native icon
edits are added; the existing SDK overlay draws the marks. Layout guard failure
retains aim/toggling and omits the new indicators. Live alignment on other classes,
resolutions and custom trees remains to be checked.

Reticle appearance can be adjusted under `[aim]` (restart to load):

```toml
ground_reticle_color = "#FFFFFF"   # bright white aim point
lock_reticle_color = "#FFD166"     # bright gold enemy lock
reticle_thickness = 2.0            # double the stroke/outline widths
```

Colors accept `"#RRGGBB"` or `"#RRGGBBAA"`; the last two digits set opacity.
Thickness ranges from 0.5 to 4.0 and changes stroke width, not reticle size or
snapping radius. Ground opacity still follows its contextual fade when locked.
Defaults retain the original gameplay colors/widths. The tree marker uses a
brighter gold and slightly thicker stroke independently of gameplay styling.
