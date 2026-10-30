# D2RLoader Mods

Quality-of-life plugins for Diablo II: Resurrected by PyreFly.
Download individual runtime ZIPs from the
[latest stable release](https://github.com/Pyre-Flies/D2R-Mods/releases/latest).

## Plugins

| Plugin | Version | What it adds |
| --- | --- | --- |
| [Controller QOL Updates](plugins/controller-qol/README.md) | `1.3.1+rev.95` | Controller item shortcuts, direct ground looting, menu navigation, and right-stick skill aiming. |
| [Pyre Loot Filter](plugins/pyre-lootfilter/README.md) | `1.3.1+rev.2` | Clean-room native `filter.json` (v1/v2/v3) ground loot filter with custom label colors, stack prefixes, drop sounds, minimap icons, mouse pickup guard, and Controller QOL visibility interop. |
| [Pyre Stash Search](plugins/pyre-stashsearch/README.md) | `1.3.1+rev.1` | Instant cross-page Personal and Shared Stash search with boolean operators, stat shorthands, active-page item highlighting, and an expandable Side Tab Flyout with custom tab names. |
| [Running Defense](plugins/running-defense/README.md) | `1.3.1+rev.1` | Configurable percentage Defense penalty while running (`defense_loss_percent`), actual running Defense on the Character Sheet with a running indicator icon, and optional running Shield Block penalty override. |
| [Ultrawide](plugins/ultrawide/README.md) | `1.3.5+rev.2` | 21:9 and 32:9 ultrawide viewport support: pillarbox/vignette removal, mouse/placard/automap unclamp, centered or full-length HUD, corner HUD anchoring, 22×22 terrain cache, 7×7 monster pop-in fix, and configurable AI range. |
| [Item Roll Ranges](plugins/item-roll-ranges/README.md) | `1.3.1+rev.56` | Stat roll ranges, prefix/suffix/runeword/set/craft/socket/corruption/enchantment source breakdowns, base damage/Defense, item levels (`Ilvl`/`Qlvl`), and max socket capacity. |
| [Gem Bag UI](plugins/gem-bag-ui/README.md) | `1.3.1+rev.2` | Interactive gem picker for withdrawing gems and one-step shortcut for depositing loose gems and gem clusters. |
| [Inventory Deposit](plugins/inventory-deposit/README.md) | `1.3.1+rev.13` | Keyboard/mouse **Deposit All** button beside inventory gold for moving eligible inventory gems, runes, materials, and rejuvenation potions into advanced storage. |
| [Floating Damage](plugins/floating-damage/README.md) | `1.3.1+rev.13` | Local single-player enemy HP-loss numbers with elemental damage colors and critical-hit emphasis. |
| [Always Show Sockets](plugins/always-show-sockets/README.md) | `1.3.1+rev.2` | Continuous or hover-based styled socket display with configurable size, opacity, ring, and lift. |
| [Day Night Cycle](plugins/day-night-cycle/README.md) | `1.3.1+rev.1` | Restores natural time-of-day lighting progression with live chat controls for auto, day, and night modes. |
| [Map Assistance](plugins/map-assistance/README.md) | `1.3.1+rev.1` | Configurable area layout and navigation tips displayed on normal `Keys` tooltips outside town. |

### Companion Packages

| Package | Version | Description & Source |
| --- | --- | --- |
| **Buff HUD** | `1.3.1+rev.3` | Separate DLL + MPQ companion package maintained in the [Buff HUD fork](https://github.com/Pyre-Flies/d2rl-buff-hud) with joined-game/dedicated-server (`d2rds`) multiplayer sync, Blood Golem (`blood_mana`) opt-out toggle, hover tooltips, runtime Excel buff catalog discovery, and 25 Hz frame-time gating. |

See [companion sources](docs/COMPANION-SOURCES.md) for details on the Buff HUD companion repository. Experimental and diagnostic plugins are maintained separately in research.

---

## Installation and Updating

1. **Install via D2R Reimagined Launcher or manual extraction:**
   - **Launcher:** Use **Install Plugin/Zip** directly on any release `.zip` archive.
   - **Manual extraction:** Close the game and extract each selected `.zip` directly into your active mod directory (e.g. `mods/Reimagined/`).
     - Plugin `.dll` files install into `d2rloader/plugins/`. Keep only one active copy of each plugin.
     - **Pyre Loot Filter** includes its Reimagined drop-sound table and `Filter01.flac`–`Filter16.flac` audio assets under `Reimagined.mpq/data/`.
     - **Ultrawide** includes its `d2rloader/data/hd/tc22sh/` shader files alongside `d2rloader/plugins/`.
     - **Buff HUD** includes both `d2rl-buffhud.dll` and `d2rl-buffhud.mpq` under `d2rloader/plugins/`.
2. **Launch through D2RLoader:** Missing `.toml` configuration files are created automatically in your active `d2rloader/config/` folder on first launch, and existing settings or `filter.json` files are preserved across updates.

---

## Controls and Chat Commands by Plugin

Enter slash commands in normal game chat (feedback is printed in the D2RLoader console). Click any plugin name below for its full guide, configuration keys, and examples.

| Plugin | Control or Chat Command | What it does |
| --- | --- | --- |
| **[Controller QOL Updates](plugins/controller-qol/README.md)** | `Hold LB` (near ground loot) | Show up to 7 direct-loot button assignments (`LB + A/X/Y/B/RB/RT/LT`). |
| | `LB + A / X / Y / R3 / L3` | Identify items, transfer to/from stash or Cube, fill belt/tomes, or bulk-deposit to advanced storage. |
| | `/qol reload` · `/qol perf` | Reload supported live settings or write a read-only performance report in the loader console. |
| **[Pyre Loot Filter](plugins/pyre-lootfilter/README.md)** | `/filter on` · `off` · `reload` · `status` | Toggle loot filtering, reload `filter.json`, or configure `minimap`, `sound`, `alt`, and `autoreload`. |
| **[Pyre Stash Search](plugins/pyre-stashsearch/README.md)** | Search box · `[<]` / `[>]` flyout · `/<page>` | Search across Personal and Shared Stash tabs, toggle the Side Tab Flyout (right-click any tab to rename), jump to page `/<page>`, or hold `Ctrl` / `Ctrl+Shift` + Mouse Wheel to cycle tabs. |
| **[Running Defense](plugins/running-defense/README.md)** | `/runningdefense [status]` (D2RLoader console) | Inspect active running Defense and Shield Block penalty settings configured in `d2rloader/config/running-defense.toml`. |
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
  - **Floating Damage**, **Running Defense**'s combat hit/block scaling, and **Ultrawide**'s monster pop-in / AI perception adjustments run on the local game server (single player or local host).
  - **Pyre Loot Filter**, **Pyre Stash Search**, **Day Night Cycle**, **Always Show Sockets**, **Item Roll Ranges**, and **Map Assistance** run entirely on the client.
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
