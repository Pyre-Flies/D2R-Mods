# Gem Bag UI native contract

First recorded 2026-10-20. This record extends the read-only
[recipe investigation](https://github.com/Pyre-Flies/D2R-Mods-Research/blob/main/docs/reverse-engineering/gem-bag-ui-feasibility.md).
Module locations below are RVAs; no absolute process address is portable.

Beta.17 adds the [deposit contract](DEPOSITS.md), with recorded recipe witnesses
and public SDK destruction. It introduces no native entry point or offset and
retains the artifact guards below. Deposits require protocol v3 on both peers.

## Target and evidence

| Artifact | SHA-256 |
| --- | --- |
| D2RLoader.exe, D2R 3.3.93787, mapped size 90451968 | `93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10` |
| D2RCore.dll, mapped size 19791872 | `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2` |
| Active Reimagined cubemain.txt | `ad1f2d2c302fa936d9cc3fab39a85f6dc7dd2ab1697a24bd1d5f7b9fda3df732` |
| Active itemstatcost.txt | `b0ce483fe83a9d8d9114c92b655500128eeba172fc3fc8bb35c5b58ef07dfb8d` |
| Active properties.txt | `716ac8a45aec2393f72baf6220a4786f706ec95905cc41669da89dccc780cefb` |
| Active misc.txt | `3f2433064c19bb853190c265bb4ef36787ede78421ce9045e8c769c0524abfc0` |

Through beta.14, admission hashed the actual loaded executable/Core files and the active loose
tables below `mods/Reimagined/Reimagined.mpq/data/global/excel`. It also requires
`activeMod == Reimagined`. Beta.15 keeps native artifact admission and replaces
the mod-name/file-data gates with the compiled contract below. A matching
native version string alone remains insufficient.
The on-disk D2R.exe is packed and is not a usable source of runtime bytes at
these main-module RVAs.

### Runtime data discovery (beta.15)

The current plugin has no active-mod-name, mod-path or loose-TXT dependency.
Native artifact/byte/ABI/ownership guards are unchanged. Register a public SDK
DataTablesLoaded listener during Load, on client and headless host. Its game-thread
callback copies definitions from available Classic/Lod/Rotw banks. Unsupported
or missing definitions leave that bag inactive; a stock game receives no new
bag, recipe or conversion path. Tables and names need not come from Reimagined.

The adopted compiled layouts are Items 0x1C0, ItemTypes 0xE8, Gems 0xC0,
Properties 0x30, ItemStatCost 0x144 and CubeMain 0x16C. Existing Item Roll Ranges
and rune-downgrade records establish their earlier fields. Items
name-string WORD +0xFC is corroborated in the October 21 read-only capture:
gmm class 692 contains 51371, agreeing with active item-names.json id/key gmm;
bag class 746 contains 50077, agreeing with its bag key. These are observations,
not required classes/IDs. Native code admission protects the adopted layouts.
See [data contract](data-contract.json) for the field and bound index.

Beta.16 corrects beta.15's misread offset +0xBC: that field actually contains
FFFF in these gem rows, not the localized name. The complete captured gmm row
is retained in `tests/recorded_data.h` (SHA-256
`1d1612f77fb98bfc9ffb2ee6ecd111984c052d7b5d1c870b051886fa3c522361`).
It puts AB C8 at +0xFC, matching native-source item-names.json id51371/key gmm;
gmr has id51376 and gmo id51517. The adjacent +0xFE WORD is version, e.g.100
for the bag, so this is a two-byte reference, not a DWORD. This is compiled
data/source correlation, not a newly qualified native function or full layout.
The earlier +0xBC claim is rejected. Replays assert actual names, rather than
using only a synthetic fixture with the same mistaken field as production.

Discover exact-item two-input op17/threshold0 recipes. The first input is the
bag; output A is usetype,mod with one certain -1 credit property, output B returns
the grabber and output C creates one normal gem. Require no class, ladder,
difficulty, season, level, additional-input or additional-effect restrictions;
only normal quantity defaults/one are accepted. The full compiled recipe shape
is qualified, including unused modifiers and output fields. Classic cannot
admit a version100 recipe. A changed cost, random effect, ambiguous choice or
mixed credit model rejects that bag. This adapter does not infer arbitrary
transmutation semantics.

The referenced property must be a single function17 targeting the recipe's
credit stat, with other functions absent. The credit stat must retain the
observed unsigned, unshifted, flat 16-bit save/send encoding. Its ID is discovered,
not fixed at 386. Display descriptions/groups do not determine admission.
Opcode and operand-shift are individual bytes at +0x50/+0x51; the following
WORD can be FFFF for an unused target. Testing a DWORD at +0x50 incorrectly
rejects the observed flat stat, so do not conflate those fields.

Resolve unique 1x1 bag/grabber/output item identities. An output must have a
unique Gems definition and unambiguous bounded ItemTypes ancestry for gema,
gemt, gems, geme, gemd, gemr, gemz or gemo. One recipe per family is allowed;
missing optional choices do not disable the others. Codes, item classes, recipe
positions, stat/property IDs and localized string IDs may change independently.
The eight family slots are the present UI contract; other conversion models
or gem families are unsupported and must not be guessed.

Store only owned values and copied row witnesses. Recheck table ABI, counts,
revision and witnesses on the actual game callback before read/debit/refund.
The host binds a fingerprint of its definition to the bag lease. A changed
fingerprint refuses the pending withdrawal before native mutation; the client
also requires a matching local definition before enabling the picker. Replies
now include bank, mask and fingerprint: 224 bytes, request unchanged at72,
channel compatibility token v2. Current peers must match version/scope/flags.
No client pointer, stat ID, output code or debit amount authorizes host mutation.

Beta.16 excludes only the two Items name-reference bytes (+0xFC/+0xFD), and the
derived Choice.nameId, from the cross-peer fingerprint. Each client resolves
its own names; the headless host never queries localization. All other witness
bytes, including adjacent version and gem effects, remain in the fingerprint.
This is not a remote checksum of every possible language-dependent field.
Local revalidation still compares every copied byte, including name references,
before native reads/debit/refund. A local name-reference change thus requires
fresh table discovery; it cannot bypass the original witness check. Recipe,
stat, item identity, bank, ownership, storage, creation and compensation checks
are unchanged. The wire sizes/token remain v2; the Shared version gate requires
beta.16 on both peers because definition fingerprint semantics changed.

Both selector variants and their controller layouts exist during Load (two
panels/four resources). The portable variant uses stock art, with a common
fallback for an unfamiliar eighth-family code; the known gmo definition keeps
the existing Onyx art. Unsupported choices are hidden and cannot send a
withdrawal. Both variants retain the same measured geometry and all eight
reserved label buffers. A UI callback selects only a fixed owned panel name.
Localized labels use the discovered compiled item string ID. Item color token
filtering and all-eight buffer preflight remain unchanged.
The public LocalizationService v2 numeric API is used only for presentation.
Its documented original/native-ID limit remains: a custom namespaced table
reference that this API cannot resolve leaves labels hidden, with its index,
ID, SDK result and required length in the first warning. No item-code-to-key
guess, mod-path dependency, private string lookup or host-side translation is
introduced. Such a failure does not block icons or authoritative withdrawal.
Static ID-lookup provenance on the pinned Core: public wrapper 0x44B150 calls
0x44B1F0. The latter rejects a null context or ID >=0x10000 at
0x44B201..0x44B219 (`4885C90F94C181FA00000100410F93C04108C8`), enters the
existing owner lease at 0x485C70, and selects a cached asset through 0x38BBE0
with R8D=5. Its selector comes through Core+0x704098, and the 16-byte cache
entries start at Core+0x7DC080. At 0x44B26D it indexes an eight-byte pointer
array at returned asset+0x50 by the supplied ID, then uses the existing copied
output helper 0x44B310. These are bounded static observations, not a complete
cache ABI or proof that arbitrary namespaced temporary references resolve.
No private entry or cache address is called or read by the plugin. The raw
function disassembly is retained with the beta.16 ignored build evidence.
Previous written strings are tracked separately for each owned panel. An
eight-gem -> portable -> eight-gem switch must not use the other widget tree's
prior string contents as a buffer-ownership witness; the generic lifecycle
suite checks a round trip between both cached panels.

The new catalog and generic-picker suites cover relocated/renamed bag classes,
stat/property/output IDs, seven-gem data, vanilla inactivity, restricted/random/
extra-effect refusal, duplicates, changed revisions/counts/witnesses, localized
IDs and mismatched host data. Native creation has128 original/relocated cases
with SDK/setter/publisher substitutes. RPC checks definition changes between
lease and withdrawal. Eleven automated suites pass. The actual current normal
package's captured compiled tables replay successfully: no Classic bags from
version100 recipes; one eight-gem bag in each Lod/Rotw bank. Generated tables,
capture hashes and replay logs are retained in the ignored scratch workspace.
The user reports beta.15's normal-package UI and withdrawals work. Its current
log independently records successful inspection and Chaos Onyx/Topaz requests
(10 ->9 ->8 credits), but names were hidden. These are user/log observations;
no new controlled inventory comparison or persistence check qualifies beta.15.
Beta.16 offline labels are now user-confirmed in the current language. PID55316
loaded the global DLL with SHA-256
`254342603ebea250ee53255bdca71738f6af8b5387a2b19dacd9951d39e12797`;
the 2026-10-21 10:54:39.479 log records successful plain-name buffer writes,
with no name warning. A later Diamond request/reply reports 8 ->7 credits. A
read-only post-observation finds bag55 at Cube(11,7), seven credits, and gmd58
at Cube(0,0) on both client and local server. No pre-click item snapshot was
taken; this is not a conservation/novelty comparison, identification-flag check,
persistence test or separate-host regression. Other languages remain untested.
Its owned lifecycle tests use separate host/client name IDs, UTF-8 client text,
continued host routing and actual gem-effect mismatch refusal. Headless Load
also discovers/rechecks definitions without any localization service. Those
substitutes do not establish live hosted behavior. Beta.13 evidence below
retains its original limited scope.

### Startup diagnostics (beta.14, historical package gates)

The previous `Unsupported executable/provider` warning covered every false
return from Native::Initialize, including table hashes and active-mod name.
Beta.14 names those refusals under `[GemBag/Admission]` without changing the
required artifacts, four table digests, native witnesses, or execution rules.
Executable/Core/mod checks form the first group. Only if all pass are the four
loose tables hashed; only if all four pass are the existing native checks used.
Every failure in a group is logged once during startup. A final warning points
to these entries and leaves the selector inert and normal recipes untouched.

Changed files report expected and actual SHA-256. Read failures report the
open/read/crypto stage, captured Windows error and NTSTATUS, without inventing
an actual digest. Module path failure and missing module are separate cases.
Labels are the artifact name or relative table path, not the user's full
installation path. Active-mod refusal includes expected/actual names. Native
memory failures include the named witness/RVA and error; changed bytes include
first mismatch offset and bounded 16-byte expected/actual previews. If the SDK
expected-byte check refuses while the observed bytes match, the refusal is
retained and explicitly reported. Matching raw bytes never override the SDK.
SetWideListStat export absence or unexpected target also gets a named refusal.
No new native path/RVA or compatibility profile is adopted by this change.

The nine-suite build includes a new diagnostic fixture using real file/module
hashing, missing and exclusively locked files, newline-only change, grouped
errors, owned native bytes and retained SDK refusal. Actual Native::Initialize
is exercised in an unsupported process to verify the environment refusals and
inert result. These checks are not real game loading or non-Ladder admission.

On October 21, read-only disk checks of the current local Reimagined folder
(modinfo version 3.0.12) found the executable/Core hashes still matching above
and all four loose table hashes different:

| File | Current local SHA-256 (unqualified) |
| --- | --- |
| cubemain.txt | 01b13f03d1c8f99b1bab0708ca6b187ce8706754ea80ffe4cff38bdce1d852b3 |
| itemstatcost.txt | 4844ada3ecfc881f3b27243c4349b36fd632fd2bf576ff16a1105244404c3b04 |
| properties.txt | ecce4ec29dd124148a168c29c82a850d5bae3d5afff0ceef9b4408e0edaf5cc6 |
| misc.txt | 42362cca8a038edc5bf2dd7bf6725dd45a64decea6c5b36260f7f0f0fd6ece74 |

The active Reimagined folder contains only Reimagined.mpq and no mod-local
loader. At the user's subsequent request, beta.14 was rebuilt to allow global
installation: its flags are Shared | NativeHooks, with only ModScopedOnly
removed. Active-mod/artifact/table/native admission and the provider ABI remain
unchanged. No new native addresses are involved. The artifact test rejects a
ModScopedOnly regression, and inert-load plus owned client/headless lifecycle
fixtures explicitly use Global scope. These fixtures do not qualify real global
loader admission, panel resources or multiplayer peer scope matching.

With no D2R/D2RLoader process running, beta.14 replaced the global
`d2rloader/plugins/Gem Bag UI.dll`, whose existing beta.13 hash matched the prior
validated artifact. Both build and installed beta.14 SHA-256 are
`325abae0cebc5174ac470a5f879e1e1d28c25d902fe99fc5297236cfd34288bc`.
The previous DLL, 15 global configuration files and 202 ReimaginedThree save
files are backed up under the ignored `outputs/backups/` workspace. Manifest
comparisons verify that other plugins, configuration and saves were preserved.
No mod-local directory was created. All nine suites pass; beta.14 remains
untested in a real game and these table contents remain unqualified. See
`deployment-beta14-global-install.json` in the ignored scratch workspace for
the concrete deployment receipt. Historical beta.13 scope statements below
describe that build's ModScopedOnly metadata, not current beta.14.

The subsequent October 21 08:56:59 startup log confirms beta.14 loaded globally
with `-mod Reimagined -txt`. Executable/Core/active-mod admission passed. All four
loose-table hashes refused, exactly matching the on-disk values above; native
byte checks were not reached and the selector remained disabled. A read-only
comparison against the exact pinned Reimagined-bak files found equal headers
and complete matching rows for all eight withdrawal recipes, stat stacked_gem,
property stacked/gem, the bag and eight output items. Recipe lines move from
1159-1166 to 1223-1230; the other selected row positions match. All four files
still differ after line-ending normalization because other definitions changed.
This supports investigating a separate normal-package profile, not admitting it
or claiming native/compiled-table compatibility. Logs and the bounded comparison
are retained as `beta14-nonladder-startup-audit.json` and prefixed log copies in
the ignored scratch workspace. Separate Charm Weight: 1/2/3 namespace errors in
automagic.txt occur later in loader compilation and did not cause this refusal.
No game file, plugin, configuration or save was changed during this audit.

Original live read-only evidence is under the ignored workspace
`outputs/scratch/gem-bag-ui-2026-10-20/`: `before.json`, `after.json`,
`final-tool-smoke.json` and `verification.json`. The user performed a normal
Ruby grabber transmute: both client/server bag ID 74, class 746, changed from
119 to 118 credits; the old grabber was replaced and Ruby class 700 was created.
This proves the original recipe, not the new plugin's native mutations.

Private Core bodies were disassembled from the pinned PE with pefile/Capstone.
Main controller bytes came from the earlier mapped read. The full publisher
witness was retained from the older exact-provider Autosort experiment at
`autosort-probe/src/sync_profile.h`.
The current plugin must match that complete witness at runtime before using it.
`src/profile.h` stores exact adopted byte windows, indexed by
[profile-witnesses.json](profile-witnesses.json); imported Controller QOL
profile headers retain guards for its established helper contracts.

## Recipe and compiled table binding

The eight active withdrawal rows use operation 17 / parameter 386, positive
credit, input `bag` and one grabber. They keep the bag with `stacked/gem -1`,
recreate the grabber and output one gem. Credits are shared across colors.

| Grabber | Output | Button |
| --- | --- | --- |
| agr | gmm | Amethyst |
| tgr | gmt | Topaz |
| sgr | gms | Sapphire |
| egr | gme | Emerald |
| mgr | gmd | Diamond |
| rgr | gmr | Ruby |
| kgr | gmk | Skull |
| ogr | gmo | Chaos Onyx |

Stat `stacked_gem` is ItemStatCost ID 386 (16-bit saved/sent value). Property
`stacked/gem` is Properties ID 396, function 17, stat 386. Before debit the active
bag bank's compiled property row must still have function 17 at +0x18 and u16
stat 386 at +0x20, row stride 0x30. The selected Items row must have stride
0x1C0 and dimensions 1x1 at +0x11E/+0x11F, with the same table revision.
Bank values 1/2/3 mean Classic/Lod/Rotw. Compiled rows are game-thread-only and
are not retained. The controller prefilter's class 746 is specific to these
hashed tables; the queued UI action additionally resolves the SDK item code.

## Owned bag stat and storage observations

These are qualified fields, not a complete Unit/Inventory/StatList definition.

| Object | Field | Qualification |
| --- | --- | --- |
| Item Unit | type/class/id at +0/+4/+8 | type 4, SDK identity |
| Item Unit | ItemData +0x10; stats +0x88; bank byte +0x1BD | bounded reads, bank 1..3 |
| Item Unit | game +0xD8 | active authoritative game for publication |
| ItemData | inventory page byte +0x55; owner inventory +0xA0 | same SDK page, owner grid contains this item |
| ItemData | next owned inventory item +0xB0 | read-only capture traversal only |
| Inventory | magic +0; owner unit +8 | 0x01020304, same owner's selected inventory |
| Inventory | grid vector +0x20; u32 count +0x28 | count <=32, page+2 in bounds |
| Inventory grid | stride 32; width/height bytes +0x10/+0x11; cell pointers +0x18 | dimensions 1..16; bag pointer at SDK x/y |
| Owner Unit | control +0xA0; ordinary/alternate inventory +0x90/+0x98 | alternate when control and its u32 ref count at +8 are nonzero |
| Owner Unit | type +0 and game +0xD8 | player type 0, same active game |
| Aggregate stats | flags +0x1C; owner +0xA0 | negative flags, owner equals bag |
| Aggregate stats | primary vector/count +0x30/+0x38; aggregate +0xA8/+0xB0 | credit primary zero; aggregate total 0..65535 |
| Aggregate stats | first modifier child +0x90 | bounded, acyclic child walk |
| Modifier list | next +0x68; parent +0x78; owner +0 | parent aggregate and owner bag |
| Modifier list | expiry i32 +0x18; flags +0x1C; state +0x20 | expiry -1, flags 0x40, state 0 |

The live bag in the carried Cube qualified a 12x8 grid with 93 empty cells and
one bag cell. Other container layouts currently have structural/synthetic
coverage only. Free space is read from this exact owning grid, including shared
fake-owner inventories, rather than guessed from the visible tab or counted
from ordinary player items. Embedded Cube views use the same carried Cube.

Wide stat records are 16 bytes: u32 layer, u32 stat ID, i32 value, u32 padding.
Vectors are bounded to 511 records, stat IDs below 512 and layers <=65535, with
strictly increasing `(id<<32)|layer` keys. A positive total must come entirely
from exactly one owned modifier child. Mutation rechecks its ordinal and FNV-1a
64 fingerprint of every non-credit record. Only the credit entry changes.

Setting the final credit to zero removes that entry. Refund must refind the
same owned child by ordinal/metadata/fingerprint even if unrelated modifiers
remain; an empty-list guess is insufficient. Synthetic tests caught and fixed
a missing-source refusal in this last-credit path. No native item/stat pointer
survives the SDK `editNativeItem` callback.

## Setter, publisher and rejected SDK refresh

| Module | RVA | Contract and guard |
| --- | --- | --- |
| Core | 0x633460 | ItemService v1 table, 88 bytes in this provider |
| Core | 0x422380/3C0/400/440/480/4C0/500/580 | public info/create/edit/destroy/exchange/native-edit/existing-transaction/split entries |
| Core | 0x831ED0 | exported `SetWideListStat`, 160-byte witness |
| Core | 0x3D95C0 | inner sorted-list setter, 0x2D0-byte witness including surrounding boundary |
| Core | 0x3D8C32 | aggregate/primary vector selection witness |
| Core | 0x424FD0 | normal edit's mutation/publication body, 0x29F-byte witness |
| Core | 0x429600 | existing move publication, complete 227-byte witness |
| Core | 0x6FFDA8/+8 | native publisher pointer and target RVA; must equal main+0x535F60 and 0x535F60 |
| Main | 0x535F60 | full stored-item publisher, complete 1194-byte witness |

Adopted setter ABI, derived from pinned callers: Windows x64
`bool(bankByte, statList*, statId_i32, value_i32, layer_u32)`. The wrapper builds
the bank context, packs stat/layer and calls the inner setter. The inner updates
the modifier record and propagates its delta to the parent aggregate. Call it
only on the qualified contributing child; read back both contribution and total.
The return alone is not success. Every mutation rechecks setter ownership bytes.

Publication uses the nine-argument ABI recovered from both Core callers:
`void(game*, owner*, item*, flags_u32, mode_u32, extra_u32, page_u8, body_u8,
position_u32)`. A stored refresh uses flags 1, mode 2, extra 0, actual page,
body 0, position `(y<<16)|x`. The owner is `bag.ItemData.ownerInventory.owner`,
which may be a shared-stash fake owner, not automatically the local player.
Verify the owner inventory, owner/item/active game and full publisher/import
witnesses before debit and again before dispatch. Re-read bag state after the
call. This does not establish client packet delivery; that requires live reads.
No publisher hook or import replacement is installed.

A no-change Identified edit was considered as an SDK refresh. Rejected by static
analysis: Core 0x424A80 compares ItemData.ownerInventory against the main player's
inventory at 0x424BA2; the existing transaction also sends non-Move operations
through 0x424CB0 at 0x427875. Both reject a shared owner. The implementation uses
the guarded stored publisher instead. Core 0x42A140 stages actual shared Move
owners and 0x429600 publishes with them; this informed the owner contract.

Other inspected but unused exports: SetWideUnitStat 0x832030, AddWideUnitStat
0x8320D0, SendWideItemStatUpdate 0x834090 and AssignAndSend 0x834160. The unit
setters target primary state. The stat sender's publication/recipient ABI was
not qualified; do not substitute a guessed client pointer.

SDK `createItem` performs exact placement and owns the output handle. The
plugin's compensating operation is not atomic SDK arbitrary-stat debit. Failed
creation refunds only on a known clean failure. After a later failure it removes
the output before refunding. An unremoved/uncertain output never gets a refund;
withdrawals latch off instead. Shared-stash destruction on a failure remains a
live qualification limit. The normal successful shared path does not require it.

## Input integration and lifetime

Controller QOL may own main 0x13EDD0. This plugin never hooks or overwrites that
entry. Instead it installs an SDK-owned inline hook over the first 18 complete
instruction bytes at the distinct caller 0x13B350 after matching the existing
full `QolNativeProfile::EventCallerBytes` witness. SDK trampoline call-through
handles original events; unknown caller bytes disable controller support.

Caller ABI is `bool(event*)`. Event fields are device/source u32 +0xC, raw key
u32 +0x10 and pressed byte +0x14. Device helper 0x13E450 maps source to native
index (96-byte prefix guard). Active index main+0x2A23704 must be <8. Helper
0x13CE90 returns the input object (existing complete profile); 0x13D080 normalizes
the raw key (61-byte guard). The original caller performs these mappings before
calling 0x13EDD0. Keys used: LB 0x100, B 0x2000, A 0x1000, D-pad 1/2/4/8.
Other unowned events pass through once; modal presses own their release.

Manager main+0x3440170, controller mode u32 +0xDC ==1; current focus +0xD0;
focus +0x190 gives inventory grid. The grid's vtable+0xC8 must equal guarded
main+0x2C49F0. Selected cell +0x544 is passed to that lookup; resulting type/id/
class are matched to an SDK enumerated `bag`, including shared-page opt-in.
This avoids calling 0x36EF50, whose existing detour was observed but not qualified.

Cursor helpers main 0x14F1E0 and 0x14F210 and UI mode helper 0xCE500 use existing
complete profile guards. Opening requires no held item, allowed cursor mode
0/7/8/9, no configuration mode 0xB and no trade mode 0x17. SDK cursor emptiness
is also checked before authoritative work. No network sender or remote mutation
path is added.

Mouse activation uses a thread-local WH_GETMESSAGE hook and a fresh SDK tooltip
item snapshot. Plain right-button down/up is owned only within 400 ms and four
screen pixels of the last matching bag tooltip; a queued UI callback rechecks
the item identity/position. This follows the existing Identify On Click source
in `d2rl-plugins/plugins/identify-on-click/`; no native hover lookup is needed.
The SDK semantic ItemInteraction listener remains a fallback, with the four
storage bits and explicit SharedStash opt-in. Beta.1 depended on that semantic
activation, and the user observed no popup. Its logs did not establish which
input gate rejected the attempt, so beta.2 adds bounded stage diagnostics.
Keyboard
and controller hooks enqueue SDK UI callbacks for all panel/widget operations.
Only the game queue calls native item mutation. Panel messages require the
exact namespaced target/command/payload. Clicks and queued work are serialized,
session/selection generations invalidate stale work, and unsupported hooks fail
open. No live Controller QOL combination has yet been exercised for this DLL.

Beta.3's assumed centered 2160-height conversion failed the user's mouse check.
Its automated ratio cases only exercised that assumption. Beta.4 removes it.
Down/up ownership remains; client points are now queued to native containment
on the UI thread. Close, outside dismissal and gem selection share that queue.

Core getItemInfo public 0x422380 forwards through 0x422600 to 0x41D0C0, which
uses the ItemInfo builder at 0x41E1F0. Static inspection of that chain found no
reason to treat the returned handle as an authoritative handle on the UI side;
the implementation continues to resolve each domain separately. These bodies
are provenance only; the plugin calls the public SDK entry, not those RVAs.

Before raw native edits, game work re-enumerates the requested container/page/
cell and requires exactly one matching code, class and quality. UI handles and
runtime IDs identify client copies and are not treated as authoritative native
handles. UI identity/position is separately rechecked before the request, and
raw server reads/writes then use the fresh authoritative handle. Item level is
not used across those copies because clients cap it at 99.

## Native popup geometry and rendering (beta.4)

The existing Inventory Deposit geometry adapter supplied FindPanel, bounded
child-vector, local rectangle, SetX/SetY and containment evidence. These are
reused from its `gold_anchor_profile.h` and Controller QOL's existing profiles;
those production plugins were not edited. The new complete origin/scale
witnesses in `src/geometry_profile.h` came from reviewed code-only capture
`inputs/captures/runtime-game-auto-deposit-20260927.exe`, SHA-256
`02c32a05f182c3f1f1b29d58b2b8da735eadc080b5e13acf8ea721a0648ce60d`.
[geometry-witnesses.json](geometry-witnesses.json) indexes them. Runtime still
requires the installed host/Core/table hashes at the start of this document.

| Main RVA | Adopted contract |
| --- | --- |
| 0x846170 | Existing FindPanel(name), complete existing entry witness; manager lookup tail at 0x89F760 |
| 0x856220 | Complete existing FindChild witness qualifies name +8, parent +0x30, child vector/count +0x58/+0x60; this adapter performs its own bounded unique-child search |
| 0x8562A0 | Existing complete 73-byte local rectangle getter, RCX widget, RDX caller-owned four-int rectangle |
| 0x8562F0 | Complete 391-byte origin body, `POINT* (widget*, POINT*)`; walks local rectangles/anchors/scales and returns caller-owned integer x/y |
| 0x1E6750 | Complete 59-byte recursive ancestor-scale body, `float(widget*)` in XMM0; reads local float +0x80 and parent +0x30 |
| 0x1CBA904 | Scale helper's leaf constant, separately requires IEEE float 1.0 |
| 0x857570 / 0x8575C0 | Existing complete SetX/SetY guards; `void(widget*, int32)`; only plugin-owned Card is moved, with fill-parent +0x52 required zero |
| 0x8566B0 | Existing complete 313-byte containment body; `bool(widget*, uint64 packedXY)`, low signed int32 x and high signed int32 y, result AL |

Containment saves RDX by value, compares EBX as x and the saved upper DWORD as
y, and calls origin at 0x85679D. It consumes current local width/height, ancestor
scale and extra edges, with inclusive left/top and exclusive right/bottom.
Each actual Card/row/CloseHit vtable slot +0x40 must point to this exact body.
The adapter requires zero extra edges +0x38/+0x3C/+0x40/+0x44, zero anchors
+0x48/+0x4C, local scale 1 and the correct direct parent for its own controls.
SDK rectangles must match the stored native +0x70 fields; native effective
rectangles are separately validated against the admitted getter/inheritance.
Plain Card and icon rows must match both representations, and icon rows must
additionally match the layout. Handles are never reinterpreted as native
addresses. See the fill-parent correction below.

Before origin/scale calls, the root ancestor chain is finite, acyclic and at
most 32 nodes, with bounded finite scales/anchors and valid fill-parent links.
Child search is bounded to 128 nonnull direct children and requires a unique
name. These bounds are adapter policies, not discovered engine capacities.
The native origin body invokes the existing float rounding helper 0x12DEA28;
that CRT-like helper was inspected but is not hooked or independently invoked.

Mouse opening captures the actual message's screen point after validating the
fresh bag hover within four pixels, converts to current game-window client
pixels, then targets eight pixels below/right of it. The
measured Card origin and scale convert this to parent-local units; native
setters are followed by SDK/native read-back and final screen containment.
Eight-pixel edge margins keep the popup visible. Controller placement currently
uses the window center. New clicks bind the tree again and invoke native
containment; neither native pointers nor a resolution-based hit map are cached.
Code ownership or object mismatch closes/refuses only the optional picker.

Core WidgetService table 0x637F70 (64 bytes) has public entries 0x495270/2B0/2F0/
330/370/3B0/3F0. Static provenance for `findPanel`: 0x495270 -> 0x495440, normalized
name then import 0x7034D8, whose adjacent RVA is main 0x89F760. `getWidgetRect`
0x4952F0 -> 0x4969F0. These private bodies are not directly called. An initial
attempt to start disassembly at 0x496730/750 landed inside unrelated code and
supplied no contract; the service table, not that guessed start, identifies
the public paths.

The eight ImageWidgets reference existing `items/misc/gem/perfect_*` sprites;
the exact filenames follow active `data/hd/items/items.json`, including the
stock spelling `perfect_saphire` and Reimagined `perfect_chaos_onyx`. No game
artwork is copied into source or the DLL. TextBox styles use byte RGBA objects
as in `_profilehd.json`; beta.3 incorrectly supplied normalized arrays. Native
widgets now own all text, icons and disabled shading. The SDK overlay callback
only schedules periodic UI refresh, with first-frame diagnostics. Visual
alignment, sprite resolution, and real mouse messages remain live test items.

### Fill-parent correction and live geometry (beta.5)

Beta.4's log showed right-click -> native panel opened -> geometry refused,
before SetX/SetY or authoritative inspection. The user saw no popup. A cached
widget capture from PID 43636 verifies the exact loaded beta.4 DLL SHA-256
`061a728793d73b89facfac789cf827bb5c18ac43f16df80aadb032e0209f2d2c` and all
adopted geometry/lookup witnesses. `live-beta4-widget.json` records:

| Native widget | Stored rectangle | Scale / fill-parent | Vtable RVA |
| --- | --- | --- | --- |
| PanelManager, no parent | (0,0,3840,2160) | ⅓ / 0 | 0x1D79CD0 |
| gem-bag-ui/Selector | (0,0,0,0) | 1 / 1 | 0x1D738E8 |
| Card, direct selector child | (0,0,426,250) | 1 / 0 | 0x1D72670 |
| Row0..7 and CloseHit | Exact layout rectangles | 1 / 0 | 0x1D72670 |
| Gem0..7 ImageWidgets | Exact layout rectangles | 1 / 0 | 0x1CE40E0 |

Each captured vtable slot +0x40 equals main 0x8566B0. Card has 49 immediate
children; all inspected anchors and extra edges are zero. Selector is disabled
and hidden after the refusal closes it; Card retains its local visible flag.
These vtable identities are observations for this build, not cross-build IDs.

The specific rejection was `Rect(root)`: Core SDK `getWidgetRect` at 0x4969F0
resolves its handle at 0x496A32 and copies the raw four integers from native
+0x70 at 0x496A42..0x496A46, including the stored zero rectangle. Main native
GetRect 0x8562A0 takes the fill-parent +0x52 branch, recursively obtains the
parent rectangle, then synthesizes x/y zero with inherited width/height.
Requiring those two results to be equal was incorrect for the selector.

Beta.5 binds SDK to stored +0x70 first, checks stored values/bounds, then checks
the native effective getter. For fill-parent roots it requires a valid parent,
effective zero x/y, and dimensions equal to the parent's native effective
rectangle. For plain widgets it requires stored/effective equality. Card still
requires fill-parent zero before any setter. Code guards, parent/type checks,
unique lookup, ancestry bounds and final viewport containment remain enforced.
No native panel or item pointer is retained between callbacks.

The geometry regression now reproduces all three levels above, including the
raw zero selector and ⅓ manager scale; the prior fixture used a non-fill root
and therefore missed this SDK/native difference. It also rejects a fake SDK
rectangle containing the effective dimensions when the stored fields are zero.

Read-only diagnostic `tools/capture_gem_bag_widget.py` qualifies main FindPanel
entry 0x846170/0x16, manager search window 0x89F7F0/0xB5 and FindChild
0x856220/0x7E against the reviewed capture. The search window proves manager
active panel vector/count +0x88/+0x90, secondary vector/count +0xA0/+0xA8, each
eight-byte pointers, and immediate-child fallback through FindChild. The tool
reads those and +0x58/+0x60 with a 1024-node manager limit, then limits the
owned panel/card child vectors to 128 and ancestry to 32. These are diagnostic
bounds. It opens only query/read access, takes sequential stability checks for
vectors, and makes no game calls, writes, debugger attachments or suspension.

`live-beta4-before.json` independently confirms the restarted offline bag ID
146, class 746, 109 credits on client and server, in Cube cell (9,0), with the
nine previously withdrawn gem outputs and 86 free cells. This is a new-session
baseline, not a cross-session runtime-ID comparison. UI refusal did not log a
new withdrawal. Beta.5 geometry/appearance and mouse delivery remain live test
items.

### Beta.5 visible picker and first mouse withdrawals

PID 2012 loaded exact beta.5 SHA-256
`5c07b7d0295a864d6b25de6ed7b74caa2e0ac1b819e31615194e78583cb8ee12`.
The user confirmed the icon picker looks correct. Its log records click point
(433,157), final native origin (441,165), scale ⅓ and Win32 client dimensions
1280x697. `live-beta5-widget.json` binds the cached Card local rectangle
(1323,495,426,250), confirming the applied parent-local position. The picker is
about 142 by 83 client pixels at this scale; this is one live window, not general
DPI/resolution qualification.

The user also clicked Amethyst at client (460,178) and Skull at (537,219).
Window-message, native selection and successful withdrawal logs agree for each;
the bag's initial inspection was 109 and the two results were 108 then 107.
`live-beta5-current.json` independently reads 107 from both client/server,
Amethyst ID 179 at Cube (10,0), Skull ID 180 at (11,0), and bag ID 146 still at
(9,0), with 84 free cells. The earlier full external snapshot is from beta.4's
previous process, so it is not treated as a strict same-session before/after
comparison. The comparison helper now explicitly requires matching process IDs.
A fresh beta.5 snapshot at 107 was captured for the next comparison.

A third left click at (502,111) is outside Card and closed the picker; the
external widget snapshot subsequently shows the root disabled/hidden. This
supports outside dismissal for that observed click, not the untested X/B/Escape
paths. All eight output recipes were already qualified through beta.2 keyboard
withdrawals; beta.5 mouse evidence currently covers Amethyst and Skull.

### Beta.5 four-location mouse run

The user continued the Ruby test across four locations and confirmed each.
The same PID 2012 logs this consecutive successful sequence:

| Output | SDK container / shared page | Resulting credits |
| --- | --- | --- |
| Ruby | Cube 5 / unknown sentinel | 106 |
| Diamond | Inventory 4 / unknown sentinel | 105 |
| Amethyst | PersonalStash 7 / unknown sentinel | 104 |
| Skull | SharedStash 8 / raw page 0 (UI page 1) | 103 |

`live-beta5-locations.json` independently confirms both client/server bag ID
146 at 103, now in inventory page 0 cell (4,2), a 10x8 owner grid and 45 free
cells. Client additions are Ruby ID 181/page 3, Diamond 182/page 0, Amethyst
183/page 4 and Skull 184/page 4. The ordinary authoritative player-inventory
capture contains the first three. It excludes the shared fake-owner inventory,
so authoritative Shared Skull coordinates are not independently captured. The
shared result is supported by native success/SharedStash page-0 logs, the client
item and the user's visible confirmation.

`live-beta5-locations-verified.json` audits the explicit four-location plan:
same process and exact beta.5 hash, 107 -> 103 on both sides, four client
additions, three ordinary server additions, and all other old sampled items
unchanged. It explicitly permits the user-reported bag relocation and checks
its identity, final page/cell and matching final client/server grid. It is not
the stricter single-grid comparison, and does not claim an unmoved bag across
the deliberate transfers or intermediate per-location coordinates. No isolated
Ruby-only after snapshot exists. Full item integrity and persistence remain
outside these sampled fields. The user has no controller available yet.

### Localized gem labels (beta.6)

Beta.5's visible hover names were hardcoded English. Beta.6 uses native layout
references `@d2r:gmm`, `gmt`, `gms`, `gme`, `gmd`, `gmr`, `gmk`, `gmo`, with the
`@d2r:` prefix on each complete key. Active Misc rows' `namestr` values equal
those eight codes. Active `data/local/lng/strings/item-names.json` defines all
eight, including mod-specific Chaos Onyx, translations and native color markup
(`ÿcU` in enUS). Labels therefore follow active game localization/mod overrides;
no copied translation catalog or language choice is introduced. English
`Gem::name` values remain diagnostics only; short status messages are English.

The SDK's `LOCALIZATION.md` and widget-localization example require explicit
namespaced references in plugin layouts and show `@d2r:strCancel`. A first-open
diagnostic optionally queries public LocalizationService version 2, size 24,
and calls its copied UTF-8 `getStringByKey` with `d2r:<code>`. It logs success
count only; buffers are owned, bounded to 1024 bytes and never escape. Missing
diagnostic service does not replace native rendering or authorize gameplay.
The session-left callback resets this diagnostic. No native widget text setter
or private localization call is introduced.

Pinned Core provenance: the Localization v2 service table at 0x635238 contains
public entries 0x44B150 (ID) and 0x44B1A0 (key), five-argument SDK ABI. The key
wrapper enters 0x44B3B0, passes its key through helper 0x44B610, invokes namespaced string
resolution at 0x387D60 and copied-output helper 0x44B310. These private bodies
are inspected provenance, not direct calls or portable bindings. Table version
and SDK query guards remain explicit. An earlier 24-byte v1 table at 0x637ED8
with wrappers 0x493E40/80 was a false lead: its wrappers inject R9 as 0/1 for
the thread queue; it does not establish a legacy localization ABI. Absence of
a contiguous `d2r:` literal in PE bytes likewise proves no API incompatibility.

Beta.6's four automated suites and layout schema checks include the eight exact
item-key references. Its 2026-10-20 18:09:18 log resolves 8/8 and opens the native
picker. The user screenshot shows `*UChaos Onyx`; controller controls feel good
and the user explicitly accepted the centered position. Controller inventory
withdrawal logs report Amethyst, Skull, Skull and credits 103 -> 100. No new
client/server inventory snapshot was completed: the read-only tool refused with
`No matching local client player`, and the process was closed by the later
check. These logs do not establish per-output persistence
or held-button behavior. Other-language rendering is not inferred from an
English game.

### Plain localized labels (beta.7)

The mod source `data/local/lng/strings/item-names.json` has 104 localized values
for these eight keys (13 languages); every value begins with source `ÿcU`.
The `U` selects a color and is separate from the translated gem name. Beta.6
passed those item-formatting instructions to plain TextBoxWidget text. The
user's screenshot establishes the visible failure; resolving a key successfully
does not establish that a widget interprets the resulting formatting.

Beta.7 uses the existing public LocalizationService v2, not another native
entry point. `src/labels.h` resolves each `d2r:<code>` into an owned 1024-byte
buffer, requires successful, nonempty, terminated copies, and removes recognized
color tokens. It accepts U+E07E (UTF-8 EE 81 BE) plus one ASCII selector, as
documented by the existing Item Roll Ranges `RangeText::Color` adapter, and the
UTF-8/legacy source `ÿc` forms. It removes repeated tokens, retains ordinary
Unicode bytes and incomplete/literal lookalikes, then JSON-escapes the names.
The schema and mock-service regression exercise the actual copied-name to layout
path, including German/CJK letters, missing/oversized/unterminated copies and
quotes/backslashes/control characters. This is not native rendering evidence.

Beta.7 required all eight labels at plugin load before resources, panel and
inputs were registered. Its 2026-10-20 18:23:34 log reported localized names
unavailable, returned inert, and the user observed no picker. Its earlier four
suites did not reproduce startup service availability. ResourceService copies
the generated layout bytes; no game catalog
or game string is modified. Each opening compares fresh plain localized names
to that layout's labels. A changed language/name requires restarting instead of
showing stale labels. This is a load-time language snapshot, not live language
switching. English names remain diagnostics only; short status messages remain
English. No private text setter, hook, native address, geometry guard, controller
placement or item mutation path changed in beta.7. Its plain-name rendering was
not reached in that live test.

### Deferred first-open labels (beta.8)

Startup still registers the panel, input listeners, lifecycle and periodic UI
refresh, but performs no localized-name reads and registers no layout bytes.
The public SDK requires panel registration during Load and documents native JSON
validation in Open/Toggle. On the first accepted UI open, all eight names are
copied and normalized, then both owned ResourceService layouts are registered
before `openPanel`. Unavailable names refuse just that open and can be retried.
A partial resource failure unregisters only returned owned handles; uncertain
cleanup latches layout setup off. Success retains the two layouts across future
opens and sessions, avoiding any assumption that re-registering can rebuild a
cached native selector. Fresh-name comparisons still reject a changed language
until restart. Position, input ownership, item policy and native mutation code
are unchanged.

Static provenance for active SDK resource setup uses the same pinned Core
artifact above. ResourceService ABI 1's 32-byte table at 0x6378F0 contains public
wrappers 0x48BCA0 (register), 0x48BCE0 (unregister) and 0x48BD20 (info). Register
enters 0x48BD60, whose registration layout, 64 MiB limit and bounded virtual-path
normalization match `resources.h`. It takes the common owner lease at 0x485C70
with mode zero; the inspected gate at 0x485D9F allows raw owner states 0 or 2,
rather than requiring only the loading state. No private call or new byte patch
is adopted. Other 32-byte/version-1 tables at 0x632BE8 and 0x633F18 were rejected
as the Resource table; size/version alone is not service identification.
Localization's existing key wrapper also enters that common lease with mode
zero, so beta.7's generic failure is not proof of a missing ABI or an inactive
owner. The observed correction is lookup timing; the precise original result
was not logged. These are bounded static provider observations, not a new
complete private ABI or live resource-registration claim.

The fifth suite's beta.8 form executed real Load/Open code with owned SDK
services and substituted native admission/geometry. It reproduced beta.7's
startup lookup failure, but its fake provider incorrectly treated late resource
registration as sufficient for native Open. No real provider resource code ran.

The actual beta.8 DLL (SHA-256
`a40ba3e8664d98bea4be590cc18c15571ae198dd4fff96196fc6f55204d00022`)
was read from PID 33840. At 18:47:40.238 its log prepared all eight plain names;
at .240 all eight game keys resolved; at .242 native Open returned result 5,
NotFound. Repeated attempts through 18:47:46 behaved the same, and the user
confirmed no picker. Both resources had returned registration Success. That
result therefore does not establish downstream native layout visibility after
Load. The exact internal cause is not asserted. A read-only item capture found
bag ID 153, class 746/code bag, at inventory page 0 cell (4,2), with 100 credits
on client and server and 42 free cells. No debit was observed. The original
failed-open log later rotated; this timing is a transcription of the tool read.
The later reference-session log is a separate artifact.

### Reserved native label buffers (beta.9)

Both static layouts are now published during Load before panel registration,
matching beta.6's working resource lifecycle. Each initially hidden Name0..7
TextBox contains a distinct 255-byte ASCII reserve plus NUL. Localization is
read after native Open; it is no longer an opening prerequisite. After the
native tree exists, `Geometry::SetLabels` qualifies all eight reserves and
replaces their text in place. Missing names or rejected text qualification
hide only the labels and log once. Input, geometry placement and the withdrawal
backend remain available. No resource is replaced after startup.

The read-only reference was the existing Buff HUD tree in PID 54084:
`buff-panel/BuffHud / BuffGrid / BuffSlot00,01 / Countdown`. Neither reference
widget was modified. Both had native TextBox vtable main+0x1CF4610, update slot
+0x10 = main+0x86E630, and Contains slot +0x40 = main+0x8566B0. Their external
heap strings were at widget+0x88, length +0x90 = 31, capacity/flags +0x98 = 31.
Both buffers contained the exact 32 bytes
`__BUFF_PANEL_TIMER_RESERVE_00__` plus NUL. Rectangles were (-16,52,112,42),
anchors/edges zero, scale 1, fill-parent zero, enabled 1 and visible 0.
The independent companion's existing countdown writer and BlizzardString
contract use these same pointer/length/capacity offsets. Reconcile native
children afresh because the game can rebuild them during input-mode changes.

Reference modules were D2RLoader 3.3.93787, image size 90451968, SHA-256
`93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10`,
and Core image size 19791872, SHA-256
`2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.
Session addresses are not persistent bindings. The new `src/text_profile.h`
records only the exact vtable/update identity, data offsets and object bound;
this is a data contract, not a new native-call ABI.

For future identification, the observed vtable slot:RVA pairs were:
0x00:0x2CE220, 0x08:0x8567F0, 0x10:0x86E630, 0x18:0x86D410,
0x20:0x856B00, 0x28:0x78D20, 0x30:0x856C70, 0x38:0x8569A0,
0x40:0x8566B0, 0x48:0x837D0, 0x50:0xF02A0, 0x58:0x86D950,
0x60:0x86C5D0, 0x68:0x2CE160, 0x70:0x2CF260, 0x78:0x2CF990.
Unadopted slots carry no callable-contract claim.

The bounded static witness was the existing code-only capture
`inputs/captures/runtime-game-auto-deposit-20260927.exe`, SHA-256
`02c32a05f182c3f1f1b29d58b2b8da735eadc080b5e13acf8ea721a0648ce60d`.
It contains no usable RTTI/vtable pages; attempting 0x1D726B0 did not establish
a type. The complete deleting destructor 0x2CE220..0x2CE27A passes size 0x230
at 0x2CE260, providing the object bound used to reject an embedded text buffer.
Update 0x86E630 calls 0x86E6A0 and checks geometry fields +0x228/+0x22C.
The latter gates on byte +0x1F4, reads text +0x88 at 0x86E78B and 0x86E85C,
and calls measurement helper 0x903CD0. Its branch writes width/height at
0x86E8D3/0x86E8D6. This suggests optional text sizing; it does not prove a
dirty flag. No write to +0x1F4 is adopted. Slot 0x58's 0x86D950 thunk enters
0x86D960, a type/registry getter, not an identified setter. The captured draw
body at 0x86D410 already contains a detour whose destination is unavailable in
that capture; it is not a pristine body and supplies no adopted draw call/hook.

The actual write admission binds the fresh owned panel/Card tree and all eight
named children through SDK/native agreement. Each child must have the exact
TextBox vtable and update slot, parent, zero edges/anchors, scale 1, no inherited
fill, and rect (8,218,380,26). Require capacity 255..16384 with no embedded flag,
length at most 255, a committed nonexecutable writable 256-byte external buffer,
and a writable eight-byte length field. Reject overlapping buffers or any
buffer intersecting its 0x230-byte native object. The entire 256 bytes and
length must match that child's unique untouched reserve or exact previous
plugin-written bytes. Ordinary text or a partial marker is insufficient.

All eight pass preflight before any write. A replacement is nonempty, contains
no embedded NUL and is at most 223 UTF-8 bytes. It replaces only the reserved
buffer and native length, retaining the unique tail marker. Data pointer,
capacity, style, flags and geometry are untouched and the string fields/bytes
are read back. No allocator, resize, private text setter, new hook or guessed
invalidation is introduced. Previous C++ strings may survive callbacks; native
addresses never do. A rebuilt child can qualify the original reserve, and a
cached child can qualify previous contents. Fresh names may replace previous
ones without resource publication; live game-language switching is unqualified.
An exception refuses labels only. This is not a general native-string writer.

Five suites pass for beta.9. The actual buffer adapter runs against eight owned
synthetic TextBoxes: it checks all-eight preflight, Unicode replacement,
reconstructed/cached markers, capacity/type/foreign-text/oversize refusal and
a buffer-plus-length-only mutation footprint. The lifecycle fake now models
native layout availability only for resources published during Load, based on
beta.8's observation. It verifies opening with unavailable names, label-writer
refusal, later names and changed names, without resource replacement. This
model does not reconstruct the provider's internal loader. No automated test
draws native text. The exact beta.9 DLL (SHA-256
`e771231a0db81e8c5ffcbedcdda554c3e8caa31dd2bd2874781efe5821e70b6a`)
was loaded in PID 50972. Its 19:16:12 log opened the picker, applied 8/8 names,
and placed it beside cursor (1437,592), origin (1445,600), scale 0.5569, client
1906x1203. The user confirmed the picker and plain names look good. At 19:16:36
the controller open retained centered placement; three inventory withdrawals
logged Chaos Onyx/Ruby/Diamond and credits 100 -> 97. The original marker/name
defect is visually resolved. Other-language rendering is not thereby qualified.

### Identified gem creation (beta.10)

The user subsequently reported unidentified gems from beta.9. The actual
`ItemCreateSpec` aggregate omitted `stateFlags`, making it zero. Normal quality
does not express identified state in this SDK contract. `item.h` exposes public
`ItemStateIdentified = 1 << 0` independently of quality, and the item-transaction
example explicitly supplies it. Beta.10 sets this field at creation. Successful
verification now requires both Normal quality and that identification bit in
the copied `ItemInfo`, in addition to the existing code/storage/cell/credit
checks. A wrong-quality or unidentified output follows the established
remove-output-before-refund compensation. No native identification write or
new function call is adopted. Existing test gems are not automatically edited.

The read-only flag witness uses the same exact host/Core artifacts above and
the code-only capture SHA-256
`02c32a05f182c3f1f1b29d58b2b8da735eadc080b5e13acf8ea721a0648ce60d`.
Native flag getter 0x36E2D0 checks item unit type 4, calls ItemData getter
0x34A500 and at 0x36E341 reads u32 [ItemData+0x18], then masks it with the
caller's value at 0x36E349. Its 132-byte witness SHA-256 is
`d208a1e97d3329659599976e0dc2a51bc4d0660287903328e0a6fb00963ee2f5`.
The getter's positive branch at 0x34A52B reads [Unit+0x10]; a bounded 48-byte
window from 0x34A500 has SHA-256
`2148c618bef2743d24d5625be344097e04b4a0d8bbb6eae4a590512bd5156570`.
That second window is not a complete-function callable witness. Existing
Controller QOL passes native mask 0x10 for identification. The native mask 0x10
and public SDK state mask 0x1 are distinct representations. Exact byte windows
are retained in [identification-witnesses.json](identification-witnesses.json).
They authorize bounded observation only, not an adopted native setter or ABI.

The new regression calls actual `Native::Execute`, including its creation
request, postconditions and compensation. It failed the prior source with
`every gem explicitly requests identified normal creation`. Its 64 cases cover
eight gem codes in four stores, each with a normal provider result and an
artificially unidentified result. Owned item/table fixtures, SDK calls, stat
setter and publication substitutes isolate the behavior; test configuration
exists only under `GEMBAG_NATIVE_TESTS`. Real artifact/byte admission is not
weakened in production. The tests verify exact identified Normal creation and
that an invalid output is removed before its credit is restored. They do not
execute D2R's setter or publisher.

Beta.10 SHA-256
`55c54ec24cdf893d92963b52b7179b016f04b902a0f06538115d49c8e82dc516`
was installed and loaded in PID 37516. A fresh-session read-only baseline found
bag ID 148 at inventory (4,2), 97 credits on client/server, free cells 39. It
also confirmed old gem identification bits clear in both domains, alongside
ordinary identified gems. The user then clicked Ruby and confirmed it appears
identified. Same-process `live-beta10-before.json`, `live-beta10-after.json`
and `live-beta10-verified.json` record exactly one new Ruby ID 190/code gmr at
(2,1), client/server credits 97 -> 96, free cells 39 -> 38, and the bag unmoved.
Its native identification bit is set on client (flags 0xA02011) and server
(0x82010). Other flag bits are not assigned semantics or equated across domains.
All other sampled records and old storage cells remain unchanged, including
old gems' identification flags. The earlier beta.9 capture attempt ended after
the active session disappeared; it did not produce a completed flag snapshot.

The 19:25:39 log reports the mouse Ruby selection and successful withdrawal;
the preceding native label application is 8/8. This qualifies identified output
for the one live inventory withdrawal, not save/rejoin or every storage. The
earlier all-eight/location audits measured identities, codes, credits and
placement but omitted identification; do not retroactively claim that field
was checked. The installed correction applies to all eight creation choices.

## Host request contract (beta.11 / beta.12)

The public SDK NetworkService ABI 1 has size/required end 64 bytes. Function
slots are registerChannel +8, unregisterChannel +16, getChannelInfo +24,
connectToHost +32, sendToHost +40, sendToClient +48 and getPeerPlayer +56.
ChannelRegistration is 56 bytes. Register during Load; connect after entry.
The SDK namespaces local channel 1 by plugin id and load scope. Both sides
must match compatibility token `0x47454D4241470001`; the maximum payload is
224 bytes. No private packet hook, networking RVA or guessed transport ABI is
adopted. The plugin role is Shared | ModScopedOnly | NativeHooks; host and
client require matching plugin identity/version/ABI/scope/flags. Shared plugins
participate in loader save history.

`protocol.h` fixes a 32-byte bag-location descriptor, 72-byte Request and
216-byte Reply (including a bounded 96-byte status string). Magic is
`0x31424752`, message ids are request 1/reply 2, and actions are Hello 1,
Inspect 2 and Withdraw 3. Exact-size copies validate all enums, storage pages,
gem indexes, bounds, reserved fields and string termination. The request carries
session nonces, sequence and a host-issued lease. It never carries a native
pointer, item handle, player identity, custom output code or chosen output cell.

Network callbacks copy values and schedule SDK callbacks. Only the scheduled
host game callback calls getPeerPlayer, then enumerates that resolved player's
requested storage. The host local player is never substituted. Inspection
records the host bag identity, seeds, handle and location behind a random
lease. Withdrawal resolves again, verifies that lease/identity, rechecks the
cursor, space and credits, and calls the unchanged guarded Native::Execute.
The output remains identified and Normal, with removal/refund for a known
invalid result. Native mutation and compensation are serialized on the host.
Client callbacks apply copied snapshots on the UI thread using generation,
presentation and intent tickets; they have no direct mutation fallback.

State is bounded to 64 peers, 64 queued jobs and 64 cached accepted requests per
peer. Per-peer high-water sequences reject evicted or altered replays; exact
duplicates receive the cached result without another debit. Disconnect removes
that peer's queued jobs. Stop unregisters the channel and serializes against
executing work. An eight-second unacknowledged withdrawal is never retried
automatically. Uncertain native results halt the resolved owner across peer
reconnects (1024 owner entries, then conservative global halt).

Beta.12 drains at most 64 copied jobs from each scheduled game callback. This
addresses the SDK's session-change callback discard: old jobs cannot strand a
new Hello behind callbacks that will never run. Each job still checks its
current peer/epoch and resolves the sending player. The regression discards
the old fake SDK callbacks, assigns a distinct new peer and verifies a fresh
inspection. Other peer isolation and uncertain-result protections still pass.

Windows CommandLineToArgvW selects host-only startup for an exact `-d2rds`
argument. That branch queries only item, inventory, thread and network services;
it registers no picker, localization, overlay or input handlers. Exact native
admission is retained, including Reimagined data hashes and the publisher's
item-game / active-game identity check at main +0x2A9E020. A dedicated multi-game
publisher remains unqualified; no guard is weakened to accommodate it.

Beta.11 SHA-256
`ac447ca93265dfb37846f5668621a85c3fe667a3c33889790d90556b1472f6a2`
was loaded in PID 33728. The real SDK loopback sent Hello then Inspect; the
19:55:32 host/client log pair reports peer 2, sequence 2, success and 95 credits.
`live-beta11-rpc-baseline.json` independently matches bag 148 at inventory (4,2),
95 credits and 37 free cells on both domains. It also records the exact loaded
module hash. No beta.11 withdrawal was requested in this validation session.
This establishes real offline host inspection, not remote sockets or D2RDS
mutation/publication/persistence.

Beta.12 SHA-256
`ba96b4d6e89981924975183432d2459c357bc7dc8055b63673df50b55ea831ec`
was installed with the prior DLL, saves and root/mod configurations backed up;
all 46 other mod plugin files were hash-preserved. PID 21088 loaded that exact
image. `live-beta12-before.json` found bag 148 at inventory (4,2), 94 credits and
36 free cells on both domains. This is a fresh-session baseline; do not infer
an exact item comparison across the earlier restart. The user then performed
the requested single Ruby click and replied Done.

`live-beta12-after.json` and `live-beta12-verified.json` prove one new Ruby
193/code gmr at inventory (5,1), 94 -> 93 credits and 36 -> 35 free cells in both
domains. The bag stayed at (4,2), old occupied cells and all other sampled item
records remained unchanged, including existing gem identification flags. The
new Ruby has identification bit 0x10 set on client (0xA02011) and server
(0x82010). Those other bits are not equated between domains. The 20:05:18 log
pair records host action 3, peer 2, sequence 3, gem 5 and success/93 credits,
followed by the matching UI reply. Both native Open and 8/8 label application
were logged; visible names were not separately reconfirmed in this test.

After the user saved/closed the game, an unsandboxed process/module guard
preceded the server copy. `server-copy.json` records all 47 active mod plugin
files plus the selected October 15 `d2rl-d2rds.dll` in root `d2rloader/plugins`
(48 files). D2RDS source SHA-256 is
`7a894fad4b1bf5bbc16dcbf1c184b442c12fe279cb3c5e5c92b6cfcfc90a8d86`.
Every copied file matches its source, the mod files remain unchanged, and both
configurations are hash-preserved. Saves and any prior root files were backed
up. No server process was launched. This qualifies installation and one real
offline loopback mutation/publication, not separate-host networking, dedicated
game admission or save/rejoin of the new output.

## Validation boundary

### Dedicated-host refusal observed after beta.12 installation

The user joined the local dedicated host and reported that the picker displays
Unavailable and gem clicks do nothing. Client PID 17380 loaded the exact beta.12
DLL and the admitted Core. At 20:15:05 the loader accepted the gem-bag-ui network
channel, D2RDS logged a 93-byte request frame and a 216-byte reply, and the client
logged `The host cannot schedule an authoritative Gem Bag action.` This text is
issued only after a non-Success ThreadService scheduling return. The numeric
thread result is not logged in beta.12. The failed request is Hello; no bag lease
is granted, so later gem clicks cannot reach native inspection or withdrawal.
The network and input paths therefore reached this refusal boundary. It does
not establish a missing registration, a bad mouse hit, a full grid or an actual
credit mutation. Root loader/plugin logs can interleave processes; do not infer
a single process's native call stack from their timestamps alone.

Failure logs are retained under `hosted-beta12-failure/` in the ignored Gem Bag
workspace. Read-only access to server PID 56500 was denied by Windows (error 5),
so its mapped modules, game pointers and actual queue predicate are not yet
observed. No D2RDS binary, config or running-process memory was modified. The
user confirms the supplied D2RDS DLL is closed source and sensitive; no source
availability or modification is assumed.

The QOL precedent is `native_identify.cpp`, `client_transfer.cpp` and
`materials_actions.cpp`, with records `REMOTE-ID-TRANSFER-TRACE.md` and
`MATERIALS-NATIVE-CONTRACT.md` in the public owning plugin. On Unavailable game
scheduling, those paths use qualified native UI actions/request builders and
wait for changed client snapshots. Existing server handlers perform the actual
identify/transfer/advanced-counter operation. In particular, advanced counter
withdrawal uses a bound counter item and season-specific stash owner; it is not
an API to debit a Gem Bag's stat 386. The Cube route submits normal transmutation
and still requires the actual recipe inputs. These paths establish a useful
client-request and guarded-forwarding precedent, not a proved bag-specific
server execution or arbitrary item-creation endpoint. The Gem Bag host adapter
must establish that missing contract rather than treating the prior QOL result
as proof that generic SDK host callbacks work in D2RDS.

Read-only static inspection of the admitted Core found ThreadService at
Core+0x637ED8: size 24, ABI 1, functions +0x493E40 (UI) and +0x493E80 (game).
Both enter +0x493EC0 with R9 selecting the queue. The game branch loads the
pointer at Core+0x700480, requires its +0x2E0 game pointer, and checks network
mode through Core+0x703A30 / pointed +0x14. Refusal branches return thread enum 2.
These are observation witnesses, not adopted calls or permission to alter those
globals. The network table candidate at Core+0x6352C0 has getPeerPlayer wrapper
+0x453970 and body +0x4578D0. Its initial gates require the captured thread id at
Core+0x7DF084, current thread identity, TLS game pointer at +0x1C38 (TLS index
Core+0x7DF224), and admitted network role before channel/player resolution.

`tools/inspect_thread_provider.py` locates bounded service-table candidates in
the exact files. `tools/capture_hosted_queue.py` verifies the service table and
full mapped functions against PE exception-boundary spans before reading state.
The client capture matches all five spans below. It found queue context
0x142A23400, its game pointer zero, network mode 0, captured game thread id zero
and ordinary main+0x2A9E020 game pointer zero. This describes the joined client,
not the protected server. Native pointers are session-specific observations.

| Core RVA | Length | SHA-256 |
| --- | --- | --- |
| 0x493E40 | 32 | c8459d1e829171751047f19fcd236cc7fcf5d01dab73e9808ec64088ede9b32a |
| 0x493E80 | 32 | 523674289254cf4154c9d745aa983f9133bbb29c3f229be2f78d8f7b28b9669f |
| 0x493EC0 | 1194 | aa81edc65d6cf5960412a6e25e8fa2c69985b2eed1209b8edcf47405bcb62374 |
| 0x453970 | 29 | 17868b0cb5d38bd117e57b72660fb527a0aea2a408e32e4d1afefb492e82a2ea |
| 0x4578D0 | 775 | aeb4f79e2290862b6261715718e699306097dc62f19157cb274dd036786a78ef |

All seven suites still pass after adding the observed host-queue refusal case:
it confirms no native reads, debit, creation, client fallback or withdrawal
lease when host scheduling is unavailable. The installed DLL is unchanged;
this additional test demonstrates safe refusal, not a dedicated-host fix.

### Qualified per-game callback adapter (beta.13)

After the user relaunched without elevation, read-only access to server PID
57208 succeeded. Its actual Core game queue state had local-game pointer zero
at state+0x2E0, network role 1 and captured game thread 50464; ordinary
main+0x2A9E020 was also zero. Client PID 52760 had role 2 and no captured game
thread. These observations replace the earlier access limitation, not the
historical failure record. The server's TEB/TLS sample observed a nonzero native
game pointer 0x189C54810 in one of 512 sequential samples; all other samples
were outside that scope. TEB 0x34C000 and TLS index 16 are session-specific.
`tools/capture_server_game_scope.py` uses OpenThread QUERY_INFORMATION and
NtQueryInformationThread basic information, verifies PID/TID, reads TEB+0x58,
then TLS[index]+0x1C38. It does not suspend, invoke or write into the process.

Core dispatcher +0x493800 captures its game-thread id, saves the previous TLS
game, establishes the incoming game in TLS+0x1C38, processes callbacks, then
restores the prior TLS value. The existing direct call at Core+0x4939C3 enters
Core+0x493620 while that scope is still established. Its ABI is
`void __fastcall(void* queue, void* localGame)`; the first argument is
Core+0x7DF030. The second is the ordinary SDK local-game pointer and can be zero
on D2RDS. The call's return value is unused. The adapter preserves both arguments
and calls the original drain exactly once after its bounded plugin work.

The complete admitted bodies are recorded in `frame-witnesses.json` and
`src/frame_profile.h`, from the exact Core file SHA-256
`2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.
Read-only capture required static/mapped byte agreement before generating them.

| Core RVA | Role | Length | SHA-256 |
| --- | --- | --- | --- |
| 0x493800 | Per-game dispatcher | 509 | ae88b90cbcef617e6b1d1d75263cc143a99893bee4aab0a979af05ad4f686634 |
| 0x493620 | Original SDK queue drain | 344 | 5b2fec5cba98786066369a057694395501e557f85cde7b3b01bd455064cc1966 |
| 0x4578D0 | Peer resolution body | 775 | aeb4f79e2290862b6261715718e699306097dc62f19157cb274dd036786a78ef |

The aligned 16-byte block at Core+0x4939C0 is
`8b55f8e858fcffff488b45c048890648`. Only its call operand, bytes 4 through 7,
changes; the E8 opcode and surrounding instructions stay identical. The SDK
inline-hook API bounds targets to the executable image, so it cannot own this
Core call. Its ABI4 public context is 200 bytes and public API table 224 bytes;
read-only server observation found table Core+0x636CE0, patchBytes +0x47BE90,
checkExpected +0x47CE20, installInlineHook +0x47D960 and queryService +0x485EE0.
The executable-image bound is observed near Core+0x47DAA6. No fabricated context
or out-of-image RVA is submitted. This plugin owns its separate guarded relay;
it does not modify any D2RDS code or provider registration.

`src/frame_bridge.h` allocates a near RX relay and separate RW descriptor. It
requires a matching original call target, exact block bytes, 16-byte alignment
and compare/exchange-128 CPU support. Publication uses a single aligned
InterlockedCompareExchange128 after temporarily changing protection, then
restores protection and flushes the instruction cache. The relay preserves the
two pointer arguments and Win64 call-frame requirements, increments its active
count before loading its target, and decrements it after return. A null target
forwards directly to the original drain. Its descriptor layout is target+0,
original+8 and active LONG+16 in a 32-byte aligned object.

Once published, the tiny relay and descriptor remain allocated until process
exit because later hooks may retain a path to that relay. Stop first clears its
target atomically, restores only bytes still owned by this bridge, then drains
active entries. It never replaces foreign bytes. After two seconds an in-flight
target can pin its own module with GetModuleHandleEx FROM_ADDRESS|PIN; if that
fails, draining continues. Retained callers then reach original Core code
without calling an unloaded plugin. The executable fixture exercises forwarding,
argument preservation, only-operand mutation, foreign-owner preservation and
concurrent teardown; it does not establish live Core behavior.

`src/server_frame.cpp` additionally requires the full bodies on every admission
and an owned call patch. It normalizes only its owned 16-byte block when checking
the dispatcher. CurrentGame requires the same plugin context, live pump,
Core+0x7DF084 equal to the current thread id, network role 1 or 3 through
Core+0x703A30 / pointed +0x14, TLS index Core+0x7DF224 below 1088, and nonzero
Core-established TLS game. The adapter requires the expected queue argument.
It never writes a TLS value or game global. Failed admission leaves the original
Core drain available and performs no plugin item work. Refused installation
retains the ordinary SDK scheduling route.

Only SDK scheduling Unavailable routes a copied request to this scoped pump.
Each game pass attempts at most the currently queued 64 scoped jobs; ordinary
SDK callbacks drain only ordinary jobs. getPeerPlayer runs inside the current
native game scope. NotFound defers the value job until a matching game's frame,
for at most seven seconds, before the client's eight-second timeout. No bag
inspection, credit debit or creation happens in an unresolved game. Other peer
errors refuse. Peer epochs, leases, replay cache, owner halt and compensation
remain unchanged; raw game/player/item pointers never enter queued jobs.

Publication now compares the item's native game to this qualified scoped game
when available, otherwise to the ordinary active game pointer. Item/owner game
agreement, owned inventory and stat source, byte/table guards, identified Normal
output verification and compensation remain required. This is the dedicated
scope replacement for the earlier ordinary main+0x2A9E020-only precondition.

Eight suites pass on beta.13, including wrong-game deferral/expiry and one-credit
withdrawal with cached duplicates using SDK/backend substitutes. The candidate
is installed in both plugin folders with rollback, saves and configuration
preserved; its SHA-256 is
`37f8522c440aeeb1b62327afc16cfa6cbb3ffb83bf08261d9161b6fca5e3050b`.
The supplied D2RDS DLL remains unchanged. Live evidence is recorded below;
automated substitutes alone do not establish hosted behavior.

The first beta.13 restart used different profiles: server PID 57848 loaded
`mods/ReimaginedLadder/d2rloader/plugins`, while client PID 59156 loaded
`mods/Reimagined/d2rloader/plugins`. The server had no Gem Bag DLL mapped; the
client mapped the exact beta.13 and logged bridge installation. Joining refused
ItemStatCost schema mismatch before any Gem Bag inspection. This attempt is not
a test of the beta.13 host adapter. Explicit matching Reimagined launch arguments
were requested; no Ladder files or configuration were changed.

That client also reported Death Tracker and Kill Tracker load crashes. Its
first-chance execute AV was at 0x7FF94FA64790, reached through client Global Chat.
Read-only module metadata places that address in the *server's* Death Tracker
at base 0x7FF94FA20000, from its different Ladder DLL (SHA-256
`8dff8f7131018de450cf5f3b684dafd750aa9da049b3bf02deded94a876c7be1`).
`d2rl-plugins/libs/chat-sdk/src/chat_input.cpp` uses the Windows named mapping
`Local\D2RLChat.ChatInput.v1` and mutex with `.lock`, without a process-id suffix.
Its 32-byte POD contains size+0, ABI+4, owner module+8, add callback+16 and remove
callback+24. `tools/capture_chat_rendezvous.py` opened that mapping FILE_MAP_READ
and captured owner 0x7FF94FA20000, add 0x7FF94FA64790 and remove 0x7FF94FA647E0,
exactly matching the failure. No pointers were invoked and the mapping was not
written. This establishes an independent cross-process chat-rendezvous defect;
matching profiles or matching DLL base addresses are not a process-isolation fix.
The Gem Bag plugin uses none of these chat services. Evidence is archived under
`hosted-beta13-attempt/` in the ignored workspace. No chat DLL was changed during
this diagnosis.

On the matching-profile restart, server PID 40124 (`-mod Reimagined -txt -d2rds`)
and client PID 47852 (`-mod Reimagined -txt`) both mapped beta.13 with the admitted
Core. All 44 client plugins loaded without the earlier tracker startup crashes.
The shared chat rendezvous defect remains unfixed; successful loading with the
same profile does not qualify that rendezvous's cross-process safety. Four Core
object-pool assertions occurred before the first Gem Bag action; their cause is
unproven. The selected D2RDS file still matches SHA-256
`7a894fad4b1bf5bbc16dcbf1c184b442c12fe279cb3c5e5c92b6cfcfc90a8d86`.
Both actual Gem Bag module paths are the active Reimagined mod folder. The
requested root copy is also hash-matched but this ModScopedOnly plugin's global
copy is rejected; the active mod copy is already loaded. The same duplicate
rejections affect four other mod-only root copies. They are distinct from the
earlier tracker load crashes. Do not describe the root Gem Bag file as the
mapped server module in this launch.

`tools/capture_frame_state.py` qualified the server's three complete mapped
bodies, owned call block, 64-byte RX relay and descriptor: target points inside
Gem Bag UI, original is Core+0x493620 and the sampled active count was zero.
Queue observation still found local-game and ordinary active-game pointers zero,
network role 1 and captured thread 52228. Thus the successful dedicated actions
do not depend on manufacturing an ordinary local game. The loader accepted the
private channel and the real host returned a 93-credit inspection. Six actions
then logged success (Onyx, Diamond, Topaz, Sapphire and two Skulls), reaching 87.
The first three have mouse-selection logs; the latter three follow controller
opening/input. No pre-action memory baseline was captured for those six, so
that cumulative debit is log evidence. Their six identified outputs and the
87-credit bag/grid were independently observed in both processes afterwards.

For the controlled seventh action, `tools/capture_hosted_gem_bag.py` first sampled
the actual server TLS game 0x189FE9BB0 and qualified matching player 1/class 1
at 0x18A202C40 through game+0x2230 and native game ownership. Both inventories
contained bag 148 at inventory (4,2), 87 credits and 29 free cells. The user then
performed exactly one requested mouse Ruby withdrawal. Server/client logs
accepted sequence 14 once. The idle post-action TLS scope was too brief for the
bounded fresh sample; `--prior` requalified the previously observed game through
the same live player table, exact process/module pair, player identity/address
and game ownership. This read anchor is diagnostic only, never plugin execution
authority or a queued native pointer.

`tools/verify_hosted_gem_bag.py` checks both PIDs, exact module identities and the
same native game/player/thread, then applies the existing strict item/grid
comparison. It confirms one Ruby 200/gmr at (6,2), 87 -> 86 credits, 29 -> 28 free
cells, the bag unmoved and all other sampled items unchanged. The Ruby's native
identified bit is set on client (0xA02011) and server (0x82010). This establishes
real dedicated-host mutation and client publication in this single game, not
multi-game isolation, other hosted storage types or repeated hosted rejoin.
Evidence is `live-beta13-hosted-before.json`,
`live-beta13-hosted-after.json`, `live-beta13-hosted-verified.json`,
`live-beta13-server-frame-40124.json`, `live-beta13-server-queue.json` and
`hosted-beta13-success/` in the ignored workspace.

The user then performed Save and Exit from that hosted game and opened an
offline game in the same client PID 47852. Both local domains again had 86
credits, bag 148 at (4,2) and 28 free cells. Matching all occupied bag-storage
cells by code/class/bank/page and gem identified state confirms the saved
inventory view survived this reload, including all seven hosted outputs. Runtime
unit ids can be reassigned (the hosted Ruby 200 became 170); no raw item-seed
identity claim is made. This qualifies that hosted-to-offline save/load transition,
not repeated hosted rejoin, shared storage persistence or the next offline gem.
`live-beta13-save-reload-verified.json` records the bounded comparison.

The exact beta.13 DLL then passed the final offline mouse withdrawal through
the same host request path: one identified Ruby 201/gmr at (7,2), 86 -> 85 credits,
28 -> 27 free cells, bag unmoved and all other sampled item records unchanged
in both domains. Native identified flags are client 0xA02011 and server 0x82010.
The comparison is `live-beta13-offline-verified.json`, with its before/after
snapshots and `live-beta13-final-rpc.log`. The offline local server is present
in this capture, unlike the remote client snapshot. New peer 5's inspection and
withdrawal succeeded after leaving peer 3's hosted session. No additional DLL
replacement or change to another plugin was needed.

The final loader-log audit also retains StatsPool.IsEmpty and repeated
object-pool assertions during later game transitions, in addition to the four
initial pool assertions. Their cause has not been isolated; a successful item
comparison does not establish that these assertions are harmless or caused by
this plugin. `hosted-beta13-success/d2rloader-final.log` and the warning audit
retain them. No plugin startup crash recurred on the matching-profile launch.

### Automated and historical evidence

The first seven CTest suites cover placement/conservation policies (18592 combinations),
synthetic native ownership/grid/source cases (32 storage/gem cases plus refusal
checks), and DLL resources/exports/metadata/inert unsupported-process loading.
The native suite additionally exercises the 64 actual withdrawal creation and
verification cases with the substitutions described above.
Strict layout JSON is parsed, including eight unique sprite references and
native text color schema. Placement covers independent scales and all four
screen edges. The fourth suite executes the complete adopted GetOrigin, Scale,
Contains, GetRect, SetX and SetY bodies in an owned test image against synthetic
widgets, with manager lookup and the rounding helper replaced. The rounding
substitute is identity; checks use interior hit points and viewport tolerance,
and do not qualify the game's rounding behavior. It checks actual packed-point
containment for all eight moved icons
and Close, screen-edge placement, only-Card-position mutation, child limits,
wrong parent, fill-parent exclusion, ancestor cycles, duplicate names, alternate
vtable method and changed byte ownership. It cannot establish real widget
lifetime, rendering, window/DPI mapping or mouse-message delivery. No automated
suite executes the bag setter or publisher in D2R. The fourth also checks the
qualified reserved-buffer writer; the fifth covers callback lifecycle with the
substitutions and limits above.

The sixth suite exercises actual host-only Load without querying UI services.
The seventh uses actual protocol/RPC code with separate owned client/host SDK
contexts and loopback, peer resolution, leases, changed bag/space/credit/cursor,
duplicate and stale messages, callback discard, timeout/disconnect/unload and
owner halt across reconnects. Its SDK queues and native backend are substitutes;
it executes no real sockets, native publisher or saves.

Beta.1's exact installed/loaded hash was
`6f37914a09b4dd33ac4023023b763183874a11d79369cb631f67380751a0c8c0`.
The loader admitted mouse and controller integration in PID 49772. External
read-only `live-beta1-before.json` confirmed client/server bag ID 137, class 746,
118 credits in the Cube. The user's right-click showed no selector; no
withdrawal was attempted.

Beta.2 SHA-256 `e6f1d5c6506dd35deb9bb774198b7b46d27258b1b383485fb9621df818df3720`
was loaded in PID 46188. The user opened the picker with right-click and used
arrows/Enter to withdraw all eight gems. `live-beta2-all8-verified.json` confirms
both client/server 118 -> 110, all eight recipe codes at Cube row zero x0..7,
bag ID 138 unmoved at (11,7), and free cells 95 -> 87. A separate Battle Axe
ID 171/code 9ba was explicitly confirmed by the user and admitted separately
by the comparison. Other sampled old identity/code/bank/page/stat records were
unchanged. The sequential read-only snapshots do not prove every hidden item
field. Mouse choice failed.

Beta.3 SHA-256 `4324d53d8dac33c976b893c2937a2ac027720dbb6951b7275fd657c8e1e09031`
logged 110 credits after the next restart; the user's screenshot shows the
eight outputs still in the Cube. Enter withdrew another Amethyst; its log
reports 109 credits. The user reported blank labels and failed mouse clicks.
The screenshot is not a native coordinate-space measurement.

Beta.4's icon geometry was rejected as detailed above. Beta.5's corrected
inheritance, visible icon picker and mouse use in inventory/Cube/personal/shared
page 1 are confirmed above. Beta.6 resolves all eight names but renders their
color marker, and its controller controls/centered position were accepted.
Beta.7 refused during startup; beta.8 resolved names but native Open returned
NotFound after late layout publication. Beta.9's restored startup resources and
plain names were visually confirmed. Beta.10 fixes unidentified creation and
has a matching client/server identified Ruby, one-credit and placement check.
Embedded Cube presentations, controller coexistence, full/empty refusal and
shared failure compensation remain pending.
Preserve these distinctions when promoting the candidate or writing release
notes.



Investigation helpers referenced above remain in the research repository under
`experiments/gem-bag-ui/tools/`; they are not runtime dependencies.
