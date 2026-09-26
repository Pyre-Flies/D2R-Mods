# Portal contact call-site migration - 1.3.1+rev.7

2026-09-26. Production snapshot; automated/offline validation passed and the user confirmed gameplay on 2026-09-26. The detailed checklist below remains a regression guide; individual distance measurements were not separately recorded.

## Change and rationale

QOL no longer installs an inline hook at game RVA 0x34BC90. Three SDK PatchBytes operations redirect only the native candidate acquisition calls to one RX relay, which jumps to CandidateContact. The wrapper calls the CURRENT entry at game base + 0x34BC90 exactly once, then applies the existing portal-only exception. This preserves entry detours installed before or after QOL, rather than caching a trampoline that bypasses another owner. Other native callers never enter QOL's wrapper.

| CALL RVA | Original bytes | Return RVA | Purpose | Full witness |
|---|---|---|---|---|
|0x191589|E8 02 A7 1B 00|0x19158E|Candidate refresh/update|0x19157E, 24 bytes|
|0x1922CD|E8 BE 99 1B 00|0x1922D2|Primary candidate insertion|0x1922C7, 19 bytes|
|0x192378|E8 13 99 1B 00|0x19237D|Final candidate success|0x19236B, 26 bytes|

All original CALLs target 0x34BC90. Immediately preceding argument moves are RDX=RSI (candidate), RCX=R12 (player); following code tests EAX. ABI: int __fastcall(void* player, void* candidate). The indirect JMP relay changes no arguments/registers/stack and retains the actual native return address for the existing whitelist.

## Evidence and admission

Source: src/portal_calls.h, src/portal_priority.cpp, src/portal_signatures.h; original disassembly: docs/portal-native-disassembly.txt; byte registry: docs/portal-evidence.json.

Offline audit used <private-workspace>/work/runtime-game.exe, SHA256 81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5. All 20 original evidence regions matched. This is a prior capture, not a fresh live-memory qualification. Startup validates the full caller witnesses again, and PatchBytes rechecks each five-byte CALL at publication.

The original full ObjectContact byte array is retained as evidence. Runtime admission now checks its body from 0x34BCB0 (entry +32) through the end, allowing another owner to patch the first 32 bytes. The full ContactRectangle helper remains guarded. This permits ordinary entry detours without accepting arbitrary changes to the remaining reviewed geometric implementation. A patch extending beyond that prefix still disables portal priority. Calling the current entry assumes other owners maintain its ABI and lifetime; this is cooperative composition, not proof that an arbitrary plugin's code is safe.

## Lifetime, failure and behavior

Allocate a near relay RW, encode FF 25 00 00 00 00 plus 64-bit wrapper address, change to RX and flush instruction cache before publication. Pin the plugin module. Precompute all three rel32 CALLs before any publication. SDK owns patch registration/restoration; no manual restoration that could overwrite a later owner. Relay and pinned wrapper remain for process lifetime after any publication attempt, including ambiguous failure.

Portal active stays false until all three CALL patches and existing score/range/comparison hooks succeed. On a partial failure installed wrappers pass through; shutdown also clears active. Shared entry pointer is initialized before publication and retained for delayed calls. The existing contact result policy is unchanged: nonzero results pass through; zero becomes one only for the exact candidate callers, an identified portal within the configured inclusive native-unit radius, active portal priority and released loot modifier. This does not globally extend object-use distance.

The other portal hooks (0x18B350, 0x18AED0, 0x18A650) are unchanged. Same-site CALL collisions, entry-hook semantic conflicts, and other hook collisions remain possible. Three guarded private sites replace one shared entry hook; this reduces one collision surface, not all update fragility.

## Verification

Release build succeeded; all 12 CTest suites passed. Portal suite covers original targets/return scopes, emitted rel32 destinations, exact SDK guards, each partial-publication failure, unreachable relay with zero publications, and existing range/LB/nonportal eligibility cases. These do not simulate the live game or another plugin's contact hook.

Regression checklist with Chronicle Ground Flag, Stash Search and Potion Auto Pickup enabled:
- Startup log reports Three contact CALL patches (shared entry untouched), with no portal profile/installation warning.
- Neutral A prefers nearby portal over loot; test configured radii 6, 10 and 20, including beyond six and just outside configured range.
- Holding LB keeps loot priority near a portal; releasing restores portal priority.
- Ordinary object interaction and portal entry still work; leave/re-enter a game.
- If inspecting memory, only the three CALL sites belong to QOL; shared entry 0x34BC90 has no QOL detour.

After a game update, re-find candidate insertion/refresh/final-success branches, prove RCX/RDX and EAX contract, derive returns from CALL ends, re-review the geometric helper and update full caller witnesses only from evidence. Do not copy arbitrary live bytes into admission guards.
