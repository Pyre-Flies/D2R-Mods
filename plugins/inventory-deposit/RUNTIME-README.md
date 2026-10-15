# Inventory Deposit 1.3.1+rev.13

By PyreFly. Windows x64 and D2RLoader 1.3.1 / plugin ABI 4. Native features require the supported game/Core build; unsupported paths retain native behavior.

## Install or update

Close the game and back up the old DLL. Extract into the game directory or
active mod directory so the DLL lands in `d2rloader/plugins/`. Keep one active
copy and restart through D2RLoader. The D2RR launcher can remain open.
For managed mod packs, use their normal updater.

The `defaults/` TOMLs are references. Existing `d2rloader/config/` settings
are preserved; missing main configs are created on first launch. Edit active
configs with the game closed and restart after manual changes. To uninstall,
close the game, remove this DLL and restart.

## Use and configure

Adds a Deposit All button beside inventory gold while the HD advanced stash
is open in keyboard/mouse mode. Click it to store eligible inventory gems,
runes, materials and rejuvenation potions. No chat command is required.
Equipment, belt, Cube and Charm Inventory contents are excluded.

The button is disabled while holding an item, while busy or when nothing
qualifies. A batch can stop partway through; check its result before repeating.
Avoid running Controller QOL's bulk deposit simultaneously.

In `inventory-deposit.toml`, `[inventory_deposit] enabled = false` disables
the button. `anchor_to_gold = true`, `gold_gap = 85` and `gold_offset_y = 1`
control default placement. Manual `button_x`/`button_y` apply only with
`anchor_to_gold = false`. Requires Reimagined-style advanced storage and HD
keyboard/mouse inventory. Controller QOL is not required.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/inventory-deposit/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
