# Inventory Deposit

**1.3.1+rev.13 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Adds a **Deposit All Gems, Materials and Runes** button beside inventory gold
while the HD advanced stash is open in keyboard/mouse mode. No chat commands.

## Installation and updates

1. Close the game and back up the previous plugin DLL and active configuration.
   The D2RR launcher can remain open.
2. Extract the ZIP into the game directory for global use, or the active mod
   directory for mod-specific use. The DLL belongs in `d2rloader/plugins/`.
   Keep one active copy of each plugin. Managed mod packs should use their
   normal updater.
3. Start through D2RLoader. Missing main configs are created automatically in
   the active `d2rloader/config/` directory.

Files under `defaults/` are references. Extracting an update preserves existing
settings; editing a reference does not change gameplay. Edit the active file
with the game closed, change existing keys instead of duplicating them, and
restart to apply manual edits. TOML booleans are lowercase `true`/`false`,
strings use quotes, and `#` starts a comment outside a quoted string.

To uninstall or roll back, close the game, remove the DLL or restore its backup,
then restart. Hot reload is unsupported.

## Use the button

Open the stash and click Deposit All. Eligible gems, runes, materials and
rejuvenation potions in main inventory move into advanced storage. Equipment,
belt, Cube and Charm Inventory contents are excluded. Native eligibility rules
apply. The button is disabled while holding an item, while busy, or when nothing
qualifies. It highlights on hover when available.

Batches can stop partway through after a failed or unconfirmed move. Check the
inventory and destination before repeating. Avoid starting Controller QOL's
bulk deposit at the same time. No separate Controller QOL installation or loose
artwork files are needed.

## All configuration settings

Edit the active `d2rloader/config/inventory-deposit.toml`, with the game closed,
then restart. All keys belong under `[inventory_deposit]`:

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Show/enable the optional button; false disables it. |
| `anchor_to_gold` | `true` | Follow the inventory gold display. False uses manual button_x/button_y. |
| `gold_gap` | `85` | Gap after the gold amount, integer 0-4096 inventory UI units. |
| `gold_offset_y` | `1` | Vertical offset from gold-centered placement, integer -4096 to 4096 UI units. |
| `button_x` | `860` | Manual panel-local horizontal coordinate, integer 0-4096; used only when anchoring is false. |
| `button_y` | `1620` | Manual panel-local vertical coordinate, integer 0-4096; used only when anchoring is false. |

Coordinates use inventory-panel UI units that scale with the panel, rather
than fixed screen pixels. Manual coordinates are retained while automatic
anchoring is enabled. A malformed recognized value or duplicate setting
prevents the button from activating; correct the file instead of adding keys.

## Editing examples

Default gold-relative placement:

```toml
[inventory_deposit]
enabled = true
anchor_to_gold = true
gold_gap = 85
gold_offset_y = 1
button_x = 860
button_y = 1620
```

Example manual placement, slightly left of the shipped manual position:

```toml
[inventory_deposit]
enabled = true
anchor_to_gold = false
button_x = 840
button_y = 1620
```

Merge into the existing section, save and restart. If another button overlaps,
adjust `gold_gap`/`gold_offset_y` or use manual placement. Set `enabled = false`
to disable the button. The [reference TOML](inventory-deposit.toml) lists defaults.

## Compatibility and validation

Requires the supported native profile in [COMPATIBILITY.txt](COMPATIBILITY.txt)
and Reimagined-style advanced storage with the HD keyboard/mouse inventory.
Controller/classic layouts are outside the button's scope. Unsupported builds
leave it unavailable.

Twelve automated suites passed. Live checks cover deposits, placement,
hover/disabled states and tab feedback. Remote multiplayer, multiple resolutions
and save/reload persistence are not fully validated.

## Support

Report problems at [GitHub Issues](https://github.com/Pyre-Flies/D2R-Mods/issues)
with the plugin/game/mod versions and steps to reproduce. Check logs for
personal information before sharing. Download checksums are alongside the ZIPs.
