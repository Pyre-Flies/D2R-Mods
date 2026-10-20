# Controller QOL validation

Run the affected targets and complete CTest suite using
[CONTRIBUTING.md](../../../CONTRIBUTING.md). CMakeLists.txt defines enabled tests;
optional exact-companion cases require their pinned fixtures.

The October 20 consolidated release passed 63 local QOL suites, including eight
optional companion cases. Its GitHub build and release workflows passed. This
establishes automated/source/artifact checks; no new live-game acceptance was
performed for the revision-name cleanup.

For a new change, report automated checks, artifact identity, installation,
runtime observations, visible behavior and persistence separately. Hosted play
and cross-plugin composition require their own evidence. Retain current guards
and test refusal on incompatible bytes.

Earlier detailed results are retained in
[historical validation](history/VALIDATION-before-reorganization.md) and
[the history index](history/README.md). Their test counts and version labels are historical.
