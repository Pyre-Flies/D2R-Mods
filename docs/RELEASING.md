# Publishing releases

Runtime DLLs and ZIPs are GitHub Release assets, not source-controlled files.
This keeps normal clones small and prevents compiled binaries from obscuring
source history.

## Automated release

1. Make sure `main` is clean and GitHub Actions `build` succeeds.
2. Choose a repository release tag. Because the plugins have independent
   versions, use a suite tag such as `release-2026.09.26.1` rather than treating
   either plugin version as the repository version.
3. Create and push the annotated tag:

   ```powershell
   git tag -a release-2026.09.26.1 -m "D2R Mods release 2026.09.26.1"
   git push origin release-2026.09.26.1
   ```

The `release` workflow performs clean Release builds, runs all tests, creates
the three versioned runtime ZIPs, creates `SHA256SUMS`, and attaches all four
files to the GitHub release. GitHub separately supplies repository source
archives.

The release is not published if either build, test suite, ZIP integrity check,
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

Runtime configs install at `d2rloader/config/*.toml` and DLLs at
`d2rloader/plugins/*.dll`. Extract the archive into the game directory; there is
no separate `configuration/` directory. When updating, preserve customized configs
or merge new defaults instead of overwriting personal settings.

Each ZIP contains its own `SHA256SUMS` for installed files. The adjacent release
`SHA256SUMS` covers the three ZIP assets themselves.

## Manual upload

If GitHub Actions is unavailable, build, test, and run the same packaging script
locally. On the repository's GitHub page, open **Releases**, choose **Draft a
new release**, select or create the suite tag, and drag the three ZIPs plus
`SHA256SUMS` into the release assets area. Do not commit them to `main`.
