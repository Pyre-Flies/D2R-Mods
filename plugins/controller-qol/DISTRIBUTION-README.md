# Controller QOL Updates

**1.3.1+rev.78 · by PyreFly**

Adds controller item shortcuts, direct ground looting, menu navigation and
optional right-stick skill aiming.

## Default controls

Xbox names are used below. On PlayStation, LB/RB = L1/R1, LT/RT = L2/R2,
A = Cross, X = Square, Y = Triangle, B = Circle. L3/R3 click the sticks.

| Control | Where | What it does |
| --- | --- | --- |
| Hold LB, then press a labeled button | Ground items | Collect the assigned item; distant reachable items use native movement. Assignments stay fixed until LB is released. |
| LB + A | Unidentified item | Identify it using an eligible tome or scroll. |
| LB + A | Identify Tome | Identify eligible carried items; offline also includes Cube contents. |
| LB + A | Potion | Move it into available belt space. |
| LB + X | Inventory, stash or Cube | Transfer the focused item to the open storage context or back to inventory. In a shop, sell it. |
| LB + Y | Eligible item | Move it into the carried Cube. |
| LB + R3 | Potion or merchant supplies | Fill available belt space, or refill a matching carried tome from merchant scrolls. |
| LB + L3 | Stash open | Deposit eligible inventory gems, runes, materials and rejuvenation potions into advanced storage. |
| LT / RT | Main menu pages | Change the main page. |
| LB / RB | Skill, Quest, Chronicle, Options, Mod Information or filter tabs | Change the inner tab. |
| LB + LT / LB + RT | Shared Stash | Change the stash page. |
| LB + Up / Down | Loot-filter rule list | Move the focused rule. |
| RB | Item tooltip | Show native roll ranges while the menu remap is active. |
| Right stick | Supported aiming skill | Move the aiming cursor; releasing the stick retains its position. |
| R3 | Supported skill in class/General Skills tabs | Toggle aiming for that skill and save the active mod profile. |

Tooltips show actions available for the focused item. Ordinary A interactions
prefer enabled nearby portals, stash, waypoints, shrines and chests. Filtered
items cannot be picked up accidentally with controller A when that option is
enabled; gold and mouse/keyboard pickup retain their normal rules.

See [Configuration](CONFIGURATION-GUIDE.md) for examples, aiming controls and
which files store your settings. This plugin has no chat commands.

## Install, update or remove

1. Close the game. The D2RR launcher may remain open.
2. Back up the previous DLL, then extract the ZIP into the game directory for
   a global install, or the active mod directory for a mod-specific install.
   The DLL belongs in `d2rloader/plugins/`. Keep one active copy per plugin.
3. Start the game through D2RLoader.

For a launcher-managed mod pack, use its normal update process. The `defaults/`
files are references: updates preserve your active `d2rloader/config/` settings.
The loader creates a missing main configuration on first launch. Edit active
configuration files with the game closed, then restart to apply manual changes.

To remove or roll back, close the game and remove the plugin DLL or restore its
backup. Restart after any DLL change; hot reload is unsupported.

## Compatibility and limits

Requires Windows x64, D2RLoader 1.3.1 / plugin ABI 4 and supported native builds;
see the repository [compatibility record](COMPATIBILITY.txt). Unsupported paths keep native behavior. Advanced
storage and General Skills features require those facilities in the active mod.

Live checks cover item actions, menu navigation, skill profiles and hosted
ground-loot repairs. Thirty-three automated suites passed. Not every skill,
controller or plugin combination has been tested. Bulk transfers can stop
partway through; check the destination before repeating. Remote Identify All
uses inventory items and an eligible highlighted tome in Inventory or Personal
Stash; Cube targets are excluded. A separate shutdown issue remains under
investigation, with its cause unconfirmed.

For requirements, see the repository compatibility record. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` is provided alongside the download ZIPs.

Runtime ZIPs contain only the DLL, reference defaults and a usage README.
Detailed guides and compatibility records stay in this repository.
