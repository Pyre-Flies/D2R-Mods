# D2R Mods - 2026.10.24.1

- **Ultrawide 1.3.5+rev.2:** Full 21:9 and 32:9 gameplay support. Removes side
  pillarboxes and widens the vignette mask during gameplay while keeping menus
  framed cleanly, unclamps horizontal mouse/placard/automap bounds, supports
  centered or full-length HUD and top-corner HUD anchoring, expands the terrain
  cache to 22×22, expands the active monster room window to 7×7 to prevent edge
  pop-in, and scales monster AI perception range.
- **Controller QOL rev.93:** Prevents `LB + A` item identification from
  triggering while holding an item on the cursor (so the `A` button never gets
  stuck after picking up an item), increases the default `LB` ground-loot
  assignment range to `14`, and adds compatibility with `UnHoarder 1.2.17`
  alongside `1.2.16`.
- **Inventory Deposit rev.13:** Fixes compatibility with `UnHoarder` (`1.2.16`
  and `1.2.17`) so the **Deposit All** button stays available regardless of
  plugin load order.
- **Gem Bag UI rev.2:** Swaps controller shortcuts so `LB + A` opens the gem
  picker and `LB + B` deposits compatible gems and gem clusters, with updated
  tooltip prompts.
- **Item Roll Ranges rev.55:** Fixes max-socket display on Set and Unique items
  with innate socket limits, preserves charged-skill tooltip formatting when
  showing roll ranges, and prevents property text truncation on heavily modified
  items.
- The other plugins and companion packages retain their current released behavior.

Close the game before replacing DLLs and keep one active copy of each plugin.
When installing **Ultrawide**, extract its `d2rloader/data/hd/tc22sh/` shader
files alongside `d2rloader/plugins/`. Existing settings and filters are
preserved; packaged defaults are references. Buff HUD needs both its DLL and
MPQ. The UnHoarder Reimagined ZIP (`1.2.16`) includes its sound tables; retain
your existing `filter.json`.

Automated build, unit, and packaging checks pass across all nine plugins.
Ultrawide gameplay, Gem Bag UI controller shortcuts, Item Roll Ranges tooltip
formatting, and UnHoarder coexistence were validated against the current game
and mod builds. See individual plugin guides for multiplayer and host/client
notes.
