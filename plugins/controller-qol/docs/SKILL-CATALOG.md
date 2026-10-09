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

## Reimagined Ladder special-skill investigation (2026-10-08)

Read-only inspection found the running client's QOL module under
`mods/ReimaginedLadder/d2rloader/plugins`, resource version rev.63, SHA256
`7DA3D25EE0168A76AFBD11CBA8616F6168EB17F0D7E72731C2F1AD612C401D6A`.
The installed Ladder MPQ directory's `data/global/excel/skills.txt` SHA256 is
`708DA23BF078CC3FC4D2EF901C7480869E18206ABA8CBC91E4030D5CBA93FEF3`;
`data/hd/global/excel/controllerskillsettings.json` SHA256 is
`4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79`.
These identify inspected disk inputs, not a dump of the compiled runtime tables.
Names below follow each skills row through `skilldesc.txt`'s `str name` to the
installed `data/local/lng/strings/skills.json` English string.

| ID | QOL catalog name | Installed table name | Installed display name | QOL targeting default |
|---|---|---|---|---|
| 43 | Telekinesis | Telekinesis | Telekinesis | snap |
| 251 | Fire Blast | Fire Trauma | Fire Blast | ground |
| 253 | Psychic Hammer | Ice Barrage | Ice Barrage | snap |
| 256 | Shock Web | Shock Field | Shock Web | ground |

Ice Barrage does not replace Shock Web. Its numeric ID already belongs to the
static catalog, so R3 toggles the existing 253 entry while the vanilla comment
still says Psychic Hammer. The Ladder log records R3 toggles for 253 and two
lookups at 07:22:21 resolving monster 51. This proves the numeric toggle and snap
coordinate path ran; the user reports this skill works. No additional custom ID
is required, and `[aim.custom]` must not redefine 253.

At inspection, the Ladder config had 43 and 253 enabled, 251 and 256 disabled,
and no active `[aim.targeting]` overrides. Enabling 251 alone retains ground mode:
`ResolveSelection` deliberately skips active ground-mode casts, so the destination
remains the cursor. To investigate enemy snapping for Fire Blast, enable 251 in
`[aim.assassin]` and set `"251" = "snap"` under the existing `[aim.targeting]`.
R3 preserves that override. Native ground placement and QOL's ground/snap policy
are separate concepts; this configuration test still needs live validation.

Telekinesis needs separate native-target investigation. Its installed row has
`TargetItem=1`, server start/do functions 12/21, and controller settings
`usageType=3`, `targetGroup=4`. The three Assassin rows instead have controller
`usageType=0`, `targetGroup=0`; these raw enum values are observations, not newly
established enum contracts. Current QOL `Selected` calls the native getter and
then suppresses its result for any admitted enabled active skill. Its replacement
`ReadMonster`/`SnapBook` pipeline accepts only unit type 1, so it cannot supply
item/object targets. Existing Selected/Lookup contracts are documented in
[AIM-NATIVE-HISTORY.md](AIM-NATIVE-HISTORY.md); no new RVA or ABI was established.
The 07:16:30-34 Telekinesis lookups show ground fallback with zero fresh monster
candidates, but do not record selected item/object targets or final dispatch.

Next evidence needed: compare Telekinesis with aim off/on against an enemy,
eligible item and interactive object; observe native selected target type/ID and
whether final dispatch is a unit or coordinate cast. Do not infer that supplying
an object's coordinates preserves its interaction semantics, or generalize
monster-only snapping to every selectable unit. No plugin or deployed config was
changed as part of that initial inspection.

### Local rev.65-beta.1 candidate

The user clarified that Fire Blast was enabled in the earlier failing test;
the current disabled config does not describe that test. Telekinesis failed for
items/objects, chiefly chests and shrines; its enemy behavior was not tested.

The candidate changes default targeting to snap for offensive placement IDs
234, 244, 251, 256, 257, 261, 262, 271, 272, 276, 393, 396 and 400. Their enable
defaults remain false. An explicit `[aim.targeting]` ground override still wins,
including after R3. Cursor defaults become 8 / 48 / 0.35 seconds. The active
Ladder config's three speed settings were updated with a local backup; no enable
state or targeting override was altered. Runtime settings require restart.

Telekinesis ID 43 now retains both `Selected`'s original unit pointer return and
`Lookup`'s original output. `ResolveSelection` skips the monster coordinate book
for this skill, preventing a monster-only overlay lock from representing an
object operation. The pointer is returned directly, never stored. Existing
`UnitTestSelect` instrumentation opens a scoring window for live native candidate
types 1/2/4 only when Telekinesis snap mode is armed and active (or its own idle
preview). `PointScore` uses the provided native position around the reticle;
original enumeration, eligibility checks, selected-unit dispatch, and callback
returns still execute. Other active skills cannot inherit this special window;
disabled Telekinesis, explicit ground mode and global snapping-off keep native
scoring. Even a ground override cannot suppress Telekinesis's unit identity.

This reuses guarded UnitTest `0x1919F0`, PointScore `0x18AF30`, Selected `0x18DDE0`
and Lookup, with their existing ABI and exact-build admission. Only established
unit type `+0` and ID `+8` are read, inside a fault boundary; no new object layout,
RVA, hook or direct native operation was introduced. `QOL/AimUnit` logs at most
64 near-reticle scored candidates and 48 selected-target observations per game,
with copied type/ID, eligibility and scores. Native selection may still impose
category or range limits not explained by the geometry. Chest/shrine/item
selection and operation remain pending live verification, as does the broader
offensive-placement policy. The UI keeps the ordinary aim cursor for Telekinesis;
this candidate does not implement an item/object lock-marker cache.

Local MSVC build and all 28 suites pass, including DLL ABI 4, lifecycle exports,
prerelease resource and version 1.3.1+rev.65-beta.1. Built DLL SHA256:
`BC9307450EB588ECE22AC9363F7AB0D107ECA54CBE36B1AC2FFB3D0964778C9B`.
Policy tests cover selected-unit preservation, enabled/disabled and active/preview
isolation, candidate categories, offensive placement defaults, explicit ground
overrides, and unchanged disabled defaults. Live validation remains pending.

After the user closed the client and process absence was verified, installed the
candidate in the same Ladder plugin directory. Built/deployed SHA256 matches the
value above; the previous DLL and configuration were backed up under ignored
`outputs/special-skills-20261008/`. Root d2rloader's separate installation was not
modified. No release was published for this candidate.

### Rev.66 first-press preparation candidate

User confirms rev.65 Telekinesis operates objects but needs a double tap. In the
07:37:45 trace, initial selected queries return monster 99; after the next natural
candidate pass, selection becomes object 40. At 07:37:54, initial queries have no
target, then object 44 is selected. Logs alone do not establish exact physical
press timing. Source shows idle object scoring previously required preview 43,
so switching from Teleport/another spell leaves a preparation gap.

Rev.66 admits idle object/item scoring when Telekinesis is snap-enabled and the
armed cursor has any enabled preview skill. It does not change scoring during
another active skill; monsters continue through the existing shared preview.
This deliberately extends idle item/object geometry around the shared cursor,
but leaves native category ranking and final selection in control. It is a
candidate correction for the first-press gap, not a proven fix for every possible
two-tap case. Regression checks cover idle transitions from Teleport/Fire Blast,
active movement/Interact exclusion, disabled previews and unchanged monster
preview ownership. No new hook, RVA or target pointer storage is introduced.

Rev.65 traces show native UnitTest returning false for objects 40/44 which native
Selected subsequently returns. Therefore the object trace's old `eligible=0`
label was misleading; rev.66 calls that field `nativeResult`. The return's full
object-specific meaning remains unproven. Do not feed it into monster SnapBook
eligibility or discard an object solely on that boolean. Selected-target traces
now report changes or one-second refreshes (64 maximum per game) instead of
exhausting the budget on repeated same-frame queries.

All 28 suites pass for rev.66-beta.1. Verified game processes absent, backed up
rev.65, and installed to the Ladder plugin directory. Built/deployed SHA256:
`0A7BED9E97FB910EBB3070F5E6C10A7E1D1EA8995D9719D1B1206B3CBC89243D`.
Speed configuration remains 8 / 48 / 0.35. The separate exit-crash investigation
is recorded in [CRASH-TRIAGE-2026-10-08.md](CRASH-TRIAGE-2026-10-08.md); this
candidate makes no crash-fix claim. Live first-press validation remains pending.

Subsequent user feedback on the deployed rev.66: "They both feel much better,
both Telekinesis and Fire Blast." Record this as live confirmation of improved
targeting for those two tested skills. The user did not separately enumerate
every Telekinesis target type or explicitly confirm all first-press scenarios;
broader placement-skill, offline and leave-game stability checks remain open.
