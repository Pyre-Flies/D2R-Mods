# Chronicle Ground Flag compatibility candidate — 1.3.1+rev.6

2026-09-26. Status: candidate, compiled and all 12 automated suites passed. In-game validation pending. Production rev.5 remains preserved.

## Change and composition

QOL no longer calls InstallInlineHook for shared ground-label builder game RVA 0xCBEB0. It redirects the CALL at game RVA 0x1FAA18 through an RX near relay to its wrapper. The wrapper calls the game entry at 0xCBEB0, including any Chronicle detour installed there, and only afterward registers the output buffer and adds the controller hint. It preserves the returned value and forwards the same unit, text, capacity and color arguments. QOL never caches Chronicle's trampoline or modifies its DLL. This composition permits Chronicle to claim the builder entry regardless of which plugin loads first; actual order/coverage testing is still pending.

Only this call is patched. Other calls to the shared builder are not intercepted by QOL. The offline scan found one direct game CALL, not proof of no indirect or Core callers. A matching full argument-setup witness is required at startup; guard failure leaves the call unchanged and makes QOL label tracking/hints unavailable (which can also affect filtered ground-pickup eligibility).

## Address/ABI evidence

All game RVAs use PluginContext::exeBase. Candidate is qualified for the same loader/core files as rev.5:

- D2RLoader SHA256 93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10.
- D2RCore SHA256 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
- The two installed files were rehashed 2026-09-26; unchanged.

Prior private mapped-game capture: caller starts 0x1FA9F0. At 0x1FA9FF–0x1FAA1C, exact 30-byte witness:

`45 8B E1 C6 02 00 49 8B F0 4C 8D 4D F0 41 B8 80 00 00 00 48 8B DA 4C 8B F9 E8 93 14 ED FF`

Call 0x1FAA18: `E8 93 14 ED FF`, destination 0xCBEB0, return 0x1FAA1D. RCX=unit; RDX=text buffer; R8D=0x80 capacity; R9=&caller-local color value. Original builder takes four Windows x64 arguments. QOL preserves RAX as before; the observed caller does not consume it before its next call. It retains the caller's construction/measurement sequence: QOL decoration happens before the caller measures the formatted label. Exact witness is stored in src/placard_call.h and checked using the SDK before publication. This candidate has not yet been verified against a newly running process; the startup guard performs that check.

Chronicle DLL SHA256 85F60C6E4843423A722FE7A85076456AD6C322CD25AD5A1D8D5C5C83AFEFB0D1. Its detour at plugin RVA 0x11B0 calls its original at 0x11C8 and then decorates via 0x11F0. Full original collision evidence is in CHRONICLE-GROUND-FLAG-CONFLICT.md.

## Label ownership and lifecycle

Previous code removed any recognized controller-looking prefix, including a generic [A]/[X]/[Y]/[B] prefix. New ownership records the exact text QOL wrote and the original post-builder text. Hint removal restores only when the current text exactly matches that record. A changed or reused buffer is left untouched and tracking is discarded until rebuilding; native rebuild registration resets ownership and evicts other GUIDs sharing that buffer. Capacity checks are bounded, and full third-party text/color escapes are retained without truncation to make room for a hint. Insufficient room means no added hint.

The existing retained-buffer lifetime/age strategy remains private and best-effort; this patch does not invent a safe public buffer-lifetime contract. External changes after QOL writes may leave the old hint until the next rebuild rather than risk erasing another owner's text. Concurrent mutation outside the game thread is not a supported new guarantee.

Relay is allocated RW, then protected RX and flushed before SDK PatchBytes publication. Module and published relay remain pinned/retained. Shutdown disables substitutions under the placard mutex; delayed calls pass through. SDK owns patch cleanup. No shared builder-entry bytes are altered by QOL.

## Verification

All existing 11 suites pass. New placard suite checks narrow/wide color/suffix preservation, replacing/removing only an owned hint, foreign text resembling a hint, foreign buffer rebuild, unterminated/exact-capacity/insufficient-capacity strings, relative CALL destination and witness consistency.

Install target for this test: Steam mods/Reimagined/d2rloader/plugins/Controller QOL Updates.dll. Existing config is retained, including its current diagnostic settings. No production archive is overwritten.

User runtime checks: both plugins load; log reports Scoped label call installed at game+0x1FAA18; no Chronicle MH_ERROR_ALREADY_CREATED; Chronicle labels remain correct with LB released/held/released; assigned button hints and pickups work; long/colored/hidden labels remain correct; leave/re-enter and rebuild labels; QOL alone and both plugin orders. Stash Search/potion pickup and the native controller path should retain their previous behavior but must not be claimed newly verified until tested.
