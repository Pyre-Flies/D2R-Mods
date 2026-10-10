# Item Roll Ranges

Experimental 1.3.1+rev.23, PyreFly. Separate client plugin for the installed D2RLoader.

Light-blue `[P] [Name] [Tn]` / `[S] [Name] [Tn]` labels identify verified rolled
magic/rare/crafted affix contributions, including fixed-value affixes. Repeated
names show when multiple property lines come from the same affix. T1 is the
highest distinct affix level in the applicable family, filtered by item type,
quality and version. Version 0.4.2 expands skill and grouped-line matching and
adds `[Unique]` / `[Base]` source labels. Original values remain intact.
See [the source audit](docs/SOURCE-AUDIT-0.4.2.md) for current coverage, evidence
and limitations. All five test suites pass; 0.4.2 visual checks remain pending.

Hold **Ctrl** or controller **RT/R2** (**R1/RB** when Controller QOL range remapping is active) in inventory, stash, Cube or vendor to show
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

Revision21 leaves the native Ctrl and selected-controller RT/R2 queries intact.
It applies display enhancements only when Core's guarded render context contains
its distinct range clone. No physical key or XInput polling overrides that
native decision. Controller QOL's existing active RT-to-RB tooltip remap
therefore takes precedence without a QOL update.

Only eligible, focused item-management screens receive this plugin's display
changes. No input events are consumed or synthesized. The loader's hint text
remains native. See [the Defense and input audit](docs/DEFENSE-COVERAGE-AUDIT.md)
for contracts and validation boundaries.

Revision 20 preserves the native current value and adds the verified underlying
base range: `Two-Hand Damage: 32 to 69 (Base: 9 - 19)` or
`Defense: 25 (Base: 2 - 2)` for Bloodrune. Fixed endpoints remain explicit.
Armor, belts, and one-hand, two-hand and throw weapon lines use this display.
Affix roll ranges remain on their individual modifier lines.

Base values include ethereal scaling. Armor generated with Enhanced Defense
uses its native maximum base Defense plus one before ethereal scaling; a
non-ethereal Sturdy Sash therefore shows `(Base: 3 - 3)`. These are intrinsic
values before affix percentage and flat additions, rather than possible total
roll outcomes. Table bounds and the item's primary stat values must agree.

Unrelated triggered skills and per-level modifiers do not prevent a verified
base from displaying. An unresolved header base still gets a gray `(?)` and
`(?) Range Unavailable - Please report item affixes`. Ordinary non-rollable
properties do not trigger the footer. Separate unresolved affix ranges can
still request a report.

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
Historical revision13 controller binding was R1/RB; revision21 restores native RT/R2. Panel/focus gating and light-blue range formatting
are unchanged. Historical 0.3.2 validation above used ALT.


## 0.4.2 source-label audit

Class/element skill bonuses, packed skill-tab selectors, parameterized skills,
fixed-level triggered/charged skills and fully accounted-for grouped lines now
participate in provenance matching. Fixed-value affixes still receive tags and
applicable progression tiers. Unique definitions (including Renewed Sunder
property groups) receive `[Unique]`, not fabricated Prefix/Suffix tiers.
Inherent automagic is presented as `[Base]`. Labels identify matching source contributions,
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

## 1.3.1+rev.2: compact named sources

Rolled affixes now use compact labels such as `[P] [Sharp] [T13]` and
`[S] [Replenishing] [T1]`. This makes separate property lines from one affix
visibly share the same source. The internal term `[Automagic]` is now displayed
as the player-facing `[Base]` label. Attribution, tiers, native ranges and actual
values are otherwise unchanged.

## 1.3.1+rev.3: combined poison attribution

Combined weapon-poison lines can bypass the ordinary single-property formatter.
When a verified loaded `dmg-pois` affix is present and exactly one displayed
property line is otherwise unidentified, that line now receives the affix's
compact name/type/tier label. Multiple unidentified candidates still fail open
without a guessed label. Native poison wording, values, duration and range text
remain untouched.

## 1.3.1+rev.4: composite damage families

The guarded paired formatter observer now preserves provenance for every native
damage-pair family, including fire, lightning, cold, magic and normal damage,
instead of admitting only Enhanced Damage. Labels are still emitted only when
the active affix tables prove the source. Encoded PropertyGroups are followed
only when they contain one fixed, positive-weight choice; randomized or
malformed groups remain unlabeled rather than being guessed.

## 1.3.1+rev.5: composite-line fallback

Live tests established that fire, lightning, cold, magic and multi-element
damage can bypass both native identity observers even while native ranges are
present. The guarded sole-unidentified-line fallback now covers every damage
family. Candidate stats must resolve to one identical complete source label;
competing sources or multiple unidentified lines remain unlabeled.

## 1.3.1+rev.6: multi-line elemental affixes

`Elemental1` emits separate fire, lightning and cold lines from one source.
Multiple unidentified damage lines are now accepted only when their count
exactly equals the number of unobserved contributing damage families and every
family resolves to the same complete source label.

## 1.3.1+rev.7: mixed damage sources

Multiple unidentified damage families may now carry different source labels
when there is an exact one-to-one match. Lines are mapped by the active loaded
`ItemStatCost.descpriority`; mismatched counts or tied priorities fail open.

## 1.3.1+rev.8: stacked damage details

When multiple affixes contribute to one composite damage line, the native
combined value and range remain on top. Each verified source is listed beneath
it in gray with unknown individual numeric contributions, matching the existing
multi-source Enhanced Damage presentation without inventing a split.

## 1.3.1+rev.9: paired-range cleanup

Damage-family attribution now follows Core's buffer order (ascending loaded
display priority), fixing `Elemental1` plus a direct element source. Paired
native bounds such as minimum `27-51` and maximum `63-95` are presented as the
clear overall possibility `[+27 - +95]`; the actual combined damage remains
unchanged.

## 1.3.1+rev.10: stacked sources without native bounds

Verified stacked damage sources now receive gray child rows even when the
provider supplies no native range span, as with `Elemental1` plus `of Flame`.
The actual total remains the `[Combined]` line and no replacement range is
invented.

## 1.3.1+rev.11: partial-provider stacked lines

Core can expose a one-number range for one source while the actual combined
damage line has two numbers. Verified stacked sources now expand from the actual
line despite that key mismatch. The partial provider interval is omitted because
it does not describe the combined total.

## 1.3.1+rev.12: signed native ranges

Range parsing now distinguishes a damage separator such as the dash in
`Adds 1-(6-8) Lightning Damage` from a unary negative sign such as
`-(11-20)% Target Defense`. Negative intervals are displayed in ascending
signed order (`[-20 - -11]`) instead of the misleading `[-(+11 - +20)]`.

## 1.3.1+rev.13: exact stacked elemental rolls

Stacked fire, lightning, and cold damage can now be decomposed when the loaded
property definitions and the combined actual endpoints admit exactly one
solution. For example, T1 `Elemental` contributes fixed `21-50` fire damage;
a combined `22-53` line therefore proves an `of Flame` contribution of `1-3`.
Ambiguous, incomplete, and out-of-range cases retain the existing `?` rows.

## 0.4.7: detail layout and unique ranges

Multi-source details now appear below their combined total in gray. Supported unique scalar properties gain roll ranges from loaded game definitions, including Renewed Black Cleft's Faster Run/Walk (5-10%), Life (10-65), Magic Find (14-25%), Damage Reduced (5-10), and enemy magic resistance (-10% to -5%). Fixed properties keep [Unique] without a range. Native endpoint formatting preserves wording and signs. See docs/SUNDER-RANGES-AND-LAYOUT-0.4.7.md in the source package for verified tables, functions and exclusions.

## 0.4.8 display cleanup

Unknown contribution values retain their gray `?`/`?%` display below the combined total; the redundant `[Roll unknown]` suffix is removed. No hooks, addresses, range calculations, or provenance rules changed.

See docs/PRODUCTION-1.3.1.0.md for the production snapshot and version policy.

Version policy: `1.3.1+rev.N` identifies the qualified loader target and the independent plugin revision. Build metadata does not participate in SemVer precedence. Native fingerprints remain authoritative.

Revision22 fixes Defense publication after Core restores its range context. The Iceblink header fix is user-confirmed in game; all five automated suites pass.

Revision23 restores the Base label on fixed items with no rollable modifiers. A plain Sash displaying Base2-2 while Ctrl is held and returning to normal on release is user-confirmed. Controller-specific no-clone checks remain unconfirmed.
