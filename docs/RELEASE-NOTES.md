# D2R Mods release 2026.10.12.1

## Item Roll Ranges 1.3.1+rev.25

Restore modifier ranges and source labels for Reimagined unique/set definitions
hidden from the Chronicle. Item Roll Ranges now supplies guarded, private
tooltip-only definition snapshots, so Chronicle Items is no longer required
for those ranges. Shared definition flags, Chronicle visibility and save data
are unchanged. Unique property labels are shortened from `[Unique]` to `[U]`,
including fixed-value properties. Set properties retain native ranges without
additional set source labels.

The user confirmed Lustwander's ranges/labels, Chiad's Heartbane's Enhanced
Defense range and Base Defense, and the shortened unique marker in game with
the standalone setup. All five automated suites pass, including immutable
definition snapshots and exclusion of unrelated callers. Existing Base headers
and native Ctrl/controller input handling are retained. Dedicated controller,
localization and every item-family/plugin combination are not fully validated.
Socket, enchantment and corruption contributions do not yet have complete
source attribution or expanded breakdowns; this release does not add `[E]`.

## Floating Damage 0.1.0-beta.12

Scale text and its offsets, lanes, motion, outline, shadow and backing to the
viewport, using 1920 x 1080 as the baseline. `resolution_scaling = true` is the
default: 4K uses 2x and 1440p uses 4/3x, bounded to 0.25-4x. Set it to false
to retain fixed screen pixels. HP values, grouping and timing are unchanged.

Choose `critical_style = "suffix"`, `"size"` or `"both"` (default). Suffix uses
normal-size `!!`; size enlarges without the suffix; both retains the existing
enlarged `!!`. `critical_size_scale` and `critical_hits` keep their existing
meaning. Damage-type colors and native critical association are unchanged.
Saved config files are preserved; omitted new keys select these defaults.

All seven automated suites pass, including the production drawing adapter at
2x scale, configuration/mode cases and exact-DLL ABI/default/version checks.
New resolution scaling and critical-style modes remain pending live visual
validation. Floating Damage retains beta status and targets local single
player; remote-client, dense-combat and broader session/resolution coverage
remain limited. No new native hook or gameplay behavior is introduced.

## Included packages and installation

Controller QOL Updates 1.3.1+rev.75, Map Assistance 1.3.1+rev.1 and Inventory
Deposit 0.1.0-beta.13 are included unchanged from release-2026.10.11.1. The
separate Controller QOL rev.76 beta is not promoted by this release.

Close the game and loader, back up installed DLLs, and extract only the wanted
ZIPs into the game directory or active mod directory. Keep one active copy
per plugin. DLLs install under `d2rloader/plugins/`; reference TOMLs are under
`defaults/`, never live config paths. Keep customized settings. The loader
creates missing configs; merge new keys manually if desired. A cold restart
is required. Item Roll Ranges requires Core item stat ranges enabled. All
native features require their documented exact build/provider admission.

Local verification: all 55 CTest suites (29 QOL, 5 ranges, 2 map assistance,
12 inventory deposit and 7 floating damage), all 19 packaging tests, five ZIP
integrity/content/config-preservation audits and internal/external checksums.
These checks are separate from the live-game evidence above. GitHub CI and
published download checksums are verified as part of publication.
