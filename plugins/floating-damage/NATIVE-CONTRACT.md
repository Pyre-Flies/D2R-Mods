# Floating Damage native contract — beta.11

Reviewed 2026-10-11. All addresses and layouts below are qualified for the
tested loader-hosted profile. Full byte guards are versioned in the source
headers and the linked research records; no future-build offsets are inferred.

## Admission

| Module | Profile / SHA-256 |
|---|---|
| D2RLoader.exe | 1.3.1-beta; image size 90,451,968; `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| D2RCore.dll | 1.3.1-beta; `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |

Both disk identities and all six base windows in `src/native_profile.h` must
match. Optional combat, poison and critical gates are independent; rejection
disables only the unsupported path. Main-profile rejection leaves the plugin
inert. DLL ABI manifest, embedded defaults, exports and metadata are tested.

## Owned HP observations

| Location | Contract |
|---|---|
| Host `0x2A9E020`; writer `0x4057AE` | Local authoritative game global; writer guard `4889056B8869024883C428C3` |
| Game `+0x2230` / `+0x2630` | Player/monster bucket arrays, 128 pointers; bucket = ID & 127 |
| Unit `+0/+4/+8` | uint32 type/class/ID |
| Unit `+0xD8` / `+0x158` | Owning game / hash-chain next; preserved lookup witness `0x9F287`, `488B8858010000` |
| Unit `+0x38`; path `+0/+4` | Dynamic path; uint32 16.16 world X/Y |
| Unit `+0x88`; stats `+0x1C` | Stat root; signed selector chooses composed `+0xA8` when negative, otherwise base `+0x30`; reviewed Core getter `0x3D8BD0` |
| Stat array `+0/+8/+0x10` | Pointer / uint64 count / capacity; maximum 2048 entries |
| Stat entry, 16 bytes | uint32 layer/id, signed int64 value at `+8` |
| Stats 6/7/172 | HP/max HP (8-bit fixed point), alignment; nonzero alignment excluded |

Reads use bounded own-process copies on the authoritative game/combat/death
thread. Render callbacks use only owned numbers/positions. Game identity,
target identity, bounds, bucket ownership and cycles are checked each sample.
Failure clears observations. Address keys distinguish reused IDs but are not
retained for later dereference. No native stats or items are allocated/freed.

The SDK's 56-byte MonsterDeathEvent has a 12-byte identity at `+24` and runs
after normal drop processing, so HP need not still be readable. A confirmed
death can consume a matching cached remainder at most two seconds old. Zero
sampling and retirement prevent duplicates; disappearance alone emits nothing.

## Combat, poison and critical provenance

The observer at host `0x44CE80` uses Win64 arguments RCX=game, RDX=attacker,
R8=defender, R9D=missile flag, fifth=damage pointer. It forwards exactly once,
with unchanged arguments/RAX. Entry guard is 48 bytes and the HP-consumer
witness at `0x44D062` is 54 bytes; both are defined in `src/combat.h`.

Owned packet size is `0x138`. Components: physical `+0x18`, fire `+0x20`,
lightning `+0x2C`, magic `+0x30`, cold `+0x34`, poison `+0x38`; poison length
`+0x3C`, total `+0x134`. Nonnegative component sum must match total, and exact
HP endpoint coverage must match the target/game/session. Magnitude is always
observed HP loss, including supported immediate/delayed killing remainders.
Missing post-hit stats can retain an owned lethal candidate for at most 500 ms;
only a matching supported death/zero endpoint resolves it. No packet is written.

Periodic poison requires state ID2, root heads `+0x90/+0x98`, node state
`+0x20`, next `+0x68`, layer-zero stat74, float32 expiration `+0x24` and game
uint32 frame `+0x170`. Stat74 alone normalizes witnessed signed-int32 values
inside widened cells. Nonnegative base regen plus negative poison rate must
exactly explain negative composite regen, while HP loss uses the full poison
rate. Both endpoints require the same rate/expiry, 1–8 frame advance and exact
predicted HP loss; supported deaths use actual remaining HP. Setter `0x2F7AB0`
(26 bytes) and consumer `0x451A25` (7) are layout guards only, never invoked.
This is guarded state/endpoint inference, not a periodic native-event ABI.

Critical result is uint16 packet `+4`, mask `0x2000`; adjacent `+6` is excluded.
Guards: builder `0x44C030`/44 bytes, passive query `0x44C320`/20, deadly query
`0x44C374`/20, common writer `0x44C3C8`/17. The writer doubles native physical
and ORs the marker. Mastery `0x44C2B9`/31 is a matched research lead, not part
of admission or a callable helper. Exact owned HP chains carry marker metadata;
unmatched gaps/healing suppress it, and synthetic poison intervals are unmarked.
The display never doubles HP or infers critical hits from size. Six marked
lethal weapon controls, eleven unmarked lightning controls and 33 unmarked
surviving physical controls establish the shared marker on those tested paths.

## Projection and drawing

| Host RVA | ABI / guard length |
|---|---|
| `0x7E65A0` | Renderer accessor, `void* __fastcall()`; 32 |
| `0x7E7CE0` | Pick witness, `bool __fastcall(void*,float*,float*,uint64 packedXY)`; 138 |
| `0xCEAED0` | ImGui draw-list accessor, `void* __fastcall()`; 36 |
| `0xD03A30` | Clip push, `void __fastcall(void*,uint64 minXY,uint64 maxXY,bool)`; 69 |
| `0xD03760` | Clip pop, `void __fastcall(void*)`; 32 |

Four current-camera samples validate inverse projection via bounded world
coordinates, a nonsingular basis, player proximity and fourth-point agreement.
A balanced callback-scoped clip push/pop handles the admitted Core float2
bridge mismatch. There is no global renderer patch or extra drawing hook.
SDK canvas/metrics are callback-owned. All text layers measure/draw the same
formatted string and effective size; optional SDK filled-rectangle backing
uses bounded geometry, zero flags/reserved and the same fade. Missing metrics
or rectangle support skips backing. Soft shadow uses weighted text passes.

## Commands and lifetime

Bounded local debug-console callbacks consume arguments synchronously. The
vendored backend resolves native parser getter `0x2E6550`, registration
`0xA281D0` and witnessed caller `0x2E87C5` through unique patterns, PE function
starts and definition-field readers. It registers on the UI thread without
an input or chat-packet detour. Registration result0 and slash toggles/motion
were verified live. Failure preserves the console fallback.

Native parser callbacks pin the DLL until process exit. Stop disables/drains
them; no unregister contract is known. Restart to replace/remove the plugin.
Real toggles clear HP/death state and invalidate queued generations; status
and idempotent actions preserve state. Capture is opt-in, bounded to 128
records, and stops on game exit. No borrowed native event/packet is retained.

## Evidence and limits

Seven automated suites validate models, readers, adapters, chat cancellation,
ABI/resources and exact artifact metadata. Live numbers/projection, lethal and
surviving component/poison/mixed behavior and the native marker are established.
The beta.11 enlarged suffix display, dense combat, poison expiration on a
survivor and broader resolution/session behavior remain pending. The reported
Unique-browser crash predates the critical presentation; root cause is unproven.

Canonical records (public source):

- [HP and projection](https://github.com/Pyre-Flies/D2R-Mods/blob/main/docs/reverse-engineering/floating-damage-numbers-feasibility.md)
- [Component and poison contract](https://github.com/Pyre-Flies/D2R-Mods/blob/main/docs/reverse-engineering/floating-damage-elemental-probe.md)
- [Critical/Deadly Strike marker](https://github.com/Pyre-Flies/D2R-Mods/blob/main/docs/reverse-engineering/floating-damage-critical-strike-probe.md)
- [Native commands](https://github.com/Pyre-Flies/D2R-Mods/blob/main/docs/reverse-engineering/floating-damage-chat-command.md)
- [Crash triage and attribution limits](https://github.com/Pyre-Flies/D2R-Mods/blob/main/docs/reverse-engineering/floating-damage-crash-triage-2026-10-11.md)
