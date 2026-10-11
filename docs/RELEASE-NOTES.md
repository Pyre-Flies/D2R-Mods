# D2R Mods release 2026.10.11.1

Five independently installable runtime packages:

| Plugin | Version |
| --- | --- |
| Controller QOL Updates | **1.3.1+rev.75** |
| Item Roll Ranges | **1.3.1+rev.23** |
| Map Assistance | **1.3.1+rev.1** |
| Inventory Deposit (Deposit All Button) | **0.1.0-beta.13** |
| Floating Damage | **0.1.0-beta.11** |

This suite adds Floating Damage and includes the already-published Inventory
Deposit beta.13. Controller QOL, Item Roll Ranges and Map Assistance retain
their stable versions and behavior. The two new suite members retain beta
status and their documented validation limits.

## Floating Damage

- Draw outlined floating numbers for observed enemy HP loss in local single
  player, regardless of attacker. Confirmed deaths show the enemy's last
  remaining HP rather than the attack's overkill amount.
- Color matching physical, fire, cold, lightning, magic, poison and mixed
  damage. Unexplained or ambiguous decreases use the fallback color.
- Configure font size, quoted hex colors, solid outline, shadow, translucent
  poison backing, fade timing and optional rise/fall/sideways motion.
- Matched native Critical/Deadly Strike events display 35% larger by default
  with `!!`; both mechanics share the native marker. The damage amount stays
  the observed HP loss.
- Toggle with `/damagenumbers on`, `/damagenumbers off` or
  `/damagenumbers status` in game chat. Configuration edits and DLL updates
  require a restart.

There is no DPS meter or attacker attribution. Native hashes/byte guards admit
only the reviewed loader-hosted build. Remote clients cannot use this local
authoritative HP reader. Controlled captures and user feedback confirm normal
numbers, elemental colors, poison ticks/deaths, mixed damage, commands and
motion. Native positive/negative captures establish the Critical/Deadly Strike
marker; beta.11's enlarged/suffixed rendering still needs a live visual check.
Dense-combat performance and poison expiration on a survivor remain unverified.

## Inventory Deposit / Deposit All Button

- Add a mouse-clickable **Deposit All** button beside the gold display in the
  HD inventory footer while the stash is open.
- Deposit eligible gems, runes, materials and rejuvenation potions into
  advanced storage. Equipment and other ineligible items remain in inventory;
  the Cube, belt and Charm Inventory are not sources.
- Include embedded stone-and-gold artwork and automatic gold-relative
  placement, with no loose sprite/MPQ files or Controller QOL runtime dependency.
- Grey out the button with no eligible items, while holding a cursor item or
  during a batch. Available items restore hover presentation; each affected
  advanced-stash tab gets supplemental feedback once per batch.
- Tooltip: **Deposit All Gems, Materials and Runes**.

The user confirmed deposits and these visible behaviors in the reviewed setup.
Remote play, simultaneous mouse/controller bulk actions, pressed/low-end
rendering, multiple resolutions, destination counts and save persistence are
not fully validated. Native eligibility and per-item checks remain authoritative;
the operation is not atomic and stops on partial or unconfirmed transfers.

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
Read the [configuration, migration and rollback guide](https://github.com/Pyre-Flies/D2R-Mods/blob/release-2026.10.11.1/plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md)
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
its active skill profile when aim initializes. Every packaged reference
config uses `defaults/`. Each ZIP includes checksums; the release `SHA256SUMS`
covers all five ZIPs. Install each DLL in one active scope only. Back up existing
DLLs and live configs before replacing them; new settings do not require
overwriting saved preferences.

Use matching QOL versions if multiple loader locations share one main config.
Managed Ladder packages should carry this update in their normal package revision.
Do not replace signed/managed pack contents outside that process. Rolling back
to an older QOL DLL requires its matching pre-migration main config backup.

## Validation and known limits

Fresh local MSVC Release builds passed all **55 CTest suites**: Controller QOL
**29/29**, Item Roll Ranges **5/5**, Map Assistance **2/2**, Inventory Deposit
**12/12**, and Floating Damage **7/7**, including built-DLL artifact checks.
All **19 packaging tests** passed and all five runtime archives were created
with ZIP integrity, internal/external checksums, reference-only config paths
and preservation of customized settings verified. Publication also requires successful GitHub CI and checksum verification
of the published downloads. These automated checks are separate
from the live-game evidence described above.

Live user feedback confirms General Skills behavior, profile layout, bounded R3
backups and improved reticle readability/motion in Ladder. This is not exhaustive
testing of every mod, offline upgrade, equipment layout or recovery scenario.
Full-Ladder Save & Exit crashes remain unresolved; this release does not claim
to fix them. The separate item-browser **Unique**-tab investigation likewise
establishes no Floating Damage root cause or crash fix. See the included
plugin compatibility and validation records.
