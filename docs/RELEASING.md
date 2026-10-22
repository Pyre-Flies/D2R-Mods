# Publishing releases

The stable suite contains eight first-party plugins and two separately maintained
companions. The [repository README](../README.md) lists current versions;
`release-companions.json` pins the exact companion packages and payload hashes.
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
   python tools/test_package_runtime.py
   python tools/test_package_inventory_deposit.py
   ```

Use the packaging tool's `--help` for required DLL, artifact-verifier and output
arguments. Write candidate ZIPs to scratch. Packages use the plugin's canonical
README.md, installable runtime files, required notices and reference TOMLs under
`defaults/`. Keep engineering notes and changelogs in source. Keep checksums
beside the ZIPs. Verify extraction layout, metadata, ABI, exports and hashes;
scan public text for private paths and credentials.

## Publish and verify

Publish only authorized scope. Staging files alone does not authorize a push or release.
Stable suite tags follow `release-YYYY.MM.DD.N`; plugin revisions are independent.
The `release` workflow builds/tests/packages the eight first-party plugins.
GitHub Actions' `build/` and `dist/` directories are disposable runner output.
Retained prerelease workflows support their historical tag conventions.

UnHoarder and Buff HUD are released from their own forks. Use the exact companion
ZIPs pinned in `release-companions.json`; QOL companion admission can depend on the
DLL hash, so an arbitrary rebuild is not interchangeable. Buff HUD requires its
DLL and MPQ together. Preserve source attribution and required notices.
UnHoarder's Reimagined package supplies sound tables; preserve pack-specific fields
when applying sound routing to another pack. See [companion sources](COMPANION-SOURCES.md).

After the eight-plugin workflow succeeds, attach the two verified companion ZIPs
and regenerate adjacent `SHA256SUMS` for all ten runtime ZIPs. Confirm the tag
commit, workflow outcome and release status. Download the published assets and
verify archive hashes, contents, payload hashes and metadata before promotion.
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
