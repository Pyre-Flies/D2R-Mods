# Always Show Sockets — experimental probe

Draws native empty socket circles and inserted item artwork after every native
inventory item icon. Native socket eligibility, placement, scaling and insertion
order remain in the game's renderer. No item data or interaction state is edited.

This is an experimental exact-build candidate, not a validated release. Install
the DLL only with D2RLoader closed, into the active mod's `d2rloader/plugins/`.
Restart to load it. In game chat, `/sockets` toggles the feature;
`/sockets on` enables it and `/sockets off` restores native selection behavior.
`/sockets status` reports its state through the existing debug-console output.
The same commands without `/` remain available in the debug console. Commands
are local client actions. The feature starts enabled each session.
Native chat registration pins callback storage until process exit; restart to
update or remove the plugin, rather than hot-reloading it.

Initial live checks: empty, filled and mixed sockets; selected and unselected
inventory items; equipped items; personal/shared stash; Cube; mouse/controller;
close/reopen panels and change resolution. Vendors, trade and classic graphics
remain unverified. Remove the standalone DLL with the game closed to roll back.

See [native contracts](../../docs/reverse-engineering/always-show-sockets.md).
