# Controller QOL Updates 1.3.1+rev.10

Includes the rev.7 scoped ground-label and portal-call compatibility improvements,
rev.8 Charm Inventory menu fix, rev.9 custom-page LB+X transfer, and rev.10
controller-mode-only item shortcut hints.

Qualified Core SHA256:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
ABI4, Windows x64. Native profiles remain build-specific and guarded. An
unsupported native input profile may fall back to XInput with reduced device
coverage; unsupported custom-page transfer is disabled. Hot reload unsupported.

User confirmed Charm Inventory menu switching and transfers. Previous device
validation covered Battle.net DualShock with Steam closed, plus Steam Controller
and DualShock through Steam, with Stash Search and Potion Auto Pickup. The full
matrix was not repeated for this release. Fourteen automated QOL suites pass.
Detailed rev.10 input-switching and custom-page full-grid/rejection/persistence
matrices remain unrecorded. Do not infer universal other-plugin compatibility.

The custom-page operation preserves the registered item policy but does not replay
the outer mouse ItemInteraction notification wrapper. See the rev.9 contract.
Default debug and portal diagnostics are off. Keep existing user configuration.
