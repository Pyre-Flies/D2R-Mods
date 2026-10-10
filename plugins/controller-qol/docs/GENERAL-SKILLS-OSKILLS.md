# General Skills and equipment-granted OSkills (2026-10-10)

Read-only investigation of the user's open Reimagined Ladder General Skills tab.
The initial capture recorded the missing UI/configuration path without changing the plugin,
deployed configuration, hooks, or game state. See the implementation section below. The user subsequently confirmed Warp works; see the follow-up evidence below.
Rev.75 moves all skill settings/R3 saves into complete per-mod profiles;
[the current configuration guide](SKILL-DISCOVERY-CONFIG.md) supersedes the initial
configuration examples and beta-stage validation notes in this investigation.

## Artifact identity

The live game was hosted by D2RLoader.exe. RVAs below are relative to its game
image base, not D2RCore, and are specific to this build.

| Artifact | Version | Size | SHA256 |
|---|---|---:|---|
| D2R.exe (disk input) | 3.3.93787 | 32107216 | `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7` |
| D2RLoader.exe (loaded host) | 1.3.1-beta | 23331160 | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |
| D2RCore.dll (loaded) | 1.3.1-beta | 18938712 | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Controller QOL Updates.dll (loaded Ladder copy) | 1.3.1+rev.72-beta.1 | 480768 | `10F1DB37EB4AC3C970EE5D896ECE04B4C20126548317AAFACE1539F8455EFD94` |

The loaded QOL copy is the prior beta, not the separately published rev.73.
The relevant tree/custom-ID exclusions are also present in rev.73 source.
The first inspected process exited during investigation; the successful bounded
capture used the replacement process after the user highlighted Warp again.
No process was suspended, injected into, or written by the reader.

Installed Ladder MPQ-directory inputs under `data/`:

| Path | SHA256 |
|---|---|
| `global/excel/skills.txt` | `708DA23BF078CC3FC4D2EF901C7480869E18206ABA8CBC91E4030D5CBA93FEF3` |
| `global/excel/skilldesc.txt` | `5424D625B9ABF8FF4BD6F2B364A345AFC14A1B22FC8668CF00B20464FD895EFE` |
| `global/excel/uniqueitems.txt` | `BE0CFEF9ADDD26AD1F5FBAACA18A32AA02490ADB6DF0DF88D1F477276C1633EA` |
| `hd/global/excel/controllerskillsettings.json` | `4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79` |
| `global/ui/layouts/controller/skillstreepanelhd.json` | `F24DC44BE97F7CB1E92FC51DA5D671D1F46E18757C46DB16DF357298EBB72198` |

These are disk-table witnesses, not proof that every compiled runtime row matches.
Only the highlighted Warp ID below was correlated with user-selected live UI.
Private bounded captures/scripts remain under ignored `outputs/oskill-20261010`
and `outputs/inspect_oskill*.py`; no full tables or binaries are committed.

## Warp identity and policy

Iceblink's sixth property is `oskill`, parameter `Warp`, minimum/maximum 1.
Warp is skill **429**, has no class assignment, and uses skilldesc `warp`.
Teleport remains skill **54**. In the inspected table both have server do
function 27, `warp=1`, `range=none`, `InTown=1`, `ItemEffect=1`, `ItemTgtDo=1`,
and `ItemTarget=2`; neither has an item/corpse target flag or a missile here.
Their controller records are identical apart from name and ID: notably
`alwaysIgnoreTarget=true`, `usePredictiveTargeting=false`, and default target
distance 23. Ground-coordinate aim with zero projectile lead is therefore the
reviewed proposed policy. This is not evidence that every teleport-like skill
shares all Teleport semantics or range/cooldown limits.

Existing configuration can declare this ID without a new DLL:

```toml
# Merge into the existing sections; do not duplicate section headers.
[aim.custom]
"429" = true # Reimagined Warp; verify ID against the active mod

[aim.targeting]
"429" = "ground"

[aim.leading]
"429" = 0
```

The parser already accepts this shape and routes custom IDs through settings.
This investigation did not apply it or validate a Warp cast. Configuration is
loaded at initialization; editing disk alone is not a live reload. A custom ID
must not be globally equated with Teleport ID 54 or added as an unconditional
vanilla class skill. Other table-discovered examples are Townportal O Skill 411
and Polar Claws 461 (melee range); neither was live-tested or approved for aim.
An OSkill which reuses a catalog ID should share that ID's existing setting.

## Live General Skills layout

Existing `FocusGetter` at RVA `0x846020`, child-layout witness `0x8547D9`, and
visibility witness `0x854780` matched the exact source byte arrays before
inspection. These guard the existing manager slot `0x3440170`, manager `+0xD0`,
focus root `+0x188`, selected widget `+0x190`, widget parent `+0x30`, children
`+0x58/+0x60`, and visibility `+0x51` paths documented in
[SKILL-TREE-AIM-TOGGLE.md](SKILL-TREE-AIM-TOGGLE.md).

The user-highlighted Warp had this visible ancestry:

| Node | Vtable RVA |
|---|---|
| `SkillsTreePanel` | `0x1FD82B0` |
| `Tab3` | `0x1D82DA0` |
| `CommonSkillsContainer` | `0x1D72670` |
| `SkillButton3` (selected Warp) | `0x1CE4210` |

A sibling `ItemSkillsContainer` also uses `0x1D72670` and contains the same
`0x1CE4210` button type. **Warp is in CommonSkillsContainer in this capture**:
looking only under ItemSkillsContainer would miss it. Common entries also
include native utility IDs 0, 1, 2, 3, 357, 358, 360, 361, 362, 363, 364, 370;
item-container buttons reported 359 and catalog ID 12. These observations do not
authorize aim on those utilities. The hidden template's ID is -1.

Class-tree buttons use vtable `0x1CE4148` and the existing record pointer at
`+0x668`. The General Skills button is different: its `+0x668` was null and its
**signed dword `+0xC08` was 429**. Do not read it as a class-tree record pointer.
The observed General buttons had `+0xB88=0` and `+0xC0C=-1`; their complete enum
and auxiliary-ID semantics remain unproven and must not be guessed.

## Native witnesses for the new field

Read-only disassembly of the loaded game supplies the following witnesses. No
new function is called or hooked. A production extension must recheck exact
bytes, widget type, current focus/root ancestry, bounded IDs and lifecycle on
the existing queued UI callback, publishing copied IDs/rectangles only.

| Location | Witness / meaning |
|---|---|
| Generic vtable `0x1CE4210 + 0x20` | Points to native message handler `0x238C20` |
| Generic vtable `0x1CE4210 + 0x58` | Points to `0x2382D0`, bytes `E9 0B 00 00 00`, jumping to type-metadata path `0x2382E0`; not a skill getter |
| `0x238C56` | `4C 8B F1`: handler preserves widget in R14 |
| `0x238E0C` | `41 83 BE 88 0B 00 00 00`: tests widget dword `+0xB88` for zero |
| `0x238E42` | `41 8B 9E 08 0C 00 00`: reads widget dword `+0xC08` into EBX |
| `0x238E8A` | `41 8B 96 08 0C 00 00 48 8B C8 E8 57 1B F1 FF`: reads the same ID into EDX, calls `0x14A9F0` with existing native context |
| `0x238E99` | `45 8B 8E 0C 0C 00 00 BA 01 00 00 00 45 8B 86 08 0C 00 00`: reads auxiliary dword `+0xC0C` and ID `+0xC08` |
| `0x238EB8` | `E8 03 F7 ED FF`: native action path calls `0x1185C0` with that ID in R8D; full call ABI is not established here |
| Generic vtable `+0x10` | Points to `0x2399F0`; captured update path writes the ID |
| `0x239C96` | `89 97 08 0C 00 00`: stores EDX to widget `+0xC08` |
| `0x239CCC` | `8B 97 08 0C 00 00`: subsequently reads that field into EDX |

Other inspected generic vtable slots: +0x00=0x237900, +0x08=0x238770,
+0x18=0x237CE0, +0x28=0x870170, +0x30=0x8702E0, +0x38=0x870100,
+0x40=0x8566B0, +0x48=0x837D0, +0x50=0xF02A0, +0x60=0x86C5D0.
They are recorded observations, not admitted functions or complete contracts.

## Implementation boundary and remaining validation

Four current exclusions explain the missing support: FocusedTreeSkill and
GatherTreeIcons admit class-button type/catalog IDs only; SkillSettings::CanToggle
refuses non-catalog IDs; RewriteSkillToggle saves only catalog/class entries.
A narrow extension can admit the verified generic button, read +0xC08, traverse
both visible General containers, and allow **explicitly configured** custom IDs
to toggle/save in aim.custom while retaining aim.targeting. Unconfigured IDs,
utility actions, locked settings and unreviewed layouts should stay native.
It must preserve a legacy custom ground/snap declaration's mode across an off/on
save cycle, rather than silently turning a ground skill into default snap.

No global OSkill auto-enablement or new per-frame logging is required. Reading
the visible panel does not require polling all equipment each frame. Future mod
identity/discovery can be separate from generic widget support. Offline and
Ladder should share the guarded UI/configuration path, with active-mod numeric
semantics reviewed separately. Required live checks after implementation:
Warp R3 on/off and first cast; saved ground mode after restart; equipment removal
and weapon swap; catalog OSkills in General Skills; utility/template rejection;
existing class-tree toggles and offline behavior. This session establishes the
highlighted Warp ID/layout and native field consumers, not those future results.


## Rev.74-beta.1 implementation

General support is independently admitted by exact `Kind`, `Id`, and `Pair`
windows at 0x238E0C, 0x238E8A, and 0x238E99 and vtable +0x20/+0x58 identities
from the table above, in addition to existing Core and skill-tree admission.
No new hook or native call is installed. Generic reads require +0xB88=0 and
+0xC0C=-1; other variants remain unsupported. IDs 0..5, 357..364 and 370 are
excluded as native action buttons, even if a legacy custom example declared
one. This is a build-specific UI exclusion, not a new interpretation of the
auxiliary field's full meaning.

Focus follows the currently selected visible button through the reviewed
container/tab/root ancestry. Each UI sample gathers IDs and bounds afresh from
both containers; no list index, item order, grid position or record pointer is
retained. The existing bounded tree traversal and icon geometry are reused.
Catalog OSkills reuse their original settings. Custom IDs must already be
explicitly declared, and locked entries cannot be enabled via runtime overrides.
R3 persists custom booleans to aim.custom through the existing SDK WriteConfig;
legacy ground declarations gain a targeting override when needed to preserve
mode across off/on and restart. Only successful saves publish a live change.

Default configuration includes a commented Warp opt-in example. It does not
authorize ID 429 for every mod. No additional file logging or equipment polling
is introduced. Existing quiet logging defaults remain unchanged.

All 28 automated suites passed in the Release build. Regression cases cover dynamic Warp/Multi Shot button
IDs, class-reader fallback, each failed General guard, unknown widgets, null
class records, utility/template/out-of-range IDs, unreviewed variants, declared
and locked custom toggles, live overrides, legacy ground migration with/without
an existing targeting section, CRLF/comments, restart parsing and ground cast
isolation. These are synthetic/automated checks, not live casting validation.


Deployment: confirmed no D2R/D2RLoader process was running, backed up the Ladder
DLL/config, and installed rev.74-beta.1 in the existing Ladder plugin directory.
Built/deployed SHA256:
`8F94C535E73140ED4B3042009513161B248437919CBEC1F184DF45A5661366D4`.
The installed DLL also passed the version/ABI/export/embedded-config artifact
check. The private rollback manifest is under ignored
`outputs/controller-qol-rev74-deploy/20261010T121100Z`.
Only three config keys were added: aim.custom.429=false,
aim.targeting.429=ground and aim.leading.429=0. Semantic comparison verified
all other settings preserved. Warp is opt-in through R3. No root-folder plugin
was replaced and no GitHub release was published. Live R3, casting, equipment
reordering and offline validation of this revision remain pending.

### First rev.74 user test

The user confirms Warp works. Equipment-granted Multi Shot displayed enabled
and showed an aim/snap reticle, but the user reports casting did not snap until
an off/on R3 cycle. Save & Exit also crashed; dump inspection identifies a
NVIDIA-module fault, with prior suspect plugins still loaded from the global
folder. These are open issues, not complete validation of General Skills.
See [the follow-up evidence](CRASH-TRIAGE-2026-10-10.md) for identities, limits,
user-corrected plugin isolation and the pending fresh-launch diagnostic sequence.
