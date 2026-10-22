# Native day/night lighting restoration

Investigated October 21, 2026. Canonical addresses, bytes, sizes and SHA-256
identities are in [native-profile.json](native-profile.json); generated C++
witnesses are in [src/native_profile.h](src/native_profile.h).

## Finding and evidence level

On the admitted D2RLoader 1.3.1-beta build, Core export ordinal **118**, RVA
`0x1F3830`, wraps the native time-of-day lighting setter. It calls the setter
only when Core's independent mode flag bit 0 is clear **and** the native
`NoDebug` byte is zero. Normal mode has a nonzero `NoDebug` byte, so the wrapper
returns without forwarding a native lighting update.

This is a **static witness**, with the original suppression and proposed correction
also exercised as machine code in owned executable memory. It agrees with the
user's reported debug-mode difference. It is not a live observation of the
installed game's renderer, a full day/night transition or multiplayer behavior.

The exact installed disk host and Core hashes were checked. The private game
analysis image is the September 27 all-sections capture (90,447,900 bytes, hash
`A09EA269F8FC10C538DC9A05E4E81E004EC6DFC84AA0F685D3527A974A542A87`).
It is historical material, not a current runtime snapshot. Startup separately
checks the relevant current loaded code, export identity and pointer bindings.
The protected disk `D2R.exe` is version 3.3.93787, 32,107,216 bytes, SHA-256
`1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7`.
Never publish the private capture or use its absolute process pointers as guards.

## Bounded native path

All game RVAs refer to the loader-hosted game image, not the disk host's code.
Core RVAs refer to the separate loaded `D2RCore.dll`.

1. Game `0x7CD8F0` receives the native input through RDX: float blend at `+0`,
   signed 32-bit period at `+4`. It calls renderer getter `0xE21140` on the
   existing native thread. No getter ABI is independently invoked by this plugin.
2. Game `0x7CD8FE` checks renderer byte `+0x11909`, the native **Override Time Of
   Day** control. A set override exits. Otherwise it derives the environment at
   renderer `+0x11760`, loads blend into XMM1 and period into R8D.
3. Game tail jump `0x7CD91B` reaches thunk `0x3E2B136` (`FF 25 F4 F4 FF FF`).
   Its slot `0x3E2A630` must resolve to the admitted Core export 118.
4. Core wrapper `0x1F3830..0x1F3865` retains its caller's RCX, XMM1 and R8D.
   Its first gate reads Core byte `0x716A38`, tests bit 0 and exits if set.
   The flag's wider meaning is **unresolved**; the plugin retains this gate.
5. Core `0x1F384D` reads pointer slot `0x7050C8`, whose descriptor at `+8`
   contains game RVA `0x2A96102`. Runtime admission requires the pointer to equal
   host base plus that RVA. `CMP BYTE PTR [RAX],0; JNE 0x1F3860` suppresses the
   update outside debug mode.
6. Core `0x1F3859` calls pointer slot `0x704E40`, whose descriptor at `+8`
   contains game RVA `0xF5B190`. Runtime admission requires that exact binding.
7. Native setter `0xF5B190` is the complete 16-byte leaf:
   `F3 0F 11 89 94 01 00 00 44 89 81 90 01 00 00 C3`.
   It writes XMM1 to environment float `+0x194`, R8D to signed period `+0x190`,
   then returns. The witnessed contract is
   `void __fastcall(void* environment, float blend, int32_t period)`.
   The plugin never manufactures, retains or directly dereferences this object.

## Patch and ownership

Replace Core `0x1F3857` bytes `75 07` with `90 90`. The first Core gate, native
time override and the one existing setter call remain intact. The game supplies
all timing and area values. The plugin does not write global debug/cheat state,
change simulation speed, force a period or call the setter independently.

Publish the containing **aligned 16-byte block** at Core `0x1F3850`:

- Original: `74 18 51 00 80 38 00 75 07 FF 15 E1 15 51 00 90`.
- Enabled: `74 18 51 00 80 38 00 90 90 FF 15 E1 15 51 00 90`.

The SDK mutation service is game-executable-relative; this separate Core block
is outside that address space. `OwnedPatch` requires CMPXCHG16B support, exact
original bytes, 16-byte alignment and committed executable memory. It briefly
permits writing, uses `_InterlockedCompareExchange128`, flushes the instruction
cache and restores code protection. Unload restores the original block only if
the entire 16 bytes still match this plugin's replacement. An intervening owner
is preserved. Protection failures are reported and require a restart if cleanup
cannot complete. No trampoline, allocated relay or plugin callback is published,
so a later owner cannot retain a pointer into this plugin through this patch.

Startup also checks the full 54-byte wrapper and caller, the thunk, setter,
descriptor offsets and three current pointer bindings. An unknown modification
refuses the feature without changing game lighting. No arbitrary predecessor
detour is admitted. Source scans found no overlapping constants in the six
production plugins or the maintained companion sources. This is not a live
coexistence test or a general third-party compatibility guarantee.

## Saved chat toggle

Beta.2 adds native slash registration through the support sources copied from
Always Show Sockets; [vendor/provenance.json](vendor/provenance.json) records the
exact inputs. It installs no additional detour or code patch, uses the SDK UI
queue and shared UI listener for deferred registration, and does not depend on a
named chat mapping. The existing [shared chat contract](../../../D2R-Mods/docs/reverse-engineering/floating-damage-chat-command.md)
describes the parser ABI and guarded discovery. Its RVAs were also checked
statically against this experiment's exact September 27 capture:

| Game RVA / layout | Contract |
| --- | --- |
| `0x2E87C5` | Unique command-loader caller; first relative call resolves the parser getter. |
| `0x2E6550` | Primary function start; `uint64_t* (__fastcall*)()`. TLS getter prologue matches, wildcarding only the RIP displacement. Preserve immediate `0x126C`. |
| `0xA281D0` | Unique registration prologue and primary function start; `uint32_t (__fastcall*)(uint64_t,const Definition*)`. |
| Registration `+0x1BB`, `+0x1CA`, `+0x1D9`, `+0x1E8` | Exact `49 8B 96 disp32` readers require definition offsets `0x188`, `0x190`, `0x198`, `0x1A0`. |
| Definition `0x1C0` bytes | Version 1; name/description at `+4`/`+0x44`, aliases `+0x84`, execute/help/completion at `+0x188`/`+0x190`/`+0x198`, literal translation `+0x1A0`, children/count `+0x1A8`/`+0x1B0`, flags `+0x1B8`. |

Discovery requires one caller and one registration match in readable `.text`,
both target entries at primary `.pdata` starts, the getter prologue and record
reader guards. Execute/help use `void (__fastcall*)(const char*)`; translation is
`void (__fastcall*)(const char*,char*,size_t)`. `/daynight` bounds borrowed
arguments to 32 characters, then synchronously consumes them. Registration
result 0 means registered, 7 duplicate; other results refuse dispatch. No network
message is sent. Replies use SDK console output and warnings also reach the log.

The parser retains callback pointers and has no supported unregister. The helper
pins its DLL with `GetModuleHandleExW(PIN)` and reserves process-lifetime slots.
Stop disables and drains dispatch before acquiring the plugin state mutex, then
conditionally restores the lighting block. Retained callbacks become inert.
Replacing this DLL or reusing its command name requires a game restart.

The command rereads at most 16,384 bytes through SDK `ReadConfig`, prepares a
conservative single-boolean edit, revalidates native bytes/bindings, applies the
transition, then saves through `WriteConfig`. A failed save attempts to restore
the previous native state; rollback failure is reported as requiring restart.
TOML BOM, line endings, comments and unrelated text survive. Duplicate target
keys/sections, quoted or ambiguous target syntax, multiline strings, unreadable,
oversized and unterminated input are refused. Read-only status/help never write.
Missing settings default to enabled; unreadable/ambiguous startup config starts
off and reports a warning. The embedded TOML is the SDK default, not an override
of an existing user's file. Beta.2 off restored only the admitted original block and retained the last
lighting. Beta.3 adds the explicit daylight reset described below. An active-state guard normalizes only the two owned NOPs
when checking the complete wrapper. Foreign changes refuse subsequent toggles
without changing the config or overwriting the later owner.

## Beta.3 instant lighting and daylight on off

The exact additional bytes are in `native-profile.json`: `RendererGetter`,
`RendererOverrideControl`, `ScriptSetterCall`, and `NormalCycleTable`. They are
separate preview guards, checked on each preview access. A failed preview guard
does not prevent the initial automatic cycle from loading. The SDK UI queue
serializes setting changes; normal rendering/gameplay speed is unchanged.

| Game RVA / field | Witness and boundary |
| --- | --- |
| `0xE21140..0xE211E0` | Complete renderer singleton getter. Its initialized path reads slot `0x34F6480`; slow path allocates `0x1408A0` bytes, constructs through `0xDF9D40`, publishes the pointer and registers process cleanup. Preview reads the slot without invoking this allocating/TLS getter. |
| `0x34F6480` | Current process-lifetime renderer pointer. Borrow again for every action and compare identity before ownership-sensitive restoration. Never dereference a remembered pointer after the slot changes. |
| `0x7CB650..0x7CB6AE` | Native `OverrideTimeOfDay` script control clears renderer `+0x11909` at `0x7CB68B` or sets it at `0x7CB69D`. The normal caller's already-guarded branch suppresses forwarding while this flag is set. |
| `0xE250DB..0xE2513C` | Native `SetTimeOfDay` tail obtains the renderer, derives `+0x11760`, clamps the integer stage to 0..5, clamps the fractional blend to 0..1, and calls the complete setter at `0xF5B190`. |
| `0x2376760`, six 12-byte records | Existing cycle order: sunrise, morning, noon, afternoon, sunset, night. The HD display stage is the six-step index, distinct from the common environment's four-valued day/dusk/night/dawn period. Existing variable names saying period in the leaf ABI denote this integer display-stage input. |
| Renderer `+0x118F0..+0x118F7` | Eight-byte pair: signed stage at environment `+0x190`, float blend at `+0x194`; normal heap alignment permits 64-bit ownership comparisons. Noon is stage 2/blend 0; night is stage 5/blend 0. |

Preview refuses missing/unreadable singleton data, unaligned fields, nonfinite or
out-of-range values, nonwritable pages, altered guards and the independent Core
mode flag's bit 0. It does not change NoDebug, debug state, the native environment
clock, server state or provider slots. There is no additional hook or network
message. The existing leaf is called with exactly the native RCX/XMM1/R8D ABI.

A forced mode claims the override flag with compare-exchange only from zero.
It retains the original pair and records its exact published pair. Changing an
owned forced mode requires the same singleton, flag 1 and matching pair. An
existing manual override or later changes are refused. Auto conditionally
restores the original pair and clears the flag, then normal updates resume at
the current clock. Off publishes noon, clears only its owned flag and closes the
normal-mode Core forwarding gate. Global debug mode can independently allow
forwarding through the unmodified Core wrapper; `day` is the explicit held state.

Save failures restore a checkpoint only while the current singleton, pair and
flag match this operation's published result, then restore the prior Core state.
Unloading cancels pending work, drains listeners and restores an owned held mode.
If another owner has changed it, cleanup leaves that owner's state intact.
Defaults remain auto; old configs with no mode are read as auto. Settings edits
preserve BOM, line endings, comments and unrelated text, and reject duplicate,
unknown or ambiguous mode values. Successful day/night/auto/off changes save to
the active TOML and are logged with actual renderer fields.

## Runtime evidence from beta.2

On October 21 the user confirmed that the natural day/night cycle is visibly
working. The plugin log records successful native chat registration and saved
off/on commands. A read-only observation verified the actually loaded beta.2 DLL
hash `EF131DE635BE2B169D6D275EA0B6255A522992C36B104D0EADCD2F96FD9F862D` from
the Reimagined mod, NoDebug=1, independent Core flag=0 and renderer override=0.
Stage remained 2 while blend advanced 0.027385375 -> 0.028321654 -> 0.02925671
across approximately 1.5 seconds. This confirms current native field progression;
it is not a stopwatch qualification of every cycle phase or hosted behavior.

The beta.3 command fixture executes the actual native setter into owned renderer
memory, checks noon/night/auto and daylight on off, fresh startup from saved night,
save-failure restoration of the prior real fields, Core gate refusal, preserved
foreign overrides and queued-command cancellation on unload. No live beta.3
visual test has yet been performed.

## Other discoveries and rejected approaches

These additional RVAs are navigation records for the same historical game image
and named Core artifact. They are not admitted callable APIs in this plugin.

| Module / RVA | Static observation and limit |
| --- | --- |
| Core `0x1F80C0` | Public `IsDebugModeEnabled` export 3 returns false when flag `0x716A38` bit 0 is set; otherwise returns the inverse of byte through `0x7050C8`. |
| Core `0x1F3440`, `0x1F347F` | Debug toggle writes zero through the NoDebug pointer to enable debug mode; subsequent native setup and saved-cheat handling are unrelated to this feature. |
| Core `0x1F3090`, `0x1F30B2` | Debug-off path writes one through that same pointer and clears multiple cheat toggles. Do not reuse the whole path for lighting. |
| Game `0x352270` | `EnvironmentProcess` path identified by metadata `0x1D010C0`, referencing the function label and `D2Common/src/Environment/Env.cpp`; calls the environment clock update. Full API/lifetime not admitted. |
| Game `0x951EA` | Client update calls `0x352270` with existing act/room and native float delta. The surrounding clock path is not changed. |
| Game `0x352DF0` | Adds the existing delta to environment `+8`, advances a six-step sequence, and applies native area-specific timing. Static presence is not a live ticking measurement. |
| Game `0x351D90` | Resolves environment through `0x2EF120`, returns `+4`; optional output divides integer-converted `+8` by `+0x2C`. Core descriptor `0x6FF220` consumes it for time-sensitive stat calculations; not a renderer API. |
| Game `0x351DF0` | Copies environment sequence `+0`, integer-converted clock `+8`, and mode `+0x34` to caller outputs. Not used by the plugin. |
| Game `0x7CB650` | `OverrideTimeOfDay` script callback can set or clear renderer `+0x11909`; `0xF2FFAA..0xF2FFDA` exposes that byte in the renderer debug panel. |
| Game `0xE24FE0`, `0xE25138` | `SetTimeOfDay` script callback parses/clamps native inputs and directly calls `0xF5B190`; separate from normal gameplay forwarding. |
| Game `0x4179C0`, `0x2AA604C` | Debug startup calls a DWORD setter here. Wider flag semantics unresolved; not needed for day/night and not modified. |

Rejected: globally enabling debug mode; suppressing its public status function;
changing the environment simulation; forcing time each frame; replacing the
game caller's jump to bypass **both** Core gates; or forcing the renderer override
byte. The observed Core wrapper supports the smaller, guard-preserving change.

The [official loader documentation](https://d2rloader.net/docs.html) and
[release notes](https://d2rloader.net/changelog.html) identify debug tools and manual
time/environment controls; the cause above comes from local binary evidence.

## Validation and remaining work

- Static verifier: exact three image identities, six byte witnesses, export 118
  ownership, two changed bytes, retained first branch and native call.
- Executable replay: witnessed game caller -> thunk -> Core wrapper -> actual
  setter instructions, with relocated operands in owned pages. The unpatched
  normal-mode command returns 1 with `RED: normal mode suppresses...`.
- Patched replay: all 16 combinations of debug, mode-flag bits and override,
  plus six periods at four blend values. Checks RCX/XMM1/R8D, exactly one call,
  only the two native field writes, unchanged flags and exact unload restoration.
- Guards: every byte in the six base and four preview windows, each stale pointer binding, unreadable
  images, unaligned/unknown/duplicate publication, restored page protection and
  later-owner preservation. These tests use substitutes, not loaded companions.
- Artifact: real DLL resource load, exactly one DWORD ABI manifest, Windows
  version resource, embedded enabled TOML, lifecycle exports, identity/ABI/client role, unsupported
  executable, wrong ABI and short/null context refusal. This is not an actual
  D2RLoader plugin-startup observation.
- Command: actual retained native dispatch with substituted parser/SDK services,
  UI queue refusal/retry, immediate real wrapper/setter changes, file-backed TOML
  persistence and fresh runtime startup, read/write failures with state rollback,
  preserved unrelated text, foreign ownership refusal and inert callbacks after
  Stop. The fake parser never invokes the actual game's registration function.

Remaining live tests: beta.3 day/night/auto/off comparisons and saved-mode restart;
controlled debug off/on comparisons;
travel among outdoor and indoor areas and acts; exercise native manual override;
exit/rejoin; and verify offline and hosted client behavior. Include current
terrain/ultrawide plugins in the visual comparison because they affect the final
image. Do not claim hosted synchronization from the client-only replay.
