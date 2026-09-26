# D2RLoader 1.3.1-beta startup investigation

Observed 2026-09-24; diagnosis only, no binaries or configuration changed.

## Evidence

Global d2rloader/logs/d2rloader.log identifies D2RLoader/D2RCore 1.3.1-beta and D2R 3.3.93787. The 08:48 launch uses no active mod and stops at plugin loading. Its qol.log identifies the older plugin ID qol; this is distinct from the 08:44 controller-qol-updates.log launch of 1.5.20. The current plugins directory contains QOL.bak, so do not infer both were simultaneously loaded from historical logs.

The 08:44 QOL log successfully registers SDK services/listeners, installs action observers, tooltip and pickup hooks, admits materials/belt contracts, installs XInput hooks and starts the 30 FPS worker. It then reports core/TabBar, labels, menu and skill compatibility rejection, ending at 08:44:29.448. The older qol log similarly ends shortly after starting its worker and reporting those compatibility rejections. This argues against a simple ABI rejection or a problem exclusive to the 1.5.20 action observers. It does not identify a faulting instruction by itself.

Installed D2RCore.dll SHA256: 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Previously reviewed SHA256: AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0.
Local SDK reference is still 0.2.0, ABI 4; user release notes identify SDK 0.3.0, ABI 4.

## Concrete unsafe path

src/controller_input.cpp EnsureNativeBridgeInitialized assigns private D2RCore addresses without checking its build:

| Core-relative RVA | Old assumed purpose |
|---|---|
| 0x67F4D0 | Controller manager slot |
| 0x456440 | GetActionStateByName |
| 0x4542F0 | IsActionActive |

IsNativeAltModifierActive dereferences the slot and invokes the function pointers if nonzero. The worker in plugin_main.cpp calls it every 33 ms, independent of debug_logging. QueryNativeBridgeDiagnostics also reaches these addresses. SEH does not make an arbitrary function call safe: code can hang, corrupt state or terminate without a catchable access violation.

Separately, qol_navigation.cpp VerifyCore correctly rejects the new file hash and protects its own ReadNative path (which also uses core RVA 0x680D28 for lookup). That protection is NOT shared with controller_input.cpp. Thus the log's claim that native navigation is disabled does not disable this second bridge. The worker timing is consistent with the abrupt log ending, but this remains a strong suspect, not a proven crash stack.

## Next implementation and verification

Remove or guard all private-core bridge calls before any XInput callback or worker can reach them. Prefer the supported 0.3.0 input API after inspecting its contract; retain physical XInput fallback where appropriate. Do not simply add the new file hash to the allowlist or reuse old RVAs. Navigation features rejected by the core guard must be separately requalified or migrated to supported services.

Capture a fault or hang stack / run a controlled build with the unsafe bridge excluded to establish causality. Confirm startup and in-game controller behavior, then validate stash/Cube, belt, loot labels, portal priority and co-installed plugins. Preserve the 1.5.19 golden archive. Current logs are under the global d2rloader/logs directory, not the previously active mods/Reimagined scope.

## 1.5.21 implementation

Shared core_compatibility.h retains the original exact SHA256 and query instruction guard, using bounded ReadProcessMemory for live bytes. Both navigation and controller_input now consult it. std::call_once publishes the bridge pointer set only after admission, preventing concurrent partial initialization. An unknown core publishes no callable pointers. Admission runs before XInput hooks and the polling worker. No new core RVA was guessed or accepted. Physical XInput remains available; core-dependent navigation/labels/glyphs retain their existing rejection behavior.

Official 0.3.0 headers inspected at https://raw.githubusercontent.com/D2RLoader/PluginSDK/master/include/D2RLPlugin/input.h and version.h. InputService still has keyboard registerAction/unregisterAction/getBinding, not a native controller-state query. Build remains ABI 4 with bundled 0.2.0 headers for this bounded repair. Broad SDK migration is separate.

Global deployment: F:/SteamLibrary/steamapps/common/Diablo II Resurrected/d2rloader. Preserve global TOML byte-for-byte and back up DLL/config before replacing only the DLL. Do not change mods/Reimagined. Ten suites include concurrent bridge fallback and rejection of absent/foreign core modules. Runtime confirmation with the new loader is still required.

Runtime confirmation: global QOL log at 2026-09-24 08:57:35.742 reports QOL 1.5.21 loaded; global loader log at 08:57:42.239 reports D2R startup complete. Startup blocker no longer reproduces in this launch. Main-menu interaction/gameplay and disabled navigation feature restoration remain separate checks. Logs captured in the 1.5.21 source package.

