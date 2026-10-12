Always Show Sockets keeps native socket circles and inserted-item artwork visible
on gear without hovering. This standalone beta installs only its own DLL and
does not replace configurations or other plugins.

- Live-confirmed: empty inventory sockets, a gem and jewel on equipped armor,
  Cube, personal stash and shared stash.
- Local chat `/sockets` toggles; `/sockets on` and `/sockets off` set the state.
  The debug-console command remains available. Enabled by default each session.
- Automated DLL and command checks pass. The chat backend is previously
  live-tested in Floating Damage; this plugin's slash dispatch remains unconfirmed.
- Controller-specific behavior, vendors, trade, classic graphics, scaling, and
  empty/filled sockets together on one item remain unverified.

Requires the exact D2RLoader.exe and D2RCore.dll hashes in the package's
COMPATIBILITY.txt. Close the game before extracting into your active mod folder
(typically `mods/Reimagined/`). Back up an existing DLL first. Restart to update
or remove; no hot reload. Please report results and issues, especially `/sockets`
chat toggling and panels/input modes not covered above.
