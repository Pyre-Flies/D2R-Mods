# 0.4.0 affix labels: implementation and live evidence

Implemented; the user confirmed rendered labels on magic and rare items. The two-pass actual
value/range renderer and Ctrl/R1 panel gating remain in place.

Read-only inspection of process 57996 observed game base 0x140000000 and Core
base 0xC0DE5000000. These process addresses are not runtime constants. The game
exited before deployment. No external memory writes occurred.

| Relative location | Verified evidence |
| --- | --- |
| Core+0x67F790 -> game+0x300A90 | Table accessor `(uint8 bank) -> table*`, bank <4 |
| game+0x300AC0 | RIP-relative table globals at game+0x2A9A580, 16-byte entries (research only) |
| Core+0x67D2E8 -> game+0x36CD50 | Tail +0x36CDC2 reads DWORD itemData-0x18+index*4 |
| Core+0x67D300 -> game+0x36CDE0 | Tail +0x36CE52 reads DWORD itemData-0x0C+index*4 |
| game+0x34A52B | ItemData getter reads unit+0x10 |
| Core+0x67CCB8 -> game+0x3D4220 | Read-only affix eligibility predicate, no item-level cutoff |
| game+0x3D42BD..0x3D430F | Five excluded types at row+0x78; seven included types at +0x6A |
| game+0x3D3210 / +0x3D329D | Properties at table+0x240/count+0x248, stride 0x30 |
| Core+0x3F20E4 -> +0x3F20EA | Single-property formatter via slot Core+0x681D08 |
| game+0x2D6520 | ABI `bool(unit,row,int value,int layer,bool grouped,char* output,int mode)` |

The six affix IDs are read without modifying ItemData. Classification uses the
loaded table partitions: suffix, prefix, automagic. Observed partition row
indices: 0, 823, 1940; total 2011. Automagic is excluded. Quality 4/6/8
(magic/rare/crafted) is required. Sets and uniques keep range/value formatting.

`tools/audit_affixes.py` compared all 1940 loaded prefix/suffix rows against
the active mod text: names, version, spawnable, level, group, maxlevel, rare,
levelreq, frequency and three min/max pairs passed. Also verified property
function bytes +0x18..+0x1E and stat WORDs +0x20..+0x2C against Properties and
ItemStatCost (434 loaded property rows). Native guards were generated into
affix_profile.h. The runtime does not require loose text files.

Tier families require equal group, class restriction and all three property
IDs/parameters. T1 is the highest distinct affix level in the applicable family.
Count higher levels after spawnable, frequency, item version, rare eligibility
and native type filtering. Current ilvl/maxlevel do not renumber existing
rolls. Duplicate rows at one level share a tier. This is a progression tier,
not a roll percentile; custom mods can define non-monotonic progression.

The new single-formatter observer records native stat ID/layer and normalized
line identity only during the original-item pass at the qualified return
address. All seven arguments, output and return value pass through unchanged.
Labels append after the original value in native light-blue U color, reset 3.
Multiple known affix contributors get separate labels. Duplicate identities
and insufficient capacity fall back without changing actual values.

Physical property functions 5/6/7 use implicit damage IDs 21/23,22/24,17/18.
Skill function 22 uses its parameter as layer. Grouped lines and unknown packed
layers omit provenance. Socket, crafted-fixed and automagic sources do not get
invented affix labels; labels identify known rolled-affix contributions.

No native item pointer or borrowed table row survives the formatting call.
The observer slot uses pin-before-publication, atomic exchange and ownership-
checked rollback/unload restoration. Additional Core and native code witnesses
are checked before installation. Five automated suites pass, covering native
metadata layout, invalid partitions, quality/group exclusions, tiers, layer
matching, original values, colors, ambiguity and capacity. The user confirmed
that magic and rare items display correctly. Crafted items and every possible
stat combination have not all been visually validated. The requested set/unique
label extension was subsequently declined; 0.4.0 remains unchanged.

Deployed after both game/loader processes exited; build and installed SHA-256:
`4C28103F8DFEDF8E25559B86FF0F4C4ECA57ED8C8806DBD3819D74D4AA6266C2`.
Previous DLL: `backups/Item-Roll-Ranges-before-0.4.0.dll`.
Runtime/source ZIPs generated with verified entries, CRCs and SHA256SUMS.
