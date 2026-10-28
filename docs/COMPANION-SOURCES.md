# Companion plugins

The ten production plugins in `plugins/` (including **Pyre Loot Filter**) are maintained in this repository.
Buff HUD retains its own source repository and release history.
`release-companions.json` records the exact companion package and payload hashes.

| Companion | Current release | Source |
| --- | --- | --- |
| Buff HUD | [1.1.2](https://github.com/Pyre-Flies/d2rl-buff-hud/releases/tag/v1.1.2) | [Fork](https://github.com/Pyre-Flies/d2rl-buff-hud) |

Buff HUD 1.1.2 includes automatic runtime Excel buff catalog discovery (`states.txt`, `skills.txt`, and `itemstatcost.txt`), 25 Hz frame-time gating (`-97%` CPU overhead, submitted in [upstream PR #9](https://github.com/Lukaszpg/d2rl-buff-hud/pull/9)), and the controller/keyboard switching fix from [upstream PR #8](https://github.com/Lukaszpg/d2rl-buff-hud/pull/8). Install its DLL and companion MPQ together in one plugin scope.

Keep existing settings, filters, and active-mod overrides. Verify release assets
against the published checksums. Build and package checks establish artifact
identity, rather than new live-game acceptance.
