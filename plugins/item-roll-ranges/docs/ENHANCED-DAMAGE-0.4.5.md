# 0.4.5: combined Enhanced Damage observer and affix types

2026-09-24, qualified Core1.3.1-beta/game3.3.93787. Live read-only PID49708;
game base0x140000000, Core base0xC0DE5000000. No game-state writes during research.

## Root cause, including correction of earlier conclusions

0.4.4's log has four captured identities but only three attached labels for
the club. Expanding capture to the range pass did not fix the combined ED line.
The ordinary stat17/18 paired formatter is the WRONG source for that identity.
Their loaded description function is19 and their localized strings3508/3509 are
`%+d%% Enhanced Maximum Damage` / `%+d%% Enhanced Minimum Damage`.
These are distinct from the combined `Enhanced Damage` property.

Core calls its special property formatter BEFORE either ordinary formatter.
That routine handles equal minimum/maximum damage percentages, appends the
combined line directly to the caller's output, then returns handled. The ordinary
observer never sees that emitted line. No translated-text substitution or
hardcoded English match is required to fix this.

## Recovery landmarks and ABI

| Module-relative location | Contract established from live code |
|---|---|
| Core+0x7043A0 | Existing data-pointer slot, target D2R+0x2DB800 |
| Core+0x3E7FB0..0x3E7FE7 | Loads special context, stat, output, capacity, fifth byte flag; calls slot at+0x3E7FD0; return+0x3E7FD6; handled result skips ordinary path |
| D2R+0x2DB800 | `int(context*, int stat, char* output, size_t capacity, unsigned char flags)` |
| D2R+0x2DB82C..0x2DB840 | R8 output preserved in R14, R9 capacity in R15, EDX stat in EBX, RCX context in RDI |
| D2R+0x2DB848..0x2DB875 | Dispatch by stat-17 through byte table+0x2DBF20 and DWORD branch table+0x2DBEE4 |
| D2R+0x2DB8A9 | Stat17 returns context+0x40 handled flag without appending |
| D2R+0x2DBCEA | Stat18 renders combined ED using context+0x28/+0x2C, handled flag+0x40, variable range flag+0x4C |
| D2R+0x2DBD2E / +0x2DBDE7 | Uses localization key at D2R+0x1CF5AE0, length20: strModEnhancedDamage |
| D2R+0x2DBE41 | Reads fifth byte argument |
| D2R+0x2DBE77..0x2DBEB9 | Appends formatted local buffer to caller output, adds native separator, returns integer handled status |

Research-only localization recovery: D2R thunk+0x3E2B730 reads pointer slot
+0x3E2AE10 -> Core+0x81F290 -> Core+0x388320. The current locale byte is reached
through Core pointer+0x704098. Locale shared pointer table is Core+0x7DC080,
stride16; cached string pointers are object+0x50+ID*8. This established the exact
loaded stat17/18 descriptions. These addresses are NOT plugin dependencies.
Property29's sole active slot is function7 with implicit physical stats, already
handled by the existing source mapper.

`tools/audit_ed_identity.py <pid>` reproduces the table, localization, dispatch,
and special-formatter reads. `--generate` captures only the reviewed witness
windows above in `src/special_profile.h`. Do not regenerate on a new patch
without rechecking semantics. Game witnesses exclude relocatable localization
thunk calls. Module hashes and existing profiles still gate installation.

## Implementation and lifecycle

The sixth adapter uses the existing Core slot, not an executable detour. It
delegates once with all five arguments and the full int return unchanged.
Only the admitted caller, an active capture pass, and stat17/18 are observed.
The adapter measures output length before/after and records ONLY the appended
segment. A handled call that emits nothing cannot relabel earlier output.
Leading/trailing CR/LF separators are removed from that segment; empty, oversized,
unterminated or multi-line segments are excluded. The captured identity remains
locale-independent because it comes from the native rendered text.

Slot target and live caller/ABI/dispatch/append/return witnesses are checked before
publication. The existing pin-before-publication, compare/exchange ownership,
all-adapters-before-activation, and owned-slot rollback/restoration rules apply.
No item stats, input state, packets or game executable bytes are modified.

Affix labels now display `[Prefix]` / `[Suffix]` plus the existing tier, including
expanded source lines. The user explicitly withdrew the affix-name preference.
Unique/Automagic sources remain unchanged. The 0.4.4 exact/ambiguous numeric split
rules remain: never manufacture individual rolls from an ambiguous total.

The bounded format log now includes captured stat/layer/group/key identities
alongside the final text, so future matching failures have direct evidence.

## Validation and expected check

All five CTest suites pass with /W4 /WX. The added adapter regression uses a
buffer that already contains another property, appends `+75% Enhanced Damage`
with a native newline, and verifies only the ED line receives `[Prefix] [T3]`
and66..80 bounds. T3 is synthetic in that fixture, not an assertion about the
live club's applicable tier. Additional cases cover handled-but-no-output,
unrelated callers, late passthrough, multiline rejection, and full int ABI.
The real club should display `[66%-80%] +75% Enhanced Damage [Prefix] [Tn]`,
with n calculated from its loaded eligible affix family. In-game confirmation
remains required. No new Sunder range support is claimed. Controller QOL untouched.
