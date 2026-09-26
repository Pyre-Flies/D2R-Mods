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

### 6. Select and guard the interception point

Prefer, in order:

1. a stable public SDK service/event;
2. an exported provider function;
3. a guarded call site or function entry with an understood ABI;
4. a direct byte patch only when lifecycle and restoration are proven.

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
