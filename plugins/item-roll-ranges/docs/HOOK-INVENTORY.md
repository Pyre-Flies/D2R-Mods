# Hook inventory — first pass, 2026-09-24

Reviewed source: Controller QOL1.3.1+rev.4 and Item Roll Ranges1.3.1+rev.1. Static inventory of installation paths, not a fresh live-process verification. Feature gates/failures can reduce actual installed counts. No DLL changes, rebuild, or new addresses discovered in this pass.

## Address and ownership conventions
Game RVAs are relative to the executable base; Core RVAs are relative to D2RCore.dll. XInput uses resolved exports rather than hardcoded addresses. A guard-only address or ordinary native function call is not an installed hook. Expected bytes live in the cited source/profile headers; preserve those with this index after a game patch.

SDK inline hooks: loader owns executable patches/trampolines; plugin typically saves/calls predecessor and deactivates on shutdown. SDK ownership is not proof of arbitrary hook chaining or load-order independence. Exact-byte checks can refuse an already-modified entry.

## Controller QOL: executable modifications

| Feature / purpose | Module and sites | Mechanism / source | Admission and coexistence notes |
|---|---|---|---|
| Ground-action observation; dispatch pending loot after native actions | Game: 18 entries listed below | SDK inline hooks; src/ground_action_hooks.h | Fixed32-byte witnesses in compatibility_signatures.h; predecessor called once then observer. Packet table is not patched. Partial install leaves observers inactive; loader owns hook cleanup. Another hook at these entries can prevent installation. |
| Ground placard text / assigned loot buttons | Game0xCBEB0 | SDK inline hook; plugin_main.cpp HookBuildGroundItemTooltip | Caller supplies current live16 bytes as expected. Separate GetItemCode fingerprint exists but does not independently qualify this entry. Broad tooltip sharing surface. |
| Native ground pickup suppression/filtering | Game0x471950 | SDK inline hook; plugin_main.cpp HookPickup | Also supplies current live16 bytes as expected. Plugin-origin bypass is in plugin_compatibility.cpp; intentional suppression can skip predecessor. Review admission and cross-plugin semantics together. |
| Label press/release, keep filtered labels visible for L1 looting | Game0xC66A0 /0xC6E90 | SDK inline hooks; qol_navigation.cpp | Fixed signatures; intentionally suppresses configured label actions. Other label-toggle plugins may want the same inputs. |
| Main menu trigger navigation | Game0x27DF80 | SDK inline hook; qol_navigation.cpp | Fixed signature; changes recognized menu messages. |
| Tab-bar navigation | Game0x878D30 | SDK inline hook; qol_navigation.cpp | native_signatures.h; filters before selection mutation. Broad UI function: review scoping for newly added panels. |
| Skill sub-tabs retain bumpers | Game0x14C6810 | SDK inline hook; qol_navigation.cpp | Fixed signature; scoped skill behavior. |
| Shared stash LB+LT/RT paging | Game0x23CAA0 | SDK inline hook; qol_navigation.cpp | shared-page profile; consumes/replaces recognized page actions. Potential semantic overlap with other stash navigation plugins. |
| Portal candidate contact | Game0x34BC90 | SDK inline hook; portal_priority.cpp | portal_signatures.h; native predecessor plus caller whitelists0x19158E/0x1922D2/0x19237D. |
| Portal score / range / comparison | Game0x18B350 /0x18AED0 /0x18A650 | SDK inline hooks; portal_priority.cpp | Fixed witnesses and scoped range state; neutral portal priority, modified Interact excludes portal selection. Other targeting mods can disagree even without byte overlap. |
| Controller prompt widget scope | Game0x86D410 | SDK inline hook; qol_glyphs.cpp | glyph_signatures.h; original draw in thread-local scope, restored in finally. |
| Controller prompt text/spacing | Game0x86D6A9 /0x86D6F3 | Two SDK PatchBytes CALL redirects; glyph_calls.h | Exact5-byte calls; near RX relay/pinned plugin retained for delayed calls. Inactive unless all glyph pieces succeed. Global renderer0x902E20 is no longer hooked. |
| Physical controller interception | xinput1_4 /xinput1_3: XInputGetState +ordinal100; xinput9_1_0: XInputGetState when available | Manual entry patch; controller_input.cpp +xinput_hook.h | Verified MOV prologue or pinnable module-backed E9; aligned8-byte CAS, original tail bytes preserved. Earlier private relay chains rejected. Shutdown drains filtering, keeps pass-through code/records for process lifetime. Other owners may still alter input semantics. |

Count:31 candidate game inline-hook entries,2 game CALL patches, and up to6 resolved XInput entries (the observed system has5; xinput9_1_0 ordinal100 absent). These are potential sites, not a runtime activation claim.

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
