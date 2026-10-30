# Repository guide

Read this file, then the affected plugin's `DEVELOPMENT.md`. Use
[docs/README.md](docs/README.md) only to find the topic you need. Search that
plugin's `src/` and `tests/` first; historical records are opt-in (`rg --no-ignore`).

## Ownership and safety

- `plugins/` contains the twelve production plugins listed in README.md plus shared
  helpers in `plugins/shared/`. Keep experiments and diagnostics in the separate
  D2R-Mods-Research repository.
- Buff HUD is maintained in its independent companion fork; see
  [companion sources](docs/COMPANION-SOURCES.md).
- Treat native addresses, RVAs, signatures, layouts, provider hashes and ABIs as
  build-specific. Record new native paths, guards, offsets and contracts in the
  owning plugin's documentation or `docs/reverse-engineering/` in the same change.
- Preserve proven native behavior and fail open when a compatibility guard fails.
- Preserve existing user work, settings and saves. Commit source and useful
  documentation; keep game binaries, generated output, logs and backups outside Git.

## Working and finishing

- Use the ignored `outputs/` workspace (override with
  `D2R_MODS_WORKSPACE`) for builds and artifacts; see [workspace layout](docs/WORKSPACE.md).
- Validate affected behavior. Distinguish automated checks, installation and live
  game results. Update the plugin changelog for player-visible or compatibility changes.
- Run `python tools/check_docs.py` after changing documentation or its layout.
- Reconcile shipping code, dependencies, tests and versions into this checkout.
  Keep one indexed release and one active candidate; update the local CURRENT-BUILDS.md.
- Retire superseded scratch builds after validation. Gold/rollback artifacts are
  temporary until their replacement is published and download-verified. Before cleanup,
  check absolute paths, ownership, active use and links; preserve unique unpublished work.
- Use `Pyre/` for new publication branches. Read [CONTRIBUTING.md](CONTRIBUTING.md)
  for contributions and [docs/RELEASING.md](docs/RELEASING.md) when packaging or publishing.
