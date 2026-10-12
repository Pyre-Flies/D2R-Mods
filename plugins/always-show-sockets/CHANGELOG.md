# Changelog

## 0.1.0-beta.1 — 2026-10-12

- First standalone prerelease of the live-tested socket renderer and local
  `/sockets` chat-toggle candidate. No rendering changes from probe.2.
- Live-confirmed: empty inventory sockets, gem/jewel artwork on equipped armor,
  Cube, personal stash and shared stash. Chat toggle remains live-unconfirmed.
- Package only this plugin, installation/rollback instructions and checksums.

## 0.1.0-probe.2 — 2026-10-12

- Add native local chat `/sockets` to toggle, plus `on`, `off`, `toggle` and
  `status`. Bare debug-console `sockets` now toggles as well.
- Reuse Floating Damage's guarded native parser backend, including UI-thread
  registration retries and retained-callback shutdown. No external DLL needed.
- Preserve socket rendering hooks and session default enabled. Status output
  uses the existing debug console; state changes are also logged.
- MSVC Release build and both artifact/command adapter checks passed (2/2).
  Socket-specific native chat registration and live slash dispatch remain untested.

## 0.1.0-probe.1 — 2026-10-12

- Experimental native socket visibility after inventory item icon drawing.
- Reuse native empty socket and inserted-item art without changing selection.
- Guard exact loader/Core SHA-256 and both renderer entry byte windows; preserve
  native behavior if admission or either hook fails.
- Debug-console `sockets on/off/status` for live comparison.
- MSVC Release build and artifact admission/refusal test passed (1/1). Live rendering,
  controller, stash, vendors and scaling remain untested.
- User screenshot confirms automatically visible filled sockets on equipped
  armor without a tooltip. A second user-confirmed screenshot shows three empty
  socket circles on an unhovered inventory sword.
- User confirms the equipped armor displays both a gem and a jewel, and socket
  visuals also work in the Cube, personal stash and shared stash. Mixed inserted
  item types are confirmed; empty and filled sockets together on one item remain
  unverified. Controller, vendors, trade, classic mode and scaling remain untested.
