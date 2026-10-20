# Floating Damage release notes

## 1.3.1+rev.13 - 2026-10-15

- Save `/damagenumbers` on/off/toggle and motion changes automatically to the
  active TOML. Restore them on startup; status remains read-only.
- Preserve unrelated settings/comments and report save failures without changing
  the previous runtime setting. Diagnostic capture remains a bounded action.
- Eight automated suites pass, including actual temporary TOML files with fresh
  startup parsing and production command/error checks. Live save/restart
  confirmation is still pending. Native hooks, drawing and guards are unchanged.

## 1.3.1+rev.12

- Promote the HP-loss overlay to the stable version format.
- Scale text and movement with resolution. Choose critical-hit emphasis with
  a suffix, larger text, or both; preserve element colors and actual HP loss.
- Seven automated suites passed. Prior live checks cover numbers, elements,
  motion and critical-marker controls. Latest 4K scaling and style-mode visuals
  still need in-game confirmation. Local single player only.
