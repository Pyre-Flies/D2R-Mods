# Shrine/chest priority and feature gates - rev.44

## Rev.46 well-family correction

Live rev.45 testing found that wells were not consistently promoted. The
original table interpretation covered the HealingWell/ManaWell family, whose
records carry the shrine SubClass bit, but missed the environmental
Fountain/Well family. In the inspected active table, all 14 of those records
use OperateFn 22 and have SubClass 0 or 32.

Rev.46 keeps wells under the existing default-on `prioritize_shrines` option
and adds an exact class allowlist rather than broadly admitting every object
with SubClass 0 or 32:

`111, 113, 115, 118, 130, 132, 137, 138, 322, 426, 493, 498, 513, 519`.

These are the complete OperateFn-22 rows in the named active table. This is a
table-specific policy correction; it adds no hook, RVA, layout assumption, or
native operation replacement. Actual use remains on the native Interact path.

## Rev.45 shared class-getter admission

The user's rev.44 log showed the entire priority feature failed closed because
game `0x349860` began with an existing five-byte `E9` detour. A captured runtime
image showed the following 27 bytes remained identical to the reviewed class
getter. Rev.45 admits either the original five-byte prefix or an `E9` resolving
to committed executable memory, still requires the exact tail at `0x349865`,
and calls the current entry. QOL neither patches this entry nor calls around its
owner. An unknown prefix, invalid target, or tail mismatch disables priority.

The updated `audit_portals.py` evidence comparison passes all 20 sites against
captured image SHA-256
`A09EA269F8FC10C538DC9A05E4E81E004EC6DFC84AA0F685D3527A974A542A87`.
This establishes static compatibility with that capture, not live selection.

## Priority scope

Rev.44 reuses the rev.43 contact, scoring, comparison and Interact-range
pipeline. It adds no hook or RVA.

- `prioritize_shrines = true` admits ObjectsTxt records whose established
  compiled SubClass byte at `+0x127` contains bit `0x01`, plus the exact rev.46
  environmental-well allowlist above. Healing/mana well records use the shrine
  bit and OperateFn 2; Fountain/Well records use OperateFn 22 instead.
- `prioritize_chests = false` admits an exact allowlist of 47 ordinary chest
  class IDs whose active-table records use the standard chest operation and a
  chest-family Class/Name. It deliberately excludes Horadric/Khalim quest
  chests, hidden stashes, StoneStash, bodies, urns, baskets, barrels, evil urns,
  scenery and generic treasure objects.

The inspected active table is
`mods/ReimaginedLadder/ReimaginedLadder.mpq/data/global/excel/objects.txt`,
SHA-256 `45851636360723F5E1B3DE98207625748AF23546D40918C1BC8254A263F2747B`.
The chest allowlist is therefore an exact-build/data-table contract. It is
preferable to treating every SubClass-8 object as a chest, which would capture
132 heterogeneous records in this table.

Ordinary chest class IDs admitted by this revision:

`5, 6, 87, 88, 139, 140, 141, 144, 146, 147, 148, 176, 177, 181, 183,
198, 240, 241, 242, 243, 246, 329, 330, 331, 332, 333, 334, 335, 336,
387, 389, 390, 391, 397, 413, 420, 424, 425, 430, 431, 432, 433, 455,
501, 502, 504, 505`.

## Batch feature gates

- `identify_all = true` gates both invocation and advertised hints for the
  tome-based batch. Single-item identification remains controlled by the
  existing `quick_identify` setting.
- `quick_deposit = true` gates modifier+L3 Stash All, its callback admission,
  and header/tooltip hints. Ordinary modifier+X item transfers remain
  controlled by `quick_move`.

Both new gates default true to preserve existing configurations. The master
feature switches still apply: `quick_identify = false` disables all quick
identification, and `quick_move = false` disables all quick movement.

## Validation boundary

Automated tests establish classifier inclusion/exclusion, family-enable policy,
priority distance/modifier behavior, artifact version and existing regressions.
They do not prove a shrine, well or chest wins visibly in game. Live testing should
cover a shrine, an environmental well and an ordinary chest beside loot, a quest chest exclusion,
`prioritize_chests=false/true`, each batch gate disabled, and ordinary
single-item Identify/LB+X behavior after disabling the corresponding batch.
