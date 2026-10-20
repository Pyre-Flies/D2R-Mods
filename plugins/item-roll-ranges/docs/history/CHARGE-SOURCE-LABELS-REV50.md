# Charged-skill source-only labels - rev.50

User decision: charges need only affix type, name and tier. Do not calculate or
show charge numeric ranges, including skill-level intervals, capacity intervals,
or missing-range indicators. Unique/set/runeword sources retain their normal
source tag rather than inventing an affix tier.

Existing 2026-10-17 evidence: item stat204 packs skill and realized level into its
layer using the active table's skillBits/mask. The eight prepared magic items
include negative function19 level operands. The saved affix IDs identify the
source definition; a negative operand requests native calculation rather than
equalling the realized level. See LIVE-GAP-BASELINE-20261017.md for the one-cast
remaining135->134 comparison, unchanged capacity142 and level11. No charge
formula, new native hook, RVA or ABI is introduced by this change.

Ordinary Contributes numeric matching stays strict. A separate source-only
encoding mode admits function19/stat204 with a negative level operand by matching
its explicit skill selector to the observed packed layer. It requires valid
encoding, nonzero realized level and nonreplacement operation. Positive levels
still match exactly. A label requires one unconditional saved source candidate;
multiple candidates are omitted. This does not prove negative-operand numeric
bounds and must never be used to supply them.

The property adapter marks witnessed stat204 lines source-only. The text merger
keeps the actual line intact, appends the verified label, and bypasses native or
reconstructed prefixes and missing-range markers. Ambiguous repeated text remains
unmodified. Synthetic production-adapter tests cover calculated and fixed levels,
wrong skills/levels/encoding, duplicate candidates, replacement, named sources,
unchanged134/142 text, native range suppression and buffer-capacity fallback.
Private active-definition operation audit checks source identity across loaded
charged-skill specifications without embedding any mod's tables in the plugin.

Rev.49 durability/physical endpoints were confirmed live by the user. Rev.50 live
validation is pending; no release has been published.

Automated validation: nine suites pass, plus 694 charge source-identity cases,
58,704 operation-isolation checks, 2,094 endpoint samples and 2,258 set scalar
samples. Candidate SHA256: F8719CE9C206CC21F00AACE2E21CD99CAD481AD201F181252E99C42E3C44CDD9. Game PID50972 was running;
installation awaits game closure. Existing settings will be preserved.

The game had closed at the final pre-install check. Rev.50 was installed in
standalone Reimagined after the process and exclusive-file checks passed.
Rev.49 backup retained; all 121 inventoried DLL/config files matched except
the intended DLL. Receipt: outputs/item-roll-ranges-test-rev50-standalone-deployment.json
in the primary workspace. Live confirmation remains pending.
