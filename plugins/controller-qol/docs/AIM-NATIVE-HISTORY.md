# Controller aim native research history

Archived prototype evidence, superseded for current ownership/configuration by
[QOL rev.50](PRODUCTION-1.3.1-rev.50.md). Revision sections below are chronological;
old current/pending statements apply to their historical revision only.


**Current aim.10 retains the aim.5 XY wrapper and aim.7 accelerated cursor.**
aim.7 is the current pre-snapping rollback; aim.4 remains the HUD-only fallback.
The raw ray-hit ABI below is historical investigation, not an admitted direct call.
The SDK HUD clipping correction remains enabled and was visibly confirmed.

Recorded 2026-09-28. All RVAs below belong to the loader-hosted D2R main image,
not D2RCore, unless explicitly stated. The current process presents that image
as `D2RLoader.exe`; resolve its module base, never use fixed process addresses.

## Identity and reproducibility

The private reference is `runtime-game-auto-deposit-full-20260927.exe`, SHA-256
`A09EA269F8FC10C538DC9A05E4E81E004EC6DFC84AA0F685D3527A974A542A87`
(private capture identity; no captured image is distributed).
It is a private captured image, not a shipped executable identity. PE timestamp
is `0x6AB3782C`. No capture or extracted assets belong in source control.

Required deployed D2RCore.dll SHA-256:
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
The public `IsInGame` export is resolved by name (the reviewed Core also exports
it at ordinal 13). SDK ABI/resources are checked by the DLL artifact test.

Exact signature windows, byte lengths, and hook lengths are versioned in
the integrated `../src/aim/native_profile.h`. A window is evidence for
that interval only; some are prefixes or stop before the routine's end.
`../tools/audit_profile.py --pid <pid>` uses read-only process memory to compare
the same windows. It also accepts `--image <captured-image>` for static comparison.
Disassembly was produced with MSVC `dumpbin /disasm:bytes /range:<begin>,<end>`.
Private work products were named `aim-input-disasm.txt`, `aim-selection-disasm.txt`,
`aim-targeting-disasm.txt`, `aim-axis-adapter-disasm.txt`,
`aim-axis-dispatch-disasm.txt`, `aim-projection-disasm.txt`,
`aim-render-disasm.txt`, and `aim-unproject-disasm.txt`.

## Aim and cast contracts

All functions use Windows x64 calling convention. These are caller/disassembly
derived contracts; successful signature audits do not prove runtime ABI behavior.

| RVA | Contract / witness | Use and bounds |
|---|---|---|
| `0x190440` | `void LookupSkillTarget(controller, player, bool adjust, float* x, float* y)`; native descriptor at `0x1CD2C20` | Original runs once; replace returned ground coordinates only for local controller active skill 54/56; 15-byte hook |
| `0x190502` | Interior cast-adapter witness, within Lookup | Links active skill and native selected-target lookup; reference only, not a second hook |
| `0x1919F0` | `bool UnitTestSelect(controller, player, candidate)` | 14-byte hook scopes Meteor scoring with thread-local candidate type; native eligibility still runs |
| `0x18AF30` | `float PointScore(controller, player, float2* position, int category, int profile)` | 14-byte hook; negative-one rejection sentinel at `0x1CBD0F4`; circle score `37 - squaredDistance`, outside radius 6 gives -1 |
| `0x18B350` | Native unit scorer; call near `0x18B47C` | Owned by stable QOL; not hooked here. Fourth PointScore argument comes from native GetUnitTargetType and is a **category, not raw UnitType** |
| `0x1923B0` | Additional usage-type-3 candidate predicate | Reference only; Meteor eligibility remains native, full predicate semantics not claimed |
| `0x18DDE0` | `Unit* GetSelectedTargetForSkill(controller, skillId)` | Read current selected Meteor unit; prefix guard only; do not retain pointer |
| `0x4FDB40` | `int Cast(game, player, unsigned x, unsigned y, unsigned flag, selectedSkill*)` | 15-byte hook, observation only; selectedSkill +0 points to record with WORD skillId +0; original args and result untouched |

Native target caches and quick-cast scheduling may prevent this first experiment
from always selecting the nearest candidate. Existing visibility, hostility,
group and skill checks still restrict the candidate set. The geometry is a full
circle around the forward point; it does not remove all native enumeration
limits. Resolving a selected unit's current position is not proof of persistent
name-lock or a unit-targeted network cast.

Unit fields used: DWORD type `+0` (player 0 / monster 1), DWORD id `+8`, dynamic
path pointer `+0x38`. Dynamic-path DWORD X/Y are `+0/+4`, 16.16 world coordinates;
integer WORD components `+2/+6` supply cast-start measurements. Lookup's native
dynamic-path conversion is the witness; Y getter `0x3419F0` reads DWORD `[rcx+4]`,
X getter reference `0x2EF880`. Static object layouts are not reused for monsters.

## Normalized secondary-stick path

| RVA / field | Evidence and contract |
|---|---|
| `0x8B2D0`, global `0x2A23704` | Active controller index getter; reject index >=8 |
| `0x77E10` | Controller-mode bool getter, prefix guarded |
| `0x9A480` | Local player getter(index), prefix guarded |
| `0x144640`, global `0x2A4EAB0` | Initialized aim singleton getter |
| `0x1440D0` | Active skill getter(aim,index); per-index stride `0xB8`, field `+0x1974` |
| `0x1446C0` | ReadAim(aim,player,float2* position,float2* facing), existing portal research corroborates path |
| `0x13CE90`, global `0x2A4DC20` | Normalized input singleton getter |
| `0x13CFF0` | SecondaryAxis(input,float2* output,index), copies per-index stride `0x1C8` fields `+0x28/+0x2C`; output pointer return |
| `0x13F470` | Axis dispatcher(kind,float x,float y), reference only |
| `0xD2E490` | Movement-stick selection setting consulted by axis dispatch: movement fields `+0x20/+0x24`, secondary `+0x28/+0x2C` |
| `0xCC1E0`, `0x92A10` | Native blocked-input predicates in secondary-axis getter; exact menu coverage unverified |

The live read-only idle sample returned activeSkill -1 and secondary Y 0.
Therefore only the UI preview may fall back to the last test skill. Native
Lookup/UnitTest require actual active 54/56; idle interact must not become Meteor.
The SDK UI thread samples normalized axes, preserving the game's swap-stick
setting. Y polarity needs live verification (F10). No XInput hook is installed.
Only elapsed steps <=100 ms integrate distance; 0.22 dead zone, maximum rate
20 tiles/sec in aim.2 (10 in aim.1), bounds 3–30. Lost focus, session exit, invalid values, absent
player, unsupported skill or non-controller mode stops modification.

## Experimental ground marker

Optional renderer prefix `0x7E65A0` yields the renderer pointer. Native routine
`0x7E7E50` is modeled as `bool(renderer, alignedFloat4* hit, const uint64* query,
uint64 packedFloat2ScreenPoint, bool ground)`. Witness `0x7E7CE0` copies renderer
field `+0x128` to an eight-byte local query object, passes its address in R8,
and retains the incoming packed screen point in R9; it multiplies hit X/Z by
0.5 (constant `0x1CBA900`). **aim.1/2 misidentified these two arguments** as a
screen-point pointer and packed screen dimensions. aim.3 corrects them.
At `0x7E7F56`, the saved R9 address becomes the second argument to camera-ray
routine `0xED62B0`; that routine divides its input X/Y by camera width/height
fields `+0x150/+0x154` at `0xED62EA..0xED631A`. This proves the input is a point,
not viewport dimensions. The `0x7F350` screen-size interpretation previously
recorded here was incorrect and is not used for this call.
The separate `0x8F220` investigation did not establish
a usable direct forward-projection contract; no hook/call to it is installed.

At most every 100 ms on the SDK UI thread, sample four screen points, validate
the fourth against an affine map (<=0.5 tile error), validate center proximity
to player (<=20 tiles) and sample spacing (1–50 tiles), then invert that map.
Invalid/degenerate mappings hide the marker. These checks cannot establish full
terrain/perspective accuracy. Unprojection ABI, performance and circle alignment
are **not live tested**. This visualization never feeds casting coordinates.
Render callbacks consume only copied values; native pointers are not published.

## QOL coexistence and failure behavior

The native UnitTest window includes two contact CALLs at `0x1922CD` and
`0x192378`, originally targeting `0x34BC90`. Current QOL rev.48 rewrites their
relative displacements. Only these eight operand bytes are exempted from direct
window comparison; both E8 opcodes and every other byte must match.

An altered contact target must be a 14-byte `FF 25 00 00 00 00 <destination64>`
relay leading into loaded `Controller QOL Updates.dll`, whose file SHA-256 must
be `E4D2452B0B9E8BB4BD121E25A903254346B0FCE0B609AE19734C67785FC937D4`.
The hook does not rewrite the calls, relay, shared contact entry, QOL scorer,
or action handlers. Future QOL versions require revalidation. This deliberately
does not admit arbitrary detours merely because the entry bytes match.

All main sites are checked before installing SDK-owned hooks. An admission
failure leaves any installed wrappers pass-through. Optional projection mismatch
disables the marker independently. Unload disables modifications before SDK
ownership cleanup. No direct packet or server-rule modifications are made.

## Skill data and validation boundary

The inspected Reimagined source table is
`data/hd/global/excel/controllerskillsettings.json`: Teleport 54 uses usageType 0,
defaultTargetDistance 23, alwaysIgnoreTarget true, validationFlags 0; Meteor 56
uses usageType 3, defaultTargetDistance 15, acceptableDistanceOffset 6,
stickyFreeTarget true, validationFlags 3. Both targetGroup 0. This records the
source table inspected, not proof of which packed table a running game loaded.
No data edit is part of this experiment.

MSVC warning-clean build and both policy/artifact tests passed. The read-only
live audit passed all 17 windows, including reviewed QOL relays. The live audit
did not install hooks or invoke these functions. Loader admission of this DLL,
actual cast distance, final displacement, target switching/name-lock, menu
isolation, and visible marker alignment remain pending a restart/live test.

### Live follow-up and aim.2, 2026-09-28

The plugin-specific log is `d2rloader/logs/controller-aim-test.log`, not the
general loader log. aim.1 hook installation was logged at 07:37:18.802, toggles
at 07:37:55.264 (off) and 07:38:34.388 (on). Subsequent native Lookup records
showed Teleport ranges 20, 23.81 and 30; Meteor scoring was observed as well.
The user reported the controls working, but neither the HUD nor circle visible.
No successful nearest-enemy selection or marker alignment is inferred from that.

aim.2 keeps the same native contracts and guards. Maximum adjustment rate is
doubled. Display status no longer depends on a fresh aim snapshot, and an
independent screen-space slider accompanies the optional ground distance line.
Text/line SDK result codes are logged (0 means submission success, not visible
presentation), plus projection stages 0 metrics unavailable, 1 renderer/size
unavailable, 2 ray miss, 3 invalid world coordinates, 4 geometry checks failed,
5 singular transform, 6 ready, 7 caught exception. Logging is bounded per session.
This instrumentation is needed to isolate the missing-overlay cause; it is not
evidence that presentation has been fixed. No new native address was introduced.

### Invisible SDK drawing: confirmed clipping mismatch, aim.3

The public SDK is 0.3.0 / ABI 4 at pinned commit
`bb0b48e080c51c37fc8296ee415d9daaba7ee33d`, matching the version described in the
[official SDK](https://github.com/D2RLoader/PluginSDK#display-only-overlays).
aim.1 onward already use its OverlayService, canvas handles and frame callback.
This is not a missing SDK migration.

On the exact Core hash above, export `FinalizeImGuiFrameWithPluginOverlays`
RVA `0x836540` calls overlay dispatcher `0x4411C0`, then the original game frame
finalizer via Core slot `0x7050F0` (game RVA `0x6590D0`). Core's OverlayService
table is at `0x634C70`; drawLine wrapper `0x442420` calls implementation
`0x4430B0`, which reaches native AddLine through Core slot `0x709C20`
(game RVA `0xCFE590`) at Core `0x4433BB`. Its color converter `0x4434B0`
produces packed RGBA. These are investigative references only, not patched sites.

At Core `0x4413DC..0x4413EA`, the dispatcher passes **addresses** of two float2
locals in RDX/R8 to clip-push slot `0x709BA8`, mapped to game `0xD03A30`.
That native function stores RDX/R8 directly and reads their component float bits
at `0xD03A3C..0xD03A70`: the actual contract is
`void PushClip(drawList, uint64 packedMin, uint64 packedMax, bool intersect)`.
Passing pointers supplies pointer bits as tiny coordinates and clips drawings
away despite success codes. The SDK's own clip API would use the same bridge.

Read-only live inspection while the game was focused found window
`##D2RLoaderPluginOverlay` correctly sized 2429x1200 in that session, with a
nonempty draw list (24 vertices, 54 indices) but malformed near-zero clip bounds.
When unfocused, this plugin submits nothing and the list becomes empty; sampling
then cannot diagnose the active draw state. The user screenshot corroborates
that neither HUD nor circle was visible. Command submission is not presentation.

`tools/read_overlay_state.py` records those values without invoking game code.
Build-specific read-only layouts: game ImGui global pointer `0x34F0E50`, context
frame counters `+0x1A08/+0x1A0C/+0x1A10`, windows vector `+0x1A28` (size/capacity,
data `+0x1A30`), window name pointer `+0`, flags `+0xC`, draw-list pointer `+0x2B0`.
Native current-window draw-list getter `0xCEAED0` witnesses the latter field.
Draw list vectors: command `+0`, index `+0x10`, vertex `+0x20`, each size/capacity/
pointer; first draw command has float4 clipping at `+0`, element count at `+0x20`;
vertex format float2 position, float2 UV, packed color (20 bytes). These are not
general ImGui SDK layouts and must not be carried to other builds without review.

aim.3 uses only the exact Core hash and guarded native getter `0xCEAED0`,
clip push `0xD03A30`, clip pop `0xD03760` (Core slot `0x709B90`). During its own
SDK callback it pushes a full-screen clip **by value**, without intersecting the
broken parent clip, draws through SDK calls, then pops on every return via RAII.
The loader's parent clip remains intact; no Core memory, file, imported function
pointer, or global hook is replaced. Guard failure skips this correction and
logs `clipCorrection=0`. Signatures are in the native profile; the new guarded
`0x7E7CE0` witness separately admits corrected picking. `audit_profile.py
--render-only --pid <pid>` audits all six optional render windows without
misreporting the already-installed aim hooks as new incompatibility.

Automated build/artifact checks and static ABI witnesses support aim.3. Until
the corrected version is observed, visible text/slider and ground alignment
remain pending, especially over uneven terrain or at changed camera zoom.

Live follow-up: aim.3 installed at 17:39:27.293; first frame logged textResult=0,
lineResult=0 and clipCorrection=1. The six optional render guards passed against
the running game. The user explicitly confirmed both the ON/OFF label and slider
visible. This validates the HUD clipping correction; it does not by itself
validate the ground picking or projected marker alignment.

### aim.3 crash and aim.4 recovery

The user reported an immediate crash when casting Teleport. Windows Application
events at 17:39 and 17:40 attributed `0xC0000409` / BEX64, fast-fail reason 2,
to `Controller Aim Test.dll` version 0.1.0.3, fault RVA `0x562D`. Failed DLL hash:
`097B81B2ACFBF6CC0E7974C5BA0EF93AA659C901B1B7F8BCB47970F0701E03B6`.
The last plugin records were Lookup and cast, with no completed projection
diagnostic. A stack-cookie failure is consistent with the ray-hit output
overrunning aim.3's 16-byte local. This is a strong diagnosis, not a recovered
complete native output layout. The native wrapper allocates substantially more
stack space after its output pointer than aim.3 did. Merely enlarging that buffer
would not establish the full output/ownership contract and is not the remedy.

aim.4 removes all calls to native Unproject; `ProbeProjection` now returns false
with status 8 (disabled). No user action or key can re-enable it. Historical byte
profiles are retained for research only. Teleport/Meteor aim logic and the
SDK-backed HUD/clip correction are retained. The failed build is stored with
`.failed.dll.disabled` and its runtime archive renamed `UNSAFE-DO-NOT-INSTALL`.
Both build suites passed for aim.4. The user relaunched and explicitly confirmed Teleport and HUD work together. Ground picking remains disabled.

### aim.5: use the native output-owning wrapper

On the user's request for further ground-marker iterations, reviewed the complete
`0x7E7CE0..0x7E7D69` wrapper again. Its guarded 138-byte window covers the entire
routine. Contract: `bool(renderer, float* worldX, float* worldY, uint64 packedScreenXY)`.
It saves RDX/R8 as the two scalar-output pointers, supplies its own stack storage
at rsp+0x40 to the inner picker, and explicitly writes just four bytes to each
caller output at `0x7E7D44` and `0x7E7D48`. The wrapper performs the X/Z-to-world
0.5 scaling itself. Its stack allocation is 0x98 bytes with the cookie at rsp+0x80;
the plugin no longer owns or guesses the raw collision output size.

Further inner reference `0x98E8F0` receives the output pointer in RDX and retains
it in R14, then places it in a callback context at rsp+0x38 (`0x98EA05`), traversing
collision structures through `0x983C00`. This reinforces that raw output is not
a standalone float4 contract. No direct call to either routine is added.

aim.5 calls only `0x7E7CE0` on the SDK UI thread, with packed float2 screen pixels
by value in R9 and separate float outputs. Four samples (15 percent of the smaller
screen dimension apart) must satisfy world bounds, player proximity, spacing,
affine fourth-point error <=0.5 tile, and nonsingular inverse checks. Rendering
uses copied snapshots. Sampling is limited to once per 100 ms. Existing renderer,
inner picker and complete wrapper signatures all remain required; no new address
is admitted. Build and artifact tests pass; live stability/alignment pending.

Live aim.5 follow-up: installed at 17:48:10.814; projection reached status 6
(ready) at 17:48:22.834. The user explicitly reported stable operation and that
the line/circle follow distance. One Teleport sample reported requested 19.24
tiles and observed displacement 19.82 at 281 ms; this is supporting observation,
not an exact landing measurement. Initial casting and visual distance tracking
are confirmed; varied terrain, zoom, long-session stability and Meteor snapping
remain separate validation tasks. aim.4 remains the HUD-only rollback.

### Meteor snap instability reported after aim.5 validation

The user observed a brief flick toward an enemy inside the circle, character
jitter, and a cast near the circle center rather than on the enemy. Runtime
scoring logs at 17:55:17 and 17:55:24 showed positive circular candidate scores
(19.455, 18.015, 18.633), so the custom scoring branch did execute. This does
not establish that the same candidate became the final cast destination.
Lookup logging was already capped after eight samples; the existing logs do
not pair every later candidate/selection/coordinate cast into one transaction.

Both ReadView and UiTick recompute center from current native facing. A native
target-induced facing change could therefore move the center and alter selection
again; this is a feedback-loop hypothesis, not a verified native turn trace.
No facing-only casting restriction has been demonstrated. The observed native
coordinate-cast contract accepts X/Y, but that alone does not establish absence
of all controller-side direction or target-cache constraints.

Next experiment, implemented in aim.6: persist a player-relative 2D aim
offset controlled by both secondary-stick axes inside an outer radius; keep the
small enemy snap circle centered on that independent aim point, and capture one
resolved destination for a cast. This separates facing/selection feedback from
the remaining native-cache/dispatch investigation. Stable target retention and
paired aim/selection/final-coordinate diagnostics should be evaluated separately.

### aim.6: independent free-placement experiment

The first stage disables plugin candidate scoring and selected-monster resolution;
native UnitTest/PointScore routines still run. Lookup still calls native once,
then writes player position plus a retained offset to its X/Y outputs. The shared
cast observer remains diagnostic only: downstream native coordinate rewriting or
selected-target precedence has not been ruled out.

Both secondary-stick components now use the existing measured projection basis
to turn screen direction into a world-space direction. Positive stick Y defaults
to screen-up; F10 reverses it. Deflection outside the 0.22 deadzone maps linearly
to 3–30 world tiles; diagonal magnitude is clamped to one. Invalid projection
retains the last offset. Facing is used only to seed/reset the 20-tile offset.
Offsets and an 80 ms inward-deflection latch are copied under the state lock;
native calls occur outside that lock. Neutral stick retains the offset. Movement
translates the aim point with the player; facing changes do not rotate it.

The rendered circle is now centered on the player with radius 30. The small
snapping circle is omitted while plugin snapping is disabled. No native addresses,
guards or ABI contracts were added. Policy tests cover cardinal screen directions,
diagonal range, partial tilt, neutral retention and release filtering; DLL artifact
checks and warning-clean build pass. Live side/rear Teleport and Meteor destination
alignment remain pending. Preserve the user-confirmed aim.5 as rollback.

Deployment: game/loader processes were absent; aim.5 was backed up as
`Controller Aim Test.aim5.rollback.dll.disabled` with SHA256
`5BD1213A9777F5D6DA8E1EEC100B0D9AD5A3FA0788CB285C8956779C44A0B23F`.
Built/deployed aim.6 match SHA256
`D436FE5B7A2232FC467C32D0FB65ACD8B7EDA3166E8AC1EF8C712955D906FCD3`.
Controller QOL's DLL hash remains unchanged. This verifies installation, not
loader admission or visible gameplay behavior of aim.6.

### aim.6 user confirmation and aim.7 motion refinement

The user confirmed independent-point casting works and the character turns to cast
as expected, including turning around. This supports the tested coordinate path;
it is not proof about every skill or controller target cache. The user reported
almost all-or-nothing radial control with light stick movement. Whether the native
secondary-axis values saturate early has not been measured; do not claim raw input
normalization as a verified cause.

aim.7 retains the native coordinate and projection paths. It integrates stick
direction as cursor velocity, with squared remapped deflection and a time-based
linear speed ramp. Default radial dead zone is 0.22, full-tilt speed starts at
4 tiles/sec and reaches 28 after 0.65 seconds. Even saturated input begins gently.
Release stops motion immediately. Direction changes exceeding 60 world-space
degrees reset the ramp; invalid state/projection and gaps over 100 ms also reset it.
The cursor crosses zero radius and is clamped at 30. The obsolete 80 ms inward
latch and absolute-distance mapping are removed. No new RVAs or ABI details.

Plugin-owned configuration uses SDK ReadConfig and embedded resource 0x03EA,
creating `d2rloader/config/controller-aim-test.toml` through the existing loader
facility. Only documented numeric [aim] keys and comments are parsed; invalid
configuration uses defaults and logs a warning. Settings are immutable after load.
Tests exercise frame-rate independence, bounded full-input taps, release, drift,
center crossing, range limits, zero ramp, parser rejection and embedded defaults.
Live aim.7 feel and configuration loading remain pending.

Build and both policy/artifact suites pass. With game/loader closed, aim.6 was
preserved as `Controller Aim Test.aim6.rollback.dll.disabled`; built and deployed
aim.7 SHA256 match
`6F4AB1C48ED50CE602CDF1440186E989909C935F8EA7D3472B520F880E847CB8`.
The previously absent plugin-owned config was installed with documented defaults;
Controller QOL DLL hash is unchanged. No other plugin configuration was edited.

### aim.8: Meteor snapping independent of facing and native selected-target cache

User confirmed aim.7 cursor feel with deadzone 0.22, initial speed 25, maximum
speed 100 and acceleration time 0.05; deployed TOML was read and matches these
values. Preserve it byte-for-byte during this upgrade.

No new entrypoint or unit field is introduced. Existing UnitTestSelect 0x1919F0
still executes once; its Meteor-only PointScore geometry override is restored.
Reviewed existing captured disassembly witnesses: 0x1921F1 calls PointScore and
0x1921F6 retains the float result; 0x192224 writes that result into native candidate
data. 0x191BD0 clears DIL on rejection, while 0x192391 sets DIL to one and jumps
to the shared epilogue. These are inside the existing guarded UnitTest window.
The remaining native checks still execute; a true predicate result is evidence
of that predicate's acceptance, not proof of final network targeting eligibility.

After predicate acceptance, read the already-reviewed monster position/ID fields
again and store only copied coordinates, player ID, monster ID and timestamp in
a bounded 128-entry book. Rejection removes that candidate. No unit/controller
pointer is retained. Existing Lookup runs once, then resolves the nearest fresh
observation around the independent cursor and writes a single coordinate pair.
GetSelectedTargetForSkill is no longer used to pick the snapped destination; its
historical guard remains part of the unchanged profile. Native target caches,
character facing, packets and the final cast routine are not rewritten.

The retained target is preferred until a challenger is more than 1.5 tiles closer
to the cursor. Every chosen position must be no older than 150 ms, within 6 tiles
of the cursor and within 30 tiles of the player. No stale grace-period cast is
allowed. Ground fallback occurs when no observation qualifies. UI and Lookup use
the same policy under stateLock; snapping never writes retainedOffset. Left/right
and rear turn-to-cast therefore cannot rotate this aim point. F8/F9/session changes
clear observations, and invalid preview clears them. Teleport does not snap.

The preview is opportunistic: native enumeration is still responsible for which
candidates are visited and when. Idle preview may have no eligible observation;
do not claim all nearby enemies are enumerated or continuous name-lock is proven.
Cast diagnostics compare same-player/same-skill lookups within 200 ms, explicitly
labeled a temporal match rather than transaction identity. Cast remains pass-through.

Tests cover acquisition, retention, deliberate switching, rejection, refreshed
positions, expiry, player isolation, cursor escape, outer range and all four aim
directions without a facing input. Warning-clean build and both suites pass;
live Meteor target acquisition, marker visibility and impact alignment are pending.

Installed with game/loader closed; aim.7 preserved as
`Controller Aim Test.aim7.rollback.dll.disabled`. Built/deployed aim.8 SHA256:
`1E327043CB57194EFBF6DF031BA5C8D77F40E1371486E162621CABD7E736A15B`.
User TOML remained byte-identical (SHA256
`34C3C5B840E47741AC63BC8618948310BD6D5D8E8C59E5069B773D48A3A9A74E`);
Controller QOL DLL hash remains unchanged.

### aim.9: observed candidate acquisition occurs after coordinate lookup

User reported green snapped markers but impacts at the unsnapped center in aim.8.
Runtime 18:36:41.160: Meteor lookup #2 chose ground (6218.50,5157.95). Accepted
candidate #5/id18 appeared later at 18:36:41.227 and candidate #8/id17 at
18:36:41.309. At 18:36:46.260 lookup #3 again chose ground, followed by cast #2
at 18:36:46.274 with (6222,5172), then candidate observations #16 onward after
18:36:46.293. All eight recorded lookups were ground. This establishes late
observation relative to lookup in that run; it does not establish an engine
overwrite of an already-snapped coordinate. Rounded coordinate differences and
one approximately 2-tile discrepancy remain separate from this timing failure.

Cause in plugin scope: UnitTest observation required ReadView with an actual
active Meteor skill, so idle native enumeration could not populate the snap book.
UI subsequently resolved later observations and painted a green destination that
had not been available for the earlier lookup. aim.9 permits passive observation
when active skill is -1 and preview skill is Meteor; it leaves original native
scoring untouched in that state. Actual Meteor still enables the existing circle
score override. Teleport, Interact and all other active skills are excluded even
if preview previously held Meteor. Lookup still requires the actual active skill.

No new address, call, ABI, unit field, native predicate re-invocation or final-cast
rewrite is added. In particular UnitTestSelect remains invoked only through its
natural caller: it updates native selection state and is not a pure validator
(see Controller QOL's PORTAL-PRIORITY.md). Idle acceptance remains subject to native
geometry and checks; this change does not prove continuous 360-degree acquisition.
Existing 150 ms expiry and copied-ID/position lifetime rules remain in effect.

Tests cover passive-versus-active observation mode, no preview inheritance by
other active skills, availability at the next lookup, and stale expiry. Build
and both policy/artifact suites pass. Visible pre-cast marker/impact alignment
and runtime idle observation still require user validation.

After user confirmed closed and process absence was verified, installed aim.9;
built/deployed SHA256 is
`1F678BFF16B5D06FE73ED3A56D5D2F88E38713DF4F879E785532BE594DDBA7A1`.
aim.8 is preserved as `Controller Aim Test.aim8.rollback.dll.disabled`; the
confirmed aim.7 control-only rollback is also retained. User TOML and QOL DLL
hashes remain unchanged.

### aim.10: selected-unit route competes with coordinate placement

User reports aim.9 still casts at the unsnapped point and native directional
name targeting can take over when an enemy crosses the character's facing line.
Runtime now proves some lookups resolve monsters: #4 at 18:42:54.440 selected
id22 (5644.06,4709.57); #15 at 18:43:38.238 and #16 at 18:43:44.130 selected
id18. None has an adjacent coordinate-cast log, whereas multiple ground lookups
do. This supports a competing unit-target route; it does not by itself identify
every dispatch branch or prove a cast was submitted on each lookup.

Existing Lookup disassembly at 0x19056B..0x19057A retrieves active skill then calls
GetSelectedTargetForSkill at 0x18DDE0. 0x190592 retains the returned unit in RBX;
0x1905A7 tests it. This establishes selected-unit input to the existing targeting
path. The fully reviewed caller is already in the Lookup signature window.

aim.10 adds an SDK inline hook at existing profile RVA 0x18DDE0 with 14-byte
instruction-aligned prefix: `48 8B C4 89 50 10 55 53 56 57 48 8D 68 B8`.
Contract remains `Unit*(controller, int skillId)` on Windows x64. Existing 32-byte
SelectedBytes prefix and exact Core identity remain required. The original getter
runs once, then returns null only when ReadView admits local in-session foreground
controller state and the requested skill equals actual active 54/56. Preview -1
cannot authorize suppression. Mouse and other skills preserve the native result.
Original target pointers are neither dereferenced by this hook nor retained.

This directs the prototype toward the native coordinate fallback while Lookup
supplies its resolved destination. It does not force character rotation, rewrite
native selection caches or mutate final-cast arguments. The existing final Cast
hook stays observation-only. A new bounded `coordinate route` diagnostic reports
when a non-null native target was suppressed. Hook conflicts refuse installation;
QOL's comparison/scorer hooks are not replaced.

Tests cover requested/active skill matching and idle/other-skill exclusions;
warning-clean build and both suites pass. Actual dispatch route, coexistence,
impact alignment and left/right turning still require live testing.

User confirmed closed; process absence verified before installing aim.10.
Built/deployed SHA256:
`77EF32425A7A6C593F0A4FA0E1A531E77ADF04B87F7FA4351B9120A5F7BFA622`.
aim.9 preserved as `Controller Aim Test.aim9.rollback.dll.disabled`; the confirmed
aim.7 rollback remains available. User TOML and QOL DLL hashes are unchanged.

### aim.11: separate selected preview from submitted coordinates

User confirms aim.10 ground casting is no longer overridden by enemies crossing
the facing direction, but reports inconsistent green-endpoint placement. Runtime
records nine snapped coordinate casts (#4,6,11,13,19,26,27,28,33), each within
0.02–0.73 world tiles of its temporally matched lookup. Lookup #61 targeted id20
5.87 tiles from center, followed by cast #33 with error 0.02. Thus near-edge
snapping can reach the coordinate path. Alternating ground/snapped lookup choices
remain; these records alone do not identify expiry versus native rejection versus
radius exit, nor confirm eventual impact or damage.

aim.11 changes diagnostics only. SnapBook counts rejected freshness/geometry/range
stages without changing ranking or thresholds; last native rejection records an
ID and time but may concern another enemy. A same-player/same-skill lookup within
200 ms permits displaying actual submitted integer coordinates for 2.5 seconds
as magenta LAST CAST. This temporal classification is not transaction proof. The
marker remains fixed in world space while green preview can follow later movement;
it does not imply that originalCast succeeded or that an impact/hit occurred.
F8/F9/session changes clear the marker. No new native hook or ABI is introduced.
Build and both suites pass; live diagnosis still required.

Game/loader processes were absent before deployment. aim.10 preserved as
`Controller Aim Test.aim10.rollback.dll.disabled`; built/deployed aim.11 SHA256
`25C30A54AFE0E090F0105DAAE786D587FEB25FD9622DF5478DF88D759829249C`.
User TOML and QOL DLL hashes remain unchanged.

### aim.12: align idle Meteor preview geometry with active Meteor

User reports magenta is the actual impact location. Within about 20 tiles it
matches the green enemy endpoint; beyond that it falls back to center while green
can still appear. aim.11 logs at 18:57:52..18:58:04 show cursor ranges 26.64 and
21.92, zero expired and zero outsideRange observations, with either zero fresh
candidates or one fresh candidate outside the circle. At 18:58:12 onward, range
17.98 acquires id24 and submits its coordinates. These observations exclude the
plugin's 30-tile cap and expiry as the logged fallback reasons for these samples.
They support acquisition-state disagreement, not a hard Meteor cast limit.

Code mismatch: passive mode intentionally left native PointScore unchanged,
whereas active Meteor replaced it with circular geometry. Native rejection then
invalidated the candidate immediately. aim.12 enables the same bounded TLS circle
override for both modes, scoped to observed monsters after ReadView admits either
actual Meteor or activeSkill=-1 with established Meteor preview. Other active
skills remain excluded. Original UnitTestSelect still runs once with native
eligibility checks; this deliberately changes idle monster candidate ranking in
the existing native tables. It does not claim idle monster targeting is unchanged.
No native persistent profile field is widened and no final cast is rewritten.

Further reviewed static context, not changed/admitted as new calls: native
ScoreUnit 0x18B350 returns PointScore result after call 0x18B47C. UnitTestSelect
also has a mode-1 distance branch at 0x191DEA..0x191E1A: position is measured from
controller float2 +0x184C/+0x1850 and compared with float +0x1854 at 0x191E0C,
rejecting at 0x191E1A if outside. No live value or invocation of that branch for
this failure has been established. That independent native gate is not bypassed;
do not claim all native range restrictions have been eliminated by PointScore.
These witnesses come from the existing captured aim-targeting disassembly and
UnitTest guard window; no additional RVA, signature or hook is introduced.

Preview-circle candidates near the cursor now get a separate capped diagnostic
budget. Build and both policy/artifact suites pass. Test 18/22/26/29-tile targets
and compare preview, lookup, magenta and impact; live correction remains pending.

After user confirmed closed and process absence was verified, installed aim.12;
built/deployed SHA256
`E88691C61592BCDD4350D2ABB743B2A22F4031392752D70F06C98E8AC63CB9D5`.
aim.11 is preserved as `Controller Aim Test.aim11.rollback.dll.disabled`.
User TOML and QOL DLL hashes remain unchanged.

Live follow-up: after the requested 22–29-tile retest, the user reported aim.12
"looks to be doing the trick." Record this as initial user-confirmed correction
of the extended-range preview/cast snapping mismatch, alongside the earlier
confirmed aim.10 fix for directional targets overriding ground casts. aim.12 is
the current working baseline. This does not establish exhaustive terrain, enemy,
skill, multiplayer or long-session coverage.

### aim.13: prepare first Meteor after Teleport

User clarified that the visual circle disappearing was acceptable; the actual
failure was Teleport -> move cursor near enemy -> first Meteor does not snap.
The uninstalled visual-only draft was discarded. Source cause: MeteorObservation
allowed idle preparation only when the preview skill was Meteor. Teleport changed
previewSkill to 54, so native enumeration could not replenish the snap book before
that first Meteor lookup.

Idle preparation now admits activeSkill=-1 with either supported preview skill
54/56. The same scoped circle geometry is used, but active Teleport still yields
ObservationMode::None, Lookup only snaps Meteor, and UI draws yellow/green only
for Meteor. No change to native calls, guards, cast routing, cursor settings,
visuals or snap lifetime. Other active skills and an unestablished preview remain
excluded. Tests cover idle-after-Teleport admission and active-skill exclusions;
warning-clean build and both suites pass. First-Meteor-after-Teleport needs a live
retest; aim.12 remains the confirmed extended-range baseline.

After user confirmed closed and process absence was verified, installed aim.13;
built/deployed SHA256
`DF3DA6AFFFA3865BAA3A186B3C8E583218EC20C196D7035226B546D080DFF4F1`.
aim.12 preserved as `Controller Aim Test.aim12.rollback.dll.disabled`.
User TOML and QOL DLL hashes remain unchanged.


### aim.14: skill registry, display sampling and QOL ownership

User confirmed aim.13 improves first-Meteor-after-Teleport behavior. aim.14 adds
an explicit allowlist verified against installed Reimagined
`mods/Reimagined/Reimagined.mpq/data/global/excel/skills.txt`, column `*Id`:
Multiple Shot 12, Guided Arrow 22, Fire Wall 51, Teleport 54, Meteor 56,
Blizzard 59, Hydra 62, Teeth 67. Teleport remains ground-only; all others use
circle snapping with ground fallback. Idle preparation admits any established
supported preview, while active unsupported skills remain excluded. Selected
unit suppression requires the actual active supported skill. No data is edited.

Configuration in `d2rloader/config/controller-aim-test.toml` adds boolean
snapping_enabled/overlay_enabled/debug_overlay and numeric snap_radius,
switch_advantage, projection_hz, overlay_smoothing_ms. The copied candidate book,
150 ms freshness and 30-tile outer limit remain. Overlay disabling does not stop
UI input updates. Debug detail is off by default.

The existing admitted four-point XY wrapper is now sampled at configurable
10..120 Hz (default 60 instead of 10). Frame rendering uses a separate exponential
projection filter (default 35 ms). It never feeds back into cursor movement or
snap/cast coordinates. A gap over 250 ms, world-origin jump over eight tiles,
viewport origin/step change or invalid inverse resets it. No additional native
entrypoint or ABI is used; sampling cost and visible smoothness need live checks.

Private export `unsigned __cdecl D2RControllerAimOwnsGuidedArrowV1() noexcept`
returns 1 only when installed/enabled/in-session, using atomic reads alone.
QOL rev.49 resolves it with a temporary module reference and yields its old
short-point expansion only for its existing controller Guided Arrow route.
Missing/inactive export retains QOL behavior. Full ABI/lifetime details are in
`../../controller-qol/docs/PRODUCTION-1.3.1-rev.49.md`.
The existing contact-hook admission keeps its relay/owner checks and accepts
QOL rev.48 or exact rev.49 SHA256
`5C5F254C5628DA7D0DCB07886D3D4DC2FE602F2B6AB449CF2F9C06D58C0F355C`.
No new native address, hook or layout is introduced.

Both aim suites and 21 QOL suites pass. Added policy coverage includes all eight
skills, active-skill isolation, configurable snapping, strict config validation,
and display-filter reset behavior. Artifact coverage checks the new export and
inactive ownership. Active coexistence, six new skills and visible smoothness
remain unverified in-game. Preserve aim.13 and QOL rev.48 as a paired rollback:
aim.13's exact hash admission does not include rev.49.

Deployment: game/loader process absence verified, then installed aim.14 SHA256
`4B2A4FA93AB9306C9A51FDCD52E36EB23E557791FD3F16CD98F016F1C12CA261`
and QOL rev.49 with the hash above; deployed hashes match both build artifacts.
Preserved `Controller Aim Test.aim13.rollback.dll.disabled` and
`Controller QOL Updates.rev48.rollback.dll.disabled` in the plugin directory.
Backed up `config/controller-aim-test.aim13.rollback.toml`, appended only missing
new settings, and verified user motion values 0.22/25/100/0.05 are unchanged.
QOL configuration hash is unchanged. No live aim.14 result is claimed.

### aim.15: Whirlwind ground endpoint experiment

Installed Reimagined row `skill=Whirlwind`, `*Id=151`, `charclass=bar` has
srvstfunc=38/srvdofunc=76 and cltstfunc=31/cltdofunc=45, TargetableOnly=1.
These are table function indices, not native RVAs or proven call contracts.
Controller JSON record name=Whirlwind/id=151 has alwaysIgnoreTarget=true,
ignoreTargetOnHold=false, defaultTargetDistance=10, acceptableDistanceOffset=6,
usePredictiveTargeting=false, useStickyFreeTarget=false, targetGroup=0,
usageType=0. Static data supports a ground-only first experiment; it does not
prove which execution route consumes each field or how held input behaves.
Installed paths relative to the Reimagined MPQ data directory and SHA256:

- `global/excel/skills.txt`: `818011AD76D0800FFDC6E261C175802607B6979E4A4AB274B93826D6B677498C`

- `hd/global/excel/controllerskillsettings.json`: `4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79`

Only the plugin allowlist/ground-only policy is expanded: actual active skill151
can use existing Lookup coordinates and selected-unit suppression, active151
cannot enable circular scoring, and idle after151 can prepare candidates for the
next snap-capable skill. Existing local/controller/build admission is unchanged.
No new native address, ABI, hook, offset, data edit or QOL change. The current
player-relative cursor is retained; neither an in-progress path nor its endpoint
is latched/rewritten. Existing pass-through cast diagnostics now observe151.
Both policy/artifact suites pass. Live path distance/direction, terrain, enemies
and held-button semantics remain pending; native movement constraints may still
limit the requested endpoint. aim.14 is the rollback baseline.

Deployment: process absence verified; built/deployed aim.15 SHA256
`381FE45881AF170F8A71025F512C484E2B8C4C26A8B1182EA65EB1E63EB402B7`. Previous aim.14 preserved as
`Controller Aim Test.aim14.rollback.dll.disabled`. Both plugin configurations
and QOL rev.49 DLL hashes are unchanged. Live validation remains pending.

### aim.16: retained Whirlwind pass-through snapshot

User confirmed aim.15 ends at its requested destination. Skill151 now admits
circular enemy scoring. Whirlwind-specific retention can survive the cursor
circle moving away: only the retained same-player enemy, with a fresh150ms copied
observation and within30tiles, gets candidate-centered TLS circle geometry.
Original UnitTest still runs once; native rejection invalidates that observation.
No retained native pointer or new RVA/offset/ABI is introduced. Stick above dead
zone restores cursor acquisition; changing resolved skill and existing lifecycle
resets clear Whirlwind retention. Other skills retain their circle-only policy.

Each Lookup computes player + normalize(enemy-player)*min(distance+3,30).
Distance below0.1, out-of-range or invalid world coordinates fall back to cursor.
The pass-through cast observer remains unmodified. This snapshots coordinates
per native lookup; it does not establish one lookup per spin or rewrite a running
path. Repeated/held native lookup timing remains a live-validation question.
Overlay is a next-lookup preview; green PASS THROUGH is the overshoot endpoint.
Both policy/artifact suites pass, including reversal, range cap, degenerate
geometry, cursor-drift retention, stale expiry and native rejection. Live
alternation and candidate refresh during movement remain pending.

Installed after process-absence check; built/deployed SHA256 `0147EFD94401E0AFF8DE3E9CBAE628091B5387F9F5F9EE63B2ABF30EBC5FFD8D`.
Rollback: `Controller Aim Test.aim15.rollback.dll.disabled`. Configurations
and QOL DLL hashes unchanged. Live validation pending.

### aim.17: shorter Whirlwind pass-through

User requested 1.5 rather than 3 tiles beyond the retained enemy. Lookup policy
now uses min(distance+1.5,30); no native contract, retention, hook or ABI changes.
Updated endpoint and reversal expectations. Live validation pending.

Aim.17 also adds Leap, verified in installed Reimagined
`data/global/excel/skills.txt`: skill=Leap, *Id=132, charclass=bar,
srvstfunc=40/srvdofunc=77, cltstfunc=29/cltdofunc=43 (table indices, not RVAs).
Leap Attack143 is separately identified and excluded. Installed
`data/hd/global/excel/controllerskillsettings.json` Leap132 has
defaultTargetDistance=16.6, useSkillScaledValueAsDefaultDist=true,
skillLevelLimitsScaledValueAsDefault=4, alwaysIgnoreTarget=false, validationFlags=3.
No data is edited. Existing guarded coordinate lookup and active selected-unit
suppression apply to132; active Leap excludes circle scoring, idle after Leap
prepares the next snap skill. Native landing checks remain. No new native
address/layout/ABI. Policy/artifact tests cover the expanded allowlist and Leap
isolation; visible distance and landing behavior require live validation.

Both aim.17 suites pass. Installed after process-absence verification;
built/deployed SHA256 `32915C9E24949D6592F5825339ED37DC3405CA5630F1BB88B9906C72253B6A8F`.
Rollback aim.16 preserved; configuration and QOL hashes unchanged.

### aim.18: configurable pass-through

[aim] whirlwind_pass_through_enabled defaults true; numeric
whirlwind_pass_through_distance defaults1.5, finite range0..15. Disabled passes
zero to the endpoint policy and uses the enemy position directly, including
coincident positions. Enabled positive distance retains degenerate-direction
fallback and 30-tile cap. Retention/scoring contracts unchanged. No native
address, offset, ABI or hook changes. Startup logs effective settings; render
label uses SNAP for zero/disabled extension. Both suites pass; live toggles pending.
Installed after process absence verification; previous aim.17 DLL/config backed
up as Controller Aim Test.aim17.rollback.dll.disabled and
config/controller-aim-test.aim17.rollback.toml. Existing config values/QOL unchanged.
Full rollback must restore both the aim.17 DLL and config because its strict
parser does not accept aim.18 keys.
Built/deployed SHA256: `5FCF40E5E7A48A965D090E489D3F1380F42AB8A9ADE7525B74A119B5B7A62B55`.

### aim.19: render-only reticle presentation, 2026-09-30

View now copies chosen.position into targetPosition under the existing state lock,
separately from destination (which can lie beyond the enemy for Whirlwind).
No native pointer is retained; no native ABI/layout, hook or guard changes.
Normal overlay draws four world-plane arcs at radius0.65 around cursor and, if
separate, the pass-through endpoint. Enemy lock uses screen-space diamond corners
at the projected copied enemy position. Bone/brass colors use dark under-strokes;
stroke/corner dimensions scale by clamped viewport height/1080 (0.75..2).
Existing display-only projection smoothing and clipping correction are reused.
All drawing uses the public SDK drawLine service. Debug mode retains old graphics.
Policy/artifact suites pass; no live visual validation is claimed.
Installed after process-absence check; aim.18 rollback preserved. Configs and
QOL DLL hashes unchanged. Built/deployed SHA256: `DA8F99FDAC02A005C8BF38C2BA087B5142AEDBC9E7EC4C5C0A71CCBE3B74F7D6`.

### aim.20: idle ground-skill lock preview

Candidate gathering already runs idle after supported ground skills, but
ResolveSelection returned before choosing a visible target for Teleport/Leap.
UI tick now explicitly permits idle (activeSkill=-1) ground-skill snap preview.
Native Lookup uses the default false flag and cannot enter this path. Active
Teleport/Leap still do not snap. Existing freshness/radius/range/snapping toggle
apply. This may prepare retention for the next snap skill, consistent with idle
acquisition. No native address/ABI/hook changes. Tests cover idle eligibility,
active ground-skill exclusion, unsupported skills and opt-in boundary.

Both suites pass; installed after process-absence check with aim.19 rollback.
Configuration/QOL unchanged. Built/deployed SHA256 `997E57E0B7EA9F9412C859A58CEBA6448BBF8054477675F6E5B49C8065F3DDD1`.
Visible idle-after-Teleport preview remains pending live validation.
