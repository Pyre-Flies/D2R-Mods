# Controller QOL validation

## Rev.89 release - October 21

All 79 local CTest cases pass, including exact UnHoarder 1.2.16 admission and
pickup-dispatch mutation fixtures. The independent companion passes 50 CTest
cases and 51 source checks. Both perf commands remain available.

The exact rev.89/1.2.16 pair was verified loaded in Reimagined. The user reports
sustained over 100 FPS, usually around 112, with mixed loot and labels visible.
The pile and camera differed from earlier tests; this is not a controlled FPS
benchmark. Logs show reduced socket/property cost and two suppressed hidden
pickup requests. Full ALT/controller interaction, settings reload and broad
hosted regression remain unverified live. Historical entries below describe
their state when recorded, not the release's current installation status.

Run the affected targets and complete CTest suite using
[CONTRIBUTING.md](../../../CONTRIBUTING.md). CMakeLists.txt defines enabled tests;
optional exact-companion cases require their pinned fixtures.

The October 20 consolidated release passed 63 local QOL suites, including eight
optional companion cases. Its GitHub build and release workflows passed. This
establishes automated/source/artifact checks; no new live-game acceptance was
performed for the revision-name cleanup.

## Rev.83 local performance candidate - October 21

All 67 QOL CTest cases pass, including all nine exact companion cases and
actual-function hold-refresh, guard-cost and performance-report regressions. The unchanged
skill-discovery fixture required normal Windows temporary-file access after
its sandbox run failed; that retry passes. See [the performance contract](LOOT-PERFORMANCE.md)
for guard preservation, the exact UnHoarder 1.2.10 candidate and measurement limits.

The pre-existing rev.82 reload changes remain included. This candidate is not
installed or published and has no live-game performance acceptance.

## Rev.82 local candidate - October 20

The Windows x64 Release build and all 64 CTest cases pass, including all eight
pinned companion fixtures. Added checks cover the actual file-watcher worker,
automatic and console reload, invalid/missing root/profile retention, busy item
actions, held-input deferral, startup-only masks, aim publication without new
hooks, profile confirmation and R3 revision races. The production input gate is
also exercised directly. See the [reload contract](CONFIG-RELOAD.md).

All 16 runtime packaging tests pass. The repository documentation check covers
250 Markdown files with zero missing local links. Artifact validation checks the
rev.82 version/resource identity, exports and SDK ABI. The local candidate and
evidence are indexed in the ignored workspace's `CURRENT-BUILDS.md`.

No rev.82 DLL has been installed in a game directory or published to GitHub.
Live automatic/manual edits, appearance changes, controller item operations,
hosted play and persistence remain untested. During game acceptance, also edit
an invalid value, change a restart-only switch, and save with R3 while a reload
is deferred. Confirm the previous valid behavior survives, the switch waits for
restart, and the newer R3 choice persists.

For a new change, report automated checks, artifact identity, installation,
runtime observations, visible behavior and persistence separately. Hosted play
and cross-plugin composition require their own evidence. Retain current guards
and test refusal on incompatible bytes.

Earlier detailed results are retained in
[historical validation](history/VALIDATION-before-reorganization.md) and
[the history index](history/README.md). Their test counts and version labels are historical.


Shared-label rev.85 validation and limits are recorded in [loot performance](LOOT-PERFORMANCE.md#rev85-shared-label-preflight). The owned paired callback fixture measures redundant query counts, not game FPS; actual action and Hidden proofs keep their original guards.
