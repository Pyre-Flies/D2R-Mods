# Loot filter controller editor investigation (2026-09-27)

Status: investigation only; no source or runtime behavior changes yet.
User reports neither D-pad nor left stick can reach controls below the
Equipment/Items row, including Gold and Potions. Wants inner tabs on LB/RB.
Baseline with Controller QOL disabled is pending to distinguish QOL interference
from a native/provider focus-routing problem.

Read-only widget-tree capture while the editor was open found:
- LootFilterOptionsPanel and its LootFilterBackground/ControllerCursorBounds.
- LootFilterRuleDetailsPanel containing BackButton, RuleNameInput,
  ProfileOptStack/ShowBtn/HideBtn, and ItemListTabs.
- ItemListTabs used the existing game TabBar vtable (game RVA 0x1D75E00),
  with TabLeftIndicator and TabRightIndicator children.
- Separate LootFilterItemPanel containing ItemList, dynamic ListItem templates,
  FilterItemToggle and CategoryExpandButton controls.
- ItemList contains TopScrollViewItemsEquipment and TopScrollViewItemsMisc.
  Misc contains FilterGoldToggle, GoldSliderStackWrapper/GoldSliderWrapper,
  and GoldSlider. Visibility is inherited: a visible checkbox under a hidden
  parent must not be treated as an available controller target.
- ItemListSelector was inactive/hidden in this initial capture.

Enumeration reused the existing panel manager game+0x3440170; widget names +8,
parent +0x30, active/visible bytes +0x50/+0x51, children pointer/count +0x58/+0x60.
These are read-only research offsets, not a new production dependency.

A later attempt to inspect tab bindings overlapped panel teardown: parent
visibility became 00/00 and some objects had base/reset vtables or invalid
slots. Its residual 7/8 bindings are NOT promoted to a live contract. Reacquire
all widgets after the next launch; never reuse the old object addresses.
The initial tree supports using the existing TabBar hook as a candidate,
but active message routing, bindings and the lower panel's focus path still
need verification. No new hook has been admitted or installed.

Do not equate the tab remap with repairing lower-panel focus. They are separate
requirements, and a native focus fix should reuse existing focus/scroll behavior
rather than synthesize clicks or modify saved loot-filter rules.

User baseline result: both D-pad and left-stick navigation into Gold/Potions
also fail with Controller QOL disabled. This rules out QOL as a necessary
cause in this configuration; it does not yet distinguish base-game behavior
from D2RLoader/mod-specific focus integration. Awaiting a fresh active editor
for live routing inspection.

## Fresh active-editor inspection and rev.23 tab candidate

Reacquired all widgets in the next process. Active ItemListTabs had immediate
parent LootFilterRuleDetailsPanel, both active/visible bytes 01/01, switch
+0x16A0 enabled, bindings +0x16A4/+0x16A8 = 7/8. Its vtable +0x20 message
slot resolved to game+0x878D30. This fresh observation supersedes the teardown
sample above and admits extending the existing scoped TabBar hook.

Rev.23 candidate tracks rule/details/item/options panels with separate submenu
bits 16/32/64, preventing ground shortcuts while the filter editor is open.
Only exact visible ItemListTabs/LootFilterRuleDetailsPanel uses copied 19/20
bumper messages translated to 7/8. Original triggers pass out of the inner tab.
The existing scoped glyph renderer changes its TabLeft/Right indicators to
bumpers. No new hook or native binding write. Lower-list focus remains unfixed.

Fresh read-only focus evidence: FocusManager (vtable game+0x1D748C8) fields
+0x188 and +0x198 pointed to LootFilterRuleDetailsPanel; +0x190 pointed to
ItemListTabs. Fields +0x170/+0x178 were zero. This is a bounded observed
relationship, not a fully established focus-manager ABI or mutation contract.
The editor's selected misc/gear presentation needs further correlation: initial
captures showed TopScrollViewItemsEquipment active and TopScrollViewItemsMisc
hidden despite the user's Items-tab report; do not infer selected-tab identity
from the screenshot alone.

Runtime provider dispatch (Core hash
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2):
- RuleDetails vtable Core+0x7B13D0, message slot +0x20 -> Core+0x2C1840.
  Its original message pointer Core+0x7B1350 -> game+0x14AFF80.
- ItemPanel vtable Core+0x7B10B0, message slot +0x20 -> Core+0x2C0F00.
  Its original message pointer Core+0x7B1030 -> game+0x14ACD50.
- ItemPanel wrapper recognizes LootFilterItemPanelMessage commands
  RuleTypeUpdated, ToggleGoldFilter, EditGoldSlider, UpdateGoldValue and
  InputMessage/ControllerActionBegin. String witnesses at Core+0x61D4D8,
  +0x61D4F8, +0x61D508, +0x61D520, +0x61D530, +0x61D558, +0x61D540.
- Wrapper +0x2C105F through +0x2C10B9 checks several non-null fields before
  forwarding to a virtual +0x20 handler. Complete receiver ownership and ABI
  are not proven, so no bypass or direct call was introduced.
These are diagnostic runtime discoveries only, not production dependencies.

Release build and all 19 suites pass for the tab candidate, including exact
widget/owner/visibility scope, inactive glyph behavior and blocking ground
shortcuts for the editor. Candidate version 1.3.1+rev.23 SHA256:
F2DF7B67C57E298004B00266B1E1516E9950BDBF21075F870561DB5B0AFCA62E.
Not installed yet. Native focus repair and candidate navigation are pending.

Controller-only follow-up: user reports the menu closes immediately on mouse
input-mode switching, so hover-to-focus is not a usable test. Right-stick down
also does not enter the lower controls.

Static native path: RuleDetails original handler game+0x14AFF80 calls input
helper +0x14AF750. At +0x14AF993 it checks action 22; +0x858010 tests whether
the panel owns FocusManager+0x188. It then calls +0x14AF520 to resolve the
selected item panel and +0x14ABC60 to enter it. Action 21 uses +0x858050.
The selected-panel resolver reads RuleDetails+0x3FC, calls +0x14B04C0 and
+0x846170, then checks metadata ancestry through virtual slot +0x58.
+0x14ABC60 calls +0x858050 followed by +0x14AA6D0 on panel+0x178 when nonnull.
These addresses describe a candidate native handoff, not a fully validated ABI.
No calls or patches to these routines have been made. Freshly reacquired rule
model +0x168 was nonnull; a null model is not established as the cause.

Candidate now includes at most 32 [QOL/Filter] numeric controller-action log
lines per process, scoped to the exact live rule-tab widget. This is temporary
bounded diagnostic output even with debug_logging=false; it records no rule
text or user input text. It will distinguish which native actions reach the tab
before choosing a focus repair. Final rebuilt candidate still passes 19 suites:
98799D064172162057924CA700BBC4E270FA395DF193E81846943BB387296678.
This supersedes the earlier candidate hash above. Awaiting closure/install.

Deployment 2026-09-27: user confirmed game/loader closed; process check found
neither running. Backed up previous DLL under the local loader backups directory
before-rev23-20260927-105812, then installed rev.23 in global plugins. Deployed
SHA256 matches the final candidate 98799D064172162057924CA700BBC4E270FA395DF193E81846943BB387296678.
Live tab behavior and diagnostic action capture await the next launch; lower
Gold/Potions focus navigation remains unresolved. No release was published.

## Rev.24 focus handoff candidate

User confirmed rev.23 Equipment/Items LB/RB works, but no downward control enters
the list. New-process action capture contains 13 (repeated Down attempts), 16
(other attempted stick navigation), and bumpers 19/20; no native section action
22 was observed. Do not globally reinterpret action 16 or other stick actions.
Fresh live tree and focus reads show RuleDetails owns FocusManager+0x188 and
ItemListTabs owns +0x190. Both filter panels are active. Game+0x846020 confirms
FocusManager is obtained from *(game+0x3440170)+0xD0; live read agrees with the
named FocusManager widget. All recorded pointers were reacquired this process.

Candidate translates action 13 only on the exact visible ItemListTabs widget
with the exact RuleDetails parent, controller mode, normal submenu admission,
and both observed focus ownership fields still matching. It sends a borrowed
copy with action 22 to the existing RuleDetails Core message wrapper +0x2C1840.
The original event and raw controller state are never edited. Nested action 22
is not translated again. Native code chooses the lower section and focus; no
focus-manager fields are written by QOL. Original Down processing is retained
when the section action does not change the observed focus fields. Success of
that focus comparison does not prove lower widgets are editable.

Admission: exact current CoreHash from native_input_profile.h, live parent
vtable Core+0x7B13D0, message slot Core+0x7B13F0 -> Core+0x2C1840, original
Core+0x7B1350 -> game+0x14AFF80. Full live byte witnesses in
src/filter_focus_signatures.h cover game+0x846020 (0x36), game+0x14AF750
(0x2D3), game+0x14AF520 (0x68), Core+0x2C1840 (0xB4); checked on each handoff.
Changed contracts disable only this focus extension. No new hook, vtable/slot
patch, remote native call, or input simulation was installed during research.
Native resolver/type checks and provider message wrapper are preserved.

Rev.24 builds and all 19 suites pass, including scoped direction policy and
rejecting nested action 22. Not yet installed or live-tested. Native resolution
can still refuse the transition; bounded focus-result logging distinguishes
that from the missing action. Actual Gold/Potions navigation remains pending.
Candidate SHA256: 2C832F326B40766B37885217AE46993FE3DBE7D8BDC0F1E3A4F61151715C1A81

Rev.24 deployment 2026-09-27: user confirmed closure; no D2R/D2RLoader process
was running. Previous deployed DLL backed up under local loader backups
before-rev24-20260927-110458. Global plugins DLL now reports 1.3.1+rev.24 and
matches tested SHA256 2C832F326B40766B37885217AE46993FE3DBE7D8BDC0F1E3A4F61151715C1A81.
Awaiting user validation of Down into Gold/Potions, lower controls, and return
navigation. No release published.

## Rev.24 live failure and rev.25 merged-panel candidate

User reports no lower navigation. Logs prove the admitted section action 22 was
sent and returned with unchanged focus (eight bounded samples). Thus rev.24
was not merely blocked by admission. Fresh process inspection found native
selector table game+0x3BBD308 / bucket count +0x3BBD300 contains
LootFilterEquipmentPanel (id 0) and LootFilterMiscItemPanel (id 1), while the
active named panel tree contains the merged LootFilterItemPanel instead. Fresh
RuleDetails+0x3FC was 1. One earlier probe produced invalid stale-object values;
those values were rejected and the panel tree reacquired. This supports a
legacy name-resolution mismatch, not a breakpoint-proven complete call trace.

Core+0x2C0F00 reveals a four-entry registry at Core+0x7B0ED0, stride 24:
+0 outer item panel, +8 misc controller helper, +16 unclassified (zero here).
The live populated pair is LootFilterItemPanel (Core vtable+0x7B10B0) and
LootFilterItemPanelMiscController (game vtable+0x1FD6098). Both +0x168 reference
the same current RuleDetails; +0x170 the same rule model; +0x178 the same ItemList.
Helper +0x188/+0x190/+0x198/+0x1A0/+0x1A8 reference FilterGoldToggle, GoldSlider,
SliderText, GoldDescription, GoldSliderWrapper respectively. This corrects the
previous uncertainty: the register RBX in the wrapper is the helper, not outer
panel. It forwards certain controls to that helper while retaining the original
outer message function for other input. No registry/handler replacement made.

ItemList has Core vtable+0x7B1240, parent +0x30 outer panel, active/visible 1/1,
and nonnull selected scroll view +0x320. Native focus entry game+0x14ABC60
(void fastcall(panel)) calls +0x858050 then +0x14AA6D0(panel+0x178).
The latter, when list+0x320 is nonnull, calls +0x8D0A50(list,0,0,2) then tail
calls +0x14A9B10(selected scroll view). This provides a native entry path with
no manual focus-field writes. Visible behavior remains unverified.

Rev.25 first preserves rev.24's existing native section attempt, then if focus
is unchanged checks the registered outer/helper pair, exact vtables, rule/model
identity, shared list identity, list visibility/parent/selection and unchanged
outer message slot before calling native entry on the outer panel. It does not
call entry on the helper or replace its input handler. It remains limited to
Down on the focused visible rule tabs in controller mode. Added live byte guards:
game+0x14ABC60 (0x2A), game+0x14AA6D0 (0x3A), Core+0x2C0F00 (0x1F3), stored in
filter_focus_signatures.h. Existing exact provider hash remains required.

Rev.25 Release build and all 19 existing suites pass. These cover build/artifact
and action policy, not real native list interaction. Candidate is not installed;
Gold/Potions editing and returning from the list require user testing. No release.
Rev.25 candidate SHA256: 5AF649FFE05525664997DC2FC176D54B4C1E47EEFFB56A6839D6073B1D4B12F2

## Rev.27 Equipment-to-Items focus repair

User confirmed the merged-panel lower navigation in installed rev.26 works.
Remaining reproducible issue: with focus inside Equipment, RB switches to Items
but the cursor disappears until leaving the panels; reverse direction works.
Read-only live inspection after reproduction: RuleDetails+0x3FC=1; merged outer
panel active/visible; ItemList+0x320 resolves TopScrollViewItemsMisc active/visible.
FocusManager+0x188/+0x198 still point to that merged panel, but +0x190 is null.
Thus the observed state is missing child focus, not a hidden selected item view.

Rev.27 captures RuleDetails+0x3FC before a scoped bumper tab translation and
checks after original TabBar processing. Only a 0->1 transition can repair, only
when the merged panel still owns focus and child focus remains null. It reuses
the same guarded native +0x14ABC60 list-entry routine used by Down. Existing
registry/model/list/vtable/byte guards remain. No new native address, hook,
focus-field write or healthy-focus override. Reverse, unchanged, unknown and
healthy-focus transitions excluded in policy tests. A delayed native selection
update outside the original handler could still require a later UI callback;
current candidate must be live-tested rather than assuming synchronous success.

Rev.27 Release build and all 19 suites pass; not yet deployed or live-tested.
SHA256 63BDF020E3A6FA067B994A9ED94EFE4B679DBD26A3121E87C67E35E3BEDBC696.

Rev.27 deployment 2026-09-27: user confirmed closure; process check found no
D2R/D2RLoader process. Backed up previous global DLL under local loader backups
before-rev27-20260927-120034. Installed DLL reports 1.3.1+rev.27; deployed SHA256
63BDF020E3A6FA067B994A9ED94EFE4B679DBD26A3121E87C67E35E3BEDBC696
matches the tested candidate. Both tab directions and continued list navigation
await live validation. No production release published.

## Rev.28 deferred bidirectional focus repair

Rev.27 was installed but failed user testing. The earlier one-direction description
was incomplete: Equipment Rarity/Quality -> Items loses focus, while Equipment
Armor/Weapons/Accessories -> Items works. Expanded Misc children -> Equipment
also lose focus, while Misc root -> Equipment works.

Read-only live capture on 2026-09-27, same admitted provider hash as above:
before RB, FocusManager+0x190 pointed to NormalRarityChbx under RarityContainer,
TopScrollViewItemsEquipment, ItemList and the merged LootFilterItemPanel.
After RB, the same merged panel remained at focus+0x188/+0x198, but +0x190 was
null. ItemList+0x320 selected the visible TopScrollViewItemsMisc; Equipment was
hidden. User then reproduced expanded Scrolls and Tomes -> Equipment with LB:
again +0x190 was null, the same merged panel owned focus, and +0x320 selected
visible TopScrollViewItemsEquipment. Thus both observed failures share missing
child focus. Captures do not establish exactly when it clears inside the update.

Additional static path detail: game+0x14A9B10 loads the selected view's +0x880
control and, if nonnull, tail-calls game+0x876C80. That routine loads control+0x544
into RDX and dispatches control vtable+0xA0. Live +0x880 destinations were
NormalRarityChbx for Equipment and FilterGoldToggle for Misc. These are research
findings, not new directly called or patched entry points. Existing guarded
panel entry game+0x14ABC60 already traverses this path.

Rev.28 replaces the immediate one-way repair with SDK runOnUiThread callbacks
in both directions, bounded to eight callbacks and 250 ms. Capture requires
lower merged-panel focus ownership. Each callback reacquires the current
registered panel and tabs, checks panel/parent/rule-model identity, controller
mode and active editor, then reuses guarded native entry only with null child
focus and changed valid tab selection. Healthy focus is preserved. Generation
invalidation cancels superseded/cleanup work; callbacks never invoke a cached
widget pointer. No new inline hook or raw focus-field write is introduced.

Release build and all 19 suites pass, including both-direction policy checks.
These tests do not prove native visible focus recovery. Candidate not installed
or live validated yet. SHA256:
74B3553844C89723CDC813AA925060D7E6BFCADCC9EA8EEC91D8A8236A41E44E.

Rev.28 deployed 2026-09-27 after user closure and process verification.
Previous global DLL backed up under before-rev28-20260927-121439.
Installed ProductVersion 1.3.1+rev.28 and SHA256
74B3553844C89723CDC813AA925060D7E6BFCADCC9EA8EEC91D8A8236A41E44E
match the tested candidate. Visible recovery in both failing transitions remains
pending user validation. No production release published.

User reported rev.28 is looking good after installation on 2026-09-27. This is user-confirmed visible improvement; no independent capture of every transition was made.
