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
- Fixed runeword and Horadric Cube names showing a stray color character in
  results (`4Spirit`, `>Horadric Cube`). The newer D2R color code (U+E07E plus
  one character) is now stripped like the `ÿc` codes, in stash rows too.
- Automated checks: new platform-independent `character_cache_tests`, plus
  config-key tests in `unit_tests`. Mercenary items are not covered.
