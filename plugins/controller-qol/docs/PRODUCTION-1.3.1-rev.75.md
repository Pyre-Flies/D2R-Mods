# Controller QOL 1.3.1+rev.75 production record

2026-10-10. Promotes the rev.74/rev.75 beta work after user testing. Native paths
remain build-specific and guarded; this release does not qualify other game builds.

## User-facing changes

- General Skills buttons use live numeric skill IDs and geometry, allowing
  equipment-granted skills without assuming a fixed tab layout. Warp uses a
  reviewed ground-target policy; class OSkills share their class skill setting.
- Skill discovery reads only the active mod's supported loose table and confirms
  candidate IDs at runtime. Names do not infer replacement skill mechanics or
  projectile speeds. Unknown skills start off; manual declarations remain possible.
- Shared QOL/aim preferences remain in `controller-qol-updates.toml`. All skill
  settings live in complete mod-specific `.overrides.toml` profiles, including
  Vanilla. Generated catalogs are reference-only. R3 saves one rolling backup.
- Old settings migrate with backups and effective-settings verification.
  Release references move to `defaults/`, keeping archive extraction away from
  live configs. See [the configuration guide](SKILL-DISCOVERY-CONFIG.md).
- Reticles use stronger dark borders, brighter cores and larger lock corners.
  New color defaults are pale cyan and amber. Final screen-position smoothing
  reduces walking jitter while keeping stick adjustments immediate. The shipped
  and fallback smoothing default is 60 ms; explicit user values stay unchanged.

## Validation and boundaries

Release validation requires fresh Release builds and all tests for QOL, Item Roll
Ranges and Map Assistance, plus archive-preservation tests and checksum audits.
Final counts and publication status are recorded in the suite release notes.
Tests cover profile migration/recovery, preserved settings/comments, discovery
limits, R3 persistence/rolling backups, embedded resources and reticle motion.

Live user feedback confirms Warp/General Skills behavior, the new profile layout,
bounded R3 backups, improved readability and smoother reticles while walking.
This does not establish every equipment layout, mod data source, offline upgrade
combination or recovery path. Earlier offline and remote action confirmations
remain scoped to their tested builds; no new exhaustive offline sweep is claimed.

Full-Ladder Save & Exit crashes remain unresolved. This is not a crash-fix release.
See [crash triage](CRASH-TRIAGE-2026-10-10.md); module fault locations alone do not
prove root cause or absolve another plugin of contributing to corruption.

## Installation and rollback

Close the client and install one matching QOL DLL in the intended loader location.
Managed packs should publish through their normal revision process. Do not copy
reference defaults over user files. If loader locations share a config, keep their
QOL versions aligned. Older DLLs do not understand the migrated profile layout;
restore the matching old main-config backup when rolling back.
