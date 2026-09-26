# Widget-scoped glyph rendering — 1.3.1.3

Fixes the known Stash Search1.10.12 startup witness collision while preserving QOL prompt substitutions and shared-page rectangle/scale handling. No Stash Search DLL or check was modified. Version-string format intentionally unchanged.

## Evidence and call contract
Disassembly source: prior read-only runtime-game.exe capture in <private-workspace>/work. Reproduce with Documentation/item-roll-ranges/tools/disasm.py 0x86d660 0x100. These game RVAs are validated again at plugin initialization before mutation.

Widget draw entry0x86D410 remains an SDK inline hook to scope the widget name/ancestors through thread-local currentHint. Original draw executes synchronously with previous scope restored in finally. Its two calls to text renderer0x902E20:
-0x86D6A9: E8 72 57 09 00; return0x86D6AE. RCX=text from[rdi+rsi], RDX=stack rectangle[rsp+30], R8=style[rbp], XMM3=scale fromXMM0.
-0x86D6F3: E8 28 57 09 00; return0x86D6F8. RCX=same text source, RDX=rectangle returned by0x1FACD0, R8=style[rsp+50], XMM3=scale fromXMM6.

Each five-byte CALL is replaced through SDK PatchBytes(expected bytes required), targeting a near RX relay with FF25 absolute jump to HookText. Tail jump preserves original CALL return address, shadow space and four-argument Windows x64 ABI. HookText uses unchanged scoped glyph/rect policies and calls the original renderer directly once. The renderer entry is no longer detoured. The existing GlyphSignatures remain checked plus both call opcodes/displacements.

Relay allocated near game widget, bounds checked for both signed32 displacements; page switches RW to RX before publication. Plugin is pinned. Relay retained until process exit after any patch attempt, including partial/ambiguous failure. active stays false until both calls and widget hook succeed. Partial installation therefore passes through native renderer without substitutions. SDK owns game byte-patch/inline-hook cleanup; QOL shutdown sets inactive. No arbitrary internal target-RVA trick is used for an out-of-module destination.

## Stash Search compatibility
Its exact32-byte witness at game0x902E20 matches the capture and is untouched. Its inspected widget witness at0x86D489 also lies outside QOL widget entry and new CALL patches. See STASH-SEARCH-1.10.12-CONFLICT.md and stash-search-1.10.12-evidence.txt for binary hash and check RVAs. This removes the established static collision; live startup and rendered prompts still need validation.

## Validation
All10 CTest suites pass. New glyph-call suite verifies both encoded native destinations, negative relay displacement roundtrip and out-of-range rejection; existing glyph policies remain unchanged. StashSearch DLL expected32 bytes at0x14650 compared equal to captured game0x902E20. Test with both plugins enabled: Stash Search loads/searches; main prompts LT/RT, skill/quest LB/RB, shared LB+LT/RT spacing. No claim of blanket compatibility with unexamined plugins.
