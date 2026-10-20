# Native tome-use candidate - rev.26

## Scope and validation boundary

User authorized replacing inventory LB+A tome identification with native hold-A
use. Initial candidate requires source tome and target in main Inventory; Cube,
stash and loose-scroll paths retain existing implementation. Explicit configured
no-consumption/free-identify behavior is unchanged. No bulk identification added.
Exact provider SHA256 is the existing native_input_profile.h CoreHash:
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
No assumption that Korean-client identity/layout matches this local build.

Read-only live evidence: native inventory open, grid type game+0x1CF3800.
User held A on tome and left targeting active: cursor state 5 and nonnull source
pointer observed. User completed native identification and confirmed 95 -> 94
charges and target identified. This validates normal game behavior, NOT QOL's
new adapter. All 19 automated suites pass; installed/live candidate test pending.

## Recovered path and witnesses

- Inventory root PlayerInventoryExpansionLayout, child grid; original-layout
  root fallback retained, but exact grid type still required.
- Grid message game+0x2C6BD0 forwards base item handler +0x2A98B0.
- At +0x2AA348 action-0 held/repeated payload +0x19 branch calls virtual +0x118
  with (grid, selected item); current slot is game+0x2AA7E0.
- +0x2AA7E0 is the native hold action, void fastcall(grid,item). Its usability
  checks lead through +0x159DD0 / +0x36E360 and +0x15C600 for applicable items.
  These subordinate candidates are static call-chain evidence, not separate
  entry points used by QOL.
- Native cursor getter +0x14F1E0 reads game+0x2A88700. Mode getter +0x14F210
  reads game+0x2A88708. No writes to these globals by QOL.
- Find panel +0x846170, find child +0x856220, grid owner +0x2A7810.
- Grid lookup +0x2C49F0 (grid, Cell{x,y}*) equals virtual +0xC8. Native item
  type +0 and runtime ID +8 checked against fresh authoritative SDK identities.
  Item code getter +0x36EF50 checked too; exact x/y resolves the current client
  object anew, never a cached native item pointer between callbacks.
- Grid virtual +0x100 is thunk game+0x3E2B274 -> slot game+0x3E2A7D8 ->
  Core+0x8150A0. ABI void fastcall(grid, packed int32 x/y). Core wrapper calls
  ItemInteraction listeners then custom-page policy and native activation
  through Core+0x70E9C8 (game+0x2C7540). Lookup Core+0x70E968 -> game+0x2C49F0.
  Route left intact so other plugins retain interception. QOL alone bypasses its
  own LB+A recursion using a synchronous thread-local forwarding flag.
- Native activation +0x2C7540 recognizes cursor mode5, source from +0x14F1E0,
  and grid target lookup, retaining game validation and native submission.
- Item identified check +0x36E2D0(item,0x10). Charge stat getter +0x2F5020
  uses stat70 layer0; exact existing rev.15 Core slot/wrapper/body guards reused.
- Reference mass-identify plugin names game+0x1C7A30 targeting worker and
  +0x46E8C0 identify/+0x46F090 quantity routines. These were research leads;
  QOL does not call/hook them, copy its mutation path, or construct packets.

native_identify_profile.h records live bytes for called game entry points.
Guard requires exact provider hash, stat getter chain, current vtable routes,
Core lookup/activation predecessor slots, visible inventory, native owner,
source/target identity and positive native charges. Unsupported admission refuses
the inventory-tome request; it never tries SDK mutation after native refusal.
No new inline hook, provider slot patch, raw button injection, raw focus write,
raw charge/identified flag write or OS input simulation.

## Lifecycle and verification

Existing authoritative scan finds target/tome using stat70, then queues UI work
through ThreadService. UI callback invokes hold once, waits for mode5 belonging
to that exact tome, activates original target once through existing virtual
route, then waits for client identified flag and exactly one fewer charge.
An authoritative game callback subsequently checks exact target identity/state
and tome stat70, reporting confirmed/unconfirmed separately. Dispatch is pending,
not success, and never sets consumed=true preemptively. Existing tests cover zero
SDK quantity/native97 dispatch without SDK mutation and refusal without fallback.

One pending request, generation-tagged callbacks, two-second/240-poll UI bound,
owner/identity checks each step and shutdown invalidation. SDK schedules UI work
on later root updates and authoritative checks on the game thread. No retained
native item pointers. Timeout or changed target/cursor does not retry or use the
SDK fallback. Native targeting, if still active, is left for the player to cancel;
no unverified cursor-reset routine is called. Changed session can discard the
verification task; next request expires that pending state after 2.5 seconds.

Testing still required: native LB+A inventory success/exact one charge; repeated
presses; empty/last-charge tome; already-identified target; foreign cursor;
inventory closing mid-sequence; other item-interaction plugins; actual reporter
build admission. Cube/stash native expansion is subsequent scope. Rev.25 filter
focus candidate remains included but unvalidated; do not publish as production.
Candidate SHA256: CD775F3895A2C3ED31097214AD627D861F7A00050F7B47877FE36C52A9BE9470

Deployment 2026-09-27: user confirmed closure; process check found no game or
loader running. Previous global DLL backed up in local loader backups under
before-rev26-20260927-115056. Installed global DLL reports 1.3.1+rev.26 and SHA256
CD775F3895A2C3ED31097214AD627D861F7A00050F7B47877FE36C52A9BE9470,
matching the tested candidate. Native LB+A adapter validation remains pending.
No GitHub release published.

Live validation follow-up: user confirmed native LB+A works, with visible native
identification cursor transition. Local log confirms authoritative=confirmed,
charges=94->93, target runtimeId130, no SDK quantity/identify edits. Reporter
Korean-client compatibility remains untested; do not infer cross-build support.

## Remote rev.26 report reviewed 2026-09-27

Reporter log timestamps 2026-09-28 01:02 show plugin rev.26 loaded. Five
Identify attempts report operation=3, consumed=0, SDK quantity zero, stat70
100 or 2, and no native pending/IdentifyNative stage messages. Rev.26's
inventory native-dispatch refusal is mapped to the same MutationFailed /
SDK-operation-failed label and Unavailable (3) used by SDK failures. Therefore
these lines no longer prove that an SDK quantity edit was attempted or failed.
For inventory source and target, a failed native guard, busy request or rejected
UI scheduling can produce these lines before native hold-A dispatch. Container
fields and specific admission failure are absent from this log, so the exact
branch cannot be proven. A separate portal-profile mismatch is present but does
not prove the identify guard failed for the same reason. No plugin culprit or
Korean localization cause is established. Next diagnostic should distinguish
route, admission/hash/byte/slot checks and UI scheduling failure without relaxing
native compatibility guards or identifying without charge consumption.

## Rev.29 diagnostic candidate

Adds first-failure module/RVA/byte-offset/expected/actual diagnostics to existing
native byte guards and expected/actual pointer diagnostics to existing slot
checks, including inventory vtable action routing. Admission reports actual and
expected on-disk Core SHA256 (or unreadable), required services/schedulers and
acceptance. No additional native address is used, and guards are not relaxed.
Hash helper optionally returns digest/readability without changing its match
result; hashing remains at initialization rather than per identify request.

Request refusal distinguishes admission, context, pending work and initial UI
scheduler result. Identify summary records native/SDK route and source/target
container values. Native rejection now uses native-request-refused instead of
SDK-operation-failed; operation=3 remains Unavailable. Startup/request diagnostics
are enabled regardless of debug_logging, with no per-item scanning log dump.

Release build and all 19 suites pass, including unreadable/mismatched/matched
hash reporting and native-refusal no-SDK-fallback coverage. Remote and local live
behavior for this diagnostic is untested. Packaged DLL, instructions and checksum
only; no TOML override. Local installed rev.28 unchanged; no production release.
DLL SHA256 FE6DCC9EDEE8EDB7630FBF8CC85B087B6D4C7052C7B362F49055CB40352E9083.
ZIP SHA256 17CAABDB0D1B757F2AECBD4413EC71B4073F2063C37AC58D60C0A730E1AFCA16.

## Rev.29 remote result: Cube source selects SDK route

Reporter log timestamps 2026-09-28 02:00-02:01: actual Core SHA256 equals
expected, all required services/schedulers exist, native admission accepted.
Every recorded failed identify uses route=sdk-consumable targetContainer=4
(Inventory) sourceContainer=5 (Cube), operation=3, consumed=0. Thus the earlier
leading admission-failure hypothesis is disproven for this run. Native hold-A
is scoped to inventory source and target, so no native request was made here.
SDK quantity edit remains the failing operation on the selected Cube tome.
Two tomes are counted; the log does not locate the other tome. Scan::Visit
selects the first positive-charge tome and stops after target+tome are known;
it does not prefer Inventory over Cube. A Cube tome can therefore win even
if an eligible inventory tome exists later in enumeration. Next isolated test:
place all Identify tomes in main inventory and retry with the same diagnostic.
A follow-up selection change should prefer charged inventory tomes, with an
order-independent regression; native Cube use requires separate validation.
Portal mismatch and materials RVA 0x3862D0 mismatch are independent diagnostics,
not evidence of the cause of these identify attempts. No plugin culprit proven.

## Rev.30 work in progress: source-container policy

User requested excluding Cube tomes, and clarified both source tomes and targets
should remain usable in Personal/Shared Stash. First incremental change excludes
Cube tomes in Visit while retaining Cube target identity/loose-scroll behavior.
Existing stash-target SDK path remains. All 19 suites pass with Cube-first,
Cube-only, empty-inventory-plus-charged-Cube, and stash-target regressions.
This is NOT complete stash-source support: existing native tome use resolves
only main-inventory grid and SDK search previously never accepted stash tomes.
Need validate native Personal/Shared source and target grids, identities and
charge confirmation before extending scope. Awaiting live Personal Stash state.
No rev.30 deployment or distribution performed.

### Personal Stash grid observation (rev.30 investigation)

2026-09-27 read-only live inspection with user-reported Personal Stash tome and
inventory unidentified target: active BankExpansionLayout -> basicstash_container
-> grid, and active PlayerInventoryExpansionLayout -> grid, both have game
vtable RVA 0x1CF3800. Vtable +0x58=game+0x2C4DF0, +0xC8=game+0x2C49F0,
+0x100=game+0x3E2B274, +0x118=game+0x2AA7E0. Thus grid lookup/activation/hold
entries match the previously observed inventory grid. Cursor global +0x2A88700
was null; mode +0x2A88708 was 7. These are structural observations only: no
function remotely called, item moved, or identification performed. Awaiting user
native hold-A on stash tome to verify resulting targeting state before extending
production routing. Shared-page source binding remains unverified.

User native hold-A on the Personal Stash tome changed cursor mode from 7 to 5
with a nonnull cursor item (read-only capture). This matches the main-inventory
native identification mode. Pending completion/one-charge confirmation and
Shared Stash comparison; no automated stash identification is enabled yet.
Personal Stash completion confirmed explicitly by user: tome charges 90 -> 89
and native identification completed. User clarified the current screen is still
Personal Stash, not Shared. Do not treat prior generic Done as Shared preparation
or validation. Shared test remains pending.

### Rev.30 final candidate scope

User corrected Shared behavior after testing: Shared Stash blocks native tome
use. Source policy now allows only Inventory and currently open Personal Stash;
Cube/Shared tomes are excluded. Inventory charged tome has priority regardless
of enumeration order. Enumeration continues past Personal candidates until an
inventory candidate and exact target are found or enumeration completes.
Loose-scroll behavior remains unchanged. Shared target identification retains
its previous SDK route; this change excludes Shared SOURCE tomes.

Native personal support resolves source/target grids independently using guarded
existing FindPanel/FindChild. Personal grid hierarchy is BankExpansionLayout /
basicstash_container / grid; Bank vtable game+0x1CE4A20, grid+0x1CF3800.
Reuses guarded SelectedTab game+0x23AF50 with existing SharedPageNative signature,
requiring tab 0 and visible/active ancestors. UI action captures availability;
each asynchronous callback reacquires and checks it again. Source/target owner
must match and remain stable. Runtime IDs, codes and cells are rechecked. Native
hold-A uses source grid; activation uses target grid. Both have the same guarded
vtable and slots. No new hook, charge write, or automatic inventory transfer.

All 19 suites pass, including Cube-first/Cube-only, empty-inventory plus charged
Cube, Shared source rejection, closed Personal exclusion, open Personal source
native dispatch, inventory priority, and Personal-vs-Shared target routing.
User's native Personal baseline consumed exactly one charge (90->89); automated
QOL Personal execution is not yet live validated. Rev.30 candidate SHA256:
C83B3222FB259B0479AE0D8BF0EDBA067C41C0F213B0720BD901DB74334F4483
No candidate installed yet.

Rev.30 deployed 2026-09-27 after user closure and process verification.
Previous installed rev.28 backed up under before-rev30-20260927-132736.
Installed ProductVersion 1.3.1+rev.30 and SHA256
C83B3222FB259B0479AE0D8BF0EDBA067C41C0F213B0720BD901DB74334F4483
match the tested candidate. No configuration changed. QOL native Personal Stash
identification, inventory preference and excluded source behavior await live
validation. No production release published.

Rev.30 user validation: user withdrew the failure report and confirmed the
Personal Stash identification works; the unsuccessful interaction was a long
hold of A rather than the intended LB+A shortcut. Do not classify that report as
a native identification failure. This confirms user-visible Personal Stash QOL
behavior; exact rev.30 charge delta, inventory priority and excluded-source
live tests were not separately reported. Temporary diagnostic debug_logging was
restored to false in the active config after this confirmation.

## Rev.31 Identify All candidate

LB+A on an inventory/Personal Stash Identify Tome queues a bounded main-inventory
snapshot through SDK game-thread scheduling. Binds the exact highlighted tome,
not automatic supply selection. Snapshot resolves its authoritative identity and
collects at most 256 distinct unidentified inventory targets; overflow or partial
enumeration refuses all work. Cube/Shared sources and all non-inventory batch
targets are excluded. Controller footer/header expose Identify All only for
eligible tome containers, alongside existing shortcuts. quick_identify controls
the action. Bulk always uses native charges even if single-item consumption is
configured off; no free bulk fallback was added.

Reuses existing native hold-A/target activation and guards; no new native RVAs,
hooks, memory writes or packet construction. Each item waits for client state
and game-thread authoritative confirmation of identified state plus exactly one
charge debit before scheduling the next. Fresh getItemInfo/Same and stat70 reads
revalidate targets/source. Moved/missing source or target stops remaining work;
already identified targets skip. Never select another tome or add new targets.
Required UI closure/tab switch, mode switch, foreign cursor, failed confirmation,
empty tome, scheduler refusal or per-item deadline cancels. Batch deadline 60s;
per-item native deadline remains 2s. Generation and busy guards prevent duplicate
batches and callbacks after shutdown. Existing single-item overlap now refuses
rather than taking over a pending native operation after 2.5s.

Release build/all 19 suites pass. Bulk regression coverage: source eligibility,
exact selected tome, inventory-only snapshot, deduplication, capacity rejection,
empty tome, moved/replaced/missing target and externally identified skip policy.
Tests do not execute the live asynchronous native sequence. Candidate live tests
needed: several items/charge delta, fewer charges than targets, empty tome,
Personal Stash source, UI closure and ordinary single-item LB+A regression.
Rev.31 candidate SHA256: D7F7775864F1F649ABA3BC1181569DCC525E95269512EA454D7742D962D296E0. Not installed or live validated yet.

Rev.31 deployed 2026-09-27 after user closure and process verification.
Previous installed rev.30 backed up under before-rev31-20260927-134520.
Installed ProductVersion 1.3.1+rev.31 and SHA256
D7F7775864F1F649ABA3BC1181569DCC525E95269512EA454D7742D962D296E0
match the tested candidate. Configuration unchanged. Identify All native sequence
and charge-count test remain pending user validation; no production release.

## Rev.31 failure / rev.32 pacing experiment

Local log 2026-09-27 13:58:19: requested batch, first native action, authoritative
charges 89->88 target127 at .968, second hold-A dispatched at .979 (11ms later),
timeout at 13:58:21.977. Completed1/planned6. User confirms six real unidentified
items and no cursor remaining. Read-only subsequent cursor capture was null,
mode7. Therefore snapshot length is correct; original diagnostics cannot prove
whether second attempt stalled before target mode or after target activation.
User should hold modifier and tap A once, then release; no continued hold needed.

Live disassembly of existing hold handler corroborates early call game+0x1C7360
and branch to exit on nonzero. +0x1C7360 calls +0x2F5810(item,2), and if null
checks local-player state via +0x3351B0(player,0x36); nonzero returns1. Meaning of
that state and whether it rejected this call is unproven. Additional inspected
+0x15C600 starts a larger item-use dispatch; not adopted as a direct entry.
No new native calls or guard dependencies introduced from this research.

Rev.32 is a pacing experiment, NOT a confirmed fix: after confirmed prior item,
wait >=250ms before next hold-A using SDK UI callbacks, never sleeping the UI.
Original one-dispatch/no-retry and charge confirmation remain. First item starts
immediately; no pacing imposed after dispatch. Added next-target id/code/cell,
mode-observed activation marker, and timeout phase/elapsed/poll diagnostics.
All19 suites pass including pacing boundaries. Pending live candidate test.
Rev.32 SHA256 E99A06E52AC24EC13F2953CA81BE18C5C0F7FFC3DF47109FC2D37C6A86F1B83C; not installed yet.

Rev.32 deployed 2026-09-27 after user closure and process verification.
Previous installed rev.31 backed up under before-rev32-20260927-140250.
Installed ProductVersion 1.3.1+rev.32 and SHA256
E99A06E52AC24EC13F2953CA81BE18C5C0F7FFC3DF47109FC2D37C6A86F1B83C
match the tested candidate. Configuration unchanged. Multi-item native sequence
and charge-count validation remain pending; no production release.

## Rev.32 live failure / continuation investigation

At 14:03:38 snapshot planned5; target129 confirmed charges88->87. Next target130
was scheduled with charges87, hold-A dispatched after ~250ms; timeout phase1,
2015ms/240polls. No Targeting mode observed marker for second item. User then
repeated physical LB+A without closing inventory at 14:05:26: target130 succeeded
87->86, next132 again expired phase1. Thus same previously failed target works
on a fresh physical request. Pacing did not resolve missing second targeting
mode; not evidence of target eligibility or quantity-edit failure.

Read-only live inspection: existing hold handler game+0x2AA7E0 branches through
+0x77E10 controller-mode query (already admitted for label recovery), condition
+0x159D20 and +0x159DD0. +0x159DD0 queries item-use information via +0x36E360 and
+0x388430 and UI-mode0x16 via +0xCE500. +0x2CA5F0 returns a cached UI descriptor
used to check the grid's type ancestry before item-use branches. +0x15C220 and
+0x15C600 are larger downstream item-use dispatches; inspected only, not new
call targets. Exact rejecting branch remains unproven. No additional native
mutation or guard relaxation made. Pending physical-modifier-held comparison.

## Rev.33 bounded eligibility diagnostic

User keeping only LB held still identified one item; modifier release hypothesis
not supported. Rev.33 records pre-hold nativeBlocked/nativeUsable, target/tome
runtime IDs, completed count and native charge count, then post-hold cursor/mode.
Calls two predicates already used by the native hold handler: int fastcall(item)
game+0x1C7360 (blocked; full0x46 byte guard), bool fastcall(item) game+0x159DD0
(usable; full0x86 byte guard). Both byte windows captured live from PID42136 and
stored in identify_probe_profile.h. Invoked on UI thread only for bulk, after
existing core admission and exact source identity checks, once per attempted
hold. Predicates query native eligibility; no forced result or state write.
A predicate byte mismatch skips supplemental diagnostics, retaining prior guard
behavior. No new hook or forced use path. This build diagnoses, not claims to fix,
the second-target phase1 timeout. All19 suites pass; live diagnostic pending.
Rev.33 candidate SHA256: 3EC31009FB0018D4BA2B3FB690A1EDFC5B05E339567FE12B58AE9FF4C0195FC7. Not installed yet.

Rev.33 deployed 2026-09-27 after user closure and process verification.
Previous installed rev.32 backed up under before-rev33-20260927-141040.
Installed ProductVersion 1.3.1+rev.33 and SHA256
3EC31009FB0018D4BA2B3FB690A1EDFC5B05E339567FE12B58AE9FF4C0195FC7
match the tested diagnostic candidate. Configuration unchanged. Awaiting live
bulk reproduction for native eligibility/cursor comparison; no release.

## Rev.33 result / rev.34 readiness gating

Live log 14:11:35.286 first pre-hold: target130 tome135 completed0, blocked0,
usable0, mode7, charges85. Post-hold cursor nonnull/mode5, authoritative85->84.
Second pre-hold target132 same tome135 completed1: blocked1, usable0, mode7,
charges84. Post-hold null/mode7, timeout phase1. Thus game+0x1C7360 is the observed
early rejection gate on second use. The other predicate was0 on both first
success and second failure; do not describe its label as decisive eligibility.
Its supplemental invocation is removed from rev.34. Exact blocked subcondition
(item stat-list predicate versus local-player state0x36) and reset duration are
not established by these logs.

Rev.34 replaces speculative250ms pause with bounded UI-thread polling of guarded
+0x1C7360 before each bulk use. When blocked, issue NO hold-A action. On zero,
start existing native sequence exactly once. Wait up to5s/1200 callbacks, then
cancel; dispatch resets ordinary2s/240poll confirmation window. Existing source,
target, owner, tab, mode and cursor checks remain on each callback. No raw input
or blocked-state writes, no bypass or retry after a dispatched action. Logs once
when waiting and once ready; no per-poll dump. All19 suites pass including ready,
blocked, deadline and release-before-deadline policy checks. Live batch success
pending. No new hook; existing rev.33 Blocked witness is now mandatory for bulk
readiness checks (single-item behavior unaffected).
Rev.34 candidate SHA256: C7FE7B2876D401F93B80E62ED8218C483D3C1A73EF97F37A7623386A3D5C60A9. Not installed yet.

Rev.34 deployed 2026-09-27 after user closure and process verification.
Previous installed rev.33 backed up under before-rev34-20260927-141408.
Installed ProductVersion 1.3.1+rev.34 and SHA256
C7FE7B2876D401F93B80E62ED8218C483D3C1A73EF97F37A7623386A3D5C60A9
match the tested candidate. Configuration unchanged. Native-readiness batch
continuation and charge-count test remain pending; no production release.

Rev.34 live success: user confirms Identify All works. Local 14:15:10-13 log
confirms six planned/six completed, authoritative charges84->78. Successive
confirmations ~0.47-0.48s apart; readiness waits ~0.37-0.38s dominate, plus dispatch
and logging/confirmation overhead. User notes this is too slow and asks about
returning to SDK identification. Cube source selection is established for the
remote diagnostic failures, but no remote SDK test with an inventory tome has
yet established that source restriction alone resolves SDK quantity rejection.
Consider SDK-first with verified charge handling and native fallback only after
proven no mutation, or an explicit native mode; never auto-fallback after partial
mutation/uncertain readback. No identification route changed in this discussion.


## Rev.35 SDK-first candidate

User confirmed rev.34 completes the native batch but finds it slow. Rev.35 restores
SDK edits by default for single and bulk identification; `native_identify = true`
selects the preserved native path explicitly. This does not prove the remote
Korean tester's SDK write works after excluding Cube tomes; retain that limitation.

Single and bulk share `QolIdentify::ConsumeTome`: read stat 70, SDK absolute Quantity
edit to N-1, stat-70 readback, SDK Identified edit, and verified quantity restoration
if that edit is rejected. Failed/uncertain debit stops without native fallback.
Batch keeps one authoritative snapshot, binds the exact highlighted tome, reacquires
player/source/target each step, verifies target identified after edit, and queues
one item per game callback (SDK threads.h specifies later authoritative updates).
No native hold action, cursor transition, blocked-tome wait, or new RVA is used by
this SDK branch. Existing stat-70 native read guards and Personal UI admission remain.
All native compatibility addresses/guards above remain unchanged.

Removed the spatial leap callback, occupancy scan, synthetic D-pad pulse engine,
and its inferred coordinate tracking. Actual tooltip item/container identity stays
available for transfers. Both native and XInput readers use the same simplified
shortcut filtering; physical directional input passes through while L1 is held.

Added six-target/six-charge SDK regression, final-charge exhaustion, rejected debit
and failed-identification compensation checks, and modifier directional passthrough.
Live rev.35 speed, visual refresh, and Personal Stash behavior are pending.

Rev.35 automated validation: Release build and all 19 CTest suites passed, including added SDK batch and directional passthrough regressions. DLL SHA256 8D7ED74B09CE5D83C4C23F617C19305785D045E99616A9A1F84FF71BC3311B08. Installed in the global plugin folder after verifying both game and loader closed; prior DLL and configuration backed up under before-rev35. Installed hash matched. Active native_identify=false; debug_logging=false. Live validation pending.


## Rev.36 Cube bulk targets

User confirmed rev.35 works nicely. Extend only SDK bulk target selection through
public InventoryService enumeration: BulkPlan::TargetMask adds ItemContainer::Cube
when native_identify=false. Observe uses that same mask; source selection remains
unchanged (inventory/open Personal Stash only). Snapshot deduplicates runtime IDs,
rechecks container and cell before each edit, and rejects moved/replaced items.
Native mode cannot resolve a closed Cube grid and retains inventory-only scope;
no additional native address, hook, memory write, or cursor routing was introduced.
Mixed inventory/Cube charge tests and source/target exclusion regressions added.
Rev.36 live Cube editing, display refresh and persistence still require testing.

Rev.36 Release build and all 19 CTest suites passed. Candidate DLL SHA256: 6BF4484349D045CADFE30CA01E162E07062A36CEEDC0597622E80F5472FA6AB2. Deployment pending game/loader closure; live Cube verification pending.

Rev.36 deployed after user closure confirmation and process verification. Previous DLL and configuration saved to d2rloader/backups/before-rev36-20260927-143612. Installed ProductVersion 1.3.1+rev.36 and SHA256 6BF4484349D045CADFE30CA01E162E07062A36CEEDC0597622E80F5472FA6AB2 match the tested candidate. Configuration preserved: native_identify=false, debug_logging=false. In-game mixed inventory/Cube identification and charge verification remain pending.
