# Publishing releases

The stable suite contains ten first-party plugins and the separately maintained
Buff HUD companion. The [repository README](../README.md) lists current versions;
`release-companions.json` pins the exact companion package and payload hashes.
Experimental and diagnostic plugins stay in D2R-Mods-Research.

## Prepare

1. Reconcile finalized code, shared dependencies, tests and version metadata into
   the main checkout. Preserve unrelated work. Use a `Pyre/` branch for new publication work.
2. Build and test the affected scope from the exact reviewed commit using
   [CONTRIBUTING.md](../CONTRIBUTING.md). Check version resources, changelogs and READMEs.
3. Prepare candidates under `outputs/scratch/<task>/` in the ignored local workspace.
   See [WORKSPACE.md](WORKSPACE.md) for the layout and optional location override.
4. Run the relevant packaging checks:

   ```powershell
   python -m unittest discover -s tools -p "test_package*.py" -v
   ```

Use the packaging tool's `--help` for required DLL, artifact-verifier and output
arguments. Write candidate ZIPs to scratch. Runtime ZIPs contain only the direct
installable files (`d2rloader/plugins/<Plugin>.dll`, plus `d2rloader/data/hd/tc22sh/*.bin`
for Ultrawide and `Reimagined.mpq/data/...` sound assets for Pyre Loot Filter) so
both the D2R Reimagined Launcher's **Install Plugin/Zip** and manual drag-and-drop
work with zero leftover files. Keep checksums beside the ZIPs. Verify extraction
layout, metadata, ABI, exports and hashes; scan public text for private paths and credentials.

## Publish and verify

Publish only authorized scope. Staging files alone does not authorize a push or release.
Stable suite tags follow `release-YYYY.MM.DD.N`; plugin revisions are independent.
The `release` workflow builds/tests/packages the ten first-party plugins and stages
the verified Buff HUD companion package.
GitHub Actions' `build/` and `dist/` directories are disposable runner output.
Retained prerelease workflows support their historical tag conventions.

Buff HUD is released from its own fork (`Pyre-Flies/d2rl-buff-hud`) and pinned in
`release-companions.json`. Buff HUD requires its DLL and MPQ together. Preserve
source attribution and required notices. See [companion sources](COMPANION-SOURCES.md).

Confirm the tag commit, workflow outcome and release status. Download the published
assets and verify archive hashes, contents, payload hashes and metadata before promotion.
A successful build or deployment is not new live-game acceptance.

## Promote and clean

Promote download-verified artifacts into the ignored local workspace's
`current/release/` and indexed current plugin locations. Update CURRENT-BUILDS.md
with source commits, paths, hashes, companion requirements and validation limits.
Retire superseded scratch builds and temporary gold/rollback copies after their
replacement is published and verified. Preserve useful unique evidence and
unpublished work rather than archiving entire obsolete generated build trees.

Changing a signed Ladder bundle requires updating its package and manifest through
the bundle's normal publishing process. Replacing only installed DLLs breaks its
manifest correspondence. Keep installed, cached and current server package claims separate.
