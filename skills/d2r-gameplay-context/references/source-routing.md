# Source routing

Use current sources for exact or changeable gameplay claims.

## D2R Reimagined

### Official wiki

`https://wiki.d2r-reimagined.com/`

Use for new-player guidance, system explanations, progression, farming concepts, item-system explanations, class-change summaries, patch notes, and historical rationale. Check the page's edit or patch date when available.

### Official game-data site

`https://www.d2r-reimagined.com/data`

Use for current structured lookups and calculators:

- `/data/skills` — class trees, prerequisites, ranks, formulas, and synergies;
- `/data/ias-calculator` — Reimagined attack-speed calculations;
- `/data/drop-calculator` — monster and item drop probabilities;
- `/data/uniques` and `/data/sets` — complete item properties;
- `/data/runewords` and `/data/bases` — base compatibility and item data;
- `/data/affixes` — prefix/suffix eligibility and properties;
- `/data/cube-recipes` — recipe inputs, outputs, categories, and notes;
- `/data/orbs` — orb behavior and corruption outcomes.

Prefer the dedicated catalog over a wiki summary when the user needs exact current properties. Recheck routes if the site changes rather than assuming a missing page means the data no longer exists.

### Official repository

`https://github.com/D2R-Reimagined/d2r-reimagined-mod`

Use the repository for exact source data, localization keys, layouts, assets, scripts, and behavior that the public UI does not expose. Record the branch and commit. Inspect `modinfo.json` for the checkout's declared version. Do not assume the default branch, `next`, a release, the live website, and the user's installed copy are identical.

When a local checkout is available, search it directly before downloading another copy. Typical areas include `data/global/excel` for game tables, `data/local/lng/strings` for localized names and descriptions, `data/global/ui/layouts` for panel layouts, and `data/hd` for presentation assets. Confirm actual paths in the selected revision.

## Vanilla D2R

For stable narrative and conceptual background, use the baseline reference in this Skill. For current patch balance, ladder rules, platform behavior, or exact numeric mechanics, verify against current Blizzard patch notes or another authoritative primary source. Distinguish original Diablo II, Lord of Destruction, and Resurrected when their behavior differs.

## Conflict resolution

1. Establish the user's variant and version.
2. Prefer the installed files for local observed behavior.
3. Prefer the matching repository revision for implementation details.
4. Prefer the live game-data site for current player-facing catalogs.
5. Use the wiki for explanations and patch context.
6. Report unresolved conflicts instead of combining incompatible values.
