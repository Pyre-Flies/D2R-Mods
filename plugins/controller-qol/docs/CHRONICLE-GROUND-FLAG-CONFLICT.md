# Chronicle Ground Flag / QOL label hook collision

Investigated 2026-09-26. Original diagnosis; subsequently implemented as candidate rev.6. See CHRONICLE-COMPATIBILITY-1.3.1-rev.6.md for implementation, tests and deployment scope.

## Evidence

Installed plugin: `<game-root>\mods\Reimagined\d2rloader\plugins\d2rl-chronicle-ground-flag.dll`.
SHA-256: `85F60C6E4843423A722FE7A85076456AD6C322CD25AD5A1D8D5C5C83AFEFB0D1`.
Embedded description: “Marks unidentified ground items missing from your Chronicle.” Do not assume arbitrary rarity coloring from this description.

Local logs: QOL reads/installs ground-label function game RVA 0xCBEB0 at 2026-09-26 06:32:16.847; Chronicle reports MH_ERROR_ALREADY_CREATED on that same RVA at 06:32:18.022. QOL source installs its entry detour in plugin_main.cpp and its callback invokes original before registering the text buffer and adding controller prefixes. This is duplicate hook ownership, distinct from expected-byte mismatch. Changing load order alone does not provide two cooperating hooks.

Chronicle binary-relative RVAs (not game addresses): export LoadPlugin 0x12E0; at 0x1419/0x1422 it copies five live bytes from game+0xCBEB0; registration at 0x1434 specifies game RVA 0xCBEB0, expected length 5, detour 0x11B0 and original-pointer storage 0x4090. Detour 0x11B0 calls original at 0x11C8, then forwards unit/text/capacity to helper 0x11F0. Helper references MissingChronicleSuffix and invokes text helper 0x1000. This supports original-then-decoration composition. The full formatting/return-value contract is not yet qualified.

## Candidate remedy and verification needed

The prior private game capture has one detected direct relative CALL to game 0xCBEB0: game RVA 0x1FAA18, bytes `E8 93 14 ED FF`, return 0x1FAA1D, inside wrapper beginning 0x1FA9F0. This was an offline scan of the previous capture, not a new live verification or proof there are no indirect/Core callers. Pre-call code sets R8D=0x80 and R9=&local color storage; RCX retains unit and RDX label buffer. The caller performs further processing after return.

Proposed: remove QOL ownership of the shared 0xCBEB0 entry and patch a qualified label-construction call site using the SDK patch API and existing near-relay pattern. QOL wrapper calls the public game entry at runtime (therefore entering Chronicle's hook if installed), waits for the complete formatting chain, then registers/adds QOL hints to the resulting text. It must not bypass Chronicle by caching its trampoline or removing its hook. Preserve original return value, arguments, colors and full buffer-capacity semantics.

Before implementation/deployment: verify current live caller and all relevant label paths; qualify its signature; inspect buffer lifetime and subsequent caller changes; ensure refresh/release strips only QOL-owned additions without deleting Chronicle text; verify labels/colors and filtered/hidden items, modifier release, rebuilds, both plugin orders, and QOL alone. Long labels/capacity bounds also need coverage. Existing prefix-removal heuristics are not a universal ownership contract and require review for this composition.

This does not require a public controller service. A loader-owned ground-label formatting/contribution event with stable item identity, capacity/lifetime rules, separate color/text contributions and ordered owners would remove the broader shared-formatting problem. SDK inline-hook availability alone does not imply automatic multi-plugin detour chaining.
