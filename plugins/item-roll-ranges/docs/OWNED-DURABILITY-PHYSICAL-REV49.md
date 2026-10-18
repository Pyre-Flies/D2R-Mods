# Owned durability and physical endpoint rendering - rev.49

Evidence: the existing 2026-10-17 active Reimagined capture, with socket-profile
witness admission and reciprocal owned stat lists; see LIVE-GAP-BASELINE-20261017.md.
Private rows/snapshots remain under <workspace>/outputs. No new native entrypoint,
RVA, patch, layout, or ABI is introduced.

The user confirmed both durability lines have [U] but no numeric prefix, and
Gemini Coat's Adds 5-10 Weapon Damage likewise lacks a prefix. Production replay
of the captured definitions and complete primary/owned/aggregate stats reproduced
both omissions before this change.

Runtime unique row894 Gemini Coat has property52 function13 -> stat75, bounds
25..25, and property140 functions15/16 -> stats21/22, operands5..10. Runtime row900
Xanadu Dreams has property52 bounds100..100. These runtime row identities are
capture evidence, never runtime name/id exceptions. Stat75 has shift0, op13,
descfunc19; stats21..24 have shift0, op0, descfunc19 in this capture.

The linked adapter resolves these displayed modifiers from owned source groups.
It does not use aggregate maximum durability or base-inclusive weapon damage as
a percentage or modifier roll. Numeric rendering still requires exact agreement
between owned totals and the visible line, one matching line, qualified metadata,
complete source bounds, and supported nonreplacement endpoint/percent operations.
Function13 is admitted only for its explicit stat75 target without replacement.
Other operation13 stats are not admitted globally. Existing corruption and
recipe-extra exclusions remain. Paired function5/6 reconstruction is not added.

Expected captured replay: [25%] or [100%] on durability, and [5 / 10] on the fixed
physical endpoints. These are fixed bounds, not a claim that damage rolls between
5 and10. Synthetic adapter regressions cover base-inclusive aggregate values,
wrong displayed values, changed metadata operations, replacement, and unsupported
endpoint operations. Catalog endpoint coverage includes physical stats21/23 as
well as elemental families. Live rev.49 confirmation is pending.

Derived charged-skill function19 negative operands are still unresolved. Remaining
charges are mutable usage state and must not be presented as a roll interval.

## Validation and test installation

All nine suites pass with MSVC19.44 /W4 /WX. The private captured-item replays
pass for both durability armors and Gemini physical damage, plus the prior cold,
fire and set regressions. The expanded catalog passes 2,094 physical/elemental
endpoint samples and 2,258 permanent-set scalar samples. Operation isolation:
58,704 checks, 1,866 ED checks, zero failures; all six durability samples now
produce model bounds. The existing 219-runeword selection and 2,026-affix model
audits also pass. These checks do not claim complete live rendering coverage.

Rev.49 was installed only into standalone Reimagined after no game/loader process
was found and an exclusive target-file open succeeded. Rev.48 was backed up;
121 inventoried DLL/config files were compared, with only the target DLL changed.
Installed SHA256: 4F6B208E2486C0A1D2D2415D0DB230C6BFEF9FADBAFAFE46A4BAC95E523BF30C.
Receipt: <workspace>/outputs/item-roll-ranges-test-rev49-standalone-deployment.json.
Not published; user-visible rev.49 confirmation is still pending.
