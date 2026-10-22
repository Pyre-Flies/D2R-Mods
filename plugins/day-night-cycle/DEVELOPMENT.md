# Day Night Cycle development

Read the repository [AGENTS.md](../../AGENTS.md) and [native contract](NATIVE-CONTRACT.md).
This directory is the shipping source; investigation helpers remain in research.

`native-profile.json` is the build-specific ledger. Run
`python tools/generate_profile.py --check` from this plugin directory to verify
the generated header. Update both files together when qualifying a new build.

`src/plugin.cpp` admits the exact host/Core identities, ordinal, native code
and runtime bindings. `src/compatibility.h` owns read-only guards;
`src/owned_patch.h` publishes and conditionally restores the owned block.
`src/runtime.h` serializes enable/disable. Preserve the original native clock,
normal wrapper and setter, global debug state and all ownership checks.

`src/preview.h` claims the renderer override only from an unowned state and
restores it only while the current object, flag and values still match.
`src/settings.h` preserves unrelated TOML text and rolls back native changes
when saving fails. Commands and shutdown serialize through the SDK UI queue.
Queued work checks the live context; parser callbacks remain pinned for the
process lifetime. Replacing the DLL requires a restart.

Build using the standard [contributor workflow](../../CONTRIBUTING.md) with
`plugins/day-night-cycle` as the CMake source. `D2R_MODS_ROOT` defaults to this
checkout. The build uses the pinned SDK, static CRT, CFG and strict warnings.
Five suites cover owned native replay, guards, command/settings behavior,
generated profile consistency and loader artifact identity. Replay and command
fixtures execute captured instructions only in private owned memory.

Package with the root `tools/package_runtime.py` according to
[release procedures](../../docs/RELEASING.md). The archive includes the DLL,
player guide/notices and reference TOML under `defaults/`.

1.3.1+rev.1 preserves beta.3 behavior with release metadata/build integration.
Natural cycling was user-confirmed on beta.2. Fixed day/night controls, area
transitions and hosted sessions still need live testing; automated success
does not establish visible rendering.
