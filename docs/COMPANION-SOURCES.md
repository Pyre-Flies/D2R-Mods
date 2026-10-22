# Companion plugins

The eight production plugins in `plugins/` are maintained in this repository.
UnHoarder and Buff HUD retain their own source repositories and release history.
`release-companions.json` records the exact companion package and payload hashes.

| Companion | Current release | Source |
| --- | --- | --- |
| UnHoarder | [1.2.16](https://github.com/Pyre-Flies/d2rl-unhoarder/releases/tag/v1.2.16) | [Fork](https://github.com/Pyre-Flies/d2rl-unhoarder) |
| Buff HUD | [1.1.1](https://github.com/Pyre-Flies/d2rl-buff-hud/releases/tag/v1.1.1) | [Fork](https://github.com/Pyre-Flies/d2rl-buff-hud) |

Buff HUD includes the controller/keyboard switching fix from
[upstream PR #8](https://github.com/Lukaszpg/d2rl-buff-hud/pull/8). The PR remains
independent of the fork release; upstream has not merged it as of October 20.
Install its DLL and companion MPQ together in one plugin scope.

Keep existing settings, filters, and active-mod overrides. Verify release assets
against the published checksums. Build and package checks establish artifact
identity, rather than new live-game acceptance.
