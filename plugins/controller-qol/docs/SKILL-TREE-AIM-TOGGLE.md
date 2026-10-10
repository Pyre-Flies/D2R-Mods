# Skill-tree R3 aim toggle, rev.56

Historical rev.56 record. Rev.75 moves all skill settings and R3 saves into a
mod-specific profile; the SDK-only persistence description below is historical.
See [current configuration and migration](SKILL-DISCOVERY-CONFIG.md).

2026-10-04. Build-specific contracts, not portable SDK widget layouts.
Game disk SHA256: `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7`.
Core SHA256: `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
The live game is hosted in D2RLoader.exe, with game base 0x140000000 in this
session. The on-disk game is packed; its raw handler bytes are not a witness.
The older private runtime capture was used only to locate candidates; all new
focus/record byte guards and listed widget ancestry were checked read-only live.
No game image or generated memory dump is added to this repository.

## Evidence and bounded contracts

- Existing Skill Tree handler: game RVA `0x14C6810`, `(panel,message)`;
  existing `skills_signatures.h` admission remains unchanged.
- Existing focus getter `0x846020` loads manager slot `0x3440170`, then manager
  `+0xD0` focus object. Full existing FocusGetter witness guards that chain.
  This feature reads the chain on the UI thread and calls no new native getter.
- Focus `+0x190` is the selected widget; `+0x188` its focused root panel.
- Focused skill widget vtable `0x1CE4148`; root tree vtable `0x1FD82B0`.
  Widget `+0x30` ancestry reached root through intermediate vtables
  `0x1D72670` and `0x1D82DA0` in both inspected selections. Runtime traversal is
  bounded to eight ancestors and must reach the exact focused root.
- Root vtable `+0x20` points to `0x14C6810`; skill widget vtable `+0x58` points
  to `0x86F0A0`. These slots are checked in admission. No virtual call is made.
- Native allocation path `0x14C6D62..0x14C6D74` reads skill widget `+0x668`
  record pointer and then its first dword as a skill ID:
  `48 8B 83 68 06 00 00 48 85 C0 0F 84 C1 00 00 00 8B 18`.
  The complete window is checked before enabling the shortcut.
- Runtime focus initially read ID 6; after the user moved to displayed Multi
  Shot it read ID 12 with the same widget/root contracts and a different widget.
  Multi Shot ID tracking is runtime observed. This does not prove every class,
  respec mode, custom tree, mod skill, mouse focus or future build.
- R3 uses existing normalized input mask `0x80`, not a newly inferred action enum.
  `ReadControllerInput` is sampled in the queued UI callback, gated by foreground,
  in-game and admitted controller UI mode. Press edges toggle once; entering the
  tree while already held is ignored until release. Native input is not consumed
  or remapped; user reports native R3 does not swap weapons while in this menu.
  Very brief presses between UI samples can be missed.

## State and persistence

The immutable parsed baseline now has atomic per-ID overrides (IDs 1..65534),
so hooks and UI can read updated modes without mutating shared configuration
arrays. Only a successful SDK WriteConfig publishes the new mode. The selected
catalog ID changes to its catalog Ground/Snap mode on enable; disable restores
native targeting. Snap candidates/Whirlwind retained lock and cast markers are
cleared after a saved toggle. It does not arm the stick or change master enable.

The rewrite preserves all other file bytes, including CRLF and inline comments.
It updates an existing numeric or accepted legacy-name key wherever its class
heading places it. Missing keys/sections are appended in their catalog class.
Duplicate/malformed skill settings and over-limit output are refused. Existing
SDK ReadConfig/WriteConfig remain the only persistence API; there is no reload
or replacement of unrelated settings. Native catalog IDs are supported; unknown
mod IDs require a chosen custom targeting mode and are deliberately refused here.
The gold status hint is independent of debug_overlay but honors overlay_enabled.
Global aim/QOL must be enabled and the integrated aim module admitted.

Guard mismatch disables only the tree shortcut, retaining gameplay aim. Existing
Core hash, controller profile and all required aim hook guards remain required.
No new hook is installed. Toggle/runtime behavior and persistence after restart
require a live test of rev.56; automated policy and artifact tests alone cannot
establish visible UI or native weapon-swap behavior.

Validation: all 23 suites passed, including shipped version/ABI/default config,
press/repeat lifecycle, CRLF/comment preservation, legacy keys, missing sections,
duplicate refusal and atomic mode publication. Verified game/loader closed before
local deployment; backup: local rollback controller-qol-rev56-20261004-083729.
Built/deployed SHA256: EEF1F07B99E0CB972DC4CCE7E8E19B99E342D4EC3778E6E2A03C0D8B6AD3521E.
Config byte comparison confirmed only the new true shortcut default/comment was
inserted, preserving every existing setting. User-visible toggle and restart
persistence testing remain pending.

## Rev.57 icon indicators and control-strip status

User confirmed rev.56 toggling works; restart persistence was not separately
confirmed. Rev.57 adds SDK drawing only, with no additional inline/byte hooks.
New discoveries were checked read-only in the same reviewed running game/Core:

- Common widget parent `+0x30`; enabled/visible bytes `+0x50/+0x51`; fill-parent
  flag `+0x52`; child array pointer `+0x58`, 64-bit count `+0x60`; local integer
  rectangle `(x,y,width,height)` at `+0x70`; float scale `+0x80`; normalized parent
  anchors at `+0x48/+0x4C`. Existing `+8` names remain bounded synchronous reads.
- Child cleanup witness `0x8547D9..0x8547EE`, constructor/visibility/scale stores
  `0x854780..0x854794`, rectangle getter `0x8562A0..0x8562E9`, origin math
  `0x856330..0x85640C`, ancestor scale math `0x1E6756..0x1E6785` are guarded in
  `tree_layout_profile.h`. These are inspection witnesses, not new call targets.
  No new native getter is invoked.
- Native origin path sums local positions and normalized anchors against the
  parent's effective dimensions, applying each parent scale. Fill-parent widgets
  contribute no local x/y and inherit dimensions. Geometry is calculated from
  copied fields in bounded ancestry (16 nodes), with cycle/finiteness/range checks.
- Observed Bow tab: `Tab2` vtable `0x1D82DA0`, visible `SkillButtonContainer`
  vtable `0x1D72670`, icon vtable `0x1CE4148`. Tab0/Tab1 nodes remained visible but
  their skill containers had visible=0. Traversal checks every ancestor and accepts
  only the reviewed root/tab/container/icon families; maximum 256 visited nodes,
  depth 5, child count 96, copied icon capacity 64. Generic/item skill button
  vtable `0x1CE4210` is intentionally not claimed as a supported catalog icon.
- Observed top icon ID6 local bounds `(601,182,132,130)`, Tab2 `(19,5,0,0)`,
  container `(0,0,0,0)`, tree `(304,179,1420,1420)` with x anchor 0.5,
  manager `(0,0,3586,2160)` with scale 0.48935184. All five fill flags were zero.
  Computed screen origin `(1329.56895,179.10277)` and dimensions about 64.6x63.6
  form a native-observed geometry fixture; visible user alignment is not yet proven.
- Current control-strip background `legendBG`, vtable `0x1D740C8`, under
  ControllerOverlay has local bounds `(-1440,68,2880,104)`. Computed live screen
  bounds start `(172.7412,33.2759)` and scale 0.48935184. A bounded visible widget
  search (512 nodes, depth 6, child limit 128) requires that exact name/type before
  placing the status in the right portion of the strip. No native instruction
  label is replaced. Compact layouts/custom instructions may still require tuning.

All native data is read on the queued UI callback, and only IDs/rectangles are
published under the existing state lock. Rendering receives no native pointers.
Marker size is proportional to the actual icon bounds, with a dark outline and
muted brass ring/cross strokes inset into the upper-right corner. Disabled
settings omit their markers; the selected-skill status is explicit ON/OFF.
Markers represent configured eligibility, not active manualAim ownership. Both
honor overlay_enabled; the R3 toggle remains separately configurable. Changing
class/tab does not use catalog grid coordinates or assume a fixed tree layout.

The new geometry guards are independent of gameplay/toggle admission. Failure
omits the new drawing and retains existing behavior. Automated tests cover the
observed hierarchy, varying position/scale, marker containment and invalid bounds.
Live indicator alignment across tabs/classes/resolutions remains pending.

Rev.57 validation: all 23 suites passed, including native-observed screen-layout
fixture, different icon positions/scales, corner containment and invalid geometry
rejection. Confirmed game/loader closed and installed locally with rollback at
local rollback controller-qol-rev57-20261004-091137.
Built/deployed DLL SHA256: 5CDD6EFDC4DFE57019166765F9A5F07F86E357798EACD68B4296FC1B9928EC97.
Runtime TOML hash unchanged. Live indicator alignment remains pending.

## Rev.58 press handling and reticle appearance

User confirmed rev.57 indicators work, but reported needing two R3 taps. Source
review finds TreePress discarded the first pressed sample whenever its context
was newly valid; a brief focus/sample gap could therefore swallow a fresh tap.
This is a demonstrated state-machine defect, not a captured trace of every live
failure. Rev.58 uses the existing admitted native event hook `0x13EDD0` instead
of relying on the UI poll to observe the press transition. No additional native
hook, address, layout or ABI is added.

Inside HookNativeEvent, after existing active-controller admission/seed, only
R3 (`0x80`) pressed with a previously clear captured raw bit increments an atomic
monotonic sequence. Repeated held events and the game-thread input pump do not
increment it. UI callbacks consume a changed sequence once, gated by current
verified skill focus and the shortcut setting; outside-tree presses are consumed
without toggling. Initial observation primes the sequence, preserving the held
entry guard. The optional XInput fallback still polls, but now tracks neutral/held
history across focus transitions and preserves history on stale samples.

The brighter tree brass is `(1.0,0.82,0.36,1)` with stroke scale 0.90 rather than
0.65. Gameplay ground/lock colors are independent `[aim]` quoted hex values:
`ground_reticle_color`, `lock_reticle_color`, and `reticle_thickness` (0.5..4,
default 1). Thickness scales both foreground and contrast outline, retaining
geometry and snap distances. RGBA opacity multiplies contextual ground fading.
These settings load on restart. SDK overlay remains the renderer; no native
assets/text are modified. Debug diagnostic geometry is not styled by these keys.

The shared section/comment scanner now recognizes quotes and escapes so a hash
inside a color literal is retained; unquoted hashes still begin comments. Aim
parsing validates exact 6/8 hex digits, duplicate keys and finite width bounds.
R3 document edits preserve color lines/comments byte for byte. Automated checks
cover first press after neutral, focus gaps/stale samples, outside-tree sequence
isolation, short presses, hold/repeat behavior, hex/alpha values, duplicate/malformed
colors, invalid thickness and config preservation. Live first-tap reliability and
new appearance still need testing after deployment.

Rev.58 validation: all 23 suites passed. Confirmed game/loader closed and
installed locally with rollback at
local rollback controller-qol-rev58-20261004-092656.
Built/deployed DLL SHA256: EA1AED47F8E28F50475EB9D0ADEBC383A0B0A76326871BC3C44AF16995233FBC.
Runtime TOML migration added only the three missing styling defaults; semantic
comparison confirmed all prior settings unchanged, and removing the inserted
block reproduces the original config bytes. Live first-tap reliability and
appearance remain pending.

## Rev.59 locked-off catalog settings

Class section values now accept `true`, `false`, or `"disabled"`. The string locks
native targeting in place and excludes the skill from the R3 shortcut. Boolean
false remains off but toggleable. Class headings remain organizational only.
The parser stores an immutable per-entry locked flag; UI press admission and
config rewriting both reject locked entries. Runtime mode reads also refuse an
attempted override of a locked baseline. A freshly locked on-disk document is
protected by rewrite validation before any runtime publication. Restart after
manual config edits to reload the status and baseline.

Locked entries omit icon markers and show `Auto-aim: DISABLED`, without the R3
hint. The stock config and catalog generation tool use the string for the 30
entries already classified as passive in the reviewed catalog; no active-mod
skill semantics are inferred from ID alone. Existing explicit true settings are
preserved during local passive migration. No native addresses, contracts or hooks
change. User confirmed rev.58 single-press behavior feels better before this
change. Automated locked-state tests cover parsing, route/scoring exclusion,
refusal of both toggle directions, immutable baseline protection, malformed values
and byte-preservation of locked entries when toggling a different skill. Live
locked-skill interaction remains pending.

Rev.59 automated validation: all 23 suites passed, including embedded DLL
configuration/ABI/version checks and the locked-skill cases above. Default config
contains 30 explicitly locked passive entries.
Confirmed game/loader closed and installed locally with rollback at
local rollback controller-qol-rev59-20261004-093519.
Built/deployed DLL SHA256: 91B9345490BA27562F8F25E576E4C74FE832B26E6F249FBA295C4A3662A497EF.
Runtime migration converted exactly 30 currently false passive entries to the
locked string; semantic comparison confirmed all other values unchanged. Live
locked-skill interaction remains pending.

## Rev.60 independent targeting overrides

Custom enable flags now accept true/false/"disabled". Custom true defaults to Snap;
legacy custom ground/snap strings still enable their specified targeting mode.
Each setting stores preferred targeting independently from current enabled mode.
The optional aim.targeting section is parsed after all enable sections, accepts
numeric IDs and ground/snap only, and requires either a catalog ID or an explicitly
declared custom ID. Duplicate numeric identities and malformed modes fail parsing
without partial publication. Overrides cannot enable or unlock entries.

R3 document rewriting excludes aim.targeting and preserves it byte for byte.
Runtime re-enable uses immutable baseline preferred targeting instead of catalog
snap flags. Manual file changes still require restart. Existing global snapping
and native compatibility guards remain authoritative. Custom skill-tree admission
is unchanged; unknown IDs are still not toggled through unverified native widgets.
No new native addresses, layouts or ABI contracts are introduced.

Rev.60 validation: all 23 suites passed, including artifact/config validation,
legacy custom modes, independent enabled/locked targeting overrides and R3
rewrite/runtime mode retention. Confirmed game/loader closed and installed with
rollback at local rollback controller-qol-rev60-20261004-094556.
Built/deployed DLL SHA256: A5A5C316FF9A56F4161C7B03221F498FFCB2F6F65DD98C91ED67CB37A2A72158.
Runtime config contains no active custom entries. Updated comments/examples and
added empty aim.targeting; semantic comparison confirmed all prior values
unchanged. Live testing of modded/custom targeting overrides remains pending.

## General Skills and equipment-granted OSkills (2026-10-10)

The highlighted Reimagined Warp was captured as ID 429 on the separate General
Skills button type. That type stores a numeric ID at +0xC08, not the class-tree
record at +0x668. See [GENERAL-SKILLS-OSKILLS.md](GENERAL-SKILLS-OSKILLS.md) for
artifact identities, exact native witnesses, configuration policy and the
remaining R3/indicator implementation boundary. No runtime behavior changed in
this read-only investigation.
