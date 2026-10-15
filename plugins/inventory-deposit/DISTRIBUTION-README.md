# Inventory Deposit

**1.3.1+rev.13 · by PyreFly**

Adds a **Deposit All Gems, Materials and Runes** button beside inventory gold
while the advanced stash is open in keyboard/mouse mode.

## Use the button

Open the stash and click **Deposit All**. Eligible gems, runes, materials and
rejuvenation potions in the main inventory move into advanced storage. The
game's normal eligibility rules apply. Equipment, belt, Cube and Charm
Inventory contents are excluded. The button is disabled while holding an item,
while busy, or when nothing qualifies. No chat command is needed or provided.

Transfers stop if a move fails or cannot be confirmed. A batch can be partly
completed: check inventory and destination totals before repeating it. Avoid
starting Controller QOL's bulk deposit at the same time.

## Configuration example

Edit existing settings in `d2rloader/config/inventory-deposit.toml`, then restart:

```toml
[inventory_deposit]
enabled = true
anchor_to_gold = true  # Follow the inventory gold display
gold_gap = 85          # Horizontal gap after gold, in inventory UI units
gold_offset_y = 1      # Vertical offset from the centered position
```

To position it manually, set `anchor_to_gold = false` and adjust the existing
`button_x` and `button_y` keys. If another button occupies the same space,
adjust the gap or manual position.

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

Requires the supported Windows x64 D2RLoader 1.3.1/Core profile in
`COMPATIBILITY.txt` and Reimagined's HD keyboard/mouse inventory with advanced
storage. Controller and classic layouts are outside this button's scope.
No separate artwork files or Controller QOL installation are required.
Unsupported builds leave the button unavailable.

Live checks cover deposits, button placement, hover/disabled states and tab
feedback. Twelve automated suites passed. Remote multiplayer behavior, multiple
resolutions and save/reload persistence are not fully validated.

For requirements, see the included compatibility file. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` lists the packaged file checksums.
