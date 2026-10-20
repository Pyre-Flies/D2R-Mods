# Stash and waypoint priority - rev.43

## Scope

Rev.43 extends the existing neutral Interact priority pipeline to the actual
town stash and waypoint objects. It reuses the three guarded candidate-contact
CALL patches and the existing score, range and comparison hooks. No new RVA,
hook entry, packet, object operation or retained unit pointer is introduced.

`ground_pickup` now gates only direct LB loot. Neutral A priority is admitted
when any of `prioritize_portals`, `prioritize_stash_boxes`, or
`prioritize_waypoints` is enabled. All three families share the existing
`portal_priority_distance` setting to preserve configuration compatibility.

## Active table evidence

Inspected table:
`mods/ReimaginedLadder/ReimaginedLadder.mpq/data/global/excel/objects.txt`,
SHA-256 `45851636360723F5E1B3DE98207625748AF23546D40918C1BC8254A263F2747B`.

| Object family | Stable classifier used | Supporting table fields |
| --- | --- | --- |
| Portal | compiled ObjectsTxt `SubClass & 0x04` | Portal records use SubClass 4 and OperateFn 15 |
| Town stash | native class ID `267` | Class `Bank`, Name `bank`, OperateFn 32 |
| Waypoint | compiled ObjectsTxt `SubClass & 0x40` | all inspected Waypoint records use SubClass 64 and OperateFn 23 |

The existing native lookup returns the class ID and compiled record for the
live object. Rev.43 still reads only the previously established compiled
SubClass byte at `record+0x127`; OperateFn is corroborating table evidence, not
a newly inferred compiled-record offset. Class ID 267 is intentionally exact.
Text rows named hidden stash, ordinary stash, chest, or hiddenstash do not
qualify unless they are the Bank class; treasure objects are therefore not
promoted accidentally.

## Ranking difference from portals

Native comparison already prevents a later item from displacing a selected
portal, but that rule tests only portal SubClass. Rev.43 adds a scoped retention
step for a selected qualifying stash or waypoint: the original comparison runs
once, and if a later item displaces that object, the prior selection and score
are restored. Conversely, while enabled direct-loot modifier input is held, a
priority-object candidate cannot replace the existing loot selection. Object
versus object, NPC, skill and non-Interact ranking remains native.

## Evidence and validation boundary

Static/automated coverage checks portal/stash/waypoint classification, rejects
hidden stash class 159, exercises independent family switches, boundary and
modifier policy, and runs the existing native signature/artifact suites. Live
validation remains required for A near loot plus each town stash and waypoint,
LB+A regression when direct loot is enabled, no-LB behavior with
`ground_pickup=false`, out-of-radius loot, and ordinary chest interaction.
