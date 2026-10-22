# Gem Bag UI development

Read the repository [AGENTS.md](../../AGENTS.md), [native contract](NATIVE-CONTRACT.md)
and [deposit contract](DEPOSITS.md). This is the shipping source; the earlier
research checkout retains investigation helpers and historical evidence.

`src/policy.h` owns recipe choices, placement and compensation ordering.
`src/native.cpp` owns guarded native admission, credit changes and item publication.
`src/catalog.cpp` discovers owned definitions from compiled data on both peers.
`src/plugin.cpp` owns UI/input lifetime; `src/geometry.cpp` and `src/layout.h`
own the native picker. `src/rpc.cpp` and `src/protocol.h` keep requests bounded
and server authoritative. `src/server_frame.cpp` retains the qualified host
callback and ownership-checked teardown.

Keep native build guards separate from recipe discovery. Do not trust client
item pointers, credits, output codes or player IDs as mutation authority.
The host resolves the authenticated sender and checks its own item and definitions.
Shared-stash deposits stage through supported inventory moves and compensate
only on proven clean refusal. Uncertain outcomes must not trigger blind refunds
or automatic retries. Drag-to-bag input is outside the accepted controls.

Build with the standard [contributor workflow](../../CONTRIBUTING.md), using
`plugins/gem-bag-ui` as the CMake source. `D2R_MODS_ROOT` defaults to this checkout
and can be overridden. Dependencies are the pinned public SDK and reviewed
Controller QOL/Inventory Deposit headers; neither plugin DLL is required.
Twelve CTest suites cover policy, deposits, native operations, geometry,
UI lifetime, headless loading, portable picker, RPC, host callback, admission
and compiled data. Artifact tests verify version, loader ABI, exports and roles.

Use [release procedures](../../docs/RELEASING.md) and `tools/package_runtime.py`
from the repository root. Keep captures, game binaries and diagnostic logs out
of releases. The research [feasibility record](https://github.com/Pyre-Flies/D2R-Mods-Research/blob/main/docs/reverse-engineering/gem-bag-ui-feasibility.md)
is historical context, not an additional compatibility admission.

## Validation

1.3.1+rev.1 promotes beta.19 behavior; only version/build integration and documentation
change. Earlier live tests cover mouse/controller withdrawals. Beta.19's loose
shared-stash deposit was checked on client and host, and save/rejoin was confirmed.
Shared cluster staging, full-inventory refusal and broader multi-game isolation
remain automated-only or untested live. Rebuilt release checks do not extend
that live coverage.
