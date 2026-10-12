# Socket, corruption and enchantment contribution validation

Started 2026-10-12 after publishing Item Roll Ranges rev.25 and Floating Damage
beta.12 in `release-2026.10.12.1`. These investigations are outside that release;
there is no new source label or attribution implementation enabled yet.

## First controls

1. Rare item with a gold-find or magic-find affix: retain the original and
   enchanted screenshots, full Ctrl tooltip, recipe and enchantment count.
2. Jewel whose stat overlaps its host: retain the host before socketing, the
   jewel's separate tooltip, and the host after socketing. A plain control with
   one known rune/gem establishes the host-context effect independently.
3. Completed corruption, both with an overlapping and a new stat if available:
   retain before/after screenshots and the resulting item for read-only inspection.
4. Combine these only after isolated controls agree. Keep each earlier stage.

## Source accounting and refusal

Native actual totals remain authoritative. A source label identifies a verified
contribution, not proof that it accounts for the entire line. Do not infer
enchantment provenance solely from a matching value or an upgrade counter.
Identify ordinary affixes/base/unique/set properties, socket contributions,
runeword additions and corruption first. Recipe inference may attribute only
the contribution shared by every valid decomposition; retain combined display
where explanations disagree. Preserve per-layer identities and native integer
scaling, saturation and rounding. Conditional set lists cannot be silently
treated as permanent item modifiers.

## Reusable socket research; pending Item Roll Ranges validation

Existing sibling item research documents ordered socket capture in
`include/D2RLItems/socket_order.h`, with independent witnesses in
`src/stat_accessor.cpp`. Reuse the evidence, not an Item SDK dependency.
For the reviewed D2R/Core profile only:

- Unit `+0xA0` is the inventory control; control `+8` reference count chooses
  the inventory pointer at Unit `+0x98` when nonzero, otherwise `+0x90`.
- Inventory magic is `0x01020304`; the first inserted item is at `+0x10`.
- Inserted unit type is 4; ItemData is Unit `+0x10`; ItemData `+0xB0` is next.
- Independent extended stat-list ownership uses list `+0xA0` owner, negative
  flags at `+0x1C`, owner Unit `+0x88` equal to that list, and list `+0x78`
  equal to the host stat-list root. Check exact identity and cardinality;
  stat-list order is not socket order.
- Earlier game instruction witnesses include `0x34A385`, `0x388C1E`,
  `0x388C42`, `0x38AC32`, `0x37233B`, `0x372355`, `0x372384`, `0x3724F0`
  and `0x372502`. Their expected bytes are recorded in the sibling research
  accessor. They are evidence candidates until copied into a focused profile
  and rechecked against the active artifacts. No new call or hook is admitted
  by this document.

Bounded captures must reject missing reads, cycles, duplicate runtime IDs,
host/self links, unrelated owners and count disagreement atomically. Only
owned identities/stat values may outlive the tooltip/capture scope. Jewels
need their own rolled-property attribution; rune/gem bonuses use the host's
weapon/armor/shield context. Runewords need ordered ingredients and an agreeing
active definition, not an unordered ingredient match.

## Corruption and enchantment distinctions

Sibling Corruption Marker research records saved stat 361 `item_corrupted`
and 362 `item_corruptedDummy`: a resolved dummy stores the original roll plus
101. The appropriate recipe must be selected by current item applicability
and native file order, and checked against active definitions. An unresolved
1-100 value is not a completed outcome. This is existing research and local
table evidence, not an Item Roll Ranges runtime validation result.

The inspected enchanted Doubleshot Machine combined its unique modifiers,
gold find 30, magic find 15 and upgrade counter 395=1 in one permanent state-0
child list. It supplied no separate enchantment source list. Rare affix IDs
establish bounds, but do not restore their original rolls after an overlapping
enchantment. Use complete active recipe combinations and counts; retain
ambiguity instead of inventing a numeric split.

Next required evidence: an occupied socket fixture with a separately captured
jewel and host-before value, followed by known rune/gem and corruption controls.

## Revision 26: first IAS control

The prepared Eagle Barb Long Sword supplied the positive ownership/order
witness. Live read-only capture: host class 31, quality 6, total stat93=35;
one ordered inserted item class658 has its own negative-flags extended list,
whose aggregate stat93=15. User reports the original host had fixed 20% IAS
from of Alacrity. The socket reader separately sums permanent host child IAS
and reconciles it with the recorded fixed affix definition; a production
adapter fixture mirrors 20 + 15 = 35. The item also has an enchanted Doubleshot
Machine control with an inserted elemental jewel; it is outside the enabled
IAS breakdown and does not acquire an inferred enchantment label.

Implementation uses only existing tooltip adapters. A separate exact-game
`socket_profile.h` contains the nine previously recorded and live-matched
instruction windows. A mismatch disables only socket breakdown. The bounded
reader copies identities/IAS amounts and retains no native pointers. Host
lists require owner equal to host, parent equal to host root, expiry -1 and
state zero; inserted roots require negative flags, state zero, owner unit type4,
owner Unit+88 equal to the child and parent equal to host root. Native selected
inventory/order and socket cardinality must agree. Sorted 16-byte stat entries
and their 64-bit counts/capacity are bounded and checked; only unlayered stat93
is interpreted. Whole actual totals are cross-checked before display.

The property scalar bounds resolver now accepts function8 only for stat93;
source and compiled swing1/swing2/swing3 definitions map it to
item_fasterattackrate. The verified Alacrity weapon row has fixed min=max=20.
No unrelated function8 property is promoted into a numeric range.

Presentation preserves the actual 35% line above gray fixed source rows, one
20% row with the original suffix name/tier and one 15% row tagged [Socket 1].
The fixed-source renderer requires exactly one percentage value, agreeing
source sum, unambiguous tooltip identity and sufficient output capacity.
Variable host bounds decline this first display path. Individual socket
amounts may be variable actuals in the reader, but variable affix/range
presentation and other stat families are intentionally pending the next control.

Six MSVC Release /W4 /WX suites pass: existing policy/adapters/artifact/text/
affix cases and new sockets. Production adapter coverage checks real capture,
fixed-source merge and guard/aggregate/host-definition disagreement; reader
fixtures check cycles, invalid owners/parents, inventory selection and missing
entries. The new visual breakdown is pending restart testing. Revision26 has
not been published.

Read-only first socket probe: all nine previously recorded game instruction
windows matched in the running qualified game. Bounded client item discovery
returned no occupied socket with the complete owner/order proof in that capture.
This is not proof that the character has no socketed items, and it does not
validate the positive traversal or socket contribution math. The requested
occupied jewel fixture and separate before-socket value are still needed.

Publication verification: release-2026.10.12.1 points to
9879d9431be1dd82af88c2271bd14eb297ac1bf1; scoped source integration on main is
610df151dbc3f81b237adec86910d5c7f48b7e00. Both build workflows and the release
workflow succeeded. Five downloaded ZIPs and external/internal checksums,
reference-only configs, UTF-8 documentation/privacy checks and all five
published-DLL artifact verifiers passed. GitHub identifies this tag as latest.
The separate QOL beta remains separate from the suite's included rev.75.

## Variable flat maximum-damage control: before insertion

Read-only baseline captured before socketing the second jewel. Eagle Barb Long
Sword has permanent own-child stat22=96 and stats17/18=109; its recorded flat
maximum-damage affix bounds are 81-100. Its root intrinsic one-hand damage is
3/19, while composed one-hand damage is 6/135. The first inserted of Fervor
jewel remains separately owner-qualified with stat93=15 (combined IAS35).
The separate of Carnage jewel has rolled value11 and loaded definition bounds
10-15; its ordinary child/aggregate carries the same addition in stats22,24,
and160. These are native attack-mode keys, not three independently additive
contributions to the same displayed line. All nine socket instruction guards
matched. Local full receipts are kept outside source control.

This control already differs from IAS: parent aggregate stat22=135 is computed
weapon damage, while host modifier stat22=96 is the flat roll. A generalized
source algorithm must not sum flat contributions against the parent damage
header value. Expected flat combined modifier after insertion is107 (96+11);
its source-interval sum would be91-115 only if native formatter/definition
context confirms complete attribution. Post-insertion observation is pending.
No plugin change, build, installation or publication was performed for this
sample. Keep the published rev25 DLL active while collecting evidence.

### Variable flat maximum-damage control: after insertion

Read-only post-insertion capture confirms the same runtime host (47) and
of Carnage jewel (50) with unchanged affix IDs/loaded definitions. Host own
permanent child stat22 remains96; second inserted jewel's owner-qualified
extended aggregate remains11 in stats22/24/160, and its nested permanent
modifier list also preserves11. The jewel's of Carnage definition remains
10-15. Host of Evisceration bounds remain81-100. The source flat sum is107;
parent one-hand computed max increases135->146, while min remains6 and total
IAS remains35. With primary max19 and host ED109, the observed max agrees
with floor(19*209/100)+96+11=146. This is a controlled single-item read, not
proof of every upgraded/ethereal/attack-mode rounding case.

Native inventory order is IAS jewel48 first, max-damage jewel50 second.
Host stat-list child order is max-damage jewel50, IAS jewel48, host modifiers.
Never use child-list iteration position as socket index. Independent identity,
owner/parent/root and cardinality proofs matched, as did all nine instruction
windows. A variable jewel's actual contribution and affix identity survive
insertion, so this sample supports exact per-source values/ranges without
solving a sum of unknown affix rolls. It does not establish a generalized
formatter mapping, elemental/poison handling or enchantment/corruption inference.

Expected source interval for the flat max modifier is91-115 (81-100 plus10-15),
if the display path accounts for both sources. Current Ctrl-held modifier text
and range are requested from the user separately; raw composed stat22=146 is
a damage-header operand and must not be mistaken for flat modifier107.
No production code, candidate install, build or publication changed during
this before/after sample. Continue collecting observations before generalizing
or deploying the held IAS prototype.

User-confirmed Ctrl tooltip after the second jewel: ordinary actual line is
+107 to maximum weapon damage; held line is [+81 - +100] +107 to maximum
weapon damage [S] [of Evisceration] [T2]. This proves current native-provider
annotation covers only the host affix while displayed actual includes the
socket contribution. Do not present that interval as complete for the107
combined modifier. The exact-source capture supports a91-115 combined flat
interval with separate96 (81-100) host and11 (10-15) socket2 rows. This is a
proposed display requirement, not enabled rendering. The jewel's other
attack-mode keys22/24/160 must not be added again to the same displayed stat.

Next contrasting control should exercise a variable Enhanced Damage jewel
on an ED host: op13 operands can be consumed by damage computation, so check
original/socket modifier lists versus composed damage and retain exact before/
after values. Rune/gem host-context and conditional-set/corruption controls
still remain before committing to a general attribution model.

## Variable Enhanced Damage control: before insertion

Read-only Stone Fang Long Sword baseline: host runtime53, class31, quality6,
Cruel affix835 with compiled ED bounds234-266 and own permanent child stats17/
18=249. Intrinsic one-hand damage4/28 and computed damage13/142; own flat max
stat22=45 from an Evisceration definition41-63. This additional flat modifier
must stay separate from percent ED. There is one empty socket and no attached
socket-owned child in this baseline. The ordinary own-child also carries IAS20,
Knockback, a hidden passive and a charged skill; these are unchanged controls.

Separate Ruby Jewel of Desire runtime55, class658, quality4: Ruby affix843
ED31-40, actual38 in stats17/18; Desire affix180 cold absorb4-7, actual7 in
stat149. Jewel aggregate and permanent own-child both preserve the ED operands
while unsocketed. All nine socket game instruction guards matched. Baseline
receipts remain local; no game calls/writes or plugin changes occurred.

Expected after insertion: additive weapon ED287 (249+38), cold absorb7, and
unchanged host flat max45/IAS20. The combined ED definition interval would be
265-306 only with complete agreeing source attribution. Post-insertion checks
must establish whether socket-root aggregate retains consumed ED operands or
whether they are available only in nested original jewel modifier lists.

### Variable Enhanced Damage control: after insertion

Read-only capture verifies Stone Fang53 and Ruby Jewel55 retain their original
recorded affix IDs/compiled bounds. Jewel55 is independently owner-qualified
as the sole socket in native inventory order. Host own permanent child still
contains stats17/18=249 and flat max stat22=45; socket-owned negative-flags
extended aggregate and jewel's nested original permanent child both retain
stats17/18=38 and flat cold absorb stat149=7. Do not count the aggregate and
nested jewel copy twice. Total ED operands account for287, while parent
composed stat list contains damage15/153, IAS20 and cold absorb7; the parent's
aggregate does not enumerate ED17/18 as separate entries.

The host is ethereal (ItemData flag0x00400000 confirmed), with intrinsic one-hand
base4/28. Observed damage agrees with floor(4*387/100)=15 and
floor(28*387/100)+45=153, compared with baseline13/142. This single control
confirms percent and flat source separation and this item's integer rounding;
it does not establish every attack mode, ethereal upgrade or modifier operation.
All nine socket instruction windows matched before/after. Current Ctrl tooltip
ED/cold-absorb annotation is requested separately from the memory evidence.
No production code, candidate installation/build or publication changed.

A further contrasting control should cover life/mana scaling. Installed source
ItemStatCost maxhp7 and maxmana9 have ValShift8, while IAS93/cold absorb149
have no such shift and ED17/18 use op13. An existing separately captured
of Hope jewel has raw maxhp4096 and definition9-20, consistent with a displayed
16 life after dividing by256; that display has not been user-confirmed in this
sample. Do not treat every owned raw scalar as an unscaled tooltip number.
This is source-table/read-only evidence; compiled descriptor offsets and new
normalization code are not admitted by this note.

User-confirmed ED Ctrl tooltip after insertion: [+234 - +266] +287% Enhanced
Weapon Damage [P] [Cruel] [T2]. Cold absorb has neither range nor source label.
This is a second positive host-only range mismatch: the current interval covers
only Cruel, while the actual includes Ruby. For this additive ED operand,
complete source bounds sum265-306 (234-266 plus31-40), with exact249 and38
source values. The7 cold-absorb line is supplied entirely by the jewel's
of Desire suffix, bounds4-7, and must be eligible for socket-only range/source
annotation even without any matching host affix. Socket attribution cannot
be implemented solely as an add-on to labels created from the host's affixes.
This is a design requirement established by live capture plus user-visible
text; no generalized socket label/range code has been enabled or installed.

Next sample: life/mana jewel on a host with an overlapping life/mana property,
preferably first captured while separate. Include host/jewel before values and
both ranges; then inspect insertion. This will contrast ValShift8 raw units
against percentage/plain integer cases before revising the shared source model.

## Overlapping Life control: before insertion

Read-only baseline on 2026-10-12 identifies Doom Wrap Serpentskin Armor as
runtime56, class360, quality6. Its own permanent child and composed aggregate
both retain maxhp stat7/layer0 raw3328, matching the user-confirmed +13 Life
after ValShift8 conversion (3328/256). Its of the Wolf affix282 uses property13,
compiled bounds11-20. Other host affix identities were captured unchanged for
the subsequent comparison. No socket-owned extended child is attached yet.

Separate Jewel of Hope runtime16, class658, quality4 retains raw4096 in maxhp7
in both composed aggregate and its own permanent child, matching the user's
+16 Life (4096/256). Its of Hope affix300 has compiled bounds9-20. All nine
existing socket instruction guards matched. Diagnostic-only capture now accepts
explicit class filters to cover armor as well as the earlier swords/jewels;
this does not change production readers, hooks or attribution code.

After insertion, expect Life29 with independently owned13 and16 sources. A
complete additive source interval would be20-40 (11-20 plus9-20). Establish
whether the socket aggregate retains raw4096, whether host own raw3328 stays
intact and whether composed maxhp becomes7424 before adopting normalization.
Ask for Ctrl-visible actual/range separately from this memory evidence.
Local receipt: outputs/flatmax-life-before.json; no game calls/writes, plugin
build/install or publication occurred for this sample.

### Overlapping Life control: after insertion

Read-only post-insertion capture finds the same host56 and jewel16 with
unchanged affix IDs/bounds. Host own permanent child maxhp7 remains raw3328
(13 Life); the independently owner-qualified socket extended aggregate retains
raw4096 (16 Life), duplicated in the jewel's original permanent modifier child.
Parent composed maxhp is7424, exactly3328+4096 and29 after ValShift8 conversion.
Native socket inventory order identifies jewel16 as Socket1, and its unit root,
parent relationship and cardinality agree with the attached stat-list source.
All nine existing socket instruction windows matched. The second socket is
empty; socket capacity2 must not be treated as two present contributions.

User-confirmed Ctrl tooltip: [+11-+20] +29 to life [S] [of the Wolf] [T6].
The displayed actual includes both sources while current bounds describe only
the host's of the Wolf roll. Complete additive bounds are20-40, with exact13
(11-20) host and16 (9-20) Socket1 of Hope contributions. This confirms source
identity survives insertion for a ValShift8 stat; normalize captured raw units
before comparing with tooltip values or unshifted affix definition bounds.
Do not double-count the jewel's extended aggregate and original modifier copy.

Receipts outputs/flatmax-life-after.json and outputs/socket-source-life-after.json
remain local. This is read-only native state plus user-confirmed visible text;
generalized source rendering has not been implemented, built or installed.
The Life case joins fixed IAS, variable flat max and consumed ED operands;
rune/gem host-context and corruption/enchantment attribution remain separate
controls before revising the shared model.

## El rune host-context control: before insertion

Read-only baseline on 2026-10-12 captured plain quality2, non-ethereal class31
Long Swords65/69 with one empty socket each and intrinsic/composed one-hand
damage3-19. No rolled affix IDs or socket-owned child lists are present. The
swords retain a native permanent skill stat97/layer449=1; do not mistake this
existing base property for a rune contribution. Plain quality2 class360
Serpentskin Armor74/75 each retain intrinsic/composed Defense stat31=125 and
one empty socket, without rolled affix IDs or attached socket owners. Multiple
matching hosts were retained in the receipt; resolve which changed by actual
socket ownership after insertion rather than guessing from pool order.

Separate El runes are compiled item class625 (code r01), including newly
prepared runtime72/73 and existing4/27. Their primary/aggregate stat vectors
are empty before insertion, despite gems.txt defining their socket effects.
This means an unsocketed rune stat vector cannot supply the host-dependent
contribution. Record source identity/table definition and inspect the attached
socket source after insertion. All nine existing instruction guards matched.

Installed gems.txt El weaponMod entries are light1 and att50; helmMod entries
(body armor context) are light1 and ac15. Expect weapon damage to remain3-19
while gaining Attack Rating50/Light Radius1, and the armor Defense to become140
while gaining Light Radius1. El class625 was confirmed live; raw text row indices
include Expansion separator rows and cannot directly substitute for compiled
item class IDs. Baseline receipt outputs/flatmax-el-before.json stays local.
No native calls/writes, production code, builds, installation or release changed.

### El rune host-context control: after insertion

Read-only post-insertion resolves Long Sword65 with El73 and Serpentskin
Armor75 with El72. Both runes remain class625, quality2 and have no affix IDs.
Each is independently owner-qualified as Socket1 using its unit stat-root,
host parent, native inventory order and one-child cardinality. All nine existing
instruction guards match. Other matching baseline hosts69/74 were not selected
as changed items merely because their class/quality matched.

Weapon-context El73 now has attached extended aggregate stat19/layer0=50
(Attack Rating) and89/layer0=1 (Light Radius); these were absent in its separate
baseline. The rune's own permanent child duplicates the same contributions.
Host65 composed aggregate gains19=50 and89=1 while intrinsic/composed damage
21=3,22=19 stays unchanged and its native base skill97/layer449=1 remains.

Armor-context El72 instead has attached extended aggregate31/layer0=15
(Defense) and89/layer0=1 (Light Radius), also duplicated in its own permanent
child. Host75 intrinsic primary31 remains125 while composed aggregate31 becomes
140; Light Radius1 is added. This validates the installed gems.txt host-context
definition mapping for El: weapon att50/light1 versus body-armor ac15/light1.
Do not annotate a body-armor rune using weapon properties or read the rune's
empty unsocketed vector as its eventual contribution; do not sum its original
modifier child with the attached aggregate a second time.

Receipts outputs/flatmax-el-after.json and outputs/socket-source-el-after.json
stay local. Current Ctrl-visible Defense header and rune-added property
annotations are requested separately. This establishes read-only native state
for one rune in weapon/body armor, not every rune, shield/helm mapping, runeword
or gem/elemental/poison descriptor. No production code, builds, install or
publication changed for this sample.

User-confirmed Ctrl tooltip for the El-socketed armor: Defense: 140 (Base:
111 - 126). Neither host shows ranges or source labels on any El-added
Attack Rating, Defense or Light Radius property. The header correctly preserves
the armor's base definition interval independently of the socket's flat Defense;
the socket property annotation path remains absent. Keep the header Base range
111-126 separate from the El's fixed15-15 modifier interval. Likewise weapon
Attack Rating is a fixed50-50 contribution and Light Radius is fixed1-1 for
each host. Exact fixed socket bounds come from the applicable definition, not
from assuming that every captured socket value is fixed. Visible header behavior
is accepted for this control; source breakdown/range rendering is still pending.

## Perfect Ruby elemental interval control: before insertion

Read-only baseline on 2026-10-12 identifies an empty one-socket Long Sword69,
class31, quality2, non-ethereal, no rolled affix IDs. Intrinsic and composed
one-hand damage remain3-19; no attached socket-owned extended child or fire
damage entries are present. Existing native base skill97/layer449=1 remains
an unchanged control. Separate Perfect Ruby candidates76/77 are compiled
class596 (code gpr), quality2, no affix IDs, and empty primary/aggregate stat
vectors; identify the actual inserted unit through ownership after insertion.
All nine existing socket instruction guards matched.

Installed gems.txt weaponMod entries are fire-min with fixed15-15 bounds and
fire-max with fixed20-20 bounds. Expected visible contribution is Adds15-20
Fire Damage, while physical damage stays3-19. The displayed15-20 is a damage
interval with two fixed endpoints, not a single scalar affix roll varying
between15 and20. Source mapping/formatting must preserve that distinction.
This adds a grouped elemental control beyond scalar IAS/Life/flat modifiers.
Local receipt outputs/flatmax-ruby-before.json; no native calls/writes or
production code, builds, installs or publishing occurred.

### Perfect Ruby elemental interval control: after insertion

Read-only post-insertion identifies Long Sword69 with Perfect Ruby77 as its
sole Socket1. Ownership proofs agree: inserted unit root, attached parent,
native inventory order and cardinality. All nine existing instruction guards
match. Gem77's attached extended aggregate contains fire minimum48/layer0=15
and maximum49/layer0=20; its original permanent child duplicates both entries.
The host's composed aggregate gains precisely48=15 and49=20, while physical
primary/composed21=3 and22=19 and base skill97/layer449=1 remain unchanged.
Gem identity class596/code gpr and no-affix IDs are unchanged from baseline.

User-confirmed tooltip: Adds15-20 Weapon Fire Damage with no source labels;
One-Hand Damage remains3 to19 (Base:3 to19). The unchanged physical header
is accepted for this control; elemental damage belongs to the separate property
line. Render fire minimum/maximum as one paired source contribution, not two
scalar labels or a scalar random roll15-20. Its definition endpoints are fixed
15-15 and20-20; elemental endpoint roll variability must remain distinct from
the attack's displayed damage interval. The nested original gem modifier list
must not be counted again alongside the attached aggregate.

Local receipts outputs/flatmax-ruby-after.json and
outputs/socket-source-ruby-after.json. Native state plus visible text confirms
this one fire/gem case; poison rates/duration, cold duration, overlapping
elemental rolls and shield/helm contexts are not established by it. No production
code/build/install/publication changed. Collected socket controls now span fixed
IAS, variable flat max, consumed ED, ValShift8 Life, rune host contexts and
paired fire endpoints; enchantment/corruption provenance remains separate.

## Next isolated enchantment control: overlapping Life on rare body armor

Installed cubemain.txt enabled recipe ENCHANT ITEM - Chest & Ruby & Magic
Jewel = +15 HP & +2 Life / Kill consumes tors, gmr, jew,mag; output useitem,
ilvl100, fixed upgrade/major1, hp15 and heal-kill2 (all chance100). Its recorded
eligibility fields are op16, param394, value4; record rather than infer complete
native predicate semantics from their names. The associated installed panel
modinfo_itemenchantmentshd.json Chest entry agrees. Use a regular Ruby gmr,
not the Perfect Ruby gpr used in the socket control.

Prepare a fresh rare Serpentskin Armor with a rolled +Life modifier, no existing
enchantments/corruption and no inserted socket items, plus separate regular Ruby
and magic jewel. Capture original affix IDs, actual Life, bounds, all permanent/
conditional/socket lists and upgrade counters before the user transmutes once.
Then retain the final item and compare any ID/list reallocation, merged stat7
Life, added heal-kill and major counter394; request Ctrl tooltip separately.
Expected Life increase15 is a controlled recipe delta, not proof that a general
runtime plugin can recover that source on arbitrary saved items. Evaluate
whether final state alone determines the recipe/split or leaves alternatives.

### Rare body armor Life enchantment: captured baseline

Read-only 2026-10-12 baseline identifies the fresh unsocketed rare Serpentskin
Armor81, class360, quality6, bank3. Its of the Mammoth affix284 has property13
Life bounds31-40; own permanent child and composed maxhp7 are raw9728 (38
Life after dividing256). Affix IDs are996/1181/1425/284/140/0. Other own-list
controls are ED16=144, magic damage reduction35=11, fire resistance39=28 and
charged skill204/layer3855=8971. Intrinsic Defense127, composed309; no stat194
socket capacity, socket-owned children, corruption361/362 or enchantment
counters392-395 are present. The sole modifier child is owned by81, parent is
its root, flags40, state0, expiry-1. All nine instruction guards matched.

The earlier Doom Wrap56 still has its Hope jewel and is explicitly excluded
from this isolated enchantment sample. Additional generated rare armors78/79/80
have no maxhp7 Life roll (suffix names of Life/of Health refer to other stats);
do not choose them by suffix wording. Actual38 and compiled31-40 identify the
positive overlap control. Requested next action is one regular-Ruby/magic-jewel
Life recipe; expect raw13568 (53 Life), heal-kill contribution2 and major
counter394 increase1. Capture actual result and counter rather than assuming
transmutation followed the disk recipe. Ingredient identity has not been proven
by a loaded base-code reader; retain the user-reported regular Ruby and blue
jewel as recipe setup evidence. Local outputs/flatmax-enchant-life-before.json.
No game calls/writes or production code/build/install/publication changed.

### Shadow Pelt Life enchantment: after transmutation

User identifies the control as Shadow Pelt Serpentskin Armor and confirms one
recipe transmutation: Life38->53 and new Life after Each Kill2. Screenshot with
Ctrl shows [+31 - +40] +53 to Life [S] [of the Mammoth] [T4], an unlabeled
+2 Life after Each Kill, Enchantments1/5, and Defense309 (Base127-127). Other
ED/resistance/charged-skill properties remain as in the baseline.

Read-only capture matches the same unit/runtime81, class360, quality6, bank3,
root and affix identities. The original permanent modifier child was replaced;
its replacement is still the sole child, flags40, state0, expiry-1, owner81
and parent equal to the host root. That one child's primary now combines
maxhp7 raw13568 (53 Life), heal-kill86 raw2 and upgrade_major394=1 alongside
unchanged ED16=144, magic reduction35=11, fire resistance39=28 and charged
skill204/layer3855=8971. Composed aggregate agrees; intrinsic Defense127 and
composed309 are unchanged. No separate enchantment-owned child or original38
Life entry survives in the captured vectors. All nine instruction guards match.

Controlled before/after delta is maxhp3840=15 Life, heal-kill2 and major count1.
For this known recipe, original38 (31-40) plus fixed enchantment15 would imply
combined Life bounds46-55, and the new heal-kill contribution could be labeled
[E]. These are desired rendering facts for the controlled sample, not enabled
annotation or proof of general inference from arbitrary saved items. Original
affix IDs retain bounds31-40 but the original roll is no longer separately
stored. A general final-state reader must account for other contributors and
all applicable recipe alternatives; it may label only an agreeing attribution.
Do not rely on a user's baseline being available for ordinary saved items.

Ingredient correction: the user had to obtain a separate recipe Ruby because
the standard socket Ruby does not satisfy the recipe. Installed recipe code is
gmr; distinguish this recipe Ruby from standard socket Rubies and Perfect Ruby
gpr in future preparation instructions. This confirms why probing candidates
using text-derived regular-Ruby class assumptions did not establish ingredient
identity before transmutation. No compiled gmr class ID has been admitted by
this test. Local receipt outputs/flatmax-enchant-life-after.json. No game calls/
writes or production code/build/install/publication changed for this sample.

## Next corruption control: observe random outcome without targeting it

User notes random corruptions can add nothing. Do not require the user to farm
a specific result: an unresolved first-stage control and a resolved no-change
outcome are useful refusal/false-attribution cases. Prepare a fresh magic or
rare Long Sword with no enchantment, corruption or inserted socket contributors,
and an Orb of Corruption ka3. Prefer an existing ED roll for possible overlap,
but do not make a particular random outcome a prerequisite. Capture baseline
first, then ask for exactly one transmutation and inspect saved361/362; then
request the second transmutation separately after predicting the applicable
definition from the recorded pending roll. Preserve successful, no-change and
quality-conversion outcomes as their own results rather than rerolling them.

Installed cubemain.txt confirms the DUMMY output includes a returned ka3 orb;
first-stage corruptedDummy is1-100 with corrupted1. SUCCESS adds101 to the dummy,
and applicable definitions are ordered by thresholds; use native applicability
and file order rather than matching text descriptions (some descriptions repeat
the wrong interval). Weapon Nothing threshold21, rare conversion11, normal
conversion1 are recorded control candidates. Current weapon ED outcomes at
thresholds77/71/65 add dmg%50 with att250/ignore-ac1/lifesteal8 respectively.
These are installed source evidence, not IRR runtime corruption validation.

Prior sibling documentation's statement that every SUCCESS mod has min==max
does not cover the current installed weapon dmg-norm50/100 definition at
threshold46. Interpret bounds via the property's actual function: this can
represent paired damage endpoints rather than a single random scalar. Do not
promote a universal fixed-value assumption from older corruption recipes.

### Two Cruel Long Swords: captured pre-corruption controls

Read-only 2026-10-12 baseline matches user-prepared magic class31 swords174/175,
quality4, bank3, non-ethereal. Both have Cruel prefix836 bounds267-300. Sword174
(of Shouting suffix494) retains own permanent ED17/18=291 and proc201/layer8849
=8; intrinsic physical3/19, composed11/74. Sword175 (of Transcendence suffix39)
retains own ED17/18=267 plus flat minimum21=37 (suffix bounds30-40); intrinsic
3/19, composed48/69. Flat37 is an existing modifier, not a corruption addition.
Both retain native base skill97/layer449=1 and sole permanent own-list child
flags40/state0/expiry-1. No corruption361/362, enchantment392-395 or socket-owned
child is present. All nine existing socket instruction guards matched. Receipt
outputs/flatmax-corruption-before.json stays local. Ask for one transmutation on
each sword first, retaining the unresolved stage before completing either.
No native calls/writes or production code/build/install/publication changed.

### Two Cruel Long Swords: observed completed corruption outcomes

After the user reported one transmutation on each, the next read-only capture
already contains applied outcomes rather than unresolved dummy1-100. The user
confirms one became rare and the magic sword's ED changed. Do not request a
second transmutation merely to follow the planned sequence: no intermediate
stage was observed, and this sample does not prove how many native recipe
evaluations or input events occurred. All nine instruction guards matched.

Magic sword174 retains class31/quality4, original affix IDs836/494, intrinsic
physical3/19 and its sole permanent own modifier list. ED17/18 becomes341 from
291, Life Steal60=8 is added, corrupted361=2 and dummy362=168 appear in that
same list and composed aggregate where applicable. Recovered completed roll
168-101=67 matches installed first applicable weapon SUCCESS threshold65,
dmg%50 plus lifesteal8. Composed physical damage13/83 agrees with
floor(3*441/100) and floor(19*441/100); proc and base skill stay unchanged.
Corrupted361 need not equal1 exactly: additive flag value2 is observed on this
preserved-item result. Positive corruption attribution for this control can
recover ED50 and Life Steal8 from saved outcome plus applicable definition;
combined ED bounds would be317-350 (Cruel267-300 plus fixed50). The original
ED291 is merged into341, so subtract only after independently identifying the
saved outcome and its property functions. Current Ctrl annotations are pending
separate user verification.

Original267-ED sword175 is absent; new rare Long Sword180 has quality6 and
new affix IDs833/932/17 (Ferocious/Divine/of Evisceration). Own ED is193,
flat max64, damage-to-demons383 and demon attack rating324; intrinsic3/19,
composed8/119. It has corrupted361=1 and dummy362=101, stored in its own
permanent child and aggregate. User-visible rare conversion plus unit/affix
replacement supports a quality-conversion result. The dummy101 does not retain
an exact1-100 original roll: blindly subtracting101 yields0 and must not select
the ordinary lowest-threshold SUCCESS row. This agrees with the disk usetype,rar
recipe rebuilding the item and adding101 to a fresh dummy, but the precise
engine transfer behavior remains an inference. Recognize the observed marker
without inventing a positive corruption modifier or reconstructing exact
original roll. New random rare affixes are ordinary affixes, not added [C]
contributions. A shared source model must handle preserved and recreated items
separately and invalidate captures after replacement; no native pointer reuse.

Local receipts outputs/flatmax-corruption-pending.json (name reflects intended
phase, not observed pending state) and
outputs/flatmax-corruption-observed-outcomes.json. No game calls/writes or
production code/build/install/publication changed. Intermediate unresolved and
resolved no-change outcomes remain unobserved controls.

User screenshot confirms the preserved magic sword tooltip: Cruel Long Sword
of Shout, One-Hand Damage13 to83 (Base3-19), red Corrupted, [+267 - +300]
+341% Enhanced Weapon Damage [P] [Cruel] [T1], and an unlabeled +8% Life
Stolen per Hit. The separate proc remains [+8 - +17] 8% Chance to Cast Level17
Shout When Struck [S] [of Shouting] [T1]. This confirms current native/provider
bounds describe only Cruel while actual ED includes the recovered corruption.
The physical header and ordinary proc annotations remain accepted controls.

Desired preserved-item breakdown: actual341% ED with combined317-350 bounds,
original291% Cruel source (267-300) and fixed50% corruption source, plus the
socket-independent fixed8% corruption Life Steal source. [C] is a proposed
short source marker consistent with [E], not enabled rendering or a settled
general label scheme. Only the saved completed roll67 and agreeing applicable
recipe establish these additions; the red Corrupted text alone does not.
The rare conversion case must retain ordinary new-affix labeling and add no
positive [C] property label based on its completed-marker-only dummy101.
Visible validation and source capture now agree for this positive corruption
control. No production code/build/install/publication changed.

## Save/rejoin verification and end of isolated sample collection

After user-confirmed Save and Exit/rejoin on 2026-10-12, read-only captures
independently match Stone Fang by class/quality/affix IDs and definitions rather
than old runtime IDs (53->29). Intrinsic, composed and own modifier vectors are
exactly equal to its pre-exit receipt. Its sole native ordered socket is the
same Ruby/of Desire jewel definition (runtime55->30); affix definitions,
aggregate and independently attached contributions are exactly equal, retaining
ED38/cold absorb7 alongside host ED249/flat max45. All nine instruction guards
still match. No persisted ID/address assumption is needed for source capture.

Preserved magic corruption sword matches174->39 with identical affixes, own
modifiers, primary and aggregate, retaining ED341/Life Steal8/corrupted2/dummy168.
The rare conversion control matches180->41 with identical new rare affixes and
all compared vectors, retaining ED193/flat max64/corrupted1/dummy101. Thus both
the recoverable positive outcome and the non-recoverable conversion marker
survive this save/rejoin. These are read-only retained source/state checks,
not a new visual rendering or implementation validation.

The other earlier test controls are absent from this loaded capture. The user
confirms the detected items are the remaining controls and directs proceeding
without recreating the others or repeating saves. Use their earlier before/
after receipts for design; stability across saves for those absent samples is
an accepted working assumption, not additional observed coverage. Persistence
receipts outputs/flatmax-persistence.json, outputs/socket-source-persistence.json
and outputs/item-contribution-persistence-comparison.json remain local.

Isolated collection can now end for the first implementation. Build a shared
owned contribution model for host/socket modifiers and paired/scaled values,
independent of the current IAS-only presentation prototype. Reconcile actuals
using property-specific semantics: computed max damage is not a flat sum; ED
operands may be consumed; Life is shifted; fire endpoints are a pair. Support
socket-only properties and variable jewel bounds, native socket order and
host-context rune/gem definitions; never double-count nested socket copies.
Corruption attribution uses recovered completed roll plus agreeing applicable
definitions, while pending/101 conversion states add no guessed [C] modifiers.
Enchantment contributions remain a separate inference problem: apply [E] only
when all permitted explanations agree, not from counters or missing affix labels
alone. Preserve native actual values/header Base meaning and expand verified
sources through the existing formatter path. These are planned requirements;
no generalized implementation/build/install/publication occurred in this check.

## Revision27 implementation after sample collection

The user authorized proceeding with the shared model. Revision27 implements
owned host/socket source capture, guarded numeric reconciliation, variable
jewel ranges, scaled Life, consumed ED and flat-max leaves, socket-only scalar
lines, context-specific active rune/gem definitions and fixed completed
preserved-item corruption attribution. See
[the canonical source/layout record](CONTRIBUTION-SOURCES-REV27.md) for all
new compiled table offsets, byte witnesses, public service ABI and limits.
The earlier IAS-only revision26 candidate was not installed or published.

Seven automated suites pass, including production adapter capture→definitions→
labels→merge and refusal cases. A local replay of captured Items/ItemTypes/
Gems/CubeMain confirms the real ED50/Life Steal8 corruption outcome and El's
weapon/armor context. The four new guards match the captured loaded code.
The game was reopened for read-only compiled layout collection; it was absent
from both process/window and CIM listings at the final deployment check.
There is no new live rendered revision27 result yet. No `[E]` inference or
publication has been added. Sample stability accepted by the user remains
the basis for absent earlier items; no repeat fixture/persistence request.
The global standalone revision25 was subsequently backed up and replaced with
revision27 after the closed-process/hash checks. Installed and built hashes
match; other runtime DLLs and TOMLs are unchanged. Restart/UI validation is
pending; the canonical source record contains the candidate hash and rollback.

## Revision27 user-confirmed rendering — 2026-10-12

Subsequent screenshots show Stone Fang native ED287 with combined265–306,
gray host249 (234–266) `[P] [Cruel] [T2]` and socket38 (31–40)
`[Socket 1] [P] [Ruby] [T1]`. Socket-only Cold Absorb7 now has4–7 bounds
and `[Socket 1] [S] [of Desire] [T1]`. Native header15–153 (Base4–28),
flat max45 and other ordinary affix lines remain present.

The preserved magic Cruel sword shows native ED341 with combined317–350,
gray host291 (267–300) `[P] [Cruel] [T1]` and fixed50 (50–50) `[C]`.
Life Steal8 has8–8 bounds and `[C]`; header13–83 (Base3–19) and the
separate Shout proc retain the earlier display. The user explicitly says
releasing restores the normal tooltip. These pass the requested first
candidate rendering controls; they do not add live coverage for converted
rares or the earlier absent IAS/Life/flat-max/El/Ruby fixtures.

Screenshot references supplied by the user:
`codex-clipboard-be41d166-3dd0-489b-ab88-ad174445d1d8.png` (Stone Fang) and
`codex-clipboard-c2fd02da-cb6b-4a0d-9bc5-4e46ad0c5ab6.png` (Cruel sword).
Images remain local attachments, not repository assets. Enchantment `[E]`
attribution remains unimplemented; revision27 remains unpublished.

## Enchantment follow-up deferred — 2026-10-12

The user reports the Reimagined team is restructuring enchantments. Defer
further attribution implementation and new fixtures until that format settles.
Revision27's validated socket/corruption behavior remains unchanged; no new
production code, DLL, deployment or release occurred in this follow-up.

Read-only investigation before deferral captured the complete active CubeMain
catalog (15866 rows, stride364), Properties434/48, ItemStatCost438/324,
Items800/448, ItemTypes120/232, affixes2011/140, uniques1483/348 and property
groups39/200. The four revision27 byte witnesses match the running build.
Local receipts/dumps remain outside source distribution:
`outputs/enchantment-active-catalog.json`, `outputs/enchantment-active-recipes.json`,
`outputs/enchantment-candidates-360.json`, `outputs/enchantment-candidates-272.json`
and `outputs/active-*-rows.bin`. Local read-only helpers:
`outputs/audit_enchantment_catalog.py`, `outputs/analyze_enchantment_candidates.py`.

The current compiled catalog has355 enabled recipes with upgrade counters.
Properties402/403/404/405 use function17 targeting stats392/393/394/395;
properties421/422 target charm counters419/420. Compiled recipe op/parameter
distribution: op16/39254, op16/393120, op16/39485, op16/39582,
op18/4195 and op18/4209. Loose-table major/uber distribution differs by one;
do not substitute loose recipe classifications for active compiled effects.
This is a snapshot of the installed version, not a contract for the redesign.

For the current preserved-output schema, Shadow Pelt class360/quality6 has30
matching major-counter alternatives; Life15 plus Life per Kill2 is compiled
row863. Doubleshot Machine class272/quality7 has27 matching uber-counter
alternatives. Preserve-output socket and corruption recipes also exist;
counter/value matches alone do not prove an enchantment source. Random skills,
paired elemental damage, multiple upgrade counts, unknown transformations,
counter limits and conditional set lists still need a complete inference
model. No general `[E]` attribution or final-state solver was implemented.

On resumption, inspect the redesigned active recipes and saved source/counter
representation first. Reuse the controlled before/after samples as evidence
for the old format, and test whether the new format retains recipe identity
or separate contribution ownership before choosing an inference approach.

## Beast Shell multi-socket Life and Defense-header control

User screenshot confirms Life90 with combined78–98 bounds and three correct
source rows: own33 (31–40) Mammoth, Hope jewel19 (9–20) Socket1 and Perfect
Ruby38 (38–38) Socket2. This establishes revision27 live rendering for scaled
Life and two inserted sources. Its Defense246 (?) exposes the legacy generated
ED header reader's rejection of extended socket siblings. Read-only item49
capture confirms stored intrinsic127, own ED94, actual246 and socket children
before the host's permanent list. Revision28 reuses validated owned capture
for that host operand; seven suites pass with a two-socket header regression.
Canonical details: [source contracts](CONTRIBUTION-SOURCES-REV27.md#revision28-socketed-generated-ed-armor-header-repair).
New header rendering remains pending the restart check; enchantments deferred.
