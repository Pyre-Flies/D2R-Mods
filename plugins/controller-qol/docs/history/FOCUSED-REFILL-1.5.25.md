# 1.5.25 focused Materials refill and potion-only footer

User verified the 1.5.24 repairs, then reported small rejuvenation focus being ignored in favor of full rejuvenations and Fill Belt appearing on all items.

Previously the refill queue always used RejuvenationCode(0) (rvl/full), then RejuvenationCode(1) (rvs/small), independent of cursor focus. TriggerAutoFillBelt now schedules UI-thread focus inspection using ItemService::getItemInfo and the current Materials tab. RequestFocusedRefill resolves the existing advanced slot for that code and verifies focused runtime identity through FocusMatches (bound item or display proxy). It captures only the item code, not a native pointer. Each later tick re-resolves the slot, validates the same selected tab/season and native contract, submits the existing withdrawal, and waits for quantity confirmation.

A focused Materials potion bypasses generic inventory enumeration and refills from that exact stack. It stops on exhausted stock or incompatible belt space; it never falls through to a different potion type. Ordinary inventory refill remains unchanged. Invalid/non-potion Materials focus does not select a default rejuvenation. Existing UI-thread scheduling and no cross-thread native pointers remain requirements.

canAutoFill now requires quickMove and IsPotionItem(code), which includes health, mana, rejuvenation and utility potions but excludes identify/portal scrolls and all other items. Footer visibility retains inventory/storage context. Other action-footer lines are unchanged.

No new addresses, exports or hooks. See LABEL-STASH-1.5.23.md and REFILL-SELL-1.5.24.md for the reused native contracts. Added tests prove small-potion selection overrides full default, selected type remains fixed and focused refill never switches families on exhaustion. Runtime acceptance: highlight small/full rejuvenations separately, verify the selected stock decreases only; check potion and non-potion footers; verify menu and Ctrl+click behavior remains intact.
