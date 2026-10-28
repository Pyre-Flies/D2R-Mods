---
name: d2rloader-development
description: Investigate, build, validate, deploy, and package D2RLoader plugins for Diablo II Resurrected, especially work involving native hooks, RVAs, provider ABIs, game data layouts, controller input, item transactions, or build compatibility. Use for D2R/D2RLoader engineering; do not use for ordinary gameplay advice or unrelated Diablo II mod configuration.
---

# D2RLoader Development

Develop against the installed D2R and D2RLoader build with evidence-led, fail-open compatibility handling. Preserve working installs and native game behavior. Prefer a small, testable probe when a native mechanism or ABI is not yet proven.

## Start with the real environment

- Locate the actual source tree, CMake target, deployed DLL, loader/provider DLLs, configuration, and logs before proposing edits. Do not assume the current working directory is the source checkout.
- Inspect local logs and artifacts directly when available rather than asking the user to reproduce long output.
- Treat executable versions, hashes, RVAs, byte patterns, structure layouts, provider tables, DLL paths, and table row layouts as build-specific.
- Find the project's existing research and compatibility records before starting fresh. Prefer repository evidence over remembered values.

## Choose the smallest useful experiment

- For an uncertain hook, input path, transaction, or native layout, isolate one observable pass/fail behavior in a standalone probe before merging it into a working plugin.
- Prefer D2RLoader SDK facilities and embedded plugin code. Add an external runtime dependency only when its concrete benefit justifies the extra installation and compatibility burden.
- Preserve native semantics, rendering, values, and interaction paths where the feature extends existing behavior. Do not invent mappings or labels when provenance is ambiguous.

## Gate native behavior

- Verify the exact module or provider artifact used at runtime, not merely a nearby build number or source snapshot.
- Guard native entry points with the narrowest sufficient combination of artifact identity, expected bytes, ABI/layout checks, table bounds, and lifecycle checks.
- Fail open: on a mismatch, disable only the unsupported feature, leave the game functional, and log a concise diagnostic.
- Keep hooks, callbacks, queued work, and teardown safe across load/unload and session changes. Do not promote a partial disassembly or static candidate into a complete ABI claim.

For native investigation, read [references/reverse-engineering.md](references/reverse-engineering.md). For recording build-specific facts, read [references/compatibility-ledger.md](references/compatibility-ledger.md).

## Coexist with other plugins & overcome RVA brittleness

`D2RL::PluginContext::InstallInlineHook` verifies `expectedSize` bytes at `exeBase + rva` against `expected` before installing a detour and **does not auto-chain** multiple plugins hooking the same RVA. If another plugin hooks `RVA X` first, live bytes at `exeBase + X` become `E9 rel32` (or `FF 25 00 00 00 00 <abs64>`), causing naive `CheckExpectedBytes` or second-comer `InstallInlineHook` calls on `X` to fail.

To maximize compatibility and avoid RVA brittleness, select interception mechanisms in this order (detailed in [references/reverse-engineering.md](references/reverse-engineering.md)):

1. **SDK Services & `PluginCommunicationService` first:** Use `SharedEventService`, `ItemInteractionService`, `LifecycleService`, `PanelService` (`registerChildLayout`), `InventoryService`, `ItemService`, and `OverlayService`. For cross-plugin cooperation (e.g. loot-filter visibility between `Pyre Loot Filter` and `Controller QOL`), publish and acquire versioned C function tables via `D2RL::PluginCommunicationService` instead of hooking another plugin's code.
2. **`D2RCore.dll` data-pointer slot CAS (`InterlockedCompareExchangePointer`) with caller scoping:** When `D2RCore.dll` dispatches through writable function-pointer slots (`0x6FE...` / `0x704...`), atomically exchange the pointer, store the predecessor, check `_ReturnAddress() == expectedCaller`, pass all other callers through to the predecessor, pin the module, and restore on unload only while the slot still equals our adapter.
3. **Specific `CALL rel32` (`E8`) site redirection via near-relay:** Leave contested function entries (`0x471950` `Pickup`, `0xCBEB0` `BuildGroundItemTooltip`, `0x34BC90` portal contact, `0x858990` `LoadSprite`, `0x13EDD0` controller input) untouched and patch the specific upstream 5-byte `E8 rel32` call site(s). Allocate 64-byte executable near-relays (`FF 25 00 00 00 00 <abs64>`) in the outer `±[0x50000000, 0x70000000]` window from `site + 5` so they stay within `±2 GiB` without competing with `D2RCore.dll`'s `±1 GiB` inline-hook pool.
4. **Predecessor-aware admission & structural call-through:** Use `D2RL::DiagnosticsService` (`queryHookStatus`, `enumerateModificationRanges`) to distinguish `ModificationState::Tracked` (`ownerPluginId`) from `Untracked`. When calling a helper whose entry may be inline-hooked by a companion plugin (e.g. `0x858990` `LoadSprite`), verify build identity via **unpatched interior body bytes** (`+0x10..+0x30`) and validate the entry jump target via `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)` + `D2RLoaderGetPluginInfo`. When writing an inline detour on a shared path, store the trampoline pointer in writable `.data` and load it via `48 8B 05 disp32` within the first 160 bytes so structural validators can verify call-through.
5. **Build-resilient runtime RVA resolution (`D2RLImage`):** Prefer `vendor/include/D2RLImage/image_scan.h` (`FindString` -> `FindLeaReferences` -> `PrimaryFunctionRange` via `.pdata` `UNW_FLAG_CHAININFO` chains -> `FindPattern` with `0x00` wildcard masks on 4-byte RIP displacements) over unmasked 32-byte witnesses that break whenever a nearby RIP displacement shifts.

## Hot-path performance & reusable building blocks

- **Respect thread & frequency budgets:** Render/UI hooks (`OverlayService`, label formatters, automap, HUD) run at **60–240+ Hz**; server/client game logic runs at **25 Hz** (`40 ms`). Never scan `128` unit hash buckets (`pAct + 0x70`), read `StatListEx`, or evaluate filter rules on every render frame; gate state rebuilds to the 25 Hz game tick (`GetTickCount64() >= nextTick`) or cache per `(unitId, generation)`.
- **Zero kernel syscalls on hot paths:** Never call `VirtualQuery` or `ReadProcessMemory` per unit/frame inside an in-process DLL (costs `~1–3 µs` per syscall). Use non-zero/alignment checks inside `__try / __except (EXCEPTION_EXECUTE_HANDLER)` (zero overhead on x64 Windows when no fault occurs).
- **Order guards cheapest-first & avoid hidden `memset`:** Check cheap scalar/register filters (`enabled`, `dwUnitType == 1`, `returnAddress`, bitmasks) before dereferencing `UnitAny` or acquiring locks. Use `SRWLOCK` (`AcquireSRWLockShared` / `Exclusive`) or lock-free atomics on high-frequency combat/render paths, and avoid zero-initializing multi-kilobyte stack buffers (`wchar_t buf[2048] = {}`) before early-out guards.
- **Reuse established helpers instead of reinventing them:**
  - **Persistent chat settings:** `plugins/shared/chat_config.h` (`ChatConfig::Save`) for comment-preserving TOML boolean updates via `ReadConfig`/`WriteConfig`.
  - **In-game `/slash` commands:** `vendor/include/D2RLChat/native_commands.h` (`D2RLChat::NativeCommands`; `0x1C0`-byte native definition, `<= 63`-byte description/help limit, deferred UI-thread registration, process-lifetime module pinning).
  - **`OverlayService` full-screen clip-rect guard:** `D2RCore.dll`'s `OverlayService` has a `float2` clip-rect ABI bug; push full-screen clip before drawing via `D2R+0xCEAED0` (`GetForegroundDrawList`), `D2R+0xD03A30` (`PushClipRect`), and `D2R+0xD03760` (`PopClipRect`).
  - **Steam `3.3.94995` `gameBuild` fix for `sounds.txt`:** On Steam (which lacks `.build.info`), `D2RCore+0x706150` (`gameBuild`) is empty, causing `D2RCore+0x1CE540` to skip compiling mod `sounds.txt`; populate `"94995"` / `L"94995"` at `D2RCore+0x706130..0x706168` when `verChars == "3.3.94995"` and `buildLen == 0`.

## Validate claims proportionally

- Separate static evidence, unit/simulated tests, artifact checks, loader admission, authoritative game state, visible UI behavior, persistence, and user-confirmed live results.
- Tests prove only what they observe. A successful native return code or authoritative state change does not prove client visibility, save persistence, or full item integrity.
- Build with the project's warning and runtime conventions. Inspect the produced DLL's exports, ABI/resource metadata, imports, version, and hash when those are part of the loader contract.
- Before replacing a deployed DLL, confirm the game and loader are closed when necessary, preserve a recoverable backup, then compare built and deployed hashes.

For test scope and evidence wording, read [references/validation.md](references/validation.md). When producing a distributable build, read [references/release.md](references/release.md).

## Build loader-admissible plugin artifacts

For every new Windows plugin DLL, include a resource script in the CMake target
and enable the `RC` language. The resource script must include
`D2RLPlugin/resource.h` and `D2RLPlugin/version.h` and embed
`D2RL_PLUGIN_ABI_VERSION` at `D2RL_PLUGIN_MANIFEST_RESOURCE_ID` as a one-`DWORD`
`RCDATA` resource. Windows `VERSIONINFO` is also expected for shipped plugins,
but it does not replace the ABI manifest. A DLL with valid lifecycle exports but
no ABI manifest is rejected by current D2RLoader as an old alpha plugin.

Add an artifact test for every new plugin. It must open the built DLL with
`LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE`, verify the manifest
resource exists, is exactly `sizeof(DWORD)`, and equals
`D2RL_PLUGIN_ABI_VERSION`. Then load the DLL normally and verify the three loader
exports, `PluginInfo` size/ABI, plugin identity/version, and a valid plugin role.
Run this test before deployment or packaging; source compilation and export
inspection alone do not prove loader admission.

## Preserve discoveries

Whenever reverse engineering finds a new path, RVA, address, byte signature, field offset, ABI/layout detail, table binding, native consumer, or rejected hypothesis, record it in the relevant project's versioned research or compatibility ledger during the same task. Include provenance, target build/artifact identity, confidence, limits, and validation state. Update an existing canonical record instead of scattering duplicate notes.

At handoff, state what changed, what was verified automatically, what was verified live, what remains unverified, and which build-specific records were added or updated.
