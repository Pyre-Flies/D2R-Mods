# Ultrawide Changelog

## 1.3.5+rev.2 - 2026-10-23

- Fix cross-plugin coexistence conflict with `gem-bag-ui` (`CheckExpectedBytes` at `0x8562F0` `Widget::GetScreenPos`) by removing all hooks from `0x8562F0..0x856476` and instead updating `m_anchor.x` (`[panel + 0x48]`) on `AutoMapPanel`, `HireablesPanel`, and `HUDWarningsPanel` at `0x8A6461` inside `UIRootWidget::Update` (`0x8A5AA0`).

## 1.3.5+rev.1 - 2026-10-23

- Fix a crash on game join (`0xC0000005` in `AdjustWidgetScreenPosImpl`) caused by `Widget::GetScreenPos` (`0x8562F0`) writing its 16-byte local `xmmword` Rect (`0x856357`: `movups [rsp+0x20], xmm0`) over `[rsp+0x28]` by saving the original `Widget*` (`rcx`) in the unused stack slot `[rsp+0x10]` instead.

## 1.3.5+rev.0 - 2026-10-23

- Add configurable **Centered HUD** vs. **Full Length HUD** mode (`enable_full_length_hud = false` / `true`, or `hud_mode = "centered"` / `"full"`) via `0x8A6635` inside `UIRootWidget::UpdateLayout` (`0x8A64F0`), keeping front-end menus clamped to 19:9 while optionally expanding `UIRootWidget` across the full ultrawide viewport during active gameplay.
- Add independent top-corner HUD anchoring toggles (`enable_corner_minimap = true`, `enable_corner_mercenary = true`, `enable_corner_game_info = true`) via `Widget::GetScreenPos` (`0x8562F0` / `0x856455`), pushing the Corner Minimap (`AutoMapPanel` vtable `0x1FDC8A8`), Hired Mercenary / Party portrait area (`HireablesPanel` vtable `0x1CDDA38`), and top-right Game Information (`HUDWarningsPanel` vtable `0x1FCFBE8`) to the true ultrawide screen corners even when Centered HUD is active.

## 1.3.4+rev.0 - 2026-10-23

- Add full-screen Automap overlay unclamping (`enable_automap_unclamp = true` at `0x14E367C` inside `AutoMapPanel::Draw` `0x14E3480`), expanding both the GPU scissor rectangle (`0x79D920`) and the isometric world-tile/icon culling bounds (`0xD1940`) across the full ultrawide viewport.

## 1.3.3+rev.0 - 2026-10-23

- Fix newly spawned or awakened monsters in 2-hop/3-hop rooms remaining invisible until a Town Portal round-trip by calling `ServerPopulateRoom1` (`0x4042A0`) before `ClientAddRoom` (`0x489DB0`) and hooking `ServerReplicateRemovedUnitsInRooms` (`0x48A149`) and `ServerReplicateDirtyUnitsInRooms` (`0x48A2F4`) to iterate the client's full active room neighborhood before `0x488410` clears dirty unit flags.
- Fix Save & Exit crash caused by double-decrementing `[pRoom1 + 0x70]` in `HookDisconnectCleanup` (`0x48A200`) on 1-hop rooms.

## 1.3.1+rev.1 - 2026-10-23

- Promote and unify the experimental `ultrawide-bars` (`0.1.0-beta.6`) and `terrain-coverage-probe` (`0.1.0-beta.28`) plugins into the standalone `Ultrawide` plugin (`Ultrawide.dll`, id `ultrawide`, version `1.3.1+rev.1`).
- Remove all diagnostic overlays (cyan/gold/white/red contour lines, startup telemetry, and visual shader test modes) and obsolete culling bypass experiments (`disable_coarse_culling`, `disable_decal_culling`).
- Add gameplay-scoped horizontal mouse coordinate unclamping (`enable_mouse_unclamp = true` at `0x7F427`) so cursor world-picking, movement, and targeting work across the full 21:9 / 32:9 viewport while keeping menu clamping intact.
- Add cross-mod and Vanilla shader path discovery for the 14 paired 22x22 terrain shaders (`data/hd/tc22sh/`), supporting both mod MPQ folders and direct OS paths (`d2rloader/data/hd/tc22sh/`) via `0xA00C60`'s native absolute-path routing.
- Add a 2-hop (5x5) local-server active room neighborhood expansion (`enable_monster_pop_in_fix = true` at `0x487254` and `0x48A200`), holding stage-0 DRLG room references on 1-hop neighbors so 2-hop rooms and monsters remain materialized and replicated to the client across ultrawide edges.
- Add configurable hostile monster AI target-perception scaling (`enable_monster_ai_range = true`, `monster_ai_range = 50` clamped to `[35, 100]`, `scale_friendly_ai_range = false`) at `0x595923`, `0x5959AF` (`max(monster_ai_range, aidist)`), `0x595B19` (player-slot distance ceiling), and `0x595D61`.
