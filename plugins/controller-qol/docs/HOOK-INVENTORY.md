# Hook inventory â€” baseline plus revision updates

The original table/count below is a historical rev.4 baseline; revision sections supersede affected rows. For current prioritization see [narrower-hook review](HOOK-SCOPE-REVIEW-2026-09-26.md).

Reviewed source: Controller QOL1.3.1+rev.4 and Item Roll Ranges1.3.1+rev.1. Static inventory of installation paths, not a fresh live-process verification. Feature gates/failures can reduce actual installed counts. No DLL changes, rebuild, or new addresses discovered in this pass.

## Address and ownership conventions
Game RVAs are relative to the executable base; Core RVAs are relative to D2RCore.dll. XInput uses resolved exports rather than hardcoded addresses. A guard-only address or ordinary native function call is not an installed hook. Expected bytes live in the cited source/profile headers; preserve those with this index after a game patch.

SDK inline hooks: loader owns executable patches/trampolines; plugin typically saves/calls predecessor and deactivates on shutdown. SDK ownership is not proof of arbitrary hook chaining or load-order independence. Exact-byte checks can refuse an already-modified entry.

## Controller QOL: executable modifications

| Feature / purpose | Module and sites | Mechanism / source | Admission and coexistence notes |
|---|---|---|---|
| Ground-action observation; dispatch pending loot after native actions | Game: 18 entries listed below | SDK inline hooks; src/ground_action_hooks.h | Fixed32-byte witnesses in compatibility_signatures.h; predecessor called once then observer. Packet table is not patched. Partial install leaves observers inactive; loader owns hook cleanup. Another hook at these entries can prevent installation. |
| Ground placard text / assigned loot buttons | Game0xCBEB0 | SDK inline hook; plugin_main.cpp HookBuildGroundItemTooltip | Caller supplies current live16 bytes as expected. Separate GetItemCode fingerprint exists but does not independently qualify this entry. Broad tooltip sharing surface. |
| Native ground pickup suppression/filtering | Game CALLs `0x410005`, `0x4112F5`, `0x416126`, `0x41738D`, `0x4BA0EF`, `0x4BBAD0`, `0x55496C` | Seven SDK PatchBytes CALL redirects; pickup_calls.h + plugin_main.cpp HookPickup | Exact five-byte witnesses for build 3.3.93847. Shared entry `0x471950` is untouched, so Auto Deposit may retain ownership and allowed calls traverse it. Wrapper remains passthrough unless every call publishes. |
| Label press/release, keep filtered labels visible for L1 looting | Game0xC66A0 /0xC6E90 | SDK inline hooks; qol_navigation.cpp | Fixed signatures; intentionally suppresses configured label actions. Other label-toggle plugins may want the same inputs. |
| Main menu trigger navigation | Game0x27DF80 | SDK inline hook; qol_navigation.cpp | Fixed signature; changes recognized menu messages. |
| Tab-bar navigation | Game0x878D30 | SDK inline hook; qol_navigation.cpp | native_signatures.h; filters before selection mutation. Broad UI function: review scoping for newly added panels. |
| Skill sub-tabs retain bumpers | Game0x14C6810 | SDK inline hook; qol_navigation.cpp | Fixed signature; scoped skill behavior. |
| Shared stash LB+LT/RT paging | Game0x23CAA0 | SDK inline hook; qol_navigation.cpp | shared-page profile; consumes/replaces recognized page actions. Potential semantic overlap with other stash navigation plugins. |
| Priority-object candidate contact | Game CALLs 0x191589 /0x1922CD /0x192378 to shared entry 0x34BC90 | SDK PatchBytes redirects; portal_priority.cpp | portal_signatures.h; native entry remains untouched. Exact return-site whitelist extends contact only for enabled portal, Bank stash and waypoint candidates. |
| Priority-object score / range / comparison | Game0x18B350 /0x18AED0 /0x18A650 | SDK inline hooks; portal_priority.cpp | Fixed witnesses and scoped range state. Read-only class getter `0x349860` admits original or bounded executable E9 ownership while its tail remains exact; QOL does not patch that entry. Other targeting mods can disagree semantically even without byte overlap. |
| Controller prompt widget scope | Game0x86D410 | SDK inline hook; qol_glyphs.cpp | glyph_signatures.h; original draw in thread-local scope, restored in finally. |
| Controller prompt text/spacing | Game0x86D6A9 /0x86D6F3 | Two SDK PatchBytes CALL redirects; glyph_calls.h | Exact5-byte calls; near RX relay/pinned plugin retained for delayed calls. Inactive unless all glyph pieces succeed. Global renderer0x902E20 is no longer hooked. |
| Physical controller interception | xinput1_4 /xinput1_3: XInputGetState +ordinal100; xinput9_1_0: XInputGetState when available | Manual entry patch; controller_input.cpp +xinput_hook.h | Verified MOV prologue or pinnable module-backed E9; aligned8-byte CAS, original tail bytes preserved. Earlier private relay chains rejected. Shutdown drains filtering, keeps pass-through code/records for process lifetime. Other owners may still alter input semantics. |

Count:30 candidate game inline-hook entries,9 game CALL patches, and up to6 resolved XInput entries (the observed system has5; xinput9_1_0 ordinal100 absent). These are potential sites, not a runtime activation claim.

### Ground action entries

0x4ac050, 0x4ace20, 0x4ace40, 0x4ace60, 0x4ace80, 0x4acf80, 0x4ad030, 0x4ad0e0, 0x4ad100, 0x4ad120, 0x4ad140, 0x4ad230, 0x4ad330, 0x4ad3e0, 0x4ad490, 0x4ad4b0, 0x4ad4d0, 0x4ad4f0

These correspond to the18 indexed native action observers (index+1) in ground_action_hooks.h. Do not substitute packet-table slot addresses for these function entries.

## Item Roll Ranges: Core data-pointer adapters

All six are aligned8-byte slot exchanges in src/plugin.cpp, using compare-and-exchange publication/restoration. Restoration only succeeds while the slot still points to our adapter. Module is pinned; active=false precedes restoration. Provider-file fingerprint and code witnesses also gate activation. Partial publication rolls back owned slots. A later owner is not overwritten, but its chain behavior remains its responsibility.

| Core slot RVA | Native game target / purpose | Scope / admission |
|---|---|---|
|0x7043A0|0x2DB800 special damage formatter|Exact target required; capture special Enhanced Damage identity at qualified return0x3E7FD6.|
|0x704400|0x2D6330 range helper|Exact target required; qualified returns0x3E80B2/0x3E80FD.|
|0x7043E8|0x2D6520 single-stat formatter|Exact target required; capture at return0x3E82EA; also used for bounded unique endpoint formatting.|
|0x704490|Baseline0x2DC4B0 property builder|Current target only checked executable; qualified return0x81D53B and panel/modifier rules. Renders actual/range passes intentionally; not universally one invocation per tooltip.|
|0x6FE3B0|Baseline0x1209FA0 keyboard reader|Current target checked executable; calls predecessor, adjusts result only at return0x8195E0 when active.|
|0x6FE470|Baseline0x13CA70 pad reader|Current target checked executable; other callers pass through. At return0x81995A replaces result using physical RB state without calling predecessor. Potential input-wrapper semantic conflict.|

Read-only dependencies (not adapters): Core0x701E70 tables,0x6FF370 affix eligibility,0x702AE0 panel reader. A conflict there may prevent activation even though we do not write those slots. More detail: NATIVE-CONTRACT.md, MIGRATION-0.4.1.md, ENHANCED-DAMAGE-0.4.5.md, SUNDER-RANGES-AND-LAYOUT-0.4.7.md in Item Roll Ranges docs.

## SDK listeners: semantic ownership rather than byte patches

QOL registers item activation at priority100 (quick identify), an ActionFooter tooltip contribution at priority100, UI-message listener at priority1000, and a diagnostic tooltip listener. UI-message Consume stops downstream listeners; high priority alone is not a conflict, but broad consumption can be. Inspect callback predicates before changing ordering. Registrations are cleaned through SDK services/loader. These do not expose a full controller state-filter service.

## Ordinary native calls and state writes

Belt refill, materials/shared transfers and vendor selling use qualified calls, scheduling and context/state handling; they are not additional installed detours in those source files. This lightweight index is not an exhaustive memory-write inventory. Native object-layout assumptions and temporary state changes deserve a separate review if cross-plugin transfer issues recur.

## First follow-ups (recommendations only)

1. Ground pickup/placard admission: replace self-matching live-byte expectations with fixed profile or explicitly recognized predecessor contracts. Preserve known plugin-origin pickup behavior; do not blindly reject supported chains.
2. Ranges properties/key/pad slots: document and qualify acceptable predecessor identities/ABIs. Current executable-only admission is weaker than exact-target checks. Do not simply relax the other guards to match it.
3. Ranges pad override: examine whether a cooperative reader can preserve predecessor effects without restoring unwanted tooltip behavior.
4. Navigation and targeting: test behavior overlaps in both load orders before expanding hook scope or listener consumption.

## Known compatibility evidence

Stash Search1.10.12 expected32 original bytes at renderer0x902E20. Old QOL renderer hook conflicted;1.3.1.3 moved to widget CALLs. User reports successful coexistence after that change. Wider third-party compatibility remains unverified. Potion Auto Pickup1.3.3 has a narrow recognized belt-hook contract and existing action-table coexistence tests; not a generic trust rule for future versions.

## Patch revisit checklist

Locate installer and witness header; identify module before resolving RVA; compare guard bytes/call destinations; preserve calling convention and predecessor return rules; check whether another plugin owns the entry; test partial failure/shutdown and both plugin orders. Update this index and the focused evidence doc together. Keep previous production snapshots unchanged.

## Production 1.3.1+rev.5: native input provider

Adds SDK inline hooks at game `0x13EDD0` (normalized key transition, 21-byte prefix) and `0x13DEF0` (per-controller reset, 14-byte prefix). Full code witnesses, exact Core hash and manager/getter/index slot checks precede installation. `0x13CA70` is a read-only seed helper; the query-only prototype was not deployed. No Core input slot writes. XInput hooks become fallback-only. [Evidence, RVAs, admission and runtime checklist](NATIVE-INPUT-1.3.1-rev.5.md). User-confirmed production matrix: Battle.net DualShock with Steam closed, Steam Controller on Steam, and DualShock on Steam; all with Stash Search and Potion Auto Pickup.

## Chronicle Ground Flag collision (2026-09-26)

Confirmed duplicate hook at game `0xCBEB0` (MH_ERROR_ALREADY_CREATED). Caller `0x1FAA18` was implemented in rev.6 and the user reports successful coexistence. See [original evidence and call-site composition](CHRONICLE-GROUND-FLAG-CONFLICT.md).

## Candidate rev.6: scoped ground-label call

Replaces QOL entry hook at `0xCBEB0` with SDK-owned CALL patch at `0x1FAA18` (30-byte argument-setup witness at `0x1FA9FF`). Shared builder remains available to Chronicle. Exact text ownership replaces heuristic prefix stripping. [Implementation evidence and pending runtime checks](CHRONICLE-COMPATIBILITY-1.3.1-rev.6.md).

## Scope review, 2026-09-26

[Prioritized narrower-interception review](HOOK-SCOPE-REVIEW-2026-09-26.md): portal contact has three confirmed direct callers in the previous capture; pickup needs coverage tracing; IRR call-site adapters remain a separate harder backlog. No DLLs changed by this review.

## 2026-09-26: rev.7 contact CALL migration

Supersedes the historical shared-entry contact hook: QOL now patches calls at 0x191589, 0x1922CD and 0x192378; entry 0x34BC90 remains untouched. Other portal hooks are unchanged. See [evidence, ownership, admission and pending live checks](PORTAL-CONTACT-1.3.1-rev.7.md).

## 2026-09-27: rev.18 Shared Stash SDK candidate

Normal Shared LB+X item movement now uses public ItemService transactions when
SharedStashWrite is available. No new hooks. Read-only native selected-page
resolution remains; old-loader and remove-only seasonal transfers retain their
native route. [SDK pin, page witnesses and validation](SHARED-SDK-REV18.md).

Rev.21 item headers reuse the existing scoped widget/text hooks. A guarded ordinary font-metrics call at game+0x903CD0 is not an additional hook. See CONTROLLER-HEADER-REV21.md for ancestry, ABI evidence, presentation scope and validation limits.

Rev.22 adds SDK inline hook game+0x13CA70 (native controller button query), scoped strictly to Core tooltip return +0x81995A / mask 0x800. It translates that query to RB 0x200; all other callers pass through. Full profile admission precedes a 15-byte SDK hook. Core slot +0x6FE470 and its caller remain untouched for Item Roll Ranges. See NATIVE-RANGES-REV22.md for ABI, guards, load-order reasoning and unresolved shared-entry ownership risk.

Rev.26 candidate: inventory native tome-use adapter adds no hooks/slot patches. See IDENTIFY-NATIVE-REV26.md for guarded hold-A/grid activation calls and pending live validation.

Rev.37 bulk stash adds no hooks or slot writes. Existing LB+X eligibility/deposit helpers are reused through SDK native-item access, with per-batch byte admission. See BULK-STASH-REV37.md for reused addresses, guard provenance and live-validation limits.

## Rev.50 optional controller aim

Adds five opt-in SDK inline hooks: 0x190440, 0x1919F0, 0x18AF30, 0x4FDB40,
0x18DDE0. Source: src/aim/controller_aim.cpp and native_profile.h. Same sites as
standalone aim.20, now owned by QOL. Exact guards, internal contact-relay ownership,
configuration gating and live-validation limits: [rev.50](PRODUCTION-1.3.1-rev.50.md).


## Rev.54 optional Cast observer

Cast at 0x4FDB40 is now optional, enabled by default. A byte mismatch or SDK
hook refusal retains the four essential aim hooks; cast_observer_enabled=false
leaves that entry free. All other profile guards remain required. See
[AIM-CAST-OBSERVER-COMPATIBILITY.md](AIM-CAST-OBSERVER-COMPATIBILITY.md) for admission,
reset fallback and automated/live validation boundaries.
