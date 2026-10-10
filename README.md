# D2RLoader Mods

Source, tests, compatibility records, and release documentation for PyreFly's
D2RLoader plugins for Diablo II: Resurrected.

## Plugins

| Plugin | Stable release version | Status |
|---|---:|---|
| [Controller QOL Updates](plugins/controller-qol/README.md) | 1.3.1+rev.75 | Production; build-specific native hooks |
| [Item Roll Ranges](plugins/item-roll-ranges/README.md) | 1.3.1+rev.23 | Production; build-specific formatter/table integration |
| [Map Assistance](plugins/map-assistance/README.md) | 1.3.1+rev.1 | Configurable non-town campaign coverage |

Each plugin is independently buildable and keeps its own README, changelog,
tests, configuration, and detailed compatibility evidence. The D2RLoader SDK is
a pinned submodule at `third_party/D2RLoader-PluginSDK`, so all three projects use the
same reviewed upstream interface without copying that project into this history.

## Install and configuration

Download the runtime ZIPs from the [latest stable release](https://github.com/Pyre-Flies/D2R-Mods/releases/latest).
Close the game and extract into the game directory so DLLs land under
`d2rloader/plugins/`. Managed mod packs must distribute the update through their
normal package/revision process. Do not install duplicate copies of a plugin.

Current archives keep reference TOMLs under `defaults/` and contain no live
`d2rloader/config/` paths. Extraction preserves existing settings. Missing live
configs are created from the DLL's embedded defaults.

Controller QOL rev.75 uses two editable configuration layers:

| Location under `d2rloader/config/` | Contents |
| --- | --- |
| `controller-qol-updates.toml` | Shared QOL actions and general aim controls: speed, deadzone, colors, smoothing, logging |
| `controller-qol-skills/<scope>.overrides.toml` | Complete skill enable states, targeting, leading and Whirlwind settings for one mod or Vanilla; R3 saves here |

Find the profile by its `# Mod:` header. Matching `.catalog.toml` files are
generated references, not active settings. Upgrades migrate old skill settings
with verification and backups; they preserve user choices. R3 uses one rolling
backup per profile. New installs default to 60 ms display smoothing; explicit
older values are kept. Read the [configuration, migration and recovery guide](plugins/controller-qol/docs/SKILL-DISCOVERY-CONFIG.md)
for profile scope, editing, default changes and rollback.

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
- Stable releases use suite tags `release-YYYY.MM.DD.N`; each included plugin
  retains its independent version. QOL-only prereleases use `beta-controller-qol-*`.

See [CONTRIBUTING.md](CONTRIBUTING.md) for the validation and release checklist.
Production DLLs are distributed as versioned GitHub Release ZIP assets. See
[Publishing releases](docs/RELEASING.md) for the automated tag workflow and the
manual fallback.
