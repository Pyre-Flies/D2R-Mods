# XInput hardening 1.3.1.1 — 2026-09-24

## Scope and rationale
Controller QOL only. Production 1.3.1.0 and Item Roll Ranges unchanged. SDK upstream contribution remains a future option (SDK-CONTROLLER-OPTIONS.md).

Old code copied any five-byte non-E9 entry without decoding and restored saved bytes/freed trampolines on shutdown. Another owner could be overwritten or still call those allocations. New src/xinput_hook.h owns installation; controller_input.cpp owns the processing gate and lifetime.

## Entry contract
Only eight-byte-aligned entries beginning with 48 89 5C 24 08 (MOV [rsp+8],rbx, complete five-byte position-independent instruction), or E9 rel32 to executable module-backed code are admitted. Unknown prologues and private relay destinations are rejected. An E9 destination's module is pinned; this prevents module unloading, not arbitrary mutation/freeing by another component. Existing plugins that use private relay chains need separate qualification. No general-purpose instruction decoder is implied.

Build-time read-only probe in a separate Python process loaded the system XInput modules and read exports with ctypes. All observed normal entries matched MOV above:
- xinput1_4.dll XInputGetState: 48 89 5C 24 08 56 57 41 56 48 83 EC 30 48 8B FA
- xinput1_4.dll ordinal100: 48 89 5C 24 08 48 89 6C 24 10 57 48 83 EC 30 48
- xinput1_3.dll XInputGetState and ordinal100: 48 89 5C 24 08 57 48 83 EC 20 48 8B DA 8B F9 E8
- xinput9_1_0.dll XInputGetState: same first16 as 1_3; ordinal100 absent.
These are export-relative entry bytes, not game RVAs or live-game hook-chain evidence. Resolve exports after loading the system modules. ASLR probe VAs are not reusable contracts. No game/Core addresses changed.

## Publication and shutdown
A single aligned InterlockedCompareExchange64 publishes E9 plus the unchanged bytes5..7, only if all eight bytes still match the observed entry. This avoids our old split opcode/displacement stores and stale-byte overwrite. It is not a universal thread-suspension or third-party patch arbitration guarantee; another writer can still patch unsafely.

Prepare/publish predecessor before redirecting calls. Relay and trampoline share one allocation, changed from writable to execute/read before publication. Unpublished failed allocations are freed. Published allocations (at most six page-rounded allocations), predecessor records and pinned code remain for process lifetime.

Shutdown disables filtering under an exclusive SRW lock, draining current QOL processing. Retained detours call their predecessor and preserve its result, without QOL button edits. No entry bytes are restored, even if we still appear to own them: a later owner or delayed caller can retain our path. No published relay is freed. Reinstall after logical shutdown is rejected; restart the process to load a different DLL. Cached raw-state writes may still occur in retained detours; gameplay callbacks are gated off.

Initialization logs each export's active/unavailable status. Debug logging default remains false. This cannot ensure compatibility with arbitrary hooks; unsupported entry contracts fail admission.

## Validation
Nine CTest suites pass. New executable MASM fixture tests cover native predecessor result preservation, an existing E9 predecessor, stale compare/exchange rejection, later-owner entry preservation, live retained trampoline after shutdown, rejected RIP-relative unknown prologue, and concurrent processing shutdown. Existing eight QOL suites pass. In-game controller/Steam overlay coexistence remains to verify.

## Deployment
The global Controller QOL file was disabled as .bak before this change. Preserve that choice: stage the updated .bak and ship runtime/source ZIPs. Do not silently enable a new DLL. Production 1.3.1.0 archives remain unchanged.
