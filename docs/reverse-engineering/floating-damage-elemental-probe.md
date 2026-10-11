# Floating Damage: native combat components and poison inference

Reviewed 2026-10-11 for `0.1.0-beta.11`. Six component fields, exact HP
correlation, surviving poison intervals, terminal poison and mixed composition
are established on the tested profile. Periodic classification is a bounded
inference from state/HP/frame evidence, not a newly proven native tick callback.

## Exact artifacts and provenance

| Artifact | SHA-256 |
|---|---|
| Loader-hosted D2R image, `D2RLoader.exe` 1.3.1-beta | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| `D2RCore.dll` 1.3.1-beta | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Older decrypted analysis image | `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D` |

The older image has readable game `.text`; the packed host has no equivalent
on-disk `.text` bytes. Static candidates require independent current runtime
window matching. A game-writer match alone does not admit the older image.
Analysis images, raw captures and deployment evidence are excluded from public
source. Addresses below are game/Core-relative RVAs as labeled.

Classic [D2MOO SUnitDmg.h](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/include/UNIT/SUnitDmg.h)
and [SUnitDmg.cpp](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/UNIT/SUnitDmg.cpp)
provided search leads for components and resistance/absorption/poison handling.
They are classic D2 1.10f sources, not a D2R ABI; the current offsets differ.

## Guarded Apply observation

`0x44CE80` uses the witnessed Win64 register/stack flow: RCX game, RDX attacker,
R8 defender, R9D missile-mode context, and the fifth argument a borrowed damage
record. Matching authoritative monster/owner/HP and controlled attacks support
game/defender/record interpretation. Attacker identity is not followed. Spells
were observed with missile context 1 and a plain weapon attack with 0; those
examples do not reconstruct every mode or return-value meaning.

The hook declares the required SDK `NativeHooks` capability. It forwards all
five arguments exactly once through the original trampoline and preserves RAX.
It synchronously copies `0x138` packet bytes and bounded hostile HP evidence
before/after the original; no borrowed record/vector pointer is retained for
later dereference. No damage, flags, HP or combat decision is changed.
SDK owner cleanup owns the detour. A competing detour at the same entry must
be refused; no second diagnostic plugin is part of this distribution.

Entry guard, 48 bytes:

```text
40 55 53 56 57 41 56 41 57 48 8D AC 24 E8 FE FF FF 48 81 EC 18 02 00 00 48 8B 05 29 E4 57 02 48 33 C4 48 89 85 00 01 00 00 48 8B BD 70 01 00 00
```

HP-subtraction guard at `0x44D062`, 54 bytes:

```text
8B 9F 34 01 00 00 85 DB 7E 2C 45 33 C0 48 8B CE 41 8D 50 06 E8 A5 7F EA FF 2B C3 45 33 C9 45 33 C0 48 8B CE 3D 00 01 00 00 44 0F 4D C0 41 8D 51 06 E8 78 AC EA FF
```

Both windows matched the reviewed live image. Refused combat guards/install
leave ordinary HP numbers available. Disabling display does not remove the
installed pass-through hook; an absent-DLL comparison requires process restart.

## Static combat witnesses

| Game RVA | Witness and limit |
|---|---|
| `0x44CEA9` | `mov rdi,[rbp+0x170]`; pushes consume `0x30`, RBP is entry RSP−`0x148`, resolving the record load to entry RSP+`0x28`, the fifth Win64 argument. |
| `0x44CF93 → 0x44DF10` | Mode-gated calculation call using game/attacker/defender/record in RCX/RDX/R8/R9; full calculation ABI is not called by the plugin. |
| `0x44D038 → 0x450C90` | Four-argument processing before HP subtraction; no new helper invocation. |
| `0x44D062..0x44D097` | Reads signed packet total `+0x134`, reads defender stat 6, subtracts total, floors results below 256 raw to zero, then writes stat 6. Packet total is not automatically observed net HP loss. |
| `0x2F5020`, `0x2F7D10` | Stat getter/setter candidates already indirect-detoured; preserve Core behavior and do not re-hook/invoke them. |
| `0x44D51D`, `0x44D535 → 0x44D570` | Flag-dependent event dispatch with record as fifth argument; resemblance to death/kill dispatch does not replace the supported lifecycle service. |
| `0x44CD03..0x44CD7F` | Conversion branches add into component slots; partial accesses do not establish every conversion's gameplay rule. |
| `0x44D1CF..0x44D22D` | Vector pointer `+0x40`, unsigned 64-bit count `+0x48`, 12-byte entries. |
| Record `+0x118/+0x11C/+0x12C/+0x130` | Passed to other helpers; meanings remain unknown. |

Thirty-six direct relative-call candidates were found in the analysis text;
aligned witnesses `0x418885`, `0x436F95`, `0x4397A4`, `0x447FDF`, `0x44B3FA`
supply caller-owned stack records in outgoing slot `+0x20` and use them after
Apply. This supports synchronous copying, not an exhaustive execution graph.
The classic `0x70`-byte damage structure must not be applied to this record.

## Component fields and HP endpoint coverage

| Component | Signed 32-bit offset | Mask |
|---|---|---:|
| Physical | `+0x18` | 1 |
| Fire | `+0x20` | 2 |
| Cold | `+0x34` | 4 |
| Lightning | `+0x2C` | 8 |
| Magic | `+0x30` | 16 |
| Poison | `+0x38` | 32 |
| Poison duration | `+0x3C` | No separate component |
| Native total | `+0x134` | Must equal the positive six-component sum |

Any negative component, nonpositive sum or unexplained total refuses packet
classification. Multiple verified components select mixed color. Unclassified
or unsupported records preserve fallback styling. Neither packet nor vector
values supply the displayed amount; it always comes from observed HP loss.

A bounded 256-entry owned history matches numeric identity, ID/class,
owner/session, HP endpoints and a two-second freshness window. Every decrease
in a merged chain must be covered. Healing, gaps, reused identities and stale
records clear classification. When after-HP is unreadable, a potentially lethal
record may remain pending for at most 500 ms only when positive total can meet
the witnessed floor (`total >= beforeHP−255`). A matching supported death or
valid zero sample resolves it. A later positive pre-hit HP retires the older
pending terminal record. Read failure/disappearance is never a zero endpoint.
This also covers death callbacks occurring inside or after the original call.

A controlled Fire Bolt demonstrated valid post-packet, unreadable post-HP,
then pending-death followed by SDK death after the hook returned. Remaining HP
was 768 raw (3 HP), while fire/total was 10930 raw. The terminal number used
768 and fire mask 2. This explains the earlier fallback color on that tested
late-death ordering without inventing a valid zero-HP read.

## Controlled component evidence

Values are raw fixed-point HP units (256 per HP); case labels are local-neutral
examples rather than process/unit identifiers.

| Controlled attack | HP before | Component before → after | Post total | Result |
|---|---:|---|---:|---|
| Fire Bolt | 1792 | Fire 9650 → 10132 | 10132 | Lethal; fire `+0x20`. |
| Plain weapon attack | 512 | Physical 1120 → 1120 | 1120 | Lethal; physical `+0x18`. |
| Cold Bolt | 3072 | Cold 5442 → 5714 | 5714 | Lethal; cold `+0x34`. |
| Lightning | 1280 | Lightning 46190 → 48499 | 48499 | Lethal; lightning `+0x2C`. |
| Teeth | 512 | Magic 1641 → 1641 | 1641 | Lethal; magic `+0x30`. |
| Poison Nova, case A | 512 | Poison 1537 | 1537 | State2 stat74 is `uint32(-1537)`, duration 50. |
| Poison Nova, case B | 512 | Poison 1364 | 1364 | State2 stat74 is `uint32(-1364)`, duration 50. |

Other components were zero in these isolated cases. Post-packet was readable;
post-HP on lethal cases was not. Default logged zero from a failed read is not
successful zero-HP evidence. The observed spell-component increase has no
established gameplay cause here and does not prove resistance/absorption rules.

Three surviving Teeth observations independently matched HP losses 1088, 1860
and 1111 raw to pure magic/total and mask 16. Healing between hits updated the
baseline without fabricating damage. Runtime classification is confirmed;
these records alone do not prove visible violet rendering. User confirmations
establish correctly colored fire/cold/lightning in the integrated plugin.

## Additional poison static consumer, reviewed analysis image

| Site | Witness and limit |
| --- | --- |
| `0x44D1CF` / `0x44D1DB` | Borrowed record vector pointer +0x40, u64 count +0x48; count times 12 determines end |
| `0x44D1F0..0x44D226` | Entry dwords +0/+4/+8 become outgoing stack argument 7, R8d and R9d respectively; call `0x44D21A -> 0x4518C0`; stride 12 |
| `0x44D237..0x44D262` | Legacy fields +0x38/+0x3C minus accumulated vector values become R8d/R9d; record +8 becomes outgoing fifth argument, fifth/sixth/seventh include -1 sentinels; call `0x4518C0` |
| `0x4518C0` | Consumer starts by rejecting nonpositive R9d, then preserves RCX/RDX/R8d/R9d as game/unit/rate/duration candidates; positive R8d required; no complete function ABI claim |
| `0x45193E` / `0x451946` / `0x4519AB` | Outgoing stack arguments 7/6/5 read at current RSP +0xC0/+0xB8/+0xB0 after four pushes and 0x68 allocation |
| `0x4519C8 -> 0x2F5AB0`, `0x4519D7 -> 0x2F5940` | Query helpers with EDX=2 and unit; additional keys in the first path. Poison-state-list resemblance only, not called by plugin |
| `0x451A25` | Adds current game-like frame field +0x170 to preserved R9d duration candidate |
| `0x451AC9..0x451AD6 -> 0x2F3030` | Negates preserved rate and writes stat-like ID 0x4A (74) to a new list candidate |
| `0x451BAB -> 0x2F5DF0`, `0x451BB0..0x451BB4` | Reads existing stat-like 74, negates it, compares old/new rates; skips replacement for stronger existing rate |
| `0x451BF0..0x451BFE -> 0x2F7C00` | Negates rate and updates stat-like 74 on existing list candidate |
| `0x451B6D` / `0x451B74` / `0x451B82 -> 0x2F7A50` | Chooses callback addresses `0x436240` or `0x4505E0`; binds to the list candidate; callback semantics unconfirmed |
| `0x451B93 -> 0x2F2CB0` | Links new list candidate to unit, R8d=1; no direct HP subtraction observed in this bounded consumer |

Entry bytes (static witness, not an installed new guard):
`45 85 C9 0F 8E 61 03 00 00 48 8B C4 55 56 41 56 41 57 48 83 EC 68 45 8B F1 41 8B E8 48 8B F2 4C 8B F9 45 85 C0 0F 8E 35 03 00 00`.

Other inspected calls in this consumer: `0x4518FB -> 0x48FF00`,
`0x45190A/0x451AED -> 0x300A90`, `0x45192C/0x451B0F -> 0x213900`,
`0x45197B/0x451B4E -> 0x213A20`, `0x4519A6/0x451A54 -> 0x34A330`,
`0x4519E2/0x451A5E -> 0x34B9D0`, `0x4519F9 -> 0x48B890`,
`0x451A20/0x451AB9/0x451BE0 -> 0x48B720`, `0x451A3F -> 0x3354C0`,
`0x451A71 -> 0x2F7300`, `0x451A83 -> 0x2F72A0`, `0x451A8E -> 0x2F7250`,
`0x451A99 -> 0x2F7200`, `0x451AE1 -> 0x2F72F0`, `0x451BBC -> 0x2F7AB0`.
These are static xrefs, not independently proven contracts. Preserve Core
detours on stat helpers; none is hooked or directly invoked by the plugin.


Further bounded state-list static witness at `0x2F5940`: RCX unit-like input,
EDX state ID; call `0x2F596A -> 0x34B870` obtains a root list candidate. Root
flags +0x1C must be negative. `0x2F59BB` reads root head +0x90, `0x2F59D5`
reads +0x98; nodes compare state dword +0x20 at `0x2F59C7/0x2F59E1` and follow
next pointer +0x68 at `0x2F59CC/0x2F59E6`. Other inspected assertion/helper calls:
`0x2F595D -> 0x2F1300`, `0x2F5987 -> 0x2F22E0`, `0x2F59A4 -> 0x2F27A0`.
Neighbor filtered lookup `0x2F5A00` calls `0x34B870` at `0x2F5A32`, chooses
head +0x98 for flag bit 13 instead of +0x90, and compares state +0x20 and node
flags +0x1C. Its additional assertion calls are `0x2F5A25 -> 0x2F1240` and
`0x2F5A4F -> 0x2F1390`. None is directly called or newly hooked.

## Poison state, frame and expiration contract

State lookup reads unit `+0x88`, negative root flags `+0x1C`, heads `+0x90/+0x98`,
node state `+0x20`, next `+0x68` and the established 16-byte base-stat array at
`+0x30`. It allows 64 distinct nodes across both heads and 2048 entries per
array, rejecting cycles, invalid pages/counts/capacities and duplicate layer-zero
stat74. State ID 2 and negative stat74 are supported by controlled Poison Nova.

At `0x2F7AB0`, null RCX returns; positive EDX sets node `+0x1C` bit 1 and converts
integer expiration to float32 at node `+0x24`. Exact 26-byte guard:
`4885C9741485D27E0483491C02660F6EC20F5BC0F30F114124C3`.
At `0x451A25`, `add r14d,[rdi+0x170]` adds the owner's current game frame to
duration. Exact seven-byte guard: `4403B770010000`.
Both matched the live reviewed image; neither helper is invoked/hooked.
Neighbor `0x2F7AD0` is already an indirect Core detour and must be preserved.

Signed stat74 may be stored as a zero-extended negative 32-bit value in a wide
cell. Normalize only this witnessed stat; do not reinterpret HP or arbitrary
wide stats. The nonnegative base74 from root array `+0x30` must explain the
negative composed74 exactly: `poisonState74 + base74 == composed74`. Missing
base74 means zero; duplicate/invalid/negative base or unexplained contribution
refuses inference. Prediction uses the full poison rate, not composed net regen.
For example, state −231 plus base +17 gives composed −214, while each observed
advancing game frame loses 231 raw HP. Requiring state==composed had incorrectly
refused this valid surviving path; the base-contribution rule corrects it.

Current frame is unsigned 32-bit game `+0x170`; expiration is finite integral
float32 node `+0x24`. Both must be below `2^24`, expiration must remain ahead
and no more than 15000 frames away. Both live endpoints require equal rate and
expiration, forward frame advance 1–8 and exact `rate × frames` HP loss. No wrap
is inferred. Only supported death/zero can allow a final tick larger than the
remaining positive HP, using a fresh frame and the original identity/session/
two-second death-cache guards. Changed rates, expiration, missing evidence,
healing or incomplete coverage refuse classification.

Owned synthetic intervals preserve merged coverage and bridge gaps around known
native hits only when the same exact prediction succeeds. State presence alone
cannot color a decrease. Unseen effects that cancel numerically may still mimic
this prediction; it is an explicit inference limit.

## Periodic, terminal and mixed live evidence

An early surviving Poison Nova sequence applied rate 296, then observed
1240 → 944 → 648 → 352 → 56 raw without another combat call, followed by a
supported terminal remainder of 56 raw (0.21875 HP). Other cases similarly
matched rates 271 and 299. This established repeated full-rate loss and the
sub-unit terminal display before frame/expiration inference was added.

Four later surviving cases established valid game-frame advance and float
expiration at application frame plus 50; rates 231/350/283/312 had base74
17/18/16/16 and matching negative composed values. All remained alive at the
capture cap, so those records do not establish expiration removal or death.
The corrected retest produced 51 poison-classified number records across five
surviving targets with rates 226/311/341/218/310 and valid state/base/composed/
frame/expiration coverage. User confirmation supports the corrected display.

Three separately supported periodic deaths each produced exactly one terminal
number with poison mask 32. Actual remainders were 176, 129 and 200 raw, while
rates 208, 187 and 264 exceeded those remainders. All final frames advanced by
one and expiration remained ahead. Amounts retained the real remainder, not a
full-rate overkill value. Whole-HP rounding shows 1 for these fractions; they
are distinct from the smaller `<1` case. Survivor expiration remains untested.

A surviving mixed hit lost 52607 raw HP, exactly physical 7691 + fire 44802 +
poison 114; all other components, including magic, were zero. Mask 35 correctly
selected mixed. Thirteen initial/merged number records retained mask 35; 48
subsequent fresh poison-only records used 32. Base74 +93 and state −114 explained
composed −21, with duration 100. One synchronously copied vector entry had
`{tag31,rate114,length100}`. Tag31 remains unnamed; it is not an extra amount
and is never added again. A single entry does not independently prove stride.
The observed similarity between old mixed/magic colors was a palette issue,
not a demonstrated magic-classification error.

## Core bindings and rejected periodic leads

| Core RVA or binding | Finding and limit |
|---|---|
| `0x81F920` / DamageSoaringStrike | Missile-specific flags, no general component callback. |
| `0x833350 → 0x3D51B0` / AssignWideMissileDamage | Upstream missile min/max construction, not observed HP loss. |
| `0x832D70` / PublishWideUnitDamageStats | Stat contribution publishing, not a proven hit event. |
| `0x832030` / SetWideUnitStat | Wide setter bridge; stat6 writes do not reveal damage type. |
| `0x80FF90` / WriteCombatDebugMessageToConsole | Bounded string queue, not typed combat notification. |
| `.data 0x701B38` plus 8 → `0xA7DB0`; `.data 0x708B38` plus 8 → `0x44D570` | Static pointer-binding entries, not directly consumed. |

The inspected `.d2rsym` contains unnamed numeric pairs. A broad `mov edx,6`
scan produced 198 mostly unrelated windows; `0x325C00/0x325C50` are predicates/
bit queries rather than the suspected HP getters. Names and immediates alone
must not be promoted to native contracts.

A broad `mov edx,74` immediate scan did **not** establish the periodic HP updater.
Rejected matches/call leads: `0xBF67D -> 0x76210`, `0x545322 -> 0x2F7D10`,
`0x5ED281 -> 0x491960`, `0x9F3588 -> 0x9F62D0`, `0xE79B0A -> 0xE6EF00`,
`0x1933371/0x1933752 -> 0x1350B70`, `0x1933ADA/0x1933C97 -> 0x1B0B8E0`,
`0x1A81F66 -> 0x76210`. A stat setter/immediate coincidence alone is not a
periodic damage-event contract; none is called or hooked by this change.

Further bounded immediate scan, same private analysis image: two nearby
`mov r8d,74` writes at `0x4514AB -> 0x2F3030` and `0x451544 -> 0x2F7C00`.
The neighboring first path sets state-like ID115 at `0x4514B9..0x4514C1` and
binds a callback via `0x4514D0 -> 0x2F7A50`; those are setters, not established
periodic HP consumers. They are rejected as a new tick hook and never invoked.
ECX/R9d immediate matches at `0x2E89ED/0x2E89EE`, `0x9B53A6/0x9B53A7`,
`0x9B56D6/0x9B56D7`, `0x1112D10`, `0x1184F45/0x1184F46`,
`0x1416A7A/0x1416A7B`, `0x144C6CD/0x144C6CE`, `0x15E0F35/0x15E0F36`,
`0x1B63815/0x1B63816`, `0x1B8322A/0x1B8322B` are unrelated assertions/values
or unaligned suffix matches. Do not treat immediate scans as an execution graph.

## Validation and remaining scope

The seven current Release suites include exact-DLL admission, native argument/
one-call/RAX preservation, synchronous/delayed deaths, six-component sums,
poison/base/frame refusal, captured endpoint sequences, merged groups and
critical metadata. Artifact checks require the ABI resource, embedded config,
exports, identity/version and native-hook capability. Tests establish simulated
contracts, not game stability or visible presentation.

Live evidence confirms admitted guards, six isolated components, surviving
magic endpoints, surviving/merged poison, three terminal poison cases and mixed
physical/fire/poison separation. Actual resistance/absorption behavior, survivor
poison expiration and unseen canceling effects remain outside the established
contract. The [critical marker record](floating-damage-critical-strike-probe.md)
provides separate marker evidence; colors never substitute for that bit.
