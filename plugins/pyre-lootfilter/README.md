# Pyre Loot Filter (`pyre-lootfilter`)

**Version:** `1.3.1+rev.7`

Clean-room native ground loot filter plugin for D2RLoader.

## Features

- **100% `filter.json` Compatibility (Schema v1, v2, and v3) & Multi-Profile `config/filter/` Folder**:
  - Scans `d2rloader/config/filter/*.json` (automatically creating `d2rloader/config/filter/` on startup) and supports legacy `d2rloader/config/filter.json` across both the active mod folder (`mods/<Mod>/d2rloader/`) and the global folder (`d2rloader/`), regardless of whether `pyre-lootfilter.dll` is installed in the mod or global `d2rloader/plugins/` directory.
  - Supports `code`, `baseName` (`weapons.txt` + `armor.txt`), `tier` (`normal`, `exceptional`, `elite` from `weapons.txt` + `armor.txt`), `itemType` (`itemtypes.txt` hierarchy + `quest` items, with non-fatal warnings for unknown/empty mod categories), `rarity`, `identified`, `ethereal`, `itemLevel`, `sockets`, `quantity`, and `sellPrice`.
  - Supports `show` / `hide` rules, v3 `continue: true` rule composition, custom item `name`, stack count prefixes (`"12x Item Name"`), `tooltip` (`backgroundColor`, `textColor`, and `borderColor`), `dropSound` (with `.flac` asset fallback), and `minimapIcon` (`circle`, `square`, `diamond`, `triangle`, `triangleDown`, `star`, `spark`, `kite`, `hexagon`, `cross`, `chevron`, `bullseye`).
- **Options > Gameplay Settings & Status Row (`D2RL::Settings`)**:
  - Integrates into **Escape -> Options -> Gameplay** via `D2RL::Settings` (`aa-settings-host`), providing a `Loot Filter Profile` dropdown (`None (Off)` + all discovered `.json` profiles in `d2rloader/config/filter/`), a live `Loot Filter Status` feedback row (`Filter Loaded: <Name> (<N> rules)`, `Loot Filter Disabled`, `Disabled (In-Game: <Name>)`, or `Filter Error: <reason>`), and toggles for Minimap Icons, Drop Sounds, 3D Drop Audio, Auto-Show Items, Ctrl Override, and Auto-Reload.
  - Automatically yields/disables `pyre-lootfilter` (`Disabled (In-Game: <Name>)`) whenever a built-in D2R 3.x in-game loot filter (`.fltr`) is active (`LootFilterConfig::activeProfileName` at `0x2358E20 + 0x20`).
  - Emits an in-game chat message (`WriteConsoleMessage`) and updates the `Loot Filter Status` row whenever a filter loads or fails, explaining specific error reasons (unsupported schema version, browser-library backup vs exported filter, wrong season/mod version unknown `baseName`/`stat`/`skill`, or JSON syntax line/reason).
- **Lightweight Single-Pass Runtime**:
  - Evaluates each ground item once per `(unitId, filterGeneration)` via SEH-guarded in-process `UnitAny` / `ItemData` / `StatListEx` reads (`UnitEvaluationCache`).
  - Rejects hidden items (`show: false`) directly inside `LabelFormatter` (`0x1FA9F0 -> return 0`) so hidden loot never consumes D2R's 32-label cap (`0x1517149` / `0x151A23D`) or occupies boxes in the 16-step screen-space collision solver (`0x1516A90` / `0x15190A0`).
  - Zero `ReadProcessMemory` kernel syscalls on label/glyph hooks, zero background inventory polling threads, and zero no-op detours on `0x36EF50` (`GetItemCode`) or `0x2F5020` (`GetUnitStat`).
- **Minimap Icons via `D2RL::OverlayService`**:
  - Observes ground items directly on the native automap pass (`0xD76E0` / `0xD4910`) and draws icons through D2RLoader's native `OverlayService` without bundling `MinHook` or `Dear ImGui` DX12 swapchain hooks.
  - Supports `size` up to `128` px (`4..128`, automatically clamping out-of-range values with `LOOT_MINIMAP_SIZE_CLAMPED` instead of rejecting `filter.json`) and uses adaptive thick scanlines (`<= 18` rows max) to minimize `drawLine` calls.
- **Mouse Pickup Guard + Keyboard `Ctrl` Override & Gold Pickup Preservation**:
  - Blocks accidental mouse clicks on `show: false` ground items while allowing intentional pickup when holding keyboard `Ctrl` (`VK_LCONTROL` / `VK_RCONTROL`).
  - **Gold (`code: "gld"`, `itemType: "Gold"`) is always picked up** (both native walk-over Auto-Gold via `0x193010 -> 0xFE3B0 -> 0x101ADF -> 0xFABE0` and manual pickup), even when a filter rule hides gold ground labels/drop sounds (`show: false`). Gold pile amounts are read from `STAT_GOLD` (stat `14`, supporting piles up to `4294967295`) without duplicating the native `"<amount> Gold"` label prefix.
- **Controller QOL Coexistence**:
  - Publishes `unhoarder-ground-visibility` and `unhoarder-label-visibility` (v1) so Controller QOL's `LB` visibility override and loot targeting work seamlessly.
- **Live Auto-Reload & `/filter` Commands**:
  - Automatically reloads the active filter profile when edited on disk (or via `Ctrl+Shift+F9`), preserving the last valid rules on syntax errors.
  - Optionally enables ground item labels once per game join when the loot filter is active (`auto_show_items_on_join = true` in `pyre-lootfilter.toml`).
  - Plays custom `dropSound` alerts at full non-positional volume by default (`drop_sound_3d_attenuation = false` in `pyre-lootfilter.toml`, configurable via `/filter sound3d [on|off]`).
  - Supports `/filter` (`status`, `reload`, `perf`, `perf reset`, `on`, `off`, `toggle`, `minimap [on|off]`, `sound [on|off]`, `sound3d [on|off]`, `alt [on|off]`, `autoreload [on|off]`).
