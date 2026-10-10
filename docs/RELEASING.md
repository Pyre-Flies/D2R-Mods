# Publishing releases

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
the three versioned runtime ZIPs, creates `SHA256SUMS`, and attaches all four
files to the GitHub release. GitHub separately supplies repository source
archives.

The release is not published if any build, test suite, ZIP integrity check,
or packaging step fails.

## Local package verification

After building all three plugins, run:

```powershell
python tools/package_runtime.py `
  --controller-dll "build/controller-qol/Release/Controller QOL Updates.dll" `
  --ranges-dll "build/item-roll-ranges/Release/Item Roll Ranges.dll" `
  --map-assistance-dll "build/map-assistance/Release/Map Assistance.dll" `
  --output dist
```

DLLs install at `d2rloader/plugins/*.dll`. Extract into the game directory.
Reference TOMLs belong only under `defaults/`, outside the live loader config
directory. **Do not ship any `d2rloader/config/` paths**, even for fresh installs:
the loader creates a missing main config from embedded defaults. QOL creates
the active skill profile on aim initialization. Map Assistance follows the same
reference-only archive rule for its main config.

Run `python tools/test_package_runtime.py` before packaging. Both the packager
and regression tests enforce safe paths and preservation of customized configs.
Verify internal and external checksums and audit packaged text for private data.
Record automated results separately from live-game validation.

For QOL, keep shared defaults in `controller-qol-updates.toml` and per-skill
defaults in `skill-defaults.toml`. Package both as references, include the
[configuration guide](../plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md),
and describe migrations and changed defaults in release notes. Existing explicit
settings must remain intact; never deploy a whole default file over user tuning.
After publication, verify the tag's exact commit, successful CI, stable/prerelease
status, all expected assets and downloaded checksums.

Each ZIP contains its own `SHA256SUMS` for installed files. The adjacent release
`SHA256SUMS` covers the three ZIP assets themselves.

## Manual upload

If GitHub Actions is unavailable, build, test, and run the same packaging script
locally. On the repository's GitHub page, open **Releases**, choose **Draft a
new release**, select or create the suite tag, and drag the three ZIPs plus
`SHA256SUMS` into the release assets area. Do not commit them to `main`.

## Controller QOL beta prereleases

Use an annotated `beta-controller-qol-*` tag for a QOL-only beta. The separate
`controller-qol-beta` workflow builds/tests QOL and publishes one runtime ZIP
plus `SHA256SUMS`, with GitHub `prerelease: true` and `make_latest: false`.
It does not invoke the stable `release-*` workflow or publish other plugins.
Update the QOL version/resources, changelog and `docs/CONTROLLER-QOL-BETA.md`
before tagging. For local packaging use `tools/package_runtime.py
--controller-only --controller-dll <path> --output <directory>`.
