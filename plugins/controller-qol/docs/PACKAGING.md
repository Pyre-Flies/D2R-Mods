# Release naming and configuration contract

Controller QOL Updates 1.5.14, PyreFly. Packaging revision: 2026-09-23.

- DLL: `Controller QOL Updates.dll`; CMake target names remain internal build identifiers.
- Plugin ID: `controller-qol-updates`; configuration: `d2rloader/config/controller-qol-updates.toml`.
- Retained TOML section: `[qol]`; no settings schema or gameplay behavior change.
- Windows VERSIONINFO OriginalFilename matches the renamed DLL.

## Evidence and reproduction

The local upstream SDK `F:/SteamLibrary/steamapps/common/Diablo II Resurrected/Documentation/PluginSDK-master/README.md`, lines 90 onward, specifies `<scope>/d2rloader/config/<plugin-id>.toml`, creation only if missing, and preservation of existing config. Its `examples/config-file/config-file-sample.toml` lines 3–4 confirms the ID-based path. Bundled `sdk/include/D2RLPlugin/context.h` exposes `pluginConfigPath` and the native `ReadConfig` API, which this plugin continues to use.

The installed `D2RCore.dll` contains the validation diagnostic `Use only lowercase letters, numbers, '.', '_' or '-'` immediately between missing-ID and invalid-ID diagnostics. SHA256: `AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0`. Diagnostic file offset: `0x5CB0A8`, PE RVA: `0x5CC2A8` (not a callable function address). Re-find this ASCII string and the neighboring `Plugin id is invalid` message after loader updates. The exact display name with spaces/capitals is therefore not used as the plugin ID; the supported lowercase, hyphenated identity preserves native configuration management. No new runtime hook or game RVA is introduced.

## Migration and checks

Remove the old QOL.dll from the scanned plugin directory; the loader sees the renamed ID as a different plugin. Rename existing qol.toml to controller-qol-updates.toml, retain [qol], and preserve user settings. Update explicit load-order IDs if present. The old log is historical; loader log naming follows the new identity. Golden archives are unchanged.

Default debug logging is false in the C++ fallback, source TOML and embedded release resource. Artifact tests check the exported new identity and embedded debug default. CTest validates policies without invoking the game's plugin-load lifecycle. A subsequent game launch should confirm loading once under the new ID and reading the renamed config. This packaging operation does not alter the installed game plugin/config.

## 1.5.15 continuation

The filename, plugin ID and config section remain unchanged from the branded 1.5.14 release. Version 1.5.15 adds portal priority; see PORTAL-PRIORITY.md. Defaults still disable debug logging. Package config is a reference copy and existing installed settings are preserved.

## 1.5.17 continuation

Fixes the native Interact portal distance gate. Default portal radius remains 10; preserve existing user settings (this installation uses 20). Debug logging remains false. The previous DLL and TOML are backed up before deployment; the installed TOML is not replaced.

## 1.5.18 continuation

Adds native portal scoring retry and optional portal_diagnostics (release default false). This test installation enables only portal_diagnostics, keeping debug_logging false and radius 20. Back up the previous DLL/config before replacing the DLL and adding that one diagnostic setting.

## 1.5.19 continuation

Extends only controller candidate contact checks for eligible portals, using guarded call-site return addresses. Existing radius 20, general debug false, and portal diagnostics true are preserved byte-for-byte in the installed TOML. Release defaults remain radius 10 and both diagnostic flags false.

## 1.5.20 continuation

Adds third-party plugin cooperation and corrects optional label-limit diagnostics. See POTION-AUTO-PICKUP-COMPATIBILITY.md for contracts and scope. Nine automated suites; combined runtime qualification remains pending. Preserve installed TOML byte-for-byte, including user portal settings. Release defaults keep debug_logging and portal_diagnostics false. The 1.5.19 golden archives remain untouched.

