# Repository instructions

- Treat D2R/D2RCore RVAs, addresses, signatures, layouts, provider hashes, and
  native contracts as build-specific.
- Record every newly discovered path, RVA, address, byte guard, structure
  offset, and ABI detail in `docs/reverse-engineering/` or the affected plugin's
  documentation in the same change.
- Preserve proven native semantics and fail open when compatibility guards do
  not match.
- Never commit game binaries, generated build output, local logs, backups, or
  extracted proprietary assets.
- Update the affected plugin's changelog for user-visible or compatibility
  changes and distinguish automated checks from live-game validation.
