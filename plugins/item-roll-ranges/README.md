# Item Roll Ranges

Experimental 1.3.1+rev.1, PyreFly. Separate client plugin for the installed D2RLoader.

Light-blue `[Prefix] [Tn]` / `[Suffix] [Tn]` labels identify verified rolled
magic/rare/crafted affix contributions, including fixed-value affixes. T1 is the
highest distinct affix level in the applicable family, filtered by item type,
quality and version. Version 0.4.2 expands skill and grouped-line matching and
adds `[Unique]` / `[Automagic]` source labels. Original values remain intact.
See [the source audit](docs/SOURCE-AUDIT-0.4.2.md) for current coverage, evidence
and limitations. All five test suites pass; 0.4.2 visual checks remain pending.

Hold **Ctrl** or controller **R1/RB** in inventory, stash, Cube or vendor to show
native roll ranges as light-blue prefixes beside actual item properties, e.g.
`[+80 - +120] +118 Defense`. Release returns to the usual tooltip.

The plugin renders property text twice: the original item without range data,
and Core's temporary range item with native range data. It preserves the first
pass's actual text and prefixes uniquely matching lines with the second pass's
range annotations. This covers numeric modifiers, percentages, damage pairs,
skill/proc text and grouped modifiers without a description-function allowlist.
Native localization, units and actual stat text are retained. Fixed properties
retain their values and can receive affix labels. Ambiguous/unmatched lines retain actual values without a
prefix; no bounds are invented. If prefixes exceed the buffer capacity, the
whole original property block is retained. Ranges reflect current definitions.

## Status

Live traces confirm the original-item pass produces +118 Defense and +30%
Enhanced Weapon Damage, while the range-item pass produces 80..120 and25..50.
0.3.0 failed to recognize the runtime U+E07E color introducer. 0.3.2 fixes that
parser mismatch; an exact captured-buffer regression verifies both light-blue
prefixes, all actual values and unchanged fixed lines. Four suites passed for 0.3.2. The
user confirmed the rendered Aldur result; its live trace shows two light-blue
prefixes and preserved +118 Defense / +30% ED. Broader item/controller coverage
still needs validation.

## Install / remove

With D2R closed, copy `Item Roll Ranges.dll` into
`mods/Reimagined/d2rloader/plugins`. Keep `[d2rcore.items] item_stat_ranges = true`
in the effective loader configuration (already true in the inspected global
config). Keep only one copy of this plugin. The QOL DLL is separate.

Restart through D2RLoader and inspect `item-roll-ranges.log` in the mod's plugin
log directory. A provider mismatch disables this plugin. To remove it, close
D2R and move this DLL out of the plugins directory; restart restores the stock
Ctrl/right-trigger behavior. The DLL pins itself after successful preflight so
already-fetched callbacks remain mapped during loader unload. Hot reload is
unsupported; use a cold restart for updates.

R1/RB is read from the selected XInput user, not from every connected pad. This
candidate requires XInput (including Steam's controller translation). Other
input backends and native R1 side effects are unqualified. No input is consumed
or synthesized, and the loader's existing hint text is not changed yet.

## Build

From an x64 Visual Studio developer prompt with CMake 3.29+:

```text
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The monorepo pins one reviewed SDK submodule under `third_party/D2RLoader-PluginSDK`.
`-DPLUGIN_SDK=<path>` may point to another explicitly reviewed SDK checkout.
Only SDK headers are used. `tools/audit_provider.py D2RCore.dll` checks the investigated provider;
the disassembly tools require capstone and pefile. Their local dependency copy
and private game disassembly are not runtime dependencies or redistributable
source-package contents.

The first 16 scoped property blocks are logged in item-roll-ranges-format.log,
with separate ACTUAL, RANGED and RESULT text, annotation counts and unmatched counts.
No original SDK context, item pointer or rendered text is retained between calls.

0.3.3 changes the keyboard hold from ALT to Ctrl (either left or right Ctrl).
Controller remains R1/RB. Panel/focus gating and light-blue range formatting
are unchanged. Historical 0.3.2 validation above used ALT.


## 0.4.2 source-label audit

Class/element skill bonuses, packed skill-tab selectors, parameterized skills,
fixed-level triggered/charged skills and fully accounted-for grouped lines now
participate in provenance matching. Fixed-value affixes still receive tags and
applicable progression tiers. Unique definitions (including Renewed Sunder
property groups) receive `[Unique]`, not fabricated Prefix/Suffix tiers.
Automagic is marked `[Automagic]`. Labels identify matching source contributions,
not a guarantee that the source accounts for an entire stacked stat value.

Coverage is not universal: automatically calculated proc levels, variable group
parameters, unidentified/ambiguous text, uncaptured paired formatters, socket
contributions, base stats, set bonuses and fixed crafting recipe sources still
need separate treatment. See `docs/SOURCE-AUDIT-0.4.2.md` in the source package.


## 0.4.3 follow-up

Enhanced Damage now supplies its stat identity through the native range/paired
formatter, allowing verified Prefix/Suffix tiers alongside its existing range.
Identified original items can receive source labels even when Core creates no
range clone. This fixes `[Unique]` labeling on Renewed Sunders with unique flag8.
It does not force Core to calculate ranges that its provider declines to build.
Diagnostics retain up to32 distinct actual tooltip texts, including comparisons.
The five suites pass; in-game visual confirmation of0.4.3 remains pending.

## 0.4.4 affix breakdown

Known affixes now show their names and tiers. Supported scalar magic/rare lines expand into individual contributions when the native range agrees with the source bounds. Exact values appear only when the total uniquely determines them. Otherwise the combined total is retained with individual ranges and explicitly unknown rolls. Numeric unique/Sunder ranges and complex grouped properties are not added in this release. See docs/AFFIX-BREAKDOWN-0.4.4.md in the source package for evidence, addresses, and limitations.

## 0.4.5: affix types and combined Enhanced Damage

Affixes display [Prefix] or [Suffix] and their tier, replacing the affix names introduced in 0.4.4. Combined Enhanced Damage uses the verified native special-property formatter, which runs before ordinary stat rendering. Its appended line is now observed directly. Exact/ambiguous roll decomposition rules are unchanged. All five test suites pass; in-game verification remains required. Research and patch recovery details: docs/ENHANCED-DAMAGE-0.4.5.md in the source package.

## 0.4.6: multiple Enhanced Damage prefixes

When the native ED range covers only one of multiple verified prefixes, Ctrl/RB now retains the combined total and lists each prefix's own range and tier on a separate line. Individual rolls are explicitly unknown on this incomplete-provider path; no split is invented. See docs/MULTIPLE-ED-PREFIXES-0.4.6.md in the source package for the bow regression and native assignment addresses.

## 0.4.7: detail layout and unique ranges

Multi-source details now appear below their combined total in gray. Supported unique scalar properties gain roll ranges from loaded game definitions, including Renewed Black Cleft's Faster Run/Walk (5-10%), Life (10-65), Magic Find (14-25%), Damage Reduced (5-10), and enemy magic resistance (-10% to -5%). Fixed properties keep [Unique] without a range. Native endpoint formatting preserves wording and signs. See docs/SUNDER-RANGES-AND-LAYOUT-0.4.7.md in the source package for verified tables, functions and exclusions.

## 0.4.8 display cleanup

Unknown contribution values retain their gray `?`/`?%` display below the combined total; the redundant `[Roll unknown]` suffix is removed. No hooks, addresses, range calculations, or provenance rules changed.

See docs/PRODUCTION-1.3.1.0.md for the production snapshot and version policy.

Version policy: `1.3.1+rev.N` identifies the qualified loader target and the independent plugin revision. Build metadata does not participate in SemVer precedence. Native fingerprints remain authoritative.
