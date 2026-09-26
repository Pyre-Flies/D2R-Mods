# Item Roll Ranges 0.4.1: D2RCore 1.3.1-beta migration

Investigated 2026-09-24. Source: `Documentation/item-roll-ranges` in the game root.
Qualified Core SHA256: `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Game: 3.3.93787. SDK ABI stays 4; this failure was a private provider profile mismatch, not an ABI migration.

## Cause and discovery

0.4.0 rejects this new Core file hash before writing any slot. Its generic error conflated unavailable providers and conflicts. No evidence of an actual competing hook caused this reported failure.

Named PE exports locate the provider (`BuildItemTooltipWithStatRanges`), overlay wrapper (`FormatItemPropertiesWithTooltipOverlay`) and wide-stat copy (`CopyWideListStat`). `migration_candidates.py` locates old instruction sequences while masking encoded RIP references and relative branches only. This is a discovery tool, never an automatic compatibility acceptance mechanism. Preserve the old profile from the 0.4.0 source archive when repeating that comparison.

Nine previous witness regions matched after relocation masking. The panel witness had two candidates; the paired Cube 0x19/stash 0x18 checks at 0x328981/0x32899A establish the chosen caller and common slot. Overlay regions required additional manual review: the TLS field changed, so they intentionally did not match the relocation-only search.

## Current contract (all addresses are module-relative RVAs)

| Core role | RVA | Evidence / live target |
|---|---:|---|
| BuildItemTooltipWithStatRanges | 0x819530 | Named export; selection witness 0x819587..0x8196CC |
| Modifier call / return | 0x8195DA / 0x8195E0 | FF15 to slot 0x6FE3B0; D2R+0x1209FA0 |
| Controller call / return | 0x819954 / 0x81995A | RCX input, EDX selected user, R8D=0x800; slot 0x6FE470; D2R+0x13CA70 |
| Input mode root slot | 0x701BB0 | Load at 0x8195E4; dereference, mode +0xDC |
| TestUiMode slot | 0x702AE0 | Calls 0x328986 / 0x32899F; D2R+0xCE500 |
| Properties wrapper | 0x81D4B0 | Named export; unchanged 10-argument property ABI |
| Properties slot / return | 0x704490 / 0x81D53B | Loaded at 0x81D515, call R14 at 0x81D538; D2R+0x2DC4B0 |
| Single formatter slot / return | 0x7043E8 / 0x3E82EA | FF15 at 0x3E82E4; D2R+0x2D6520 |
| Tables slot | 0x701E70 | FF15 at 0x815451; D2R+0x300A90 |
| Eligible-affix slot | 0x6FF370 | RCX unit, RDX row at 0x8157D2; call 0x8157D8; D2R+0x3D4220 |
| TLS index global | 0x7DF224 | RIP loads at 0x81D4E5 and 0x81A2E9 |
| Overlay TLS field | block+0x1840 | Was +0x490; GS:[0x58], indexed TLS block |
| Overlay installation/restoration | 0x81A2D4..0x81A378 | Local {source, clone, previous}; saves and restores same TLS field |
| CopyWideListStat | 0x832940 | Named export; 16-byte stride at 0x8329BF |
| WideRead | 0x3D88B1..0x3D8934 | Original wide-stat interpretation retained |
| WideRangeCaller / WideActual | 0x3E807C / 0x3E7E82 | Original paired range / actual consumers retained |
| Affix selection | 0x81544A..0x815506 | Rows+0x15E8, count+0x15F0; suffix/prefix/auto+0x1600/08/10; stride140 unchanged |

Wrapper checks overlay.source==incoming item then replaces RCX with overlay.clone. Plugin sees the clone at its property adapter and recovers overlay.source only when the identities match. ABI, member order and scope lifetime remain unchanged; only TLS index RVA and block offset migrated.

## Verification and patch recovery

Read-only live audit used main-menu PID60876, Core base 0xC0DE5000000, game base 0x140000000. These absolute bases are observations, not portable addresses. All 12 current Core witness regions exactly matched disk/generated profile; all seven existing game affix witnesses matched unchanged, including tables, item metadata, affix IDs, eligibility, single formatter and property rows. All seven slot targets were inspected against the above table. The new build continues to require an exact file hash and live witnesses before CAS publication; no bypass was introduced.

Run `tools/audit_provider.py <D2RCore.dll> --generate` only after manually qualifying a new build; its semantic assertions verify export, call slots, input masks, wide stride, TLS reference and eligible caller. `migration_live.py <pid>` is read-only, checks the captured module bases, prints live slots and validates both witness sets. Pass the optional Core/game base arguments if ASLR changes; do not mistake old process IDs for persistent identities. `audit_affixes.py` is a historical Reimagined table-to-text audit and must not be used against vanilla data without adapting its table source.

Build with MSVC x64/CMake; run all five CTest suites. Then restart and verify Ctrl/RB held tooltips on a known set/unique roll plus a magic/rare affix, release restoring normal text, and inventory/stash/Cube/vendor coverage. Main-menu code validation and unit tests do not prove in-game visual behavior. Crafted/grouped-stat limitations documented in the original contract remain.

No Controller QOL binary or configuration was changed.
