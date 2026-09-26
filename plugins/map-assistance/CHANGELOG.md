# Changelog

## 1.3.1+rev.1 - 2026-09-26

- Adopted the shared D2RLoader plugin version line used by the other plugins.
- Added visible in-game and packaged credit to Kryszard's PD2 Loot Filter.
- Prepared the first production runtime and reproducible source packages.

## 0.3.0 - 2026-09-26

- Moved zone names, layout labels, and tooltip lines into `map-assistance.toml`.
- Keyed each entry by numeric `map_id`, allowing custom or future map ids without rebuilding the plugin.
- Embedded the default configuration so D2RLoader creates it when missing while preserving user edits.
- Added bounded parsing, duplicate and malformed-entry rejection, multiline tooltip support, and config-resource tests.

## 0.2.0 - 2026-09-26

- Expanded Keys-tooltip assistance to all 127 non-town LoD level ids across Acts I-V.
- Added concise classifications for static and random zones and actionable routing for tile-based and semi-static zones.
- Excluded PD2-only rewards and mechanics so guidance remains suitable for D2R and D2R Reimagined.
- Added automated coverage checks for every level id from 1 through 132, including intentional town omissions.

## 0.1.2 - 2026-09-26

- Split Jail Level 1 entrance and waypoint guidance onto separate tooltip lines.
- Recorded user-confirmed loader admission and visible Tower/Jail Keys tooltips.

## 0.1.1 - 2026-09-26

- Added the required embedded D2RLoader ABI manifest resource.
- Added Windows version metadata and a loader-artifact admission test.
- Fixes D2RLoader rejecting the DLL as an old alpha plugin.

## 0.1.0 - 2026-09-26

- Added the first Keys-tooltip map-assistance proof of concept.
- Added area-aware static tips for selected LoD campaign areas.
- Used public Item, Shared Event, and Lifecycle services; no native hooks.
- Added automated coverage and ordering checks for the tip table.
- Live-game validation remains pending.
