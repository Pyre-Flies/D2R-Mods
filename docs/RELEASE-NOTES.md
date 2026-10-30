# D2R Mods - 2026.10.30.1

- **Pyre Loot Filter 1.3.1+rev.3:** Fixes an issue where rules set to `Hide` that still contained saved `Show`-mode action settings (such as when toggling an existing `Show` rule to `Hide` in the web filter builder, or when a `continue: true` rule chained into a later `Hide` rule) could still trigger minimap icons or custom drop sounds. Hidden items now suppress both custom and vanilla drop audio and are excluded from minimap icon tracking.

Close the game before replacing `pyre-lootfilter.dll` (and remove legacy `unhoarder.dll` if present). Existing settings and `filter.json` files are preserved.

Automated build, unit, artifact, and packaging checks pass, and the fix was validated live in D2R Reimagined.
