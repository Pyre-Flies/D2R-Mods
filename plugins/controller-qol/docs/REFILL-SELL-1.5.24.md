# 1.5.24 refill, keyboard pass-through and selling

User confirmed startup, menus, ground looting, label lock, transfers and game re-entry in 1.5.23. Remaining observations: refill drains normal stash before Materials; keyboard Ctrl+click does nothing; NPC sell refuses.

Bulk refill previously enumerated Inventory, PersonalStash, SharedStash, then requested Materials. It now enumerates inventory only before the existing UI-thread Materials rejuvenation refill. Single focused-item belt actions retain their original supported sources. Materials still requires its tab (3) to stay open, season to remain unchanged, stock eligibility and a compatible belt slot, and confirms destination quantities before another withdrawal. Separate stock/capacity messages replace the ambiguous preflight result. It never switches the user's page or drains a hidden ordinary stash.

OnItemInteraction previously treated keyboard Control as the QOL modifier and consumed potion clicks for belt work (also unidentified items for identification). It now returns Continue for non-controller events before QOL inspection/action scheduling. Native Ctrl+click gets its original event; no deposit hook was installed for LB+X. Existing controller shortcuts remain.

Selling: live read-only comparison of the 1651-byte transaction profile at D2R RVA 0x10D160 found only six differing bytes, all within three rel32 calls: +0x218, +0x256, +0x28C. 1.3.1 thunks observed at D2R RVAs 0x3E2B736 and 0x3E2B730 target named D2RCore exports ResolveNamespacedStringKey (ordinal 1201, current core RVA 0x81ECD0) and GetNamespacedStringById (ordinal 1202, current core RVA 0x81F290). Export table and disassembly confirm string resolution, not changed sale routing. vendor-1.3.1-live.json preserves complete old/new bytes. These thunk/core RVAs are evidence only; code resolves by exported name.

ValidateTransaction accepts the prior full profile or requires every byte outside the three call displacements to match, including E8 opcodes; each changed call must go through FF25 and resolve exactly to its expected named export in loaded D2RCore. Unknown targets, changed transaction logic or unreadable memory fail closed. Original native vendor wrapper 0x23FED0 still performs eligibility, pricing and transaction dispatch; no custom packet/gold mutation. All other vendor guards are unchanged.

Twelve automated suites cover prior policies and relocation-body rejection. In-game selling, Materials refill and Ctrl+click must still be tested. Global d2rloader only; preserve existing TOML and all golden archives.
