# 0.4.6: separate prefixes when the native ED range is incomplete

2026-09-24. Same qualified Core/game build and hooks as0.4.5. Read-only live
PID13500, game base0x140000000, Core base0xC0DE5000000.

## Evidence

Bramble Song Diamond Bow, item code268, quality6. Unit0x3CF3AD8B0,
ItemData0x3733B6560, StatListEx0x36B8CD950, child0x37ED5A8D0.
Stored affixes:

| ID | Internal source | Definitions relevant to the tooltip |
|---|---|---|
|976|Savage|property29 (dmg%),66..80; displayed Prefix T4|
|1043|Fine|property15 (attack rating),21..40; property29,21..30; Prefix T6|
|1202|Triumphant|property110,1..1|
|198|of Slaying|property28,5..7|

The child list contains17=95,18=95,19=37,24=6,138=1 (all layer0).
No separate66..80 /21..30 roll values exist in this list. The log captures
stat18 with the correct combined key, so0.4.5's special observer is working.
The remaining error is range accounting and presentation:

* Actual: `+95% Enhanced Damage`.
* Provider: `+(21-30)% Enhanced Damage`.
*0.4.5 attached both Prefix tags to the combined line after rejecting expansion
 because21..30 does not equal the two source intervals' sum87..110.

## Native explanation and recovery locations

D2R+0x2DAED0 prepares special-property ranges; its definition loop advances
16 bytes at+0x2DB32F and returns to+0x2DAF48. At+0x2DB302 it compares property
ID against0x1D (29). The branch+0x2DB308..0x2DB329 assigns definition low/high
to context+0x28/+0x30 and+0x2C/+0x34, setting flags+0x48/+0x4C. These are MOV
assignments, not accumulated bounds. A second dmg% definition replaces the first.
The already documented special renderer+0x2DBCEA consumes this context.

Reproduce item metadata with `tools/audit_contributions.py <pid> --adder`.
Disassemble assignments with `tools/audit_property_sources.py <pid>
0x1402db302 0x32` for this observed game base. Adjust module base after patches.
These assignment addresses are research landmarks; no new runtime calls or
hooks depend on them. Existing complete source metadata supplies each interval.

## Fix and constraints

For complete supported magic/rare ED provenance with at least two sources,
the renderer may expand when the provider interval matches ONE source interval
instead of the sum. This exception is restricted to ED17/18, layer0, with all
contributors accepted by ScalarBounds. Other mismatches still fail closed.

On this incomplete-provider path, exact decomposition is explicitly disabled.
The total must still fit the sum of source bounds; no per-affix numbers are
guessed, even at an endpoint. The output is:

```
+95% Enhanced Damage [Combined]
[66%-80%] ?% Enhanced Damage [Prefix] [T4] [Roll unknown]
[21%-30%] ?% Enhanced Damage [Prefix] [T6] [Roll unknown]
```

The misleading21..30 range is no longer attached to the combined95 value.
Each source gets its own line and one Prefix/tier pair. Ordinary complete-range
cases retain the existing exact-decomposition rules. Normal non-Ctrl/RB output
and Controller QOL are unchanged.

All five suites pass. New regression uses the exact bow total, native interval,
source bounds and tiers, verifies three lines and one tier per source, and
checks no-exception, unrelated interval, endpoint uncertainty and capacity
fallback. Game visual verification remains required after installation.
