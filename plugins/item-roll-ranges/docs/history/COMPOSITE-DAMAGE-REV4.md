# Rev.4 composite-damage attribution

Date: 2026-09-26. Target remains the qualified Core 1.3.1-beta / game
3.3.93787 profile documented by the existing native-contract records.

## Runtime evidence

The existing guarded paired-helper observer saw the native cold-damage range
`Adds (5-9)-(16-50) cold damage`, but rev.3 discarded its identity because the
capture function admitted only stats 17 and 18 (Enhanced Damage). The helper
ABI, call sites, pointer slot, and byte guards are unchanged from
`FOLLOWUP-0.4.3.md`:

- Core return RVAs `0x3E80B2` and `0x3E80FD`;
- Core pointer slot `0x704400`, resolving to D2R `0x2D6330`;
- ABI `int(unit, statList, stat, layer, low, high, output, mode)`.

Rev.4 captures the stat/layer identity from every successful call at those two
already-qualified call sites. This does not itself produce a label. The loaded
MagicPrefix/MagicSuffix/Properties tables must still prove that an active
source contributes the captured identity.

Expected paired families include physical min/max, fire, lightning, magic,
cold, and poison. Poison retains its separate guarded fallback because its
combined native line can bypass both the single-property and paired-helper
observers.

## Encoded PropertyGroups

Affix property IDs with high WORD `1` address the loaded PropertyGroups table
at table pointer `+0x258`, count `+0x260`, stride `0xC8`. Each row contains up
to eight 24-byte choices beginning at `+0x08`:

`property, parameter-min, parameter-max, min, max, weight`

The layout was already consumed by unique-source expansion; rev.4 adds bounded
magic/rare contribution traversal. A group is attributable only when:

- its mode is in the observed range 0..2;
- exactly one entry has positive weight;
- that entry has a fixed parameter (`parameter-min == parameter-max`);
- nested groups terminate within four levels; and
- the final property/stat/layer mapping is valid in the loaded tables.

Multiple positive-weight choices are randomized and therefore ambiguous. They
are deliberately left unlabeled, as are cycles, excessive nesting, malformed
modes, and out-of-range group/property IDs. Scalar range decomposition remains
limited to direct additive properties; group traversal proves provenance only.

## Validation status

Automated: Release build passed range-policy, range-adapters, range-artifact,
range-text, and range-affixes. Tests cover generic paired-helper capture,
deterministic nested groups, and fail-open randomized/cyclic/out-of-range groups.

Live rev.4 result: poison suffixes and physical `Jagged` worked. Fire
(`Smoldering`/`of Flame`), lightning (`Glowing`/`of Shock`), cold
(`Shivering`/`of Frost`), magic (`Apprentice`), and `Elemental1` showed native
ranges but no source labels. This establishes that those lines bypass both
identity observers on the qualified build; generic capture at the existing
paired-helper call sites was insufficient.

Rev.5 therefore generalizes the already proven poison recovery. It acts only
when exactly one displayed line is unidentified, skips a damage family if any
of its member stats was observed natively, and requires every remaining loaded
damage candidate to resolve to the same complete source label. This permits one
multi-stat `Elemental1` source while refusing multiple unidentified lines or
competing affix labels. Rev.5 live validation confirmed the individual fire,
lightning, cold, magic, poison, and physical cases; `Elemental1` remained
unlabeled because it emits three unidentified native lines. Rev.6 admits that
shape only when the unidentified-line count exactly equals the contributing
family count and all families prove the same complete source label. No new RVA,
hook, signature, ABI, or structure offset was introduced in these revisions.

Further rev.6 live combinations showed that equal-family sources correctly
combined on one line, while different families (fire+lightning, fire+poison,
fire+Elemental1, and magic alongside another quiver prefix) remained unlabeled.
Rev.7 permits different family labels only with exact cardinality, ordering the
families by the active compiled ItemStatCost `descpriority` WORD at row `+0x30`.
The source is table pointer `tables+0x1258`, count `tables+0x1260`, stride
`0x144`, already validated for existing display metadata. Equal priorities,
invalid tables, or count mismatches omit every fallback label. This adds no new
hook or RVA; the `+0x30` field is now an additional consumed layout detail and
requires live confirmation on the qualified build.

Rev.7 live validation confirmed that these mixed-family labels attach. Rev.8
adds presentation-only decomposition for a single composite damage line with
multiple verified sources. The native combined actual value and native range
remain authoritative. Each source is repeated on a gray child row with numeric
tokens replaced by `?`; no individual contribution is calculated or inferred.
This reuses the established Enhanced Damage child-row ordering and `[Combined]`
layout and introduces no native dependency.

Rev.8 live screenshots showed two remaining issues. Core appends property lines
in ascending `descpriority` and D2R displays the buffer bottom-to-top; the prior
descending reconciliation therefore put `of Flame` on the cold line of an
`Elemental1` item. Rev.9 uses ascending priority for buffer matching. The same
screenshots showed that the hyphen between two colored damage spans was parsed
as a unary negative sign. It is now treated as the min/max separator whenever a
first span already exists. Player-facing paired bounds collapse from the two
component intervals to the lowest possible minimum and highest possible maximum
(for example `27..51` and `63..95` becomes `[+27 - +95]`). Native actual values
and the underlying captured intervals are unchanged.

The rev.9 screenshot confirmed correct `of Flame` placement on the fire line,
but no child rows appeared because `Elemental1` supplied no native range span.
Rev.10 removes that presentation-only prerequisite: two or more verified source
labels can expand beneath the actual `[Combined]` line with unknown numeric
contributions even when the provider range is empty. The plugin still emits no
range bracket in that case.

Rev.10 diagnostics captured the actual remaining mismatch: the actual fire line
was `Adds 22-53 Weapon Fire Damage` (key `Adds##WeaponFireDamage`), while Core's
ranged line covered only the direct suffix as `+(2-5) Weapon Fire Damage` (key
`#WeaponFireDamage`). The verified label already contained both `Elemental1`
and `of Flame`, but range reconciliation requires equal keys. Rev.11 permits
unknown source expansion from the actual line when there are at least two
verified sources even if Core's partial ranged key differs. The partial 2..5
interval is deliberately omitted because it is not the combined line's range.
