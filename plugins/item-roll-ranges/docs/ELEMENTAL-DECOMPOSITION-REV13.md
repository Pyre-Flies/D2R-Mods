# Exact elemental decomposition (rev.13)

## Evidence

The active `Properties` definition for `dmg-elem` uses function 15 for each
minimum-damage stat and function 16 for each maximum-damage stat. In the affix
row, those functions consume the row minimum and maximum as fixed displayed
damage endpoints. Separate `fire-min` / `fire-max`, `ltng-min` / `ltng-max`,
and `cold-min` / `cold-max` properties use function 1 and retain independent
roll intervals.

For the live-observed T1 `Elemental1` plus `of Flame` example:

- `Elemental1`: fixed fire contribution `21-50`.
- `of Flame`: minimum `1`, maximum roll `2-5`.
- Actual combined fire damage: `22-53`.
- Unique subtraction: `Elemental1` is `21-50`; `of Flame` rolled `1-3`.

## Admission rule

The plugin reads endpoint semantics from the loaded property table. It solves
minimum and maximum independently across every verified source. Exact gray
child values are emitted only if every source has one solution and the sums
equal the native actual endpoints. Unsupported functions, missing endpoint
definitions, ambiguous solutions, malformed bounds, or out-of-range totals
retain the existing unknown (`?`) presentation.

This adds no hooks, RVAs, addresses, offsets, or ABI assumptions. Automated
tests cover fixed `dmg-elem` endpoints, independently rolled direct-element
endpoints, the `21-50 + 1-3 = 22-53` rendering, and fail-open rejection.
Live validation remains pending.
