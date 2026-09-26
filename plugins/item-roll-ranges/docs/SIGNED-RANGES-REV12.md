# Signed native ranges (rev.12)

## Problem

Core places a sign or separator immediately before the colored native range
span. The previous parser treated every leading dash before the first captured
range as unary. That misread both of these native shapes:

- `Adds 1-(6-8) Lightning Damage`: the dash separates a fixed minimum from a
  ranged maximum and the captured interval is positive.
- `-(11-20)% Target Defense`: the dash is unary and negates the entire native
  interval.

## Rule

The parser now tracks whether the preceding meaningful token was numeric. A
dash before a colored range is a separator when it immediately follows a
number or another range; otherwise it is a unary negative sign. A simple
unary-negative pair is reversed
and negated so its displayed bounds remain numerically ascending:

`(11-20)` becomes `[-20 - -11]`.

Unrecognized or localized complex spans retain the conservative parenthesized
fallback. This change adds no hooks, RVAs, offsets, table layouts, or ABI
assumptions.

## Validation

Automated regressions cover fixed-number plus ranged lightning damage, unary
negative Target Defense, and the final merged tooltip line. Live-game checks
remain required for `of Shock`, `of Puncturing`, reduced requirements, and
enemy-resistance reductions present in the active tables.
