# Active property rolls and runeword sources: revision41

2026-10-14 candidate for D2R3.3.93787 / qualified D2RCore1.3.1-beta.
Core SHA256: `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
All RVAs, offsets, strides and native contracts below are build-specific.
No new hook, slot replacement, game-memory write or runtime database is added.
Existing formatter admission/ownership, input queries, session toggle and
`always_show_ranges=false` default remain in place.

## Shared property rolls

Active Properties, MagicAffixes, UniqueItems, PropertyGroups, Items, ItemTypes,
Gems and Runes tables provide definitions. Saved item IDs and owned stat lists
identify sources. A source definition alone does not prove a numeric transform
or uniquely identify a contributor's roll from a final total.

`property_rolls.h` models each property-function roll as one bounded integer
variable. Function1 rolls an additive stat. Function3 copies a preceding
nonzero roll; a zero-inclusive domain cannot prove that relation because the
native function rerolls at zero. Independent function8 and function17
parameter-or-roll forms are included. Replacement flags, unknown operations,
unresolved groups and non-additive ItemStatCost operations refuse the affected
equations. A zero value from a refused defined source is never silently omitted.
Unresolved weighted groups taint every potentially affected stat; malformed
or cyclic groups refuse the model. Ambiguous weighted unique candidates are
not admitted as independently additive modifiers. Existing witnessed unique
marginal bounds for the observed selected stat remain available; that proof is
distinct from assuming all pool candidates are present simultaneously.

Owned stat totals constrain connected variable components. Bound propagation
plus bounded integer feasibility rejects inconsistent components. Only uniquely
constrained contributions receive actual values; ambiguous decompositions retain
`?`. Displayed bounds remain the definition's roll domain, rather than narrowing
to the roll already obtained. No Prismatic/Russet/Viridian/item-name checks exist.

For example, a shared16..20 resistance roll and Cold20/Poison20/Lightning50/
Fire32 prove shared20, fire-only12 and lightning-only30 when definitions and
ownership agree. All four overlap positions are tested. Grouped native lines
require equal current group members/shifts and identical linked-variable terms.
The prepared Crafted Flame Rift uses active All Attributes5..10; the reported
Renewed item's claimed3..8 is a different definition. Both domains are tested,
and neither domain is hardcoded in the DLL.

Functions10/21/22/24 expose marginal bounds at verified class/tab/skill layers,
including selector0. Packed proc/charge values are not scalar rolls. Fixed
positive proc chance/level reuses the qualified proc path. No-clone recovery
uses the admitted native single-value formatter and exact, unique localized
line matching. Unknown operations retain native display.

## All active runeword recipes

`runeword_sources.h` reads the active compiled Runes table, with no known-name
list or shipped vanilla/D2RR stat database. It requires Normal/Superior quality,
flag0x04000000, no preview flags0xC0000000, reciprocal owned sockets in physical
insertion order, socket stat194 equal to that count, one enabled recipe matching
ordered ingredients (including repeats) and included/excluded type ancestry,
current SDK bank/stride/count/revision/pointers, and exactly one permanent
host-owned state171 modifier list.

Runeword-owned values remain separate from host and socket lists. Proved
additive contributions must sum to the actual item. Socket rune/gem effects
come from current Gems rows for weapon/armor/shield context. The code does not
assume every recipe always adds ordinary rune effects: unsupported replacements
and unexplained sums cannot receive a speculative split. `[RW]` identifies
runeword-owned properties; sockets retain `[Socket n]` and active rune/gem names.
Empty PropertyGroups tables remain valid for direct unique/recipe properties.

Prepared Bone has state171 mana119, +2 Necromancer and two15% procs. Sol supplies
DR7; the two active Um sockets supply20 resistance each. The reporter's Bone
has117 mana/30 resistance, a separate item/data version. Local values are not
substituted for that report.

## New native evidence

Read-only process26744, game base0x140000000, Core base0xC0DE5000000; absolute
addresses are evidence only, runtime uses module bases. New semantic guards
live in `property_source_profile.h`.

| Module/RVA | Meaning and exact witness bytes |
|---|---|
| Game0x3CFB60, witness0x3CFBD7 | Function3 reads preceding value at native frame+0xC0; nonzero copies, zero rolls via0x3D5860. `8B8424C000000085C0750F448B470C488BCB8B5708E86F5C0000` |
| Game0x3D44E0 | Rune property application obtains state171, flags64, accessor0x2F5B80. `BAAB000000488BCF448D4295E88F16F2FF4885C0` |
| Game0x3143B0 | Runes getter calls table-bank getter0x300A90, adds0x1860. `4883EC28E8D7C6FEFF4805601800004883C428C3` |
| Core0x38192D | Enabled byte+0x44 and localization WORD+0x46. `807944000F84AA0000006683794600` |
| Core0x381945 | First ingredient signed DWORD+0x5C, -1 sentinel. `8B415C83F8FF` |
| Core0x381A01 | Runes row stride232. `4969C8E8000000` |
| Game0x3CC59B | Function17 chooses nonzero parameter+4 rather than random min/max. `8B5F0485DB751C` |
| Game0x3CC980 /0x3CC9C7 | Function22 rolls spec+8/+12 via0x3D5860. `448B430C488BCF8B5308E88A8E0000` |
| Game0x3CCA09 | Function22 parameter WORD+4 becomes layer. `0FB743046689442438` |
| Game0x3D10A0 /0x3D10E7 | Function24 uses preceding nonzero frame+0xB0 or rolls spec min/max. `8B8424B000000085C0750F448B430C488BCF8B5308E85F470000` |
| Game0x3D110C | Function24 parameter DWORD+4 becomes layer passed as WORD. `8B5304` |
| Game0x3D5940 /0x3D5A1F | Shared application shifts roll by ItemStatCost ValShift, forwards stat/layer to add/set0x2F3330/0x2F7C00. Supplementary disassembly; existing consumers normalize signed low DWORD. |

Runes pointer/count at native table+0x1860/+0x1868. Row: six included type WORDs
+0x4A, three excluded+0x56, six signed ingredient DWORDs+0x5C, seven16-byte
`{property,param,min,max}` specs+0x74 (**not0x78**). Item type fields+0x12E/+0x130,
Items stride448, ItemTypes stride232, parent WORDs4/6 and table+0x1348/+0x1350
reuse revision27's qualified contract. Compiled Properties begin with uint16 ID,
then seven replacement flags at bytes2..8 (active color property74 confirms).
Dispatch pointer table Game0x2386AB0: function1 at0x3CF960, function8 at0x3D0EC0.
Source group expansion continues to use documented modes/counts/parameter
bounds; uncertain candidates cannot become an additive split.

## Replenishes Quantity

Both repository datasets (`excel` and `excel/base`) contain D2RR definitions;
`base` is not asserted to be unmodified Blizzard data. Both and installed source
normalize replenishing suffix/unique parameters to25. Runtime reads the active
compiled parameter, never the files or historical vanilla values in the report.

Table notes use nominal100/coefficient seconds. This build's recurring event
delay is more specific: `max(125, floor(2500/coefficient) + 1)` simulation frames
at25 frames/second. Display uses that recurring interval with exact hundredths,
shortest to longest. Coefficient25 yields1 per5s; coefficient5 yields1 per20.04s.
This is a tooltip conversion, without gameplay changes.

| Game RVA | Scheduling evidence |
|---|---|
| 0x43EAA0 /0x43EABB | Initial event3 scheduler reads stat253 via0x2F5020; initial delay lacks recurring125-frame clamp. `BAFD000000E85B65EBFF8BF0` |
| 0x476060 /0x4760E3 | Recurring handler checks quantity, reads stat253, increments quantity stat70 via0x46F4F0. `BAFD000000488BCB8BE8E82EEFE7FF8BF085C07453` |
| 0x476117 | 2500/rate division, +1, minimum125; schedules event3 via0x48B720 using current frame at game-context+0x170. `B97D000000B8C409000099F7FE448D4186488BD3FFC03BC10F4CC1488BCF` |
| 0x3706E7 /0x5DF325 /0x5ECEE7 | Other stat253 consumers found read-only; not called or depended on by this plugin. |

Quantity cadence has a separate optional guard. Mismatch retains native text.
Timing is derived from qualified code, not a live stopwatch measurement.

## Validation and limitations

Nine suites exercise production captureâ†’definitionsâ†’merge: inverse resistance
rolls, grouped/direct unique attributes with/without PropertyGroups, runeword
mana/class/skill layers, fixed/variable replenishment, Busy/drift/invalid sums,
owned sockets, Base headers and artifact admission. Solver fixtures compare343
integer systems with exhaustive reference assignments. Existing hold/controller,
`/AlwaysShowRolls`, byte-width fixed-Sash and full tooltip-order checks remain.

Optional `range-property-roll-tests <local active-property-tables.bin>` replays
compiled rows without redistributing them. All219/219 enabled recipes resolve
across eligible bases, with zero ambiguous/refused recipe/base matches. They
contain1378 specs;824 variables are admitted by the linked scalar compiler.
That count is not a complete tooltip-coverage percentage: selectors, procs,
physical and paired stats have separate paths; other transforms remain native.
Installed source has219 completed recipes, repository datasets209. Installed
Purity references missing `res-max-all`; runtime follows compiled data rather
than manufacturing that definition.

Raw tables/disassembly/output stay in private `outputs/`, never source/release
fixtures. Read-only observations and recipe replay prove data/contract coverage.
Revision43's sampled visible confirmation is recorded below; no public release
is claimed. Set bonuses, changing enchantment recipes, additive splits
of ambiguous weighted groups, automatic proc levels and unverified transforms
remain outside numeric attribution. No arbitrary formatter composition is added.

## Revision42: weighted selection and single-source endpoints

The live revision41 charm still lacked Attributes bounds: active group23 has
three candidates (FRW10..20, FHR15..30, All Attributes5..10), not one mandatory
modifier. Native mode2 and count1..1 select exactly one weighted entry. Its
actual four Attributes10 values prove the Attributes alternative; the earlier
additive-only model correctly refused the alternatives but missed that proof.

Source expansion now retains bounded pool/entry identity separately from
unconditional scalar proof and existing marginal unique bounds. `SolveSources`
enumerates at most128 layouts per connected pool component, with an8192
definition/layout work bound. It considers all possible affected stats, including
the requirement that a missing modeled stat has zero contribution. A choice
becomes unconditional only when every feasible layout selects it. Unknown
operations cannot eliminate an alternative. Ambiguity, unsupported pool shapes,
randomized parameters, nested choices or budget exhaustion retain the previous
display. No pool or affix names are used by the solver.

Raven Song (class47, item52 in the read-only capture) has Scorching prefix1167:
property20 rolls fire minimum121..170, property21 maximum181..240, actual156/181.
The scalar fallback could not attach its one-number bounds to that two-number
line. The new paired path verifies both owned endpoint sums and uses their
separate domains, including a single host affix without external sockets.

The complete active catalog audit inspected2026 compiled affix rows:1154 have
linked scalar variables,1045 contain operations refused by that scalar compiler,
zero models have malformed/unresolved metadata. These counts overlap; other
operations use dedicated native/paired/scaling/layer paths. They are not a
percentage of unsupported item tooltips. The219 enabled recipe replay remains
passing. All13 newly qualified witnesses match reopened process40456. Revision41
loaded successfully there, but its two visible gaps above are user-reported.
Revision42 synthetic production fixtures pass; visible restart proof is pending.

Standalone Reimagined test installation: revision41 was backed up, then
revision42 deployed with SHA256
`AC6AAF98F7452487D1E75FBBF72A9A445F4639500921AEAEFED06369315CE1BE`.
Path: `mods/Reimagined/d2rloader/plugins/Item Roll Ranges.dll` beneath the game
installation. Companion hashes and all runtime TOML hashes were preserved.
The game was closed and the DLL unloaded at replacement. The D2RR launcher
was not required to close. Revision40 and41 rollback artifacts remain local
under `Documentation/item-roll-ranges/backups/`, excluded from source/releases.

## Revision43: actual item operand metadata

Revision42's live Attributes prefix still failed. The exact captured unique433
replay verified weighted selection and stat0, but grouped members1/3 were omitted
by the normalization filter: native ItemStatCost+0x50 contains op8 for Energy1
and op9 for Vitality3. Strength0 and Dexterity2 have op0; all four have ValShift0
and group1. Their captured item-owned values are10 each. Ops8/9 consume attribute
points for derived character resources when equipped; the property functions
store/display the attribute operands unchanged. Existing function1/3â†’0x3D5940
shift/store and single/group formatter contracts establish the item operand
boundary. Rejecting every nonzero operation incorrectly excluded plain attribute
points. `ItemScalarOperand` now admits0/8/9; level/scaling transforms retain their
separate paths. Synthetic grouped tests now contain the actual8/9 metadata.

`range-adapter-tests <local rev41-unique-roll-fixture.bin>` replays current
Properties/PropertyGroups/UniqueItems/ItemStatCost rows and owned values through
the production label/merge path. It now renders `[+5 - +10] +10 to All Attributes
[U]` without a missing-range warning. Raw captured data remains private. Nine
suites pass. Solver outcomes distinguish proven inconsistency from unknown
budget exhaustion; unknown implicit targets cannot establish a false selection.
The user confirmed the Attributes prefix after the revision43 restart.

Standalone revision43 deployed SHA256
`9D1AD86DD70078C09FB291BDD2F64A2CCF76BB448C51265013AF6D816C9482A2`,
with revision42 backup and preserved config/companion hashes. The user confirmed
the Attributes prefix after restart. Nine suites and the full private captured
charm replay pass. The final catalog pass still inspects2026 rows/219 recipes;
9 scalar models now decline unsupported implicit targets (these are not claimed
to be corrupt game data). Recipe lookup remains219/219 with zero ambiguous
matches. Numeric completeness and visible validation are separate from lookup.

### Revision43 live validation, 2026-10-14

After confirming the charm's Attributes prefix, the user reported that all five
requested validation checks looked good on the active standalone Reimagined
installation:

- Prismatic amulet resistance overlap and contribution breakdown.
- Bone's runeword bounds and separation of word-owned versus socket sources.
- Raven Song's independently rolled fire-damage endpoints.
- Quantity-replenishment interval/source display.
- Ctrl hold/release and `/AlwaysShowRolls` on/off behavior, including the plain
  Sash header regression.

The user additionally tested Lightning, a Reimagined runeword, and Honor, a
vanilla-origin runeword, and reported both looked good. Both still resolve from
this installation's active compiled definitions; a vanilla-origin recipe name
does not select a separate vanilla stats database.

These are user-confirmed visible sample checks. They complement the nine
automated suites and219-recipe lookup replay; they are not a visual audit of
every runeword, a stopwatch measurement of replenishment timing, or confirmation
of previously excluded transforms. No code or runtime configuration changed
while recording these results. Revision43 remains unpublished.

Revision44 extends the same bounded source proof to random skill/class/tab
layers. See [the random-selector contract](RANDOM-SKILL-PROPERTIES-REV44.md)
for the loaded operand witnesses, tier families, coverage and validation limits.
