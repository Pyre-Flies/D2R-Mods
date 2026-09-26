# Validation

## 0.1.1: loader rejection corrected

The user's 0.1.0 test produced no display because the loader never loaded the
DLL. `d2rloader/logs/d2rloader.log`, 2026-09-23 20:50:16.806, reports it as
incompatible/old alpha. The RC file omitted the required RT_RCDATA manifest
resource ID 1001 (`0x03E9`), containing the four-byte plugin ABI version (4).
This was a packaging bug, not evidence that the input adapters executed.

0.1.1 adds that resource using the SDK macros, matching the working QOL RC.
A new artifact test inspects the compiled DLL's manifest and verifies it agrees
with exported PluginInfo, lifecycle exports and role. It rejects the installed
0.1.0 DLL with exit 3 (missing manifest); all three suites pass for 0.1.1.
The installed replacement passes that same artifact test. Built, packaged and
installed SHA-256: `A69B81C6B0257A0C0B5B6812D10DA88DD7FA01D50F4D3F3BB434735624982D03`.
The user subsequently confirmed the hook works and shows the same information
as Ctrl/RT. This confirms the input integration, not the new 0.2.0 formatter.

## Completed

* Windows x64 Release build, MSVC 19.44, `/W4 /WX /guard:cf`.
* Policy tests: all 16 allowed-panel combinations, foreground loss, ALT release,
  Ctrl-bit removal, preservation of unrelated modifier bits/callers, R1 release,
  disconnect and outside-panel rejection.
* Adapter tests invoke the actual callbacks: original parameters/full return
  preservation for unrelated callers, inactive late calls, atomic slot exchange,
  conflict rejection, restoration, invalid load and exported plugin identity.
* Static provider audit: exact core hash, exported entry, RIP-relative slot
  arithmetic, alignment, Ctrl interpretation and controller/panel witnesses.

These checks do not establish that the plugin successfully loads or that a
range appears correctly in a running game.

## Historical 0.1.0 installation (superseded)

With D2R closed, the new DLL was copied (without replacing an existing plugin)
to `F:/SteamLibrary/steamapps/common/Diablo II Resurrected/mods/Reimagined/d2rloader/plugins/Item Roll Ranges.dll`.
Built and installed SHA-256 both equal
`88E8C3A6A0F9E87B99452D294661A12A6FBF30A91C64513319964F1D0A0F84D3`.
QOL and loader configuration were not changed. First launch remains pending.

## Required in-game checks

1. Cold launch Reimagined with QOL plus this DLL. Confirm its startup log,
   capture original slot target RVAs and inspect their ABIs. A mismatch must
   leave stock input behavior intact. Verify the slots remain installed after
   loading completes.
2. Inspect identified Aldur's Stony Gaze. Hold ALT: the Defense modifier must
   show the active 80–120 range on its own line, without confusing total item
   defense. For 0.2.0 require `[+80 - +120] +118 Defense`, preserving the actual roll.
   Repeat using the user's actual +118 item if available.
3. Hold/release ALT repeatedly with a stationary hover; ranges must track hold
   immediately. Ctrl alone must not trigger them. Alt-tab must hide them.
4. Controller: R1/RB hold/release, switch selected items, disconnect, switch
   active controller, reconnect. RT alone must not trigger ranges. Check whether
   native R1 changes tabs/weapon sets; this candidate does not consume that input.
5. Repeat in inventory, personal/shared stash, Cube and vendor on both input
   methods. Check ground labels, closed panels, trade and other overlays for
   unintended display, including cases where inventory remains open underneath.
6. Check unidentified items, socketed/merged stats, magic/rare/crafted items,
   runewords, set bonuses, compared items and non-English text. Native coverage
   must be recorded rather than assumed. No extra lines or duplicate ranges.
7. Verify QOL identify, sell, quick move, shared-page controls and ground
   looting. Check existing range hints: their Ctrl/RT wording may need a separate
   scoped localization change after observing the installed UI.
8. Restart with DLL removed and confirm stock Ctrl/RT behavior returns. Reload
   mod data/rejoin and confirm the native range provider uses current tables.

No live gameplay test was available during the initial build. Keep this version
labelled experimental until the matrix is completed.

## 0.2.0 candidate

Five Release suites pass: policy, real adapters, DLL ABI artifact, inline formatting,
and compiled x64 stack fixture. Inline tests cover the exact Defense example,
negative bounds, localized text, long lines, fixed bounds, grouping/suppression,
unsupported descriptions and missing-row delegation. The stack fixture verifies
eight native arguments and actual value at return-slot+0x4C through a JMP detour.
No automated result establishes live game behavior for this new formatter.

Built DLL SHA-256: `DC4A85196B29B04309D5FE96C7E260C06D23B5C65F100E010A8C331D5AF11151`.
Rollback package: `outputs/Item-Roll-Ranges-0.1.1-experimental.zip` in the Codex
2026-09-22 workspace. Complex/grouped stat lines currently retain actual values
without range prefixes. Run the in-game matrix above after a cold launch.

Deployed with D2R closed to mods/Reimagined/d2rloader/plugins/Item Roll Ranges.dll.
Installed SHA-256 matches the build above. The artifact check passes against the
absolute installed DLL path (its DLL-load flags require an absolute path).

## 0.2.1 correction

User rejected 0.2.0 in-game: screenshot retains +(80-120) Defense without +118.
0.2.0 loaded successfully, per plugin startup log. 0.2.1 corrects the mode gate
from only 4 to 0/4. Five suites pass, including exact Defense text in BOTH modes
and rejection of modes -1,1,2,3,5,6,7,8. Read the bounded format diagnostic after
the next live test; absence of records would identify a different execution path.
Live 0.2.1 result remains pending.

Deployed 0.2.1 with D2R closed; installed artifact check passes. Build/installed SHA-256: 5D2811C7426FCCB1ACEE5868AE91A3383510FD1DB5DF9DD236A0A518593F50AA.

## 0.2.2 generated-caller fix

User again saw the native range-only output with 0.2.1. The bounded diagnostic
records prove caller-address mismatch for stat 31, mode 0, bounds 80..120.
0.2.2 removes stack-local extraction and reads the supplied stat list. Four
suites pass: policy, adapters/stat-list extraction, compiled DLL artifact and
inline formatting. The obsolete JMP-only stack fixture is excluded from CTest.
Regression coverage includes an extra wrapper, +30/+A8 list layouts, exact
stat/layer selection, oversized count, missing entries and duplicates.
Next live check must show a v0.2.2 diagnostic with stat=31, actualRaw matching
the item and text containing both range and actual value. Result -3 identifies
an unavailable actual value; -2 identifies input/panel/mode gating. Live output
is not yet verified. Previous successful unit tests did not qualify gameplay.

Initial 0.2.2 deployment was blocked by a locked DLL. D2RLoader.exe (PID 57856)
was running even though Get-Process D2R returned no process. Future deployment
checks must include D2RLoader as well as D2R. Installed version remains 0.2.1
until the process is closed and replacement is verified.

Deployment completed after both game/loader processes closed. Installed 0.2.2
artifact passes; build/installed SHA-256: F4B0DB70312C9C229A54D70F601E62388854D534B1B8858A211478D26981CC6B.

## 0.2.3 wide-stat correction

User confirmed 0.2.2 still range-only. Trace result=-3 identified actual lookup
failure. Read-only live code inspection and matching installed Core disassembly
show 16-byte entries (64-bit key, signed int32 actual at +8), not eight-byte
native-game entries. Core+0x3F1EB2 is the verified caller. Four updated suites
pass, including wide layer keys, normal/extended lists and exact prefix text.
Live rendered output remains pending. Startup logs alone do not validate it.

Installed 0.2.3 with both processes closed. Artifact check passes. Build/installed SHA-256: DE3F8896FB6A332B82E51D40F9464FD8C53D063B900B84AC28D94D74767EBEB8.

## 0.3.0 actual-source correction and broad property prefixes

User confirmed the Defense prefix worked in 0.2.3, but unheld value was +118 and
held value +80. This is a correctness failure, not a completed actual-value test.
0.3.0 renders the original item through the native property formatter and merges
only native range annotations from the clone pass. Light blue uses native U.

Four active suites pass: policy, real adapters/two-pass source selection, DLL
artifact and whole-property text matching. Cases include +118 versus range
80..120, ED actual38 versus25..50, fire damage pairs, proc level fields, grouped
resistances, ambiguous duplicate labels, localized text/UTF-8 tags and capacity
fallback. Historical scalar and stack fixtures are not counted as active tests.

Installed with game/loader closed; artifact passes. Build/installed SHA-256:
`CDECDFFDD9780E1E66C8AACE072E7BD2462F4DC368E6320CBA56EFFCF3158CEF`.
Next runtime check must confirm ACTUAL contains +118, RESULT preserves +118,
ED retains its actual percentage, prefixes use light blue, and release restores
the usual tooltip. Base header fields outside the property block remain a
separate unverified scope. Live 0.3.0 validation is pending.

## 0.3.2 parser regression from live buffers

0.3.0 user test failed to add property prefixes. 0.3.1 diagnostic confirmed both
native passes correct, but runtime color bytes EE 81 BE (U+E07E) were not parsed.
0.3.2 adds that color introducer. Four suites pass; the exact captured complete
Aldur blocks now produce two annotations, zero unmatched lines and preserve
+118 Defense / +30% Enhanced Weapon Damage. No fake-item values are substituted.
Rendered color and runtime output remain pending until a fresh 0.3.2 launch.

Installed with game/loader closed, installed artifact passes. Build/installed SHA-256: FDDECB1E23A8E7F703ED50605CF71002AEC35E5E38E19ADB7615EF48DA212BB4.

### 0.3.2 live Aldur verification passed

User confirmed "Looks right now!" after cold-launching 0.3.2 and holding ALT.
The live trace independently reports annotated=2, unmatched=0. RESULT contains
U+E07E U[+80 - +120] U+E07E 3 +118 Defense, and
U+E07E U[+25 - +50] U+E07E 3 +30% Enhanced Weapon Damage.
The three fixed lines remain unchanged. This verifies real-item actual values,
fake-item bounds, matching and rendered light-blue prefixes for this helmet.
It does not qualify every item family, localization, controller path or panel.

## 0.3.3 Ctrl / R1
Keyboard gesture changed to VK_CONTROL in both query and fresh hold checks; R1/RB unchanged. All four automated suites pass. Installed with game/loader closed; installed artifact passes. Build/installed SHA-256: B237B153774AD070FAD491D4CBC2796C05FC76D3C3A7BB53D0B3DE861FBF74A8. Live Ctrl gesture validation remains pending; formatting was confirmed in 0.3.2.
