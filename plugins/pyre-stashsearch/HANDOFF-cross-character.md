# Handoff: cross-character search (branch `Pyre/stashsearch-all-characters`)

Temporary working note for continuing this feature on Windows. Delete it before
opening a pull request against `Pyre-Flies/D2R-Mods`.

## What the branch adds

Stash Search previously matched only the logged-in character's personal stash and
the shared stash pages. This branch adds two result groups after those:

- **Carried items of the current character** (`search_carried_items = true`):
  inventory, Horadric Cube, equipped gear, belt, cursor and D2RLoader extra pages.
  Rows end in ` - Inv`, ` - Cube`, ` - Worn`, ` - Belt`, ` - Cursor` or ` - Page`.
- **Other characters** (`search_other_characters = true`): every item of every
  other character, read from per-character snapshot files. Rows look like
  `Shako - MuleSorc (Cube)`; hover shows the full location and snapshot age.

Both kinds of rows are display-only (`MatchEntry::displayOnly`): clicking them
does not jump, because only stash rows have a page target. Duplicates merge into
one row with a `x N` count.

### Snapshots

- Written to `<config dir>\pyre-stashsearch-characters\<active mod or _base>\<Name>.txt`
  (next to `pyre-stashsearch.toml`), so vanilla and each mod stay separate.
- Captured on `LocalPlayerReady` (forced), on `LevelChanged`, and at the end of
  each stash-panel draw, throttled to once per 30 s and skipped when unchanged.
- Written atomically (`.tmp` + `MoveFileExW`). Other characters' files are
  rescanned at most every 5 s and only reparsed when size/mtime changed.
- Text format, version 1, one record per line, tab-separated, `\\ \t \r \n` escaped:

  ```text
  # pyre-stashsearch character snapshot
  version	1
  name	MuleSorc
  saved	1763000000
  item	cube	<short name>	<full name>	<normalized tooltip search text>
  ```

  Source codes: `stash inv cube worn belt cursor page`. The parser skips
  malformed item rows and unknown keys and rejects other versions or bad names.

## Code map

| Area | Location |
| --- | --- |
| Pure snapshot logic (format, classification, names, UTF-8, age) | `src/character_cache.h`, `src/character_cache.cpp` |
| Native item mode read (`unit+0x0C`) | `ReadItemMode`, `src/plugin.cpp` |
| Walk + classify every item of an inventory | `ForEachClassifiedInventoryItem`, `src/plugin.cpp` |
| Character name (`unit+0x10` -> PlayerData `+0x00`, `char[16]`) | `ReadPlayerCharacterName`, `src/plugin.cpp` |
| Cache directory | `ComputeCharacterCacheDir`, `src/plugin.cpp` |
| Capture / load / search | `CaptureLocalCharacterSnapshot`, `RefreshLoadedCharacterSnapshotsLocked`, `ScanCarriedItems`, `ScanOtherCharacterSnapshots` |
| Lifecycle hooks | `OnGameplayEventForSnapshots`, `RegisterSnapshotLifecycleListeners` |
| Config keys | `StashSearchConfig` in `src/stash_search.h`; parsing in `src/native.cpp` |
| Native evidence and status | "Cross-character search additions" in `docs/reverse-engineering/stash-search-1.10.12-decompilation.md` |

## Verification status

| Check | Status |
| --- | --- |
| `pyre-stashsearch-character-cache-tests` | Passed on Linux with g++ (`-Wall -Wextra -Wpedantic -Werror -Wconversion`) |
| All sources compile for Windows | Passed with zig/clang `x86_64-windows-gnu`, no warnings in plugin code |
| MSVC build (`/W4 /WX /EHa`) and `ctest` | **Not run** |
| `pyre-stashsearch-unit-tests` (includes new config-key test) | **Not run** (MSVC only) |
| In-game behavior | **Not run** |

## Next steps on Windows

1. Build and test from a Visual Studio x64 Developer Prompt at the repo root
   (procedure from `CONTRIBUTING.md`):

   ```powershell
   git submodule update --init --recursive
   $work = python tools/local_workspace.py
   $build = Join-Path $work 'scratch/pyre-stashsearch/build'
   cmake -S plugins/pyre-stashsearch -B $build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
   cmake --build $build
   ctest --test-dir $build --output-on-failure
   ```

   Fix any MSVC-only warnings (the zig build cannot catch every `/W4` C4xxx warning).
2. Install the built `pyre-stashsearch.dll` into one plugin scope only, e.g.
   `<game>\d2rloader\plugins\` (vanilla) or `<game>\mods\<Mod>\d2rloader\plugins\`.
   Back up saves first.
3. Live checks, in order:
   1. **Character name offset (highest risk).** Log in with character A, open the
      stash, and confirm `A.txt` appears in the snapshot folder with the right name.
      No file means `ReadPlayerCharacterName` failed (it fails closed; no snapshot
      is written). Wrong text means PlayerData `+0x00` is wrong for this build.
   2. **Item classification.** Put known items in inventory, cube, belt, and wear
      gear; open `A.txt` and confirm each `item` row has the right source code.
      Socketed runes/jewels must not appear as separate rows.
   3. **Carried search.** Search for an inventory/cube item on the same character;
      expect ` - Inv` / ` - Cube` rows below stash results, not clickable.
   4. **Other characters.** Log in with character B and search for something only
      A has; expect `Item - A (Location)` and a correct snapshot age on hover.
   5. **Refresh.** Move an item on B, change area, wait 30 s, and confirm `B.txt`
      updates. Check the D2RLoader console for `PyreStashSearch:` warnings.
   6. **Config off.** Set both new keys to `false` and confirm behavior matches
      the released plugin.
4. Record the live results in the reverse-engineering doc's status column and,
   if offsets changed, in the same commit. Then update `CHANGELOG.md`, remove this
   note, and open the PR.

## Known limitations

- Mercenary items are not searched (no proven path to the mercenary unit here).
- Other characters' snapshots are only as fresh as their last captured session;
  quitting right after moving items can miss the move until next login.
- Snapshots of deleted characters remain until their file is deleted.
- The version is still `1.3.1+rev.1`; bump it as part of the release process
  (`docs/RELEASING.md`).
