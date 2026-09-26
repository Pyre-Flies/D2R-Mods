# 0.4.3: Enhanced Damage and no-clone unique tooltips

2026-09-24. Qualified Core 1.3.1-beta / game3.3.93787 unchanged.
Read-only live evidence: PID22776, game base0x140000000, Core base0xC0DE5000000.
These absolute process addresses are observations; hooks use module-relative RVAs.

## Evidence and correction

The 0.4.2 log for the club shows five actual lines but only three captured stats.
The +75% Enhanced Damage line has a native66..80 range, while the single-stat
observer never receives it. Core's property loop falls back in mode1 to its
range helper even when low==high; that helper calls the paired formatter.

| Module-relative location | Verified contract |
|---|---|
| Core+0x3E80AC, return+0x3E80B2 | Variable-bound helper call |
| Core+0x3E80F7, return+0x3E80FD | Equal-bound mode1 actual-value call |
| Core+0x704400 | Both calls' shared pointer slot; live target D2R+0x2D6330 |
| D2R+0x2D6330 | `int(unit, statList, int stat, int layer, int low, int high, char* output, int mode)` |
| D2R+0x2D634D..0x2D6379 | Argument decoding; output stack argument7; table lookup by stat |
| D2R+0x2D64C3..0x2D64EF | Forwards normalized values, layer, grouping flag, buffer and mode to paired formatter+0x2D7850 |

A fifth owned data-pointer adapter observes successful Enhanced Damage stat17/18
results only during our original-item capture pass and at the two exact Core
return addresses. It delegates all eight arguments and preserves the int result.
Other stats, callers, and calls outside a capture pass are unchanged. The existing
function7 contribution mapping resolves the rolled damage affix and its tier.
The generic paired-stat problem remains separate; this change does not guess
contributors to unrelated multi-stat lines.

The Renewed Black Cleft's unique record437 has DWORD flags8 at row+0x2C.
Core+0x377D0D tests that bit and declines its unique range definitions. Therefore
its overlay can contain {original, null}, not {original, rangeClone}. Version0.4.2
rejected that valid no-clone state and also copied Core's flag test into provenance
lookup. Both conditions blocked labels.

0.4.3 accepts an overlay with source==incoming item and clone==null. With no overlay,
the wrapper also leaves the incoming original item unchanged. Mismatched or unreadable
overlays still fail closed. Before annotation, source must be a readable item unit
(type4) with ItemData identified flag0x10 at+0x18. The original-item renderer then
captures its actual properties in one native call with the original arguments and
return value preserved. It avoids a redundant second render when there is no clone. Unique provenance no longer
inherits Core's flag8 exclusion. Flag8 was incorrectly called a disabled-definition
flag in the previous audit; this evidence proves a range-provider exclusion, not
permission to treat the original item's properties as absent.

The native range provider remains unchanged. No range clone is forced and no item
stats or network packets are written. Sunder source labels are fixed here; extra
numeric ranges that Core declines remain outside this fix.

## Lifecycle, guards, validation

The new slot uses the existing pin-before-publication and CAS ownership rules.
All five slots publish before activation; rollback/restoration changes only slots
still owned by the plugin. Late calls retain valid originals/code. Preflight verifies
the exact range-helper target and new caller/ABI witnesses; a conflict prevents
activation rather than overwriting another hook.

`audit_provider.py` now verifies both FF15 calls resolve to0x704400 and records the
mode1 caller window. `audit_property_sources.py --generate` includes the reviewed
helper ABI and paired-call windows; as before, generation follows semantic review.
The existing single observer remains in place for skills and ordinary stats.

Regression coverage: helper argument/result passthrough, unrelated/late calls,
Enhanced Damage source label, original value preservation, null/mismatched overlays,
flag8 unique group expansion, full no-clone render retaining `[Unique]`, and existing
atomic slot ownership tests. All five CTest suites must pass before deployment.

Diagnostic deduplication changed from previous-text-only to a bounded set of32 texts.
Compared items alternate every frame, so the former approach exhausted the quota
with the same two items and hid subsequent Sunder observations. The new set is
bounded per thread and retains the global32-entry limit.

In-game verification remains pending: hold Ctrl/RB over the club and Renewed Black
Cleft, check label appearance and release restoring the native tooltip. Controller
QOL is untouched.0.4.2 DLL is retained as the rollback copy before installation.
