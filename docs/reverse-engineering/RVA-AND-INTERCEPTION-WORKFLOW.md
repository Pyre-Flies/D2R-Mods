# Finding RVAs and interception points

This guide records the repeatable workflow used to locate D2R and D2RCore
functions, verify native call contracts, and turn a research address into a
guarded plugin interception point. It is intentionally tool-agnostic: a
debugger or disassembler can replace the included Python helpers as long as the
same evidence is retained.

## Safety and publication boundary

The repository stores derived facts: RVAs, short byte witnesses, structure
offsets, call relationships, hashes, and validation notes. It must not store:

- D2R.exe, D2RCore.dll, decrypted runtime captures, dumps, or extracted assets;
- full process addresses without the corresponding module base and derived RVA;
- Windows usernames, local drive layouts, save paths, account data, or logs;
- credentials, tokens, private keys, or unrelated process memory.

The live-memory helpers in this repository use Windows read-only process access.
They do not inject code, suspend threads, install hooks, or write game memory.
`capture_code.py` does create a private PE-shaped `.text` artifact for offline
analysis; keep that artifact outside the repository and delete it when it is no
longer needed.

## Address vocabulary

- **Process address:** valid only for one running process, after ASLR. Never use
  it as a durable identifier.
- **Module base:** the loaded address of D2R.exe or D2RCore.dll for that process.
- **RVA:** `process address - module base`. This is what source and evidence
  should record.
- **File offset:** location in an on-disk PE. Convert through the PE section
  table; it is not interchangeable with an RVA.
- **Interception point:** the reviewed instruction or call site where a hook,
  listener, or byte patch can observe or alter the intended behavior.

## Discovery workflow

### 1. Freeze the build identity

Record the module filename, SHA-256, PE timestamp, file/product version, SDK
release, and plugin ABI. A matching version string without a matching hash does
not qualify an RVA for reuse.

```powershell
Get-FileHash .\D2R.exe -Algorithm SHA256
Get-FileHash .\D2RCore.dll -Algorithm SHA256
```

### 2. Start from a semantic anchor

Prefer anchors that describe behavior rather than a guessed address:

- an exported function such as a D2RCore provider entry;
- a setting or localization string and its RIP-relative cross-references;
- an SDK service slot or callback reached by a known UI action;
- a stable caller/callee relationship seen while reproducing one action;
- a structure field whose mutation has a visible, reversible result.

`core_scan.py` enumerates relevant exports and strings and can find executable
RIP-relative cross-references to a supplied RVA. The feature audit scripts then
verify the exact reviewed sites for one compatibility profile.

### 3. Convert runtime addresses to RVAs immediately

Capture the module base from the debugger/module list, then subtract it from
every observed instruction and data address. The result must fall inside the
expected PE section. Record the process address only in private session notes.

```text
observed instruction = 0x00007FF7ABC12340
module base          = 0x00007FF7AB000000
RVA                  = 0x00C12340
```

### 4. Disassemble a bounded neighborhood

Use the smallest region that establishes the calling convention, register and
stack inputs, branch behavior, and return path:

```powershell
python plugins/item-roll-ranges/tools/disasm.py 0xRVA 0xSIZE C:\private\runtime-code.exe
```

For a direct `CALL rel32`, resolve the destination as:

```text
destination = call_rva + instruction_length + signed_displacement
```

For `FF 15 disp32` indirect calls, resolve the RIP-relative pointer-slot RVA and
record both the call site and slot. Follow at least one caller above and one
callee below the candidate so the interception point is based on semantics, not
only coincidental bytes.

### 5. Observe the narrow live action

Reproduce one action at a time—press versus release, one panel, one item, one
controller chord. `live_code.py` and `audit_native_controller.py` read bounded
regions from a user-selected process. Prefer debugger breakpoints and read-only
traces until the ABI is understood.

Confirm:

- which thread executes the path;
- whether the data is borrowed, copied, or retained;
- object and payload lifetime;
- register/stack parameter meaning;
- whether multiple UI consumers receive the same action;
- what state has already mutated at the candidate point.

An interception point that runs after the competing native consumer is too late
to suppress that behavior safely.

### 6. Select and guard the interception point (Overcoming RVA Brittleness & Missing Hook Chains)

`D2RL::PluginContext::InstallInlineHook` validates `expectedSize` bytes at `exeBase + rva` against `expected` before installing a detour and **does not automatically chain** multiple plugins hooking the same RVA. If Plugin A hooks `RVA X` first, live bytes at `exeBase + X` become `E9 rel32` (or `FF 25 00 00 00 00 <abs64>`), causing Plugin B's `InstallInlineHook(X, pristineExpected, ...)` or naive `CheckExpectedBytes(X, ...)` to fail.

To prevent cross-plugin collisions and RVA brittleness, prefer interception strategies in this order:

1. **Public SDK services and `PluginCommunicationService` interop:**
   - Use `SharedEventService` (`ItemTooltipListener`, `UiMessageListener`), `ItemInteractionService`, `LifecycleService`, `PanelService` (`registerChildLayout`), `InventoryService`, `ItemService`, and `OverlayService`.
   - When two plugins need shared domain state (such as ground-loot and label visibility between [Pyre Loot Filter](../../plugins/pyre-lootfilter/README.md) and [Controller QOL](../../plugins/controller-qol/DEVELOPMENT.md)), publish and acquire versioned C function tables via `D2RL::PluginCommunicationService` (`unhoarder-ground-visibility`, `unhoarder-label-visibility`) instead of hooking each other's detours.
2. **`D2RCore.dll` data-pointer slot atomic exchange (`InterlockedCompareExchangePointer`) with caller scoping:**
   - When `D2RCore.dll` dispatches through writable function-pointer tables (`0x6FE...` / `0x704...`), atomically swap the slot pointer, store the predecessor pointer, check `_ReturnAddress() == expectedCaller` inside the adapter, and forward all other callers to the predecessor (as in [Item Roll Ranges](../../plugins/item-roll-ranges/docs/PLUGIN-COEXISTENCE.md)). Pin the plugin DLL and restore on unload only if the slot still holds our adapter.
3. **Specific `CALL rel32` (`E8`) site redirection via near-relay (`PatchBytes` / `PatchCallRel32`):**
   - When a popular function entry (`0x471950` `Pickup`, `0xCBEB0` `BuildGroundItemTooltip`, `0x34BC90` portal contact, `0x858990` `LoadSprite`, `0x13EDD0` controller input) is hooked by other plugins, leave the function entry untouched and patch the specific upstream `E8 rel32` call site(s).
   - Because a 5-byte `E8 rel32` cannot reach a plugin DLL loaded >2 GiB away, allocate a 64-byte executable near-relay (`FF 25 00 00 00 00 <abs64>`) in the outer `±[0x50000000, 0x70000000]` window from `site + 5` (see `QolRelay::Allocate` in `plugins/controller-qol/src/relay_allocation.h`) so it stays within `±2 GiB` without competing with `D2RCore.dll`'s `±1 GiB` inline-hook pool.
   - Calling the target function through `exeBase + TargetRva` from the relay wrapper then naturally traverses any companion plugin's entry detour at `TargetRva`.
4. **Predecessor-aware admission & structural call-through conventions:**
   - Use `D2RL::DiagnosticsService` (`queryHookStatus`, `enumerateModificationRanges`) to distinguish `ModificationState::Tracked` (owned by `ownerPluginId`) from `ModificationState::Untracked`.
   - When calling a native helper whose entry may be inline-hooked by a companion plugin (e.g. `0x858990` `LoadSprite` hooked by `d2rl-item-quality-backgrounds.dll`), verify build identity using **unpatched interior body bytes** (`+0x10..+0x30`) and validate the entry jump target with `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)` + `D2RLoaderGetPluginInfo`.
   - When installing an inline hook on a shared path, store the original trampoline pointer in writable `.data` and load it via RIP-relative `48 8B 05 disp32` within the first 160 bytes of the detour function so structural validators can verify call-through safety.
5. **Build-resilient runtime RVA discovery (`D2RLImage`):**
   - Unmasked 32-byte witnesses that include 4-byte RIP-relative displacements (`E8 rel32`, `48 8B 05 disp32`, `48 8D 0D disp32`) break whenever an upstream function shifts by even a few bytes.
   - Use `D2RLImage` (`ReadSections`, `FindString`, `FindLeaReferences`, `FindPattern` with `0x00` wildcard masks on 4-byte RIP displacements, and `PrimaryFunctionRange` walking `.pdata` `UNW_FLAG_CHAININFO` unwind records) plus instruction-offset field guards (as in `native_commands.cpp`) to resolve or cross-check RVAs dynamically.

Store enough expected bytes to reject a changed instruction sequence. Include
semantic witnesses such as the expected destination, nearby field access, or
caller relationship. Never locate a hook from a short byte pattern alone.

All mismatches must fail open: skip the feature or plugin initialization and
leave the game path untouched. Do not partially install a group of related
hooks.

### 7. Separate static, automated, and live validation

- **Static audit:** hash, section bounds, instruction decoding, xrefs, and byte
  witnesses match.
- **Automated tests:** policy, adapters, artifacts, guards, rollback, and
  publication ordering pass without a running game.
- **Live validation:** the intended action works visibly in the qualified game
  build, competing behavior is absent, unrelated surfaces remain native, and
  save/reload persistence is checked when claimed.

A transaction return value or successful hook installation is not live proof.
For item operations, validate authoritative state, UI publication order,
visible state, and persistence separately.

### 8. Record the result before implementation moves on

Add the module/hash, RVA, expected bytes, ABI, derivation, validation status,
and fail-open behavior to the feature document and the central registry. Mark
unknowns explicitly. If a game update changes the profile, add a new profile;
do not silently overwrite the historical evidence.

## Included tools

| Tool | Purpose | Writes game memory? | Public output |
|---|---|---:|---|
| `capture_code.py` | Private bounded `.text` capture for offline analysis | No | None; captured image stays private |
| `disasm.py` | Disassemble a private PE at an RVA or find direct-call xrefs | No | Bounded derived disassembly |
| `live_code.py` | Read and disassemble a selected live address range | No | Convert addresses to RVAs before recording |
| `core_scan.py` | Enumerate exports/anchors and RIP-relative xrefs | No | Export, string, and xref RVAs |
| `audit_provider.py` | Verify one D2RCore hash, exports, call slots, and witnesses | No | Hash and derived RVA evidence |
| `audit_portals.py` | Compare reviewed portal sites with a private image | No | Site names, RVAs, and match status |
| `audit_native_controller.py` | Inspect controller-service targets in a selected process | No | Derived game RVAs only |
| `verify_native_profile.py` | Verify recorded native-profile evidence | No | Compatibility result |

The audit scripts intentionally refuse or report mismatches. Regenerating a
profile is a review step, not an automatic response to a failed guard.

## Established findings

The central [reverse-engineering registry](README.md) indexes the known D2RCore
transaction entry, controller navigation paths, label paths, and Shared Stash
ABI offsets. Plugin-specific hook inventories and evidence files remain under
each plugin's `docs/` directory.
