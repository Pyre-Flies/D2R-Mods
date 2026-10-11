# Inventory Deposit

Version **0.1.0-beta.13** for the reviewed D2R `3.3.93787` / D2RLoader `1.3.1`
profile. This is a Windows x64 prerelease.

Adds a mouse-clickable **Deposit All Gems, Materials and Runes** button beside
the inventory gold display while the stash is open. It deposits eligible main
inventory gems, materials, runes and rejuvenation potions into advanced storage.
Native rules decide which items qualify; equipment, belt items, the Cube and
Charm Inventory are not sources.

The button follows the gold display, greys out while holding an item or when
nothing is eligible, and highlights on hover when available. Each affected
Gems, Materials or Runes tab receives the existing supplemental animation once
per batch. Custom stone-and-gold artwork and all four interaction states are
embedded in the DLL. No MPQ, loose images or Controller QOL installation is needed.

## Install or update

1. Close D2R and D2RLoader. Back up any previous `Inventory Deposit.dll`.
2. Extract the ZIP into the game directory, placing the DLL at
   `d2rloader/plugins/Inventory Deposit.dll`. For a mod-scoped installation use
   that mod's active `d2rloader/plugins/` directory instead. Install one copy.
3. Start through D2RLoader, open the stash in keyboard/mouse mode, and hover the
   button to see **Deposit All Gems, Materials and Runes**.

`defaults/inventory-deposit.toml` is a reference only. The loader creates missing
settings from the embedded defaults. The ZIP contains no live
`d2rloader/config/` files and preserves existing settings.

The live config is `d2rloader/config/inventory-deposit.toml` in the active scope.
Default gold-relative placement uses `anchor_to_gold = true`, `gold_gap = 85`
and `gold_offset_y = 1`. `button_x` and `button_y` apply only when automatic
anchoring is disabled. Restart after config or DLL changes; native children
are not hot-reloadable. To roll back, close the game and restore the previous
DLL. To remove the feature, remove its DLL while the game is closed.

## Compatibility and validation

The exact Core hash and native byte/layout guards are required; see
`COMPATIBILITY.txt`. Unsupported profiles leave this optional button unavailable.
The reviewed setup is Reimagined's HD keyboard/mouse inventory and advanced
stash. Controller and legacy layouts are outside this button's scope.
No stock or Charm Inventory panel resource is replaced. Another plugin's button
may occupy the same footer space; adjust the placement settings if needed.

The user confirmed deposits, artwork, fit, tab feedback, disabled states and
hover behavior in game, including the final shortened tooltip. Automated
checks cover UI lifecycle, eligibility, batch/transfer rules, native state
transitions, embedded art and DLL admission. Multiple resolutions, low-end
rendering, remote TCP/IP behavior and save/reload persistence are not fully
validated. This is why the first public release remains a prerelease.

Transfers are not atomic; partial or unconfirmed batches stop without retrying.
Controller QOL and Inventory Deposit have independent queues. Avoid starting
both bulk actions concurrently while a transfer is awaiting completion.

Source, detailed evidence and the changelog are available at
[Pyre-Flies/D2R-Mods](https://github.com/Pyre-Flies/D2R-Mods/tree/main/plugins/inventory-deposit).
