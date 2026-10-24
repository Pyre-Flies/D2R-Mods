# Ultrawide

**1.3.5+rev.2 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Unlocks full-viewport ultrawide (21:9 and 32:9) gameplay in Diablo II: Resurrected while keeping menus framed cleanly:

- **Pillarbox & Vignette Removal**: Removes the left/right black bars and widens the vignette mask during active gameplay while keeping main/character menus at native framing.
- **Horizontal Mouse Unclamp**: Unclamps horizontal cursor coordinates during gameplay so clicking, movement, and skill targeting reach the far left and right ultrawide edges.
- **Ground Loot Item Placard Unclamp**: Unclamps ground item placards/labels across the full ultrawide viewport while still narrowing bounds cleanly when a left or right UI panel (Character, Inventory, Stash, etc.) is open.
- **Full-Screen Automap Overlay Unclamp**: Expands the full-screen Automap overlay (both GPU scissor rectangle and isometric world-tile/icon culling bounds) across the entire ultrawide viewport.
- **Centered vs. Full Length HUD & Top-Corner HUD Anchoring**: Choose between **Centered HUD** (`enable_full_length_hud = false`, keeping the bottom HUD bar and left/right inventory/stash panels inside 19:9) and **Full Length HUD** (`enable_full_length_hud = true`, expanding `UIRootWidget` across the full ultrawide viewport during gameplay), plus independent toggles (`enable_corner_minimap`, `enable_corner_mercenary`, `enable_corner_game_info`) to anchor the Corner Minimap, Mercenary/Party portraits, and top-right Game Information to the true ultrawide screen corners even when Centered HUD is active.
- **22x22 Terrain Cache Expansion**: Expands the HD terrain tile cache from 18x18 (324 cells) to 22x22 (484 cells) with paired 22x22 terrain shaders (`data/hd/tc22sh/`) and 2-hop fallback-ground room supply so ground tiles render cleanly to the ultrawide corners. Works in both mods (such as D2R Reimagined) and Vanilla.
- **Monster Pop-In Fix (7x7 Active Room Window)**: Expands the local-server active room neighborhood around the player from 3x3 (1-hop) to 7x7 (3-hop, configurable via `monster_pop_in_radius = 1..3`) so monsters at the ultrawide edges spawn smoothly and do not pop in or vanish abruptly even across long-distance Teleports or high movement speed.
- **Configurable Monster AI Perception Range**: Increases default hostile monster perception range from 35 subtiles to 65 subtiles (configurable from 35 to 100) so enemies near the ultrawide edges react naturally instead of standing idle until you walk closer.

## Installation and updates

1. Close the game and back up any previous `Ultrawide.dll` (and remove experimental `Ultrawide Bars.dll` or `Terrain Coverage Probe.dll` if previously installed).
2. Extract the release archive into your game directory (for global/Vanilla use) or your active mod directory (for mod-specific use):
   - `Ultrawide.dll` goes in `d2rloader/plugins/`.
   - The `data/hd/tc22sh/` folder (14 `.bin` shader files) goes in `d2rloader/data/hd/tc22sh/` (or `<activeMod>/<activeMod>.mpq/data/hd/tc22sh/`).
3. Start the game through D2RLoader. If `d2rloader/config/ultrawide.toml` does not exist yet, it will be created automatically from the embedded default config.

All settings in `ultrawide.toml` are **startup-only**. Edit `d2rloader/config/ultrawide.toml` with the game closed and restart to apply changes.

## Configuration (`ultrawide.toml`)

All keys belong under `[ultrawide]`:

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enable_bars_removal` | `true` | Remove left/right pillarbox black bars and widen the vignette mask during gameplay while keeping menus framed at 19:9. |
| `enable_mouse_unclamp` | `true` | Unclamp horizontal mouse coordinates during gameplay so cursor picking and targeting work in the outer ultrawide wings. |
| `enable_placard_unclamp` | `true` | Unclamp ground loot item placards/labels horizontally across the full ultrawide viewport during gameplay. |
| `enable_automap_unclamp` | `true` | Unclamp the full-screen Automap overlay across the full ultrawide viewport during gameplay. |
| `enable_full_length_hud` | `false` | `false` = **Centered HUD** (keeps bottom HUD bar and left/right inventory/stash panels inside 19:9); `true` = **Full Length HUD** (expands `UIRootWidget` across the full ultrawide width during gameplay while keeping front-end menus at 19:9). Also accepts `hud_mode = "centered"` or `"full"`. |
| `enable_corner_minimap` | `true` | Push the Corner Minimap (top-left or top-right) and its fade buttons to the true ultrawide screen corner even when Centered HUD is active. |
| `enable_corner_mercenary` | `true` | Push the Hired Mercenary / Party portrait area (`HireablesPanel`) to the true ultrawide top-left corner even when Centered HUD is active. |
| `enable_corner_game_info` | `true` | Push the top-right Game Information (`HUDWarningsPanel`: Area Name, Terror Zone info, Difficulty, Game Name/Password, Clock) to the true ultrawide top-right corner even when Centered HUD is active. |
| `enable_terrain_expansion` | `true` | Expand the terrain cache from 18x18 to 22x22 cells using the 14 paired `tc22sh` shaders and 2-hop fallback-ground neighbor supply. |
| `enable_monster_pop_in_fix` | `true` | Expand the local-server active room window so monsters do not pop in or despawn inside the ultrawide viewport. |
| `monster_pop_in_radius` | `3` | Active room hop radius around the player's room (`1` = `3x3` vanilla, `2` = `5x5`, `3` = `7x7` default for Teleport/high-FRW coverage). |
| `enable_monster_ai_range` | `true` | Scale hostile monster AI target-perception distance up to `monster_ai_range`. |
| `monster_ai_range` | `65` | Hostile monster perception distance in subtiles (integer `35`–`100`; vanilla is `35`). Bosses with larger `monstats.txt` `aidist` keep their larger distance. |
| `scale_friendly_ai_range` | `false` | Also scale friendly/mercenary/summon AI target scan distance from `35` to `monster_ai_range`. |

### Default configuration example

```toml
[ultrawide]
enable_bars_removal = true
enable_mouse_unclamp = true
enable_placard_unclamp = true
enable_automap_unclamp = true
enable_full_length_hud = false
enable_corner_minimap = true
enable_corner_mercenary = true
enable_corner_game_info = true
enable_terrain_expansion = true
enable_monster_pop_in_fix = true
monster_pop_in_radius = 3
enable_monster_ai_range = true
monster_ai_range = 65
scale_friendly_ai_range = false
```

## Compatibility and validation

- Requires the supported `D2R.exe` and `D2RCore.dll` (`1.3.1`, ABI 4) native profile documented in [NATIVE-CONTRACT.md](NATIVE-CONTRACT.md). On unsupported builds, the plugin logs a warning and remains inert.
- Five automated CTest suites verify the DLL manifest, embedded config, exports, TOML parser/clamping, assembly gates (`UltrawideMenuEntry`, `UltrawideMouseEntry`, `UltrawideRoomDiffEntry`), 2-hop room reconciliation, AI patch encoding, 22x22 arena relocation/unwind registration, cross-mod/Vanilla shader path resolution, and GroundSupply room extension.
- Monster pop-in and AI perception adjustments run on the local `D2Game` server (offline singleplayer / local host).
