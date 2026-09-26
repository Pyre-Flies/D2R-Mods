# Affix and tier extension: research ledger

Status: initial investigation, superseded by AFFIX-TIERS-LIVE.md for 0.4.0. The 0.3.3
two-pass formatter remains the baseline. Keep the actual native property text,
light-blue range prefixes, Ctrl/R1 and existing panel restrictions.

Requested presentation: `[+6 - +9] +8 to Minimum Damage [Suffix] [T1]`.
Proposed tier convention: T1 is the strongest eligible member of the affix
family for the base item type, independent of the item's current level.
The user has been asked to confirm this convention.

## Public SDK evidence

`Documentation/PluginSDK-master/include/D2RLPlugin/item.h`: ItemInfo exposes
three prefixIds and three suffixIds. The contract explicitly calls these native
one-based MagicAffix IDs. `handles.h` explicitly forbids treating native item
pointers as public ItemHandles. The current tooltip source pointer cannot be
passed directly to ItemService::getItemInfo.

`data_tables.h`: MagicAffixes is table 60, combining MagicSuffix, MagicPrefix
and AutoMagic. Properties is table 16; ItemStatCost is 35. TableView pointers
are borrowed, game-thread-only, and invalidated by the next table load. SDK
table access outside its captured game thread returns Busy. Do not retain a
borrowed row in a tooltip cache across reloads.

`item_interactions.h`: its event requires an activation, does not cover vendor
or shared-stash interactions, and is not a hover metadata API. It cannot serve
as the general source of affix metadata for this plugin.

## Static evidence from the qualified D2RCore

Image SHA-256:
`AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0`.
All addresses below are module-relative RVAs, never process addresses.
These are observations, not permission to invoke an unverified ABI.

| Location | Observation |
| --- | --- |
| Core+0x78E360 | Export SelectExpansionMagicAffix |
| Core+0x78E3AA..0x78E3B7 | Reads unit byte +0x1BD and calls table accessor through Core+0x67F790 |
| Core+0x78E3C4..0x78E466 | Table object: combined affix rows +0x15E8, count +0x15F0, partition pointers +0x1600/+0x1608/+0x1610; row stride 0x8C |
| Core+0x78E515 | Getter through Core+0x67D2E8 with (item, index=0) |
| Core+0x78E53A/0x78E57F/0x78E5BE | Getter through Core+0x67D300 with (item, index=0/1/2) |
| Core+0x78E549..0x78E568 | Validates one-based affix ID, subtracts one, indexes stride 0x8C and reads group DWORD +0x5C |
| Core+0x78E6EB | Row byte +0x54 gates one selection mode |
| Core+0x78E6F2 | Row version WORD +0x22 compared with 100 |
| Core+0x78E718 | Row DWORD +0x58 feeds selection weighting |
| Core+0x78E738 | (item, affix row) predicate through Core+0x67CCB8; full eligibility semantics not yet verified |
| Core+0x78E777 | Row byte +0x66 is compared with a class value, with 0xFF as unrestricted |
| Core+0x78E75E/0x78E7D9 | Row byte +0x82 is a nonzero selection gate and weight multiplier |
| Core+0x379CC0 | Copies an affix row (0x8C bytes); copies +0x30/+0x40/+0x50 into paired fields for upper-bound construction |
| Core+0x792D9B..0x792DDA | Lower-bound construction copies +0x2C/+0x3C/+0x4C into paired fields |

Getter identities (prefix versus suffix), partition ordering, level fields,
property-to-stat mapping and the type predicate still require verification.
ReadItemFormatMagicAffix (Core+0x7A7260) is a serialized-format reader, not an
item affix getter; do not use it for this feature.

## Local table evidence and implementation boundaries

Active loose tables live under
`mods/Reimagined/Reimagined.mpq/data/global/excel/`.
magicprefix.txt contains group, level, maxlevel, levelreq, spawnable, rare,
mod1..3 code/param/min/max, included types and excluded types. There is no
explicit Tier column in its header. properties.txt maps property codes to up
to seven stat/function pairs; a property is not necessarily a single stat.
Tier inference must use the active data, not a bundled vanilla ranking.

Do not infer Prefix/Suffix from displayed values alone: overlapping affixes,
hybrid properties, sockets, automagic and crafted fixed properties can share
stats. Unique/set properties are not automatically prefixes or suffixes.
Combined lines need provenance from each contributing affix or must omit an
ambiguous label. Original-value preservation is mandatory regardless of whether
metadata is available.

Next validation: inspect the live getter targets and loaded table partitions
with a magic/rare item available. Then establish stat-line provenance and add
bounded, capacity-safe labels to the existing actual/range merger. No new hooks,
table accesses or tier labels have been activated by this research change.
