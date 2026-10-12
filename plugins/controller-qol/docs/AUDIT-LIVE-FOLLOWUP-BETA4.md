# rev.76-beta.4: follow-up to beta.3 live failures

This testing prerelease follows beta.3 live failures. Beta.3 passed automated checks,
but the user **actually moved the Horadric Cube into itself with LB+X**. That
invalidates any blanket claim that Cube transfers were fixed. Do not repeat
Cube self-insertion tests on beta.3. The original fixture lacked real Cube
item codes on the legacy quick-move path.

Beta.4 is now installed locally in Reimagined after the user closed D2RLoader.
Installed/build SHA256 is
`6F81B8D4233F91F7C1AAE1D08B58F7EE27464B440A869EE8B8E596430F2531D6`.
All 28 config files and 39 other plugin files remained byte-identical. Beta.3
and the earlier beta.2 backups remain preserved. The agent did not launch the
game; the user performed live testing. Publication was subsequently authorized
with a request to test the release and report all bugs.

On 2026-10-11, the user confirmed beta.4 in Reimagined: **Cube self-insertion
is fixed, escape from Shared Stash is fixed, and Cube behavior works through
rejoins**. These are user-confirmed live results for the tested single-client
setup. Individual shortcut/page combinations and the current network role were
not separately reported; this does not qualify the entire T03/T05/T06 matrix
or distinct-peer hosted behavior.

The user subsequently stated: "I think outside of online play, we should be
all set gameplay-wise." Record beta.4 **non-online gameplay as accepted based
on the user's testing**, with online qualification remaining open. This is
overall acceptance, not evidence that every matrix subcase was individually
run. The current pickup reach/no-approach behavior remains documented and
unchanged; no further pickup repair is requested by this acceptance. Local
acceptance remains distinct from online qualification. The user subsequently
authorized publication as a testing prerelease.

## Evidence and repair

The LB+Y queue had an exact `0x20786F62` (`box `) exclusion. The legacy
`QueueLegacyQuickMove` selected Cube for an Inventory source with the Cube
open, and `ExecuteMove` submitted it without a Cube-item semantic check.
The confirmed LB+X gesture matches this bypass. The preserved log does not
contain the actual Cube transaction: this is user-confirmed behavior and a
source-demonstrated route, not a traced transaction. The earlier code-padding
hypothesis is **not** the confirmed cause; SDK `MakeItemCode("box")` pads spaces.

`QolCubeShortcut::IsCube/Allows` recognize space and equivalent NUL padding
(`0x20786F62` / `0x00786F62`) without aliasing an arbitrary fourth character.
They refuse Cube destinations for a Cube item in the LB+X/LB+Y queues and
hints, the final executor using freshly resolved SDK ItemInfo (including
secondary destinations and misleading cached codes), the Shared SDK helper,
the existing remote policy and advanced proxy withdrawal. Rejection never
tries another destination or implementation. Cube -> Inventory remains
eligible, including an already nested Cube, subject to game acceptance.
No save is edited, item reconstructed, or recovery move submitted automatically.

The user also reported Shared -> Gems escape while holding LB+LT then pressing
RT, but no reverse escape. `SharedPageGesture` passed an unowned repeat through
even when the opposite trigger owned a Shared-page chord. The second trigger
is now consumed and owned until its release, whether its begin carries a repeat
flag or not. Both orders are tested. The first trigger turns a page once; the
opposite held-trigger gesture neither changes the main tab nor reverses the
page direction. Fully release both triggers to rearm. This closes a demonstrable
leak; the user's exact native message flags were not captured.

No new native hook, ABI, RVA, provider admission or configuration key is added.
The existing guarded BankPanel `0x23CAA0` / selected-tab `0x23AF50` contract is
reused. No network packet or host/guest authority rule is changed. Hosted
admission still requires identical plugin sets and versions on both sides.

## Pickup and persistence limits

T01 picked up two of three items; slight movement enabled the third. T03
reported that shortcut pickup does not approach loot, while mouse pickup does.
These behaviors remain unchanged and are documented limitations within the
accepted non-online gameplay. Distance/collision/native acceptance have not
been isolated as the cause of the third-item failure; no pickup fix is claimed.

Read-only reinspection of the already recorded capture's
`0xFA405..0xFA48C` branch confirms coordinate assembly and immediate opcode
`0x16` submission at `0xFA482`, followed by the interaction exit. This is not
evidence for a complete native approach contract. Beta.4 keeps pickup behavior
unchanged; no custom movement or server-side position manipulation is added.

Save & Exit followed by a new offline game does not preserve dropped ground
items. T01 verifies cancellation and fresh assignments, not ground persistence.
Check persistence only for items actually transferred into saved storage.

## Verification and reassessment

Release build and all **33/33 CTest suites** pass; production integration
regressions report **424 checks**, and Shared-page/glyph tests **61 checks**.
New production regressions
cover both Cube code forms, LB+X/LB+Y, Inventory/Personal/Shared sources,
secondary Cube destinations, ordinary-item LB+X, nested Cube withdrawal and
actual BankPanel decoding in both overlapping-trigger orders. Shared SDK and
remote policy tests separately cover rejection before submission and valid
withdrawals. Private mock SDK/native fixtures do not prove live game behavior
or hosted persistence.

| Beta.3 live case | User report | Current assessment |
| --- | --- | --- |
| T01 | Two of three pickups; movement needed; held X caused no issue | Partial; pickup reach open. Fresh-game ground disappearance expected. |
| T03 | Most controls worked; no pickup approach; overlapping trigger escape | Beta.4 Shared escape fix user-confirmed. Overall case remains partial; pickup approach open. |
| T04 | Cleared | User-confirmed for the tested setup. |
| T05 | LB+X moved Cube into itself | Beta.4 Cube self-insertion fix user-confirmed. Other closed/changed-context steps remain unreported. |
| T06 persistence | Not separately reported on beta.3 | Beta.4 Cube behavior through rejoins user-confirmed. Specific Shared pages/full-Cube cases remain unreported. |
| Others | No further result supplied | Pending, or automated only when a second actor is required. |

Use [the updated single-client checklist](AUDIT-REPAIRS-IN-GAME-TESTS.md).
Beta.3 source, DLL rollback, private log and deployment receipt are preserved
under local `outputs/`. Reproduce with:

```powershell
python plugins/controller-qol/tools/build_local.py --source plugins/controller-qol --build outputs/controller-qol-beta4-20261011-build
```
