# Source and local workspace

The repository checkout contains the files tracked on GitHub: source, tests,
documentation, build tooling and the pinned SDK submodule. It is the editable
authority for the ten first-party plugins.

Keep all machine-specific data in the single ignored `outputs/` directory inside
the checkout. GitHub shows the tracked project; `outputs/` holds local data only.
`D2R_MODS_WORKSPACE` can point local helpers at another workspace.
Run `python tools/local_workspace.py` to print the resolved location.
Paths below are relative to that workspace.

| Local path | Purpose |
| --- | --- |
| `CURRENT-BUILDS.md` | Current artifact, source, hash and validation index. |
| `current/release/` | Download-verified published ZIPs and checksums. |
| `current/<plugin>/` | Current DLL and relevant validation records. |
| `current/candidates/<plugin>/` | One validated local candidate, clearly separate from the published release. |
| `scratch/<task>/` | Reusable temporary builds, tests and candidate packages. |
| `companions/` | Independent Buff HUD source checkout. |
| `dependencies/` | Reusable machine-local tools and caches. |
| `backups/` | Temporary protection for unpublished work during reconciliation. |
| `archive/` | Unique historical source, settings and useful evidence. |
| `notes/` | Preserved unpublished working records. |
| `services/` | Local helper process receipts and logs. |

Use one scratch directory per active task. Build and test there, promote only the
selected artifacts, update CURRENT-BUILDS.md, then retire superseded scratch work.
Keep a release and an active candidate when needed. Retire temporary gold/rollback
artifacts after their replacement source and runtime assets are published and verified.
Preserve unique unpublished work, settings and saves before removing anything.

Do not create separate root-level `build/`, `dist/`, `releases/`, `backups/`
or `companions/` directories. Their local contents belong under `outputs/` as
described above. Git ignores those legacy names defensively. GitHub Actions uses
temporary `build/` and `dist/` directories inside disposable runners.

Independent projects such as the website belong in separate checkouts. Their
source, dependencies and build products are not part of this plugin repository.
