# Advanced-stash withdrawals — QOL 1.5.3-materials

Reviewed 2026-09-23. This extends the 1.5.1 potion work with **LB+X to withdraw one advanced-stash item to inventory**, and **LB+R3 to refill rejuvenations directly from the Materials tab**. New runtime qualification is pending; automated native-call/policy tests do not execute D2R.

## 1.5.2 failure and 1.5.3 correction

The user tested Chipped Emerald, full rejuvenation and Um rune; none withdrew. `1.5.2-runtime-failure.log` confirms native profile admission at 18:15:22, legacy LB+X fallthrough at 18:15:42/48 and 18:16:01, and refill cancellation at 18:16:03. No advanced withdrawal was submitted. Earlier unit tests checked policy/call arguments, but did not validate the UI lookup against the actual layout.

**The resolver confused a widget type with its name.** Both installed controller and mouse layouts begin with type `BankPanel`, name `BankExpansionLayout`. At RVA 0x846170 the native helper passes the requested string in RDX to 0x89F760 with the UI manager from global RVA 0x3440170. At 0x89F810..0x89F821 the manager loop compares the requested string with the widget name pointer at widget+0x08, through 0x12277A0. It does not search by widget type. `panel-name-lookup-disassembly.txt` preserves that evidence from the same decrypted image as the prior audit. These are analysis witnesses, not new hooks or new calls.

1.5.3 changes the lookup to `BankExpansionLayout`. Unknown panel/tab resolution no longer silently falls back to an ordinary stash move; only positively identified ordinary tabs 0/1 may use that fallback. The resolver logs action, stage, selected tab, code and scoped pointer values at action boundaries. An unavailable tab is explicitly UINT32_MAX instead of looking like personal tab 0. Diagnostic stages distinguish closed stash, missing panel/category/slot, wrong widget type, absent/mismatched item binding and native-read exception. These logs are INFO and do not depend on debug_logging. Pointer values in logs are session-specific observations, never patch constants.

Run `python tools/audit_materials_layout.py --layout <installed-controller-bankexpansionlayouthd.json>` after layout changes. It compares the source lookup string to the real root name and checks direct slot paths for gcg, rvl/rvs and r22. `materials-layout-audit.json` records the installed layout hash. The check passes 1.5.3 and was verified to reject the unchanged 1.5.2 source. It validates names/hierarchy, not runtime binding or packet acceptance.

The live user's config was backed up and debug_logging set to true for diagnosis. The packaged default remains unchanged. **1.5.3 withdrawal and belt results still require a live test**; correcting this pre-call failure does not establish the downstream native contract at runtime.

## Established runtime baseline

The user reports 1.5.1 mostly works. The saved `1.5.1-runtime-excerpt.log` independently shows inventory belt requests followed by SDK confirmation (for example runtime ID 38 into slot 6 at 17:52:58, IDs 37/39 at 17:53:20, and full rejuvenations 51/52 at 17:53:33). This establishes the inventory stored-item fix in the user's session; it does not establish Materials withdrawal.

At 17:53:09 and 17:53:26, Materials rejuvenation LB+X arrives with SDK container 2 (Cursor) and a UI handle that cannot be resolved on the game thread. At 17:55:20 and 17:56:43, rune/gem actions arrive as container 8 (SharedStash); the legacy code resolves a same-code/position item and reports a normal transfer, but the user observes no withdrawal. The Materials/runes/gems grids are native `AdvancedStashSlotWidget` controls representing counters, not ordinary stash-grid items. The source container enum alone cannot distinguish them reliably.

## Decision and SDK boundary

The SDK's existing-item transactions explicitly reject shared-stash/belt moves. Inventory enumeration cannot be assumed to expose advanced counters as ordinary owned items. WidgetService can find and operate public UI properties but does not expose a native widget pointer or a documented advanced-withdraw command. We therefore retain SDK scheduling, snapshots and lifecycle ownership, and call the game's existing advanced-withdraw action on the **UI thread**.

No new hook, synthetic modifier, custom packet encoder, cloned item or manually edited material counter is used. LB+X first runs a UI callback that compares the tooltip item identity with the current native advanced widget binding. Only ordinary items then enter the older game-thread mover. A failed match on an advanced tab is logged and stops; it cannot select some other same-code item as fallback.

## Primary evidence and provenance

The disassembly source is the same earlier decrypted `work/runtime-game.exe` documented in [the belt contract](BELT-NATIVE-CONTRACT.md), SHA-256 `81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5`. The installed disk executable reports 3.3.93787; protected on-disk .text is not a substitute for decrypted runtime code. No game was running during static analysis. The subsequent 1.5.2 runtime log admitted this byte profile but exposed the panel-name bug below. Runtime byte admission is required.

Supporting local sources:

- `Documentation/RuffnecKk-D2RLoader-Suite-main/research/d2r/known-rvas.json`: entries for advanced single/batch withdrawal and More Materials Tabs. Used as leads, then checked against disassembly.
- Installed mod `mods/Reimagined/Reimagined.mpq/data/global/ui/layouts/controller/bankexpansionlayouthd.json`: BankTabs has personal/shared/gems/materials/runes at indices 0..4. Direct category names are `advancedstash_gems`, `advancedstash_materials`, `advancedstash_runes`. Slot widget names match item codes; the rvs/rvl entries occur around lines 690/697. The non-controller layout also defines these slots.
- Prior data dump `work/game-1cb0000.bin`: the pointer at RVA 0x1CF4740 is `base+0x2CE900`; at 0x1CF4748 it is `base+0x2CF510`; at 0x1CF4750 it is `base+0x2CF680`. Combined with the native virtual call at offset +0xD8, this locates the bound-item getter at vtable +0xC8. This data is an analysis witness, not an absolute pointer copied into the plugin.

`materials-disassembly.txt` preserves the relevant bodies/branches. `materials-evidence.json` records 21 byte-admitted ranges and their hashes. `materials_signatures.h` is generated from that reviewed image. All addresses below are **D2R.exe-relative RVAs**; add `context->exeBase`, not a hardcoded VA.

## Address and layout registry

| RVA / offset | Role and contract | Evidence / use |
| --- | --- | --- |
| 0x159B30..0x159BA5 | `void(item*, stashOwner*, uint8 destination)` | Native single withdrawal, 117 bytes; opcode 0x63, operation 1, native item/owner IDs; destination 1 inventory, 3 belt |
| 0x2CF680..0x2CF759 | `void(AdvancedStashSlotWidget*, int32 cell[2]*, uint8 targetPage)` | Native widget withdrawal, 217 bytes. Target page 0 maps to destination 1; page 3 maps to destination 2 (Cube). Coordinates are supplied as valid zero storage |
| 0x2CE900..0x2CE908 | `void*(AdvancedStashSlotWidget*)` | Loads widget+0x608 and returns; vtable +0xC8 binding also checked |
| widget+0x608 | Bound advanced-stash item/counter representation | Used by native inventory, cursor and belt withdrawal branches |
| widget+0x600 | Display item pointer | Native 0x2CF4A0..0x2CF508 reads it at 0x2CF4BA; accepted only for matching tooltip runtime ID/code, never as the withdrawal item |
| 0x46D9A0..0x46D9B4 | `uint8(item*)` counter availability | Null→0; otherwise reads pointer item+0x10, then byte +0x9C. Follow the native nonzero predicate; do not reinterpret this as a full-width stored quantity |
| 0x23B980 | `bool()` previous-season selection | Both native withdrawal paths use it before choosing the stash owner |
| 0x46D9C0 / 0x46DA50 | `void*(localPlayer*)` previous/current owner | Resolve the correct owner, rather than passing the player as the stash owner |
| 0x846170 | Find top-level panel by name | Resolve `BankExpansionLayout` afresh on each UI callback |
| 0x856220..0x85629E | Find direct child by name | Iterates parent's child pointer array +0x58/count +0x60. Not recursive: resolve category first, then code-named slot |
| 0x23AF50 | `uint8(BankPanel*)` selected tab | Gates known 2/3/4 category mappings; refill requires Materials tab 3 |
| 0x34A330 / 0x36EF50 | Native runtime ID / item code getters | Match focused SDK snapshot to display/bound representation; no cross-thread native pointer storage |
| 0x08B2D0 / 0x09A480 | Local data context / local player | Existing resolver chain |
| 0x34A360 / 0x3862D0 | Player inventory / free belt slot | Reuse the native family/capacity rules before requesting a belt withdrawal |
| 0xCE500 | UI mode query | Require stash mode 0x18 before resolving widgets |
| 0x1C3420 / 0x1A0780 | Item interaction mode / finish UI interaction | Follow native belt branch completion; inventory widget wrapper already handles its own completion |

The profiles also admit the display-item witness `[0x2CF4A0,0x2CF508)` and belt-withdraw witness `[0x2CF88B,0x2CF8FD)`. These support the field/argument interpretation; they are not installed hooks or callable function entries.

Other discovered paths retained for future work, **not newly called by this change**:

| RVA | Finding |
| --- | --- |
| 0x2CF510..0x2CF58C | Native advanced withdrawal to cursor; calls single sender at 0x2CF57C with R8D=0 |
| 0x2CF590 | Native batch-withdraw widget method; reads widget+0x608, resolves season owner, maps page, and calls 0x1599A0 at 0x2CF63E |
| 0x1599A0 | Corpus identifies native batch withdrawal `(item, owner, destination, count)`; not used to avoid speculative counts |
| 0x2CF760 | Advanced widget click handler; shift/cursor/ctrl branches lead to the native withdrawal actions |
| 0x2CF8EB | Direct belt callsite to 0x159B30; writes R8B=3 at 0x2CF8E2, owner in RDX, bound item in RCX |
| 0xEC880 | Native sender reached at 0x159B90; QOL does not encode or call this packet API directly |
| 0x15F100 / 0x15A110 | Native click-handler cursor and belt-eligibility checks, seen around 0x2CF7D0/0x2CF882; not reused as separate APIs |

## Why the new arguments are correct

In the single sender, R8B is zero-extended into ESI at 0x159B42. Native owner and item IDs are resolved through 0x34A330. At 0x159B85 CL=0x63; at 0x159B87 EDX=1; at 0x159B8C ESI is written to the fifth sender argument. The helper then calls 0xEC880. Its destination parameter is **not** an inventory page number.

The widget wrapper instead accepts a page byte in R8B, saved to SIL at 0x2CF699. At 0x2CF6F5 it tests SIL; page zero reaches 0x2CF71C and writes R8B=1 before calling the single sender at 0x2CF725. Thus LB+X calls the wrapper with **page 0**, not destination enum 1. The wrapper handles previous/current owner selection and UI finish itself.

For direct belt withdrawal, the game's branch reads the bound item at 0x2CF88B, obtains local player, selects previous/current season owner, checks the native counter predicate, then explicitly sets **R8B=3** at 0x2CF8E2 and calls the sender at 0x2CF8EB. It obtains interaction mode at 0x2CF8F3 and finishes UI. QOL follows this branch without faking a mouse Shift state and without routing through temporary inventory space.

## Execution and confirmation

LB+R3 retains the existing SDK-confirmed inventory/ordinary-stash batch. On clean completion, if stash was included, it asks the Materials module to refill. This module requires the selected Materials tab. It prefers rvl, then rvs; it resolves the current slot each time and requests one bottle only if the native counter is nonzero and native belt-slot search reports space. At most 16 observed requests are processed per batch. A depleted family advances to the other family; full/no eligible stock ends the batch.

All widget discovery and native UI calls run through `ThreadService::runOnUiThread`. No raw widget, item or owner pointer crosses a callback. InventoryService supplies player/cursor state and destination quantity snapshots on the UI thread; LifecycleService clears pending work on join/leave. The existing polling loop schedules only while active, with one queued tick. LB+X and belt batches are serialized; repeated shortcuts cannot start overlapping batches.

For advanced withdrawals, the new item can have a different runtime identity from the counter representation. Confirmation therefore uses an **increase in the requested code's destination quantity**, not the proxy's runtime ID. Logs intentionally say `Observed destination increase`; this is a UI-side observation, not an independent authoritative transaction acknowledgment. Concurrent manual changes to the same item code can confound this measurement. No increase within 1.5 seconds stops the batch without resubmitting. An occupied/unreadable cursor, closed stash, changed tab/season/session, service failure or admission failure also stops it.

Capacity/full-inventory refusal remains native behavior. In particular, a native void call returning is still only a submission, never proof of withdrawal. Empty stock, full belt, full inventory, both rejuvenation sizes, previous-season mode and other plugins require live tests.

## Patch recovery

1. Preserve the current source/DLL/config/logs and record the new executable hashes. Check whether the loader now offers a documented advanced-counter withdrawal API before keeping this adapter.
2. Capture decrypted code with the read-only `tools/capture_code.py` procedure in the belt guide. Do not use protected disk bytes as decoded machine code.
3. Locate candidates without changing admission:

   ```text
   python tools/audit_belt.py --image new-runtime.exe --locate-from docs/materials-evidence.json --output materials-candidates-new.json
   ```

4. Re-establish the advanced widget getter and its virtual slot, native wrapper page mapping, previous/current owner selection, opcode 0x63 operation 1, direct belt destination 3, UI finish sequence and counter predicate. Follow relative calls from the widget click handler if prologues move. Verify the installed layout's category names/tab ordering separately from code addresses. The current 0..4 mapping is not a promise about mods adding tabs.
5. Update the adapter RVAs/offsets and MATERIAL_SITES in the audit script only after disassembly review. Generate evidence and guards together:

   ```text
   python tools/audit_belt.py --materials --image reviewed-runtime.exe --output docs/materials-evidence.json --header src/materials_signatures.h
   ```

6. Rebuild, run CTest, and qualify one rune withdrawal before belt refill. A changed or already hooked native function must fail admission until its compatibility is reviewed. Record the DLL hash and live outcomes in the validation document; do not silently turn fresh bytes into an approved profile.

## Runtime acceptance checklist for this build

- LB+X on one rune, one gem and one material: exactly one item appears in inventory; source stock decreases; full inventory does not lose stock.
- LB+R3 on Materials tab: full rejuvenation first, then small if needed; stop at native belt capacity/stock exhaustion; a full inventory should not block direct belt withdrawal.
- Repeat LB+R3 with a full belt: no requests that drain stock and no misleading confirmation.
- Close/switch stash tab, switch previous-season toggle, or change session mid-batch: stop cleanly.
- Regression: inventory LB+A/LB+R3, ordinary stash LB+X, identify, loot labels and navigation.

Look for `[QOL/Materials] Native advanced-stash UI profile admitted`, `Submitted`, and `Observed destination increase`. A failed focus match or timeout is a diagnostic to investigate, not a success result.
