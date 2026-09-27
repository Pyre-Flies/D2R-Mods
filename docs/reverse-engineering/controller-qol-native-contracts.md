# Controller QOL native-contract registry

This is the cross-project index for reusable D2R/D2RCore findings discovered while developing Controller QOL. The authoritative byte witnesses, signatures, source implementation and feature-specific validation remain under `plugins/controller-qol/`. This file prevents other plugins from rediscovering the same paths without turning a plugin observation into an unsupported universal ABI.

Reviewed source profile: installed D2R.exe file version 3.3.93787, disk SHA-256 `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7`; D2RCore SHA-256 `AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0`; prior decrypted analysis image SHA-256 `81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5`. Matching version text without matching bytes/hashes is not qualification.

All game locations below are D2R.exe-relative RVAs unless marked D2RCore. Current executable guards live in the plugin's signature headers.

## Controller, navigation and presentation

| RVA / module | Observed role | Scope and evidence owner |
| --- | --- | --- |
| `0x13EDD0` | Normalized native controller key transition | Exact-build Controller QOL native-input profile; `plugins/controller-qol/docs/NATIVE-INPUT-1.3.1-rev.5.md` |
| `0x13DEF0` | Per-controller reset path | Paired with normalized transition; same profile and lifecycle restrictions |
| D2RCore `0x67F4D0` / lookup `0x680D28` | Native raw-controller manager access | Exact Core hash and layout checks; never reuse as unguarded input API |
| `0x878D30` | TabBar controller navigation | Filters before selection mutation |
| `0x27DF80` | Main UI switcher | Menu navigation path |
| `0x14C6810` | Skill Tree navigation | Skill sub-tab handling |
| `0xC66A0` / `0xC6E90` | Ground-label press/release | Scoped controller-label behavior |
| `0x86D410` | Controller-prompt widget scope | Prompt draw scope; global renderer hook was removed for coexistence |
| calls `0x86D6A9` / `0x86D6F3` | Controller prompt text/spacing | Exact five-byte CALL redirects; retained relay/lifetime requirements |

See `plugins/controller-qol/docs/HOOK-INVENTORY.md` for current versus superseded hook ownership and `plugins/controller-qol/docs/GLYPH-CALLS-1.3.1.3.md` for prompt-call evidence.

## Inventory, belt and advanced storage

| RVA / layout | Observed role and contract | Important limit |
| --- | --- | --- |
| `0x15F660` | `UI_ShiftRightClickPlaceAction(item, owner, page, flag, optionalSlot)` | Stored inventory item route uses page 0 and flag 1; asynchronous native submission, not completion |
| `0x3862D0` | Native free-belt-slot resolver | Retains native capacity and item-family policy |
| `0x15F8B0` | Existing native transfer between storage/inventory pages | Arguments depend on actual native storage page, not UI category or displayed sub-page |
| `0x1A0780` | Finish inventory interaction | Existing helper; do not add redundant completion calls |
| `0x159B30` | Advanced-counter single withdrawal sender | Opcode/owner behavior is build-specific; called only after UI/layout qualification |
| `0x2CF680` | AdvancedStashSlotWidget withdrawal wrapper | Page 0 maps to inventory; page 3 maps to Cube in the reviewed wrapper |
| widget `+0x608` | Bound advanced-stash item/counter representation | Pointer is UI-lifetime scoped; not an ordinary owned stash item |
| widget `+0x600` | Display item pointer | Identity/display witness only; not substituted as the withdrawal object |
| `0x846170` | Top-level panel lookup by **name** | `BankExpansionLayout`, not widget type `BankPanel` |
| `0x856220` | Direct child lookup by name | Non-recursive; resolve category then slot |

Reimagined's Gems, Materials and Runes surfaces are advanced counter widgets, not ordinary stash-grid items. Container enums and visible tab order are insufficient identity. See `plugins/controller-qol/docs/MATERIALS-NATIVE-CONTRACT.md`, `MATERIALS-CUBE-ROUTING.md`, `CUBE-ROUTING.md`, and `BELT-NATIVE-CONTRACT.md` for full layouts, byte evidence, UI-thread rules and live results.

## Shared stash identity and storage pages

| RVA / field | Observed purpose | Important limit |
| --- | --- | --- |
| `0x23AF50` | Selected Bank category | Category 0 Personal, 1 Shared in the reviewed layout; not a native storage page |
| `0x23AD80` | Selected Shared owner **record** | Resolve record ID to a client unit before transfer |
| Bank `+0x168` / `+0x170` | Current / previous-season selected sub-page | Display sub-page identity is separate from storage page |
| `0x2EF880` | Owner-record ID getter | Small reviewed helper |
| `0x09A5D0` | Client unit lookup by ID/type | Shared owner lookup uses unit type 0 |
| grid `+0x630` | Native storage page used by transfer caller | Reviewed Shared grid value is 4; do not substitute category 1 or displayed page 105 |

Shared deposit uses destination page 4 and inventory source 0. Shared withdrawal uses inventory destination 0 and source page 4. A native boolean return is submission feedback, not proof of visible or authoritative completion. Preserve exact item identity and `sharedStashPage` when crossing UI/game threads. See `plugins/controller-qol/docs/SHARED-DEPOSIT.md` and `SHARED-PAGE-CONTROLS.md`.

## Vendor selling

| RVA / value | Observed role |
| --- | --- |
| UI mode `0x0B` | Native shop mode used by the inventory quick-sell caller; supersedes an older `0x16` assumption |
| `0x846190` | Panel lookup by UI mode |
| `0x374370` | Native item sale eligibility |
| `0x34AB60` | Unit mode; native caller rejects mode 2 |
| `0x36CFE0` | Native inventory page getter; carried inventory requires page 0 |
| `0x23FED0` | VendorPanel quick-sell wrapper `(panel, player, item, sell, immediate, forceShift)` |
| `0x10D160` | Native transaction path beneath the wrapper |
| `0x36F0B0` | Native price calculation used by the sell branch |
| `0x1114C0` | Native transaction dispatch |

The reviewed caller supplies `true, true, false` for the wrapper's final flags. Selling stays on the UI thread and revalidates runtime ID, code, page, eligibility and mode. Invocation does not prove that the item left inventory or that gold changed. See `plugins/controller-qol/docs/NPC-SELLING.md` and `vendor-evidence.json`.

## Ground loot and portals

| RVA / calls | Observed role | Current ownership direction |
| --- | --- | --- |
| `0x471950` | Native ground pickup path | Shared entry is deliberately untouched in rev.42; allowed redirected game calls traverse the current entry owner, including Auto Deposit |
| calls `0x410005`, `0x4112F5`, `0x416126`, `0x41738D` | Direct pickup calls in `D2Game\\src\\Player\\PlrMsgCheats.cpp` | Exact five-byte CALL guards; redirected to QOL's pickup policy wrapper |
| calls `0x4BA0EF`, `0x4BBAD0` | Direct normal player-message pickup calls (`0x4BBAD0` carries `D2Game\\src\\Player\\PlrMsg.cpp` evidence) | Exact five-byte CALL guards; expected A/interact coverage requires live confirmation |
| call `0x55496C` | Direct pickup call in `D2Game\\src\\Skills\\SkillSor.cpp`, consistent with the Sorceress skill path | Exact five-byte CALL guard; redirected without changing the shared entry |
| `0xCBEB0` | Ground placard builder | Shared entry collided with Chronicle; current QOL uses scoped caller `0x1FAA18` |
| `0x18B350` / `0x18AED0` / `0x18A650` | Portal score, range and comparison paths | Neutral portal priority with modifier-specific exclusions |
| calls `0x191589`, `0x1922CD`, `0x192378` | Portal-contact call sites | Current rev.7 scoped CALL migration leaves shared entry `0x34BC90` untouched |
| ObjectsTxt class `267` | Town stash (`Bank`, OperateFn 32 in the active table) | Rev.43 exact-class priority classifier; hidden stashes and chests are excluded |
| ObjectsTxt `SubClass & 0x40` | Waypoint family (OperateFn 23 in all inspected active rows) | Rev.43 priority classifier using established compiled SubClass offset `+0x127` |
| ObjectsTxt `SubClass & 0x01` | Shrine plus healing/mana-well family (OperateFn 2 in inspected active rows) | Rev.44 default-on priority classifier using established compiled SubClass offset `+0x127` |
| ObjectsTxt classes `111, 113, 115, 118, 130, 132, 137, 138, 322, 426, 493, 498, 513, 519` | Complete environmental Fountain/Well family using OperateFn 22 in active table SHA-256 `45851636360723F5E1B3DE98207625748AF23546D40918C1BC8254A263F2747B` | Rev.46 exact allowlist under `prioritize_shrines`; avoids broad SubClass 0/32 promotion |
| ObjectsTxt class allowlist (47 IDs) | Ordinary active-table chest family | Rev.44 opt-in priority; exact IDs in `SHRINE-CHEST-PRIORITY-REV44.md`, deliberately not generic SubClass 8 |
| `0x349860` entry / exact tail `0x349865..0x34987F` | Object class getter shared-entry admission | Rev.45 accepts original prefix or executable rel32 `E9`, never patches/bypasses the owner, and still requires the reviewed 27-byte tail |

See `plugins/controller-qol/docs/PORTAL-PRIORITY.md`, `STASH-WAYPOINT-PRIORITY-REV43.md`, `SHRINE-CHEST-PRIORITY-REV44.md`, `PORTAL-CONTACT-1.3.1-rev.7.md`, `CHRONICLE-GROUND-FLAG-CONFLICT.md`, and `HOOK-SCOPE-REVIEW-2026-09-26.md`.

## Reuse rules

- Start from these semantic anchors, then re-establish the exact current bytes, module identity, calling convention, object lifetime and thread requirements.
- Prefer public D2RLoader services when they expose the required operation.
- Keep UI categories, SDK containers, native storage pages and mod-visible sub-pages as distinct namespaces.
- Treat native submission, authoritative state, client visibility and persistence as separate evidence levels.
- Update this index and the authoritative plugin evidence together whenever a reusable contract changes.
