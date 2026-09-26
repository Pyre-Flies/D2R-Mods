## Controller QOL Updates 1.3.1+rev.10

- Fix controller menu remapping with Charm Inventory.
- Support LB+X transfers between inventory and the visible registered custom
  page, using D2RCore's transfer policy and placement handling.
- Show controller item shortcuts only while in controller mode.
- Retain the scoped portal/ground-label interception that leaves shared
  function entries available to other plugins.

## Item Roll Ranges 1.3.1+rev.13

- Use compact [P]/[S] source labels with affix names; label inherent properties [Base].
- Improve poison and composite elemental-damage attribution and stacked-source rows.
- Correct signed range formatting and omit incomplete provider intervals.
- Resolve stacked fire/lightning/cold contributions only when the loaded
  definitions and combined total prove one unique in-range solution; ambiguous
  contributions retain gray question marks.

## Map Assistance 1.3.1+rev.1

Included unchanged: configurable Keys-tooltip navigation tips for 127 non-town
LoD areas, with credit to Kryszard's PD2 Loot Filter.

## Installation and compatibility

Close the game and loader, back up existing DLLs, and extract the desired ZIPs
into the game folder for global installation. Preserve your existing configuration;
configuration files in the archives are reference defaults. Keep one active copy
of each plugin. Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.
Diagnostic logging remains off by default.

These builds target D2RLoader 1.3.1 / ABI4. QOL and Item Roll Ranges use guarded,
build-specific native contracts; see the bundled compatibility records. Map
Assistance uses public SDK services. User confirmed Charm Inventory menu/transfer
behavior and the latest tooltip display. Automated tests do not establish full
multiplayer, persistence, controller or other-plugin compatibility.
