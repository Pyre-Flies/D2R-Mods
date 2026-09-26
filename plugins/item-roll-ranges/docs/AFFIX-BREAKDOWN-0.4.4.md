# Affix breakdown: 0.4.4

2026-09-24. Same qualified Core 1.3.1-beta / game 3.3.93787 as 0.4.3.
Research was read-only, PID23296, game base0x140000000 and Core base0xC0DE5000000.
Absolute addresses below are session evidence, never runtime constants.

## Findings and limits

The rare club stores affix IDs976 (Savage),1365 (Slayer's),1203 (Victorious),
372 (of Strength). Savage's definition is property29, parameter0, bounds66..80.
The affix row's first32 bytes contain its bounded internal name; this field was
already compared against loaded text by audit_affixes.py. These are internal
English names, not a newly resolved localization API.

The club's Unit was0xD9A83B5A0, ItemData0x372337550, StatListEx0xCC094750.
Unit+0x88 points to StatListEx. Its +0x90 list was0xDAC2FE6E0; the child holds
strength1, Enhanced Damage stats17/18 both75, class-layer4 stat83 value1, and
mana-after-kill stat138 value3. All four affixes share this child list; there is
not a child per affix. The aggregate at StatListEx+0xA8 lacks ED17/18 because
weapon damage has already been applied there. Do not infer missing ED from it.

The Sunder's Unit was0xD9A83B920, ItemData0x372337880, StatListEx0xCC099D50,
child0xDAC2FE800. It holds stats7=10496 (41 life in fixed-point),34=5,37=-45,
80=22,96=6,193=300,358=9. No Prefix/Suffix IDs are present.

## Native recovery landmarks

| Module RVA / structure offset | Evidence |
|---|---|
| D2R+0x2386AB0, slot7 | Property-function dispatch target D2R+0x3D0720 |
| D2R+0x3D076E..0x3D077E | Reads min/max from definition+8/+C, obtains roll through +0x3D5860 |
| D2R+0x3D0946/+0x3D0950 and +0x3D097B/+0x3D098A | Writes ED18/17 through replace/add stat functions |
| D2R+0x3D5230 | Finds/creates property stat list; not an affix-ID-indexed ledger |
| D2R+0x2F3330 | Loader detour FF25 via D2R+0x3E2A240 |
| Core+0x832020 | Detour target, jumps to Core+0x3D9990 |
| Core+0x3D99DE..0x3D99E8 | Key is `(stat << 32) | layer`, no affix ID |
| Core+0x3D9A14..0x3D9A6F | Searches StatList vector+0x30/count+0x38, stride16, value DWORD+8 |
| Core+0x3D9A73..0x3D9A87 | Adds incoming contribution to prior value, sends total to +0x3D95C0 |
| Native original D2R+0x2F3384..0x2F33AF | Older packed key/stride8 similarly adds into one value; DO NOT use this layout under current Core |

Current Core entries are `{uint32 layer, uint32 stat, int32 value, uint32 padding}`.
Reading them as the old 8-byte layout produces nonsense; corrected during this
audit. +0xA8/+0xB0 is the extended aggregate vector/count; child +0x30/+0x38 is
the ordinary vector/count. These offsets remain research-only, not new plugin
dependencies. The observations show no per-affix roll ledger in these paths;
they do not prove that every possible alternate game/provider structure has
been exhaustively searched. We do not replay RNG or mutate items to guess rolls.

Reproduce with `tools/audit_contributions.py <pid> [--adder]`; uses read-only
process access and the previously documented client unit buckets. Optional
--adder inspects the Core detour entry. `audit_property_sources.py <pid> <VA>
<length>` disassembles explicit regions above. Review addresses after patches.

## Rendering behavior

Names replace generic Prefix/Suffix labels for known named affixes. Unique and
Automagic labels remain distinct and have no invented affix names or tiers.

For observed, ungrouped, layer-zero magic/rare scalar lines, functions1 and7
can provide numeric source intervals. All contributors must qualify; the sum
of those intervals must exactly match the native rendered interval. Only one
integer in the actual line is accepted. This deliberately excludes proc/skill
level pairs, elemental min/max pairs, grouped lines, crafted fixed properties,
and unique property groups from numerical decomposition in this release.

Given total T and bounds Li..Hi, each contribution's feasible interval is:
`max(Li, T - sum(other highs)) .. min(Hi, T - sum(other lows))`.
Only when every interval collapses to one number do we replace the combined
line with exact separate lines. A single Savage affix therefore displays its
actual75 and66..80 range with its name/tier. Multiple sources totaling their
combined minimum/maximum, or one variable source plus fixed sources, can also
resolve exactly. Two variable sources allowing50+25 and40+35 cannot.

Default ambiguous presentation retains `[Combined]` total and prints each
source's range/name/tier with `?` and `[Roll unknown]`. These are not sampled or
estimated numbers. Buffer overflow falls back to the original native actual
text, never truncating a property. The ordinary non-Ctrl/RB tooltip is unchanged.

0.4.3 only captured formatter identities during the original-value pass. 0.4.4
also scopes capture during the existing range pass, resolving identity through
that pass while retaining actual values and source metadata from the original
item. Identical observed labels are deduplicated; differing identities remain
ambiguous. A live scan found only two direct Core calls to slot0x704400:
+0x3E80AC and +0x3E80F7, matching existing admitted return addresses. No new
slots, executable patches, or game memory writes were added.

## Validation / remaining work

All five test suites pass. New regressions cover source names/bounds, rejected
layers/functions, exact fixed+variable and endpoint splits, ambiguous totals,
incomplete bound accounting, multiple numeric tokens, capacity fallback, and
capture during the range pass. Tests demonstrate algorithm/adapter behavior;
the real Enhanced Damage display still requires in-game verification.

Sunder numeric ranges and broader source types remain separate unresolved
coverage, not implicitly completed by this affix breakdown. No change to
Controller QOL. 0.4.3 remains the installed version until the user closes the
game and 0.4.4 is copied and hash-verified.
