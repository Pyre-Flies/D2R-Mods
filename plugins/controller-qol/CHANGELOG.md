# Changelog

All notable changes to Controller QOL Updates are recorded here. Versions before
the GitHub migration are reconstructed from release and validation records.

## [1.3.1+rev.10] - 2026-09-26

- Hide controller item shortcuts while the game is in keyboard/mouse mode.
- Reuse the existing native input-mode query; no additional hooks.
- Fourteen automated suites pass. User accepted the update before packaging;
  a separate detailed input-switching test matrix was not recorded.

## [1.3.1+rev.9] - 2026-09-26

- LB+X transfers to/from the visible registered custom inventory page using
  D2RCore's existing quick-transfer operation and item policy.
- Validate active grid, player and focused item identity; refused moves cannot
  fall through to the Personal Stash. No additional hook or Ctrl spoof.
- User confirmed Charm Inventory transfers work.

## [1.3.1+rev.8] - 2026-09-26

- Route controller menu remapping before D2RCore's custom-page route consumes
  shoulder inputs, fixing coexistence with Charm Inventory.
- Guard the exact dispatcher, forwarding links and vtable slot.
- User confirmed menu switching works with Charm Inventory installed.

## Repository migration

### Changed

- Moved the source into the D2R Mods monorepo and switched to the shared,
  pinned Plugin SDK submodule.

## [1.3.1+rev.7] - 2026-09-26

### Added

- Native portal-priority behavior and contact-gate integration.
- Build-specific guarded native input, ground-label, glyph, storage, belt,
  materials, shared-page, and vendor integrations documented under `docs/`.

### Validated

- Twelve automated suites passed.
- Chronicle Ground Flag coexistence and portal behavior were confirmed in game.
- Earlier native-controller validation covered Battle.net DualShock and Steam
  controller paths with reviewed plugin coexistence.

### Known limitations

- Native profiles are specific to the tested game/module builds.
- Hot reload is not a supported update path.

See `README-HISTORICAL.md` and the versioned production documents for earlier
experimental 1.5.x checkpoints and detailed provenance.
