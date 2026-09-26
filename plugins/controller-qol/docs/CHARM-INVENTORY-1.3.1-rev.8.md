# Charm Inventory compatibility candidate 1.3.1+rev.8

## Cause and evidence

Charm Inventory registers a public PanelService controller route. The loader handles bumper actions before the native UISwitcher handler QOL previously intercepted. Reporter trace showed 18 translated triggers and zero blocked bumpers; local reproduction confirmed the problem. This is route handling order, not lack of Xbox input.

Inspected Charm DLL SHA256: B14F6A0BD4C9DF0AF9C392EA97DC879AD3760592481517DBDB0983595145D88B. Core SHA256: 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2. See XBOX-MENU-REPORT-2026-09-27.md and src/menu_route_profile.h.

Live read-only verification in local PID 39920 (D2RLoader.exe, game base 0x140000000, Core base 0xC0DE5000000):
- Game virtual dispatch slot RVA 0x1CEF450 -> game thunk 0x3E2B1A8.
- Thunk FF25 indirect jump uses game .maho import slot 0x3E2A6C8 -> Core 0x45D7D0.
- Core dispatcher calls 0x45D880 -> 0x463CC0 route handler first. True returns early. False forwards through Core slot 0x70EA68 -> game 0x27E620 (outer native message handler), which can reach game 0x27DF80 (old QOL menu interception).
- Handler action mask 0x980200 selects 9,19,20,23; triggers 7/8 pass through. Message hashes at +0/+0x88, payload pointer +0x110, action payload +0x10. ABI void(panel*, message*) at outer dispatch; route predicate returns bool.

## Candidate implementation

SDK PatchBytes owns one eight-byte virtual dispatch slot patch at game 0x1CEF450. The wrapper reuses existing scoped menu policy: consume 19/20 only for main menu scope, translate 7/8 into 19/20, and invoke the original loader thunk once for allowed input. Skills/Quest/dedicated-panel policy remains in place. The native 0x27DF80 entry is not hooked when the route-aware path is admitted, preventing duplicate remapping and letting the loader include the Charm tab in navigation. No Charm files or private Core slots are modified.

Admission checks exact Core file hash, live dispatcher/route code witnesses, existing native menu signatures, exact virtual slot/thunk/import chain and native forwarding target. Another slot owner is rejected. SDK rechecks the expected pointer at publication. Plugin is pinned for delayed calls; SDK handles patch cleanup, and cleared ctx makes wrapper passthrough. No fallback after a patch publication attempt. If the route profile fails before publication, legacy guarded native menu remap remains available; that fallback does not resolve custom route bypass.

## Validation

Release build and 13 suites passed, including new route-admission negatives and simulated main/registered-route navigation. Live addresses and predecessor chain verified read-only before build. These checks do not prove the candidate's in-game navigation behavior. User gameplay validation pending.

Test inventory LB hold plus identify (no screen rotation); bare LT/RT cycling including Charm Inventory; Skills and Quest bumper sub-tabs; Shared LB+LT/RT; close/reopen inventory and leave/re-enter game. Test both Xbox/Battle.net report and previously working controllers if available. Capture startup Route-aware UISwitcher dispatch installed and qol trace/status. Separate portal/belt mismatches in reporter logs are not fixed or attributed by this change.

## Patch recovery

Re-find route registration handler and outer virtual dispatch; verify incoming panel/message ABI, consumption ordering and native forwarding path. Re-find the game thunk and import slot, not just the Core function. Preserve full reviewed code witnesses and test all exceptions before accepting a new artifact hash. Never blindly regenerate expected bytes from unknown running code.
