# D2RLoader Mods

Quality-of-life plugins for Diablo II: Resurrected by PyreFly.
Download individual runtime ZIPs from the
[latest stable release](https://github.com/Pyre-Flies/D2R-Mods/releases/latest).

## Plugins

| Plugin | Version | What it adds |
| --- | --- | --- |
| [Controller QOL Updates](plugins/controller-qol/README.md) | `1.3.1+rev.92` | Controller item shortcuts, direct ground looting, menu navigation, and right-stick skill aiming. |
| [Ultrawide](plugins/ultrawide/README.md) | `1.3.5+rev.2` | 21:9 and 32:9 ultrawide viewport support: pillarbox/vignette removal, mouse/placard/automap unclamp, centered or full-length HUD, corner HUD anchoring, 22×22 terrain cache, 7×7 monster pop-in fix, and configurable AI range. |
| [Item Roll Ranges](plugins/item-roll-ranges/README.md) | `1.3.1+rev.55` | Stat roll ranges, prefix/suffix/runeword/socket source breakdowns, base damage/Defense, item levels (`Ilvl`/`Qlvl`), and max socket capacity. |
| [Gem Bag UI](plugins/gem-bag-ui/README.md) | `1.3.1+rev.2` | Interactive gem picker for withdrawing gems and one-step shortcut for depositing loose gems and gem clusters. |
| [Inventory Deposit](plugins/inventory-deposit/README.md) | `1.3.1+rev.13` | Keyboard/mouse **Deposit All** button beside inventory gold for moving eligible gems, runes, materials, and rejuvenation potions into advanced storage. |
| [Floating Damage](plugins/floating-damage/README.md) | `1.3.1+rev.13` | Local single-player enemy HP-loss numbers with elemental damage colors and critical-hit emphasis. |
| [Always Show Sockets](plugins/always-show-sockets/README.md) | `1.3.1+rev.2` | Continuous or hover-based styled socket display with configurable size, opacity, ring, and lift. |
| [Day Night Cycle](plugins/day-night-cycle/README.md) | `1.3.1+rev.1` | Restores natural time-of-day lighting progression with live chat controls for auto, day, and night modes. |
| [Map Assistance](plugins/map-assistance/README.md) | `1.3.1+rev.1` | Configurable area layout and navigation tips displayed on normal `Keys` tooltips outside town. |

### Companion Packages

| Package | Version | Description & Source |
| --- | --- | --- |
| **UnHoarder** | `1.2.16` | Separate Reimagined upgrade package maintained in the [UnHoarder fork](https://github.com/Pyre-Flies/d2rl-unhoarder). |
| **Buff HUD** | `1.1.1` | Separate DLL + MPQ companion package maintained in the [Buff HUD fork](https://github.com/Pyre-Flies/d2rl-buff-hud) with the controller/keyboard switching fix. |

See [companion sources](docs/COMPANION-SOURCES.md) for details on both companion repositories. Experimental and diagnostic plugins are maintained separately in research.

---

## Installation and Updating

1. **Extract the plugin archive:** Close the game, back up any previous DLLs, and extract each selected ZIP into your game directory (for global use) or active mod directory (for mod-specific use).
   - Plugin `.dll` files belong in `d2rloader/plugins/`. Keep only one active copy of each plugin.
   - If installing **Ultrawide**, ensure the `data/hd/tc22sh/` shader folder is extracted alongside `d2rloader/`.
   - For launcher-managed mod packs (such as D2R Reimagined), you can also use the launcher's normal mod/plugin update flow.
2. **Launch through D2RLoader:** Missing configuration files are created automatically in your active `d2rloader/config/` folder on first launch.
3. **Customizing settings:** Files inside `defaults/` in a release ZIP are reference copies so extracting an update never overwrites your active settings. To customize a plugin, edit its file under `d2rloader/config/` with the game closed and restart (or use the plugin's in-game chat commands where supported).

---

## Controls and Chat Commands by Plugin

Enter slash commands in normal game chat (feedback is printed in the D2RLoader console). Click any plugin name below for its full guide, configuration keys, and examples.

| Plugin | Control or Chat Command | What it does |
| --- | --- | --- |
| **[Controller QOL Updates](plugins/controller-qol/README.md)** | `Hold LB` (near ground loot) | Show up to 7 direct-loot button assignments (`LB + A/X/Y/B/RB/RT/LT`). |
| | `LB + A / X / Y / R3 / L3` | Identify items, transfer to/from stash or Cube, fill belt/tomes, or bulk-deposit to advanced storage. |
| | `/qol reload` · `/qol perf` | Reload supported live settings or write a read-only performance report in the loader console. |
| **[Item Roll Ranges](plugins/item-roll-ranges/README.md)** | `Hold Ctrl` or `Hold RT/R2` | Show roll ranges and affix sources on item tooltips (`RB/R1` when Controller QOL's menu remap is active). |
| | `/AlwaysShowRolls on` · `off` | Toggle continuous roll-range display without holding a button and save the preference. |
| **[Floating Damage](plugins/floating-damage/README.md)** | `/damagenumbers on` · `off` | Enable or hide floating enemy HP-loss numbers and save the setting. |
| | `/damagenumbers motion on` · `off` | Switch between curved drift and straight-rise number animation and save the setting. |
| **[Always Show Sockets](plugins/always-show-sockets/README.md)** | `/sockets always` · `default` · `off` | Show styled sockets continuously, on hover/selection only, or use native socket display. |
| | `/sockets size 65` · `opacity 65` | Adjust socketed gem/rune/jewel artwork size or opacity (`10`–`100%`) and save the setting. |
| **[Day Night Cycle](plugins/day-night-cycle/README.md)** | `/daynight auto` · `day` · `night` · `off` | Use the natural time-of-day cycle, hold fixed noon/night lighting, or disable lighting updates. |
| **[Gem Bag UI](plugins/gem-bag-ui/README.md)** | `Right-click` bag / `LB + A` | Open the interactive gem picker to withdraw an identified gem. |
| | `Shift + Right-click` / `LB + B` | Deposit compatible gems and gem clusters from the current storage page into the bag. |
| **[Map Assistance](plugins/map-assistance/README.md)** | Hover `Keys` outside town | Display area layout and navigation notes directly on the Keys item tooltip. |
| **[Inventory Deposit](plugins/inventory-deposit/README.md)** | Click **Deposit All** (stash open) | Move eligible inventory gems, runes, materials, and rejuvenation potions into advanced storage. |
| **[Ultrawide](plugins/ultrawide/README.md)** | Configured in `ultrawide.toml` | Automatic during gameplay; configure HUD mode, corner HUD anchoring, terrain cache, pop-in radius, and AI range in `d2rloader/config/ultrawide.toml`. |

---

## Multiplayer and Compatibility Notes

- **Client vs. Host Requirements:**
  - **Gem Bag UI** requires the same plugin version and mod data on both client and host in multiplayer games.
  - **Floating Damage** and **Ultrawide**'s monster pop-in / AI perception adjustments run on the local game server (single player or local host).
  - **Day Night Cycle**, **Always Show Sockets**, **Item Roll Ranges**, **Map Assistance**, and **UnHoarder** run entirely on the client.
- **Safe Native Guards:** Each plugin validates the exact native instructions it interacts with at startup and disables unsupported hooks cleanly if a game or mod update changes the underlying binary.

---

## Support

Report bugs at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues). Please include your plugin, game, and mod versions along with reproduction steps, and review any log excerpts for personal information before sharing. Release downloads include `SHA256SUMS` alongside the ZIP archives for verification.

If you enjoy these plugins, you can support their development on Ko-fi:

[![Support on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D3J128U0X2)

## Repository Navigation

Source code and automated test suites live under `plugins/`. For development and release documentation:
- [Documentation Map](docs/README.md) — developer guides and native contracts for every plugin
- [Contributing Guide](CONTRIBUTING.md) & [Release Process](docs/RELEASING.md) — build, test, and packaging workflows
- [Agent Instructions](AGENTS.md) & [Workspace Guide](docs/WORKSPACE.md) — repository conventions and local workspace layout
