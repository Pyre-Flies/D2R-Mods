Current follow-up: [0.4.3 Enhanced Damage and no-clone uniques](FOLLOWUP-0.4.3.md).

See [0.4.2 source audit](SOURCE-AUDIT-0.4.2.md) for source-label additions, current limitations and new layout witnesses.

# Current compatibility

Version 0.4.1 targets D2RCore 1.3.1-beta and D2R 3.3.93787. See [the 0.4.1 migration audit](MIGRATION-0.4.1.md) for current RVAs, TLS layout and evidence. The following original investigation is historical; its addresses apply to 0.4.0 only.

# Native range provider and input adapters

Investigated 2026-09-23/24. Addresses below are RVAs, never absolute VAs.
New source root: `F:/SteamLibrary/steamapps/common/Diablo II Resurrected/Documentation/item-roll-ranges`.
Prior note: `E:/d2r-reimagined-mod/d2r-reimagined-mod/plugins/controller-qol/docs/ITEM-ROLL-RANGES.md`.

## Discovery that supersedes the proposed independent calculator

`d2rloader/config/d2rloader.toml` contains `[d2rcore.items] item_stat_ranges = true`
and documents Ctrl/right trigger as the stock hold gesture. This was omitted
from the initial feasibility investigation. D2RCore provides named exported
native range implementations; the SDK contribution API is unnecessary for this
adapter. The plugin deliberately delegates item/stat association, loaded-table
selection, identification checks and formatting to that existing implementation.
It does not establish broader correctness of the loader's calculator.

Inspected D2RCore.dll SHA-256:
`AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0`.
Runtime requires this on-disk hash plus the live instruction witnesses generated
into `src/provider_profile.h`. Hash logic follows QOL's `src/qol_navigation.cpp`.

| D2RCore RVA | Observation |
|---|---|
| 0x792490 | Export `BuildItemTooltipWithStatRanges`; main provider |
| 0x792450 | Export `BuildComparedItemTooltipWithStatRanges`; forwards through a native slot, not separately altered |
| 0x790D50 | Export `DispatchWidgetTooltipWithStatRangeHint`; existing hints remain unmodified |
| 0x796410 | Export `FormatItemPropertiesWithTooltipOverlay` |
| 0x78F860 | Export `FormatItemDamageWithChronicleRanges` |
| 0x5875D8 | ASCII configuration key `item_stat_ranges` |
| 0x79253A | `FF 15 E8 97 EE FF`, zero-argument modifier query through slot 0x67BD28 |
| 0x792540 | Exact return address identifying the range-only modifier caller |
| 0x7925A6 | `and r14d,2; shr r14d,1`, interprets Ctrl bit 2 |
| 0x792544 | Loads current input-mode root through pointer slot 0x67F4D0; +0xDC selects controller |
| 0x7928A9 | Controller query setup: RCX=input, EDX=selected user, R8D=0x800 (stock right-trigger mask) |
| 0x7928B4 | `FF 15 2E 95 EE FF`, controller query through slot 0x67BDE8 |
| 0x7928BA | Exact return address identifying range-only controller caller |
| 0x680330 | Existing QOL TestUiMode function-pointer slot |
| 0x323351 | `mov ecx,0x19; call [0x680330]`, Cube witness |
| 0x32336A | `mov ecx,0x18; call [0x680330]`, stash witness |

The selection range `[0x7924E7,0x79262C)` and controller range
`[0x7928A9,0x7928CA)` are exact runtime byte witnesses. They include the feature
enable mask, keyboard/controller selection, and native identified-item check.
The keyboard mask and controller boolean are replaced only when the immediate
return address is exactly the one above. Other consumers of the shared slots
delegate unchanged, preserving parameters and full return register values.

## Publication / lifetime

The input adapters do not change global key state, item stats or item-definition
files. Version 0.2.0 additionally installs the SDK executable hook documented below. Two aligned D2RCore function-pointer data slots are exchanged using
InterlockedCompareExchangePointer after preflight. Originals are captured
before publication. Both must publish before `active=true`. A failure deactivates
and restores only slots still owned by this plugin. A conflicting pointer is
never overwritten. Unload deactivates before ownership-checked restoration.

The adapter DLL is pinned before either slot is published. A concurrent caller
that already fetched an adapter therefore cannot jump into an unmapped DLL.
Inactive adapters delegate to the originals and retain their input module
reference through process exit. No callback retains or uses the SDK context.
Cold restart is the recovery path; do not hot-reload a second copy.

ALT uses the high bit of GetAsyncKeyState(VK_MENU). Controller uses the selected
user index supplied by D2RCore, XInputGetState, and RIGHT_SHOULDER=0x200. It does
not aggregate inactive pads. Disconnect, focus loss and no eligible panel all
return false. TestUiMode IDs inherited from QOL: inventory=1, vendor=0x0B,
stash=0x18, Cube=0x19. Native controller backend behavior is still a runtime gate.

The adapters depend on private provider slots; this is not a public SDK service.
The loader does not own their restoration. Future providers must be re-audited;
do not merely change an expected hash or accept their current bytes as a new
fingerprint. Verify the target functions' runtime ABI and native selected-user
mapping when the game becomes available.

## Additional formatter research preserved for future work

Read-only source artifact:
`<private-workspace>/work/runtime-game.exe`.
This is an earlier captured code image, not the current running process.

* D2R+0x2D6520: single-value formatter candidate. RCX is a unit/context used
  for table-bank selection (NOT proven to be the hovered item), RDX is an
  ItemStatCost row, R8D is value, R9D parameter, stack arguments are grouped flag,
  char output buffer, and mode. Native string-copy calls use capacity 0x100.
* D2R+0x2DABDC calls 0x2D6520 from the property loop. Another caller is 0x2D98BA.
* D2R+0x2D7850: paired-value formatter candidate, called at 0x2D64E7. Additional
  argument shifts its grouped flag/output/mode stack positions. Do not conflate
  its ABI with the single-value formatter.
* D2R+0x2DA760 property-loop entry is already redirected in the prior capture;
  it is not an unowned hook surface.
* D2R+0x34A0E0 reads the bank byte at Unit+0x1BD; confirmed instruction at
  0x34A103. D2R+0x2D9730 uses ItemStatCost stride 0x144, witnessed at 0x2D97AB.

These candidates were unused in 0.1.1; 0.2.0 uses the verified subset below. The
earlier apparent item argument was corrected after caller analysis. Private
disassemblies are retained locally, excluded from distributable packages.

## Historical 0.2.0 inline formatter (stack assumption superseded by 0.2.2)

User confirmed 0.1.1 displays the same native information as Ctrl/RT. To retain
both bounds and the realized value, 0.2.0 installs one SDK-managed inline hook
at D2R+0x2D6330, after all provider and native witness checks. Its eight-argument
ABI is `int(contextUnit, statList, statId, layer, minimum, maximum, char* output,
mode)` using the Windows x64 convention. The second argument is a stat list,
not an item pointer. The entry guard covers 14 bytes. The SDK owns the hook's
restoration; the plugin owns restoration of the two Core data slots.

Only return RVA 0x2DAADC, mode 4, active plugin and eligible foreground panels
use the new formatter. Other calls delegate to the original trampoline.
The caller loads the realized raw stat at 0x2DA940 from `[rbp+rbx*8+0x144]`
and stores it at caller RSP+0x44 at 0x2DA955. Call 0x2DAAD7 returns at 0x2DAADC.
Consequently `_AddressOfReturnAddress()+0x4C` addresses the realized raw value
at detour entry. This is checked with a compiled MASM caller/JMP-detour fixture.
The value is not reconstructed from bounds or parsed out of rendered text.

| D2R RVA / offset | Verified use |
|---|---|
| 0x34A0E0; witness 0x34A103 | Bank getter; context byte +0x1BD |
| 0x2D9730; witness 0x2D97AB | ItemStatCost row getter `(byte bank, int stat)`; stride 0x144 |
| 0x2DB700 | Normalize `(context, stat, raw, row, OptionalValue*, bool boundsMode)` |
| 0x2DAD40 | Group `(context, statList, stat, normalized, row, int* emit)` |
| 0x2D6520 | Single `(context, row, actual, layer, bool grouped, char* output, mode)` |
| 0x2DABDC | Native single-format call; setup 0x2DAB2D..0x2DABE6 |
| row +0x32 | Description function selects supported numeric bounds transform |
| 0x2D6852 | Description functions 20/21 negate values |
| 0x2D691B | Functions 5/10 scale by 100/128 |
| 0x2D732E | Function 29 takes absolute value |
| 0x2D7334 | Function 19 formats the normalized integer in localized text |
| 0x2D7794..0x2D7810 | Description-function dispatch table |

OptionalValue is eight bytes: int value, bool present, three padding bytes.
Actual normalization uses empty optional and boundsMode=false; bounds use empty
optionals and true, preserving the native distinction for fixed-point shifts.
Group suppression is retained. The native single formatter supplies the actual
localized line in a 256-byte buffer. Supported ungrouped numeric descfuncs are
1-10 except 11 (specifically 1,2,3,4,5,6,7,8,9,10), 12,19,20,21,29.
Grouped and unsupported lines retain actual text without a speculative prefix.
Equal bounds omit the prefix. Prefix overflow preserves the complete actual line.

`tools/audit_formatter.py` records witnesses in `formatter-audit.json` and
`src/formatter_profile.h`. Thirteen exact byte regions guard the range entry/body,
actual-value load, range and single callers, normalization, grouping, single entry,
bank/row access, scalar branches, template and dispatch. Runtime mismatch logs
its RVA and prevents activation. The earlier captured executable is research
input only; runtime bytes must match independently. Captures/disassemblies and
research Python dependencies are excluded from the source package. Future game
builds require renewed ABI/caller analysis, not merely updated signatures.

Recovery: close the game and restore the 0.1.1 DLL from the prior experimental
package, or remove this plugin and restart for stock behavior. Hot reload is
unsupported. 0.2.0's live formatting and full controller/panel matrix are pending.

## 0.2.1 property mode correction

The user screenshot shows 0.2.0 still displaying native range-only text. The
plugin log confirms successful 0.2.0 installation at 2026-09-23 21:22:47.689.
This disproves the prior assumption that passing formatter unit tests established
that the live item path entered the new branch.

Newly traced D2R+0x36A630 reads signed Properties row+0x2E, validates 0..7 and
returns it at 0x36A6C3; invalid property/type falls back to 4 at 0x36A6DC.
The property-loop call at 0x2DA9D9 stores its result in R15D at 0x2DA9DE,
which becomes the range helper's eighth argument at 0x2DAABE. Thus the initial
R15D=4 at 0x2DA959 is only a default, not an invariant. Installed properties.txt
has a blank uiRangeType for ac (armorclass). Mode 0 is the ordinary table default;
the screenshot does not itself capture the live mode value. This is the likely
cause of delegation and is now covered by regression tests and bounded logging.

0.2.1 accepts modes 0 and 4 for the verified caller, preserving the other gates.
Mode 1 has special level-bound optionals; 2/3 override description functions;
5+ encode other property behaviors and remain delegated. Native Single only
overrides descfunc for modes 2/3 at 0x2D6582..0x2D65A8; modes 0/4 use the row.
Fifteen native witness regions now include PropertyModeCaller and PropertyMode.
No addresses from these investigations are claimed portable across game builds.

A diagnostic path is derived from the loaded plugin module's directory, pointing
to its sibling logs/item-roll-ranges-format.log. At most 32 helper calls per
process are appended. It records caller/expected caller, stat ID, mode, bounds,
raw actual and output. Delegation result -2 does not claim a raw actual value
(the diagnostic zero is a placeholder). No SDK context pointer is retained.
Runtime validation must confirm actualRaw for the tested Defense line and its
rendered prefix; the correction is not claimed gameplay-confirmed.

## 0.2.2: live caller evidence and stat-list extraction

Live 0.2.1 log: stat=31, mode=0, min=80, max=120, caller=00000C0DE53F1EB2,
expected=00000001402DAADC, result=-2, repeated 32 times. The generated caller
address is process-specific, NOT a reusable RVA. The hook was called; its exact
return-address gate rejected the call. actualRaw=0 in these records is only a
placeholder because no stat read occurred. This proves the previous direct-JMP
stack fixture did not model the live loader path. It is retained as historical
research but removed from CTest. The generated wrapper implementation has not
been decoded, and no wrapper stack layout is assumed.

0.2.2 never reads caller locals or accepts arbitrary stack offsets. Instead it
reads the exact list supplied as argument 2, using the same array selection as
the game's enumeration used to populate the original caller's local stat array:

| Native RVA / layout | Evidence |
|---|---|
| 0x2DA90C..0x2DA929 | List/stat/output/capacity argument setup; call 0x2F65C0 |
| 0x2F65C0 | Tail jump to 0x2FA060 |
| 0x2FA083..0x2FA0A3 | Signed flags at list+0x1C select vector header +0x30 or +0xA8 when negative |
| vector+0 / +8 | Entry pointer and 64-bit entry count |
| 0x2FA180..0x2FA1B2 | Eight-byte entries: low word layer, high word stat ID, signed value at +4 |

The 0x2FA060 entry is already hooked in the captured image; this plugin neither
replaces nor calls that hook. The read-only layout is verified against exact
body witnesses instead. Eighteen formatter witness regions now include the
stat-enumeration caller, array selection, and entry layout.

ReadActual validates readable memory, stat/layer in 0..65535, count 1..65536,
and exactly one matching key. Invalid, absent or duplicate keys delegate to
native output. No borrowed pointers are retained after the call. The extracted
raw value enters the existing normalization and native actual-line formatter.
Tests cover both vector layouts, layer isolation, duplicate/missing data and
an additional calling wrapper with no original-return-address requirement.

Scope now requires a thread-local request from the existing scoped Core input
query, a fresh ALT or selected-pad R1 hold check, an eligible foreground panel,
and scalar mode 0/4. Keyboard queries select keyboard input; controller queries
replace that selection with the supplied pad index. No inactive controller is
polled. Unrelated Core slot consumers do not update the request. On deactivation
callbacks delegate without reading item memory. Caller addresses remain only
in the bounded diagnostic log, version-tagged v0.2.2.

Runtime confirmation of the actual value and rendered output remains pending.
The startup message describes installed capability, not observed gameplay success.

## 0.2.3: Core wide stats and verified live caller (supersedes 0.2.2 layout)

0.2.2 logs reached mode 0, stat 31, bounds 80..120, result=-3: actual lookup
failed. On the user's requested running session, read-only inspection of PID
62084 found D2RCore base 13253816090624 (0xC0DE5000000). The observed caller
0xC0DE53F1EB2 is Core+0x3F1EB2, NOT generated wrapper code. Earlier descriptions
of it as generated were incorrect. Its on-disk and live instructions agree.
Only module-relative addresses below are reusable within the attested build.

Core replaces the property loop and original stat enumeration with wide-stat
implementations. Do not use the captured game's eight-byte entry layout here.
The vector header offsets remain +0x30/+0xA8 selected by signed list+0x1C,
but each entry is 16 bytes: uint32 layer at +0, uint32 stat ID at +4, signed
32-bit actual value at +8. The trailing four bytes are not used by this reader.
The lookup key is `(uint64(stat)<<32)|uint32(layer)`, not a pair of 16-bit IDs.
Stat IDs >=0x8000 are rejected consistently with the Core exports.

| D2RCore RVA | Verified evidence |
|---|---|
| 0x7AA530 | Export CopyWideListStat; same list/stat/output/capacity ABI |
| 0x7AA567..0x7AA57A | +30/+A8 selection from signed flags at +1C |
| 0x7AA589 | Stat key shifted left 32 |
| 0x7AA5AF | Index shifted left 4, i.e. 16-byte entries |
| 0x7AA5F2..0x7AA607 | Copy 16 bytes, advance pointers by 16 |
| 0x3E2B21..0x3E2BA4 | Effective list lookup key, header selection, stride and int32 value +8 |
| 0x7AA318 | Export ReadWideStatEntry reads int32 value at entry+8 |
| 0x3F1C82..0x3F1C9D | Replacement loop reads 64-bit key, shifts stat by32, loads actual at+8 |
| 0x3F1E7C..0x3F1EB9 | Replacement loop assembles range helper arguments |
| 0x3F1EAC -> 0x3F1EB2 | Indirect helper call and return; slot Core+0x681D20 |

0.2.3 reads the wide list by exact stat/layer key, preserving bounds and native
normalization. It restores a narrow caller check against the VERIFIED Core
return RVA 0x3F1EB2, plus existing held-input/panel/mode gates. It never reads
caller-local stack slots. Four new Core witness regions guard WideCopy,
WideRead, WideRangeCaller and WideActual in addition to the exact file hash.
Tests now represent 16-byte entries and include a layer above 65535 to catch
16-bit aliasing. Native-game stat layout witnesses are historical research;
Core's live replacement layout governs current extraction.

Read-only inspection utility: tools/live_code.py PID ADDRESS SIZE, maximum
65536 bytes per call; opens PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,
never writes or pauses the process. Raw disassembly stays local. The live caller
code inspection confirms layout, not the rendered output or the actual item's
value. The next gameplay check must show a v0.2.3 success trace and both values.

## 0.3.0: original-item property pass and native colored ranges

User confirmed the prefix appeared in 0.2.3, then confirmed actual +118 became
+80 while held. The 0.2.3 trace explicitly shows actualRaw=80: the temporary
range item was being read. The wide-stat layout was correct, but the source was
wrong. All former direct stat-list extraction and scalar hook behavior is now
retired. Historical source/tests remain for research; they are not active hooks.

Core's FormatItemPropertiesWithTooltipOverlay at 0x796410 maps original item to
clone immediately before calling its underlying native formatter. The mapping
is available in Core TLS. It is stack-scoped by BuildItemTooltipWithStatRanges.

| D2RCore RVA / offset | Contract |
|---|---|
| 0x691910 | DWORD TLS index, loaded at 0x796445 |
| GS:[0x58], indexed by Core TLS index | TLS block pointer |
| TLS block +0x490 | Current overlay frame pointer |
| overlay +0 / +8 / +16 | Original unit, clone unit, previous overlay |
| 0x793234..0x7932D8 | Frame construction/publication and restoration around tooltip build |
| 0x796410..0x7964A9 | Identity check, source-to-clone substitution, argument forwarding |
| 0x681DB0 | Underlying property formatter data slot (corrected arithmetic; NOT 0x67DDB0) |
| 0x796498 -> 0x79649B | Underlying formatter call and exact governed return address |

The plugin atomically wraps 0x681DB0, in addition to its two input slots.
It requires exact Core hash and all new live wrapper/overlay byte witnesses.
This version installs no executable inline hook. All three pointer slots use
ownership-checked restoration; the plugin remains pinned for late callbacks.

Property ABI: uint64 Windows-x64 return (preserved opaquely), arguments
`(item, char* buffer, int capacity, int mode, int state1, int state2, int flags,
int extra, const void* propertyDefinitions, void* optionalCount)`.
Native D2R+0x2DC4B0 prologue and calls at 0x2DA150..0x2DA18B substantiate the
10-argument layout. At 0x2DC559..0x2DC5B9 it finds selected stat lists, allocates
a transient aggregate, and merges properties; this is why reading one item stat
list was not equivalent to rendering the original modifier text.

Scoped wrapper execution:
1. Require return Core+0x79649B, held input, eligible foreground panel, no nested
   render, bounded writable output capacity 2..65536, and live overlay whose
   clone pointer equals the supplied item.
2. Resolve the original source item from overlay+0. Readability/identity gates
   reject missing or unrelated overlays.
3. Copy initial output into two independent buffers of the same capacity.
4. Call the original property formatter on the source item, forwarding all
   mode/state/flag arguments, with definitions=null and a zeroed 16-byte optional
   count (uint64 value, bool present, padding). This yields actual native text.
5. Call the original property formatter on the clone with the incoming native
   range arguments. Preserve its opaque return value.
6. Add range prefixes to unique matches while retaining actual native text.
   No item memory is written. Buffers and source references are call-local.

Native localization keys in installed item-modifiers.json:
ChronicleModifierRange (27979): `ÿcU(%s-%s)ÿc3`.
ChronicleModifierRangeNoParen (28084): `ÿcU%s-%sÿc3`.
`U` is the requested light blue; `3` restores the property blue. Both single-byte
FF and UTF-8 C3 BF color markers are supported. Existing actual-line color tags
remain untouched and take effect after the prefix.

Matching compares localized text with numeric fields normalized to placeholders
and native U-colored range spans treated as one numeric field. Matching must be
unique in BOTH blocks. This avoids assigning bounds between repeated labels or
changed grouping. Only native U-colored spans contribute range text. Simple
integer pairs get signed `[+X - +Y]` formatting; multiple fields use semicolons.
Complex/decimal spans retain their native contents. Fixed lines have no prefix.
Ambiguity, changed grouping or no native range leaves the original actual line.
Insufficient capacity preserves the entire original property block.

This broadens property coverage without claiming every possible tooltip field
has been verified. Base header fields and unrelated tooltip sections outside this
property formatter have not been separately replaced. Broad item/localization
and runtime performance checks remain pending. Header ranges must not be
mistaken for a +Defense modifier's roll range.

## 0.3.1 diagnostic / 0.3.2 runtime color fix

The user observed only the header Defense range in 0.3.0. Its log confirmed the
property original-item pass was correct (+118 Defense and +30% Enhanced Weapon
Damage), but annotated=0, unmatched=2. The 0.3.1 diagnostic build logged the
previously missing RANGED buffer. It contains precisely these two native spans:
`+<EE 81 BE>U(80-120)<EE 81 BE>3 Defense`
`+<EE 81 BE>U(25-50)<EE 81 BE>3% Enhanced Weapon Damage`.
Fixed lines remain Slows Target by15%, +25% Faster Hit Recovery and +1 Druid skills.

EE 81 BE is UTF-8 U+E07E. The live localization pipeline replaces the source
`ÿc` introducer with that private-use character, directly followed by the color
selector. It is not the UTF-8 encoding of `ÿc` (C3 BF 63), nor raw FF 63. Prior
0.3.0 test fixtures only modeled the source encodings, causing false confidence.

0.3.2 recognizes all three encodings. Analyze retains the exact introducer from
the native range span, so emitted prefixes use EE 81 BE U and reset EE 81 BE 3
for this build. No new RVA or hook is needed. The entire captured actual/range
blocks are included verbatim (escaped bytes) in range_text_tests.cpp; expected
output has two annotations, zero unmatched lines, +118 Defense and +30% ED.
This test supplements synthetic cases instead of substituting for runtime proof.
The runtime trace retains ACTUAL/RANGED/RESULT for subsequent verification.

0.3.2 runtime follow-up: the user confirmed both prefixes look correct. Live
trace annotated=2/unmatched=0 preserves +118 Defense and +30% ED, with native
U+E07E U/3 selectors. This validates the two-pass source/range pairing on Aldur.

## 0.3.3 keyboard gesture change

At the user's request, both the scoped input-query adapter and the fresh hold
check now use GetAsyncKeyState(VK_CONTROL) instead of VK_MENU. Either Ctrl key
activates the existing range path. The keyboard query still gates Core's Ctrl
bit to eligible foreground panels and preserves unrelated modifier bits/callers.
Controller remains selected-user XInput RIGHT_SHOULDER (R1/RB). No new addresses,
ABIs or hook surfaces. Prior ALT references describe historical releases.
