# Rev.47 active-definition catalog audit - 2026-10-16

The user confirmed the Nord, Griswold and Maadi fixes visibly in standalone
Reimagined. An additional optional offline audit reads the private rev.46
compiled-table capture through rev.47 production source/model/merge functions.
It does not redistribute game rows or add a runtime item catalog.

## Coverage and result

Scanned 1,484 unique, 455 set and 2,026 affix rows from the captured active bank.
These counts include table records, not necessarily enabled/spawnable items.

- 1,287 elemental samples passed: minimum, maximum and midpoint values for
  429 fire/lightning/magic/cold row/family combinations. Independent expected
  domains come from supported property function operands; production Build and
  ExpandOwned must preserve both endpoints.
- 2,258 permanent set scalar samples were exercised across 422 set rows.
  2,220 passed; 38 exposed the known incomplete-source-model limitation below.
- No unexpected failures remained. All nine ordinary suites passed.
- Production DLL SHA256 remains
  `6400A101DE80FD38D9F4EB482BB39CE7290AFCFEAD244D7A8FB7A864FDF470CD`.
  This audit changes tests/documentation only; no deployment occurred.

The synthetic stat lists populate shared resistance/attribute operands together
and encode shifted resource values in stored units. Selector-dependent copies,
poison duration/rate, physical paired damage, level operations and conditional
set bonuses are outside this scalar oracle. The elemental sweep supplies the
line identity; it does not establish every native localized line's discovery.
Native text placement, equipped/full-set state and saves were not exercised.
These counts are not a claim that every property on every item is covered.

## Confirmed fallback-model coverage gap

`PropertyRolls::Compile` marks the entire model incomplete when a property
contains an implicit target (`0xffff`) with function20 or function23. In this
active table those operations are indestructible and ethereal. The existing
`BlockPossible` implementation already distinguishes those functions, but the
main compilation path does not. Ordinary independent scalar ranges consequently
remain unavailable through this fallback path. Native clone-derived ranges may
still work; this is not a claim that every listed tooltip is blank in-game.

Affected set records (names resolved through the installed text table's `*ID`,
not physical line position, which includes the Expansion marker):

| Record | Item | Operation |
| --- | --- | --- |
| 69 | Aldur's Advance | Indestructible |
| 75 | Immortal King's Stone Crusher | Ethereal |
| 101 | Taebaek's Glory | Indestructible |
| 111 | Hwanin's Justice | Indestructible |
| 252 | Darkmage's Solar Flair | Both |
| 253 | Darkmage's Super Nova | Both |
| 254 | Darkmage's Meteor Shower | Both |
| 255 | Darkmage's Celestial Fury | Both |

The catalog test prints these as KNOWN COVERAGE GAP rather than counting them
as successful scalar rendering. It still fails on unexpected refusals or wrong
bounds. This limitation is not fixed by the audit. A follow-up should qualify
these functions' complete effects and add focused isolation/refusal tests before
changing the production compiler; do not blanket-ignore unknown operations.

## Useful next visible samples

- Gimmershred or Baranar's Star: separate fire/cold/lightning lines on one item.
- Lightsabre: lightning, with any other property lines preserved.
- Aldur's Advance and Immortal King's Stone Crusher: fallback behavior beside
  indestructible/ethereal properties.

## Reproduction

`range-adapter-tests --reported-capture <private-fixture.bin> catalog`

The fixture uses the already documented IR47/table layouts. No native address,
hook, ABI, guard, game data, player inventory or runtime configuration changed.

## Resolution in rev.48 - 2026-10-17

The main compiler and BlockPossible now share a narrow BlockImplicit classifier:
only function20/23 with target0xffff and no replacement flag receive local
isolation. Their definitions do not supply scalar equations. Both conservatively
block durability stats72/73 and indestructibility152. Function23 also blocks base
armor16, physical damage21-24 and requirement operands91/92. This is a conservative
exclusion domain, not a claim that both handlers directly write every listed stat.
Other properties' independent equations can still be solved. Mixed definitions
remain blocked as a unit, including any stat they could affect. Unknown implicit
functions or replacement-marked definitions still invalidate the model.

Existing evidence: active Properties rows242/264 and source property names identify
these functions; RANDOM-SKILL-PROPERTIES-REV44.md records the function23 handler
census at Game RVA0x3CCC60. No additional native call, address, patch, ABI claim or
header calculation was added. Base/ethereal arithmetic stays in its existing path.

The new focused regressions failed on rev.47 and pass on rev.48 for Set/U/RW
source tags, independent fire resistance, blocked durability/base domains,
unknown function25 and replacement refusal. The catalog now requires gaps==0;
it is no longer allowed to pass with the eight documented refusals.

Final rev.48 result: 1,287 elemental and 2,258 set scalar samples across422 set
rows pass, including the38 samples on all eight listed records. Zero incomplete
models or unexpected failures. All nine standard suites and the original three
captured-item replays pass. Gimmershred and Baranar's Star were user-confirmed on
rev.47. Rev.48 is built but not deployed or published; its visible validation is
pending. The scan retains the limitations stated above.
