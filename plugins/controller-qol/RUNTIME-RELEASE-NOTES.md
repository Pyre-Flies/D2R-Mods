# Controller QOL Updates release notes

## 1.3.1+rev.79-beta.2

- Repair remote stash/Cube transfers and identification compatibility after
  unrelated Global Chat updates, while retaining native safety checks.
- Preserve existing settings, controls, destinations and duplicate-request guards.
- Thirty-three automated suites passed. This is a testing prerelease: remote
  transfers, identification charges and persistence after rejoining still need
  live validation. See the [test checklist](../../docs/CONTROLLER-QOL-BETA.md).

## 1.3.1+rev.78

- Improve distant ground-loot pickup, keep assignments stable while LB is held,
  and prevent filtered controller pickups while preserving gold collection.
- Add LB + Up/Down rule reordering and retain contextual item/storage shortcuts.
- Keep per-mod skill profiles and configurable right-stick aiming.
- Thirty-three automated suites passed. Hosted retesting confirmed the reported
  slow approach/blocked-ground symptoms no longer reproduced; fresh offline
  retesting and clean shutdown remain unconfirmed.
