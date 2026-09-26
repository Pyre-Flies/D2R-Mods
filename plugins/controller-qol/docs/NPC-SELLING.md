# NPC quick sell — 1.5.14-native-sell

2026-09-23. Golden rollback is outputs/QOL-1.5.13-golden.zip (accepted by user before selling work). This revision is build-tested, not live-qualified. Do not label native invocation as confirmed item removal or gold credit.

## Discovery and choice

The existing LB+X shortcut called a 650ms synthetic X hold. The vendor predicate accepted shop, NPC dialogue, gamble and trade modes, and an input-hook predicate also armed selling state during polling. A background monitor interpreted a missing SDK handle as a completed sale, even though handles can become stale across threads. This implementation removes that monitor/state and disables the vendor synthetic-hold predicate. Normal unmodified X input follows the game; LB+X retains the existing edge-triggered shortcut.

SDK inspection: item.h ExistingItemTransaction does not provide a sell operation; item_interactions.h explicitly excludes Vendor in V1. network.h exposes plugin transport channels, not a native item-sale transaction API. We continue using D2RLoader UI scheduling, ItemInfo identity and CheckExpectedBytes. The native game owns vendor pricing, item eligibility and network dispatch; no plugin-authored packet, gold write or item deletion is used.

The most direct evidence is the native inventory quick action at 0x2AA588..0x2AA66B. It checks UI mode 0x0B, native sellability 0x374370 and item mode !=2, locates the VendorPanel by mode, obtains the local player, and calls 0x23FED0 with (panel, player, item, true, true, false). This gives a concrete caller-derived ABI rather than guessing from packet documentation.

## RVAs and ABI

All RVAs are relative to loaded D2R.exe. Evidence image work/runtime-game.exe SHA256 81AF5ADEEF90F6BE190CA596CC39E6A6C794A47F0231F71A09C0DD27105904C5. vendor-native-disassembly.txt contains reviewed slices; vendor-evidence.json records exact guarded bytes and sizes.

| RVA / value | Evidence and use |
|---|---|
| 0x2AA588 | Quick-sell branch checks mode 0x0B via 0xCE500. Corrects old plugin assumption that shop was 0x16. |
| 0x846190 | panel pointer(int mode); native caller supplies 0x0B at 0x2AA5D7/0x2AA5DC. |
| 0x23FDF0 | VendorPanel type descriptor lookup used by native caller's type walk; recorded for patch recovery, not directly called by plugin. |
| 0x374370 | int(item*) native sale eligibility; zero refuses. |
| 0x34AB60 | int(unit*) mode; native caller rejects 2. |
| 0x09A5D0 | client unit lookup(id,type); type 4 is item. Resolves copied SDK runtimeId on UI thread. |
| 0x36EF50 | uint32(item*) code getter, compared with SDK focused code. |
| 0x36CFE0 | uint8(item*) native inventory page getter; require 0 for carried inventory. |
| 0x08B2D0 / 0x09A480 | Existing local context/player helpers. |
| 0x23FED0 | void(panel*,player*,item*,bool sell,bool immediate,bool forceShift). Caller at 0x2AA656 supplies true,true,false. First panel argument is unused in current body but passed faithfully. |
| 0x120A100 | Native wrapper queries key state 0x10 and combines it with forceShift. Plugin does not reproduce modifier handling. |
| 0x10D160 | int(player*,item*,int sell,bool shift,int immediate,bool special). Called by wrapper at 0x23FF2C. Validates active NPC, computes price, prepares transaction state and dispatches. |
| 0x2A48760 / 0x2A4875C | Native transaction's active-NPC state / ID, resolved as unit type 1. Documented only; plugin never writes them. |
| 0x36F0B0 | Native price calculation called in sell branch at 0x10D523. |
| 0x2A487D8 | Native computed price storage; documented only. |
| 0x2A48768 | Native transaction kind; sell branch stores 2 at 0x10D602. |
| 0x2A48888 / 0x2A48884 | Native item runtime ID / class ID transaction state. |
| 0x2A48894 | Native immediate-action time gate, checked against 500ms. |
| 0x1114C0 | Native transaction dispatch reached at 0x10D73A for immediate action. |
| 0x111D3B / 0x111D48 | Sell-kind branch resolves item ID as type4 and selects opcode 0x33 in R13B. Packet assembly/sending remains native; plugin does not assume raw packet layout. |

## Implementation and admission

LB+X enters existing runOnUiThread callback. Obtain a fresh SDK ItemInfo; require Inventory, shop mode 0x0B, stash closed and Cube closed. Stash/Cube behavior therefore retains priority. On that same UI callback resolve the exact client item by runtimeId and validate code/native page/sellability/mode. Borrow no SDK native pointer and retain no client pointer beyond the call. No position/code-only item substitution and no game-thread handle recovery is used for selling.

The plugin invokes the reviewed wrapper once with sell=true, immediate=true, forceShift=false and adds a 500ms UI-thread cooldown. The wrapper/native transaction decides whether the sale is accepted and handles engine UI cleanup. Missing context, profile mismatch or native fault produces a refusal log without retry, synthetic input or drop fallback. Logging says native quick-sell invoked, not completed. No automated live sale is performed during build verification.

vendor_signatures.h guards the complete wrapper (0xBC bytes), transaction body plus switch data (0x673 bytes), complete client lookup entry, and entry profiles for panel lookup/item access/eligibility. These are admission guards, not proof that every transitive callee is unchanged. Historical runtime-wrapper addresses in disassembly must not be mistaken for stable original functions.

## Validation and patch recovery

Seven CTest suites pass, including injected wrapper ABI flags, null-panel refusal, inventory-only shop routing and stash/Cube exclusions. They do not verify live vendor pricing, server acceptance or item/gold updates.

Live qualification: restart, open an NPC shop, focus an ordinary carried item and press LB+X; verify that item leaves inventory and gold increases according to native price. Check unsellable items, repeat press, closing shop, dialogue-only state, and stash/Cube regressions. Capture QOL/Sell logs if refused; an invocation line alone is not sale confirmation. Golden archive remains the pre-selling rollback.

After a patch, locate the inventory quick-sell branch by mode/eligibility sequence and VendorPanel action call. Re-establish all six arguments and UI threading before updating guards. Follow 0x23FED0 -> 0x10D160 -> 0x1114C0 -> sell-kind branch, verify native pricing and opcode choice, then compare code profiles. Use the PE comparison tool audit_shared_pages.py with --evidence docs/vendor-evidence.json for a read-only byte audit; never regenerate admission bytes without reviewing changed code.
