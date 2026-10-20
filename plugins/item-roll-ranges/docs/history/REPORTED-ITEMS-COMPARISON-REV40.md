# Prepared item comparison, rev.40, 2026-10-14

Read-only loaded item/table capture on the qualified local game/Core profile.
The user prepared three samples for reports of grouped unique attributes,
overlapping resistances and missing Bone runeword modifier ranges. No native
function was invoked and no game memory, item or installed file was changed.
The bounded existing socket/layout witnesses matched before scanning.
Raw process snapshots remain local under outputs and are not release assets.

## Charm is a different definition

Prepared item: unique class 691, record 433, `Crafted Flame Rift`.
The owned permanent stat list contains Strength/Energy/Dexterity/Vitality
stats 0/1/2/3 at 10 each. Active unique groups expand property 251 (`all-stats`)
to a 5..10 roll in group 23, mode 2, count 1..1. The current loaded table
contains Flame Rift, PreCrafted Flame Rift and Crafted Flame Rift; its names
do not include the reported Renewed Flame Rift. The reporter's claimed 3..8
range must not be substituted into this local record.

Active property 251 handlers: function 1 -> stat 0, then function 3 -> stats
1, 2, 3. This is a shared-roll family. Current UniqueRange declines grouped
display lines; UniqueScalarBounds additionally supports only functions 1/8.
The missing attributes range is a model coverage gap, not absent definitions.

## Amulet shared roll

Prepared rare amulet class 535 has Prismatic ID 1217 (property 41, 16..20)
and Viridian ID 1202 (property 39, 11..20). Other affixes are Vulpine,
of Perfection and of the Dynamo, with no resistance contribution in this sample.
Permanent owned stats: fire 39=20, lightning 41=20, cold 43=20, poison 45=32.
Thus the shared Prismatic roll is 20 and Viridian poison contribution is 12.
No socket child or corruption contribution appears in this sample's stat lists.

Active property 41 handlers: function 1 -> fire 39; function 3 -> lightning 41,
cold 43, poison 45. Property 39 is function 1 -> poison 45. ScalarBounds excludes
function 3. The reported amulet instead overlaps on fire, which is the first
function-1 slot and takes a different existing supported breakdown path.
Reporter screenshot cold/poison 20 and fire 32 imply Prismatic 20 and Russet 12
provided their definitions and contributions agree. Lightning 50 with Coral
then implies Coral 30. Those reporter inferences are screenshot comparisons,
not live captures of the remote item.

## Bone runeword

Prepared armor class 324, Normal quality, item flags 0x04802811, has the active
runeword flag 0x04000000. Distinct host-owned permanent list (observed state 171)
contains mana stat 9=30464 (119 at shift 8), Necromancer class skill stat 83
layer 2=2, get-hit proc stat 198 layer 5386=15 and striking proc stat 201
layer 4362=15. State 171 is an observation here, not a universal admission rule.
Separate attached socket sources are Sol class 636 (damage reduction 7),
and two Um class 646 (all four resistances 20 each). These yield total all
resistances 40. Attachment traversal lists Um/Um/Sol in reverse insertion order;
that traversal must not be treated as proof of runeword recipe order.

Local runes.txt row Bone1 declares Sol/Um/Um and mana 100..150, fixed +2 Necro,
15% level-10 Bone Armor when struck and 15% level-10 Bone Spear on striking.
This source matches captured effects but was not matched against a loaded
compiled Runes row by this read-only capture; automatic identity remains pending.
The remote screenshot mana 117 and resistances 30 is a different item/definition
combination and must not be used to overwrite this sample's observed values.

Rev.40 does not bind Runes definitions into its source model. Existing related
research at the sibling item-sdk's LISTING-CONTEXTS-AND-RUNEWORD-IDENTITY.md
requires active flag, ordered socket inventory, unique compiled recipe match,
base restriction/code agreement and modifier-block agreement. Reuse that proven
contract rather than infer a source from unordered children or tooltip name.

## Baseline correction

Define support by property-function semantics and source binding, not affix
names. Shared-roll functions need a property-level relation reused for attributes
and resistances; stat totals alone do not always identify individual host rolls.
Cross-stat deductions are valid only when all contributing sources are accounted
for. Runewords require an additional source family, rather than special-casing
Bone. Keep remote/local definition differences explicit and maintain fixtures
for overlapping, copied, ambiguous and externally augmented contributions.

This comparison adds no new native ABI, offset or executable contract. Existing
unit/data/stat-list/table offsets were reused. No plugin fix is claimed here.
