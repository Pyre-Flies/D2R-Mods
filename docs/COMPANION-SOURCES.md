# Companion plugins

The ten production plugins in `plugins/` (including **Pyre Loot Filter**) are maintained in this repository.
Buff HUD retains its own source repository and release history.
`release-companions.json` records the exact companion package and payload hashes.

| Companion | Current release | Source |
| --- | --- | --- |
| Buff HUD | [1.3.1+rev.3](https://github.com/Pyre-Flies/d2rl-buff-hud/releases/tag/v1.3.1+rev.3) | [Fork](https://github.com/Pyre-Flies/d2rl-buff-hud) |

Buff HUD `1.3.1+rev.3` (`d2rl-buffhud.dll` and `d2rl-buffhud.mpq`) includes joined-game and dedicated-server (`d2rds`) multiplayer buff timer synchronization, Blood Golem (`blood_mana`) aura opt-out toggle (`AuraCheck` checkbox and `buff-blood-aura` console command), hover tooltips and quick-cast bar clearance (`BuffGrid.fields.rect.y = -665`), automatic runtime Excel buff catalog discovery (`states.txt`, `skills.txt`, and `itemstatcost.txt`), 25 Hz frame-time gating (`-97%` CPU overhead, submitted in [upstream PR #9](https://github.com/Lukaszpg/d2rl-buff-hud/pull/9)), and the controller/keyboard switching fix from [upstream PR #8](https://github.com/Lukaszpg/d2rl-buff-hud/pull/8). Install its DLL and companion MPQ together in one plugin scope.

Keep existing settings, filters, and active-mod overrides. Verify release assets
against the published checksums. Build and package checks establish artifact
identity, rather than new live-game acceptance.
