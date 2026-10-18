# Fresh Reimagined definition inventory - 2026-10-17

Read-only PID52088 active-bank3 capture used the previously documented layouts
and matched all nine socket-profile byte witnesses. Table pointers/counts were
rechecked after each read. Eleven captured tables: Properties438, groups42,
UniqueItems1484, SetItems455, ItemStatCost441, affixes2026, Items936,
ItemTypes122, Gems82, CubeMain16142 and Runes220. Raw bytes and detailed per-row
reports remain private under outputs/definition-audit-20261017.

The inventory expands viable group entries as alternatives and records source,
record, property, operands, function/stat targets and group ancestry. It traces
19,568 references to250 property rows and21 function IDs. No invalid reference
or group-depth violation was found in the documented fields scanned.

Rev48 production tests on this fresh capture:1,287 elemental and2,258 permanent
set scalar samples pass, zero incomplete-model cases. All219 enabled runeword
recipes resolve for eligible bases. All2,026 affix models are structurally
complete;1,154 have scalar variables and1,046 have blocked scalar operations
(overlapping counts).56 random-selector specifications pass layer-bound checks.
These results do not certify every property's native text or every composition.

Priority review families are function2 enhanced defense (622 references),
function13 max-durability percent (2 references), function14 sockets (347),
packed procs/charges, mixed selectors and group choices. Separate paths exist;
REVIEW classification is a testing gap, not a proven tooltip defect.

Uncovered scope is explicit: global Sets bonus definitions/state transitions,
all cube output variants (only first output five slots inventoried), base-item
eligibility/automod assignment, recipe attribution, native localized discovery,
and a fresh vanilla comparison. No new RVA/layout/ABI was inferred and no
runtime code, configuration or deployed binary changed during this audit.

## Runtime name correction - 2026-10-17

The UniqueItems compiled row stores its internal name at byte2. Captured row894 is Gemini Coat (durability+25%), and row900 is Xanadu Dreams (durability+100%). These are the audited function13 definitions and the items produced by the supplied commands. The installed text table *ID values do not match these compiled row positions; mapping them to Steelflesh/Shattering Blow was incorrect. Captured rows896/902 have those names and no function13 property. Use runtime compiled identities for future test-item selection.
