Always Show Sockets 0.1.0-beta.2 fixes the rendering-hook collision with Item
Quality Backgrounds. Socket drawing now runs after the native icon draw while
leaving the shared icon entry and original caller checks intact. Quality
Backgrounds requires no changes. This standalone update installs only its own
DLL and does not replace configurations or other plugins.

- Live-confirmed: empty inventory sockets, a gem and jewel on equipped armor,
  Cube, personal stash and shared stash.
- Local chat `/sockets` toggles; `/sockets on` and `/sockets off` set the state.
  The debug-console command remains available. Enabled by default each session.
- User confirms Quality Backgrounds and sockets work together, and `/sockets`
  toggles correctly. Automated DLL, command and assembly-adapter checks pass.
- Controller-specific behavior, vendors, trade, classic graphics, scaling, and
  empty/filled sockets together on one item remain unverified.

Requires the exact D2RLoader.exe and D2RCore.dll hashes in the package's
COMPATIBILITY.txt. Close the game before extracting into your active mod folder
(typically `mods/Reimagined/`). Back up an existing DLL first. Restart to update
or remove; no hot reload. Please report results and issues, especially panels
and input modes not covered above.
