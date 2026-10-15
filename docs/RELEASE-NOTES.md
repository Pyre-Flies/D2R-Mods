# D2R Mods stable suite - 2026.10.15.1

Six independently installable plugins now use the shared `1.3.1+rev.N` format.
Revision numbers remain independent per plugin; all six are stable releases.

| Plugin | Version |
| --- | --- |
| Controller QOL Updates | 1.3.1+rev.78 |
| Item Roll Ranges | 1.3.1+rev.45 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 1.3.1+rev.13 |
| Floating Damage | 1.3.1+rev.12 |
| Always Show Sockets | 1.3.1+rev.1 |

- Item Roll Ranges adds `(Ilvl: XX, Qlvl: YY)` below Required Level, before
  Max Sockets and weapon Attack Speed. Items without Required Level have
  placement fallbacks. Values come directly from the item and active base
  definition; the existing range hotkey and opt-in always-show mode apply.
- Always Show Sockets joins the suite with default/always/off display modes,
  scaled/faded inserted artwork, independent ring/center opacity and artwork
  lift. Appearance commands save to the active config; startup display mode
  remains default. Includes the command/configuration guide.
- Inventory Deposit and Floating Damage move from beta names to stable
  loader-plus-revision versions. Their previously released behavior, native
  guards and configuration defaults are unchanged. Controller QOL and Map
  Assistance retain their released source and versions.

Each ZIP contains only its own runtime DLL and reference TOMLs under defaults/.
No live d2rloader/config/ files are shipped. Existing saved settings are
preserved; Item Roll Ranges always-show remains false by default. Close the
game before replacing DLLs, retain a backup and use one active copy per plugin.
Pack-managed Ladder installs distribute through the approved package route.

## Validation and limits

All67 plugin suites pass: Controller QOL33, Item Roll Ranges9, Map Assistance2,
Inventory Deposit12, Floating Damage7 and Always Show Sockets4. All30 packaging
checks pass. The six-package checks
cover stable/beta channel validation, exact runtime layout, checksum integrity,
config preservation, privacy and actual DLL ABI/resource/version admission.
Unchanged Controller QOL/Map source retains its prior released coverage.

The user confirmed revision45's level placement and Ctrl release in Reimagined.
Socket rendering, controls and modes have accepted live coverage on preceding
artifacts; autosave/restart is verified by SDK fixtures and still needs an
actual loader-backed persistence check. Promoting a version does not imply
new live coverage for all items, resolutions, vendors, trade or companion mods.
Existing UI/object-pool assertions on exit remain unattributed; this suite
does not claim a whole-game shutdown fix. Floating Damage targets local
authoritative gameplay, with the documented multiplayer limitations.

Exact executable/Core byte/hash guards and ABI4 requirements remain in the
included compatibility records. Packages are staged locally; GitHub CI and
published-download verification must pass before publication is reported.
