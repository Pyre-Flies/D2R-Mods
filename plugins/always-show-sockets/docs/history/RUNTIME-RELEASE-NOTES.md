# Always Show Sockets release notes

## 1.3.1+rev.2 - 2026-10-15

- Save default/always/off and bare/on/toggle visibility commands, as well as
  existing appearance settings. Restore saved visibility at startup and retain
  the last styled mode across off/on toggles. New installs still use default.
- Store mode in `enabled` and `always_visible` under `[always-show-sockets]`.
  Status stays read-only; save failures remain clearly reported.
- Four suites pass, including retained native chat dispatch, real startup
  settings reloads and save-failure checks. Live save/restart confirmation remains
  pending. Native hooks, resources and rendering guards are unchanged.

## 1.3.1+rev.1

- Promote configurable socket visibility and appearance to the stable format.
- Offer default hover/selection, continuous display and native appearance modes.
- Save size, opacity, ring, center and lift settings automatically. Start each
  session in default visibility mode.
- Four automated suites passed. Display modes and appearance were confirmed
  in game; actual configuration save/restart still needs live confirmation.
