# Floating Damage / Unique-browser crash triage

Reviewed 2026-10-11. The reported trigger is selecting **Unique** in D2RLoader's
item browser. This record distinguishes repeated exception signatures and
qualified native leads. No isolated reproduction, observer root cause or crash
fix is claimed. Investigation changed no installed plugin, config, save or table.

## Evidence and profile

Three loader last-resort reports and five Windows dumps were inspected.
Report filenames use process startup time; the internal generated timestamp is
the exception time. Dump header times can follow the exception by several
seconds. Public source includes no raw dumps, local process/thread identifiers,
heap addresses, deployment hashes, module paths or private evidence filenames.

| Signature | Reports/dumps | Floating Damage resource versions | Established immediate failure |
|---|---:|---|---|
| Host `0x29394C`, `C0000005` read | 2 | `0.1.0.9`, `0.1.0.11` | Invalid nonzero nested pointer read. |
| Host `0x3289AA`, `C0000005` read | 1 | `0.1.0.10` | RDX is null; read at `+0x50`. |
| Trade Notifications `0x30F95`, `C0000409`, parameter 7 | 2 | `0.1.0.9`, `0.1.0.10` | Fatal-app-exit fast fail during joinable-thread cleanup. |

Host/Core resource versions are `1.3.1.0`, with image sizes
90,451,968/19,791,872. Reviewed current disk identities are:

| Artifact | SHA-256 |
|---|---|
| `D2RLoader.exe` | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| `D2RCore.dll` | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |

These are disk identities, not reconstructed loaded-image hashes. Older
Floating Damage resource versions come from their respective dumps; today's
overwritten DLL path cannot establish an older deployed hash. Module lists
establish which files were active; historical disabled filenames do not.
Trade Notifications has resource version `0.0.0.0`, image size 442,368;
its cleanup analysis below is qualified by compared windows rather than a
complete captured-image integrity claim.

## Recurring browser fault

Both browser-fault dumps validate this 21-byte leaf at host `0x293940`:

```text
488B81800000004885C074080FB68048010000C3C3
mov rax, [rcx+0x80]
test rax, rax
je   +0x293954
movzx eax, byte ptr [rax+0x148] ; fault +0x29394C
ret
ret
```

The invalid read is through RAX+`0x148`. RCX's structure type, the byte's
meaning and pointer ownership/lifetime are unknown. Even small reads at RCX
and RCX+`0x80` are absent from the dumps; offsets alone do not establish an
item/skill/inventory structure.

The leaf does not adjust RSP; both top stack slots contain host `0x2E08BC`.
An older decrypted analysis image has a compatible caller:

```text
2E08B3: 488B4B20       mov rcx, [rbx+0x20]
2E08B7: E88430FBFF     call +0x293940
2E08BC: 413B85F8000000 cmp eax, [r13+0xF8]
```

Its SHA-256 is
`02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D`.
Its `0x293900`/128-byte window matches both fault dumps, but current caller
bytes are absent. The caller remains a static lead, not a current ABI witness.
The older image has no parsed exception-directory ranges; arbitrary linear
starts are not function boundaries.

Stack strings include `Uniques` and item categories, consistent with the user
report. No particular invalid unique row is identified. Further raw candidates
are host `0x2DDD58`, `0x2DDD9E`, `0x2DE2BC`, `0xCE82DF`, `0x2DD16B`, `0xCE5D5A`,
`0x2A02FB`, `0xCCAC08`, `0xCFFC83`, `0xCF762D`, `0x1D42A28`; `0x2A96140` occurs
as a register/stack value. These candidates are not an unwound stack.

The same fault/first return candidate also appeared in earlier triage. The
beta.9 case predates enlarged critical styling, arguing against a regression
introduced specifically by beta.11's size/suffix change. It does not exclude
older corruption or an observer/cross-plugin interaction.

## Separate null-room signature

At host `0x3289AA`, bytes `8B4250` are `mov eax,[rdx+0x50]`, with RDX=0.
The dump validates this 32-byte start window at `0x3289A0`:

```text
48895C2408574883EC208B4250488BDA0FB6F90FBAE0187208E8B2AF0C008B43
```

Raw candidates include host `0x3DA709`, `0x2EF8E3`, `0x4882BE`, `0x410288`
and Core `0x8117F9`, `0x80F918`. An older-image room/level lookup at `0x3DA650`
passes the result of `0x3DAC20` (alternate `0x3DAD90`) to `0x3289A0`:

```text
3DA6F5: E826050000 call +0x3DAC20
3DA6FA: 488BD0     mov rdx, rax
3DA704: E897E2F4FF call +0x3289A0 ; return +0x3DA709
```

This path was previously investigated for portal landing. Its current caller
window is absent, so that historical cause cannot be assigned here. The null
read is confirmed; initiating command, failed lookup and owner remain unknown.
Command-like stack text is a lead rather than a proven initiating frame.

Other inspected older-image windows: `0x2E0870`/104, `0x2DDCB0`/192,
`0x2DDCD0`/128, `0x2DE270`/112, `0x2DE2A0`/48, `0x296C80`/112,
`0x3DA650`/192, `0x3DAC20`/128. They are reference windows, not admitted hooks,
function boundaries or callable contracts. No helper was invoked.

## Trade Notifications cleanup signature

Both cleanup dumps fault at module RVA `0x30F95`, `CD29` (`int 0x29`), after
`B907000000` loads ECX=7. The captured/current-file `0x30F60`/96-byte window
matches. The installed SDK names 7 `FAST_FAIL_FATAL_APP_EXIT`; generic
`C0000409` alone does not establish a buffer overrun.

A limited offline AMD64 unwind using current plugin PE metadata and saved stack
bytes produces the same initial walk in both dumps. Function intervals are
end-exclusive; all listed addresses below are relative to Trade Notifications.

| Function interval | Return PC | Return slot from exception RSP | Unwind RVA |
|---|---|---|---|
| `0x30F60..0x30FB6` | `0x2E956` | `+0x28` | `0x5AEC8` |
| `0x2E938..0x2E957` | `0x778F` | `+0x58` | `0x5D8AC` |
| `0x7770..0x7795` | `0x48ED0` | `+0x88` | `0x5AE70` |
| `0x48EC0..0x48ED6` | `0x334CA` | `+0xB8` | `0x5AEC8` |
| `0x33440..0x33555` | `0x33279` | `+0xF8` | `0x5D490` |

This is a metadata-derived walk qualified by current-file use: only the fault
window was compared with captured code, and no PDB/full captured-image integrity
attestation exists. Unsupported operations/chaining or incomplete prologues are
rejected instead of guessed.

RVA `0x48EC0` is a global-destructor wrapper passing object `0x614A0` to
`0x7770`. Its `0xB3B0` call checks DWORD object+8 for nonzero, then calls
`0x2E938` and ultimately `0x30F60`. Both dumps capture the nonzero thread-ID
field. This matches MSVC `<thread>`'s destructor of a still-joinable thread and
its `_Thr._Id != 0` test. Source has a global worker and an explicit unload join.

These cleanup signatures are explained by destruction of a joinable worker
while DLL/CRT cleanup runs. Why normal cleanup was bypassed/insufficient and
what initiated exit are unproven. A finished worker remains joinable, so the
dump does not prove it was still executing. This is not evidence that Trade
Notifications caused browser read faults in different processes.

Further recovered return PCs: `0x335A5`, `0x32A32`, `0x329B1`, `0x32B2F`,
`0x238B7`, `0x23DE9`, `0x23F09`, then `ntdll+0x11EADA` for reviewed module
version `10.0.26100.9444`. Additional function/unwind pairs are
`0x33254/0x5DE0C`, `0x33568/0x5DDCC`, `0x329C4/0x5DD50`,
`0x3298C/0x5DD70`, `0x32A84/0x5DD30`, `0x2388C/0x5AEC8`,
`0x23DA4/0x5D090`, `0x23E28/0x5D0E4`. No Windows-module unwind was attempted.
Trade Price Check `0xDA678` raw references are data-like values, not proven
frames. Notifications `0x614A0` is an object rather than executable code.

## Assertions, attribution and useful isolation

A separately observed later session logs `s_StatsPool.IsEmpty()` at
`D2Common/src/Stats/Stats.cpp:1285`, caller host `0x78EF6`, with Core `0x360BC4`
and host `0x1CF7B48` candidates. It also logs object-pool shutdown assertions
`m_allocated`, `m_sortedBlockCount`, usable blocks and `m_freed`, in
`package_cache/bc/0.13.9/src/object_pool.cpp` lines 333/338/340/350, caller host
`0x7600A`. Dialogs are disabled and execution continues. Allocation owner is
unknown; these assertions are not the preceding exception records. Overwritten
logs from another launch must not be correlated with earlier crashes.

The audited damage observer copies owned snapshots and forwards combat once;
it does not create items or allocate/free native stats. It still uses guarded
projection/clip helpers and the drawing service, so read-only observation is
not blanket proof against interaction. None of the identified fault instructions
is the observer or enlarged-number code. No speculative getter patch or unrelated
plugin change was made.

A useful controlled comparison is the same browser sequence in fresh processes
with Floating Damage physically absent, then present, both before and after
character switching. `/damagenumbers off` leaves the detour installed and is
not an absent-plugin control. If absence reproduces, investigate first-fault
browser preview/cache ownership; if presence changes reproduction, investigate
observer/overlay interaction. Controlled reproduction and allocation-lifetime
attribution remain pending.

Automated investigation parsed dump streams, decoded faults, compared current
hashes/resource versions/windows, and bounded the cleanup unwind. Live evidence
is the reported Unique-tab trigger. No code/config change occurred; these data
do not justify claiming a crash fix or new plugin stability validation.
