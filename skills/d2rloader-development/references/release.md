# Release and sharing

Use this reference when the user asks to package or share a D2RLoader plugin.

## GitHub branches and publication text

Follow the `github-publishing` skill for GitHub contributions, forks, PRs, and releases. Use `Pyre/` for new contribution or release branches unless the user explicitly chooses another name. Preserve existing branch names and repository release-tag conventions.

Write publication titles, descriptions, PR bodies, and release notes in plain, human-readable language. Lead with what changed and why it helps; keep release notes brief and player-facing. Summarize relevant validation honestly and distinguish automated checks from live-game testing. Keep detailed packaging audits in the working record rather than copying checklists into public notes.

Forking or opening a PR does not itself require reverse-engineering documentation. Keep RVAs, addresses, byte guards, provider hashes, structure offsets, ABI details, and investigation history out of publication prose unless requested or essential to review. Link relevant documentation when useful. Continue recording newly discovered native contracts in repository documentation when repository instructions require it; this is separate from the publication description.

## D2R-Mods release routing

For this project, resolve the D2R-Mods checkout and read `docs/RELEASING.md` plus the selected plugin's release instructions and current workflows. Those records define suite versus standalone/beta packaging, config preservation, CI and downloaded-asset verification. Use the user-authorized release scope. This reference is the maintained skill entry for packaging/release work formerly referred to as `d2r-mods-runtime-release`; do not require a separate skill with that name.

## Release contents and Launcher ZIP compatibility

Prepare:

- **A clean, drag-and-drop runtime archive (`<Plugin>-<version>.zip`) with zero extra fluff:**
  - Include **only** the installable files in their exact target folder structure (e.g., `d2rloader/plugins/<Plugin>.dll`, plus `d2rloader/config/...` only when shipping an installable config).
  - Do **not** include `reference/`, `defaults/`, `licenses/`, `README.md`, `SHA256SUMS`, or other non-runtime files inside the runtime `.zip`. A player extracting the archive into `mods/Reimagined/` and overwriting must be completely done with no leftover folders.
- **D2R Reimagined Launcher "Install Plugin/Zip" (`LocalPluginInstallService`) compliance:**
  - In `I:\reimagined-launcher\ReimaginedLauncher\Utilities\LocalPluginInstallService.cs`, `MapPath` routes `.dll`/`.mpq` to `plugins/<filename>`, paths containing `config/` to `config/<subpath>`, and `.toml`/`.ini`/`.cfg` to `config/<filename>`.
  - Any `.json` file outside a `config/` directory causes the launcher installer to abort with an error (`Cannot determine whether <name> is a config or patch`), and any other file types (such as `.bin` shaders or `.txt` MPQ tables) are skipped (`Skipped`) by the launcher installer.
  - Before publishing any plugin release, check all archive entries against `LocalPluginInstallService.MapPath`. If any required file cannot be installed by the launcher's "Install Plugin/Zip" button (or would be skipped/rejected), **explicitly notify the user before publishing** and explain why so you can pivot as needed.
- **Separate companion release assets (outside the runtime `.zip`):**
  - `SHA256SUMS` and any source archive (`*-source.zip`) belong as separate release assets alongside the runtime `.zip`, never inside the runtime `.zip`.
  - Put installation instructions, compatibility requirements, known limitations, and validation status in the GitHub release notes and repository `README.md`.

Exclude build caches, local absolute paths, logs containing personal information, backups, save files, unrelated binaries, and credentials.

## Pre-release checks

- Build cleanly with the project's configured warning level and runtime linkage.
- Run the relevant test suites and artifact verifier.
- Check exported loader entry points, plugin ID/version, ABI/resource manifest, and imported DLLs.
- Confirm the packaged DLL is byte-identical to the tested artifact.
- If installing locally, close D2R/D2RLoader as needed, preserve the previous working DLL, and confirm the deployed hash.
- Scan the package for machine-specific paths and accidental secrets.

Do not describe static or simulated coverage as live validation. State the exact build family supported and fail-open behavior for mismatches.
