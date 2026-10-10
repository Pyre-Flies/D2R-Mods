# Defense coverage audit — 2026-10-05

Report: Defense disappears while holding Ctrl, including sashes. The exact
item is unknown. No live reproduction has been captured in this investigation.

## SDK comparison

Item Roll Ranges does not use Item-SDK. It renders original-item native property
text and merges Core's clone-based ranges, then attributes sources using loaded
Properties, affix, automagic and unique rows. Numeric stat/layer identities, not
English Defense names, determine attribution.

The research SDK distinguishes intrinsic and aggregate armorclass (31), recovers
flat socket Defense from independent raw-vector witnesses, and separately
recovers op=13 operands (16/17/18). Its listing projection must not treat total
Defense as +Defense. Those raw-list fixes do not fix this plugin's separate
header path: this plugin delegates actual property values to native formatters.

Current source coverage is not equivalent to the SDK catalog. Set/runeword/socket
source attribution and contextual properties are not a complete independently
verified roll calculator. Native ranges can still be displayed without a source
tag. Unique fallback bounds require one verified additive scalar source and
descfunc19; this does not cover every Defense formatter. Keep native text when
these guards do not establish bounds.

## Source-table evidence

Inspected installed Reimagined `data/global/excel`: ac maps through function1 to
armorclass (31); ac% uses function2 and item_armor_percent (16). These are distinct
from armor.txt minac/maxac. Function2 is correlated, not an ordinary additive
scalar; widening ScalarBounds to treat it as function1 is unsupported.

Sash (lbl) has minac=maxac=2; Light Belt=3/3, Belt=5/5, Heavy Belt=6/6.
Demonhide Sash=29/34 and Spiderweb Sash=55/62. A fixed base value is still useful
information and should remain visible; it need not acquire a redundant [2-2].
Magic/unique/ethereal/Enhanced Defense totals cannot be replaced by these base
table values. `tools/audit_defense_tables.py <excel-directory>` records table
hashes, all armor-related property components and all belt base intervals.
Source evidence is not proof of the selected compiled table or live value.

## New native observations

Disk inspected: D2RCore SHA256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
All addresses below are Core RVAs for this exact image, static disassembly only.

| Site | Observation |
|---|---|
| 0x819ACF | Loads two base armor bounds from the selected row +0xD4 |
| 0x819AE4..0x819AE6 | Compares endpoints; equal/reversed bounds skip range setup |
| 0x819AF4 | Queries flag 0x400000; subsequent SIMD arithmetic applies 1.5x to endpoints |
| 0x819B24 / 0x819B2C | Stores endpoint pair and strict low<high presence byte |
| 0x81A39E / 0x81A3A5 | Absent presence byte skips the Defense header range-replacement stage |
| 0x81A3B2 -> 0x62BB60 | Localization lookup name ItemStats1h (11 bytes) |
| 0x81A471 -> 0x62BC40 | Localization lookup name ItemStats1hRange (16 bytes) |
| 0x81A4FA..0x81A516 | Supplies the two endpoints to localized range template |
| 0x81A666 | Builds replaced tooltip text through Core 0x380520 |
| 0x81A6B7 | Publishes replaced text through a pointer slot; separate from property adapter |
| 0x8328D0 | Export ReadWideItemDefense jumps to 0x3D9350 |
| 0x3D937D / 0x3D93A5 | Defense reader selects stat31/layer0 in wide native vector |

Installed item-modifiers.json defines ItemStats1h as `Defense: %d` and
ItemStats1hRange as `Defense: <light-blue>(%d-%d)<blue>`. Thus the stock header
range template carries bounds without the actual Defense value. The strict
presence check explains why a 2-2 base does not get a header range. It does
**not** yet prove why the reporter's entire fixed Defense header disappeared.
Do not change these branches or infer a new callable ABI from partial analysis.

The existing property merge preserves fixed actual lines and separates Defense
header, +Defense and % Enhanced Defense by native text identity. Regression
fixtures cover those cases, including a ranged block missing its header. They
cannot restore a header which was never supplied to the property formatter.

## Input audit

KeyAdapter calls the original modifier query, then substitutes only bit2 for
the qualified tooltip return address (Core 0x8195E0). Other modifier bits and
other callers retain native results. Ctrl is polled with GetAsyncKeyState.
PadAdapter substitutes the boolean result of the qualified tooltip query
(Core 0x81995A) using the selected XInput user's RIGHT_SHOULDER bit (0x200),
in place of the provider's stock mask 0x800. It calls the original for other
callers. Neither adapter consumes input, clears XInput state nor emits events.
Held() rechecks foreground, eligible panels and current hold before rendering.
Native Ctrl/RB actions may therefore still run.

## Required live check

Compare released/held Ctrl and RB on ordinary Sash, Demonhide Sash and Spiderweb
Sash; then one +Defense item, one Enhanced Defense item and one ethereal item.
Record top Defense line separately from modifier lines. Compare inventory,
stash and vendor. For the same items, disable only this plugin and test Core's
stock Ctrl gesture on a cold restart. This separates Core header behavior from
plugin property merging without modifying saved items or mod tables.

Expected UX: actual Defense remains readable while held; variable ranges are
additional information; fixed Defense retains its ordinary value. Header
restoration remains pending reproduction and a qualified header contract.

Validation: the disposable MSVC Release build passed all five CTest suites,
including ABI artifact checks and the new Defense text regressions. The table
audit completed against installed Reimagined sources. No runtime change or
deployment was made; these checks do not prove the reported visual issue fixed.

## User-supplied live screenshots — 2026-10-05

Existing installed plugin; no audit-build deployment:

- Ordinary Sash shows Defense 2; ethereal magic Sash shows Defense 3 alongside
  its maximum-fire-resistance range. Fixed base Defense is visible in these cases.
- Demonhide Sash shows header Defense (29-34), without its actual value.
- Bloodrune Sash shows header Defense 25 and modifier [15-25] +23 Defense
  [Unique]. This is consistent with base2 + flat23 and confirms flat Defense
  range display on this tested unique sash.
- Ethereal Sturdy Sash of the Four Seasons shows header Defense (3-5) and
  [+21 - +30] +28% Enhanced Defense [P] [Sturdy] [T10]. Enhanced Defense
  modifier attribution/range is present; the actual header value is absent.

These observations narrow the remaining issue to preserving the actual total
alongside a variable header range, including a fixed-base sash with Enhanced
Defense. They do not establish that (3-5) is a correct total interval for the
Sturdy item; native rounding, ethereal handling and modifier contributions
require independent qualification. The subsequent released-Ctrl screenshot of
the same Sturdy sash confirms actual Defense 5, +28% Enhanced Defense and +2%
to All Maximum Resistances. Holding Ctrl replaces the actual header value 5
with (3-5), while preserving the modifier values alongside their ranges.
The desired header presentation for this observed pair is Defense: 5 [3-5],
subject to separately validating the provider's interval arithmetic.

## Revision 14 implementation qualification

Read-only loaded-image audit on 2026-10-05: the game is hosted inside the
D2RLoader process, rather than a separately enumerated D2R.exe process. PID
10980, game base 0x140000000 and Core base 0xC0DE5000000 were observations only;
runtime code uses the loader-supplied executable base and Core module handle.

| Qualified contract | Location |
|---|---|
| Native full-tooltip slot / target | Core+0x704430 -> D2R+0x2BD480 |
| Exact native call / return | Core+0x81A35E / +0x81A360 |
| Native assign slot / target | Core+0x6FDBD8 -> D2R+0x80CF0 |
| Defense publication call / return | Core+0x81A6B7 / +0x81A6BD |
| Armor row lookup slot / target | Core+0x702230 -> D2R+0x314110 |
| Localization lookup | Core+0x387D60; two {char pointer, size} views and bool; char pointer return |
| Full-tooltip call arguments | RCX/RDX/R8 opaque contexts; R9 source item; stack+0x20 int, +0x28/+0x30 byte flags, +0x38/+0x40 opaque pointers |
| Full-tooltip return / assign receiver | Returned owning text object; +0 char pointer, +8 size; native assign owns capacity/reallocation |
| Native assignment arguments | RCX owning object, RDX source bytes, R8 size; returns receiver; copies synchronously |
| Qualified Core frame | Prologue allocates 0x19B8, RBP=caller RSP+0x80; range presence RBP+0x18B0 |
| Armor bounds | Loaded armor row +0xD4/+0xD8, signed integer pair |

Derived instruction bytes are recorded in `src/header_profile.h`, including
the full native assignment function through RET, native argument witnesses,
Core call sites and frame prologue. These join the existing exact Core file
hash and per-site live guards. The initially investigated adjacent slot
0x6FDBE0 is **not** the assignment slot; +0x6FDBD8 is proven by the encoded call
and loaded target. No artifact or instruction at the adjacent slot is modified.

The builder adapter forwards all nine arguments, preserves the return object,
and annotates only the qualified held-tooltip caller. If Core already computed
a variable header interval, its subsequent publication adapter combines native
actual text with that interval. Equal-endpoint annotation requires equal loaded
base bounds, identified source, supported quality, no runeword flag, no socket
capacity in the wide native vector, bounded affix definitions and no Defense
or uncertain/contextual contributor. Incomplete evidence yields `(?)`; it never
infers fixed total Defense from a fixed armor base alone. Unique Defense sources
currently take the conservative unavailable-header path when Core supplies no
header interval; their separately verified modifier ranges remain visible.

Localization uses the provider's own `d2r` resource group and ItemStats1h
lookup contract; no English Defense label is hardcoded. Ambiguous lines and
unsupported templates preserve native text. Header edits use the native owning
string assignment, never item/definition writes or borrowed string-buffer writes.
The plugin replaces two additional aligned pointer slots with compare/exchange,
checks their exact game targets, and restores only slots it still owns.

Expected varying scalar modifier sources trigger the gray unavailable marker
only when neither the native nor verified fallback range is present. A fixed
property or an unqualified source alone does not assert that data is missing.
The footer is emitted once and bounded-output fallback retains the full native
property block. This indicator is conservative; it is not an exhaustive audit
of every possible set, socket, runeword or contextual modifier.

Regression checks include fixed and variable headers, native colors/localized
labels, duplicate-line ambiguity, footer suppression when a range is available,
bounded output and nine-argument native delegation. Visual validation of
revision14 remains pending a cold restart with the candidate.

## Candidate deployment

With the game and loader closed, installed revision14 to the active global
`d2rloader/plugins/Item Roll Ranges.dll`. Built/installed SHA256:
`B43F94349715B354F01198ABD127DA386506158E8031C6DD64449E30CE798730`.
Installed DLL passed the ABI manifest/export/metadata/role artifact check.
Rollback copy: `d2rloader/backups/Item Roll Ranges-before-rev14-20261005-094247.dll.bak`,
SHA256 `104C28C9BF6762321199FC415E148136EBE46573FEBEFB8109321FF38389563D`.
The separate ReimaginedLadder deployment and loader configurations were not
changed. All five final CTest suites and `git diff --check` passed. No visible
revision14 result has yet been claimed.

## Revision 15 correction

User confirmed revision14 preserves actual Defense beside variable ranges but
does not show fixed intervals. Static review found the native lookup group at
Core+0x618EB0 is `d2r\0`, not `eng`. Revision14 had passed the wrong group to
Core+0x387D60 and thus skipped both fixed and unavailable headers. Corrected to
the observed group; a new literal byte witness and adapter lookup regression
cover this boundary. The variable publication adapter does not use this lookup.

Recommended unavailable-header case: Bloodrune Sash. Its variable +Defense
modifier has a known range, but fixed-base total-header interval arithmetic is
not supplied by Core or independently qualified here. Expect actual total
Defense followed by gray `(?)` and the report footer, while the +Defense
modifier retains its known interval. This tests honest missing total-range
coverage without pretending the already-covered +Defense modifier is unknown.

Revision15 installed with the game and loader closed to the same global plugin
path. Built/installed SHA256:
`93B97D3A3BE53A283D4FC6A78F0035FF2325E6643A92BA5B313859AE65776E36`.
Installed version and ABI/export/metadata/role check passed. Revision14 rollback:
`d2rloader/backups/Item Roll Ranges-before-rev15-20261005-095711.dll.bak`,
SHA256 `B43F94349715B354F01198ABD127DA386506158E8031C6DD64449E30CE798730`.
Revision15 visible fixed/unavailable header validation remains pending.

## Revision 16: witnessed additive flat Defense

User confirmed revision15 behavior, including Bloodrune's unavailable header.
Revision16 adds a narrow calculation without linking or depending on Item-SDK.
For fixed-base non-ethereal unique armor, independently read native intrinsic
stat31/layer0 and aggregate stat31/layer0. Native aggregate must equal the unique
actual header value; intrinsic must equal the fixed loaded armor base. Their
difference is the observed flat modifier, which must lie inside the complete
direct unique-source interval. Add the base to both source endpoints.

Bloodrune example: displayed/aggregate25, intrinsic2, observed flat23. Its
loaded direct ac bounds15-25 imply total17-27; preserve actual25. Ivywrap's
direct flat10-20 similarly gives total12-22 on the fixed base2.

Native fields reused from the build-specific wide-stat contract: unit+0x88
StatList pointer; list+0x1C negative flags qualify aggregate; list+0x30/+0x38
intrinsic vector/count and +0xA8/+0xB0 aggregate vector/count. Entries are16 bytes,
with key `(stat << 32) | layer` at+0 and signed DWORD value at+8. Require one
matching entry in each bounded vector; do not sum duplicate vectors/children.
Existing Core WideRead guards witness the16-byte layout and aggregate selector.
Source specs reuse qualified unique record+0x98,12 specs of16 bytes, fields
property+0,param+4,min+8,max+12 and Properties function/stat offsets already
recorded by the source profile. No new entry point or item write is introduced.

This path requires no sockets/runeword/automagic, no variable base, no ethereal
flag and no contextual/correlated source functions. Reject groups, unsupported
functions, ED and per-level/time Defense stats; flat Defense requires function1,
zero parameter and one stat31 component per source. All source contributions
and output arithmetic are bounded. Incomplete evidence keeps `(?)` rather than
deducing a base from a possibly transformed total alone.

Automated fixtures cover base2,total25,flat23,source15-25 -> total17-27,
native/display disagreement, modifier outside bounds, ethereal, ED and property
group refusal. All five suites pass. Visible revision16 validation is pending.

Revision16 installed after confirming the game/loader were closed, to the same
global plugin path. Built/installed SHA256
`310A378EE33252D3513BCC9E82A8320261D206496B99EA2892D6A667FB70A47D`;
installed artifact check passed. Revision15 rollback:
`d2rloader/backups/Item Roll Ranges-before-rev16-20261005-100534.dll.bak`
(previous installed SHA256 `93B97D3A3BE53A283D4FC6A78F0035FF2325E6643A92BA5B313859AE65776E36`).

## Revision 17 label

User confirmed Bloodrune displays the calculated17-27 interval in revision16.
Revision17 labels independently calculated additive intervals `Total:`, e.g.
`Defense: 25 (Total: 17-27)`. Native Core intervals are not newly claimed to be
complete total intervals, so their label is unchanged. This is a text-only
change with no new native address/layout contract. All five automated suites
passed; visible label validation and deployment remain pending.

Revision17 subsequently installed with game/loader closed to the same global
plugin path, with matching built/installed SHA256
`40BD742E2EB324426CC6B6C31777D6560492B1382BD800E0795E26D9A6CB8690`.
Installed artifact check passed. Revision16 rollback:
`d2rloader/backups/Item Roll Ranges-before-rev17-20261005-121448.dll.bak`,
SHA256 `310A378EE33252D3513BCC9E82A8320261D206496B99EA2892D6A667FB70A47D`.
Visible revision17 label confirmation remains pending.

## Documented Enhanced Defense arithmetic

User confirmed revision17 Bloodrune label. The reported non-ethereal magic
Sturdy Sash has actual Defense3, +18% Enhanced Defense, source bounds10-20.
Blizzard's Arreat Summit explicitly documents integer truncation/rounding down
at each stage and the maximum-base-plus-one rule for armor generated with ED:

- https://classic.battle.net/diablo2exp/items/magic/pre.shtml
- https://classic.battle.net/diablo2exp/items/uniquebasics.shtml
- https://classic.battle.net/diablo2exp/items/basics.shtml

For ordinary positive ED on generated armor, base=maxac+1. Non-ethereal final
Defense=floor(base*(100+ED)/100). Ethereal generation first uses
floor((maxac+1)*3/2), then applies ED and truncates again. Do not combine the
stages into one floating-point calculation. These standard rules do not alone
establish upgraded/socket-added/custom-mod context or completeness of sources.

For this Sash, maxac2 implies base3. Both floor(3*110/100) and
floor(3*120/100) equal3. Thus this witnessed10-20% affix permits a fixed final
total interval3-3 even though its percent roll varies. +18% produces3, matching
the screenshot. The earlier ethereal +28% Sash yields floor(3*3/2)=4, then
floor(4*128/100)=5, matching its released screenshot. These are documented-rule
comparisons with visible evidence, not a new native ABI qualification.

Revision17's calculator deliberately lacks percentage support; its `(?)` is a
coverage gap, not evidence that the standard formula is undocumented. Adding
the witnessed ED operands/source bounds with integer arithmetic remains work
to be implemented and validated.

## Revision 18: Enhanced Defense and weapon headers

Revision17's Bloodrune label was confirmed by the user. Revision18 adds integer
arithmetic from independently witnessed source bounds and raw operands. No
Item-SDK runtime dependency is introduced.

Defense requires identified, socket-free normal/magic/rare/direct-unique source
provenance, positive ED for the entire permitted interval, primary stat31 equal
to the generated maxac+1 base (after ethereal truncation), aggregate ED inside
fully accounted source bounds and recomputed current Defense equal to the
rendered header. Combined ED plus flat Defense remains unavailable until its
native operation ordering is independently qualified. Existing Bloodrune flat
Defense remains supported. Upgraded/nonstandard bases fail the intrinsic check.

Weapons apply ethereal floor(base*3/2), then floor(base*(100+onWeaponED)/100),
then additive endpoint damage. Minimum ED is stat18; maximum ED is stat17.
Blizzard's primary documentation explicitly specifies percentage before flat
weapon additions and separate integer truncation:
https://classic.battle.net/diablo2exp/items/basics.shtml (Ethereal, Superior,
and the question about King's plus Slaughter). This is weapon-item damage,
not the character-screen formula. Raw primary and aggregate endpoint operands,
complete source bounds and the native actual damage text must agree. Unsupported
qualities/functions, random groups, runewords, sockets, nonzero stats111/218/219/
272/273 or endpoint intervals that could cross the min/max clamp stay unavailable.

### Newly qualified native path and layout

Target remains D2R3.3.93787 and the pinned Core SHA256
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Read-only loaded-process inspection used D2RLoader PID45780 on 2026-10-05;
manual-map bases were verified MZ at game0x140000000/Core0xC0DE5000000.
The PID and bases are discovery-session observations, not new runtime assumptions.

- Core export FormatItemDamageWithChronicleRanges RVA816900 first invokes the
  original damage formatter through slot7044e8 at816930, return816936. It saves
  the native RAX return at its frame+470. For supported unique/set cases it
  creates a range clone and invokes the same slot again at816a27, return816a2d,
  before localized range replacement. The second call is deliberately not
  captured. Earlier hypothesis that816d12 was an actual formatter call was
  rejected: that address is the epilogue.
- Slot7044e8 targets game2c0fb0. Adapter ABI is four arguments: byte mode inCL,
  item inRDX, 1024-byte output inR8, opaque fourth argument inR9; preserve the
  entire RAX result. Native builder calls the Core damage wrapper via its game
  trampoline at2bdf71, and its surrounding item-type test is45 (0x2d). The
  formatter itself dispatches one-/two-hand and throw families, not English
  label matches. Another builder call was observed at2bfa65 to2c21c0; this is
  a separate formatter and is not a new hook target.
- The existing game314110 getter is a shared base-item row getter, despite the
  earlier Armor name. Bank tables+15a0/+15a8 hold rows/count, row stride1c0.
  Its native address calculation at314162 independently witnesses that stride.
  Loaded weapon rows correlate with installed Reimagined weapons.txt by code
  at row+80. All positive source entries matched unique one-byte field offsets:
  mindam10f (181 entries), maxdam110 (187), 2handmindam113/2handmaxdam114
  (137 each), minmisdam111/maxmisdam112 (30 each). Maxmisdam also coincidentally
  matches a two-byte read because the neighboring byte is zero; the implementation
  uses the one-byte field consistently. These correlations qualify current data,
  not an arbitrary future row layout.
- Discovery-only bank array is game RVA2a9a580, bank stride16. Game300a90 reads
  it after validating bank<4. Runtime continues using the already guarded
  getTables API, not this discovery global. Bank1 table was0xc6693c0,
  shared base rows0x376cedc10/count800 in this session; never retain these
  ephemeral pointers in configuration or later sessions.
- Formatter loads ItemStats1l (one-hand), ItemStats1m (two-hand), and
  strItemStatThrowDamageRange (throw) from the d2r resource group. Game literals
  observed at1cf3388,1cf3378 and1cf3358 respectively. Native formatter one-hand
  stat pair21/22 and secondary23/24 call helper2c2b20 at2c142b/2c1413;
  throw pair159/160 calls it at2c1570. Its ordinary weapon branch clamps maximum
  at least minimum+1 at2c1475-2c1480, while combo branches differ. Intervals that
  could reach the clamp are rejected rather than extrapolated across modes.
- Active ItemStatCost: stat16 op13 targets armorclass31; stat17 op13 targets
  maxdamage22 and secondary_maxdamage24; stat18 op13 targets mindamage21 and
  secondary_mindamage23. Time-based weapon operands are272/273, not274/275
  (the latter are Strength/Dexterity). Native helper2c2c1c and2c2d02 consumes
 272/273. stat111 is item_normaldamage and requires separate semantics.

Instruction-only guards are recorded in src/weapon_profile.h for native damage
formatter entry, Core initial actual caller, and shared row-stride calculation.
Slot target is verified exactly and publication/restoration uses checked ownership.

### Lifetime and publication

The guarded header builder scopes a thread-local capture while invoking the
original builder. Only the initial Core actual damage call for that same item
copies output into an owned string; all four native arguments and RAX pass through.
The item pointer is discarded when the builder call returns. Native actual
localized lines replace Core's numeric range fragments; unique localized header
prefixes handle Core wrapping an entire pair in one color span. Duplicate headers
or unfamiliar localization templates fail open. Actual values remain visible even
when arithmetic is unavailable. No item/stat/table memory is written.

For Defense that Core would subsequently publish, the builder defers its verified
annotation using an owned actual/annotated string pair and receiver identity.
The guarded Defense publication consumes the proof only when the current owning
text exactly equals the saved actual block. It retains no item/table pointers and
never writes borrowed native text. The next scoped builder resets the proof.

### Validation

All five automated suites passed the first rev18 build. Fixtures cover Sturdy
Sash3-3, ethereal Sturdy4-5, staged weapon rounding, independent endpoints,
flat additions after ED, raw/display disagreement, changed intrinsic bases,
random source rejection, socket rejection, duplicate/localized headers and native
adapter argument/return delegation. Artifact suite checks ABI4, exports/resources
and revision18 identity. Final build, deployment and live visible verification
are recorded below when complete. Automated fixtures do not establish the live
contents of a particular spawned item.

Final revision18 build passed all five suites. Read-only live comparison also
matched all seven HeaderProfile witnesses and all three WeaponProfile witnesses
against the running game/Core. This verifies the guarded instruction contract;
it does not confirm tooltip display or the arithmetic for a live spawned item.
The candidate is outputs/item-roll-ranges-defense-audit-build/Item Roll Ranges.dll.
Deployment awaits game/loader closure; revision17 remains installed.

Revision18 installed after game and loader closure was verified to
<game directory>/d2rloader/plugins/Item Roll Ranges.dll.
Built and installed SHA256 match:
CD96B718E36FF3A2332CB5F57D7723BB7C8F6FF501DDC934C80ED5401CF29880.
Installed artifact admission check passed. Revision17 rollback backup:
d2rloader/backups/Item Roll Ranges-before-rev18-20261005-130249.dll.bak,
SHA25640BD742E2EB324426CC6B6C31777D6560492B1382BD800E0795E26D9A6CB8690.
Live revision18 tooltip confirmation remains pending. Suggested checks: the
non-ethereal Sturdy Sash (Total3-3), ethereal Sturdy Sash (Total4-5 for21-30% ED),
Bloodrune regression, and magic/rare weapons with ED and flat min/max modifiers.

## Revision19 correction: consumed ED operands

User live test of revision18: Sturdy of Balance Defense3(?) persisted;
Bloodrune25(Total17-27) worked; rare bow22-67(?) persisted. Read-only inspection
of restarted game/loader PID35604 found exact causes. The revision18 fixture
incorrectly retained percent operands in the aggregate vector and left raw
aggregate damage unscaled. These were implementation assumptions, not live facts;
the fixtures and readers are now corrected.

Sturdy unit0x3ea76b050, native class344, quality4, bank-specific source IDs
988(Sturdy10-20) and322(Balance10 fixed) had primary/aggregate Defense31=3,
but ED16=18 only in child list0xd4234e6f0. Aggregate contains no ED16 entry.
Ethereal Sturdy unit0x3ea76b210, class344, source989(Sturdy21-30) and199(Four
Seasons1-3) has primary31=4 and aggregate31=5, also with consumed child ED.
Bloodrune unit0x3ea76ae90 has primary31=2, aggregate31=25, child31=23.
All pointers here are transient read-only session evidence, not runtime constants.

Loaded Properties5 (ac%) uses function2/stat16, not function1. Function2's
handler table entry game2386ab0+2*8 targets game3cfa60. At3cfad2 it reads the
source specification min/max at+8/+c and calls game3d5860, then passes that roll
to the stat adder at3cfb31. The prior item preparation call at3cfacd is game3d5ac0.
This and intrinsic-base/current-total checks qualify positive scalar ac% bounds.
Loaded Properties79(FHR) uses function8/stat99; Properties42(all max resistance)
uses function1/stat40 then function3/stat42/44/46. Function3 handler entry is
game3cfb60. Known unrelated copy/elemental functions do not affect physical totals;
unknown functions and relevant unsupported transformations remain unavailable.

Active modifier traversal is already proven in the SDK research's
experiments/item-sdk/docs/NATIVE-STAT-CONTEXT-CORRECTION.md and independently
checked against the loaded Core here. Unit+88 is the parent extended list;
parent+90 head, child+68 sibling, child+78 parent. Core3da3da reads the head and
3da3f6 follows+68. Child+0 owner must equal this item, parent must agree, flags
+1c must be nonnegative, expiration+18 must be-1, and state+20 must be0. State
is read at Core3dadbd; +18 is not the state field. The revision19 reader excludes
primary/aggregate copies, socket owners, conditional/timed lists, unreadable or
oversized vectors, duplicates, cycles and more than64 children. Only relevant
unlayered nonnegative scalar operands are summed. Primary and aggregate values
are separate corroborating witnesses, never alternative modifier sources.
New instruction-only guards at Core3da3da and game3cfad2 join weapon_profile.h.
No SDK dependency or additional hook is introduced.

Client-item discovery reused historical SOURCE-AUDIT-0.4.2.md's read-only bucket
contract: game2a23910, type stride1024, 128 buckets, Unit+158 next. It is used
only by the offline audit, not runtime. An initial query of PID5856 raced game
closure and returned partial-read299; no data from it was accepted.

The rare Reflex Bow (class282), unit0x3ea76b590, has Merciless126-150%, Strange,
Trump, Leech, Ease and Winter sources. Primary23/24=9/19, aggregate23/24=22/47,
child17/18=148. Its Trump source includes property145 parameter8, matching
aggregate/child stat218=8 (item_maxdamage_perlevel). The native maximum67 exceeds
aggregate47 because the header formatter applies that contextual operand.
Revision19 intentionally retains(?) rather than claim ordinary ED/flat totals
for a per-level item. An ordinary socket-free magic Merciless Reflex Bow without
Trump should yield min20-22 and max42-47 from these base fields/source bounds.
These bounds are arithmetic examples for the stated definition, not a live-spawn
receipt. Unrelated cold/triggered affixes can require additional source qualification.

Revision19 fixtures now model consumed operands and computed aggregate totals,
function2 Sturdy, function8 Balance, function3 Four Seasons, native flat damage
addition, changed base/display disagreements, cycles and conditional-state rejection.
All five suites pass. All seven HeaderProfile and five WeaponProfile byte guards
match loaded PID35604. Revision19 visible UI validation and installation pending.

Revision19 installed after verified game/loader closure to the same global
plugin path. Built/installed SHA256:
6A55D1282F5F92F4C0727C06C1C2FD9CE952E142CACF0FD48548A7115250B1B5.
Installed artifact check passes. Revision18 backup:
d2rloader/backups/Item Roll Ranges-before-rev19-20261005-131636.dll.bak,
SHA256CD96B718E36FF3A2332CB5F57D7723BB7C8F6FF501DDC934C80ED5401CF29880.
Live revision19 display confirmation remains pending.

## Revision20: intrinsic Base display

The user confirmed revision19 Cruel Reflex Bow displayed current32–69 and
Total30–32 /63–69; Reflex Bow of Armageddon remained unavailable because its
proc source blocked total qualification. The requested UX now preserves the
native current number and displays intrinsic Base instead:32 to69
(Base:9 -19). Bloodrune25 displays Base2 -2.

This reuses the documented guarded base-row getter, primary stat vectors,
original native header capture, and permanent state-zero child ED operand.
No new RVA, offset, signature or ABI was introduced. Weapon table endpoints
must agree with primary endpoints after ethereal integer scaling (base*3/2,
truncated). Armor primary Defense must lie within scaled table min/max, or
match scaled maxac+1 with a verified positive permanent Enhanced Defense
operand. The latter is the game's generated intrinsic armor base before
affix additions/percentage: Sturdy Sash3 -3; ethereal Sturdy4 -4.

Source-specific total reconstruction is removed. Unrelated proc, per-level,
socket or other modifiers cannot reject an otherwise proven weapon base.
Unknown intrinsic bases retain the unavailable header and report footer;
independent unresolved affix ranges can still report. Pending owned header
text overrides later Core range publication only under the existing receiver
and original-text equality checks, preserving fail-open behavior.

Automated validation: five suites, including native current preservation,
fixed/base bounds, enhanced-defense generation, ethereal truncation, mismatched
primary values, child-cycle/state rejection, and unrelated weapon modifiers.
Live revision20 visual checks remain pending.

Deployment: revision20 installed to the global game d2rloader/plugins/Item Roll Ranges.dll after confirming game and loader closed. Previous DLL preserved at d2rloader/backups/Item Roll Ranges-before-rev20-20261005-133417.dll.bak. Built and installed SHA256 agree: 30DEB762F14CAA40CD10608590F640FDFE065A4D8AB768CB2326F2FB631E83D8. Installed artifact ABI/export/metadata check passed. Live visual validation pending.

## Revision21: follow native input decisions

User confirmed revision20 Base header display. Publication is on hold by user
request until revision21 input validation. This change is limited to Item Roll
Ranges. Keyboard/controller slots Core6fe3b0/6fe470 are no longer exchanged,
and physical GetAsyncKeyState/XInput polling is removed. Core resolves native
Ctrl or RT (mask800) with its unchanged original caller81995a, so Controller
QOL's existing exact-caller RT-to-RB remap continues to apply unchanged.

Core's already-qualified TLS index7df224 and block offset1840 expose the range
render context {source,clone,previous}. Revision21 requires distinct, readable
source/clone item units (type4) alongside active/focus/panel gates. The native
no-range path publishes clone=null; no affix/header enhancement occurs there.
SourceFromOverlay still checks the property item matches the range clone.
Both context pointers are already recorded; new witnesses prove their native
publication from the attested Core image (same provider hash):

- Core81a2c4..81a2d3 clears1908, EAX and R12 on the no-range path.
- Core81a2d4..81a317 copies source from frame1910 to frameb60, clone RAX to
  frameb68, saves previous TLS1840 at frameb70 and publishes frameb60 to TLS1840.
- Core81c007..81c012 loads frame18f0 (range clone) into RAX and jumps81a2d4.

HeaderProfile guards these exact bytes in addition to existing frame/caller
witnesses. No native memory is written by the context observer. Existing
formatter slot ownership, original returns/arguments and fail-open contracts
remain intact. Automated context fixtures cover distinct clones, native off,
aliased/null/unreadable pointers and non-item clones. Live Ctrl release/focus,
RT without QOL and RB with active QOL remain to be checked. No QOL files are
changed, and no push/release will happen before the user's validation.

Revision21 local deployment: all five suites pass; installed ABI/export/metadata check passes. Built/installed SHA256: E78B883AA7BFAF61B10034E3141B8F3ED577EFC965D6BAEE86CEBF4E05872917. Previous DLL: <game directory>/d2rloader/backups/Item Roll Ranges-before-rev21-20261005-140000.dll.bak. Game and loader confirmed closed before copying. No QOL DLL/config changes; no publication. Live validation pending.

Publication authorized after discussion of revision21 compatibility limits. Release notes retain the distinction between user-confirmed Base displays and pending dedicated native controller checks.

## Revision22: Defense publication after TLS restoration

Iceblink screenshot showed range-only Defense163-172 while Be eswarm retained
its current/base damage display. Attested Core disk disassembly confirms
Core81a360 stores the builder result to frame1900,81a367 forms TLS1840,
81a36e loads previous context from frameb70 and81a375 restores it. Defense
publication at81a6bd therefore occurs after the range context ends. Rev21's
Held() recheck at that later caller rejected its own saved header proof.
Rev22 uses the earlier builder's owned actual/annotated text and receiver
equality proof, with current active/focus/panel gating, to publish the Base
header. Mismatched proof retains native output when no current range context
exists. No additional native hook or input override. Regression fixtures
cover restored TLS, mismatched original text and disabled publication. Live
Iceblink display remains pending.

User confirmed revision22 fixes the Iceblink Defense header in game on 2026-10-10. Both installed artifacts passed ABI checks; release publication authorized.
