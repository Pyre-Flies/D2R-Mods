# Floating Damage: native Critical / Deadly Strike marker

Reviewed 2026-10-11 for `0.1.0-beta.11`. A shared physical-doubling writer and
result bit are established on the reviewed build. Six lethal mixed weapon hits
retained the bit; 33 complete surviving physical-bearing controls kept it clear.
A surviving guaranteed-positive hit and actual enlarged rendering remain untested.

## Exact artifacts and provenance

| Artifact | SHA-256 |
|---|---|
| Loader-hosted D2R image, `D2RLoader.exe` 1.3.1-beta | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| `D2RCore.dll` 1.3.1-beta | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Older decrypted analysis image | `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D` |

Normal plugin admission requires both current hashes and its code windows.
The older analysis image has no parsed exception-directory function ranges;
bounded linear disassembly supplies search regions, not function boundaries.
Aligned instructions and actual prologues establish the final witnesses.
All five windows below matched the reviewed live main module using read-only
access. No memory was changed, suspended or called by that audit. Raw captures,
image filenames, deployment receipts and absolute process addresses are excluded
from public source.

Classic [D2MOO damage header](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/include/UNIT/SUnitDmg.h)
and [implementation](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Game/src/UNIT/SUnitDmg.cpp)
provided the search lead for mastery/passive Critical Strike and item Deadly
Strike sharing physical doubling. Those are not D2R ABI evidence. The reviewed
D2R physical field is independently established at `+0x18`, not classic `+0x08`.

## Writer and branch witnesses

All addresses are game-image RVAs. The mastery interior is an additional
static/live witness; display admission checks the independent builder/passive/
deadly/writer windows and the existing combat observer.

| RVA / kind | Exact bytes | Established meaning |
|---|---|---|
| `0x44C030`, builder entry, 44 bytes | `40565741544881EC10050000488B0585F257024833C448898424D004000048894C2468498BF94D8BE0488BF2` | Binds incoming R9 to RDI (packet), R8 to R12, RDX to RSI. Full arguments/return/ownership are not reconstructed. Never called or hooked by the plugin. |
| `0x44C2B9`, mastery interior, 31 bytes | `488BCEE8EF7FFDFF4885C0745A458D4E024533C0488BD0488BCEE81812EFFF` | Calls `0x4242B0` with RSI, then `0x33D4F0` with RCX=RSI, RDX=returned pointer, R8D=0, R9D=2. Positive result is compared to modulo-100 draw; `0x44C31A` branches to the writer. Mastery interpretation is qualified by source comparison; helper ABIs are incomplete. |
| `0x44C320`, passive query, 20 bytes | `4533C0BA51010000488BCEE8F08CEAFF8BD885C0` | RCX=RSI, EDX=337 (`0x151`), R8D=0 calls `0x2F5020`; success branch `0x44C372` reaches the shared writer. |
| `0x44C374`, deadly query, 20 bytes | `4533C0BA8D000000488BCEE8DC98EAFF8BD885C0` | RCX=RSI, EDX=141 (`0x8D`), R8D=0 calls `0x2F5C60`; failed branch `0x44C3C6` skips the writer. |
| `0x44C3C8`, shared writer, 17 bytes | `8B471803C0894718B80020000066094704` | Doubles signed 32-bit physical at `+0x18`, then ORs `0x2000` into the unsigned 16-bit result word at `+0x04`. |

```asm
44C3C8  mov eax, [rdi+18h]
44C3CB  add eax, eax
44C3CD  mov [rdi+18h], eax
44C3D0  mov eax, 2000h
44C3D5  or  word ptr [rdi+4], ax
```

The three chance paths use `0x34A1E0`, with an inspected multiply/add seed
update followed by integer remainder modulo 100. The full helper ABI is not
established and is never invoked by this plugin. A 100-percent chance passes
that comparison regardless of the draw. `0x44C2B4` skips the mastery lead when
R14D is nonzero; the plugin neither interprets nor changes that context.

Reviewed ItemStatCost tables identify `passive_critical_strike` as 337 and
`item_deadlystrike` as 141. Related mastery melee/throw critical IDs are 344/347;
no new runtime helper or stat reader uses those IDs. Table operands support the
passive/deadly names independently of the historical header. Skill chance is
table-dependent; a skill level alone is not a guaranteed-positive control.

## Owned marker observation and controlled evidence

The existing synchronous `0x44CE80` observer copies `0x138` bytes. Diagnostic
fields isolate raw 32-bit `header0` at `+0`, unsigned 16-bit `result4` at `+4`
and adjacent unsigned 16-bit `extra6` at `+6`. The adjacent word is not part of
the critical bit. `critical_candidate` requires the optional guards plus
`(result4 & 0x2000) != 0`. No attacker attribution, new hook, helper invocation
or game write is introduced. Capture is opt-in, capped at 128 log records and
stops on game leave/unload. Guard refusal disables marker interpretation only.

| Controlled evidence | Completed hits | Observed marker and endpoint behavior |
|---|---:|---|
| User-reported over-100-percent Deadly Strike weapon setup | 6 | Mixed physical/lightning/poison packets retained `result4=0x2007` before/after; all lethal. Post totals exactly equaled the six-component sum; supported death resolved actual cached remainders. |
| Physical-bearing negative control setup | 33 | `result4=0x0005`, clear `0x2000`, valid surviving HP endpoints exactly matched total. Includes 32 pure physical hits and one physical+poison hit. |
| Separate pure-lightning path | 11 | Physical/poison zero, lightning/total 3840 raw, `result4=0x4021`, bit clear, exact surviving loss. This is not the zero-chance physical control. |

Chance values and attacker identities were not independently read; positive
and negative setup labels reflect the controlled user-directed tests. Packet
behavior is independently observed. Incomplete records cut off by the capture
cap were excluded from completed-hit counts.

The surviving mixed negative control lost 17088 raw HP: physical 16970 plus
poison 118, other components zero, bit clear before/after. Initial 13 merged
number updates retained mixed mask 33; later 48 used poison-only mask 32.
Poison base74 +70 plus state −118 explained composed −48, and each observed
advancing frame lost 118 raw while expiration remained ahead. Poison presence
therefore does not itself set the native marker. The unnamed vector tag28 had
a rate candidate distinct from final poison; it is not added as extra damage.

All six positive packets retained the native bit through Apply; they do not
prove every weapon/missile/conversion path preserves it or directly observe the
builder's before/after multiplication. A surviving guaranteed-positive path
remains an additional test. The shared bit cannot distinguish Critical Strike
from Deadly Strike or identify the chance source/attacker. Large HP loss,
ordinary variance and lethal clamping cannot replace the bit, and displayed
HP must never be multiplied a second time.

## Presentation and endpoint policy

Beta.11 carries owned critical metadata with native hits and displayed numbers.
An exact complete HP chain containing a marked hit receives emphasis. Gaps,
healing, stale/foreign/reused identities and expired pending hits clear that
association. Unknown after-HP is not zero. Supported synchronous/delayed death
emphasizes the actual cached remaining HP with no overkill substitution.

Synthetic poison-only intervals are unmarked. Small poison ticks merging into
an initial marked mixed group can retain `!!` because the group contains that
marked event. Fresh poison-only groups and later periodic killing remainders
remain ordinary. The marker is never attached to ongoing poison state.

`critical_hits = true` enables `!!` and enlargement; `critical_size_scale = 1.35`
accepts a multiplier 1–2. Precision and positive sub-unit formatting are retained,
including `<1!!`. The SDK measures the complete suffixed text at the effective
size, centers that width and uses the same size/string for shadow, outline and
fill. Backing follows measured bounds. Colors, motion, lifetimes and fading
remain independent; disabling element colors does not disable critical evidence.
A critical guard mismatch suppresses emphasis while preserving other admitted
HP and component behavior.

Seven MSVC Release `/W4 /WX` suites cover positive/negative association, independent
colors, guard refusal, foreign/stale/gap refusal, immediate/delayed death,
remaining-HP preservation, mixed merging, poison-only isolation, suffix/measurement/
glyph/backing geometry and bounded configuration/formatting. Actual hook and SDK
adapters are compiled into fixtures. Exact-DLL checks cover ABI/config resources,
exports, identity/version and role. Simulated elemental-critical cases do not
establish new live elemental paths.

Live matching and the six-positive/33-negative marker evidence are confirmed.
Beta.11's actual enlarged `!!` rendering and dense-combat readability remain
pending. No standalone native critical helper is called and no additional
native patch is justified by the display fixtures.

## Rejected leads and search corrections

- `0x41D928`, bytes `0C0D`, decoded `or al,0x0D` inside a noisy linear region:
  data/desynchronization false lead, not a bit-13 writer. Immediate 13 is
  relevant to `bts` only, not to ordinary OR masks.
- `0x4344E5` and `0x439A18`, both `2500200000` (`and eax,0x2000`): inspected
  list/query filtering contexts following `0x2EFDE0` and involving
  `0x2F0750`; no damage-record write or physical doubling witnessed.
- `0x444549`, `83C820` (`or eax,0x20`): an unrelated result derived from a
  byte at an object `+0xC0`, followed by a shift; not a damage result writer.
- `0x44C0F1`, `830F20` (`or dword ptr [rdi],0x20`): changes the record's
  first word, not the critical bit in `+0x04`.
- `0x44ADA5`, `BAB7000000` (`mov edx,183`): unrelated stat-query lead,
  not a named critical/deadly chance field in the inspected table.
- `0x44B6C0`, `037B18`, and `0x44EC7F`, `41034718`: add physical into an
  aggregate, not a physical-times-two operation. Nearby `0x44B6F2` ORs 2
  into the result word (`66834B0402`), a separate terminal-result lead;
  not used as a critical marker.

Searching only OR instructions with literal immediate `0x2000` missed the
actual writer because the compiler first loads the mask into EAX, then ORs
AX into memory. The named deadly stat query and neighboring physical doubling
located the real block without promoting literal-mask matches to semantics.
