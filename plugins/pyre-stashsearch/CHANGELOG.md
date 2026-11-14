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
  is unreadable). Live in-game verification is still pending; see
  [the reverse-engineering notes](../../docs/reverse-engineering/stash-search-1.10.12-decompilation.md#cross-character-search-additions).
- Automated checks: new platform-independent `character_cache_tests`, plus
  config-key tests in `unit_tests`. Mercenary items are not covered.
