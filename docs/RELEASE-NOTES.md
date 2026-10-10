# D2R Mods release 2026.10.10.3

Stable runtime packages for Controller QOL Updates **1.3.1+rev.75**, Item Roll
Ranges **1.3.1+rev.23**, and Map Assistance **1.3.1+rev.1**. Ranges and Map retain
their existing runtime behavior and are freshly built for this suite release.

## Controller QOL: new configuration layout

Shared actions and aim preferences stay in
`d2rloader/config/controller-qol-updates.toml`: deadzone, cursor speed,
acceleration, reticle appearance/smoothing and logging.

All per-skill settings now live in
`d2rloader/config/controller-qol-skills/<scope>.overrides.toml`: enable/lock
states, targeting, leading/prediction caps and Whirlwind behavior. This is a
complete editable profile for the mod or Vanilla, shared across its characters.
Find it by the `# Mod:` header. R3 saves here and applies successful toggles
immediately. Manual edits require a restart. The matching `.catalog.toml`
contains generated reference data and is never loaded as active settings.

Upgrading imports old main-file skill choices and R3 preferences with recovery
backups and a comparison of effective settings before removing old sections.
Shared settings/comments and existing skill choices are preserved. R3 keeps
one rolling `.r3.bak` per profile; migration recovery copies are separate.
Read the [configuration, migration and rollback guide](https://github.com/Pyre-Flies/D2R-Mods/blob/release-2026.10.10.3/plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md)
before editing or downgrading. Moving/renaming a mod directory selects a new
profile; separate loader config directories are not automatically merged.

## Skills and reticles

- General Skills supports reviewed equipment-granted skill buttons using live
  IDs and layout, including Warp and class OSkills. Supported loose mod tables
  supply discovery candidates, which require runtime confirmation. Discovery
  does not infer arbitrary replacement skill behavior or projectile speeds.
- Reticles have stronger dark borders, brighter strokes and clearer lock corners.
  Fresh defaults use pale cyan (`#8FE8FF`) and amber (`#FFD166`). Custom colors stay.
- Screen-position smoothing reduces jitter as the character moves, without
  changing cast coordinates or delaying direct stick adjustments. New configs
  and omitted `overlay_smoothing_ms` keys default to **60 ms**. Existing explicit
  values remain unchanged; zero disables smoothing. Larger values can add visible lag.
- Detailed aim logging remains off by default (`[aim] verbose = false`).

## Install and update safely

Close the client and extract the desired ZIP into the game directory. DLLs land
under `d2rloader/plugins/`. **Current archives contain no live config paths.**
Reference TOMLs are under `defaults/`; do not copy them over customized files.
The loader creates missing main configs from embedded defaults, and QOL creates
its active skill profile when aim initializes. Map Assistance's reference config
also uses `defaults/`. Each ZIP includes checksums; the release `SHA256SUMS`
covers all three ZIPs.

Use matching QOL versions if multiple loader locations share one main config.
Managed Ladder packages should carry this update in their normal package revision.
Do not replace signed/managed pack contents outside that process. Rolling back
to an older QOL DLL requires its matching pre-migration main config backup.

## Validation and known limits

Fresh Release verification: Controller QOL **29/29**, Item Roll Ranges **5/5**,
Map Assistance **2/2**, and **2/2** archive-preservation tests. Runtime packages
are checked for ZIP integrity, internal/external checksums and safe paths.

Live user feedback confirms General Skills behavior, profile layout, bounded R3
backups and improved reticle readability/motion in Ladder. This is not exhaustive
testing of every mod, offline upgrade, equipment layout or recovery scenario.
Full-Ladder Save & Exit crashes remain unresolved; this release does not claim
to fix them. See the included rev.75 production and crash-triage records.
