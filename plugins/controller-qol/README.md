# Controller QOL Updates

**Made by PyreFly for D2RLoader**  
Version **1.3.1+rev.75** | Windows x64 | Diablo II: Resurrected

Controller QOL Updates adds direct controller looting, contextual item actions,
stash and Cube transfers, belt management, clearer controller prompts, and
controller-friendly menu navigation. It preserves the game's native item,
storage, vendor, and interaction rules.

Xbox button names are used below. On a PlayStation-style controller, LB/RB are
L1/R1, LT/RT are L2/R2, and A/X/Y/B are Cross/Square/Triangle/Circle.

Rev.75 includes per-mod skill profiles, General Skills support and clearer,
smoother reticles. See the [configuration guide](docs/SKILL-DISCOVERY-CONFIG.md)
before editing or rolling back. Full-Ladder Save & Exit crashes remain under
investigation; this release does not claim to fix them. See the
[production record](docs/PRODUCTION-1.3.1-rev.75.md) for validation limits.

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
| A | Normal world interaction | Prefer an eligible nearby portal, stash, waypoint, shrine/well, or enabled chest over ground loot; rev.68 also preserves these objects when combat arbitration would discard them |

Direct-loot assignments are stable while the modifier is held. Empty slots are
refilled as items are collected or the player moves. Only nearby, reachable
items with an active visible ground label are assigned. Releasing the modifier
removes the shortcut overlays and does not pick anything up automatically.

Bare **A** remains the normal native interaction button. It is not an LB
shortcut: when no enabled priority object wins, native item pickup and world
interaction continue normally. With `block_filtered_pickup = true`, items whose
labels are filtered out cannot be collected accidentally with bare A. This guard
applies only while D2R's controller UI is active. Mouse and keyboard pickup
continues normally, even when this setting is enabled.

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
| LB + A | Rejuvenation on the Materials page | Withdraw one potion directly to an available belt slot |
| LB + Y | Rune, gem, or material counter | Withdraw one item directly to the carried Horadric Cube |
| LB + R3 | Potion in inventory, Cube, or supported open storage | Fill available belt slots |
| LB + R3 | Merchant potion | Refill the belt and buy missing potions where native rules allow |
| LB + R3 | Merchant Identify / Town Portal scroll | Fill a matching Inventory tome through native Shift-buy |
| LB + L3 | Stash open | Deposit all eligible materials, gems, runes, and rejuvenation potions from inventory when `quick_deposit` is enabled |

The focused-item tooltip and controller header show only actions currently
available for that item and screen. Shared-stash transfers retain the selected
page and exact item identity. A failed advanced-storage deposit does not spill
the item into another container.

In remote multiplayer, Identify All covers main inventory only and requires the
highlighted Identify Tome in Inventory or open Personal Stash. It sends one native
request at a time, consuming and confirming one charge per item. Cube contents
are not included in the remote batch. Offline Identify All retains its existing
inventory/Cube support.

Remote single-item ID supports Inventory and open Personal Stash targets, using
a charged tome or loose Identify scroll from either container. Inventory supplies
are preferred within each type, and charged tomes are preferred over scrolls.
Cube/Shared Stash supplies and modded scroll stacks remain excluded.

Remote LB+L3 deposits eligible items from one Inventory snapshot sequentially,
waiting for each source to disappear. A changed stash tab, changed item, session
exit or timeout stops the batch. Source removal alone does not prove the stored
counter total; check destination totals during testing.

LB+X on the Cube grid beside Gems/Materials/Runes deposits eligible items
directly into advanced storage. Ineligible items stay in the Cube. LB+X in the
standalone Cube screen still moves to Inventory.

LB+A supports ordinary potions in either Cube view, Personal Stash and the selected
normal Shared Stash page. Remote placement passes the native source page
directly. LB+R3 retains its Inventory/Materials scope and does not drain ordinary
stash or Cube contents.

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

Vendor scroll refill requires a matching non-full tome in Inventory. It sends one
native purchase request and observes charge growth; missing/full tomes cause no
purchase. Native gold and capacity rules apply. Vendor tome refill was confirmed
in live Ladder testing after providing sufficient gold.

## Configuration

Edit `d2rloader/config/controller-qol-updates.toml` while the game is closed.
The action settings below live under `[qol]`; shared aim controls live under
`[aim]` in the same file. All per-skill settings live in
`d2rloader/config/controller-qol-skills/<scope>.overrides.toml`, including R3
choices, targeting, leading and Whirlwind options. Find the active profile by
its `# Mod:` header. The matching `.catalog.toml` is generated reference data,
not active configuration. See the [complete configuration and upgrade guide](docs/SKILL-DISCOVERY-CONFIG.md).

Updates preserve existing choices. Release ZIPs carry reference TOMLs under
`defaults/`, with no files that extract over live configs. The loader creates
a missing main file; QOL creates and completes the active skill profile when
aim initializes. Old skill settings migrate with backups and verification.
R3 keeps only one rolling `.r3.bak` per profile; migration backups are separate.

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
| `block_filtered_pickup` | `true` | Prevent controller pickup of ground items without an active visible label; mouse and keyboard pickup is unaffected |
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
cyan reticle marks the cursor, amber corners mark the enemy, and a separate cyan
ring marks a Whirlwind pass-through endpoint. Debug mode restores diagnostic
lines/circles/text. These are aim indicators, not skill area-of-effect outlines.

### Aim configuration

All keys below belong in the main file's `[aim]`, separately from `[qol]`.
Restart after manual edits; R3 skill changes apply immediately.
The 60 ms smoothing default applies to fresh configs and omitted keys; an
existing explicit value (including 35 or 0) remains unchanged. Larger values
can add visible tracking lag, so 60 ms is a default rather than a forced upgrade.
Missing values use defaults; an invalid aim section disables aim and logs a warning
without disabling other QOL features. Overlay settings do not affect targeting.

| Key | Default | Purpose / allowed values |
| --- | --- | --- |
| `enabled` | `true` | Enable aim hooks, controls and overlay |
| `skill_tree_toggle_enabled` | `true` | R3 toggles a supported skill and saves its mod profile |
| `ground_reticle_color` | `"#8FE8FF"` | Cursor color, quoted #RRGGBB or #RRGGBBAA |
| `lock_reticle_color` | `"#FFD166"` | Target-lock color with optional opacity |
| `reticle_thickness` | `2.0` | Line-width multiplier, 0.5-4 |
| `verbose` | `false` | Detailed aim file logging; restart required. Warnings remain visible. |
| `deadzone` | `0.22` | Radial stick dead zone, 0-0.9 |
| `initial_speed` | `8.0` | Initial full-tilt speed, 0.1-100 tiles/sec |
| `maximum_speed` | `48` | Held speed, initial_speed-100 tiles/sec |
| `acceleration_seconds` | `0.35` | Time to maximum speed, 0-5; zero is immediate |
| `snapping_enabled` | `true` | Snap attack skills; false uses ground cursor |
| `snap_radius` | `6.0` | Acquisition radius around cursor, 0.5-15 tiles |
| `switch_advantage` | `1.5` | Competitor must be this much closer, 0-15 tiles |
| `overlay_enabled` | `true` | Show aiming graphics |
| `debug_overlay` | `false` | Show detailed HUD, lines and submitted-cast marker |
| `cast_observer_enabled` | `true` | Optional cast diagnostics/reset observer; false leaves its hook site free |
| `projection_hz` | `60.0` | Ground projection sampling, 10-120; limited by UI rate |
| `overlay_smoothing_ms` | `60.0` | Display-only screen-motion smoothing, 0-150 ms; zero disables; direct stick adjustments remain immediate |

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

### Skills and mod profiles

Whirlwind's two pass-through options also belong in the skill profile, under
`[aim.whirlwind]`: `whirlwind_pass_through_enabled` (default `true`) and
`whirlwind_pass_through_distance` (default `1.5`, range 0–15 tiles). Zero distance
targets the enemy directly. Their old `[aim]` values migrate automatically.

The main `controller-qol-updates.toml` holds `[qol]` and shared `[aim]` preferences:
cursor speed, deadzone, acceleration, colors, snapping radius and logging.
All per-skill enable states, targeting modes and leading values belong in
`config/controller-qol-skills/<scope>.overrides.toml`. Open the profile whose
`# Mod:` line matches the active pack, or `Vanilla`. R3 writes to that same file.
These profiles are shared by characters using that mod and main config scope.

The adjacent `.catalog.toml` is generated reference data. It is not loaded as
settings or a report of your current choices. For example, its Warp entry can
say `false` while your skill profile says `true` and Warp aim works in game.
Do not edit or copy the whole catalog over your profile.

Profiles organize the 240 built-in class IDs under `[aim.amazon]`,
`[aim.sorceress]`, `[aim.necromancer]`, `[aim.barbarian]`, `[aim.paladin]`,
`[aim.druid]`, `[aim.assassin]` and `[aim.warlock]`. Extra IDs belong in
`[aim.custom]`. Headings organize settings only: equipment-granted Multi Shot
uses the same numeric ID and preference on any class. Names are labels; a mod
can change the skill behind an ID.

- `true`: enable QOL aim using the skill's targeting mode.
- `false`: keep native targeting; R3 can enable QOL aim.
- `"disabled"`: lock QOL aim off and remove that skill's R3 toggle.
- `[aim.targeting]`: `"ground"` uses the cursor; `"snap"` uses an eligible enemy
  near the cursor, falling back to the cursor when none is selected. This mode
  does not enable a skill or bypass the shared snapping toggle.
- `[aim.leading]` and prediction-limit sections tune enabled snap skills only.

Manual edits load after restart. R3 applies immediately after a successful save,
with one rolling `.r3.bak` per profile containing the preceding saved state.
Migration backups remain separate. Existing choices win over defaults; startup
adds missing entries without replacing settings already in the profile. Review
new skills in game before enabling them. Discovery does not infer every custom
spell's correct targeting or projectile speed.

Recognized Reimagined Warp (429) is discovered with ground targeting and starts
off unless an existing preference enables it. R3 can enable it directly; no
uncommenting in the main file is required. For a pack where discovery cannot
read the skill data, this manual fallback belongs in its **skill profile**:

```toml
# Merge into existing sections; do not duplicate the headers.
[aim.custom]
"429" = false # Warp: R3 can enable it after the active mod's ID is verified
[aim.targeting]
"429" = "ground"
```

Profiles also work for vanilla and packs without readable loose tables. Unknown
IDs must be discovered or declared manually before targeting/leading entries can
refer to them. The total limit is 1024 IDs, including the built-in catalog.
Legacy custom `"ground"`/`"snap"` enable values remain readable. Duplicate IDs,
invalid profiles or failed migration stop aim initialization and log a warning;
other QOL features remain available and original files are retained for recovery.

On upgrade, the plugin imports old main-file skill sections and existing mod R3
overrides, verifies equivalent skill behavior, then removes only the migrated
sections from the main file. Shared setting bytes remain intact; backups and a
legacy import archive preserve the originals. See [migration and recovery](docs/SKILL-DISCOVERY-CONFIG.md).

## Install or update

1. Close Diablo II: Resurrected and D2RLoader.
2. Back up the existing DLL and configuration.
3. Remove or disable any older `QOL.dll` so only one Controller QOL plugin is
   active.
4. Copy `d2rloader/plugins/Controller QOL Updates.dll` into the loader's active
   plugins directory.
5. Launch through D2RLoader. A missing main file is created from embedded
   shared defaults; QOL creates/migrates the active skill profile when aim initializes.

The plugin ID is `controller-qol-updates`, so its configuration file is
`controller-qol-updates.toml`. If upgrading from the older `qol` identity,
rename `qol.toml` and update any explicit loader ordering entry that names it.
Runtime ZIPs put reference settings under `defaults/`, outside the live config
folder. Extracting an update preserves existing settings; first installs use the
DLL's embedded defaults. Do not manually copy a reference over a customized file.

Mod-aware skill discovery and separate per-mod overrides are described in
[Skill discovery and configuration](docs/SKILL-DISCOVERY-CONFIG.md).

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

### Skill-tree aim shortcut

Highlight a skill in the controller skill tree and click R3 to toggle its aim
setting. A control-strip `Auto-aim: ON/OFF` hint shows the highlighted skill's setting.
The change takes effect immediately and is saved to the active mod skill profile;
comments, tuning and other skills remain intact. These preferences apply to characters using the same mod profile. Hold does not repeat; release before pressing
again. A save failure leaves the runtime setting unchanged and shows an error.

`[aim] skill_tree_toggle_enabled = true` enables this shortcut by default.
Set it false to disable the shortcut. Catalog skill IDs are accepted regardless
of character class. Discovered or manually declared custom IDs can also be toggled.
General Skills indicators follow each live button's ID and geometry as equipment
rearranges the tab. Enabling an untested skill
does not establish that its provisional ground/snap behavior is appropriate.
The shortcut reads the existing controller input snapshot on the UI thread and
leaves native button delivery intact; R3 is currently blocked by the native tree.
See [skill-tree compatibility evidence](docs/SKILL-TREE-AIM-TOGGLE.md) and
[General Skills / OSkill support](docs/GENERAL-SKILLS-OSKILLS.md).
The reviewed General representation excludes native utility actions and
item-specific/charged variants; unknown widget layouts retain native behavior.

Rev.65 defaults offensive ground placement (including Fire Blast, Shock Web,
Fissure, Volcano and sentries) to enemy snapping when enabled. A ground-cast
spell can use an enemy's position as its destination. To retain exact cursor
placement, set that numeric ID to `"ground"` in `[aim.targeting]`; R3 preserves
this preference. Movement, wall and minion placement keep their existing defaults.
Telekinesis uses a separate native unit-selection path for enemies, items and
objects; it does not display a custom object lock marker. These new behaviors
are local beta candidates awaiting live validation.

Mod-renamed skills use their numeric ID even when the comment has a vanilla name.
For example, the inspected Reimagined Ladder version maps Ice Barrage to 253
(Psychic Hammer in this catalog), while Shock Web remains 256. See
[the special-skill investigation](docs/SKILL-CATALOG.md).

Rev.67 adds optional experimental projectile leading. Set estimated travel time
in milliseconds per world tile for individual numeric skill IDs:

```toml
[aim.leading]
"47" = 50   # Fireball
"84" = 50   # Bone Spear
"251" = 100 # Fire Blast
"253" = 100 # Ice Barrage in the inspected Reimagined version
```

Values are integers from 0 to 200 in the skill profile; zero explicitly disables
leading. Missing values use profile initialization defaults (zero for new, unreviewed skills). These are
initial tuning estimates, not measured flight times. The skill must also be
enabled and use snap targeting. R3 preserves its leading preference. Prediction
requires consistent recent movement, defaults to caps of 600 ms and three tiles, and falls
back to current-position snapping when samples are unreliable. The lock marker
stays on the enemy. Telekinesis, ground-only targeting and Whirlwind do not use
leading. Rev.71 ships Reimagined-oriented baselines for all 240 IDs: 42
projectile estimates and 198 zero-lead settings. Values apply only when a skill
is enabled for snap aim. Existing leading preferences are preserved during
profile migration. Discovery identifies candidate skills, but does not calculate
projectile speed or qualify replacement targeting behavior.
See [baseline scope and variant limits](docs/LEADING-BASELINES-REV71.md) and
[the evidence and limitations](docs/PROJECTILE-LEADING.md).

Rev.69 allows per-skill prediction limits when a slow projectile reaches those
caps. Both maps use integer values and preserve the defaults for omitted IDs:

Keep enable state and tuning in separate sections: `"6" = true` under
`[aim.amazon]` enables Magic Arrow, while `"6" = 50` under `[aim.leading]` sets
its travel estimate. The numeric ID links them. Class/name comments can group
the leading entries without changing their TOML section. R3 only rewrites the
enable state, preserving targeting and leading preferences.

```toml
[aim.leading_max_ms] # 100..1500 milliseconds
"39" = 1200 # Ice Bolt
"45" = 1200 # Ice Blast
[aim.leading_max_tiles] # 1..8 world tiles
"39" = 6
"45" = 6
```

These limits do not enable leading or the skill itself. Larger predictions are
more sensitive to enemy turns. Movement estimates follow observed enemy speed,
including slowing/stopping; projectile-speed debuffs such as Slow Missiles are
outside this revision's scope.

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
reticle_thickness = 2.0            # double the colored stroke width
```

Colors accept `"#RRGGBB"` or `"#RRGGBBAA"`; the last two digits set opacity.
Thickness ranges from 0.5 to 4.0 and changes stroke width, not reticle size or
snapping radius. A narrow dark border stays proportional to screen resolution
instead of growing with the colored stroke. The ground marker keeps configured
opacity both free and locked. Borders are drawn for the complete shape before
any colored strokes, so adjacent borders cannot darken the colored joints.
The lock has slightly larger open corners; the ground ring stays the same size.
There is no glow or flashing, and drawing still uses two passes per segment.
Screen-motion smoothing filters the final projected positions, keeping player
and camera movement together. Cursor-offset changes from the right stick apply
immediately; target changes, large jumps and stale views reset visual history.
Higher smoothing values reduce small display jumps but let moving lock markers
trail slightly. Cast coordinates, snap selection and leading remain unfiltered.
Shipped defaults are #8FE8FF for the cursor, #FFD166 for the lock, and thickness 2.0. The tree marker uses a
brighter gold and slightly thicker stroke independently of gameplay styling.

### Ladder plugin coexistence

The candidate supports the reviewed Global Chat request-sender hook and Maps
stat-reader hook while retaining both plugins' forwarding behavior. Standard
offline and non-Ladder installations keep their original paths and require
neither plugin. Compatibility is tied to the reviewed plugin builds; unknown
hooks are refused. See [the compatibility record](docs/REMOTE-ID-TRANSFER-TRACE.md#2026-10-07-ladder-forwarding-compatibility-candidate)
for evidence and the remaining live checks.

### Aim diagnostic logging

`[aim] verbose = false` is the default, including existing configs that omit it.
It skips detailed candidate, lookup, leading, scoring, cast, Teleport, projection,
overlay and skill-toggle log messages and their logging-only counters/formatting.
A concise startup status and compatibility/save-failure warnings remain visible.
Set `verbose = true` and restart only when collecting aim diagnostics; existing
trace limits still apply. `debug_overlay` controls the HUD separately and does
not turn on file logging. `[qol] debug_logging` and `portal_diagnostics` remain
independent. Aiming, NPC category validation and cast-observer state still run.

This reduces diagnostic work; Steam Deck frame-time improvement has not yet
been measured, and other causes of handheld performance remain possible.
