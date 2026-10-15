# Controller QOL Updates

**1.3.1+rev.79-beta.2 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Controller item shortcuts, direct ground looting, menu navigation and optional
right-stick skill aiming. This README covers the default button combinations,
all shared settings and skill-profile editing. There are no chat commands.

This beta repairs remote item actions blocked after Global Chat updates. The
shared compatibility check covers stash/Cube transfers, bulk advanced-storage
deposits, Quick Identify and Identify All. Automated checks passed; live remote
transfers, identification and persistence still need testing. See the
[beta test checklist](../../docs/CONTROLLER-QOL-BETA.md), and report all bugs.
Existing settings and controller mappings are preserved.

- [Install and update](#installation-and-updates)
- [All default controller shortcuts](#default-controller-shortcuts)
- [Controller aiming](#controller-aiming)
- [Find the active configuration](#configuration-files)
- [All shared settings](#shared-settings-reference)
- [Customize skills, targeting and leading](#customize-skills)

## Installation and updates

1. Close the game and back up the previous plugin DLL and active configuration.
   The D2RR launcher can remain open.
2. Extract the ZIP into the game directory for global use, or the active mod
   directory for mod-specific use. The DLL belongs in `d2rloader/plugins/`.
   Keep one active copy of each plugin. Managed mod packs should use their
   normal updater.
3. Start through D2RLoader. Missing main configs are created automatically in
   the active `d2rloader/config/` directory.

Files under `defaults/` are references. Extracting an update preserves existing
settings; editing a reference does not change gameplay. Edit the active file
with the game closed, change existing keys instead of duplicating them, and
restart to apply manual edits. TOML booleans are lowercase `true`/`false`,
strings use quotes, and `#` starts a comment outside a quoted string.

To uninstall or roll back, close the game, remove the DLL or restore its backup,
then restart. Hot reload is unsupported.

## Default controller shortcuts

Xbox names are used below. PlayStation equivalents are LB/RB = L1/R1,
LT/RT = L2/R2, A = Cross, X = Square, Y = Triangle and B = Circle.
L3/R3 mean clicking the left/right stick. The default item-action and ground-loot
modifiers are both LB; their configuration settings are independent.

### World and ground loot

| Shortcut | Context | Action |
| --- | --- | --- |
| Hold LB | Nearby visible ground labels | Show up to seven direct-loot assignments. |
| LB + A | Assignment labeled A | Pick up that item. |
| LB + X | Assignment labeled X | Pick up that item. |
| LB + Y | Assignment labeled Y | Pick up that item. |
| LB + B | Assignment labeled B | Pick up that item. |
| LB + RB | Assignment labeled RB | Pick up that item. |
| LB + RT | Assignment labeled RT | Pick up that item. |
| LB + LT | Assignment labeled LT | Pick up that item. |
| A without LB | Normal world interaction | Prefer an enabled nearby portal, stash, waypoint, shrine/well or chest; otherwise retain normal interaction. |

Assignments stay fixed for one modifier hold. Release and hold again to refresh
them after drops change. An assigned reachable item can use native movement
when it is beyond pickup reach. Releasing LB removes the assignment overlay;
it does not automatically collect anything. Filtered items with no visible
label are excluded. Gold retains native collection rules.

### Items, storage, belt and merchants

| Shortcut | Focus/context | Action |
| --- | --- | --- |
| LB + A | Unidentified item | Identify using an eligible tome or scroll. |
| LB + A | Identify Tome | Identify eligible inventory items; offline also includes Cube contents. |
| LB + A | Supported potion in inventory, Cube, Personal Stash or normal Shared Stash | Put the potion into available belt space. |
| LB + A | Rejuvenation counter on Materials | Withdraw one directly into available belt space. |
| LB + X | Inventory with storage open | Transfer to the active storage context. |
| LB + X | Personal, Shared, custom-stash or Cube item | Transfer back to inventory. |
| LB + X | Inventory beside Gems/Materials/Runes storage | Store eligible items in advanced storage; send other eligible inventory items to the embedded Cube. |
| LB + X | Embedded Cube beside Gems/Materials/Runes | Deposit eligible Cube contents into advanced storage; other items stay in the Cube. |
| LB + X | Inventory with NPC shop open | Sell the item using native vendor rules. |
| LB + Y | Eligible carried or stored item | Move directly into the carried Horadric Cube. |
| LB + Y | Rune, gem or material counter | Withdraw one directly into the carried Cube. |
| LB + R3 | Potion in inventory or Materials storage | Fill available belt slots. |
| LB + R3 | Merchant potion | Refill belt space and buy missing potions where native rules allow. |
| LB + R3 | Merchant Identify or Town Portal scroll | Fill a matching, non-full tome in inventory. |
| LB + L3 | Stash open | Deposit eligible inventory materials, gems, runes and rejuvenation potions into advanced storage. |

Tooltips show actions available for the focused item. Native eligibility, gold,
capacity and item rules still apply. The Cube cannot be put inside itself.
Bulk actions can stop partway through; check the result before repeating them.
Avoid starting another plugin's bulk action while a transfer is pending.

Remote Identify All covers main inventory and requires an eligible highlighted
Identify Tome in Inventory or open Personal Stash. Remote single-item identify
supports Inventory/Personal Stash targets and supplies from those containers;
Cube/Shared supplies and modded scroll stacks are excluded. Offline Identify All
retains inventory/Cube coverage. Advanced-storage features require a mod that
provides that storage.

### Menus, stash pages and filter rules

| Shortcut | Screen | Action |
| --- | --- | --- |
| LT / RT | Main menu pages | Previous / next main page. |
| LB / RB | Skill Tree | Previous / next skill tab. |
| LB / RB | Quest Log | Previous / next act tab. |
| LB / RB | Chronicle | Previous / next inner tab. |
| LB / RB | Options | Previous / next inner tab. |
| LB / RB | Mod Information | Previous / next information tab; triggers do not change the background page. |
| LB / RB | Loot-filter rule editor | Switch Equipment / Items inner tabs. |
| LB + Up / Down | Loot-filter rule list | Move the focused rule up / down. |
| Down | Focused filter tab | Enter the rule list when native focus is missing. |
| LB + LT / LB + RT | Shared Stash | Previous / next Shared Stash page. |
| RB | Item tooltip | Show native roll ranges while the menu remap is active. |
| R3 | Supported skill in class or General Skills tabs | Toggle aim for that skill and save its profile. |

## Controller aiming

Move the right stick beyond its dead zone to activate and move the cursor.
Holding accelerates; releasing the stick retains the position. An unsupported
or disabled skill restores native targeting. Release and tilt the stick again
to resume manual aim. Mouse/keyboard casts keep their normal route.

Ground-mode skills use the cursor. Snap-mode skills can select an eligible
nearby enemy and fall back to the cursor. Teleport and Leap use ground placement.
Whirlwind can retain a target and extend through it by a configurable distance.
Guided Arrow ground casts also retain the plugin's distance correction when
manual aim is off.

| Control | Effect |
| --- | --- |
| Right stick | Activate/move the cursor; release to retain position. |
| R3 on a supported skill | Save its enabled/disabled aim preference immediately. |
| F8 | Toggle aim for this session, when configuration allows it. |
| F9 | Recenter ahead of the character and release the current target. |
| F10 | Invert right-stick Y for this session. |

Keyboard bindings appear under **Controller QOL Aim** in loader controls.
F8/F10 are session controls, separate from saved skill preferences. The cyan
cursor, gold target corners and Whirlwind endpoint ring are aiming indicators.
Hiding reticles does not disable aim. Skill-icon crosshairs and the R3 hint
describe saved skill preferences, rather than whether the stick currently owns
the cast.

## Configuration files

All paths below are relative to the active `d2rloader/config/` directory.

| File | Edit for |
| --- | --- |
| `controller-qol-updates.toml` | Shared `[qol]` actions and `[aim]` speed, snapping, reticles and logging. |
| `controller-qol-skills/<scope>.overrides.toml` | All skill enable states, targeting, leading and Whirlwind options for one mod or Vanilla. |
| Matching `<scope>.catalog.toml` | Generated reference names/IDs/defaults. Editing it does not change settings. |

Start the intended mod once to create its skill profile, then close the game.
Find the `.overrides.toml` whose `# Mod:` header matches your pack or `Vanilla`.
R3 saves to that same file. Profiles are shared by characters using that mod
and config scope. Moving or renaming the mod directory can select a new profile.

Manual edits require a restart. R3 takes effect after a successful save; a failed
save retains the previous state. Existing choices survive updates. New keys can
be added without replacing choices already present. Do not copy the generated
catalog or the entire defaults file over your profile.

## Shared settings reference

### `[qol]` in `controller-qol-updates.toml`

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Master switch for QOL actions; controller aiming also requires this. |
| `quick_identify` | `true` | Enable focused-item identify. |
| `identify_all` | `true` | Enable Identify All on a focused Identify Tome. |
| `native_identify` | `false` | Use the slower native hold-identify route instead of the default fast route. |
| `quick_move` | `true` | Enable transfer, Cube, belt and vendor shortcuts. |
| `quick_deposit` | `true` | Enable the stash Deposit All shortcut. |
| `require_tome_or_scroll` | `true` | Require an eligible identify supply. |
| `consume_tome_or_scroll` | `true` | Consume an identify charge for each successful identification. |
| `require_modifier` | `true` | Require the modifier for A-button item actions; X/Y/stick chords remain explicit shortcuts. |
| `modifier` | `"bumper"` | Item-action modifier; see aliases below. |
| `trigger_threshold` | `30` | Reference/reserved setting; this version uses 30. |
| `ground_pickup` | `true` | Enable labeled direct-loot assignments. |
| `ground_pickup_button` | `"bumper"` | Independent modifier for ground-loot assignments; see aliases below. |
| `ground_pickup_distance` | `6` | Nearby assignment distance, clamped 1-20; effective minimum is 10. Already mapped items may use native travel beyond it. |
| `debug_logging` | `false` | General diagnostic logging; leave false for ordinary play. |
| `block_filtered_pickup` | `true` | Block controller A pickup for hidden-label items. Gold and mouse/keyboard pickup retain native behavior. |
| `prioritize_portals` | `true` | Prefer an eligible nearby portal for ordinary A interactions. |
| `prioritize_stash_boxes` | `true` | Prefer the town stash. |
| `prioritize_waypoints` | `true` | Prefer waypoints. |
| `prioritize_shrines` | `true` | Prefer shrines and supported wells/fountains. |
| `prioritize_chests` | `true` | Prefer supported ordinary chests. |
| `portal_priority_distance` | `10` | Shared priority-object radius, clamped 1-20 game units. |
| `portal_diagnostics` | `false` | Targeted object-interaction diagnostics, independent of general logging. |

For item `modifier`, common aliases are `"bumper"`/`"lb"`/`"l1"`,
`"trigger"`/`"lt"`/`"l2"`, `"rb"`, `"rt"`, `"l3"`, `"r3"`, or `"any"`.
Ground pickup accepts bumper/trigger/stick aliases, single keyboard letters or
digits, and `"["`/`"l4"`/`"bracket"`/`"leftbracket"` or
`"]"`/`"r4"`/`"rightbracket"` keyboard mappings. Paddle aliases require a
controller mapping that emits the corresponding bracket key.

Example: edit these existing keys to move item actions to LT while keeping
ground-loot assignments on LB:

```toml
[qol]
modifier = "lt"
ground_pickup_button = "lb"
quick_identify = true
quick_deposit = true
block_filtered_pickup = true
```

### `[aim]` in `controller-qol-updates.toml`

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Enable right-stick aiming; also requires `[qol] enabled = true`. |
| `verbose` | `false` | Detailed aim log output; leave false unless diagnosing a problem. |
| `deadzone` | `0.22` | Radial stick dead zone, 0-0.9. |
| `initial_speed` | `8.0` | Initial full-tilt cursor speed, 0.1-100 world tiles/second. |
| `maximum_speed` | `48` | Maximum held-stick speed; at least initial_speed and at most 100. |
| `acceleration_seconds` | `0.35` | Time to reach maximum speed, 0-5 seconds; 0 is immediate. |
| `snapping_enabled` | `true` | Allow enabled snap-mode skills to select an eligible enemy near the cursor. |
| `snap_radius` | `6.0` | Enemy acquisition radius around the cursor, 0.5-15 tiles. |
| `switch_advantage` | `1.5` | How much closer another enemy must be before changing target, 0-15 tiles. |
| `overlay_enabled` | `true` | Show reticles; hiding them does not disable aiming. |
| `ground_reticle_color` | `"#8FE8FF"` | Cursor color, quoted #RRGGBB or #RRGGBBAA. |
| `lock_reticle_color` | `"#FFD166"` | Enemy-lock color with optional alpha. |
| `reticle_thickness` | `2.0` | Colored stroke multiplier, 0.5-4; does not change targeting radius. |
| `debug_overlay` | `false` | Show the diagnostic HUD and extra aim indicators. |
| `cast_observer_enabled` | `true` | Enable optional cast diagnostics/reset observation. Can be disabled for compatibility while keeping ordinary aim. |
| `skill_tree_toggle_enabled` | `true` | Allow R3 on a supported skill to change and save its aim preference. |
| `projection_hz` | `60.0` | Projection sampling frequency, 10-120; actual updates are limited by UI rate. |
| `overlay_smoothing_ms` | `60.0` | Display-only smoothing, 0-150 ms. Zero disables it; high values add visible lag. |

Example: use a slower cursor and a white reticle:

```toml
[aim]
enabled = true
initial_speed = 6.0
maximum_speed = 30
acceleration_seconds = 0.5
ground_reticle_color = "#FFFFFF"
lock_reticle_color = "#FFD166"
reticle_thickness = 2.0
overlay_smoothing_ms = 60.0
```

Merge into existing sections; do not duplicate their headers. `enabled = false`
under `[aim]` disables controller aiming; under `[qol]` it disables QOL actions
and also prevents aiming. Reticle smoothing affects display only; higher values
can visibly lag moving targets without changing the cast destination.

## Customize skills

All examples in this section belong in the active `.overrides.toml` skill
profile, not in `controller-qol-updates.toml`. Edit existing numeric keys and
restart. A mod can replace the behavior behind an ID, so use the active
profile/catalog names to identify the right skill.

### Skill state and sections

| Section | Values / meaning |
| --- | --- |
| `[aim.amazon]`, `[aim.sorceress]`, `[aim.necromancer]`, `[aim.barbarian]`, `[aim.paladin]`, `[aim.druid]`, `[aim.assassin]`, `[aim.warlock]` | Existing quoted numeric skill IDs: `true` enables aim, `false` leaves it off but R3-toggleable, `"disabled"` locks it off and removes its R3 hint. |
| `[aim.custom]` | Extra IDs not in the built-in catalog, with the same enable/lock values. Declare an ID here before assigning its targeting or leading. |
| `[aim.targeting]` | Per-ID `"ground"` or `"snap"`; does not enable a skill by itself. |
| `[aim.leading]` | Per-ID integer travel estimate, 0-200 milliseconds per tile. Zero disables prediction. |
| `[aim.leading_max_ms]` | Per-ID maximum prediction time, 100-1500 ms. General default 600; shipped Ice Bolt/Ice Blast values are 1200. |
| `[aim.leading_max_tiles]` | Per-ID maximum prediction distance, 1-8 world tiles. General default 3; shipped Ice Bolt/Ice Blast values are 6. |
| `[aim.whirlwind]` | `whirlwind_pass_through_enabled` (boolean, default true) and `whirlwind_pass_through_distance` (0-15 tiles, default 1.5). |

Class headings organize IDs; an equipment-granted class skill uses the same ID
and setting on another class. Keep each enable ID in only one class/custom
section. Targeting and leading maps can reference that same ID in their own
sections. Up to 1024 IDs are supported. Invalid/duplicate settings stop aim
initialization and report a warning while leaving other QOL actions available.

### Enable a skill or keep it native

For vanilla Teleport (ID 54) and Ice Bolt (ID 39), merge these choices into the
existing Sorceress section:

```toml
[aim.sorceress]
"54" = true        # Enable Teleport cursor placement
"39" = false       # Keep Ice Bolt native; R3 may enable it later

[aim.targeting]
"54" = "ground"
"39" = "snap"
```

Use `"disabled"` in place of `false` when a skill should remain locked off.
Snapping also requires the shared `snapping_enabled` option and an eligible
target; unsupported skill behavior is not inferred from its name.

### Tune projectile leading

For an enabled snap skill, the estimate anticipates enemy movement. A higher
estimate aims farther ahead. These example Ice Bolt values do not enable the
skill; enable ID 39 separately if you want to use them:

```toml
[aim.leading]
"39" = 80

[aim.leading_max_ms]
"39" = 1200

[aim.leading_max_tiles]
"39" = 6
```

Use zero in `[aim.leading]` to disable prediction for that skill. Existing
shipped estimates vary by ID. Discovery does not calculate every modded
projectile's travel speed; tune one skill at a time and compare in game.
Leading is for supported enabled snap skills, not Teleport-style ground casts.

### Add a modded skill

Recognized Reimagined Warp uses ID 429. If the active profile already lists it,
edit that entry or use R3. For a pack that does not discover it automatically,
first verify its actual ID, then merge:

```toml
[aim.custom]
"429" = true

[aim.targeting]
"429" = "ground"
```

The example ID is Reimagined-specific. Other mods can use a different ID.
An added entry does not guarantee that every custom skill supports cursor or
enemy-snap targeting.

### Whirlwind pass-through

```toml
[aim.whirlwind]
whirlwind_pass_through_enabled = true
whirlwind_pass_through_distance = 1.5
```

False or distance zero targets the enemy directly. Moving the right stick
releases target retention. These options belong in the skill profile; do not
put them back in the shared `[aim]` section.

## Upgrades and recovery

Older main-file skill choices migrate into per-mod profiles with verification
and backups. R3 keeps one rolling `<scope>.overrides.toml.r3.bak` containing the
previous saved state. Migration backups and the legacy import archive are
separate. Keep matching DLL/main-config backups when rolling back to an older
version that predates complete skill profiles.

For recovery details, see
[profile migration and recovery](docs/SKILL-DISCOVERY-CONFIG.md).
The complete shipped reference files are
[shared defaults](controller-qol-updates.toml) and
[skill defaults](skill-defaults.toml). They show every default skill ID/value;
your active profile holds your choices.

## Compatibility and validation

Requires Windows x64, D2RLoader 1.3.1 / plugin ABI 4 and supported native builds;
see [compatibility](COMPATIBILITY.txt). Disable the old standalone Controller
Aim Test plugin before using integrated aim. Another plugin changing the same
native paths can still conflict; unsupported paths retain native behavior.

Thirty-three automated suites passed. Live checks include item actions,
profiles, menu navigation and hosted ground-loot repairs. Coverage is not
exhaustive for every skill, controller, resolution or companion plugin.
An existing shutdown issue remains under investigation with its cause unconfirmed.

## Support

Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues)
with the plugin/game/mod versions and steps to reproduce. Check logs for
personal information before sharing. Download checksums are alongside the ZIPs.
