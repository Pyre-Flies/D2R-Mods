# Remote item sender compatibility - 2026-10-15 / 2026-11-02

## Failure and scope

On a client connected to a separate TCP/IP server, the user reports LB+X single
transfer and LB+L3 Stash All do nothing; LB+R3 belt refill works. Rev.78 admits
the unhooked sender during startup, before companion plugins load. A later
Global Chat hook invalidates its exact companion hash/RVA profile. Single-item
stash capture used to return false without explaining that rejection.

Read-only live inspection of client PID 25232 identified these loaded artifacts:

| Artifact | SHA-256 |
| --- | --- |
| D2RLoader.exe | `93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10` |
| D2RCore.dll | `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089B948fdeb5a2` |
| QOL rev.78 | `5f1d2fa8565b8355b2bc1f9547acbb2bd41f1f11d243ddcdee8f46f086434353` |
| Global Chat 0.5.1 (Debug) | `dfb3dc947421283cc7e90f63c28170e81263020b8c68a3979ba2ee506c9e026f` |
| Global Chat 0.5.1 (Release) | `4b1e07ecffd2f58e87ca134094b950c54a9343e393babec9a2a5878277590b36` |
| Maps 0.11.7 | `c459f2092a6b35e5465999484e0f232c60712bccf13977ee4fe399fe8cb892b6` |

The live `Core+0x822DD0` sender begins `FF 25 00 00 00 00`, followed by an
absolute pointer to **Chat+0x54330** (Debug build) or **Chat+0x40FB0** (Release
build), replacing the older `Chat+0x538B0` target.
The recorded sender tail remains unchanged. This establishes an admission
blocker, not proof that Global Chat drops item requests. The chat hook owns a
shared request sender; it is not a stash-specific hook.

## Relocation-aware forwarding proof

`chat_forwarding_profile.h`, `chat_forwarding_contract.h` and
`chat_forwarding_memory.h` qualify the ordinary non-chat forwarding path, without
requiring a particular whole-Chat DLL hash or private companion RVA. This is
limited to QOL's separately guarded native item-request builders, whose packet
opcodes differ from chat `0x15`. It is not a public sender API or general hook
chain. No change to Global Chat, network protocol, request serialization,
destination choice or input mapping is involved.

The existing exact Core hash, game thunk/slot, native item actions, identity,
session, destination, eligibility, scheduling and observation guards remain.
Both historical Chat artifacts and the Release-compiled Chat artifact remain
supported by complete forwarding proofs. A failed proof cannot fall back to a
whole-file hash: that identifies the disk file, not patched helper code or
memory protections in the loaded image.
Maps stat-reader admission is unchanged; this repair does not qualify new Maps
stat hooks.

The shared request guard also covers remote ordinary Cube transfers, Quick
Identify and Identify All. Include these in live regression testing; their
existing target, supply, charge-consumption and request-ordering rules remain.

The new proof starts only from the installed Core absolute jump and its named,
loaded `d2rl-global-chat.dll` owner. It reads PE sections and actual memory
protection on every admission; it does not scan for a similar function or cache
a former admission. All operative instructions are exact. The only variable
code operands are explicitly listed RIP-relative data references and direct
relative calls/jumps. Each operand resolves to a named, checked graph role;
repeated references must agree, distinct roles cannot overlap, and arithmetic
must stay in range. Every helper on the successful non-chat path is checked.

Code must belong to readable, executable, non-writable MEM_IMAGE pages in that
companion's non-writable executable section. The hook object and initialized
security cookie must belong to writable, non-executable data in the same image.
The hook forwards the original pointer and byte count once and preserves the
original result. Both hook-object references and both destructor calls must
agree in Debug builds, and in Release builds all four inlined `lock inc`/`lock dec`
counter references at `+0x40` and the continuation load at `+0x08` must resolve
to the same `0x44`-byte hook object (`Counter == Continuation + 0x38`).

The dispatch gate tests null pointer/nonpositive size and opcode `0x15`. Every
non-chat request returns Continue through the checked epilogue. The original
cookie load and security-check comparison must refer to the same initialized
cookie with zero upper 16 bits. The complete successful /GS return block is
checked. Chat-only parsing and stack-corruption failure handling are outside
this ordinary-path proof; QOL does not invoke or bypass them. A changed gate,
ordinary return, helper behavior, owner, or continuation still refuses.

The checked getter (or inlined Release load) reads the original continuation at
hook object `+8`. That pointer must be executable and contain the exact original
14 stolen Core bytes followed by `FF 25 00 00 00 00` and the exact
Core-entry-plus-14 target. The Core tail is still exact. QOL calls the normal
native route, preserving Global Chat's hook and counter accounting; it never
calls the trampoline directly.

Unrelated chat-only logic, other plugin features, data and image layout may
change without invalidating this proof. Compiler changes to the checked code,
new hook behavior and additional detours deliberately remain unsupported. No
claim of arbitrary future companion compatibility is made.

## Current artifact witnesses

All RVAs in the tables below are relative to the named Global Chat artifacts above.
They are provenance, not runtime lookup constants. Full byte witnesses and
reference offsets are in `chat_forwarding_profile.h`.

### Debug artifact witness (`dfb3dc94...`)

| Role | RVA | Checked bytes | Outgoing roles |
| --- | --- | ---: | --- |
| Sender hook | `0x54330` | 170 | object, constructor, dispatch, destructor, getter |
| Non-chat dispatch gate | `0x541B0` | 86 | cookie, dispatch epilogue |
| Dispatch epilogue | `0x54312` | 22 | cookie check |
| Counter scope constructor | `0x543E0` | 51 | enter |
| Counter scope destructor | `0x54420` | 28 | leave |
| Enter | `0x54440` | 50 | atomic add |
| Leave | `0x544F0` | 50 | atomic subtract |
| Atomic add | `0x54540` | 60 | order helper, identity helper |
| Atomic subtract | `0x54580` | 49 | negate helper, atomic add |
| Negate helper | `0x54530` | 11 | none |
| Order helper | `0x147E0` | 5 | none |
| Identity helper | `0x90E0` | 11 | none |
| Continuation getter | `0x17C30` | 15 | none; object `+8` read |
| Successful /GS check | `0x5FCA0` | 21 | cookie |
| Hook object | `0xA85C0` | 68 data | continuation `+8`, counter `+0x40` |
| Security cookie | `0xA8800` | 8 data | shared load/check identity |

### Release artifact witness (`4b1e07ec...`, 2026-11-02; `659c9f4e...`, 2026-11-14)

| Role | RVA (`0.5.1`) | RVA (`1.0.3` `/O2`) | Checked bytes | Outgoing roles |
| --- | --- | --- | ---: | --- |
| Sender hook | `0x40FB0` | `0x57950` | 107 | inlined counter (`+0x40`), dispatch, continuation (`+8`) |
| Non-chat dispatch gate | `0x40E90` | `0x57830` | 51 | cookie, dispatch epilogue (all 3 non-chat/null/len branches) |
| Dispatch epilogue | `0x40F66` | `0x57906` | 29 | cookie check |
| Successful /GS check | `0x477F0` | `0x5E2C0` | 21 | cookie |
| Hook object | `0x8C690` | `0xAB060` | 68 data | continuation `+8` (`0xAB068`), counter `+0x40` (`0xAB0A0`) |
| Security cookie | `0x8C8C0` | `0xAB2C0` | 8 data | shared load/check identity |

### Maps stat-reader Release (`/O2`) witness (`d2rl-maps.dll` `883,712` B, 2026-11-14)

`maps_quantity_profile.h` (`QolMapsQuantity::ReleaseProfile`) and `maps_quantity_contract.h` (`ValidateRelease`) also admit the `/O2`-compiled `d2rl-maps.dll` `ReadStat` detour alongside the `/Od` `dist/` build (`0x17BC0`), where `StatIndex` is inlined into the hook gate and `stat=70` (`choice=8`) branches directly to the shared return epilogue:

| Role | RVA | Checked bytes | Outgoing roles |
| --- | --- | ---: | --- |
| Stat-reader head (`Read`) | `0xCAA0` | 38 | original continuation slot (`+0x0F`, `kind=Data`) |
| Stat-70 gate & inlined switch | `0xCADB` | 84 | `Return` (`4` branches), `levelSlot`, image base, byte table (`+53 == 8`), jump table (`+32 -> Return`) |
| Return epilogue (`Return`) | `0xCC38` | 21 | restores saved result and returns |

ABI established for these artifacts: sender accepts `(packet pointer, int32 byte
count)` in RCX/EDX and preserves the original sender result in RAX; internal
helpers follow the shown x64 code, not a proposed callable public ABI. Data
addresses and the allocated trampoline vary across processes. Companion module
ownership/lifetime follows the existing loaded-plugin route; this repair adds
no hook, allocation in the game, patch publication, teardown or bypass.

## Validation and delivery

The runtime candidate is based on the released rev.78 source, with only this
repair and rev.79-beta.2 metadata applied. Publication retains the repository's
subsequent user-documentation and minimal runtime-package conventions. The
isolated release checkout excludes unrelated working-tree changes. Main/skill
defaults remain identical to rev.78. Installed plugins/configuration have not
been changed. The publication checkout also passes all 16 current runtime-package
regression checks, retaining the minimal ZIP layout and reference-only defaults.

Automated validation: Release build and all CTest suites passed, including
production integration checks, DLL metadata/ABI/import/default checks, and
single-byte instruction/reference mutation refusals across both the Debug and
Release forwarding graphs. Independent helper reordering and data relocation pass;
foreign/unreadable memory, altered Core tail, changed continuation, uninitialized
cookie and changed operative code refuse. Unreachable chat-only and unrelated
image changes remain admissible. The synthetic x64 executable fixtures exercise
**2,550 opcode/length combinations** across Debug and Release profiles, covering
every non-chat opcode, short and maximum native request lengths, and
null/nonpositive input. Each forwards exactly once with original arguments,
unchanged packet contents, preserved 64-bit return and balanced in-flight
accounting. This invokes only a test callback.

Routine CTest also invokes the production sender guard against a test-only
MEM_IMAGE fixture, substituting only companion module lookup. It verifies real
PE ownership/page protections, altered helpers, changed Core tail, missing or
nonexecutable continuation, missing owner, and compatible chat-only changes.
The production DLL contains no test lookup, image sections or companion binary.

The production `Send` guard also accepts the actual current Global Chat image
and previously supported Chat artifacts mapped with
`DONT_RESOLVE_DLL_REFERENCES` in a standalone test process. That
check executes no companion initialization or companion code; its continuation
slot and synthetic Core/trampoline exist only in the test process. All 11
runtime-packaging regression tests passed using a workspace-local temporary
directory. The two historical artifact hashes are already recorded in
`plugin_coexistence_profile.h`; they are provenance, not sender admission
exceptions. Mutating a helper in either historical image must now refuse through
the production guard. No new runtime dependency is introduced.

The refusal diagnostic says further submission is blocked. It does not claim
that an operation already awaiting confirmation was never submitted; the
operation's finish record retains its submitted status and no-retry behavior.

The live process had closed before the full new graph could be re-read; only
the original live sender target/module identity inspection is claimed. Live
transfer success, destination visibility and remote-server/save persistence
remain unverified and require package-managed client deployment and a user
test. Test one low-value LB+X deposit/withdrawal first, then a small LB+L3 batch,
including Personal, Shared and advanced tabs. Check full destination refusal,
tab/selection changes, rejoin/persistence and existing LB+R3 belt behavior. Also
test ordinary Cube transfers, Quick Identify and remote inventory Identify All,
including exact charge use, insufficient supplies and identified-state persistence.

Private reproduction material is under `outputs/qol-remote-fix-20261015/`:
baseline log, source preparation, bounded disassembly/profile generation,
read-only inspection, build output and tests. Do not commit these local files
or companion/game binaries.
