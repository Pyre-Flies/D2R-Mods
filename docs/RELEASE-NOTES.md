# D2R Mods - 2026.10.25.1

- **Controller QOL rev.94:** Always prioritizes quest interactables (Cain's
  Gibbet, Inifuss Tree, Wirt's Body, Horadric Malus, Tainted Sun Altar,
  Horadric/Khalim quest chests, Compelling Orb, Hellforge, Diablo Seals,
  Ancients' Altar, Frozen Anya, and pre-activation Cairn Stones, Horadric Staff
  orifice, and Arcane Tome) and act-transition portals over combat skills bound
  on `A` and nearby ground loot. Includes the `rev.93` empty-cursor `LB + A`
  identify guard, default `LB` loot range `14`, and compatibility with
  `UnHoarder 1.2.16`–`1.2.18`.
- **Ultrawide 1.3.5+rev.2:** Full 21:9 and 32:9 gameplay support. Removes side
  pillarboxes and widens the vignette mask during gameplay while keeping menus
  framed cleanly, unclamps horizontal mouse/placard/automap bounds, supports
  centered or full-length HUD and top-corner HUD anchoring, expands the terrain
  cache to 22×22, expands the active monster room window to 7×7 to prevent edge
  pop-in, and scales monster AI perception range.
- **Inventory Deposit rev.13:** Fixes compatibility with `UnHoarder`
  (`1.2.16`–`1.2.18`) so the **Deposit All** button stays available regardless
  of plugin load order.
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
MPQ. The UnHoarder Reimagined ZIP (`1.2.18`) includes its sound tables; retain
your existing `filter.json`.

Automated build, unit, and packaging checks pass across all nine plugins.
Controller QOL quest-object priority, Ultrawide gameplay, Gem Bag UI controller
shortcuts, Item Roll Ranges tooltip formatting, and UnHoarder `1.2.18`
coexistence were validated against the current game and mod builds. See
individual plugin guides for multiplayer and host/client notes.
