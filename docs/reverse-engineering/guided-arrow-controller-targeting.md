# Guided Arrow controller targeting candidate

Investigation date: 2026-09-27. This is a cursory static/data investigation,
not a live-game validation or a reconstructed native ABI.

## Target build and artifacts

| Artifact | Version | Size | SHA-256 |
| --- | --- | ---: | --- |
| Installed `D2R.exe` | `3.3.93787` | 32,107,216 | `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7` |
| Installed `D2RCore.dll` | `1.3.1-beta` | 18,938,712 | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Reimagined `controllerskillsettings.json` | data file | 324,766 | `4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79` |

The controller file hash and Guided Arrow record were identical in the source
checkout, installed Reimagined and Reimagined Ladder mods, retained Reimagined
backup, and two ladder-bundle backups examined during this pass.

## Data path and candidate trigger

The installed/source path
`data/hd/global/excel/controllerskillsettings.json` contains the controller
record for Guided Arrow (`name=Guided_Arrow`, `id=22`). Its relevant fields are:

```text
usageType=0
targetGroup=0
defaultTargetDistance=2.0
acceptableDistanceOffset=6.0
usePredictiveTargeting=true
alwaysIgnoreTarget=false
ignoreTargetOnHold=false
useStickyFreeTarget=false
validationFlags=0
```

This is the strongest initial candidate for the keyboard/mouse versus controller
difference. The controller path is allowed to acquire and preserve a predicted
unit target. Guided Arrow has no apparent data-level exception telling that path
to ignore the acquired unit and cast toward a free point. Magic, Fire, Cold,
Exploding, Ice, and Freezing Arrow use the same relevant controller pattern;
Multiple Shot and Strafe differ by disabling predictive targeting.

The candidate interpretation is: a mouse cast toward empty ground can create a
targetless Guided Arrow that enters its missile search behavior, while controller
aim supplies a unit target up front. This interpretation is not yet proven by a
runtime trace of the cast request or missile state.

## Separate missile retarget path

The installed/source `data/global/excel/skills.txt` row for Guided Arrow is skill
ID 22. It selects `guidedarrow` for its server and client missiles and uses server
start/do functions 4/10 and client start/do functions 11/18.

The `data/global/excel/missiles.txt` row for `guidedarrow` is missile ID 86 and
contains a distinct periodic retarget contract:

```text
pSrvDoFunc=7
pCltDoFunc=7
Param1=1       # Periodic Delay until Retarget
Param2=15      # Radius to search for Retarget
CltParam1=1    # client periodic delay
CltParam2=15
```

This establishes that controller target selection and missile retargeting are
separate data-driven stages. It does not establish the native function RVAs,
the unit/point target representation, or the precise condition under which
missile do-function 7 performs its first search.

## Native string witness

The installed `D2R.exe` contains controller schema strings at these PE file
offsets (not RVAs and not function addresses):

| String | File offset |
| --- | ---: |
| `alwaysIgnoreTarget` | `0x170AC90` |
| `ignoreTargetOnHold` | `0x170ACA8` |
| `useStickyFreeTarget` | `0x170AD80` |

These witness that the executable recognizes the controller targeting fields.
A bounded scan did not establish their parsing or consumption functions, so no
native RVA is claimed.

## Smallest useful live test

Test a data-only override for skill ID 22 before attempting a native hook:

1. Preserve the working file and change only `alwaysIgnoreTarget` to `true` for
   `Guided_Arrow`.
2. Compare controller casts with an enemy under the aim-assist cone and casts
   into empty space; record initial direction, first acquired target, reacquisition
   after target death, and keyboard/mouse behavior.
3. If that removes all useful aim rather than restoring homing, revert and test
   `useStickyFreeTarget=true` as a separate experiment, never both at once.
4. If neither is discriminating, trace the controller cast request versus the
   mouse cast request and then locate the missile-ID-86 do-function dispatch.

No game/mod file was changed and no live-game claim was made in this pass.

## Test deployment

On 2026-09-27, the launcher log identified `Reimagined` as the active mod on
the target build above, and no D2R process was running. A single-field test was
then deployed only to the active installed Reimagined copy:

```text
Guided_Arrow (id 22): alwaysIgnoreTarget=false -> true
```

The Reimagined Ladder copy and source checkout remain unchanged controls. The
exact original was preserved outside the game tree as
`controllerskillsettings.original.4B5E7902.json`; its SHA-256 is the original
controller-file hash recorded above. The deployed test file SHA-256 is
`C8D19D3AC7D12ECD40BF02309E9E21534E9A67A7CBC143E8C6BB6F41DBF3EC5F`.
JSON parsing passed, and a recursive semantic comparison found exactly one
difference: `/controllerskilldata/11/alwaysIgnoreTarget`, `false` to `true`.

Live behavior remains pending user validation. Restart the game before testing;
the controller-skill table is expected to load during startup.

### Test 1 result and test 2 deployment

The user live-tested `alwaysIgnoreTarget=true` with controller and reported no
behavioral change. This rejects that field alone as the observed lock-on trigger
for this build and scenario. It was restored to `false`.

With the game closed, the second isolated data test was deployed only to the
same active Reimagined copy:

```text
Guided_Arrow (id 22): useStickyFreeTarget=false -> true
```

All other Guided Arrow controller fields are back at their original values.
Live behavior for this second test remains pending user validation.

### Test 2 result and rollback

The user live-tested `useStickyFreeTarget=true` and again reported no behavioral
change. This rejects that field alone as the observed lock-on trigger for this
build and scenario.

With the game closed, the preserved original controller-skill file was restored
byte-for-byte. The installed active Reimagined copy again has SHA-256
`4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79`;
Guided Arrow has `alwaysIgnoreTarget=false` and `useStickyFreeTarget=false`.
There are no remaining test changes in the installed controller data.

The two negative tests shift the next investigation toward the cast request and
missile initialization/do-function paths: compare the controller and mouse
representations of unit target versus free-point target, and establish whether
missile ID 86's periodic retarget routine is entered in each case.

## Rev.47 action-request observation probe

Controller QOL already owns exact-build-guarded observers for the 18 native
client action handlers (`0x01` through `0x12`). Rev.47 reuses that observation
point; it introduces no new interception site, RVA, signature, or native ABI.

The opt-in `guided_arrow_probe` setting copies only the bytes synchronously
reported to the existing observer, after the original handler returns. Each
record contains the action opcode, reported size, and no more than 24 bytes.
Capture stops after 128 records for the process. The probe is read-only and
fails locally on an unreadable packet; it never changes a request or target.

The first live comparison will label matching mouse and controller casts, then
use packet shape differences to identify coordinate-target versus unit-target
requests. No opcode semantic is claimed until that trace is recorded.

### Live trace result

The user completed the ordered four-group test on 2026-09-27. D2RLoader
admitted Controller QOL `1.3.1+rev.47`, and the observer executed at runtime.
The first two records in each requested group were:

| Requested group | Observed action records |
| --- | --- |
| Mouse, monster targeted | `0D 01 00 00 00 18 00 00 00` twice |
| Mouse, empty ground | `0C 4D 12 ED 11`; `0C 4C 12 EC 11` |
| Controller, monster selected | `07 01 00 00 00 18 00 00 00` twice |
| Controller, no monster selected | `05 4E 12 24 12`; `05 4C 12 26 12` |

The nine-byte forms share the same payload: little-endian unit type `1` and
unit ID `0x18`. The five-byte forms contain two little-endian coordinate words.
Therefore, both input methods preserve the selected-unit versus free-coordinate
distinction at the observed action-handler boundary. The input paths do differ
in opcode family: mouse used `0x0D`/`0x0C`, while controller used
`0x07`/`0x05` in this test.

This rejects the narrower hypothesis that controller always collapses both
cases into a unit target before these handlers. It does not yet prove why the
two opcode families produce different Guided Arrow behavior. The next useful
experiment is a narrowly guarded diagnostic at, or immediately after, the
shared skill/missile construction path to compare the resulting target pointer
and Guided Arrow missile target-state initialization for `0x0C` versus `0x05`.

The trace also contained frequent `0x03` records with reported size 210 and
three later skill-action records after the requested pairs. They are retained
as unclassified observations and are not used for the conclusion above. After
capture, `guided_arrow_probe` was disabled in the active configuration.

## Rev.46-ga.1 correction candidate

Static comparison of the runtime-captured coordinate handlers established that
controller action `0x05` at game RVA `0x4ACE80` and mouse action `0x0C` at game
RVA `0x4AD230` perform matching size, coordinate, game and execution checks and
both call game RVA `0x4FDB40`. Their material difference is the selected-skill
accessor used for the final argument:

| Path | Selected-skill accessor | Guarded 32-byte entry |
| --- | ---: | --- |
| Controller coordinate (`0x05`) | game `0x34A540` | `40534883EC20488BD94885C97513884C2430488D4C2430E874BDFFFF84C07401` |
| Mouse coordinate (`0x0C`) | game `0x34B400` | `40534883EC20488BD94885C97513884C2430488D4C2430E8F49AFFFF84C07401` |

Evidence level is a static witness for the named D2R artifact plus the earlier
runtime opcode trace. Function names remain descriptive; a complete ABI beyond
the observed one-argument pointer return is not claimed.

The `rev.46-ga.1` candidate is opt-in and reuses Controller QOL's already-owned
action-handler trampolines. It redirects only an exact five-byte `0x05` cast
when both guarded accessors return readable skill records whose first table
field is skill ID 22. It copies the packet locally, changes only the local
opcode byte to `0x0C`, and calls the existing `0x0C` original trampoline. All
guard failures, other skills, other opcodes/sizes and memory faults preserve the
rev.46 controller path. The classic skill-record layout is supporting context,
not sufficient evidence by itself; live admission and visible homing remain
pending.

### Candidate result and rollback

The user live-tested `rev.46-ga.1`. Runtime logs established all intended
layers before visible behavior: D2RLoader admitted the candidate, both native
getter byte guards matched, both selected records passed the skill-22 gate, and
the controller coordinate cast was dispatched through the original mouse/right
coordinate-handler trampoline. Guided Arrow behavior did not change.

This rejects the action-opcode family and left-versus-right selected-skill
accessor as the cause of the observed homing difference for this test. The
divergence must be earlier than the server action request or later than the
shared action execution call, with controller-specific client targeting/missile
state now the stronger candidate.

The active installation and source build path were restored to protected
Controller QOL `1.3.1+rev.46`; SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
The failed candidate remains only as a private build artifact and this research
record, not as active Controller QOL code.

## Shared execution convergence

Further static tracing of the existing runtime D2R capture shows both coordinate
handlers call game RVA `0x4FDB40`. That routine validates the cast through game
RVA `0x4FDEF0` and, on success, calls game RVA `0x42D2C0` with the resolved
skill pointer, coordinates, and derived state. This convergence is consistent
with the `rev.46-ga.1` negative live result: substituting the right-skill action
handler did not change Guided Arrow behavior.

These are static call-path witnesses for installed `D2R.exe` SHA-256
`1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7`, not
complete function names or ABIs. The next investigation boundary is the loaded
client/Core skill-to-missile construction and missile-do-7 state, which requires
a read-only runtime Core capture before selecting any hook or correction.

## Exact-build missile and skill target-state path

A read-only, all-sections runtime capture established that classic gameplay
dispatch resides in the main game image rather than the initially scanned Core
image. The following main-image RVAs are static witnesses for the installed
`D2R.exe` identity above:

| Purpose | RVA | Witness |
| --- | ---: | --- |
| Missile server-do dispatch table | `0x2358430` | 77 slots; slot 0 null, slot 7 points to `0x1B6110` |
| Guided Arrow/Bone Spirit missile do 7 | `0x1B6110` | retrieves missile row, tests target-state bit 0, periodically validates/ranges a target, then redirects the path |
| Skill server-do 10 | `0x556CF0` | constructs the missile and calls the target-state setter below |
| Target-state setter | `0x3BD8A0` | validates a missile unit and writes the integer state through its missile data |
| Target-state setter call site | `0x556EA1` | bytes `E8 FA 69 E6 FF` |

At `0x556E96`, server-do 10 derives the setter argument from the resolved unit
target: state `1` for a non-null target and state `2` for no target. The do-7
routine at `0x1B618C` enters its homing/retarget block only when bit 0 of that
state is set. This matches the independently recovered D2MOO control flow, but
the RVAs and byte witnesses above come from the exact D2R runtime capture.

The standalone `guided-arrow-targeting-test` ga.2 candidate patches only the
guarded call at `0x556EA1`. Its nearby relay checks the still-live `r12d` skill
ID and changes `edx` from `2` to `1` only for skill ID 22 before tail-jumping to
the original setter. Bone Spirit, other skills, and already-targeted Guided
Arrow casts pass through unchanged. Controller QOL rev.46 is not modified.
The ga.2 DLL passed its loader ABI/resource/export/metadata artifact test and
was deployed separately as `Guided Arrow Targeting Test.dll`, SHA-256
`999E93B97C8740B2BF4E884684721BD19A7C563F68391A78B35D5A1F5BABEF4B`.
Protected Controller QOL remained `1.3.1+rev.46`, SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission, runtime execution, and visible homing remain pending live
validation.

### ga.2 early-load refusal and ga.3 lifecycle correction

The first ga.2 launch failed open at the `0x556CF0` byte guard. Log timestamps
showed plugin admission at `18:28:16` and D2R startup completion at `18:28:23`.
The full runtime capture has image size `0x5643000`, matching the loader-hosted
image, and contains the expected gameplay bytes after startup; the on-disk
loader image does not contain raw bytes for those late virtual sections.
Therefore the RVA evidence was retained, but initial plugin load was rejected
as the wrong lifecycle boundary.

ga.3 registers `GameJoined` and `LocalPlayerReady` listeners and remains inert
until one fires. Only then does it check the same function-entry and call-site
guards and publish the same skill-22/state-2 relay. A mismatch still leaves
native behavior unchanged and allows the later ready event to retry. Live
loader admission and patch execution remain pending. The ga.3 artifact test
passed and the built/deployed DLLs match at SHA-256
`0C23F4BE69942A97687C32F69581DCDF0B60C6A2129A748F12FB183BFABE26EA`.

Both ga.3 lifecycle callbacks then failed the function-entry guard. A fresh
read-only capture of the failing live process proved the module, RVA, and call
site were correct. The documented entry bytes begin
`40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C`; ga.2/ga.3 had incorrectly
omitted the leading `40` and included the next byte. ga.4 corrects only this
transcription error and retains both lifecycle deferral and the independent
`E8 FA 69 E6 FF` call-site guard.

ga.4 passed the loader artifact test and was deployed after confirming D2R was
closed. Its built and deployed SHA-256 is
`CFE919473CBA859C8C894C74397522A17F16D73ABDE6ADA9725243AE0DBC8B6E`.
The protected Controller QOL deployment remained `1.3.1+rev.46` with SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Late patch admission and visible Guided Arrow behavior remain pending.

Both ga.4 lifecycle callbacks reached the combined installation failure warning
without the loader's earlier byte-mismatch error. This narrows the refusal past
the corrected entry witness, but does not distinguish the independent call-site
guard, nearby relay allocation, relative-call encoding, relay protection, or
tracked patch publication stages. ga.5 keeps every RVA, expected byte, relay
instruction, and skill-22/state-2 condition unchanged while giving each stage a
distinct fail-open log message. No new native contract is claimed by this
diagnostic-only revision.

ga.5 passed its loader ABI/resource/export/metadata artifact test and was
deployed only after confirming D2R and D2RLoader were closed. Its built and
deployed SHA-256 is
`53D4D1379D6329DAA68491DA155E56D59031E5373D35A83009E5C8C6158B0050`.
The protected Controller QOL deployment remained `1.3.1+rev.46`, SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Runtime-stage diagnosis and visible Guided Arrow behavior remain pending live
validation.

Live ga.5 validation reached both byte guards and then failed only at relay
allocation with Win32 error 487 (`ERROR_INVALID_ADDRESS`) on both lifecycle
callbacks. No call-site patch was published. The rejected allocator scanned
fixed addresses only above the call site; ga.6 instead walks free virtual-memory
regions below and above the site with `VirtualQuery`, bounded to rel32 reach.
All build-specific guards and relay semantics remain unchanged.

ga.6 passed the loader artifact test and was deployed after confirming D2R and
D2RLoader were closed. Its built and deployed SHA-256 is
`2E8C1C39D18F9B2E22AA2F33C2A0EDE98AAF69CA297763CC39A80301F4DEAB8E`.
The protected Controller QOL deployment remained `1.3.1+rev.46`, SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission, patch publication, and visible behavior remain pending live
validation.

Live ga.6 validation confirmed loader admission and successful late patch
publication, but a controller free-point Guided Arrow still did not visibly
home. This rejects promotion of the post-creation target-state value from `2`
to `1` as a sufficient correction. The exact do-7 disassembly explains the
limit: after testing bit 0, it only validates and follows the missile's existing
dynamic-path target; its initial helper does not acquire a new unit when the
target pointer is null.

The exact skill-do-10 disassembly also confirms that a targetless cast enters
missile construction with a null target pointer and flags `0x420`, whereas a
unit-target cast uses flags `0x20`. Supporting classic source shows the extra
`0x400` adjusts current-frame calculation from coordinate distance; it does not
perform target acquisition. Therefore neither forcing target state nor merely
clearing `0x400` can supply the missing unit identity. The next safe boundary is
the controller target-selection stage that decides whether to emit the observed
unit-target `0x07` request or coordinate-target `0x05` request. No further
missile-state mutation should be attempted without that evidence.

The user then clarified the visible behavior boundary: controller Guided Arrow
flies straight even when a monster is visibly selected, while keyboard/mouse
shots curve toward a selected monster and targetless ground shots can later make
hard turns to acquire one. Thus this is not merely controller aim assist failing
to emit a unit-target request. The keyboard/mouse ground case demonstrates a
real post-launch acquisition path that the controller-created client projectile
does not enter. Because ga.6 modified only the server skill/missile state, the
next discriminating experiment must observe the client Guided Arrow missile
update/initialization state for matching mouse and controller casts. Candidate
client dispatch tables remain unproven and are not yet suitable patch targets.

### Rejected ga.7 candidate and exact client dispatch slot

ga.7 tested main-image RVA `0x459850` as a candidate client Guided Arrow update
routine. The exact-build guards and inline hook installed, but mouse and
controller Guided Arrow tests produced no per-projectile observer lines. This
is runtime evidence that the candidate is not the active Guided Arrow client
dispatch path for this build; its previously inferred meaning is rejected.

ga.7 removes the ga.6 server-state mutation and installs only an SDK-managed
inline observer at `0x459850`, after gameplay-ready. It separately guards the
client routine and both read-only accessors. For at most 64 distinct projectile
pointers, it logs only the first native update's state and acquisition gate
before and after calling the unmodified original routine. It never writes game
state, targeting state, input state, or Controller QOL configuration. The ga.6
runtime DLL was removed after confirming the game and loader were closed; its
source and build evidence remain reproducible.

ga.7 passed its loader ABI/resource/export/metadata artifact test and was
deployed after confirming D2R and D2RLoader were closed. Its built/deployed
SHA-256 is
`E1B4EB23A8DCA06AD98153A051863F618AC8E5AE01338AB9A06419FEAD6FBF30`.
The protected Controller QOL deployment remained `1.3.1+rev.46`, SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and hook installation were live observed; the silent runtime
path rejected the candidate before any mutation was attempted.

Subsequent bounded analysis of the same exact runtime capture identifies the
client missile dispatch table at main-image RVA `0x2390E80`. Slot 7 points to
RVA `0x458560`, whose control flow mirrors the independently identified server
Guided Arrow routine: it reads missile state, resolves a target, validates it,
uses the missile record timing value at offset `0x5C`, checks distance, and can
redirect the missile path before tail-jumping to standard client missile
processing at RVA `0x463100`. This is a static semantic witness for the exact
captured build; the table layout and function ABI are not portable claims.

The exact-build ga.8 guard and observed call contracts are:

- client missile dispatch slot 7 function entry, RVA `0x458560`, bytes
  `48 89 5C 24 18 55 56 41 57 48 83 EC 20 48 8B DA`, called as
  `int __fastcall(void* client, void* missile)`;
- missile state getter, RVA `0x3BC0A0`, bytes
  `40 53 48 83 EC 20 48 8B D9 48 85 C9 74 0A E8 1D`, called as
  `int __fastcall(void* missile)` and tested at the dispatch routine with
  `test al, 1`;
- target resolver, RVA `0x48FE20`, bytes
  `40 53 48 83 EC 20 48 8B DA E8 22 1D 00 00 48 8B`, called as
  `void* __fastcall(void* client, void* missile)`.

ga.8 is an observer at slot 7. It calls the state getter and target resolver
immediately before and after the original function and logs the first
observation plus later state or target-presence changes for at most 64
projectile pointers. It does not directly write the missile or patch the
state/target helpers. The build completed with MSVC under `/W4 /WX`, and its
loader ABI/resource/export/metadata artifact test passed. The built and
deployed DLLs have matching SHA-256
`C534AC12A51D5EE2F8D50162C3E0D57AD90125E5B397ED628DA70B210A68FCB3`.
Deployment was performed only after confirming D2R and D2RLoader were closed;
the protected Controller QOL rev.46 hash remained unchanged. Loader admission
and hook installation were live observed on 2026-09-28. A user-identified
mouse ground shot produced three observations for one projectile pointer:

```text
state=2->2 target=0->0 result=1
state=2->5 target=0->1 result=1
state=1->1 target=1->1 result=1
```

This is runtime evidence that slot 7 is the active client Guided Arrow path and
that a mouse ground shot can begin without a resolved target, later resolve one
inside the native client path, and subsequently remain targeted. The numeric
state meanings are not yet established, and these observations alone do not
identify which upstream input/cast field enables acquisition. Two additional
projectile observations (`2->6`, no target; and `1->1`, target present) were
captured in the same session but were initially unmapped to exact test inputs.
Visible mouse homing was user-confirmed.

A controlled four-shot comparison on 2026-09-28 subsequently established:

| Input and cast | Observed client transition |
| --- | --- |
| controller ground | `state=2->6 target=0->0 result=1` |
| controller selected target | `state=1->1 target=1->1 result=1` |
| mouse ground | `state=2->2 target=0->0`, then `2->5 target=0->1` |
| mouse selected target | `state=1->1 target=1->1 result=1` |

This is strong runtime evidence of a client-side divergence for targetless
casts: the controller
ground projectile enters state 6 without acquiring a target, while the mouse
ground projectile remains in state 2 until the native routine acquires a target
and changes it to state 5. Conversely, the observer cannot distinguish the two
selected-target casts: both enter slot 7 in state 1 with a resolved target. The
user still observes the controller-selected projectile flying straight, so
client target presence at this boundary is insufficient to establish homing;
the remaining difference may be in another missile/path field or in the
corresponding server-created projectile. State 5 and state 6 meanings remain
unknown. No mutation should be selected until the branch producing those states
and the targeted-shot path fields are compared.

Post-test static review found that RVA `0x48FE20` is not guaranteed to be a
pure accessor: it calls RVA `0x491B50`, which reconciles the missile's dynamic
path target and may clear a stale target through RVA `0x342920`. ga.8 therefore
invokes native target-maintenance logic additional times despite making no
direct writes, so its “read-only” runtime wording is too strong. The native
slot-7 routine already invokes the same resolver, which limits but does not
eliminate possible probe influence. Stop ga.8 live testing and reconfirm the
four-way result with a passive replacement that observes the resolver's native
return rather than calling it. The state getter at RVA `0x3BC0A0` is a bounded
accessor for the `DWORD` at missile-data offset `0x30`; the setter at RVA
`0x3BD8A0` writes that same field. These offsets and call contracts are static
witnesses for the captured build only.

### Passive ga.9 observer

ga.9 removes all extra target-resolver calls. It retains the guarded pure state
read and installs pass-through observers at the existing client target resolver
and path-redirection routine. A thread-local call frame associates only calls
naturally nested inside Guided Arrow client dispatch slot 7 with the active
missile. It reports the resolver's native return as target present/absent and
counts native steering calls without invoking either operation itself.

The newly guarded path-redirection entry is main-image RVA `0x340E70`, with
16-byte witness
`48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55`. At the slot-7 call site
the observed register contract is `rcx=path`, `rdx=missile`, `r8d=0`, and
`r9d=1`; ga.9 conservatively models it as
`void __fastcall(void* path, void* missile, int unknown, int enabled)`. The
return value is unused at the known call site. This is an exact-build static
witness, not a general ABI claim.

ga.9 built under MSVC `/W4 /WX`; its loader ABI/resource/export/metadata
artifact test passed. Deployment occurred only after confirming D2R and
D2RLoader were closed. Built and deployed SHA-256 match at
`E60E45D17D18D0D9DBD06C3D9325CF8249B53E17A4A5D1B25A851DBA49290DFE`.
Controller QOL remained version `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and the passive four-shot comparison remain pending live
validation.

ga.9 loader admission and hook installation were live observed on 2026-09-28.
The controlled order (controller ground, controller selected target, mouse
ground, mouse selected target) produced:

```text
controller ground:   state=2->6 nativeTarget=not-called redirectCalls=1
controller targeted: state=1->1 nativeTarget=present    redirectCalls=1
mouse ground:        state=2->2 nativeTarget=not-called redirectCalls=0
                     state=2->5 nativeTarget=not-called redirectCalls=1
mouse targeted:      state=1->1 nativeTarget=present    redirectCalls=1
```

This passively reconfirms the client state divergence and that both selected-
target casts naturally execute the target resolver and receive a non-null
target. It does not yet prove which casts execute the Guided Arrow-specific
steering call at RVA `0x458688`: the controller-ground observation counted a
redirect despite skipping the target resolver, proving that RVA `0x340E70` is
also reached by a generic path update nested in the shared client-missile tail.
The next observer must classify calls by their return address; the exact return
address for the Guided Arrow-specific call is main-image RVA `0x45868D`.

ga.10 implements that passive classification with MSVC `_ReturnAddress()`. A
call to RVA `0x340E70` is counted as `guidedRedirects` only when its return
address is exactly main-image RVA `0x45868D`; other nested calls are counted as
`otherRedirects`. It otherwise retains ga.9's native-call-only observation and
does not invoke target resolution or steering.

ga.10 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred after a
fresh closed-process check. Built and deployed SHA-256 match at
`073A038F36480742138DD75515014B1A45513C92D2F555D38C5961604AA2CA60`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Live validation remains pending.

ga.10 loader admission and passive runtime execution were live observed on
2026-09-28. The controlled comparison produced:

```text
controller ground:   state=2->6 nativeTarget=not-called guided=0 other=1
controller targeted: state=1->1 nativeTarget=present    guided=1 other=0
mouse ground:        state=2->2 nativeTarget=not-called guided=0 other=0
                     state=2->5 nativeTarget=not-called guided=0 other=1
                     state=5->5 nativeTarget=present    guided=1 other=0
mouse targeted:      state=1->1 nativeTarget=present    guided=1 other=0
```

This confirms that controller- and mouse-targeted casts are indistinguishable
at the client Guided Arrow steering boundary: both resolve a target and execute
the exact Guided Arrow-specific redirect call. Since the user still observes
the controller-targeted projectile flying straight, the client target and
steering path cannot by themselves explain the visible failure. The remaining
high-confidence boundary is the authoritative/server missile or subsequent
server correction. For ground casts, controller state 6 remains even and never
enters acquisition, whereas mouse state 5 is odd and enters native acquisition
on the following update. No client mutation is justified by these results.

### Passive ga.11 server comparison

The exact server missile do-7 routine is main-image RVA `0x1B6110`, guarded by
entry bytes
`48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57`. Static register use and
the dispatch context support a provisional wrapper contract of
`void __fastcall(void* missile, float parameter)`: the missile arrives in
`rcx`, and the routine preserves the second scalar argument from `xmm1` across
its processing tail. This remains an exact-build static ABI witness pending
runtime execution.

The authoritative Guided Arrow steering call is at RVA `0x1B6225`, calling the
same guarded redirection routine at RVA `0x340E70`; its exact return address is
RVA `0x1B622A`. ga.11 wraps server do-7 without adding native calls, reads the
same guarded missile state before and after the original, and classifies nested
redirection calls by that return address. It retains ga.10's passive client
observations so each controlled shot can be compared at both boundaries.

ga.11 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256
match at
`984F6D35574CDC6F1EBA63CD6542FA4AC35109CD186D743A5E86AA9DC7232578`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live server ABI/steering observations remain pending.

ga.11 loader admission, the provisional server wrapper ABI, and passive
client/server execution were live observed on 2026-09-28. The controlled
comparison produced:

```text
controller ground:   client 2->6 guided=0 other=1
                     server 2->6 guided=0 other=1, later 6->0
controller targeted: client 1->1 target=present guided=1
                     server 1->1 guided=1, later 1->0
mouse ground:        client/server 2->2, then 2->5 via other redirect
                     client 5->5 target=present guided=1
                     server 5->0 without a recorded guided redirect
mouse targeted:      client 1->1 target=present guided=1
                     server 1->1 guided=1, later 1->0
```

The server comparison rejects the hypothesis that controller-targeted casts
fail because the authoritative missile lacks a target or never reaches Guided
Arrow steering: controller and mouse targeted casts both execute the exact
client and server Guided Arrow redirect calls from state 1. Since their visible
behavior still differs, the next discriminating boundary is the path object
passed in `rcx` to RVA `0x340E70`, including the path mode/flags and direction
fields consumed or changed by that routine. For ground casts, the passive
client/server observations reconfirm the controller-specific transition to
even state 6 and the mouse transition to odd state 5. No state or target
mutation is justified yet.

Static analysis of the exact RVA `0x340E70` steering routine narrows the next
passive comparison to fields it directly consumes from the dynamic path:

| Path offset | Width | Observed use |
| --- | --- | --- |
| `0x02`, `0x06` | `WORD` each | current path coordinates copied into the working path request |
| `0x10`, `0x12` | `WORD` each | requested destination coordinates; an all-zero pair refuses steering |
| `0x20` | pointer | room/act lookup context |
| `0x48` | `DWORD` | path flags; bits `0x40000`, `0x1000`, `0x800`, `0x20`, and `0x1` control distinct branches |
| `0x50` | `DWORD` | path type/mode used to select the path generator |
| `0x58`, `0x5C`, `0x64` | `DWORD` each | copied path-generation parameters |
| `0x70` | pointer | associated/target unit consulted during redirection |
| `0xBD`, `0xBE` | `BYTE` each | copied directional/path parameters |

These are bounded accessed offsets, not a complete path layout. A final passive
snapshot at only the exact Guided Arrow return-address call sites can compare
controller-targeted and mouse-targeted path mode, flags, destination,
associated-unit presence, and direction before/after steering. That evidence is
required before changing a path mode or flag.

ga.12 implements that bounded snapshot using `memcpy` reads at the documented
offsets immediately before and after only return-address-proven Guided Arrow
redirects. Generic redirects remain count-only. It does not call any additional
game routine and does not write the path or missile.

ga.12 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred after a
fresh closed-process check. Built and deployed SHA-256 match at
`032E7458568342C15D3E54A6FB1ECB475D8305F8F6917FB6433CDBDE023513C6`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live path-field comparison remain pending.

ga.12 loader admission and bounded path snapshots were live observed on
2026-09-28. In the requested order (controller selected target, then mouse
selected target), both client and server paths matched on mode `4`, associated
unit presence, direction bytes `0,0`, and parameters `1,1,388`. The first shot
had client/server flags `0x00060021`/`0x00060031`; the second had
`0x00060020`/`0x00060030`. Thus bit `0x1` was the only captured non-coordinate
field difference. Coordinates and destinations differed substantially between
the shots, so this does not yet establish bit `0x1` as input-source-specific.

The classic D2MOO path reference uses path flag bit `0x1` to mark path points
or a target coordinate outside the current room. This is supporting semantic
evidence only, not proof that the exact D2R field retains the same complete
meaning. The exact D2R steering routine also clears or sets bit `0x1` in its
path construction tail. A same-position/same-target comparison is required to
separate controller initialization from ordinary room/path geometry.

The earlier mouse-ground trace retained the same observed client missile
pointer through state `2->5` and the subsequent state-5 Guided Arrow redirect;
the server trace likewise retained its missile pointer through the transition.
This is evidence that the sharp acquisition turn recomputes the existing
missile's path rather than spawning a second do-7 Guided Arrow. It does not
exclude an unrelated visual helper missile outside do-7, but no such helper is
needed to explain the observed native redirect.

A follow-up same-position sequence on 2026-09-28 (controller targeted, mouse
targeted, controller ground, mouse ground) removed the targeted path-field
difference. Both targeted shots used client/server flags
`0x00060020`/`0x00060030`, mode 4, associated-unit present, directions `0,0`,
and parameters `1,1,388`. Their destinations differed as the target position
changed, but no captured non-coordinate field distinguished the input source.
This rejects the earlier flag-bit candidate as an input marker and supports the
classic interpretation that it reflected path geometry.

The same run reconfirmed controller ground as client/server `2->6` through a
generic redirect and mouse ground as client/server `2->5`. No target was close
enough for the latter shot to reach a later client Guided Arrow redirect during
the captured interval. Interpreting the status as flags is now the strongest
working model: bit 0 is precisely the gate tested by both client and server
do-7; state 5 (`0b101`) enables acquisition, while state 6 (`0b110`) retains an
even/non-acquiring state. The meanings of bits 1 and 2 remain provisional. The
next passive boundary is the exact setter/write and return address that chooses
5 versus 6 inside the shared missile-processing tail.

### Narrow ga.13 ground-acquisition mutation

The installed Reimagined `missiles.txt` identifies `guidedarrow` as missile row
86 and assigns server/client do functions 7. Exact disassembly of the unit class
accessor used at both do-7 entries confirms the class identifier is the
`DWORD` at unit offset `0x04`. ga.13 therefore adds the first post-observation
mutation with three simultaneous gates:

1. execution is inside the exact guarded client or server do-7 wrapper;
2. the missile class at unit offset `0x04` is exactly row 86;
3. native processing entered with status 2 and returned with status 6.

Only then does ga.13 call the guarded native status setter at RVA `0x3BD8A0`
with value 5. Its 16-byte entry witness is
`48 89 5C 24 10 57 48 83 EC 20 8B FA 48 8B D9 48`. Targeted state-1 casts,
mouse ground casts already reaching state 5, other do-7 missile rows, input
handling, and Controller QOL are outside the mutation gate. The experiment
tests whether enabling bit 0 after the controller-specific ground transition is
sufficient for next-update native acquisition; it does not yet establish a
production fix.

ga.13 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256
match at
`BAB54684E9A194CF3B6C577D9E2B7E2587ED42F988778408C70A1A5A127ABE58`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission, mutation execution, and visible homing remain pending live
validation.

ga.13 loader admission and both client/server mutation gates were live observed
on 2026-09-28. A controller ground shot logged `2->5 promoted=1` on both sides,
proving the exact missile-row and transition gates executed as designed. On the
next client update, however, the native target resolver returned null and no
Guided Arrow redirect occurred; the server subsequently changed state `5->0`.
The projectile visibly remained straight. A mouse ground control naturally
changed `2->5`, then obtained a non-null target and executed client/server
Guided Arrow redirects.

This rejects status promotion as a sufficient fix. State 5 accompanies or
permits a target-bearing path, but synthesizing it after native processing does
not supply the missing target identity. Stop ga.13 mutation testing. The next
passive experiment should capture the return address and bounded path snapshot
for the earlier generic redirect that naturally produces controller state 6
versus mouse state 5, and observe the native path-target assignment rather than
forcing either status value.

### Passive ga.14 generic-path comparison

ga.14 removes the ga.13 setter reference and mutation entirely. For redirects
nested inside the guarded client/server do-7 wrappers that do not return to the
known Guided Arrow-specific call sites, it records the exact return-address RVA
and the same bounded path snapshot immediately before and after the native
call. Exact Guided Arrow redirects remain separately classified. This is meant
to identify the earlier native caller and path inputs that naturally produce
state 6 for controller ground casts versus state 5 plus a target-bearing path
for mouse ground casts.

ga.14 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256
match at
`51FB6A2D31E0EE5F09CA2CE510BDDDAE064CD049699AE220E70D2189B6C74D93`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live generic-call comparison remain pending.

ga.14 loader admission and passive generic-path observation were live validated
on 2026-09-28 with single controller and mouse ground shots from the same
position/direction. Both used the same generic callers: client return RVA
`0x46781D` and server return RVA `0x1BCEB8`. Before those calls, the controller
path had no associated unit (`path+0x70 == null`) and retained a near-adjacent
destination; it naturally became state 6. The mouse path already had an
associated unit and the native call replaced its destination with the target's
position; it naturally became state 5 and immediately entered Guided Arrow
steering. Therefore the generic caller does not originate selection: the
mouse-only unit association exists before it, while the controller path never
receives one.

### Passive ga.15 target-association caller trace

RVA `0x342920` is the exact dynamic-path target setter. It writes the target
pointer to path offset `0x70`, then records the target unit type and ID at path
offsets `0x78` and `0x7C`. Its 16-byte entry witness is
`48 89 5C 24 08 57 48 83 EC 20 48 89 51 70 48 8B`, and the observed ABI is
`void __fastcall(void* path, void* target)`. ga.15 passively hooks this setter
and logs only non-null assignments whose path matches the measured Guided Arrow
fingerprint: mode 4 and parameters `1,1,388`. Each event records the exact
caller RVA and target type/ID, then calls the unmodified original setter.

ga.15 built under MSVC `/W4 /WX`, and its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256
match at
`16B582BEE3FAB4687706B215B74C9D11A4693BE7B657F7965C9FBA9ADA62B96D`.
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live association-caller observation were subsequently
validated. A controller ground shot produced no target-set call. A mouse ground
shot produced a server assignment returning to RVA `0x1BCDC6`, followed by a
client assignment returning to RVA `0x46772B`; both assigned the same unit
(`targetType=1`, `targetId=14`). This proves that the mouse-only association is
created symmetrically on server and client before each generic redirect, while
the equivalent controller-ground path never calls the target setter.

### Shared association routines and ga.16 upstream trace

Static reconstruction identifies the containing mirrored routines as server
RVA `0x1BCCE0` and client RVA `0x467640`. The server entry witness is
`40 55 57 41 54 41 56 48 83 EC 38 48 8B EA 4C 8B`; its observed input shape
is provisionally `void __fastcall(void* missile, void* candidateTarget)`. The
client entry witness is
`40 55 57 41 55 41 56 48 83 EC 38 4C 8B F2 49 8B`; its observed input shape is
provisionally
`void __fastcall(void* clientContext, void* missile, void* candidateTarget)`.
These complete ABIs and return types remain candidates until runtime passage is
confirmed, but the register roles are direct static witnesses: the server saves
`RDX` as the candidate and `RCX` as the missile; the client saves `R8` as the
candidate and `RDX` as the missile.

Both routines validate the incoming candidate. When it is non-null, they call
the path target setter (`0x342920`), select state 5, and continue to the generic
redirect (`0x340E70`). When it is null, they write fallback coordinates, select
state 6, and then use that same generic redirect. Therefore neither routine
chooses the candidate: selection has already happened at their entry. A scan of
the captured runtime image found no direct relative call or absolute stored
pointer to either entry, consistent with indirect dispatch.

ga.16 adds pass-through entry observers at these two exact-build-guarded RVAs.
For missile row 86 only, it records the real upstream return-address RVA and
whether the incoming candidate is absent or identifies a unit. It does not
modify the candidate, target association, state, path, input, or Controller QOL.
It built under MSVC `/W4 /WX`; the loader ABI/resource/export/metadata artifact
test passed. Deployment occurred only after confirming D2R and D2RLoader were
closed. Built and deployed SHA-256 match at
`04B5F41E42329235B4221993048634F68B84F470343166FD697BF2A8A2CFB23C`.
The ga.15 deployment was preserved as a rollback copy, and Controller QOL
remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live upstream-caller observation were validated on
2026-09-28. Both input modes used the same upstream calls: client return RVA
`0x45D5B6` and server return RVA `0x1AD149`. The controller ground shot supplied
a null candidate to both. The mouse ground shot supplied the same unit to both
(`targetType=1`, `targetId=23`). Therefore there is no controller-specific
association or redirect implementation at this layer; the divergence is in the
candidate value produced immediately beforehand.

The client caller is the routine beginning at RVA `0x45D3F0`. Its target search
call is RVA `0x4331D0`, returning the candidate in `RAX`, which is moved into
`R8` before the call to `0x467640`. The server caller begins at RVA `0x1ACFA0`.
Its mirrored search call is RVA `0x213C90`, whose `RAX` result is moved into
`RDX` before the call to `0x1BCCE0`. These are static register-flow witnesses;
the full search ABIs and filter flags remain only partially reconstructed.

### Acquisition-delay gate and ga.17 observer

Both caller routines test RVA `0x3BB1E0` before invoking their target search.
The client call returns at `0x45D4A7`; the server call returns at `0x1AD04A`.
When its result is positive, the routine exits without association and leaves
state 2 available for a later retry. When it is non-positive, native search is
performed once: a non-null result flows to state 5, while null flows to state 6.
This matches the live ga.16 sequence: the mouse projectile first remained
`2->2`, then acquired a target roughly half a second later, whereas the
controller projectile immediately searched with no candidate and became state
6.

RVA `0x3BB1E0` has the 16-byte entry witness
`40 53 48 83 EC 20 48 8B D9 48 85 C9 74 0A E8 DD` and the bounded observed ABI
`int __fastcall(void* unit)`. For a missile unit (native unit kind 3), it loads
the pointer at unit offset `0x10`, reads a float at pointed-object offset
`0x14`, divides it by a build-global scalar, and converts the result to an
integer. The semantic name "acquisition delay" is contextual to this caller;
the underlying field's general engine name and units are not yet proven.

ga.17 passively hooks this getter and logs its result only for missile row 86
and only at the two return-address-proven acquisition gates. It retains ga.16
observations and performs no mutation. It built under MSVC `/W4 /WX`, and the
loader ABI/resource/export/metadata artifact test passed. Deployment occurred
only after a process check confirmed D2R and D2RLoader were closed. Built and
deployed SHA-256 match at
`54174B2FB847D9BADF77CFAD63E1AA386BC0E7D843305E9A333CF1046525470E`.
The ga.16 DLL was preserved as a rollback copy, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
ga.17 failed open during loader admission because its source guard accidentally
omitted the leading `0x40` prefix and shifted the witness by one byte. No hook
was installed and native behavior was retained. ga.18 corrects only that guard;
it built under MSVC `/W4 /WX`, and the loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256 match
at `72147742E5AAF80A23443FFAD82D71E2015BA063005A5C3591C453E8F0730171`.
The ga.17 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live delay comparison remain pending.

ga.18 loader admission and its live delay comparison were validated on
2026-09-28. For the controller ground shot, both client and server returned
`remaining=-1`, immediately supplied null candidates to the association
routines, and transitioned `2->6`. For the mouse ground shot, the projectile
first remained `2->2`; after travelling, both client and server returned
`remaining=0`, supplied unit type 1 / ID 14, and transitioned `2->5`.

This establishes that the controller divergence precedes search: its ground
shot reaches the acquisition gate already negative and searches beside its
near-adjacent destination, whereas the mouse projectile remains eligible to
travel before the successful native search. It does not yet prove whether the
negative value is directly initialized by controller casting or emerges from
the controller ground destination's extremely short travel interval.

### ga.19 one-update deferral experiment

ga.19 is the first mutation following the ga.18 gate proof. Only for Guided
Arrow row 86, only at the return-address-proven client/server acquisition gates,
and only when the native result is negative, it returns `1` exactly once per
observed client/server projectile. It does not write the underlying timer,
target association, path, missile state, input state, or Controller QOL data;
every subsequent call returns the unmodified native value. This discriminates
whether one additional native update is enough to let the controller projectile
advance into a useful search position without risking an indefinite state-2
loop.

ga.19 built under MSVC `/W4 /WX`; its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256 match
at `E0E9B67C4715AAAE91335029F9122CD1A2C0FAFF62E69B9882D1F18948141603`.
The ga.18 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live behavior remain pending.

ga.19 live testing used two controller ground shots. Its one-update deferral
did not activate: both observed gates returned native/effective `0`, with
`deferred=0`. The distant-target shot supplied no candidate and became state 6.
The close-range shot, deliberately fired while facing away, supplied unit type
1 / ID 23 on both server and client and naturally became state 5; it visibly
turned and hit the target. This proves controller-ground native acquisition and
homing work when a unit lies within the search centered near the controller's
adjacent ground point. ga.19's override is therefore removed rather than
expanded.

### ga.20 shared coordinate-cast observer

Controller action `0x05` calls shared coordinate-cast RVA `0x4FDB40` and returns
to RVA `0x4ACF58`; mouse action `0x0C` calls the same entry and returns to RVA
`0x4AD308`. The entry's 16-byte witness is
`48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57`. Static register and stack
flow gives the bounded provisional ABI
`int __fastcall(void* context, void* playerUnit, uint32 x, uint32 y, uint32 flag, void* selectedSkill)`.
The routine receives coordinates in `R8D/R9D`, the apparent player unit in
`RDX`, and the selected-skill pointer as the sixth argument. Both known callers
pass flag 1.

ga.20 passively logs the player path coordinates and requested coordinate delta
only when the return RVA is one of those two exact sites and the selected-skill
record begins with ID 22. This hook is downstream of Controller QOL's already
owned action-handler hooks and makes no coordinate, skill, input, missile, or
QOL mutation. ga.19's acquisition-delay override was removed; the retained
delay hook is pass-through observation only.

ga.20 built under MSVC `/W4 /WX`; its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256 match
at `452EF9FF89581447C057435EF428B5E46494F1EE68DFE84F59CC55A7426AA269`.
The ga.19 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live coordinate comparison remain pending.

### ga.22 live coordinate comparison and ga.23 projection

ga.22 live observation captured the same player position for the controlled
pair. The controller call was native return RVA `0x4ACF58` with target delta
`(-2,+1)` (Euclidean distance about 2.24 tiles). The mouse call was native
return RVA `0x4AD308` with target delta `(-28,+7)` (about 28.86 tiles). Both
selected-skill accessors returned the same object, so accessor pointer identity
is ambiguous and must not be used as the source classifier. The exact return
RVA does distinguish the source in this hook topology; the earlier ga.20
absence was solely its incorrect direct skill-ID filter.

The raw selected-skill fields `2089059448,3` are the low/high DWORDs of the
pointer stored at selected-skill offset 0, not a skill ID. Static inspection of
native RVA `0x33DBA0` confirms it loads that pointer and reads the first WORD of
the pointed record. That first WORD is the bounded skill-ID discriminator used
by ga.23.

ga.23 is the first coordinate mutation. At shared coordinate-cast RVA
`0x4FDB40`, it changes arguments only when the return address is exact native
controller site `0x4ACF58`, the pointed selected-skill record's first WORD is
22, the player path is present, and the original nonzero target delta has
Euclidean length under 8 tiles. It preserves the supplied direction and
normalizes the point to 25 tiles from the player, close to the controlled mouse
sample's 28.86 tiles. Invalid or out-of-range projected coordinates fail open
to the original point. Mouse calls, other skills, and already-distant
controller points are unchanged.

ga.23 built under MSVC `/W4 /WX`; its loader
ABI/resource/export/metadata artifact test passed. Deployment occurred only
after confirming D2R and D2RLoader were closed. Built and deployed SHA-256 match
at `44ADCB817829C7B8EE45267F76E9640833B8105FFE691529517E24EF354DACE6`.
The ga.22 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live projected behavior remain pending.

ga.23 loader admission and visible behavior were live validated by the user on
2026-09-28: a controller ground cast projected to the farther point and Guided
Arrow acquired and homed successfully. At the user's request, ga.24 changes
only the normalized projection distance from 25 to 18 tiles, a 28% reduction
approximating 30%. Every caller-RVA, skill-ID, near-point, coordinate-range, and
fail-open guard remains unchanged. ga.24 built under MSVC `/W4 /WX`, and its
loader ABI/resource/export/metadata artifact test passed. Deployment and live
distance tuning remain pending. Deployment then occurred only after confirming
D2R and D2RLoader were closed. Built and deployed SHA-256 match at
`B92D8198ED5A451FA10691CF5B0633B54B1FA2F8432FED21A467247505AADC64`.
The live-working 25-tile ga.23 DLL was retained as the rollback artifact, and
Controller QOL remained `1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.

### Controller QOL rev.47 production integration

The production correction reuses Controller QOL's existing exact-build-guarded
action `0x05` handler hook at RVA `0x4ACE80`; it does not retain the standalone
shared-coordinate-cast hook at `0x4FDB40`. Before the original handler runs, it
copies the five-byte packet (`opcode`, WORD X at `+1`, WORD Y at `+3`) and only
projects the copy when the guarded controller selected-skill accessor at RVA
`0x34A540` resolves a wrapper whose offset-zero record pointer begins with
WORD skill ID 22. The player path pointer is at unit `+0x38`, with current X at
path `+0x02` and Y at path `+0x06`.

The selected-skill accessor's 32-byte witness is
`40 53 48 83 EC 20 48 8B D9 48 85 C9 75 13 88 4C 24 30 48 8D 4C 24 30 E8 74 BD FF FF 84 C0 74 01`.
The original nonzero coordinate delta must have squared Euclidean distance less
than 64; the copy is normalized in the same direction to 20 tiles. Invalid or
out-of-WORD coordinates fail open. The accessor guard is optional and isolated:
its mismatch disables only Guided Arrow projection, while the established
Controller QOL hooks remain eligible. The standalone 25- and 18-tile builds
were live validated; loader admission and visible validation of the merged
20-tile rev.47 DLL remain pending. All 21 Controller QOL automated suites pass.
The built and deployed rev.47 DLLs match at SHA-256
`BCD569DF40630A6AC34F81549E5A17C6DD52D1732D17BD6B15527D89A3DA928A`;
the standalone ga.24 DLL is disabled in place so it cannot stack with the
merged implementation. Rev.46 remains the protected rollback artifact.

ga.21 loader admission succeeded and all retained missile observers fired, but
it again emitted no shared-cast coordinate records. The remaining suppressor
was the unproven assumption that the selected-skill object's first DWORD is
directly skill ID 22. Static flow establishes that the sixth argument is the
object returned by the selected-skill accessor, not its complete object layout;
the offset-zero ID claim is therefore rejected for this pointer type.

ga.22 removes that filter. It remains pass-through and bounds output to the
first 64 shared coordinate calls whose observed flag is 1. Each line records
selected-skill pointer equality against both guarded accessors, native-versus-
trampoline caller classification, player-path presence, player/target
coordinates and delta, plus raw DWORDs at selected-skill offsets 0 and 4.
Build succeeded and the artifact test passed; deployment and live ABI
observation remain pending. Deployment then occurred only after confirming D2R
and D2RLoader were closed. Built and deployed SHA-256 match at
`2BCCDE58951F24E957838DA4558D2FCF3FF3A339311749286271BD40755A3754`.
The ga.21 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.

ga.20 loader admission succeeded, but its `source=` coordinate lines never
fired. The retained missile observers did fire normally. This is not evidence
that `0x4FDB40` was unused: Controller QOL already owns the `0x05` and `0x0C`
action-handler entries, so the original handler bodies and their relative calls
execute from MinHook trampolines. Consequently `_ReturnAddress()` at the shared
cast entry is in relay memory rather than native RVAs `0x4ACF58`/`0x4AD308`.
The exact native caller RVAs remain valid static witnesses but cannot classify
this coexisting runtime hook topology.

ga.21 replaces only that passive classifier. At the shared cast entry it calls
the already documented, exact-build-guarded controller selected-skill accessor
`0x34A540` and mouse selected-skill accessor `0x34B400` with the apparent player
argument, then compares both returned pointers with the supplied sixth
argument. It reports controller-only, mouse-only, or ambiguous equality and
logs whether the caller was a native site or a trampoline. All three operations
remain pass-through; build succeeded and the artifact test passed. Deployment
occurred only after confirming D2R and D2RLoader were closed. Built and deployed
SHA-256 match at
`5E8EDA07440ACB4AD1ACCB8FEA549B55037F85286B870CE6314ED7477F45BF00`.
The ga.20 DLL was retained as a rollback artifact, and Controller QOL remained
`1.3.1+rev.46` with protected SHA-256
`FE4A089E48C8E9860ADF38094255883008AAF1A6E83C075AF18F8DC3FB593C76`.
Loader admission and live coordinate comparison remain pending.
