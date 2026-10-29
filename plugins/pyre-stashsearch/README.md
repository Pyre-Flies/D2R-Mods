# Pyre Stash Search (`pyre-stashsearch`)

Instant cross-page stash search, boolean stat filtering, active-page item highlighting, and an expandable Side Tab Flyout with custom tab names for `D2RLoader`.

## Installation

Copy `pyre-stashsearch.dll` into your `d2rloader/plugins/` directory (for example, `mods/Reimagined/d2rloader/plugins/pyre-stashsearch.dll` or a global `d2rloader/plugins/pyre-stashsearch.dll`).

No UI layout (`bankexpansionlayouthd.json`) edits or extra `.mpq` files are required—the search bar anchors automatically to the stash tab strip.

## Features

- **Instant Cross-Page Stash Search**
  - Searches your **Personal Stash** and all **Shared Stash** pages simultaneously as you type.
  - Matches item names, base types, and full localized stat/affix text (including socketed runes, gems, and jewels).
  - Click any result in the dropdown (or use `Up` / `Down` / `Mouse Wheel` and `Enter`) to jump directly to that stash tab and center the tab strip on the target page.
- **Active-Page Item Highlighting**
  - Items on the currently open stash page that match your search query are highlighted with a gold border and tint directly on the stash grid so you can spot them immediately.
- **Boolean Operators & Search Shorthands**
  - **AND (`&` or space):** `monarch 4os`, `ring & fcr & allres`
  - **OR (`|`):** `shako | griffon`
  - **NOT (`-` or `!`):** `monarch 4os -eth`, `ring & fcr & !cold` (hyphenated item names like `Bul-Kathos` and negative stat rolls like `-15%` are matched literally).
  - **Quality & Flag Shorthands:** `unique` (`uni`), `set`, `rare`, `magic` (`mag`), `crafted` (`craft`), `runeword` (`rw`), `ethereal` (`eth`).
  - **Socket Shorthands:** `0os`–`6os`, `os`, `sockets`.
  - **Stat Shorthands:** `fcr`, `ias`, `fhr`, `frw`, `mf`, `gf`, `ll`, `ml`, `cb`, `ds`, `ow`, `allres`, `fres`, `cres`, `lres`, `pres`, `dr`, `mdr`, `pdr`, `cbf`, `tele`, `bo`, `ed`, `ar`.
- **Expandable Side Tab Flyout & Custom Tab Names**
  - Click the `[<]` / `[>]` toggle on the left edge of the stash panel to open a vertical flyout listing `Personal` and all Shared Stash pages (`#1`..`#N`).
  - **Right-click any tab** in the flyout to assign a custom client-side name (e.g., `Runes`, `Bases`, `Uniques`). Press `Enter` to save or `Esc` to cancel; saving an empty name restores the default label.
  - Custom tab names expand the flyout automatically, appear in search dropdown results (`Shared #2 (Runes)`), and persist in `d2rloader/config/plugins.toml`.
- **Quick Page Navigation**
  - **Slash Jump:** Type `/<page>` (for example, `/14`) and press `Enter` to jump straight to that shared stash page.
  - **Wheel Shortcuts:** Hold `Ctrl + Mouse Wheel` anywhere over the stash to cycle pages `1` at a time, or `Ctrl + Shift + Mouse Wheel` to jump `5` pages at a time.

## Configuration

Settings and custom tab names are stored automatically in `d2rloader/config/plugins.toml` under `[pyre-stashsearch]`:

```toml
[pyre-stashsearch]
tab_flyout_expanded = false
# tab_name_personal = "Main"
# tab_name_shared_1 = "Runes"
# tab_name_shared_2 = "Bases"
```

## Credits

Inspired by `yinyin333333` (Yin), who created the original Stash Search concept for Diablo II: Resurrected.
