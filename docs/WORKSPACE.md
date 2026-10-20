# Source and local workspace

The repository checkout contains the files tracked on GitHub: source, tests,
documentation, build tooling and the pinned SDK submodule. It is the editable
authority for the six first-party plugins.

Keep machine-specific data in the sibling `../D2R-Mods-Local/` directory.
`D2R_MODS_WORKSPACE` can point local helpers at another workspace.
Run `python tools/local_workspace.py` to print the resolved location.

| Local path | Purpose |
| --- | --- |
| `CURRENT-BUILDS.md` | Current artifact, source, hash and validation index. |
| `current/release/` | Download-verified published ZIPs and checksums. |
| `current/<plugin>/` | Current DLL and relevant validation records. |
| `scratch/<task>/` | Reusable temporary builds, tests and candidate packages. |
| `companions/` | Independent Buff HUD and UnHoarder source checkouts. |
| `dependencies/` | Reusable machine-local tools and caches. |
| `backups/` | Temporary protection for unpublished work during reconciliation. |
| `archive/` | Unique historical source, settings and useful evidence. |

Use one scratch directory per active task. Build and test there, promote only the
selected artifacts, update CURRENT-BUILDS.md, then retire superseded scratch work.
Keep a release and an active candidate when needed. Retire temporary gold/rollback
artifacts after their replacement source and runtime assets are published and verified.
Preserve unique unpublished work, settings and saves before removing anything.

The former checkout-local `outputs/`, `build/`, `dist/`, `releases/`, `backups/`
and `companions/` directories are retired local conventions. Git ignores those
legacy names defensively. GitHub Actions uses temporary `build/` and `dist/`
directories inside disposable runners.

Independent projects such as the website belong in separate checkouts. Their
source, dependencies and build products are not part of this plugin repository.
