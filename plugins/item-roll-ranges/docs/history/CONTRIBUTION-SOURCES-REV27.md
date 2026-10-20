# Owned contribution sources — revision27 local candidate

Recorded 2026-10-12 for D2R3.3.93787 and the inspected D2RLoader1.3.1-beta.
Qualified D2RCore SHA256:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Every offset/RVA below is build-specific. Evidence is read-only loaded code,
compiled tables and the controlled items in
[the validation timeline](../ITEM-CONTRIBUTION-VALIDATION.md). Private code/table
dumps and receipts remain under local `outputs/`, outside source distribution.

## Source and presentation contract

Capture host permanent modifiers and independently owned attached socket
aggregates into owned vectors and identities. Reconcile each source against
active affix definitions before replacing a partial native range. Keep native
actual text and Base headers. The complete combined range appears above gray
source rows, with original affix name/tier and `[Socket n]` in native insertion
order. A socket-only property has one labeled row, without a duplicate total.
Unknown individual allocations within a proven combined host interval stay
`?`; interval arithmetic may establish exact values but never chooses an
arbitrary solution. A failed capture or transform leaves previous rendering.
Buffer overflow retains the entire original property block atomically.
The revision26 fixed-IAS production prototype is retired: a failed shared
source proof cannot fall back to treating a socket's captured value as a fixed
roll definition. Its isolated capture/formatter fixtures remain research tests.

Supported scalar operations are property function1 named stat, function8 IAS93,
function7 ED17/18, and standalone functions5/6 physical min/max additions.
Functions5 and6 together are excluded from scalar interpretation. Paired
elemental48–55 uses independently named function1 endpoints or functions15/16
fixed endpoints. Poison rate/duration and layered/grouped descriptions decline.
Scalar display metadata must have matching stat ID and descfunc19. Life/Mana
raw values require exact division by the active `ValShift`; fractional values
decline. Host/socket sums must match the composed stat for ordinary scalars.
ED operands16/17/18, physical21–24 and Defense31 use independently captured
leaves: the composed value may omit consumed percentage operands or include
intrinsic/computed damage or Defense. The native modifier text must still
equal the reconstructed modifier total. ED17 and18 must agree within each
source. The header's intrinsic Base interpretation is unchanged.

Unique socket sources reuse the existing unique table/group reader and `[U]`.
Only the existing `uniqueScalar` deterministic selection proof permits numeric
source bounds; uncertain weighted choices cannot become an additive split.
Runeword flag`0x04000000` or any own enchantment counter392–395 disables this
candidate's additional attribution. Enchantment sampling proved original and
added Life merge in the same permanent list. That observation does not prove
how to recover every recipe; no `[E]` guess is made. Set bonus attribution and
general mixed/inherent source recovery remain outside this candidate.

## Unit/stat-list layout and limits

Unit: type DWORD+0 must4, compiled class ID DWORD+4, runtime ID DWORD+8,
ItemData pointer+10, stat root+88, socket control+a0, bank byte+1bd in1–3.
ItemData: quality DWORD+0 (1–8), flags DWORD+18, quality record signed DWORD+34,
version WORD+40; auto ID DWORD at data−28 and six affix DWORDs at data−24.
Capture copies these identities, not addresses. Runtime IDs are not persisted.

Stat root must have exact extended flag`0x80000000` at+1c. Primary vector+30
and aggregate vector+a8 each contain pointer/count/capacity at+0/+8/+10.
Entry stride16: key QWORD`(stat << 32) | layer`, **signed low DWORD at+8**.
The high DWORD is not reliable sign extension. Native accessor consumers and
positive/negative fixture checks support this representation. Vectors require
strictly increasing keys, count≤capacity and count≤2048.

Root first child+90; sibling+68; parent+78 must equal root; state+20 must0.
Host permanent child: flag+1c=`0x40`, expiry+18=−1, owner+0=host; collect primary
vector+30. Attached socket extended child: flag`0x80000000`, expiry0,
owner unit+a0; that unit's+88 must point back to the child and its bank must
match the host. Collect its aggregate+a8 once. Do not walk its nested original
modifier children, which duplicate that aggregate. Reject other flags, timed/
conditional lists, cycles, duplicate units/IDs, parent/owner mismatches and
partial reads. At most64 child lists and6 inserted units are admitted.

Native insertion order comes from socket inventory, not child-list order:
host+a0 control refs DWORD+8 selects host+98 when nonzero, otherwise host+90.
Inventory magic DWORD+0 must`0x01020304`, first unit+10; inserted ItemData+b0
is next. Require exact correspondence with captured attached owners, complete
cardinality and no repeated/missing units. The guarded getter evidence is in
`socket_profile.h`: game RVAs34a385,388c1e,388c42,38ac32,37233b,372355,
372384,3724f0,372502. Existing nine byte witnesses are also required here.

## Active compiled table contracts

The public PluginSDK `DataTableService` ABI1 has size40: serviceSize DWORD+0,
serviceVersion DWORD+4, `getTable` pointer+8, getRow+10, findRowById+18 and
findRowByCode+20. No Item SDK is used. TableView is read-only, game-thread-only
and expires at the next table load. Query each needed table synchronously
within tooltip formatting; do not cache row pointers. Validate its structSize,
TableId, bank, exact stride, nonzero revision, count≤65536 and readable span.
All queried views must share one revision; the Properties view must also
match the existing native Properties pointer/count. Busy, unavailable, wrong
ABI/stride/revision or missing services decline only the extra source path.
Jewels with already available affix metadata do not require this service.

Read-only discovery also bound these views to the inspected native bank
structure. Game global+2a9a580 holds bank slots of16 bytes. The addresses are
not retained by the runtime adapter; it uses public table views.

| Table | Native pointer/count offsets | Row stride | Relevant fields |
| --- | --- | --- | --- |
| Properties |240/248|30|Function bytes18–1e, target stat WORDs20–2c|
| ItemStatCost |1258/1260|144|Stat WORD0, **ValShift byte14**, descpriority WORD30, descfunc WORD32|
| ItemTypes |1348/1350|e8|Code DWORD0, signed parent WORDs4/6|
| CubeMain |1570/1578|16c|Recipe schema below|
| Items (combined) |15a0/15a8|1c0|Code DWORD80, primary/secondary signed type WORDs12e/130|
| Gems |1658/1660|c0|Name ASCII32 at0, compiled class ID DWORD28, context specs30/60/90|

Loaded counts: Items800, ItemTypes120, Gems76, CubeMain15866. These are evidence,
not required counts. Gem contexts are weapon0/armor1/shield2; each has three
16-byte `(property u32, parameter i32, low i32, high i32)` specs. A compiled
class ID identifies the inserted rune/gem. Native `weap`, `armo`, `shld` ancestry
selects host context; reject ambiguous classifications, invalid parents and
cycles while allowing shared DAG ancestors, bounded to64 distinct nodes.
El625's first property73 is Light Radius1 in every context; second is property15
Attack Rating50 on weapons, property0 Defense15 on armor/shields. Slot order
is not presentation order. Perfect Ruby's fire endpoints remain15 and20,
rather than a random scalar15–20 interval.

The item row getter is game314110 via existing Core slot702230. Witnesses:
game31411d reads count15a8, game314132 accesses15a0, game314162 multiplies1c0.
Native type consumers game373970/373a06 read WORDs12e/130, Core4f1e44 reads
the two type parents. **ValShift14, not byte16 (old save bits)** is proven by
game3cff74: `0f b6 4e 14 48 8b d0 d3 e3`.

Additional optional guards in `contribution_profile.h`:

| Module/RVA | Bytes |
| --- | --- |
| game3cff74 |`0f b6 4e 14 48 8b d0 d3 e3`|
| Core4f1e44 |`49 0f bf 54 0d 04 4d 0f bf 44 0d 06`|
| game373970 |`0f bf 98 2e 01 00 00`|
| game373a06 |`66 83 b8 30 01 00 00 00 0f 8e 41 ff ff ff 0f bf 98 30 01 00 00`|

These four match the read-only captured code windows. Admission additionally
requires existing exact provider/hash/formatter and socket witnesses. No new
native hook, input slot replacement or gameplay write is introduced.

## Corruption recipe schema and reconstruction

CubeMain row364: enabled byte0, ladder byte1, difficulty byte2 (`ff` any), op
byte3, parameter DWORD4, threshold signed DWORD8, number of inputs bytec,
first ladder season DWORD10. Seven inputs start14, stridea: flags WORD0
(`1` specific item / `2` type), identity DWORD2, quality byte6, quantity byte7,
remaining restriction bytes8/9. First output ordinal DWORD5a, quality byte64,
kind byte67 (`fe` useitem). **Five output modifiers start77**, stridec:
property DWORD0, parameter signed WORD4, low/high signed WORD6/8, chance bytea,
reserved byteb. Loose text recipe order is not the loaded compiled order.

Observed preserved completion has own stat361=2 and stat362=dummy168. Recover
roll67 as dummy−101 only with marker2 and dummy102–201. Native compiled order
selects the first applicable enabled op15/parameter362 recipe with threshold
1…roll, two inputs including item code`ka3 ` and agreeing host type/quality.
Reject unsupported ladder/difficulty/season/input restrictions rather than
guessing a later positive recipe. The observed winner is loaded row212:
property29 ED50, property66 Life Steal8, property374 corruption flag1 and
property375 dummy101. Preserve-output proof requires kindfe and quality0.
Marker/dummy properties are found dynamically through Properties targets361/
362; actual bonus specs require chance100 and reserved0.

Only fixed additive property-function-proven bonuses can be subtracted from
merged host stats. The original residue must fit all host definitions and the
formatted combined value must agree. Then `[C]` receives its independent fixed
source row/bounds. Sample: native ED341 becomes combined317–350 with host291
in267–300 and corruption50; Life Steal8 gains `[C]`. Variable/unrecognized
operations decline. Dummy101 is a recreated item with no recoverable positive
roll; retain its new ordinary rare affixes and add no `[C]` properties. Pending
or invalid states similarly establish no bonus. Multiple/unknown marker
states are excluded; this is not a general corruption history decoder.

## Validation boundary and restart cases

Seven CTest suites pass under MSVC/W4/WX, including artifact ABI/version/export
admission and the full production capture→metadata→label→merge path. Coverage
includes IAS20+15, Life13+16 with shift8, flat max96+11 despite composed146,
consumed ED249+38, socket-only cold absorb7, El weapon/armor contexts, Ruby
paired endpoints, fixed corruption50/8, ordinary conversions, pending states,
cycles/reversed child order, missing ownership, mismatched totals, stale
table revisions, conditional/enchantment/runeword refusal and atomic capacity.
Unique uncertain choices and paired physical scalar misuse have refusal tests.

An optional local invocation of `range-contribution-tests.exe <capture-dir>`
replays captured Items/ItemTypes/Gems/CubeMain bytes (not shipped fixtures).
It confirms weapon/armor classification, row212 ED50/Life Steal8 selection and
El's actual spec order/context. This caught and corrected a fixture assumption
that Attack Rating occupied El's first slot. Production decoding already
walked every spec. The four new byte witnesses match captured loaded code.

Read-only source/recipe evidence is live. User screenshots on 2026-10-12
confirm revision27 rendering on the retained controls:

- Stone Fang ED287 with complete265–306, host249 in234–266 `[P] [Cruel] [T2]`
  and socket38 in31–40 `[Socket 1] [P] [Ruby] [T1]`; gray rows appear beneath
  the combined total. Socket-only Cold Absorb7 shows4–7 and
  `[Socket 1] [S] [of Desire] [T1]`. Header15–153 (Base4–28) is retained.
- Preserved Cruel sword ED341 with complete317–350, host291 in267–300
  `[P] [Cruel] [T1]` and fixed50 `[C]`; Life Steal8 has explicit8–8 bounds
  and `[C]`. Header13–83 (Base3–19) and the separately labeled Shout proc
  retain their native actual values and earlier display behavior.
- The user explicitly confirms releasing Ctrl restores the normal tooltip.

These establish visible attribution and successful candidate admission for
the tested paths. Converted-rare, Life/flat-max/IAS, El and Ruby fixture
rendering are not additional user-confirmed revision27 coverage; their owned
source evidence and automated tests remain separate. Available retained
items suffice; do not require recreating deleted samples or another persistence
loop. Enchantment attribution stays pending. No publication yet.

Revision27 was installed for this standalone restart test after confirming
the game/loader processes were absent and the active global DLL matched
revision25 SHA256`4666A4F1AA252BBC6DF453621F613F480680F4E5E2411A542A1B1F4318DEC213`.
Installed location:
`<game-directory>/d2rloader\plugins\Item Roll Ranges.dll`.
Candidate and installed SHA256 both
`0E26ACF6E4DFF0A4BC191DBF1AF2CE54F9A827AB68E5B95E6F7A24FB7085C7FA`.
Rollback copy:
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev25.before-rev27.20261012-111339.dll`
under that game installation. All other runtime DLLs and TOMLs matched the
before/after hash inventory. Receipt stays local in
`outputs/item-roll-ranges-rev27-deployment.json`. That receipt proves disk
installation; the subsequent user screenshots establish the visible checks
listed above. No publication yet.

A subsequent read-only release-readiness check confirms the installed DLL
still matches the candidate hash above. The standalone plugin log records
revision27 initialization at2026-10-12 11:16:23 without a compatibility or
optional-table-service warning. This startup evidence is additional to the
user's accepted tooltip screenshots, not broader item/controller coverage.

## Revision28 socketed generated-ED armor header repair

User screenshot shows Beast Shell Serpentskin Armor with Defense246 (?) and
a correct Life90 breakdown: own33 in31–40, Hope jewel19 in9–20 and Perfect
Ruby38 fixed; complete Life bounds78–98 and native socket labels/order agree.
This adds visible multi-socket and scaled Life coverage for revision27.

Read-only capture of runtime item49/class360/quality6/bank3 confirms primary
Defense31=127, own permanent ED16=94, aggregate Defense246. Generated armor
uses loaded maxac126+1=127, so floor(127*194/100)=246. Intrinsic Defense is
already stored; no inverse calculation is needed. Children appear in reverse
insertion order: Perfect Ruby56/class596 extended aggregate Life9728, Hope
jewel55/class658 extended aggregate Life4864, then own permanent list with
Life8448/ED94. The old `ModifierOperand` required every child to have host
owner+0, non-extended flags and expiry−1; it rejected the first socket child.
Thus `ArmorBaseRange` lost its positive host ED witness for the maxac+1 shape.
This was a pre-existing header restriction exposed by the new socket test,
not a failure of the new Life source calculation.

Revision28 adds shared `CaptureItemContributions` and uses its copied own
operand for the header generation check. Its existing source byte guards,
owner/parent/state/expiry checks and full insertion-order/cardinality proof
apply. Socket ED and nested original socket copies are excluded. The legacy
no-socket path remains available when shared capture is unavailable; malformed
or unqualified socket lists cannot pass it. Intrinsic/table/ethereal checks
and all native header values remain unchanged. No new RVA/offset/hook is added;
the source contracts recorded above are reused.

All seven suites pass. A production regression reproduces the old rejection
with the shared guard disabled and verifies `Defense: 246 (Base: 127 - 127)`
with native colors after admission. It covers two reversed socket siblings,
no added socket ED counted as host ED, missing own ED, bad parents/conditional
state/cyclic inventory and ethereal fixed base190. Existing socket/corruption,
header/input, artifact and text suites still pass. New header rendering is
pending live confirmation. Receipt `outputs/flatmax-beast-shell-defense.json`
and probe output remain local, along with the user's screenshot
`screenshot-b2701fab-84f7-4834-8011-bdfb93dc5b7b.png`; no game assets/logs
are committed. Enchantment inference remains deferred; no release published.

After the user confirmed the game/loader closed, revision28 was installed in
the same standalone global plugin path. Built/installed SHA256:
`84808C71C3F411CBFC4939C42688885CD64CE525543F99F5F095A4A2BA2A7CCA`.
Revision27 rollback:
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev27.before-rev28.20261012-122506.dll`
under the game installation. Other runtime DLLs and TOMLs matched their
before/after hash inventory. Local receipt:
`outputs/item-roll-ranges-rev28-deployment.json`. Installation is confirmed;
revision28 restart admission and rendered header remain pending.

User confirmed revision28 after restart: Beast Shell displays Defense246 with
Base127–127, and its Life breakdown remains correct (2026-10-12).
