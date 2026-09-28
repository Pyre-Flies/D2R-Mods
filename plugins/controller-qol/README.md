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
| `ground_pickup_distance` | `6` | Direct-loot search distance, clamped to 1–20 game units |
| `block_filtered_pickup` | `true` | Prevent native pickup of ground items without an active visible label |
| `prioritize_portals` | `true` | Prefer eligible portals over ground loot for neutral A |
| `prioritize_stash_boxes` | `true` | Prefer the town stash over ground loot for neutral A |
| `prioritize_waypoints` | `true` | Prefer eligible waypoints over ground loot for neutral A |
| `prioritize_shrines` | `true` | Prefer shrines and supported wells/fountains over ground loot for neutral A |
| `prioritize_chests` | `false` | Prefer supported ordinary chests over ground loot for neutral A |
| `portal_priority_distance` | `10` | Shared priority-object range, clamped to 1–20 game units |
| `debug_logging` | `false` | Enable general diagnostic logging |
| `portal_diagnostics` | `false` | Enable targeted, throttled object-priority diagnostics independently of general debug logging |

`modifier` controls item actions; `ground_pickup_button` independently controls
the seven world-loot chords. Keeping both at LB gives the default layout shown
above.

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
[`PRODUCTION-1.3.1-rev.48.md`](docs/PRODUCTION-1.3.1-rev.48.md) for the current
release record and `SHA256SUMS` in the release archive for file verification.

## Build from source

Use an x64 Visual Studio Developer Prompt with CMake 3.29+ and the Windows SDK:

```powershell
cmake -S plugins/controller-qol -B build/controller-qol -A x64
cmake --build build/controller-qol --config Release
ctest --test-dir build/controller-qol -C Release --output-on-failure
```

The output is `Controller QOL Updates.dll`. The repository pins the D2RLoader
PluginSDK as a submodule; clone with submodules or initialize it before building.
