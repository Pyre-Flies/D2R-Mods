# First public prerelease validation

Target: `beta-inventory-deposit-0.1.0-beta.13`, recorded 2026-10-11.

The release was prepared in a separate clean checkout based on public main
`1245e0dbeee887f653aad3aa8c5ba3eb740b1594`. Only Inventory Deposit, its release
tooling/docs/skill, and the optional shared batch observer were added. The
existing stable suite release and unrelated local development are outside scope.

## Local automated evidence

- Release build with MSVC 19.44.35220.0 and the pinned PluginSDK succeeded.
- Inventory Deposit: all 12 CTest suites passed, including the exact DLL
  manifest/exports/resources and native ButtonWidget probe.
- Controller QOL: all 29 CTest suites passed with the optional observer changes.
  Existing callers use the default null observer; no QOL package accompanies
  this prerelease.
- Packaging: all 8 regressions passed, including existing-config preservation,
  checksum completeness, verifier failure, DLL changes during verification,
  deterministic archive contents, tag/version matching and privacy checks.
- The local package verified the exact supplied DLL again before archiving.
  DLL SHA-256:
  `6CBB6F6726B002EF85D83B00BACFBD95704529D060DD0632A32F8D337753B4BD`.
  This is a local build identity, not a claim that a separate CI build has
  identical PE bytes. Published assets carry their own internal/external sums.
- An independent read-only review found no release-blocking issue in native
  guards, object binding, callback lifetime, observer scope or package contents.
  The reusable button skill passed schema validation and scenario review.

## Live evidence and boundaries

The user's local beta.13 build was accepted, including the final tooltip.
Earlier iterations established working deposits, embedded artwork, approved
fit, per-destination feedback and beta.12 disabled/hover appearance. See the
feature records for exact historical hashes, logs and user-confirmed results.

The CI-produced DLL must separately pass the same artifact tests before release.
Local user reports do not constitute a live test of those separately built PE
bytes. Pressed appearance, low-end rendering, other resolutions, remote
destination counts, save/reload integrity and simultaneous Controller QOL
batches remain separately scoped validation targets.

## Publication contract

The main-branch build covers all four plugins. The dedicated Inventory Deposit
tag workflow rebuilds/tests this plugin, verifies and packages its DLL, and
publishes one ZIP plus `SHA256SUMS` as a prerelease with `make_latest: false`.
Verify successful workflows at the exact tag commit and audit the downloaded
assets before reporting publication complete. GitHub's tagged source archives
contain the original editable PNGs; runtime assets contain no MPQ, loose game
assets, logs, captures or live configuration.
