## Controller QOL Updates 1.3.1+rev.22

- Fix stash withdrawal selecting items from the wrong container; use SDK transactions for normal Shared Stash transfers with explicit page selection.
- Keep ground labels on only in controller mode and recover stale labels after identification, preserving keyboard/mouse Alt behavior.
- Read Tome of Identify charges through stat 70, with checked SDK edits and rollback handling.
- Improve potion compatibility with Auto Belt Refill while retaining standalone operation. LB+R3 on vendor potions fills from inventory, then buys missing potions through native bulk-buy behavior.
- Use LB/RB inside Options and Chronicle while retaining LT/RT outer menu navigation.
- Remove invalid SDK-handle-to-native-pointer conversions and block menu navigation from queuing ground-loot actions, fixing the reported Chronicle-to-Quest crash.
- Add available item shortcuts to controller headers and move native Show Ranges to RB independently of Item Roll Ranges, preserving that plugin's detailed-range interception.

This release packages the previously built rev.22 artifact; all 19 QOL suites passed during development. No additional local test run was requested for publication. The header layout remains an initial implementation with spacing limitations. Rev.22's standalone and combined-plugin RB behavior still awaits explicit live confirmation; earlier fixes have the validation details recorded in the included documentation.

## Included unchanged

- Item Roll Ranges 1.3.1+rev.13
- Map Assistance 1.3.1+rev.1

## Installation

Close the game and loader, back up existing DLLs, and extract the desired ZIPs into the game directory for a global installation. Preserve existing configuration and keep only one active copy of each plugin. Debug logging defaults remain off. Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.

Target: Windows x64, D2RLoader 1.3.1 / ABI4; SDK pinned to v0.3.0. Native features retain build-specific guards. See the bundled production record for compatibility limits. Hot reload is unsupported.
