# Random skill, class and tab properties: revision44

2026-10-14 local candidate. Qualified loaded D2R3.3.93787 / Core1.3.1-beta;
Core SHA256 `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
All offsets, RVAs, function numbers and byte witnesses below are build-specific.
No new hook, native call, game-state mutation or item/skill database is added.

## Operand contract

Read-only process9044 dispatch slot12 identified game function12 at0x3CCA50.
It rolls the property specification's signed min/max fields at+8/+12 through
0x3D5860, puts the selected low WORD in the raw stat-layer argument, and passes
the signed parameter at+4 as the bonus to the previously qualified0x3D5940
shift/store helper. The min/max fields select a skill/class/tab; they are not
the bonus's displayed range. The raw layer is not transformed with function10's
class/tree encoding. Properties.statN still identifies the target stat.

The active table contains three independent single-handler function12 forms:
`skill-rand` targets stat107, `randclassskill` stat83, and `tab-rand` stat188.
No property name or target stat is used to select the numeric transformation;
the handler and loaded metadata establish it. The current amulet's saved
Gnostics prefix1158 has parameter5 and selector domain251..280. Its permanent
item-owned state0 list contains stat107/layer262/value5: Wake of Fire is the
selected skill, with a fixed+5 bonus. Expected annotation is `[+5]` plus the
Gnostics prefix/tier. Other classes and strengths use the same code.

## Optional admission

`property_source_profile.h` records exact byte arrays:

| Witness | Game RVA | Purpose |
|---|---:|---|
| RandomSkillEntry | 0x3CCA50 | Qualified handler entry, preserving refusal of entry detours. |
| RandomSkillOperands | 0x3CCA97 | Min/max roll, raw layer, parameter bonus and call to0x3D5940. |
| RandomSkillDispatchRva | 0x2386B10 | Slot12 of the existing0x2386AB0 function-pointer table must point to the qualified handler. |

The optional gate also requires the existing property/source ownership guards.
A mismatch disables random-skill annotations while retaining existing native
text and the previously admitted range features. Dispatch ownership is checked
as a pointer identity; no call is made through the observed dispatch slot.

Entry/operand bytes were captured from the running process before it exited.
An attempted comparison with the nearby on-disk D2R.exe did not match the
loaded witness. That file was rejected as a source of additional guard bytes;
no guards were weakened and no static-image identity claim is made.

Supplementary handler census inspected function16 at0x3CC450 (normal path
0x3CC485, assertion helper0x3D2600) and function23 at0x3CCC60 (assertion
helper0x3D1F10). Function12's null-spec assertion helper is0x3D1C70.
Function23 calls0x34B9D0,0x3740A0,
0x373540,0x36D4D0 and0x3D3510. These observations do not establish complete
ABIs and are not new entry points used by revision44. Functions15/16 already
have the paired-endpoint path documented in ELEMENTAL-DECOMPOSITION-REV13.md.

## Source proof and tier families

Each physical source's owned layers for the target stat are projected into
temporary scalar cells. A function12 application becomes an exactly-one choice
over eligible positive observed layers, with fixed strength from the parameter.
Fixed class/tab/skill selectors remain mandatory with their original bounds.
The existing bounded choice solver checks the entire family of stored layers,
including unexplained totals. Only choices forced by all feasible assignments
receive an annotation. This prevents a random skill selected elsewhere from
being attached to an overlapping fixed-skill line.

The proof uses at most256 owned entries,128 projected definitions and512 cells,
and inherits the128-layout/8192-work and4096 integer-search bounds. Ambiguity or
budget exhaustion retains native output. Replacement flags, negative
cancellation involving random selectors, mixed-handler function12 properties,
unresolved conditional groups and unsupported transforms remain unproved.
Unknown implicit targets inside unexpanded groups also refuse the proof;
an unrecognized group cannot be silently treated as an empty contribution.
Fixed-only negative selector bonuses keep their previously supported behavior.
Only positive bounded parameters and valid unsigned WORD selector domains are
admitted. No-clone recovery uses the existing exact localized native formatter
match; candidate identity matching alone does not publish a random source label.

For tier grouping, function12's strength parameter can differ across tiers but
its selector domain must agree. Thus the+2/+3/+4/+5 variants form one family,
while different class pools remain separate. Fixed skill selectors and the
existing function17 coefficient-family rule retain their semantics.

## Coverage and validation

The read-only census found56 function12 affix specifications,145 direct unique
table specifications and3 enabled runeword specifications. These are handler
usage counts, not complete visible-tooltip coverage percentages; conditional
groups and implicit operations have separate limits. All active definitions
remain runtime inputs rather than a shipped named database.

Automated regressions cover all eight installed class selector pools and+2..+5
strengths, selector endpoints, raw random class/tab layers, fixed/random
overlaps, random selection elsewhere, ambiguous equal-strength sources,
replacement/conditional/mixed-handler refusal, negative cancellation, proof
limits, native-clone absence and optional admission refusal. The existing
resistance, Attributes, runeword, quantity, Base-header and toggle suites remain.
Private snapshots and native disassembly remain under ignored `outputs/`.

Revision44 builds with `/W4 /WX`; all nine suites pass, including artifact
ABI/exports/resource/version admission. The final private catalog replay
inspects2026 affix rows and verifies all56 random-selector specifications at
their lower/middle/upper selector layers with fixed parameter bounds. Recipe
lookup still resolves219/219 enabled words, with no ambiguous/refused eligible
base matches. The exact captured revision43 charm replay still renders the
active5..10 Attributes interval. Nine scalar models retain their existing
unsupported-implicit-target refusal; this is not malformed game data.

The game exited after the read-only amulet/native census and before the optional
full amulet replay capture. A fresh capture after the revision44 restart then
replayed its exact Properties/PropertyGroups/MagicAffixes/ItemStatCost tables,
saved IDs, version, quality and owned values through the production path. The
result is `[+5] +5 to Wake of Fire (Assassin Only) [P] [Gnostics] [T1]`.
The fixture formatter initially asserted on unrelated grouped metadata before
checking the requested skill; moving that test-only check after the stat/layer
filter corrected the replay. All nine suites then passed again. The plugin
DLL hash did not change, and no running game DLL was replaced for this repair.

Standalone Reimagined revision44 was deployed after rechecking processes and
target-DLL holders, retaining a verified revision43 backup and every runtime
TOML/companion DLL hash. Built/deployed SHA256 is
`5F2A61C53E30E1511A0E48903F5A9B9325224084A95E0AB402A160AD13256B5B`.
Path: `mods/Reimagined/d2rloader/plugins/Item Roll Ranges.dll` beneath the game
installation. Backup remains under local `Documentation/item-roll-ranges/backups/`,
excluded from source and releases. Default always-show remains false; installed
configuration was preserved byte-for-byte.

Reopened process15216 loaded this exact standalone DLL and the qualified Core
artifact. All15 property/quantity/random-handler byte witnesses match. The
successful annotation and absence of the optional-guard refusal establish
startup admission, whose gate also requires dispatch slot12 to point to0x3CCA50.
This pointer identity is an inference from the admitted production gate, not a
separate post-replay pointer observation: the game exited before that last read.
The current plugin log admits the linked
source model, retains native Ctrl/controller hold mode, and reports successful
`/alwaysshowrolls` registration result0 after its normal UI-queue retry. No
plugin errors or guard refusals appeared in this startup log. The user confirmed
the requested Wake of Fire bounds and source/tier looked good in-game. The exact
amulet replay, native admission, and user-visible check are separate supporting
evidence. Revision44 is included in release-2026.10.14.2; other recipes/items are not claimed
visually tested by this one sample.

### Global vanilla-test installation

At the user's request, the same revision44 DLL was copied to the game's global
`d2rloader/plugins/Item Roll Ranges.dll` for testing without Reimagined. The game
and target DLL were unloaded at installation. The global plugins directory had
no prior Item Roll Ranges DLL to back up. Source/destination SHA256 both match
the tested artifact above; all pre-existing global DLL/TOML hashes were preserved.
The existing global display configuration still has `always_show_ranges=false`.
This verifies the file installation. The user subsequently reported improved
vanilla behavior and authorized production publication. No named vanilla sample
list or separate vanilla table census was recorded; the219-recipe count above
belongs to the captured Reimagined data. Included in release-2026.10.14.2.
