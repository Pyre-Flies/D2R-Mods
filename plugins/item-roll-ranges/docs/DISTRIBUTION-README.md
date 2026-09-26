# Item Roll Ranges

**By PyreFly · Version 1.3.1+rev.13**

Show item roll ranges beside the actual stat value in light blue:

`[+80 - +120] +118 Defense`

Hold **Ctrl** on keyboard or **R1 / RB** on controller. Available in inventory,
stash, Horadric Cube and vendor screens while the game is in the foreground.
Release the button to return to the usual tooltip. Magic/rare/crafted affixes
also show light-blue labels, for example `[+6 - +9] +8 to Minimum Damage [S] [Craftsmanship] [T1]`.
Fixed-value affixes can receive labels too. T1 is the highest affix level in
the applicable family for that item type/quality; duplicate levels share a tier.
The item's current level does not renumber the tiers. Unmatched or ambiguous properties retain their actual value without
a range prefix.

Grouped lines and unknown skill/proc layer encodings may omit labels. Set,
unique, socket and crafted-fixed properties are not assigned invented affix
tiers. Set and unique items still show ranges beside their actual values.
Earlier affix labels were confirmed in game on magic and rare items. The expanded
0.4.2 labels still require visual verification.
Crafted items and every possible stat combination have not all been validated.

## Install

1. Close Diablo II: Resurrected and D2RLoader.
2. Extract into the game folder for a global install at
   `d2rloader/plugins/Item Roll Ranges.dll`. For a mod-scoped install, extract
   into that active mod folder instead. Keep only one active copy.
3. In the effective D2RLoader configuration, enable:

   ```toml
   [d2rcore.items]
   item_stat_ranges = true
   ```

4. Launch through D2RLoader. Keep only one copy of the plugin installed.

## Requirements and compatibility

Requires D2R, D2RLoader plugin ABI 4, and the supported D2RCore build listed in
`COMPATIBILITY.txt`. This standalone plugin does not require Controller QOL,
Python, SDK installation, or another plugin. It can coexist with Controller QOL.
Controller input uses XInput, including compatible Steam Input translation.

This release is build-specific. Unsupported provider/code signatures disable
the plugin and are reported in `d2rloader/logs/item-roll-ranges.log` beneath the
active mod folder. `item-roll-ranges-format.log` records a bounded diagnostic
sample of property text. Cold restart is required after installing or updating.

The range and actual-value display has been confirmed on Aldur's Stony Gaze.
Other item families, controller interactions and localizations have not all
been validated. Header fields outside the property block retain native behavior.
Displayed ranges reflect the active item definitions, including mod changes.

## Uninstall

Close the game and D2RLoader, remove `d2rloader/plugins/Item Roll Ranges.dll`
from the active mod, then relaunch. This restores the native range behavior.

The source archive is distributed separately and includes SDK headers and their
license. `SHA256SUMS` records the files in this runtime archive.


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

Rolled affixes use compact labels such as `[P] [Sharp] [T13]` and
`[S] [Replenishing] [T1]`. Repeated names identify property lines supplied by
the same affix. Inherent automagic is shown as `[Base]`. Native ranges and actual
values remain unchanged.

## 1.3.1+rev.3: combined poison attribution

Combined weapon-poison lines receive their compact affix label when the active
loaded `dmg-pois` source is verified and exactly one tooltip property line is
otherwise unidentified. Ambiguous cases retain native text without a guessed
label. Poison values, duration, localization and native range behavior are not
reimplemented.

## 1.3.1+rev.4: composite damage families

The paired formatter observer now covers all native damage-pair families.
Active affix tables remain authoritative, and encoded property groups are used
only when their single fixed choice makes attribution deterministic. Randomized
or malformed groups are intentionally left unlabeled.

## 1.3.1+rev.5: composite-line fallback

Fire, lightning, cold, magic and multi-element lines now use the same guarded
sole-unidentified-line recovery proven by poison. The loaded affix definitions
must resolve every candidate stat to one identical source label; ambiguous
tooltips retain native text without a guessed tag.

## 1.3.1+rev.6: multi-line elemental affixes

One `Elemental1` affix can emit separate fire, lightning and cold lines. These
are labeled together only when line count and loaded family count match exactly
and every family proves the same complete source label.

## 1.3.1+rev.7: mixed damage sources

Different unresolved damage families can now receive different affix labels
when their count matches exactly. Native loaded display priority determines the
mapping; ambiguous counts and tied priorities remain unlabeled.

## 1.3.1+rev.8: stacked damage details

Composite damage supplied by multiple affixes keeps its native combined line
and range, with one gray unknown-contribution row per verified source below it.
No individual elemental or poison roll is inferred.

## 1.3.1+rev.9: paired-range cleanup

Mixed elemental sources now follow native buffer order. Two-part native damage
bounds are collapsed to their lowest possible minimum and highest possible
maximum, for example `[+27 - +95]`, while actual values remain untouched.

## 1.3.1+rev.10: stacked sources without native bounds

Gray source rows no longer require a native range span. If the provider has no
range for a verified stacked line, its actual value remains `[Combined]` and
the plugin does not manufacture a replacement range.

## 1.3.1+rev.11: partial-provider stacked lines

Stacked sources can expand when Core supplies a one-number range for one source
but the actual combined damage line has two numbers. That incomplete interval
is not shown as though it covered the total.

## 1.3.1+rev.12: signed native ranges

Range parsing now distinguishes damage separators from unary negative signs.
Positive elemental spans remain positive, while reductions such as Target
Defense display an ordered signed interval like `[-20 - -11]`.

## 1.3.1+rev.13: exact stacked elemental rolls

When loaded property definitions prove each fire, lightning, or cold endpoint
and the combined total has one unique decomposition, gray source rows now show
their exact contributions. Ambiguous cases continue to display `?`.

## 0.4.7: detail layout and unique ranges

Multi-source details now appear below their combined total in gray. Supported unique scalar properties gain roll ranges from loaded game definitions, including Renewed Black Cleft's Faster Run/Walk (5-10%), Life (10-65), Magic Find (14-25%), Damage Reduced (5-10), and enemy magic resistance (-10% to -5%). Fixed properties keep [Unique] without a range. Native endpoint formatting preserves wording and signs. See docs/SUNDER-RANGES-AND-LAYOUT-0.4.7.md in the source package for verified tables, functions and exclusions.

## 0.4.8 display cleanup

Unknown contribution values retain their gray `?`/`?%` display below the combined total; the redundant `[Roll unknown]` suffix is removed. No hooks, addresses, range calculations, or provenance rules changed.

Production 1.3.1.0 disables formatter diagnostic dumps.

Release identity uses 1.3.1+rev.N; see VERSIONING.md in source documentation.
