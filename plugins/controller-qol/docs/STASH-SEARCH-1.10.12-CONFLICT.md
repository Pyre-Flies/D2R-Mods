# Stash Search 1.10.12 startup conflict — 2026-09-24

Binary SHA256: d2305982248980583c54e4db73c9cf7973dd7dcd1c71e2ef7eb9ca8f794e1a20. Analysis of installed binary, not third-party C++ source. Public folder contains DLL/layout/readme/archive: https://github.com/yinyin333333/D2RL-yin-Junk-Room/tree/main/for%20d2rloader/StashSearch

User A/B: loads without QOL, fails with QOL1.3.1.2. Latest loader log 16:34:10.097 reports Stash Search could not be loaded. stash-search.log contains only EF BB BF (UTF-8 BOM), not a useful diagnostic. Previous successful log at16:31:36.777 lists1.10.12. These logs are overwritten by subsequent launches.

## Confirmed static conflict
QOL src/qol_glyphs.cpp Initialize installs SDK inline hook at game RVA0x902E20 (native text renderer), then0x86D410 (widget draw). This implements controller prompt text/spacing, not stash item movement.

StashSearch DLL export LoadPlugin RVA0x11800 calls initializer0xC5A0. Initializer0xC8B7 invokes game-byte validator0x10500; false at0xC8BE exits through0xCD24/0xCD29 returning false without diagnostic. Validator0x109F9..0x10A0F checks game RVA0x902E20 against32 bytes at StashSearch RVA0x14650. Helper0x1290 reads32 bytes via0xF1F0 and compares via0x132DA; no hook-chain tolerance. Expected bytes start48 89 5C 24 08. QOL's entry detour necessarily differs, making this a deterministic incompatibility when QOL loads first, independently of the absence of a detailed runtime log. Not a new XInput site.

Stash Search additionally validates other native sites/Core slots, starts a Windows input-hook worker, and registers a17-byte SDK inline hook at game RVA0x23AA60 (registration built at plugin0xCC4A..0xCC7E). QOL does not hook that entry. Do not claim all its runtime behavior is mapped from this limited audit.

See stash-search-1.10.12-evidence.txt for instructions and exact32-byte witness.

## Remediation direction (not implemented)
Prefer narrowing QOL glyph rendering interception so the global0x902E20 entry stays original, retaining prompt behavior. Verify a widget-scoped call path or supported service before moving the hook; cannot assume every draw call uses a Core slot. Do not patch Stash Search's validator or temporarily hide QOL bytes. Loading Stash Search first is only a potential workaround; not verified and not a robust architecture fix.

Separate screenshot finding: loader UI renders our four-component metadata versions as invalid. Investigate the loader's accepted version syntax before changing the requested1.3.1.X identity. No version or DLL changes made in this audit.
