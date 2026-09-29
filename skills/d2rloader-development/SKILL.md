---
name: d2rloader-development
description: Investigate, build, validate, deploy, and package D2RLoader plugins for Diablo II Resurrected, especially work involving native hooks, RVAs, provider ABIs, game data layouts, controller input, item transactions, or build compatibility. Use for D2R/D2RLoader engineering; do not use for ordinary gameplay advice or unrelated Diablo II mod configuration.
---

# D2RLoader Development

Develop against the installed D2R and D2RLoader build with evidence-led, fail-open compatibility handling. Preserve working installs and native game behavior. Prefer a small, testable probe when a native mechanism or ABI is not yet proven.

## Start with the real environment

- Locate the actual source tree, CMake target, deployed DLL, loader/provider DLLs, configuration, and logs before proposing edits. Do not assume the current working directory is the source checkout.
- Inspect local logs and artifacts directly when available rather than asking the user to reproduce long output.
- Treat executable versions, hashes, RVAs, byte patterns, structure layouts, provider tables, DLL paths, and table row layouts as build-specific.
- Find the project's existing research and compatibility records before starting fresh. Prefer repository evidence over remembered values.

## Choose the smallest useful experiment

- For an uncertain hook, input path, transaction, or native layout, isolate one observable pass/fail behavior in a standalone probe before merging it into a working plugin.
- Prefer D2RLoader SDK facilities and embedded plugin code. Add an external runtime dependency only when its concrete benefit justifies the extra installation and compatibility burden.
- Preserve native semantics, rendering, values, and interaction paths where the feature extends existing behavior. Do not invent mappings or labels when provenance is ambiguous.

## Gate native behavior

- Verify the exact module or provider artifact used at runtime, not merely a nearby build number or source snapshot.
- Guard native entry points with the narrowest sufficient combination of artifact identity, expected bytes, ABI/layout checks, table bounds, and lifecycle checks.
- Fail open: on a mismatch, disable only the unsupported feature, leave the game functional, and log a concise diagnostic.
- Keep hooks, callbacks, queued work, and teardown safe across load/unload and session changes. Do not promote a partial disassembly or static candidate into a complete ABI claim.

For native investigation, read [references/reverse-engineering.md](references/reverse-engineering.md). For recording build-specific facts, read [references/compatibility-ledger.md](references/compatibility-ledger.md).

## Validate claims proportionally

- Separate static evidence, unit/simulated tests, artifact checks, loader admission, authoritative game state, visible UI behavior, persistence, and user-confirmed live results.
- Tests prove only what they observe. A successful native return code or authoritative state change does not prove client visibility, save persistence, or full item integrity.
- Build with the project's warning and runtime conventions. Inspect the produced DLL's exports, ABI/resource metadata, imports, version, and hash when those are part of the loader contract.
- Before replacing a deployed DLL, confirm the game and loader are closed when necessary, preserve a recoverable backup, then compare built and deployed hashes.

For test scope and evidence wording, read [references/validation.md](references/validation.md). When producing a distributable build, read [references/release.md](references/release.md).

## Build loader-admissible plugin artifacts

For every new Windows plugin DLL, include a resource script in the CMake target
and enable the `RC` language. The resource script must include
`D2RLPlugin/resource.h` and `D2RLPlugin/version.h` and embed
`D2RL_PLUGIN_ABI_VERSION` at `D2RL_PLUGIN_MANIFEST_RESOURCE_ID` as a one-`DWORD`
`RCDATA` resource. Windows `VERSIONINFO` is also expected for shipped plugins,
but it does not replace the ABI manifest. A DLL with valid lifecycle exports but
no ABI manifest is rejected by current D2RLoader as an old alpha plugin.

Add an artifact test for every new plugin. It must open the built DLL with
`LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE`, verify the manifest
resource exists, is exactly `sizeof(DWORD)`, and equals
`D2RL_PLUGIN_ABI_VERSION`. Then load the DLL normally and verify the three loader
exports, `PluginInfo` size/ABI, plugin identity/version, and a valid plugin role.
Run this test before deployment or packaging; source compilation and export
inspection alone do not prove loader admission.

## Preserve discoveries

Whenever reverse engineering finds a new path, RVA, address, byte signature, field offset, ABI/layout detail, table binding, native consumer, or rejected hypothesis, record it in the relevant project's versioned research or compatibility ledger during the same task. Include provenance, target build/artifact identity, confidence, limits, and validation state. Update an existing canonical record instead of scattering duplicate notes.

At handoff, state what changed, what was verified automatically, what was verified live, what remains unverified, and which build-specific records were added or updated.
