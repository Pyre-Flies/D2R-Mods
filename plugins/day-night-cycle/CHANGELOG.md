# Changelog

## 1.3.1+rev.1 - 2026-10-21

- Restore natural day/night lighting without enabling debug mode.
- Choose natural, fixed day or fixed night lighting with persistent chat controls.

Promotes beta.3 behavior. Natural cycling is user-confirmed; fixed modes and hosted sessions still need live testing.

## 0.1.0-beta.3 - 2026-10-21

- Add immediate `day` and `night` lighting controls; use `auto` or `on` to resume
  the natural cycle. The selected mode survives restarts.
- Make `off` return to daylight instead of freezing the previous lighting.
- Add actual renderer values to status/log output and preserve another owner's
  manual override. No additional hook or server component is needed.
- Natural cycling on beta.2 is user-confirmed. New controls have automated
  coverage and still require in-game validation.

## 0.1.0-beta.2 - 2026-10-21

- Add `/daynight` with on, off, toggle, status and help in normal game chat.
- Save the setting across restarts while preserving other TOML text; restore the
  previous state and report an error if saving fails. Start enabled by default.
- Add command dispatch, persistence, rollback and shutdown checks. In-game
  validation is pending.

## 0.1.0-beta.1 - 2026-10-21

- Restore native time-of-day lighting updates outside debug mode on the reviewed
  D2RLoader build, retaining the native timing and time override.
- Refuse unsupported binaries, changed code and unexpected native bindings.
- Add executable native-path, ownership/compatibility, DLL artifact and profile
  consistency checks. In-game validation is pending.
