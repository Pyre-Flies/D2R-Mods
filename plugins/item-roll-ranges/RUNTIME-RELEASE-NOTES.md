# Item Roll Ranges release notes

## 1.3.1+rev.45

- Show item level and base quality level while ranges are visible, including
  items without Required Level. Keep Max Sockets in the normal header position.
- Preserve actual Defense/damage and show intrinsic Base bounds. Support fixed
  rolls, compact unique labels, and verified socket/corruption source breakdowns.
- Resolve shared resistances, grouped Attributes, runeword ownership, random
  skill bonuses and replenishment using active vanilla/mod definitions.
- Always-show is optional and defaults to false. `/AlwaysShowRolls` changes the
  session; the saved configuration chooses startup behavior.
- Nine automated suites passed. Live checks confirmed the new header placement
  and Ctrl release, plus the earlier source/range fixes.
