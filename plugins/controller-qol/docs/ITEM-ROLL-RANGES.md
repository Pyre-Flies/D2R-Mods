# Item roll-range plugin feasibility

## Follow-up: existing native provider found

The 2026-09-23/24 implementation investigation found that the installed
`d2rloader/config/d2rloader.toml` already enables `d2rcore.items.item_stat_ranges`:
the loader provides Ctrl/right-trigger native ranges. The independent-calculator
proposal below is superseded for the first plugin by remapping the provider's
range-only input checks to ALT/R1 and adding panel/focus gates.

Standalone source, native address ledger and tests:
`F:/SteamLibrary/steamapps/common/Diablo II Resurrected/Documentation/item-roll-ranges`.
See its `docs/NATIVE-CONTRACT.md` for exact D2RCore exports, callsites, slots,
fingerprints, original formatter candidates and patch recovery; see
`docs/VALIDATION.md` for remaining in-game checks. Version 0.1.0 is experimental,
build/test verified only; runtime display and item coverage are not yet attested.

## Original feasibility investigation

Investigated 2026-09-23 against installed Reimagined files and bundled D2RLoader SDK. Research only; no plugin/DLL or item data changed.

## Verified example

Installed source: F:/SteamLibrary/steamapps/common/Diablo II Resurrected/mods/Reimagined/Reimagined.mpq/data/global/excel/setitems.txt, line 69. Row index label Aldur's Stony Gaze, *ID 66, item dr8 (Hunter's Guise). Property slot 1: prop1=ac, min1=80, max1=120. User reports their item rolled +118; inventory value was not independently read during this investigation.

properties.txt maps ac to armorclass (func1=1). itemstatcost.txt line 33 identifies armorclass as stat 31, descpriority=71, descfunc=19, descstrpos/descstrneg=ModStr1i. Distinguish the +Defense property roll from the helmet's overall defense display (base defense and other contributions).

Other variable intrinsic properties in this installed row: dmg% 25..50 and sock 2..3. Fixed intrinsic modifiers and conditional set bonuses appear separately. Use active mod definitions, not vanilla external databases.

## Storage distinction

The min/max bounds are demonstrably stored in generation definitions. The item's realized stats and quality identity allow association with those definitions; a per-item copy of every original min/max bound has not been established and is unnecessary for this example. Current table bounds describe current definitions, not necessarily historical generation bounds for items retained across balance changes. Do not present current ranges as provenance for old/edited/crafted items without qualification.

## Built-in services

SDK root: E:/d2r-reimagined-mod/d2r-reimagined-mod/plugins/controller-qol/sdk/include/D2RLPlugin.

item.h: ItemInfo includes quality, qualityRecordId (zero-based SetItems/UniqueItems record), code, classId, runtimeId, itemLevel, prefixIds/suffixIds (native one-based MagicAffix IDs). ItemInfo is a copy, but does not expose arbitrary complete item stats or a min/max array. Actual modifier-value extraction needs a separately verified native/stat service path if displaying a custom full line.

data_tables.h: DataTableService getTable/getRow expose active compiled rows, including Properties=16, ItemStatCost=35, Sets=42, SetItems=43, UniqueItems=44, Items=59, MagicAffixes=60 and Runes=66. Banks are Classic/Lod/Rotw. getRow supports all listed tables; findRowById does NOT support SetItems/UniqueItems. Validate rowSize and record bounds; packed row field layouts still require verification. Read only on captured game thread; borrowed rows expire when next table load starts. Build a copied immutable lookup cache keyed by bank/revision for use in UI callback, refreshed on DataTablesLoadedEvent. No raw table-address scanning is needed for access itself.

shared_events.h: registerItemTooltipListener supplies UI-thread ItemTooltipEvent. text starts empty and is only the listener's contribution, not the native tooltip. Regions Description/Attributes/ActionFooter, positions Top/Bottom/AboveAnchor/BelowAnchor and anchors such as Defense/Damage/Sockets provide block insertion. Defense anchor is not a per-property armorclass-line rewrite mechanism. The contribution API cannot directly prefix each existing stat line. Respect capacity/null termination and listener coexistence with QOL.

## Proposed separate plugin

1. Start with SetItems/UniqueItems identity and verified simple scalar properties; Aldur's flat Defense is a clear first case.
2. Prefer SDK table lookup and tooltip contribution for a range-summary prototype. Example `Defense roll: +80 to +120` next to existing tooltip content.
3. For exact desired `[+80 - +120] +118 Defense`, research the native property-line formatter and a narrowly scoped hook tied to property/stat identity. Do not search-and-replace arbitrary rendered numbers or assume one property produces one line.
4. Extend after verifying property mapping for combined stats, sockets, conditional set bonuses, affixes, runewords and scaled/per-level values. Multiple sources can merge into one displayed stat; affix or socket contributions must not be mistaken for the base set-property roll. Unidentified items should not expose hidden rolls/affixes.
5. Record new formatter RVA, signature, ABI, callers, localization behavior and patch recovery steps when established. No new RVA/offset is claimed or needed by this feasibility note.

Acceptance: correct Aldur 80..120 range from active definitions, exact +118 displayed only from real item value/native line, coexistence with QOL, no duplicate/replaced lines, controller/mouse and localization checks, cache invalidation on table reload, no annotation for ambiguous unsupported merged stats.

## Implemented follow-up (0.2.0 experimental)

Standalone source: F:/SteamLibrary/steamapps/common/Diablo II Resurrected/Documentation/item-roll-ranges.
The user confirmed 0.1.1's ALT/R1 hook displays the native Ctrl/RT range information.
Version 0.2.0 adds `[+80 - +120] +118 Defense` using the realized raw stat from the
verified native caller and the localized single-value formatter. Five automated
suites pass; live 0.2.0 formatting is pending. Unsupported/grouped lines retain
actual values without prefixes. Full RVAs, stack ABI, code witnesses, scope,
packaging and rollback are in that source's docs/NATIVE-CONTRACT.md and
 docs/VALIDATION.md. The initial independent-calculator proposal above is historical.

## 0.3.0 follow-up

0.2.3 rendered the prefix but incorrectly used minimum-clone Defense +80 instead
of the user's +118. Version 0.3.0 uses Core overlay original-item mapping and
renders the native property block twice, preserving actual text and adding
native light-blue range spans on unique matches. See standalone
Documentation/item-roll-ranges/docs/NATIVE-CONTRACT.md for the corrected source,
TLS/slot/ABI ledger and matching limitations. Four current suites pass; live
broad-property rendering and fields outside that block remain unverified.
