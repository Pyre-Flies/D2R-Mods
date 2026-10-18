# Header and set-transition live batch - 2026-10-17

Command preparation only; no spawning, configuration change or deployment.
Installed Core help confirms spawn <code>[,<option>...] and sets optional
SetItems start/end ranges. Spawn typ=n and typ=r are supported by the recorded
native parser in docs/reverse-engineering/cheat-spawn-socket-limits.md (normal2,
rare6). Ring/amulet codes rin/amu confirmed in local misc.txt. Great Sword gsd
is verified in that parser investigation and its local item data.

Commands: cheat spawn rin,typ=n; cheat spawn amu,typ=n;
cheat spawn rin,typ=r; cheat spawn amu,typ=r; cheat spawn gsd,typ=n.
Check Ctrl and always-show placement, title preservation and no duplicates.
Rare affixes/requirements are random; these commands do not guarantee a rare
without Required Level. Any such item tests the explicitly unhandled title
fallback and should be reported if levels are omitted.

Captured compiled SetItems rows35..40 are Sigon's Gage, Visor, Shelter, Sabot,
Wrap, Guard. cheat sets 35,41 requests those six rows (exclusive end). Check a
single piece and then two, three and all six equipped, removing pieces afterward.
Verify existing permanent numeric rolls stay stable and extra bonus activation
has no duplicated/misassigned labels. Full conditional-set numeric annotation is
not asserted as supported by this check. Item names/rows are mod-snapshot-specific.

Run this batch in Reimagined first. Vanilla parity requires the same candidate
DLL in the normal loader; do not reuse Reimagined compiled affix row numbers as
vanilla IDs. Newly issued commands have static/data verification, not yet a
user-confirmed spawn result for this batch.
