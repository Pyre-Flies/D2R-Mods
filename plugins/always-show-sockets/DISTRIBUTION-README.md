# Always Show Sockets

**1.3.1+rev.2 · by PyreFly**

Style empty and filled sockets on hover/selection, or keep them visible
continuously. Loose gems, runes and jewels retain their normal appearance.

## Quick start

Enter these in game chat. Feedback appears in the loader console.

| Command | What it does |
| --- | --- |
| `/sockets default` | Show styled sockets only on normal hover/selection. This is the fresh-install default. |
| `/sockets always` | Keep styled sockets visible. |
| `/sockets off` | Restore native visibility and appearance. |
| `/sockets size 65` | Set inserted artwork to 65% size; circles follow at 80%. |
| `/sockets ring 80` | Set ring opacity to 80%. |
| `/sockets center 40` | Set black-center opacity to 40%. |
| `/sockets opacity 65` | Set inserted artwork opacity to 65% of its native fade. |
| `/sockets lift 1` | Move inserted artwork upward one rendered pixel. |
| `/sockets status` | Report the current mode and appearance values. |

Visibility modes and appearance settings save automatically and survive
restart. Fresh installs use `default`; subsequent launches restore the saved mode. See [Configuration](CONFIGURATION-GUIDE.md)
for accepted ranges, examples and manual settings.

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

Requires Windows x64 and the exact D2RLoader 1.3.1/Core profile in
the repository [compatibility record](COMPATIBILITY.txt). Appearance controls apply to HD graphics; classic graphics
retain native size and opacity. Unsupported builds preserve native behavior.

Live checks cover inventory/equipment, Cube, personal/shared stash, mixed
empty/filled sockets, appearance and display-mode changes. Four automated
suites passed, including appearance saving through the configuration API;
actual loader-backed save/restart still needs live confirmation. The latest
revision has not been retested alongside Quality Backgrounds. Vendor, trade,
controller-specific and broader resolution coverage remain unverified.

For requirements, see the repository compatibility record. Report problems at
[GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues), including the plugin
version, game/mod version, and steps to reproduce. Check logs for personal
information before sharing them. `SHA256SUMS` is provided alongside the download ZIPs.

Runtime ZIPs contain only the DLL, reference defaults and a usage README.
Detailed guides and compatibility records stay in this repository.
