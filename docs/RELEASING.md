# Publishing releases

## Runtime documentation

Runtime ZIPs contain only each plugin's DLL, reference TOMLs under defaults/
and one complete user README with commands, all settings, editing examples
and required notices. The package uses each plugin's canonical README.md.
Changelogs, separate compatibility records, licenses and engineering
reports remain in the repository. Required redistribution notices are included
in the runtime README. Keep checksums alongside the ZIPs, not inside them.
Check examples against the parser and distinguish session-only from saved
settings. Scan packaged text for private paths and internal tooling references.
Source archives retain technical evidence; export-ignore excludes local workflow
material. Documentation changes must preserve the tested DLL bytes.

## Current stable suite

The staged suite contains Controller QOL rev.78, Item Roll Ranges rev.46,
Map Assistance rev.1, Inventory Deposit rev.13, Floating Damage rev.13 and
Always Show Sockets rev.2. Every plugin uses `1.3.1+rev.N`; the revision is
independent per plugin. Windows file/product versions use `1.3.1.N` and no
prerelease flag. Full suite manifests cover six runtime ZIPs, plus one adjacent
SHA256SUMS file. Historical beta tags remain channel-validated for reproduction.

## Historical Inventory Deposit prerelease route

The standalone `inventory-deposit-beta` workflow runs for annotated tags
`beta-inventory-deposit-*`. It builds/tests Inventory Deposit, packages one
runtime ZIP plus `SHA256SUMS`, and publishes a prerelease without changing the
latest stable suite release. Its version, resources, changelog and
`plugins/inventory-deposit/RELEASE-NOTES.md` must agree before tagging.

Install Python 3.10+ and the plugin's pinned Pillow dependency for the embedded
sprite conversion. For local packaging after a Release build and CTest pass:

```powershell
python tools/test_package_inventory_deposit.py
python tools/package_inventory_deposit.py --dll "build/inventory-deposit/Release/Inventory Deposit.dll" --artifact-verifier "build/inventory-deposit/Release/inventory-deposit-artifact-tests.exe" --tag beta-inventory-deposit-0.1.0-beta.13 --output dist/inventory-deposit
```

The archive contains only this plugin's DLL, reference config under `defaults/`,
a usage README with required notices. Editable original PNGs live
in the source archive; no MPQ or generated sprite files need separate install.
Verify the tag commit, CI, prerelease status, downloaded archive checksums and
DLL identity after publication. Preserve the existing stable release and all
unrelated working-tree changes. This focused path does not replace the six-plugin
suite workflow.

Runtime DLLs and ZIPs are GitHub Release assets, not source-controlled files.
This keeps normal clones small and prevents compiled binaries from obscuring
source history.

## Automated release

1. Select the exact reviewed release commit and verify GitHub Actions `build`.
   Build from a clean checkout of that commit. Exclude unrelated working-tree
   changes rather than including or reverting them.
2. Choose a repository release tag. Because the plugins have independent
   versions, use a suite tag such as `release-2026.09.26.1` rather than treating
   any plugin version as the repository version.
3. Create and push the annotated tag:

   ```powershell
   git tag -a release-2026.09.26.1 -m "D2R Mods release 2026.09.26.1"
   git push origin release-2026.09.26.1
   ```

The `release` workflow performs clean Release builds, runs all tests, creates
the six versioned runtime ZIPs, creates `SHA256SUMS`, and attaches all seven
files to the GitHub release. GitHub separately supplies repository source
archives.

The suite includes Controller QOL Updates, Item Roll Ranges, Map Assistance,
Inventory Deposit (the Deposit All Button), Floating Damage and Always Show
Sockets. All six use stable `1.3.1+rev.N` versions with independent revisions.
Promotion does not extend the documented live-validation coverage.
Inventory Deposit's artwork conversion requires Python 3.10+ and Pillow in
the build environment; generated sprites are embedded in its DLL, not shipped
as loose runtime assets.

The release is not published if any build, test suite, ZIP integrity check,
or packaging step fails.

## Local package verification

After building all six plugins, run:

```powershell
python tools/package_runtime.py `
  --controller-dll "build/controller-qol/Release/Controller QOL Updates.dll" `
  --ranges-dll "build/item-roll-ranges/Release/Item Roll Ranges.dll" `
  --map-assistance-dll "build/map-assistance/Release/Map Assistance.dll" `
  --inventory-deposit-dll "build/inventory-deposit/Release/Inventory Deposit.dll" `
  --inventory-deposit-artifact-verifier "build/inventory-deposit/Release/inventory-deposit-artifact-tests.exe" `
  --floating-damage-dll "build/floating-damage/Release/Floating Damage.dll" `
  --floating-damage-artifact-verifier "build/floating-damage/Release/floating-damage-artifact-tests.exe" `
  --always-show-sockets-dll "build/always-show-sockets/Release/Always Show Sockets.dll" `
  --always-show-sockets-artifact-verifier "build/always-show-sockets/Release/socket-artifact-tests.exe" `
  --output dist
```

DLLs install at `d2rloader/plugins/*.dll`. Extract into the game directory.
Reference TOMLs belong only under `defaults/`, outside the live loader config
directory. **Do not ship any `d2rloader/config/` paths**, even for fresh installs:
the loader creates a missing main config from embedded defaults. QOL creates
the active skill profile on aim initialization. All six plugins follow the same
reference-only archive rule for their main configs. An embedded default does not
replace a saved value; document new settings and let users merge only the keys
they want to change.

Run `python tools/test_package_runtime.py` before packaging. Both the packager
and regression tests enforce safe paths and preservation of customized configs.
Verify adjacent ZIP checksums and packaged file bytes; audit text for private data.
Record automated results separately from live-game validation.

For QOL, keep shared defaults in `controller-qol-updates.toml` and per-skill
defaults in `skill-defaults.toml`. Package both as references, link the repository
[configuration guide](../plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md),
and describe migrations and changed defaults in release notes. Existing explicit
settings must remain intact; never deploy a whole default file over user tuning.
After publication, verify the tag's exact commit, successful CI, stable/prerelease
status, all expected assets and downloaded checksums.

The adjacent release `SHA256SUMS` covers the six ZIP assets. The ZIPs
contain no checksum files, separate guides, changelogs or development documents. Verify that there are exactly
six plugin ZIPs and one adjacent `SHA256SUMS`, and that each ZIP installs exactly
its own DLL. Source archives are separate GitHub-generated downloads.

## Manual upload

If GitHub Actions is unavailable, build, test, and run the same packaging script
locally. On the repository's GitHub page, open **Releases**, choose **Draft a
new release**, select or create the suite tag, and drag the six ZIPs plus
`SHA256SUMS` into the release assets area. Do not commit them to `main`.

## Controller QOL beta prereleases

Use an annotated `beta-controller-qol-*` tag for a QOL-only beta. The separate
`controller-qol-beta` workflow builds/tests QOL and publishes one runtime ZIP
plus `SHA256SUMS`, with GitHub `prerelease: true` and `make_latest: false`.
It does not invoke the stable `release-*` workflow or publish other plugins.
Update the QOL version/resources, changelog and `docs/CONTROLLER-QOL-BETA.md`
before tagging. For local packaging use `tools/package_runtime.py
--controller-only --controller-dll <path> --output <directory>`.
