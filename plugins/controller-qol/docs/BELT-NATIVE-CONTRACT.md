# QOL potion placement: native contract and patch recovery

Reviewed: 2026-09-23. Development build: **1.5.1-potions**. This is a test build on top of the user's 1.5.0 work, not a declaration of live-game qualification.

## Result and confidence

The previous belt wrapper called `UI_ShiftRightClickPlaceAction(item, player, 0, 0, state)`. Disassembly shows that fourth argument zero selects a different action branch from the game's stored-item callers. Those callers pass **one**. The old wrapper also called UI finish twice and treated a void native call returning without an exception as a completed move.

The new code submits the stored-item action with page 0, flag 1, and a validated native belt slot. D2RLoader supplies scheduling, enumeration, item identity snapshots, native-pointer access, personal-stash transactions, session notifications, and byte admission. It submits only one action at a time and waits for the SDK to observe that exact runtime item in the belt. No new hook or packet encoder is introduced for potions.

Evidence levels:

| Finding | Evidence | Confidence / limit |
| --- | --- | --- |
| Stored items require flag 1 | Both callers at 0x2AA7C5 and 0x2C7D54 set R9B=1; target branches on BL copied from R9B | Direct machine-code evidence in the captured image |
| Flag 0 selects cursor-style placement/swap | Branch to 0x15F7B2 sends 0x23 or 0x25; flag 1 sends 0x53 | Branch/packet distinction proven; names reflect inferred game semantics |
| Native action is asynchronous | Function constructs/sends a packet, returns void, and exposes no server commit result | Returning from it cannot establish movement |
| Old success logs were inadequate | 07:40:48–07:40:52 log repeatedly resolves the same source positions and reports slot 4 | Supports a failed/unconfirmed move; logs alone do not prove its precise server rejection |
| Belt transactions cannot replace native placement | Bundled SDK `item.h`, ExistingItemOperation contract, explicitly rejects belt moves | Public API contract, not an assumption from the enum existing |
| Raw pointers cannot escape editNativeItem | `item.h` states the pointer expires when its synchronous callback returns | New belt module obeys this; unrelated legacy paths were not rewritten |
| Corrected call works in the actual game | Not yet observed | Requires the runtime test matrix below |

## Source and binary identities

The latest work was in `<development-worktree>` and mirrored in `plugins\controller-qol`. Before this work, both `src/plugin_main.cpp` files had SHA-256 `E95AB95AF7786293C846BE0BD83351C6877F707639CA5417CD67BA04E40489C3`. Existing changes were preserved. The QOL release source was staged into the writable `outputs\QOL-potions` directory; the original `QOL - With Working Glyphs` directory and golden ZIP were not overwritten.

The archive called `QOL-1.5.0-golden.zip` contains code that still reports 1.2.0, and contains different root/build DLL sizes. Therefore filenames and printed version strings were not treated as authoritative build identity. The new DLL metadata and navigation diagnostics consistently report `1.5.1-potions`.

| Artifact inspected | SHA-256 |
| --- | --- |
| Installed on-disk D2R.exe; file version 3.3.93787 | `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7` |
| Installed D2RCore.dll | `AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0` |
| Previously captured/reconstructed `work/runtime-game.exe` used for this analysis | `81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5` |

The runtime image lives at `<private-workspace>\work\runtime-game.exe`. It is an earlier research artifact, not a new capture of a running game in this session. Live SDK byte checks decide admission. Version strings do not authorize the same RVAs on other builds.

**Do not disassemble protected on-disk bytes as if they were the loaded program.** At RVA 0x15F660, the installed disk image produced nonsensical instructions; the runtime reconstruction begins `40 53 55 56 57 41 57 48 83 EC 60` and produces coherent functions and callers. This is why the earlier scratch script reading D2R.exe directly is not sufficient evidence.

## Address convention

All table values below are **D2R.exe-relative RVAs**, unless explicitly marked D2RCore. Runtime VA = module base + RVA. The captured image uses base `0x140000000`, so RVA `0x15F660` appears in dumpbin as `0x14015F660`. Use `context->exeBase` at runtime; do not hardcode that example VA. File offsets must come from PE section mappings; an RVA is not generally a raw-file offset.

## Potion address/ABI registry

| RVA | Working name | Observed contract / reason retained |
| --- | --- | --- |
| 0x08B2D0 | GetLocalDataContext | `int32_t()`; feeds local-player resolver |
| 0x09A480 | GetLocalPlayer | `void*(int32_t context)`; native client player, never an SDK handle cast |
| 0x34A360 | GetUnitInventory | `void*(void* unit)`; native callers also populate diagnostic registers, but observed inventory resolution uses RCX |
| 0x3862D0 | INVENTORY_GetFreeBeltSlot | `int32_t(inventory, item, int32_t* slot, bool allowAnyBeltable)`; native capacity/family policy, final slot constrained to 0..15 |
| 0x15F660 | UI_ShiftRightClickPlaceAction | `void(item, owner, uint8_t page, uint8_t flag, void* optionalSlot)`; new inventory call is `(item, localPlayer, 0, 1, &state)` |
| 0x15F8B0 | TransferItemToInventoryPage | Existing shared-stash withdrawal helper: `bool(item, destinationUnit, uint8_t destinationPage, uint8_t sourcePage, bool, placementOut)`; request `(item, player, 0, 1, true, placement)` |
| 0x1A0780 | FinishInventoryInteraction | Existing UI completion helper, `void(int32_t, void*, int32_t, int32_t, bool)`; retained only in the existing shared withdrawal wrapper, not added after belt placement |

`src/belt_signatures.h` contains the exact 588-byte body `[0x15F660,0x15F8AC)` plus 32-byte entries for the other six functions. `docs/belt-evidence.json` records each range, bytes, hash, and candidate callers. Admission uses **D2RLoader `CheckExpectedBytes`**, at initialization and again before each batch. Mismatches disable potion work; no code is patched to make a mismatch pass.

This is a deliberately strict profile. A different plugin hooking these entries can disable belt support even on the same game version. Do not bypass the guard; establish a compatible hook/call contract first. The guard is not proof that every transitive callee is unchanged. After a patch, re-audit the call graph, not just the seven entry addresses.

## How the flag and state conclusions were reached

Read `docs/belt-disassembly.txt` for the saved dumpbin output. Relevant instructions, expressed as RVAs:

1. `0x15F67A` loads the fifth argument from `[rsp+0xB0]` after the function's prologue. Under Windows x64 this is the stack argument originally at entry `rsp+0x28`.
2. `0x15F682` copies R9B to BL. `0x15F686` copies R8B to R15B. These preserve the fourth flag and third source-page arguments.
3. `0x15F690` tests byte `[state+4]`. If false, the helper calls the local-player/inventory/free-slot chain at `0x15F69E`, `0x15F6A5`, `0x15F6BA`, and `0x15F6CE`, then writes the selected DWORD to `[state+0]` and sets `[state+4]`.
4. `0x15F6F4` tests BL. A zero branches to `0x15F7B2`. A one continues through the stored-item route. That route loads opcode **0x53** at `0x15F780` and calls the native sender at `0x15F782` (target `0xEC820`).
5. The zero branch checks an existing belt slot via `0x388390`. It sends opcode **0x25** at `0x15F829` through `0xEC780` when occupied, or **0x23** at `0x15F865` through `0xEC730` when empty.
6. `0x15F88B` calls `0x1A0780` itself. `0x15F890` clears `[state+4]`, regardless of whether a later server update will accept the request. The engaged byte is not a success flag.
7. Raw E8 reference search found candidates at `0x2283A6`, `0x2AA7C5`, `0x2C7D54`. Raw searches can find instruction-data false positives; the two inventory callers were disassembled to confirm alignment and argument setup.
8. At `0x2AA7B2` and `0x2C7D41`, the callers explicitly write `R9B=1`. They load a source page from widget `+0x630`, pass item and owner, and pass a stack optional-slot pointer. This independently corroborates the target's flag branch.

The wrapper's `BeltPlacementState` has a signed slot at +0 and an engagement byte at +4. Its 64-byte allocation is conservative zero-filled storage inherited from the earlier fix, **not evidence that the native optional object requires 64 bytes or 64-byte alignment**. Static assertions lock the wrapper layout. A valid, freshly preflighted slot is passed engaged. Null inventory, failed free-slot lookup, and out-of-range slots never reach placement.

As a secondary cross-check, the earlier `game-1cb0000.bin` data dump has an eight-byte pointer table at RVA `0x1D2A790`. Reading table + opcode*8 yielded 0x23→0x4B1BA0, 0x25→0x4B3780, 0x53→0x4C70B0, 0x54→0x4AD790 after subtracting the captured module base. The 0x53 target checks packet length 0x15 (21), whereas the 0x23 target checks 9. These are research cross-checks; the new belt implementation neither hooks these handlers nor builds these packets. Earlier notes associating packet 0x54 with 0x4C5570 are not supported by this captured table.

## SDK ownership and execution flow

| Responsibility | Mechanism |
| --- | --- |
| LB+A interception | Existing ItemInteractionService listener; accepted request returns Consume |
| LB+R3 detection | Existing controller input callback, honoring enabled/quick_move |
| Scheduling | Existing 33 ms polling loop only calls `QolBelt::Pump`; ThreadService queues work onto the authoritative game thread |
| Enumeration and confirmation | InventoryService `forEachInventoryItem`, copied ItemInfo records |
| Cross-thread identity | Runtime ID + item code + class ID within the current session; source container/page/cell also checked before acting |
| Native access | ItemService `editNativeItem`; pointer used only synchronously inside its callback |
| Personal stash → inventory | ItemService `executeExistingItemTransaction`, requiring Success and exactly one committed operation |
| Shared stash → inventory | Existing native helper because SDK transactions explicitly reject shared stash; wait for inventory observation before placement |
| Capacity and item-family rules | Native GetFreeBeltSlot, not a plugin recreation of belt rules |
| Session cleanup | LifecycleService GameJoined/GameLeft listeners reset pending work; SDK discards old game tasks |

There is no new worker thread, permanent per-frame inventory scan, allocation per queued tick, detour library, or packet serialization. Work is bounded to one active batch and 512 copied candidates. Only while active does the existing poller request game-thread work; only one such tick is outstanding. SDK callbacks are documented as queued, not inline. The mutex protects the mailbox and batch state across UI/worker/game callbacks; no callback userData owns heap storage that could leak when the loader drops queued work.

Phases:

```text
Ready → native capacity preflight
  Inventory     → submit stored placement → AwaitBelt
  Personal stash → SDK withdrawal        → AwaitInventory → re-preflight → submit → AwaitBelt
  Shared stash   → native withdrawal     → AwaitInventory → re-preflight → submit → AwaitBelt
AwaitBelt → SDK finds exact runtime item in Belt → increment confirmed → next candidate
```

An unconfirmed operation stops the batch after 1.5 seconds; it is not resubmitted blindly. A queued game task that has not run within 5 seconds cancels the batch. No room for one family skips that item and allows other families to be tried. A full inventory or refused stash withdrawal stops the batch. If the player changes, the session changes, or identity/source no longer matches, work is canceled or that candidate is skipped. Repeated shortcut input while a batch is active is consumed without starting a second batch.

If the belt fills between preflight and a stash withdrawal, a withdrawn potion can remain in inventory. The code does not fabricate a rollback or move a different same-code item. Logs distinguish Submitted from Confirmed. Only an SDK observation in Belt contributes to the confirmed count.

## Scope and known limits

- This is placement/refill, not automatic ground pickup, potion stacking, automatic drinking, or mercenary feeding.
- Inventory and ordinary personal stash single-item actions use the existing SDK interaction path. The bundled interaction SDK explicitly does not promise shared-stash events; do not advertise LB+A on every shared/advanced stash panel as proven.
- Refill considers inventory first and then ordinary personal/shared stash when the stash UI is open. The shared route is inherited native behavior and requires live testing. Advanced stash/custom-page and Cube source withdrawals are not admitted by this module.
- Item codes hp*/mp*, rvs/rvl, utility potions and loose scrolls are candidates; native capacity validation makes the final decision. Modded potion tiers need live checks. No custom column preferences were added.
- Remote TCP/IP clients lack the authoritative game thread required by these SDK mutation APIs; scheduling is rejected rather than bypassed.
- Confirmation does not prove client visual synchronization. Refill issues subsequent work on later poll ticks, but live tests must cover latency and other plugins.
- Older identify/loot/stash systems retain their existing implementation. This module's pointer-lifetime and admission guarantees do not retroactively apply to all legacy code.

## Re-finding addresses after a game patch

1. Preserve the known working DLL, source, configuration, test results, logs, and hashes. Record the new D2R.exe and D2RCore.dll versions/hashes. Rebuild against the intended loader SDK and inspect whether it now supports belt moves publicly before retaining native calls.
2. Launch the target build and let it initialize. Capture decrypted code, without modifying the process. The bundled `tools/capture_code.py` uses OpenProcess with QUERY_INFORMATION + VM_READ, verifies the main module is D2R.exe, reads its headers/.text, and writes a **non-runnable disassembly artifact**. It refuses to overwrite an earlier file. Do not ship full game-memory dumps with the plugin. The capture tool was syntax-checked here; no fresh capture was possible because D2R was not running.

   ```powershell
   Get-Process D2R | Select-Object Id
   python tools/capture_code.py --pid <PID> --output runtime-code-new.exe
   python tools/audit_belt.py --image runtime-code-new.exe --locate-from docs/belt-evidence.json --output candidates-new.json
   ```

3. Candidate discovery reports exact 16-byte and weaker 8-byte prefix matches. The old image demonstrates why a short prologue is insufficient: the 8-byte placement prefix matches many functions; its 16-byte prefix finds 0x15F660. A zero or multiple match is a research task, not permission to pick the first address. If both prefixes changed, trace the stored-item UI caller or packet-sender call chain structurally.
4. Disassemble the candidates and surrounding functions with dumpbin, Ghidra, or another disassembler. In an x64 VS Developer Prompt, the old-profile commands are:

   ```text
   dumpbin /DISASM:BYTES /RANGE:0x14015F660,0x14015F8AC runtime-game.exe
   dumpbin /DISASM:BYTES /RANGE:0x1402AA795,0x1402AA7CB runtime-game.exe
   dumpbin /DISASM:BYTES /RANGE:0x1402C7D24,0x1402C7D5E runtime-game.exe
   ```

   Substitute the captured module base and newly found RVAs. Confirm instruction boundaries and return paths; do not stop analysis at the first conditional branch.
5. Re-establish the whole contract: register/stack arguments, optional-slot +0/+4 layout, source page encoding, flag-one caller setup, free-slot return and output semantics, opcode branch distinction, internal UI finish call, and asynchronous server acknowledgment. Follow the new relative calls to local-context/player/inventory/free-slot helpers. Inspect transitive callees and shared-stash withdrawal if that feature will remain enabled.
6. Use the local primary research corpus `Documentation/RuffnecKk-D2RLoader-Suite-main/research/d2r/known-rvas.json` and `plugins/potion-auto-pickup/src/plugin.cpp` as search leads. Names/confidence there are not substitutes for verifying the new binary. The pickup plugin corroborates the free-slot ABI and SDK hook ownership; its ground-pickup hook is not necessary for inventory placement.
7. Update `include/native_d2r.h` and the reviewed sites in `tools/audit_belt.py` only after that review. Capture evidence and generate admission bytes with `--header src/belt_signatures.h`. The script's reviewed placement-prefix check is intentional; if it changes, review and update that check too. **Never generate signatures from arbitrary current memory solely to make a failing check pass.** The `--locate-from` mode cannot generate a header.

   ```text
   python tools/audit_belt.py --image reviewed-runtime.exe --output docs/belt-evidence.json --header src/belt_signatures.h
   ```

8. Update this registry, hash provenance, disassembly, and tests together. Build, run CTest, then qualify one potion before batch refill. Restart the game for changed DLLs; do not hot-reload ownership-sensitive hooks.

## Existing QOL hook map for patch triage

These are retained code locations, **not newly requalified by this potion change**. They help separate a potion failure from an existing input/label failure. See the existing `NAVIGATION-HANDOFF.md` for prior navigation research and the signature headers alongside each source.

| System | Location(s) | Ownership / evidence location |
| --- | --- | --- |
| TabBar message routing | D2R RVA 0x878D30 | SDK InstallInlineHook; `qol_navigation.cpp`, `native_signatures.h` |
| Main menu routing | 0x27DF80 | SDK InstallInlineHook; `menu_signatures.h` |
| Skill Tree routing | 0x14C6810 | SDK InstallInlineHook; `skills_signatures.h` |
| Label press/release | 0xC66A0 / 0xC6E90 | SDK InstallInlineHook; `label_signatures.h` |
| Set/refresh filtered labels | 0x1FAE90 / 0xCE450 | Existing native calls in `qol_navigation.cpp` |
| Scoped glyph widget/text draw | 0x86D410 / 0x902E20 | SDK InstallInlineHook; `glyph_signatures.h` |
| Ground tooltip / pickup | 0xCBEB0 / 0x471950 | SDK InstallInlineHook in `plugin_main.cpp` |
| Ground action dispatch | Table 0x1D2A790 + opcode*8 | SDK PatchWriteU64 with expected original pointer; GroundLoot namespace |
| Native raw controller manager | **D2RCore** 0x67F4D0 / lookup 0x680D28 | Module/hash-specific reader; `qol_navigation.cpp` |
| Filtered-label state globals | **D2R.exe** 0x235D360 / count 0x235D368 | `ReadFilteredLabelState` in `qol_navigation.cpp`; not controller raw state |

The older log's label-cap mismatch at 0x1516EBE is separate from potion admission. Do not suppress that diagnostic or use it as proof that the belt sites matched. Existing `controller_input.cpp` also retains its XInput hook implementation; this change does not replace it with a new library.

## Build and verification

The merged deliverable uses the staged QOL CMake project. From an x64 VS Developer Prompt:

```text
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`tools/build_local.py` records the local MSVC/Windows SDK setup and allows source/build/toolchain overrides. It normalizes environment-variable casing to avoid the observed MSBuild duplicate Path/PATH failure. The mirror also builds as controller-qol.dll; **deploy the merged QOL.dll only**, not both binaries. The mirror's old config/package identity should not be treated as the merged release package.

Automated checks cover the stored-item flag/page/optional-slot call, invalid slots/null arguments, identity despite handle changes, same-code impostors, source changes, wait/confirm/timeout behavior, plus the pre-existing routing, glyph, physical-input and DLL artifact suites. They do not execute the native game or verify packet acceptance.

Runtime acceptance matrix (pending at handoff):

| Case | Expected observation |
| --- | --- |
| LB+A on inventory health/mana/rejuv with room | One Submitted then one Confirmed; same runtime ID actually in belt |
| Full belt / wrong-family column | Item stays in place; no false Confirmed |
| LB+R3 with several candidates | One move confirmed before the next submission; correct family rules |
| Empty belt and 1/2/3/4-row belts | Native chooses legal slots only |
| Personal stash refill, room in inventory | SDK withdrawal, later belt confirmation |
| Full inventory during stash refill | Stop without further withdrawals |
| Ordinary shared stash | Confirm inventory arrival before belt submission; no advanced-stash claim |
| Hold/repeat chord while pending | No overlapping batch and no accidental ordinary drink action |
| Move an item manually / close session mid-batch | No same-code substitute, stale pointer access, or batch in the next session |
| Existing identify, loot tags, navigation, stash transfer | No regression |

Look for `[QOL/Belt] Native contract admitted`, `Submitted ... awaiting SDK confirmation`, and `Confirmed item runtimeId=...`. A timeout is an honest failure report, not permission to count a move or keep draining the stash. Record actual behavior and the log with DLL hash before marking this build game-tested.

## 1.5.2 Materials follow-up

The user's 1.5.1 runtime evidence confirms ordinary belt moves; the retained timestamps and source-log hash are recorded in [LEGACY-REIMAGINED-MIGRATION.md](LEGACY-REIMAGINED-MIGRATION.md). Advanced stash counters require a separate native widget withdrawal route, documented in [MATERIALS-NATIVE-CONTRACT.md](MATERIALS-NATIVE-CONTRACT.md). The new route remains pending live qualification.

