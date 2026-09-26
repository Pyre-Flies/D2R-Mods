# Plugin compatibility — Controller QOL Updates 1.5.20

Implemented 2026-09-24. The tester confirmed Potion Auto Pickup 1.3.3. Contract tests pass; combined game runtime testing is still required. Preserve the separate user-confirmed 1.5.19 golden release.

## Evidence and cause

The reported failures at 0x1D2A798..0x1D2A820, stepping by eight, are slots 1..18 in the action table at 0x1D2A790. QOL 1.5.19 and RuffnecKk both replaced these pointers and expected native values first. Load order only changed which plugin failed.

Local reference (relative to game directory): Documentation/RuffnecKk-D2RLoader-Suite-main/plugins/potion-auto-pickup/src/plugin.cpp. SHA256: ca5466a0bfd99c141d63d81d8190810f1c5ea48690922378033d3aa2a90e78e7. Review ValidateRuntime, InstallMutations, HookTrigger, HookGetFreeBeltSlot, ScanUnsafe. Reference metadata identifies 1.3.3; the tester's actual DLL has not been inspected.

## Action observation without table ownership

QOL now uses D2RLoader InstallInlineHook at each original handler entry. It never validates, writes, or restores the shared table. RuffnecKk can still validate native pointers and install its table wrapper in either order. Its saved native addresses enter QOL hooks, which call the appropriate loader trampoline once, preserve the result, then run QOL observation. RuffnecKk's wrapper then resumes its scan.

Opcode 01..12 handler RVAs, in order:

    4AC050 4ACE20 4ACE40 4ACE60 4ACE80 4ACF80
    4AD030 4AD0E0 4AD100 4AD120 4AD140 4AD230
    4AD330 4AD3E0 4AD490 4AD4B0 4AD4D0 4AD4F0

ABI: Windows x64 int64_t(game, player, packet, int32_t size), RCX/RDX/R8/R9D. Small wrappers save R9D as the fifth argument, move packet R8 to R9, and supply an opcode argument; larger handlers save game/player and inspect packet/size. Reference table wrappers use the same ABI. See compatibility-disassembly.txt for entry disassembly and compatibility-evidence.json / src/compatibility_signatures.h for exact fixed 32-byte guards. All 18 guards are checked before installation and again by the loader when installing. Partial installation stays passive, forwarding native calls. Shutdown disables observation; the loader owns removal and trampoline lifetime. No table restoration overwrites another owner.

SDK evidence: Documentation/PluginSDK-master/include/D2RLPlugin/hooks.h and shared_events.h. Inspected shared events expose UI/tooltip events, not authoritative actions. We use loader hook ownership, not a separate detour library. Other plugins that inline-hook these same handlers still need separate review.

## Direct pickups and belt cooperation

Native pickup RVA: 0x471950. QOL identifies the immediate return address via GetModuleHandleExW and D2RLoaderGetPluginInfo. Direct calls from a module exporting valid plugin metadata retain that plugin's policy, including with LB held or filtered items. Ordinary game callers retain existing suppression; QOL's own scoped bypass remains. This trusts installed plugin metadata, not a security boundary. Tail calls/intermediaries may not preserve a plugin return address. No per-frame module scanning or filename assumptions.

Free belt slot RVA: 0x3862D0. ABI: int32_t(inventory, item, int32_t* slot, bool allowAnyBeltable). RuffnecKk 1.3.3 delegates to native outside its own thread-local forced-routing scope. QOL continues calling the native entry, preserving that hook, never taking its private trampoline. Admission accepts the fixed 96-byte native body OR an E9 rel32 / FF25 absolute entry detour targeting metadata ID ruffneckk-potion-auto-pickup, version exactly 1.3.3. One standard relay hop is allowed. Every byte after the replaced 5 or 14 bytes must match the reviewed native body. Unreadable memory, unknown owner/version, another jump shape or changed body fails closed. This checks a trusted-plugin contract, not every instruction inside that plugin. Other belt fingerprints remain unchanged.

## Stash Search report: misleading diagnostic

QOL's separate error at 0x1516EBE came from an informational check expecting 48 83 F8 20 (compare label count against 32). The paired diagnostic is 0x1519AF9, expecting 49 83 7C 24 10 20. Neither result gates a feature or installs a patch. Using the loader safety-check API for optional observations produced an alarming error for modified code.

These observations now use bounded ReadProcessMemory reads and an informational original/modified/unreadable status. Actual mutation admission checks remain intact. QOL neither overwrites these sites nor approves modified instructions for execution.

User-associated plugin: https://github.com/yinyin333333/D2RL-yin-Junk-Room/tree/main/for%20d2rloader/StashSearch . Repository tree inspected at commit 2d6949992143c21ed2f6c61375359145cc8e3152 contains DLL/layout/readme/archive, no C++ hook source. Attribution is unconfirmed. RuffnecKk's local patches/ruffneckk-ground-item-label-limit-128.json also changes 0x1516EBE to 66 3D 80 00 and 0x1519AF9 to 41 80 7C 24 10 80, with five companion edits. Do not infer the writer from this diagnostic or apply a partial label-cap patch. This resolves the misleading QOL error, not a claim of full Stash Search compatibility.

## Reproduction and patch recovery

All addresses are D2R.exe-relative RVAs; runtime VA = exeBase + RVA. Evidence: decrypted work/runtime-game.exe under the original Codex project directory, SHA256 81af5adeef90f6be190ca596cc39e6a6c794a47f0231f71a09c0dd27105904c5. Raw offsets equal RVAs in this capture; use PE section mapping for others. Its .pdata is unavailable, so unwind data was not used to infer function boundaries. Pickup entry 0x471950 already contains a process-specific E9 detour: never adopt it as a native signature. The legacy pickup hook admission remains unchanged; new fixed evidence covers only 18 handlers and the belt entry.

Run: python tools/audit_portals.py --image <decrypted-PE> --evidence docs/compatibility-evidence.json

This compares 19 sites without regenerating expected bytes. After a patch, recover the packet dispatch table, follow opcode 01..12 pointers, disassemble entries/callers to verify ABI, then recheck free-belt-slot callers and the supported plugin's delegation contract. Update guards/evidence together only after review. Optional label diagnostics can be found by collection-count/truncation logic; no gameplay admission depends on them.

Nine automated suites include per-handler exactly-once calls/results, both table-owner order models, shutdown, partial installation, fixture DLL metadata identification, belt relays, and rejection of unknown versions/owners or changed bodies. These are contract tests, not D2R runtime tests. Still playtest both plugin orders, each alone, auto potions with LB held/released and visible/filtered potions, full/partial belts, QOL inventory/stash refill, ground chords, stash search, and golden portal range behavior. Hot unload/reload needs separate qualification if supported by the loader.
