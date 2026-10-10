# D2R Mods release 2026.10.10.1

## Item Roll Ranges 1.3.1+rev.22

Fix armor Defense headers reverting to Core's range-only replacement. Iceblink
now retains actual Defense and the verified `(Base: ...)` display, consistent
with weapons and belts. Core restores its range context before publishing the
Defense header; the plugin uses its saved receiver/text proof at that step.
Focus/panel checks and fail-open behavior on proof mismatch remain in place.
Native Ctrl/controller input and Controller QOL behavior are unchanged.

The user confirmed the Iceblink fix in game. All five Item Roll Ranges suites
pass, including restored-context and invalid-proof regressions. This does not
establish coverage for every item, localization or plugin combination.

Controller QOL 1.3.1+rev.73 and Map Assistance 1.3.1+rev.1 are included unchanged
from the previous stable release. Their clean automated suites also pass.

## Installation

Close the game and loader and back up existing files. Extract the wanted ZIP
into the game directory: DLLs install under `d2rloader/plugins/`, configs under
`d2rloader/config/`. Keep only one active copy per plugin. For a mod-scoped
install, extract into the active mod folder. Preserve customized TOML files.
Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true` and the
qualified D2RCore/game build in its compatibility notes. Cold restart required.
