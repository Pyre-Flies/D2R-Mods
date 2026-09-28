# Controller QOL Updates 1.3.1+rev.47

## Guided Arrow controller projection

Rev.47 integrates the live-proven standalone Guided Arrow correction into
Controller QOL's existing action-handler interception. For controller action
`0x05` only, a targetless Guided Arrow point less than eight tiles from the
player is copied and projected in the same direction to 20 tiles before the
native handler runs. The original five-byte packet is never modified.

The feature requires selected skill ID 22 and an exact 32-byte guard at game
RVA `0x34A540`. A guard mismatch or invalid player, skill, path, packet, or
coordinate disables the projection for that cast and forwards native input.
The optional feature guard does not control admission of Controller QOL's
other hooks.

No new hook is installed. The change reuses the already-owned, guarded
controller action handler at RVA `0x4ACE80`; mouse action `0x0C`, other skills,
explicit targets, already-distant coordinates, and all menu/stash controller
mappings are outside the mutation gate.

## Validation boundary

The 25-tile standalone projection was visibly confirmed to acquire and home;
the subsequent 18-tile standalone tuning also worked. The merged implementation
uses the requested 20-tile distance. All 21 automated suites pass, including
projection policy, hook isolation and DLL ABI/export/version checks. The built
and deployed DLLs are byte-identical with SHA-256
`BCD569DF40630A6AC34F81549E5A17C6DD52D1732D17BD6B15527D89A3DA928A`.
Loader admission and a focused live test of this merged rev.47 DLL remain
pending.

The previous rev.46 DLL was preserved as
`Controller QOL Updates.rev46.rollback.dll.disabled`, retaining SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
The standalone ga.24 DLL was removed from the active load set and preserved as
`Guided Arrow Targeting Test.ga24.dll.disabled`; only the rev.47 Controller QOL
DLL remains active under these names.
