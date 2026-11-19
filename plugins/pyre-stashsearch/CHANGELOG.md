# Changelog

## Unreleased

- Search every character: each character's stash, inventory, Horadric Cube,
  equipped gear, belt and D2RLoader extra pages are saved to a per-mod text
  snapshot (`config/pyre-stashsearch-characters/<mod>/<name>.txt`) when it
  enters a game, changes area or has its stash open. Searches list matches from
  all other snapshotted characters as display-only rows such as
  `Shako - MuleSorc (Cube)`, with the snapshot age in the hover text.
- The current character's inventory, cube, equipped and belt items are now
  searched too (`- Inv`, `- Cube`, `- Worn`, `- Belt`). Stash rows stay first
  and keep click-to-jump; the new rows are informational.
- New settings `search_carried_items` and `search_other_characters` (both on
  by default). Saving tab names keeps these lines unchanged.
- New native reads: item mode at `unit + 0x0C` and the character name at
  `PlayerData + 0x00`. Both fail closed (no snapshot is written when the name
  is unreadable). Both were verified live on vanilla D2R 3.3.94995; see
  [the reverse-engineering notes](../../docs/reverse-engineering/stash-search-1.10.12-decompilation.md#cross-character-search-additions).
- Current-character rows get the same multi-line item hover tooltip as stash
  rows; other characters' rows show a short hover laid out like the live one
  (item name, then base type such as `Monarch` when the item shows one, a
  divider, then character and location and the snapshot age), so the age is
  never cut off by the tooltip's line-width limit. Only stash items count
  for the duplicate search term, so carried and snapshot rows never match it.
- The U+E07E color-code stripping (also fixed in `1.3.1+rev.7`) now
  also drops a code cut off at the end of a line instead of leaving its bytes
  in the result.
- Automated checks: new platform-independent `character_cache_tests`, plus
  config-key tests in `unit_tests`. Mercenary items are not covered.
