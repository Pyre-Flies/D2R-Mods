# Floating Damage: HP observation and projection contracts

Reviewed 2026-10-11 for `0.1.0-beta.11`. This record describes the validated
reader and preserves rejected discovery leads. Raw process captures,
deployment receipts, saves and game images are excluded from public source.

## Scope and exact build

Floating Damage reports observed hostile-monster HP loss from any source.
It does not identify an attacker or provide a DPS meter. Local single player
is the initial validation target; a remote client has no local authoritative
game thread for this reader.

The loader-hosted D2R `3.3.93787` / D2RLoader `1.3.1-beta` profile requires:

| Artifact | SHA-256 |
|---|---|
| `D2RLoader.exe` | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| `D2RCore.dll` | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |

File identities do not alone fingerprint decrypted code. Runtime admission
also checks the game-writer, projection and clip windows in
[the native profile](../../plugins/floating-damage/src/native_profile.h).
All six windows matched the reviewed live image. A mismatch leaves the feature
inert; constants in this record are not a portable D2R ABI.

## Reference and public SDK boundary

The [Nexus reference](https://www.nexusmods.com/diablo2resurrected/mods/1085?tab=description)
and [D2RDamageNumbers source](https://github.com/Fr4nsson/D2RDamageNumbers)
provided a design reference for floating totals, tick grouping and lanes.
Its older-version support claims do not validate the admitted artifacts above.
A direct `curl.exe -L` request returned a Cloudflare challenge; a web reader
supplied the description. HP differences alone establish neither damage type
nor critical/deadly status.

The inspected PluginSDK `overlay.h` provides callback-scoped text measurement,
drawing, frame dimensions and a canvas rather than raw ImGui/DirectX pointers.
`lifecycle_events.h` supplies gameplay and monster-death events. Death is not
a general damage callback. `shared_events.h`'s item-tooltip `Damage` anchor is
not a combat notification. No general HP-enumeration or elemental-damage
listener was found in this SDK inspection.

## Authoritative HP reader

| Native binding or layout | Established meaning |
|---|---|
| Game global RVA `0x2A9E020` | Owns the reviewed authoritative game pointer. |
| Writer RVA `0x4057AE` | Bytes `4889056B8869024883C428C3`; stores the game pointer to that global. |
| Unit `+0xD8` | Owner must equal the current authoritative game. |
| Game `+0x2630` | 128 monster bucket heads; bucket index is `id & 127`. |
| Unit header `+0/+4/+8/+0xC` | Type/class/ID/mode, unsigned 32-bit fields; eligible monster type is 1. |
| Unit `+0x158` | Next unit in the reviewed server bucket chain. |
| Unit `+0x88` | Stat-list pointer. |
| Stat-list `+0x1C` | Signed flags select composed array `+0xA8` when negative, otherwise base array `+0x30`. |
| Stat array `+0/+8/+0x10` | Pointer/count/capacity; count and capacity are unsigned 64-bit. |
| Stat entry, 16 bytes | Unsigned layer at `+0`, unsigned stat ID at `+4`, signed 64-bit value at `+8`. |
| Stats 6 and 7 | Current/max HP; local ItemStatCost `ValShift=8`, so 256 raw units equal one HP. |
| Stat 172 | Alignment; nonzero excludes friendly/neutral monsters. Missing stat follows the native zero-default behavior. |
| Unit `+0x38`, path `+0/+4` | Path pointer and unsigned 16.16 XY values, copied into owned world coordinates. |
| Game `+0x2230` | 128 player bucket heads used to locate the local player with matching owner/type/ID. |

The reader runs through the SDK authoritative game-thread service. It copies
values using bounded current-process `ReadProcessMemory`; the renderer receives
owned records, never a native pointer to dereference. Bounds are 1024 eligible
monsters, 2048 distinct visited units, 128 nodes per bucket chain and 2048 stat
entries per array. Invalid pages, ownership, identities, cycles, counts,
capacities, coordinates or HP ranges refuse the snapshot.

A controlled surviving hit decreased authoritative HP by 1095 raw, or
4.27734375 HP, consistent with the reported 2–5 attack range and the table's
8-bit shift. General enumeration found five hostile HP records and three
neutral NPC records, with the latter excluded by alignment. Client health-bar
values changed differently and are not used for damage magnitude.

## Number ownership, terminal hits and classification

The first observation establishes a baseline. Only a decrease for the same
identity, class, owner and session emits damage. Healing updates the baseline;
ordinary disappearance emits nothing. Session transitions invalidate pending
work and clear owned state.

The supported monster-death event may arrive after native stats are no longer
readable. Recent positive HP/position baselines are retained for two seconds;
a matching confirmed death emits that actual remaining HP in its own lane.
Polling zero and death retire the same baseline, preventing duplicate terminal
numbers. Stale/foreign events are refused. Amounts are HP lost to a zero floor,
not overkill from the native packet. A failed HP read is never treated as zero.

Damage components and critical metadata use independent guarded combat evidence
and exact HP endpoint coverage. See the [elemental contract](floating-damage-elemental-probe.md)
and [critical marker contract](floating-damage-critical-strike-probe.md).
Unmatched or unsupported evidence preserves an ordinary fallback-colored number.

## Projection and drawing

The four-point inverse projection reuses the reviewed Controller QOL path.
Renderer accessor RVA `0x7E65A0` and wrapper RVA `0x7E7CE0` have the source-level
call shape `bool __fastcall(void* renderer,float* x,float* y,uint64_t packedScreenXY)`.
The wrapper owns its collision buffer. Four current screen/world samples must
pass finite-coordinate, distance, affine-consistency and determinant checks.
The player's world position anchors validity; camera changes are reflected by
new samples. This is a guarded build-specific helper call, not an exported SDK ABI.

Native draw-list RVA `0xCEAED0`, push-clip `0xD03A30` and pop-clip `0xD03760`
provide a balanced callback-scoped clip correction around SDK drawing. Their
exact byte windows are in the native profile. The plugin measures and centers
text, draws an optional translucent backing, weighted shadow glyphs, continuous
outline and fill, then restores clip state. Rise/fall/side motion and fading
operate only on owned records. Styling changes no native damage or HP.

## Rejected and superseded discovery leads

| Lead | Limit or correction |
|---|---|
| External 8-byte stat entries | Rejected: this runtime uses 16-byte wide entries. External `+0x80` stat-array assumptions do not replace the reviewed `+0xA8` selector. |
| Client normalized HP | Initial values used a full-scale 32768; two controlled drops were 11776 and 7168. Those are health-bar changes, not 46/28 HP obtained by shifting them. |
| Lookup `0x9A5D0` | `488D0536939802` at `0x9A5D3` derives table base `0x2A23910`; `48C1E10A` at `0x9A5E5` gives type stride `0x400`, hence monster table `0x2A23D10`. This client lookup does not establish authoritative HP. |
| Lookup `0x9F270` | Existing detour `E9EBE8D8039090` must be preserved. Remaining instruction at `0x9F287`, `488B8858010000`, reads next `+0x158`, superseding the external `+0x150` assumption for that table. |
| `0x34B440` and `0x341C30` | A path through Unit `+0x38` then pointed object `+0x20` (`488B4120C3`) did not establish the authoritative game/table. A GetGame label alone is insufficient. |
| Global `0x2A2CF68`, writer `0xD18B6` | Failed authoritative `+0x2630` table validation; not an admitted game path. |
| Accessor `0x2EFDE0` | Bytes `8B4140418900488B01488902C3` read input count `+0x40` and bucket pointer `+0`; input-object semantics remain unproven. |

## Validation and remaining limits

Seven MSVC Release `/W4 /WX` suites cover model, reader, components, critical
metadata, native-call/drawing adapters, chat lifetime and actual DLL resources,
exports, identity and unsupported-build refusal. Automated checks do not prove
live presentation or stability.

Live evidence establishes loader admission, ordinary floating numbers,
projection, terminal remainders, native command dispatch and elemental/poison/
mixed correlation on the reviewed setup. User reports confirm brighter outlined
numbers, motion, fire/cold/lightning colors and corrected surviving-poison display.
Beta.11's enlarged `!!` rendering, dense-combat performance, additional camera/
resolution combinations, survivor poison expiration and a surviving guaranteed
positive critical hit remain separate validation targets. Sampling can merge
multiple changes or hide damage canceled by healing.

[Crash triage](floating-damage-crash-triage-2026-10-11.md) records a Unique-browser
fault and distinct cleanup signatures without establishing observer causation.
No stability claim or speculative native patch follows from those reports.
