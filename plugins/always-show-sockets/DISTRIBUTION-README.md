# Always Show Sockets 0.1.0-beta.1

Shows native empty socket circles and inserted gem, rune and jewel artwork on
gear without hovering. Starts enabled each session.

## Install

Close Diablo II: Resurrected and D2RLoader. Back up any existing
`d2rloader/plugins/Always Show Sockets.dll`, then extract this ZIP into the
active mod directory containing `d2rloader/`. For Reimagined this is typically
`mods/Reimagined/` inside the game installation. The ZIP installs only one DLL.
Requires D2RLoader plugin ABI 4, Windows x64, and the exact executable/Core
hashes in COMPATIBILITY.txt. Other builds leave normal socket display intact.
No configuration files or other plugins are replaced.

## Commands

In game chat: `/sockets` toggles, `/sockets on` enables, `/sockets off` restores
normal selection-dependent socket display. `/sockets status` writes the current
state to the loader debug console. The same commands without `/` work in the
debug console. State changes are logged in `d2rloader/logs/always-show-sockets.log`.
These commands act locally. Settings do not persist across game restarts.

## Validation and limits

User-confirmed live results: empty sockets in inventory, gem and jewel artwork
on equipped armor, Cube, personal stash and shared stash. Automated checks cover
DLL admission, unsupported-build refusal and slash/console command handling.
The native chat backend has prior live validation in Floating Damage, but this
plugin's `/sockets` registration and dispatch still need live confirmation.
Empty and filled sockets together on one item, controller-specific behavior,
vendors, trade, classic graphics and resolution/scaling changes remain unverified.
Please report issues with your loader/Core versions, affected panel and commands.

## Roll back

Close the game and remove this DLL or restore your previous copy. Restart the
game after installing, updating or removing it; do not hot-reload the plugin.
The native chat parser retains its callbacks until process exit.
