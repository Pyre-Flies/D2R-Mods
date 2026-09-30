# QOL rev.50: integrated controller aim

Date: 2026-09-30. Integration baseline: standalone aim.20, QOL rev.49.
The user confirmed aim.20's idle-after-Teleport lock marker before this merge.
That is evidence for the standalone implementation, not this combined DLL.

## Ownership and lifecycle

Implementation now lives in `src/aim/`, compiled into Controller QOL; no build
or runtime dependency on the prototype directory. Its SDK config, input actions,
callbacks and hooks belong to the QOL context. The prototype remains historical
source/rollback material. No second plugin identity or ownership export is built.

`QolAim::Initialize(context, qolEnabled)` runs after priority-object initialization.
`[qol] enabled=false`, missing `[aim]`, or `[aim] enabled=false` prevents aim
service queries, input registrations and hook installation. Invalid aim settings
return failure to the caller, which logs and keeps other QOL features loaded.
Registration callbacks remain inert until all required services/listeners/actions
succeed (ready flag). Natural gameplay lifecycle triggers exact-byte admission;
already-in-game load attempts it after registration. Partial hook failure leaves
installed=false so wrappers call originals. SDK teardown owns removals. Shutdown
clears ready/enabled/installed/session before QOL tears down other modules;
queued UI tasks check ready before touching native state. Hot reload unsupported.

`QolAim::OwnsGuidedArrow()` reads installed/enabled/session atomics. The existing
controller-only skill22 projector yields when true, so short cursor distances
survive. Off/failed aim retains normal QOL projection. The old cross-DLL export
and GetModuleHandle/GetProcAddress ownership path are removed.

Contact admission still requires CALL opcodes at game0x1922CD/0x192378 and exact
UnitTest body except the two documented displacement fields. Original destination
0x34BC90 is accepted. A redirected call must have the exact FF25/zero-displacement
14-byte relay shape, with destination equal to `QolPortal::CandidateContact`
through `OwnsContactDestination`, and a non-null originalContact. Checking exact
internal function identity replaces the impossible self-referential DLL hash.
No arbitrary module-owned wrapper is accepted; CALLs/contact entry are unchanged.

## Existing native contracts moved with source

All game RVAs/byte guards/layouts remain build-specific. Full witness bytes are
in `src/aim/native_profile.h`; detailed ABI/provenance and rejected hypotheses
remain in [prototype native history](AIM-NATIVE-HISTORY.md).
Reviewed Core SHA256 stays
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.

| Game RVA | Integrated use |
| --- | --- |
| 0x190440 | Coordinate lookup, 15-byte inline hook |
| 0x1919F0 | Candidate observation, 14-byte inline hook; native eligibility runs |
| 0x18AF30 | Scoped circular score, 14-byte inline hook |
| 0x4FDB40 | Pass-through cast observer, 15-byte inline hook |
| 0x18DDE0 | Active supported-skill selected-unit suppression, 14-byte inline hook |
| 0x7E7CE0 | Guarded XY wrapper owns full ground-pick output buffer |
| 0x7E65A0 | Guarded renderer access |
| 0xCEAED0 / 0xD03A30 / 0xD03760 | Reviewed drawing-list/clipping correction |

0x7E7E50 remains a guard witness, never a direct raw ground-picker call. No new
native RVA, layout or ABI was discovered by this integration. Native character
turning, travel/collision and unsupported/mouse targeting semantics remain.
Copied candidates retain 150 ms TTL and 30-tile cap; no native unit pointer is
retained across frames. Render smoothing never modifies cast coordinates.

## Configuration and migration

One config: `d2rloader/config/controller-qol-updates.toml`. `[qol]` stays separate
from `[aim]`; bounded reads grow from2048 to16384bytes. Section extraction strips
comments and excludes other sections, so aim.enabled cannot affect qol.enabled.
The aim parser accepts the existing strict numeric/boolean keys plus enabled;
duplicates, unknown keys or invalid values disable only aim. Defaults are opt-in
false with the prior motion/snapping/display defaults. All keys are documented
in the main README. Runtime performs no automatic config writes/migration.

Local migration copies the user's current prototype values into `[aim]`, enables
it, and preserves every existing QOL value. Disable the standalone DLL before
launch; integrated initialization refuses its standard loaded module name, and
exact hook guards also refuse patched entries. Keep prototype config unchanged.
Full rollback restores QOL rev.49 plus its original config and standalone aim.20;
restoring the old QOL DLL alone cannot host the merged feature.

## Validation boundary

23 automated suites pass: the original21 plus aim policy and runtime gating.
Artifact checks validate the combined DLL's ABI/identity/version, embedded parsed
aim defaults off, no prototype export, and retained QOL defaults. Runtime gating
uses a fake loader API to verify global-off/missing/explicit-off/invalid aim do
not query services/install hooks/claim Guided Arrow. Policy tests cover section
ordering/isolation, duplicates and tuning plus the carried targeting tests.
These do not establish live combined hook admission or overlay/input behavior.

Live checklist: confirm QOL rev.50 and [QOL/Aim] install logs; Teleport then Meteor
marker/cast agreement; short Guided Arrow with F8 on/off; Whirlwind reversal and
pass-through toggle; Leap; one inventory shortcut, neutral-A priority object and
stash paging. Restart with aim.enabled=false to confirm native aiming and QOL
remain functional. New merged runtime validation is pending.

## Local deployment

Verified game/loader absent, then installed combined DLL SHA256
`6E5E0DB9A9008B75440035C7465854133A429E19E6E37ED73B7A4D570A2C23E1`; deployed hash matches built. Preserved
`Controller QOL Updates.rev49.rollback.dll.disabled`,
`Controller Aim Test.aim20.rollback.dll.disabled`, and
`config/controller-qol-updates.rev49.rollback.toml`. Disabled the exact active
prototype DLL after hash-verifying its rollback copy. Prototype TOML unchanged.
All existing QOL values and all current prototype aim values were preserved;
local `[aim] enabled=true`. Live integrated validation remains pending.
