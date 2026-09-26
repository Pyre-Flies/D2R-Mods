# Shared stash page controls — 1.5.9-indicators

## 1.5.9 inward spacing correction

The user's full screenshot shows the wider 1.5.8 labels render but extend into the main-tab RT prompt on the left and beyond the panel on the right. The prior outward expansion was therefore the wrong placement rule.

SharedChordRect now centers the three-width draw box on the original one-width indicator slot: newWidth=3*width, newX=x-width for either side. Relative to 1.5.8, the left chord moves right by one original indicator width, and the right chord moves left by one width, using the blank space around the page count. Nominal layout width is 64 units; native screen-coordinate dimensions determine the actual movement at each UI scale. Page-count text, native layout assets, font scale (0.75) and main-tab indicators are unchanged. This reduces the visual gaps around PAGE without moving or shrinking its text.

No new memory fields, addresses or hooks. Uses the previously documented scoped draw rectangle. Updated geometry tests assert center preservation for both directions. Merged Release and 7/7 suites passed; source mirror is synced but was not separately rebuilt for this spacing-only revision. Live spacing still requires visual confirmation after restart.

## 1.5.8 indicator layout follow-up

The user confirms 1.5.7 page controls function. Their screenshot shows only LT and RT beside Page 1/105, so the compound indicator was not visually qualified by the earlier passing string tests.

The old hook substituted a multi-button string into the original 64x64 one-button draw rectangle and reduced draw scale. Native text layout still receives width/height constraints separately: 0x902E20 forwards the text, rectangle, style and scale to HD renderer 0x907B70; at 0x907C2C it reads rectangle+0x0C as integer height and at 0x907C8A reads +0x08 as integer width; at 0x907CB8 these limits are supplied to 0x90ADB0. The borrowed rectangle contract is four 32-bit integers x/y/width/height. shared-glyph-layout-disassembly.txt preserves the inspected HD path from the same runtime image hash recorded below. These are newly documented analysis witnesses, not new hooks or directly called functions.

1.5.8 gives Shared chord text a local copy of its rectangle, three times as wide, extending away from the page count. Left retains its original inner/right edge; right retains its inner/left edge. Scale is 0.75 rather than 0.55. Neither the native rectangle, widget nor assets are modified. Invalid dimensions fall back to original rendering. Shared legend ancestry now takes precedence over generated Cycle/Tab indicator children, preventing generic child names from obscuring the owning Shared hint.

Insufficient layout width is consistent with the screenshot; the actual game draw sequence has not been captured, so clipping versus nested-indicator precedence is not independently established. Both identified code limitations are addressed. No input logic changed. Visual placement remains pending an in-game check, including whether an ancestor clip rectangle restricts the expanded area. Existing glyph hook entry guards remain unchanged.

Validation: 7/7 merged suites and 3/3 mirror suites passed. Shared suite now has 33 checks, including nested-name precedence, outward rectangle geometry, original rectangle immutability and invalid/unrelated rectangle refusal. These tests do not substitute for visual verification.

## 1.5.6 failure: upstream trigger suppression

The saved 1.5.6-runtime-failure.log admits the BankPanel hook at 18:50:57.749 but contains no mapped page press. At 18:51:39.947 physical LT rises with LB held; the next entry routes it to loot slot L2, without a ControllerActionBegin reaching the tab handler. The same pattern repeats for RT. This distinguishes a missing upstream input from a failed native page call.

ProcessGamepadLeap in controller_input.cpp unconditionally zeroed chorded analog triggers for ground-loot slots before the game could produce trigger UI messages. 1.5.6 unit tests exercised the new message state machine but missed this pre-existing input filter.

1.5.7 adds a pass-through predicate to that existing filter. It is true only while stash UI is open and the admitted BankPanel hook has observed selected tab 1 with LB-mode QOL enabled. BankPanel updates an atomic context snapshot before and after its original handler (including tab changes); close/unload and cleanup clear it. The raw hook reads the snapshot without querying native widgets or acquiring the navigation mutex. Only analog trigger suppression is bypassed; face-button/R3 handling is unchanged. The ground-loot portion of the existing poll loop skips open stash after inventory shortcut handling, preventing duplicate pickup-slot requests.

No new hook, RVA or native pointer layout was added in 1.5.7. The same 0x23CAA0 hook and 0x23AF50 getter are reused. Existing input-boundary predicate TestUiMode uses the previously resolved D2RCore adapter. A new FilterLootTrigger policy test verifies that Shared navigation survives the raw filter while ground-loot trigger masking remains active elsewhere. The Shared suite has 28 checks; merged 7/7 and mirror 3/3 suites pass. Bounded input diagnostics now log action/tab/begin/repeat/LB/decision at BankPanel. Live page turning remains pending.

## Behavior and implementation

On the Shared stash tab, hold LB and press LT/RT for previous/next Shared page. Unmodified LT/RT retain main-tab navigation. Bare LB/RB do not change Shared pages. Physical input state is never edited, so LB remains available to inventory shortcuts. Each trigger must release before another page turn. Repeats after releasing LB first stay consumed until the owned trigger releases. Outside Shared, original BankPanel navigation is retained.

The existing TabBar hook alone cannot implement this: BankPanel handles cycleLeft/right directly before delegating to its widget children. The new detour is installed using PluginContext::InstallInlineHook at BankPanel's native message handler. It copies only the synchronous message/payload view and maps the trigger action to the original native cycle action. The original handler performs page bounds, season handling and page changes. Child widgets receive the copied cycle action rather than the original main-tab trigger. No custom page index mutation, packet, synthetic key or parallel polling path is introduced.

The hook consumes only BankPanel's original bumper navigation on Shared. Other controller messages and keyboard/mouse BankPanel messages pass through. QOL disabled, non-LB modifier mode, unavailable physical input or binary-profile failure retain original input. Profile failure also retains original glyphs.

## Evidence and native registry

Source is the prior decrypted work/runtime-game.exe SHA-256 81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5, associated with the current 3.3.93787 executable. All addresses below are D2R.exe RVAs, added to PluginContext::exeBase. Protected on-disk executable code is not a substitute. Full disassembly from 0x23A000..0x23F000 is saved in shared-page-disassembly.txt. Expected bytes and hashes are in shared-page-evidence.json; checked-in header is shared_page_signatures.h.

| RVA / field | Finding and proof |
| --- | --- |
| 0x23CAA0..0x23CC36 | BankPanel message handler, void(bank*,message*) in RCX/RDX. Recognizes InputMessage and BankPanelMessage hashes. Whole 406-byte body checked before hook install; 16-byte instruction-aligned prologue passed to SDK trampoline |
| 0x23CAB0 | InputMessage hash 0x4AE067C6248B6042 |
| 0x23CAC2 | BankPanelMessage hash 0x89CA1B1C5996F44E; tail-calls 0x23BCF0 command handler |
| 0x23CAEA | ControllerActionBegin hash 0x1E439768F7DD4594 |
| 0x23CB6E..0x23CB93 | Reads message+0x110 payload, action at payload+0x10; subtracts 5,5,9,1 to identify actions 19/20 |
| 0x23CBCC..0x23CBD4 | Action 19/cycleLeft -> 0x23A730(bank,false,true) |
| 0x23CBB8..0x23CBC2 | Action 20/cycleRight -> 0x23A730(bank,true,true) |
| 0x23A730 | Native page-turn implementation; at 0x23A749 calls selected-tab getter and at 0x23A74E requires tab 1. Retained behind the original handler, not directly called |
| 0x23AF50..0x23AFD2 | Selected-tab getter: BankTabs lookup/type validation, dword TabBar+0x16C4 -> uint8. Full 130-byte body admitted before installing new hook |
| 0x23CB54 | Tail call to 0x856B00 base widget handler, retaining native child message propagation |
| message+0x88, +0x110 | Command hash and payload pointer, same contract as existing menu/tab hooks |
| payload+0x10, +0x18 | Action and repeat byte, inherited from reviewed menu message contract. Borrowed copy sizes 0x120/0x20 are unchanged from existing MenuCopy |
| ControllerActionEnd hash 0x4D833CC71F08B3C0 | Existing logged end-message identity; clears per-trigger ownership |

The action meanings 7/8 = LT/RT and 19/20 = LB/RB are established by existing menu/glyph work. The native handler proves which cycle actions invoke the page-turn function and in which direction. This is a static ABI conclusion; runtime admission and manual behavior checks are still required.

## Prompt rendering

Existing SDK-installed glyph rendering hooks at 0x86D410 and 0x902E20 are reused. Only descendants of StashLeftArrow/StashRightArrow under BankExpansionLayout receive chord text. The controller layout defines these as ButtonLegendWidget inside SharedStashTabContainer/StashNavigation with 64x64 rectangles. Left text is native action glyph E027 + E01B (LB+LT); right is E027 + E01C (LB+RT). Borrowed draw scale is multiplied by 0.55 to fit two glyphs into the existing one-glyph legend. No layout asset or persistent widget state is modified. Other legends remain unchanged. Visual fit must be checked in-game.

## Revisit after a patch

1. Capture decrypted runtime code with tools/capture_code.py, then run `python tools/audit_shared_pages.py --image <runtime-code.exe>`. This only compares code, never regenerates admission bytes.
2. If mismatch, locate the handler by its InputMessage/BankPanelMessage and ControllerActionBegin constants, then confirm action decoding and both calls into the same page-turn function. Confirm that callee still gates on selected tab 1.
3. Recheck message offsets, repeat/end semantics, handler ABI, child propagation and selected-tab getter field/type validation. Disassemble with `dumpbin /disasm:bytes /range:<base+start>,<base+end> <runtime-code.exe>`.
4. Update the registry and header only after review; choose complete prologue instructions for SDK installation. Do not mask hook bytes or blindly admit a new image.
5. Inspect installed controller bankexpansionlayouthd.json for legend names, ancestry and size. Recheck native glyph mapping. Run routing/glyph tests and the live checklist.

## Verification

Merged Release and 7/7 CTest suites passed; mirror Release and 3/3 passed. The new suite has 24 checks covering directions, repeats, duplicate begin, modifier-first release, rearming, inactive tab, bare bumpers, face actions, disabled behavior, initial repeat and scoped glyph strings. The read-only evidence audit matches both reviewed ranges. Older-code compiler warnings remain.

**No live qualification yet.** Restart D2R, open Shared and test LB alone, RB alone, LT/RT alone, then LB+LT/RT. Hold each chord, release LB first, repress LB while the trigger remains held, and ensure one Shared turn with no main-tab change until release. Confirm LB+X/Y and potion shortcuts, other stash tabs, first/last Shared page, previous-season pages, close/reopen and prompt fit. Startup should report guarded BankPanel remap installed; each mapped press logs [QOL/Shared].
