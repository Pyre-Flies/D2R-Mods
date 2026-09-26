## Controller QOL Updates 1.3.1+rev.11

- Remove synchronous per-item identification logging and duplicate UI-side
  inventory scanning, a significant source of identification stalls.
- Resolve the exact item and consumables in one authoritative game-thread scan.
- Distinguish unavailable inventory, empty tomes, missing supplies and failed
  operations with concise diagnostics even when debug logging is off.
- Combine identification and charge debit where supported; check final-charge
  operations and attempt compensation on failure.
- Preserve rev.10 controller-mode hints and Charm Inventory compatibility.

Fifteen QOL test suites pass. The external reporter's mod-specific tome failure
and quantified live performance improvement remain unverified. No new native
hooks or RVAs were added for identification.

## Included unchanged

- Item Roll Ranges 1.3.1+rev.13
- Map Assistance 1.3.1+rev.1

## Installation

Close the game and loader, back up existing DLLs, and extract the desired ZIPs
into the game directory for a global installation. Preserve existing configuration
and keep only one active copy of each plugin. Debug logging defaults remain off.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.
Target: Windows x64, D2RLoader 1.3.1 / ABI4. See bundled compatibility records
for build-specific native guards and validation limits.
