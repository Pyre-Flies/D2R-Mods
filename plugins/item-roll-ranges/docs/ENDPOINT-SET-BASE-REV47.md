# Endpoint ranges, permanent set properties and implicit base stats - rev.47

Target: D2R 3.3.93787 and the existing qualified D2RCore 1.3.1-beta profile.
All addresses, layouts and contracts here are build-specific. No additional
hook, patch, ABI call or predecessor composition is introduced.

## Reproduced failures

A read-only rev.46 capture from process43228 contains Nord unique384, Griswold
unique30 and Maadi set437, with reciprocal permanent state0 modifier lists.
Private snapshots/replays remain under local outputs and are not distributed.

- Cold205/455 and length50 are fixed property-function15/16/17 operands. The
  numeric paired model already handles them. Unrecognized scalar/nonnumeric
  lines incorrectly participated in two-number composite identity cardinality.
  Limit that fallback to two-number candidates; keep exact family counts,
  source identity, unique display priorities and ambiguity refusal.
- Griswold's active fire minimum10..12 and maximum15..25 are two separate
  property-function1 specs. Keep their marginal domains separate; permit
  partial endpoint blocks only inside complete, owned-source reconciliation.
  Named unique/set/word blocks from the same source become one paired row.
  Unknown/replacement operations and conditional/contradictory values refuse.
- `RangeText::DisplayBounds` and its prior regression explicitly collapsed
  two native intervals to their outside endpoints. Correct both to preserve
  the two intervals, independent of whether an owned-source proof exists.
- Maadi's active permanent stats are All Skills1..2, FCR30, MF10..30 and custom
  Bone Spear projectiles1. Direct set definitions were absent from fallback.

## Qualified set layout

Existing Core selector witness `VisibilityProfile::SetSelector` at0x377AF0
reads table-bank SetItems rows/count at+0x13A8/+0x13B0, stride0x1CC. It copies
nine16-byte permanent specs beginning+0x98, then ten conditional specs at+0x128.
The captured set row independently confirms All Skills at+0x98, custom
projectiles at+0xA8, FCR at+0xB8 and MF at+0xC8. Parse only the nine permanent
specs using the same bounded property/group expansion as unique definitions.
Use `[Set]` to distinguish set properties from the `[S]` suffix marker.
No gameplay flag or original definition row is changed. Active set-bonus lists
remain outside the owned-source snapshot contract and decline numeric recovery.

## Formatter-only undead bonus

The already-admitted `FormatterProfile::Normalize` at Game0x2DB700 includes:

- 0x2DB7BA: compare stat with122 (`0x7A`).
- 0x2DB7C8: unit-type query0x34B9D0; require item type4.
- 0x2DB7D2: set requested item type57; predicate0x373890 at0x2DB7D8.
- 0x2DB7E1: add50 to the normalized display value.

The capture contains no stat122 in either Nord's or Maadi's stored primary,
permanent modifier or aggregate vectors. This is not an unknown rolled affix.
Reconcile it using the existing qualified448-byte base-row getter0x314110,
primary/secondary type fields+0x12E/+0x130, active ItemTypes rows/count at
bank+0x1348/+0x1350,232-byte rows and parent WORDs+4/+6. Existing contribution
witnesses qualify the type fields and ancestry; require active type57 code
`blun`, acyclic bounded ancestry, an additive active stat122 property, matching
owned/aggregate totals, and exact localized single-value formatter output.

Add an owned local `[Base]`50 contribution to the display model only. Preserve
actual native text and all game memory. Separate any proved rolled/socket
contributions; unsupported recipes, set-bonus states, missing metadata or
contradictory totals keep native output. No new native entry point is introduced.

## Validation status

Regression coverage includes four elemental families (fire/lightning/magic/cold)
across unique/set/word source tags, native independent intervals, fixed endpoints,
replacements/unknown operations, impossible values, unrelated lines, ambiguous
candidate rejection, permanent set skills/MF/custom stats, conditional exclusion,
and base-only/overlapping undead contributions with malformed ancestry and
mismatched values. All use production capture/model/merge functions.

Captured replay command: `range-adapter-tests --reported-capture <private-fixture>
<cold|fire|set>`. The three failures were reproduced before the respective fixes.
`range-text-tests` also reproduced the incorrect flattening rule; the implicit
base regression failed before its label implementation. Automated output does
not establish visible game rendering. Visible rev.47 confirmation is pending.

The final Release build passes all nine suites and all three private captured-item replays. The DLL artifact suite verifies rev.47 metadata, exports and ABI manifest. Live visual confirmation remains pending.

User subsequently confirmed all three reported item fixes visually. See CATALOG-AUDIT-REV47.md for the broader scan and remaining fallback limitation.
