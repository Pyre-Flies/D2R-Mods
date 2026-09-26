# Shared deposit investigation — 1.5.10-shared-deposit

2026-09-23. Live confirmation of this change is pending. Existing seven CTest suites pass; they do not execute the game functions.

## Failure and correction

The 1.5.9 log at 19:08:52 and 19:09:00 shows ordinary inventory items going through the Personal transaction while Shared was selected. The helper still looked up `BankPanel`, then returned tab 0 on lookup failure. The actual registered name is `BankExpansionLayout`. Lookup failure now returns UINT32_MAX and refuses a Personal fallback. The old bank+0x278 array contains category widgets, not destination units.

The Shared path now resolves the selected page through native code, converts its owner record to a client unit, then calls the existing transfer helper. Item access stays inside D2RLoader editNativeItem's synchronous game-thread callback. Existing smart advanced-storage routing remains first. No new input hook or page-index writes are introduced.

## Evidence and ABI

All addresses below are D2R.exe-relative RVAs, not absolute ASLR addresses. Evidence image: work/runtime-game.exe, SHA256 81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5. See shared-page-disassembly.txt and shared-deposit-resolvers.txt.

| RVA / field | Observed purpose and contract |
|---|---|
| 0x846170 | Existing panel lookup, pointer(const char*), name BankExpansionLayout |
| 0x23AF50 | Selected category, uint8(BankPanel*); 0 Personal, 1 Shared |
| 0x23AD80 | Selected Shared owner **record**, pointer(BankPanel*); not itself a unit |
| 0x23B910 | Season selector used by owner resolver |
| Bank+0x168 / +0x170 | Current / previous-season selected sub-page; resolver bounds-checks its owner vector |
| 0x388210 | Current-season owner vector population called at 0x23AE8F |
| 0x3E2B242 | Previous-season runtime wrapper target in this dump; do not assume a stable original RVA |
| 0x2EF880 | uint32(record*): mov eax,[rcx]; ret, obtains owner ID |
| 0x09A5D0 | pointer(uint32 ID,uint32 type), called with type 0; client unit lookup, table RVA 0x2A23910 and tail call 0x09F270 |
| 0x15F8B0 | Existing bool transfer(item, destinationUnit, uint8 destPage, uint8 srcPage, bool, placementOut); Shared uses (1,0,true) and 16-byte placement storage |
| 0x1A0780 | Existing finish interaction, called (3,null,0,0,false) |

The decisive native call chain is Bank refresh: selected category at 0x23E84F, Shared comparison at 0x23E854, owner resolver at 0x23E85B, ID getter at 0x23E868, client unit lookup at 0x23E874 (EDX=0), then grid binding at 0x23E89B. The implementation follows this conversion exactly. Category widget array assignments/accesses appear near 0x23B38F and 0x23E69A.

src/shared_deposit_signatures.h guards all 0x1C0 bytes at 0x23AD80 plus the complete 3-byte ID getter and 33-byte client lookup entry. A mismatch refuses Shared deposit. These guards supplement existing native-profile checks; they do not prove every callee unchanged.

## Patch recovery

Start from BankExpansionLayout and the Bank refresh selected-category branch. Recover selected-tab getter and follow the category==1 branch to owner resolver, ID getter and client lookup. Verify both season indices, vector bounds, unit type and transfer ABI in the new build. Compare reviewed disassembly before updating signatures; do not blindly regenerate admission bytes. The previous-season wrapper address is dump-specific. Verify live deposits on multiple Shared pages and with a full page, then confirm Personal and Materials/Cube regression behavior. Native return true is logged as submitted, not server-confirmed completion.

## Indicator adjustment

Shared glyph chord scale changed from 0.75 to 0.70 of the incoming native scale. This modestly reduces both glyph size and spacing using the existing renderer; it is not independent kerning. Centered rectangle geometry and input behavior remain as in 1.5.9.


## 1.5.11 correction: category index is not storage page

User test of 1.5.10 failed: vibration, no movement. Saved log 1.5.10-shared-failure.log shows tab=1 and native return true at 19:19:39.652 and repeated attempts. Personal SDK transaction still worked at 19:19:47.248. Thus lookup/signature admission was no longer the immediate blocker; the native return was not proof of movement.

The previous version mistakenly passed destination page 1. Static evidence conclusively supplies page 4: Bank refresh binds the selected Shared unit to the grid at RVA 0x23E89B, then writes byte [grid+0x630]=4 at 0x23E8A7 (`C6 83 30 06 00 00 04`). Native grid transfer loads that byte into R8D at 0x2C6244 and calls 0x15F8B0 at 0x2C6265. Source page comes from 0x36CFE0. The automatic advanced-storage caller also explicitly uses R8B=4 at 0x2AAAFF. Category 1 means Shared in BankTabs; native page 4 means stash storage within the selected owner. These are separate namespaces.

SubmitSharedTransfer now passes (item, selectedSharedUnit, 4, 0, true, zeroedPlacement). This corrects only inventory-to-Shared deposit; inherited withdrawal behavior is not changed. The earlier table's (1,0,true) describes the FAILED 1.5.10 call and is superseded by (4,0,true). Existing owner resolver guards remain. No new hook or RVA call is introduced. Native return is still only submission feedback; live confirmation is pending.

Regression coverage in belt_tests injects the native transfer callback and checks owner/item identity, destination 4, source 0, flag=true, disengaged automatic placement, and null-owner rejection. For patch recovery, verify the Bank grid page assignment together with the grid transfer argument load; do not derive storage-page constants from UI tab indices or SDK container enums.

## Gems and embedded Cube

Gems is selected category 2, Materials is 3. Both inventory LB+X routes now share QueueQuickMoveToCube(true): eligible items use the existing smart-storage helper, other items use the established LB+Y Cube action. Cube-source LB+X already shares LB+Y withdrawal to inventory regardless of tab. Runes/category 4 is unchanged. Materials policy tests cover both deposit categories and exclude Cube source, Shared and unknown categories. No additional native address is needed.


## 1.5.12: Shared withdrawal source-page correction

User confirmed 1.5.11 deposits work, but withdrawals do not. Saved log 1.5.11-withdrawal-failure.log shows source container 8 and repeated misleading success messages at 19:26:33.540 onward. TransferFromStashToInventory still used native source page 1. It now submits (item, localPlayer, destination=0, source=4, true, zeroedPlacement) to existing RVA 0x15F8B0, followed by the existing finish call 0x1A0780. This supersedes the old (0,1,true) contract in historical belt documentation.

Evidence: stash grid storage page is set to 4 at 0x23E8A7. Native transfer callers obtain source page via 0x36CFE0, then pass its result in R9B (see 0x2C623F / 0x2C6256 and shared-transfer-page-disassembly.txt). Inventory destination is page 0. Shared category index 1 must never be substituted for native source page 4. No new RVA, hook, packet encoder or resolver is introduced.

Shared withdrawal executes fully inside D2RLoader editNativeItem callback rather than retaining the borrowed pointer afterward. A refused native access/transfer ends this route without a transaction fallback. Logging now says submitted/awaiting game update, not successfully transferred. Tests inject the transfer and verify destination/source 0/4, local-player argument, flag and disengaged automatic placement, plus missing-player rejection. Live confirmation pending; test withdrawals from multiple Shared pages and with full inventory. Build tests cannot confirm server acceptance.


## 1.5.13: preserve Shared item identity across threads

User confirmed 1.5.12 withdraws from page 1 but not 105. Saved 1.5.12-page105-failure.log shows initialTarget=25, cell=(0,0), hax code 0x20786168 repeatedly becoming stale on the game thread (19:30:26.104 onward); legacy code then reports resolved by position. That resolver searched Personal, Shared, custom and Cube containers without checking sharedStashPage or item identity. A matching cell on another page could win. Native storage page 4 remains correct for all Shared sub-pages and must not be replaced with sub-page number 105.

The fix uses existing D2RLoader services: obtain ItemInfo on the UI thread, copy that value into MoveRequest, runOnGameThread, and resolve an authoritative handle with getItemInfo/forEachInventoryItem. Require SharedStash container, exact runtimeId, code, classId and, when supplied by the UI snapshot, exact sharedStashPage. An unknown UI page (UINT32_MAX per SDK contract) still requires exact runtime identity. A known page cannot match an unknown authoritative page. No code-only or position-only fallback is allowed for Shared. A missing snapshot or unresolved identity refuses the action. Native pointer use remains inside editNativeItem. No new RVA or memory offset introduced.

The SDK sharedStashPage value is compared without conversion; no assumption about display numbering is needed. Logging records runtimeId, SDK-page, code, scanned count and resolved handle on re-resolution. If page 105 remains unavailable, this log will distinguish missing SDK enumeration/identity from a submitted native request. It does not claim that all modded pages have been observed through the SDK; live qualification is pending.

Regression tests reject page 1 replacing page 105, duplicate codes with different runtime IDs, Personal substitutes, and unknown candidate pages for known snapshots. They accept a changed SDK handle for the same identity/page and enforce exact identity when UI page is unknown. Existing native transfer ABI tests still pass. Patch recovery: check the SDK ItemInfo contract and UI-to-game handle lifetime before changing native addresses; this failure was in plugin resolution policy.
