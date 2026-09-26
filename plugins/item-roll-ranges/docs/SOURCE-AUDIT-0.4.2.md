Historical 0.4.2 audit. The flag8 interpretation and no-clone behavior were corrected in [0.4.3](FOLLOWUP-0.4.3.md).

# Source-label audit: 0.4.2

2026-09-24, D2R 3.3.93787 / qualified Core 1.3.1-beta (same hash as 0.4.1).
No new hook was installed; all work uses the existing scoped formatter observer,
active native tables and original range provider. The audit does not equate
roll percentile with affix tier. T1 remains the highest applicable affix-level
family tier; a fixed +1 can be below a fixed +2 in that family.

## Verified corrections

| Area | Former gap | Current behavior |
|---|---|---|
| Class/element skills | Nonzero selector discarded | Function21 reads Properties.valN at +0x0A+2*N and matches the stat layer |
| Skill tabs | Assumed zero layer | Function10 encodes param as (param/3)*8+param%3; supports 8 classes |
| Single skills / auras | Function22 only | Functions22 and24 use the parameter layer, including oskills and monster-specific properties |
| Triggered / charged skills | Packed layer unhandled | Functions11/19 match skill ID and fixed positive level using the loaded skill bit count/mask |
| Combined display lines | All grouped lines discarded | Read group at ItemStatCost+0x3A; require every group member to have a known contributor, emit each source once |
| Unique properties | Quality7 rejected | Match actual unique record properties, including bounded property-group expansion; label [Unique] |
| Automagic | Stored ID omitted | Read ItemData-0x1C in the native automagic partition and label [Automagic]; no guessed tier |
| Diagnostic quota | First16 repeated frames exhausted it | Skip consecutive identical actual text and retain32 changed samples |

Unknown packed/automatically calculated levels remain unmatched. Group expansion
supports modes0..2, positive-weight entries with a fixed parameter; it is bounded
by depth4 and128 source rows. It does not invoke item mutation or random selection.
A source label means the source definition contributes to that stat. Multiple
sources may contribute to one displayed total. It is not a numerical decomposition.

## Live evidence

Read-only process3632, game base0x140000000, Core base0xC0DE5000000.
Absolute addresses below are session observations only, not runtime constants.
The user's rare axe was unit0x3C0931150, code14, quality6, bank3. Its stored IDs
were976/1365/1203/372: Savage, Slayer's, Victorious, of Strength. Slayer's row1365
has property71 (`bar`), param0, min=max1. The live Properties row has function21,
stat83, val1=4. This proves why matching only layer0 or function22 omitted the tag.

The user's Renewed Black Cleft was unit0x3C09314D0, code691, quality7, bank3,
unique record437. Its six affix IDs and automagic ID were all zero. Record437's
internal name is `Crafted Black Cleft`, matching the user's displayed Renewed
Black Cleft. Its definitions are direct properties274/37 and encoded group IDs
0x10007,0x1000D,0x10013,0x10019,0x1001F. These are NOT indices into Properties.
The high word1 dispatches PropertyGroups, proving this is a separate source
system rather than missing Prefix/Suffix IDs. No tier family is stored there.
Groups contain alternative property rolls with weights and bounds; different
possible properties are not strength tiers of the same affix.

## Address and layout recovery

All following addresses are RVAs unless described as a structure offset.

- D2R+0x3D46FB loads property-function dispatch table D2R+0x2386AB0.
  Functions10/21/22/24 target0x3D1180/0x3CC7C0/0x3CC980/0x3D10A0.
  Dispatcher0x3D4757 loads Properties.valN (WORD +0x0A+2*N), +0x20+2*N is stat,
  +0x18+N is handler. These are distinct from the affix parameter.
- D2R+0x3D1211..0x3D1241 establishes the tab encoding; 0x3CC849..0x3CC863
  passes the property valN as layer;0x3CCA09 and0x3D1135 pass affix parameter.
- Core+0x3DE08A/+0x3DE0AF loads table+0x14D0 skillBits and+0x14D4 mask;
  +0x3DE287..0x3DE28D packs charged skill; +0x3DE5DA..0x3DE5E1 packs proc skill.
  Live values are6/63. Positive level comes from property spec+0x0C; negative/
  automatic levels require additional calculation and remain excluded.
- D2R+0x2D9730 resolves ItemStatCost: table+0x1258 pointer/+0x1260 count,
  stride0x144 (0x2D97AB). Stat ID is WORD+0, display group WORD+0x3A.
  D2R+0x2D65A8..0x2D65DB selects grouped formatter metadata.
  Live group1 contains strength/energy/dexterity/vitality; group2 contains four
  elemental resistances. The plugin reads current groups instead of hardcoding them.
- Core+0x377CC0 is the unique provider branch. Call at+0x377CF6 uses slot
  +0x702260 -> D2R+0x194320. Getter establishes unique table+0x13C8/count+0x13D0,
  stride0x15C. Record ID is ItemData+0x34 (live437); definition properties start
  +0x98 with12 entries of16 bytes {property,param,min,max}. Core excludes flag8
  at record+0x2C. Plugin retains that exclusion.
- D2R+0x3D47BE distinguishes encoded property group high word1 and calls
  +0x3D4DB0. Getter+0x2D9670 reads table+0x258/count+0x260, stride0xC8.
  Each group has mode byte+4 and8 entries from+8, stride24:
  {property,parameterMin,parameterMax,min,max,weight}. The native converter at
  +0x3D1A31..0x3D1A7F copies property/min/max and rolls the parameter bounds.
  Plugin expands only equal parameter bounds, never guessing a randomized selector.
- Core+0x819A1F/+0x819E80 demonstrates automagic ID at ItemData-0x1C.
  Suffix/prefix IDs remain the six DWORDs from ItemData-0x18.

`tools/audit_property_sources.py <pid>` disassembles handlers read-only;
`<pid> <VA> <size>` inspects an explicit range. `--generate` writes the nine
manually reviewed game witnesses into source_profile.h; do not run it merely to
accept a new build. Review semantics first. Bases currently reflect this audit.
The new witnesses join the prior Core and affix admission checks before mutation.

Research-only player/item discovery: D2R+0x9A4BE derives player ID array
+0x2A238F0; +0x9A4F4 derives unit hash tables+0x2A23910. Buckets128, type stride
1024 bytes, Unit+0x158 next link. No runtime dependency was added for this scan.

## Remaining coverage gaps (not represented as invented affixes)

- Intrinsic base stats, sockets/runes/gems and runewords need their own source
  traversal; looking only at a final stat value cannot recover source identity.
- Set definitions/partial bonuses and fixed crafted recipe properties need
  their own verified definition paths. Crafting does not inherently create a
  Prefix/Suffix tier for each property; Renewed Sunders remain unique quality.
- Paired damage formatting may bypass the current single-stat observer. Native
  ranges remain available; no extra unqualified formatter hook was installed.
- Grouped lines with any unidentified member remain unlabeled.
- Property groups with variable parameters, unknown modes, unsupported nested
  definitions or ambiguous contributors are not claimed as resolved.
- Duplicate normalized tooltip identities and insufficient buffer space still
  omit labels rather than attach them to the wrong line or truncate text.

## Validation

Five CTest suites pass, with regressions for class and tab layers, wrong-class
rejection, skill/level packing, invalid masks, automatic-level exclusion,
complete/incomplete groups, unique-source deduplication and cyclic/out-of-range
property groups. Live layouts and source records were read before the build.
User visual verification of0.4.2 is still required; no claim of universal stat
coverage or complete in-game validation is made. Controller QOL stays untouched.
