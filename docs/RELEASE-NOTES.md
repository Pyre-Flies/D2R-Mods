# D2R Mods - 2026.11.08.1

- **Ultrawide 1.3.5+rev.3:** Fixes a crash in multi-floor dungeons (such as Forsaken Labyrinth Floor 21) where the 3-hop monster pop-in expansion could reach through staircase/doorway links and force-load rooms on connected floors before you actually entered them. Room pre-loading now stays strictly on your current floor while keeping the full `monster_pop_in_radius = 3` (`7x7` room window) on the active map.

Close the game before replacing `Ultrawide.dll`. Existing `ultrawide.toml` settings and `data/hd/tc22sh/` terrain shaders are preserved.

Automated build, unit, artifact, and packaging checks pass; still needs a live check in Forsaken Labyrinth.
