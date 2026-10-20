# Crowded tooltip annotations - revision 55

October18,2026: user reported no property affixes at all on equipped, upgraded/corrupted Tal Rasha's Horadric Crest with four facets, although base/level headers worked. PID35788 read-only capture retained the full visible tooltip and exact item/socket stats. No native calls or memory writes. Private IR47 replay fixture outputs/irr55-crest.bin extends the local replay with four socket identities/vectors and the captured property text. Extracted table/item assets are not committed.

## Reproduced cause

Exact replay through AffixLabels and RangeText::Merge: actual426 bytes, fully annotated1118 bytes,10 source-label identities. At native capacity1024, rev54 returns actual text and zero labels. With16384 the same definitions produce annotations. This is a general all-or-nothing capacity fallback, independent of missing enchantment attribution.

The initial activated-state guard hypothesis was not established. A bounded read-only stack scan found a plausible governed Core+0x81D53B return frame with (state1,state2,flags,extra)=(0,171,64,1); retained frames may be stale, so this alone does not prove a current call. Live disassembly Game+0x2DA173..0x2DA18B independently shows the permanent call sets flags64, state2=171, state1=0 before the governed call. No set-state guard or ABI is changed in this revision.

Native call at0x2DA155 sets capacity0x400 (1024). Existing Core wrapper remains governed by exact provider SHA2562a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2 and existing call/slot guards. New observations consume the existing ABI; there are no new hooks, buffer-size patches or guard relaxations.

## Behavior

First attempt preserves detailed breakdowns. If the full merge exceeds capacity, retry with each expanded property reduced to its final total line, including numeric bounds and Combined/single-source label. If compact totals still exceed capacity, append only verified source labels that fit while reserving space for all remaining native lines. Do not truncate a property, numeric value, source tag or UTF8 color token. If there is no annotation space, return native content.

This improves crowded unique/set/runeword/magic/rare tooltips through their shared merger. It does not add enchantment definitions or reclassify set-state upgrade lists. Those gaps remain documented in EQUIPPED-GAPS-20261018.md.

## Validation

Captured four-facet helmet replay failed before the fix (zero annotations) and passes afterward (10 labeled lines at1024). It verifies every native property line is preserved, Set and Combined totals remain, and child socket rows are omitted under pressure. Synthetic production-merger tests cover roomy detailed output, compact numeric totals and source-label-only fallback; a sweep of every capacity from native-size+1 through256 preserves all native lines and stays below capacity. Existing suites cover duplicate identities, footer behavior, native ranges and formatter append isolation. Live display is pending.

Final automated validation: all10 suites pass, including Controller QOL query composition and loader ABI/version artifact checks. Captured helmet now retains10 labels at1024. Catalog replay passes2094 endpoint samples and2258 permanent-set scalar samples across422 rows. Candidate SHA2569F26AE20B0F23782E780728F38A79778B450C716F6CE44B06536B6E64FA05779. Installation awaits game closure and live display confirmation.
