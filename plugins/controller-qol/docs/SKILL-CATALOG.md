# Rev.53 class skill catalog and review defaults

2026-10-03. The config enumerates 240 player class skills, 30 each for Amazon,
Sorceress, Necromancer, Paladin, Barbarian, Druid, Assassin and Warlock. It does
not enumerate monster-only skills or internal item/utility actions. Class names
are organizational; no native player-class check is added.

Source inspection found the Reimagined `base/skills.txt` is already modified:
for example ID 14 is Toxic Whirl and ID 81 is Corpse Field. It is not a vanilla
name/behavior authority. The vanilla catalog was cross-checked against the
current extracted table maintained at
https://github.com/pinkufairy/D2R-Excel/blob/main/skills.txt and Blizzard's class
overview https://diablo2.blizzard.com/ plus the classic name listing
https://classic.battle.net/diablo2exp/skills/alphaskills.shtml.
Retrieved table SHA256:
`56944CB6B6B0F3C57F7B690977987A732AFEAFC961FA9DC969E7796E54C93F3F`.
The complete source table is retained only in ignored
`local rollback d2r-class-skill-catalog-20261003.txt`; no native records,
formulas, extracted proprietary assets or full table are packaged/committed.
The catalog contains only semantic IDs, readable labels and plugin review policy.

Internal table labels such as Fire Trauma and Plague Poppy are normalized to
Fire Blast and Poison Creeper. The existing Multi Shot migration alias remains.
Numeric IDs are authoritative for configuration; names are comments. Mod-replaced
IDs can behave differently and require review against the active mod's table.
This is a static vanilla class catalog, not runtime mod-table discovery.

Defaults enable only the ten previously tested skills: 12, 22, 51, 54, 56, 59,
62, 67, 132, 151. Every other class entry defaults false. The explicitly requested
native defaults are Nova 48, Corpse Explosion 74 and Poison Nova 92. A false value
keeps native routing and disarms custom aim when cast, as in rev.52. Missing
new entries also default false; an old partial config does not enable them.

Comment categories use table corpse/passive/aura/range/summon flags plus curated
directional/placement/self-cast distinctions. They are preliminary review notes,
not proofs of native targeting. Explicit true entries use the catalog's proposed
ground or snap coordinate mode; no automatic runtime compatibility claim follows.
Corpse selection, passives, self-centered skills and auras should remain false.
Warlock entries are included but all start false. New native skill implementations
and mod replacements still require live tests before enabling them by default.

`tools/expand_skill_catalog_local.py <skills-table>` deterministically regenerates
the semantic header and stock class sections; it resets stock skill defaults,
not the deployed configuration. Review catalog changes before running it against
a different source. Parser storage now holds 240 catalog entries and 32 additional
custom IDs; custom sections cannot redefine a catalog ID. Both QOL and aim read
buffers grow to 65536 bytes because the commented stock TOML exceeds 16384 bytes.
No new native RVA, byte witness, structure offset, function call or ABI is added.

All 23 QOL suites passed, including all 240 IDs/defaults,
30 entries per class, explicit additional ground/snap modes, duplicate identity
rejection, bounded custom storage, >16 KiB config reads, and DLL resources/ABI.
Live validation of newly enabled skills remains pending.

Local installation: confirmed game/loader closed; installed rev.53 DLL and
expanded TOML at the existing `<game directory>/d2rloader`
deployment. Migration validated 240 unique class entries, preserved all ten prior
skill toggles, all QOL/aim tuning and any custom entries, and checked IDs 48/74/92
remain false. Built/deployed SHA256 match:
`AA68FE9591A0C11C99CE0950936636B8D6073F50430753A55A77C13E8C8693EF`.
Prior rev.52 DLL/TOML are preserved in ignored
`local rollback controller-qol-rev53-20261003-063240`.
Stock embedded config is 17122 bytes; the migrated local config is 16333 bytes.
These checks prove installation/configuration consistency, not live skill behavior.
