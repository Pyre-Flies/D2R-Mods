# D2RLoader Mods

Source, tests, compatibility records, and release documentation for PyreFly's
D2RLoader plugins for Diablo II: Resurrected.

## Plugins

| Plugin | Current source version | Status |
|---|---:|---|
| [Controller QOL Updates](plugins/controller-qol/README.md) | 1.3.1+rev.48 | Production; build-specific native hooks |
| [Item Roll Ranges](plugins/item-roll-ranges/README.md) | 1.3.1+rev.13 | Production; build-specific formatter/table integration |
| [Map Assistance](plugins/map-assistance/README.md) | 1.3.1+rev.1 | Configurable non-town campaign coverage |

Each plugin is independently buildable and keeps its own README, changelog,
tests, configuration, and detailed compatibility evidence. The D2RLoader SDK is
a pinned submodule at `third_party/D2RLoader-PluginSDK`, so both projects use the
same reviewed upstream interface without copying that project into this history.

## Repository layout

```text
plugins/
  controller-qol/       Controller QOL Updates source, tests, and docs
  item-roll-ranges/     Item Roll Ranges source, tests, and docs
  map-assistance/       Keys-tooltip pathfinding tips
third_party/
  D2RLoader-PluginSDK/  Pinned upstream build-time SDK submodule
docs/
  reverse-engineering/  Cross-project RVA and native-contract registry
skills/                 Reusable D2R engineering and gameplay skill source
.github/                CI, issue forms, and pull-request template
```

## Build

Requirements: Windows x64, Visual Studio 2022 with the C++ workload, Windows
SDK, CMake 3.29+, and Python 3 for the optional local build helpers.

Clone with submodules, or initialize them after a normal clone:

```powershell
git clone --recurse-submodules https://github.com/Pyre-Flies/D2R-Mods.git
# Existing clone:
git submodule update --init --recursive
```

```powershell
cmake -S plugins/controller-qol -B build/controller-qol -A x64
cmake --build build/controller-qol --config Release
ctest --test-dir build/controller-qol -C Release --output-on-failure

cmake -S plugins/item-roll-ranges -B build/item-roll-ranges -A x64
cmake --build build/item-roll-ranges --config Release
ctest --test-dir build/item-roll-ranges -C Release --output-on-failure

cmake -S plugins/map-assistance -B build/map-assistance -A x64
cmake --build build/map-assistance --config Release
ctest --test-dir build/map-assistance -C Release --output-on-failure
```

Native offsets, byte signatures, structures, hashes, and live-validation claims
are build-specific. Review [the compatibility registry](docs/reverse-engineering/README.md)
before carrying a hook or layout to another D2R or D2RCore build.
The accompanying [RVA and interception workflow](docs/reverse-engineering/RVA-AND-INTERCEPTION-WORKFLOW.md)
documents how those locations were discovered and how future findings must be
qualified and recorded.
For a compact map of the evidence and validation boundaries, start with the
[research handoff](docs/AI-HANDOFF.md).

## Releases and contributions

- Use one plugin-scoped change per pull request where practical.
- Update that plugin's `CHANGELOG.md` under `Unreleased`.
- Update the reverse-engineering registry whenever an RVA, signature, layout,
  hash, or native call contract is discovered or changed.
- Do not commit game binaries, crash dumps, local logs, build products, or
  extracted proprietary assets.
- Release tags should be plugin-scoped: `controller-qol/vX.Y.Z`,
  `item-roll-ranges/vX.Y.Z`, and `map-assistance/vX.Y.Z` (including `+rev.N`
  where required).

See [CONTRIBUTING.md](CONTRIBUTING.md) for the validation and release checklist.
Production DLLs are distributed as versioned GitHub Release ZIP assets. See
[Publishing releases](docs/RELEASING.md) for the automated tag workflow and the
manual fallback.
