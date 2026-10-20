# Contributing

Read [AGENTS.md](AGENTS.md) and the affected plugin's `DEVELOPMENT.md`.
Use `Pyre/` for a new contribution branch and keep unrelated working changes intact.
Current plugin versions and player behavior are documented in [README.md](README.md).

## Build and validate

Initialize the pinned SDK with `git submodule update --init --recursive`.
From the repository root in a Visual Studio x64 Developer Prompt, build one plugin
in the [external local workspace](docs/WORKSPACE.md), for example:

```powershell
$work = if ($env:D2R_MODS_WORKSPACE) { $env:D2R_MODS_WORKSPACE } else { Join-Path (Split-Path (Get-Location) -Parent) 'D2R-Mods-Local' }
$build = Join-Path $work 'scratch/controller-qol/build'
cmake -S plugins/controller-qol -B $build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build $build
ctest --test-dir $build --output-on-failure
```

Substitute the affected plugin. Its CMakeLists.txt defines any additional Python
or artwork dependencies. Run its complete relevant test suite after behavior changes;
for documentation-only changes, verify links and affected packaging checks instead.
Record automated checks separately from installed and live-game validation.
Update CHANGELOG.md for player-visible or compatibility changes.

## Native findings

Record each new native path, RVA, address, byte guard, signature, layout, offset,
provider hash or ABI contract in the owning feature document or native registry.
Include the exact module/build, evidence, guard, failure behavior and validation
limits. Preserve uncertainty and fail open on a mismatch. See the
[native registry](docs/reverse-engineering/README.md) for the record format.

## Review and publication

Complete the pull-request template for the actual scope. Use plain language and
honest validation; native implementation details belong in the feature record.
Do not include game binaries, extracted assets, generated output, local logs,
settings, saves or credentials. Reference configs in runtime packages belong under
`defaults/`, preserving active user configuration.

Use [RELEASING.md](docs/RELEASING.md) for publication and
[WORKSPACE.md](docs/WORKSPACE.md) for promotion and cleanup.
