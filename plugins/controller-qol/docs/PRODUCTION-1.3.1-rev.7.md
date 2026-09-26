# Controller QOL Updates 1.3.1+rev.7

Made by PyreFly for D2RLoader. Production snapshot: 2026-09-26.

Controller QOL now intercepts specific ground-label and portal-targeting calls instead of taking over their shared function entries. This preserves the established controls while reducing conflicts with other plugins, including the resolved Chronicle Ground Flag collision.

## Validation and scope

All 12 automated suites passed. The user confirmed Chronicle coexistence in rev.6 and the portal contact migration in rev.7. The earlier rev.5 input validation covered Battle.net DualShock with Steam closed, Steam Controller on Steam, and DualShock on Steam, with Stash Search and Potion Auto Pickup enabled; that full device matrix was not repeated for rev.7. The latest portal confirmation is a general gameplay confirmation, not a recorded measurement of every distance boundary.

This release retains native controller input, modifier-based direct looting, neutral-A portal priority and LB loot priority, item management, and the earlier compatibility improvements. General debug logging and separate portal diagnostics are OFF in packaged defaults. Existing user configuration is preserved.

Qualified target: D2RLoader 1.3.1 (tested binaries report 1.3.1-beta), Windows x64, plugin ABI 4. Core SHA256: 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2. Loader SHA256: 93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10. Version metadata is not a guarantee of compatibility with every same-version binary; startup guards remain authoritative. Private call sites still need qualification after updates, and another plugin patching those same sites can still conflict.

## Install

Close the game and loader, back up the existing DLL, and copy d2rloader/plugins/Controller QOL Updates.dll into the active loader's plugins directory. The current deployment uses the game's global d2rloader/plugins directory. Do not leave duplicate old QOL DLLs in a loaded plugins directory. Keep existing configuration; a new installation creates config/controller-qol-updates.toml from embedded defaults. The loose configuration directory contains reference defaults, not an instruction to overwrite user settings.

The source ZIP includes SDK, tests and address/recovery documentation. See docs/PORTAL-CONTACT-1.3.1-rev.7.md, docs/CHRONICLE-COMPATIBILITY-1.3.1-rev.6.md and docs/HOOK-INVENTORY.md. Earlier production archives and the pre-rev.7 global DLL backup are retained.
