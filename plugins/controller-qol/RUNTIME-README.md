# Controller QOL Updates 1.3.1+rev.78

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

Controller item shortcuts, menu navigation and optional right-stick aiming.

| Default control | Effect |
| --- | --- |
| Hold LB/L1, then press a labeled button | Collect the assigned ground item. |
| LB/L1 + A/Cross | Identify the focused item or move a potion to the belt. |
| LB/L1 + X/Square | Transfer to the open storage context; sell in a shop. |
| LB/L1 + Y/Triangle | Move an eligible item into the Cube. |
| LB/L1 + L3 at stash | Deposit eligible carried materials, gems, runes and rejuvenation potions. |
| Right stick | Move the aiming cursor for supported skills. |
| R3 on a supported skill | Toggle aim and save the active mod profile. |
| RB/R1 on a tooltip | Show ranges while the menu remap is active. |

No chat commands. General settings: `controller-qol-updates.toml`; skill settings:
`controller-qol-skills/<scope>.overrides.toml`, identified by its `# Mod:` header.
Advanced-storage features require a mod that provides that storage.

Full instructions, compatibility details and validation limits:
[User guide](https://github.com/Pyre-Flies/D2R-Mods/blob/main/plugins/controller-qol/DISTRIBUTION-README.md).
Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues).
