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
- Keep this repository's plugin source scoped to Always Show Sockets, Controller
  QOL, Floating Damage, Inventory Deposit, Item Roll Ranges, and Map Assistance.
  Experimental and diagnostic plugins, their tools, and unpublished research
  belong in the sibling `D2R-Mods-Research` checkout; never commit them to this repository.
- Maintain UnHoarder in its fork and Buff HUD in its upstream contribution
  checkout. Index their companion artifacts here without importing their source
  as first-party plugins.

## Task completion and workspace cleanup

- Finish each task by retiring its superseded test DLLs, runtime archives,
  generated build folders, and isolated game installs after the replacement
  passes the relevant validation. Reuse a task workspace instead of creating
  another dated copy for every iteration.
- Keep one clearly indexed current build per plugin and an active candidate
  when needed. Maintain a stable index at `outputs/CURRENT-BUILDS.md` with the
  authoritative source checkout and revision, artifact paths and hashes,
  required companion files, and validation status. Distinguish current releases
  from experimental candidates and historical handoffs; update the index when
  those change and link it in the final handoff.
- Reconcile finalized implementation, shared dependencies, tests, and version
  metadata into the main source tree before considering a release task complete.
  Preserve existing working changes during reconciliation. Publication worktrees
  are temporary staging; the only current implementation must not remain under
  `outputs/` while the main checkout drifts behind.
- Treat gold and rollback copies as temporary publication baselines. Retire
  superseded copies once their replacement revision and required runtime
  artifacts are published and verified on GitHub. Preserve source and
  uncommitted work, settings, saves, and useful research and validation records;
  retain uncertain unique artifacts until their replacement is established.
- Before deletion, verify each absolute target stays within the intended
  workspace, check tracked-file ownership and active use, and inspect junctions
  and symlinks. Remove obsolete links themselves without traversing their
  targets. Preserve unique settings and saves before retiring a test install.
- Record what was removed and retained, verify retained build and gold hashes,
  and report any remaining cleanup or source/build mismatch explicitly.
  Routine cleanup of superseded artifacts created by the task is part of task
  completion; permission to clean outputs does not authorize deleting source,
  unpublished unique source or active installations.
