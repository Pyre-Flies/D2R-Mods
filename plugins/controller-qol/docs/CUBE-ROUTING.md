> Historical 1.5.4 behavior, superseded by [MATERIALS-CUBE-ROUTING.md](MATERIALS-CUBE-ROUTING.md).

# Embedded Cube routing — 1.5.4-cube

## Evidence and cause

The user reports the Horadric Cube is embedded in Materials. The installed controller layout places advancedstash_horadriccube as a BankExpansionLayout child, containing horadriccube_grid (InventoryGridWidget), convert and horadriccube_cover. Its presence is not equivalent to standalone UI mode 0x19.

Saved 1.5.3-runtime.log shows:

- 18:27:48–52: focused SDK container 5 (Cube), code 0x206C676C, was intercepted by Materials while native BankTabs still reported tab 4. Resolve stage=slot-missing suppressed ordinary withdrawal. Cube contents are real items, not advanced counter proxies.
- 18:27:59: the same Cube source successfully used an SDK transaction into Inventory after the ordinary stash tab became active.
- 18:28:04: inventory item routed to PersonalStash with Stash=1, Cube=0. Legacy destination selection prioritized stash, and ExecuteMoveTask independently forced every inventory move through stash smart-deposit whenever stash mode was open. The latter also affected explicit LB+Y Cube requests.
- The same run records advanced withdrawals followed by inventory quantity increases, including Um at 18:27:21. The panel-name correction reached the native withdrawal path successfully.

## Correction and exact behavior

Materials admission now only considers observed proxy-compatible containers Cursor, SharedStash and CustomPage. Cube, Inventory and PersonalStash bypass it. Cube stale-handle searches enumerate only Cube, preventing a same-position stash item from being selected.

LB+X on a Cube item records the current stash tab as the Cube return context before using the existing game-thread transaction. Subsequent inventory LB+X targets Cube while that tab remains current. Focusing a different non-inventory/non-Cube item clears the context. Closing stash clears it on the existing polling loop; a different tab observed at the next action also clears it. Standalone Cube mode selects Cube directly. Without a Cube return context, the existing stash route remains the default; LB+Y remains the explicit Cube shortcut. No inference from mere Cube-widget existence overrides stash deposits.

The game-thread move now honors the requested destination: stash smart-deposit runs only for a requested PersonalStash/SharedStash destination. Cube has no secondary stash destination, so a full Cube cannot spill the request into Personal Stash. Context is routing state, not proof that the earlier withdrawal succeeded.

## SDK and address contract

No new native addresses, hooks or widget offsets were introduced. SelectedStashTab executes on the SDK UI thread and reuses the admitted 0x846170 name lookup for BankExpansionLayout and 0x23AF50 selected-tab getter. These remain protected by the existing Materials profile. Item classification uses SDK ItemInfo.container. Actual Cube movement uses runOnGameThread and executeExistingItemTransaction with automatic placement. See MATERIALS-NATIVE-CONTRACT.md for image hashes, native profile evidence and patch-recovery steps.

## Validation

Merged Release build and 6/6 suites passed; source mirror 2/2 passed. Materials/policy suite now has 29 checks, including exclusion of ordinary containers, observed proxy admission, Cube return context, changed/unknown tabs and explicit Cube destination protection. Existing compiler warnings remain in older code. **1.5.4 has not yet been tested in-game.**

Restart D2R. On embedded Cube, withdraw an item with LB+X, focus it in inventory and press LB+X to return it. Test on Materials and with a different BankTabs selection. Then focus a stash item and verify normal inventory-to-stash routing resumes. Test full Cube (no stash fallback), full inventory, panel close/reopen and LB+Y while stash is open. Check [QOL/Cube] and ExecuteMoveTask source/destination: Cube=5, Inventory=4, PersonalStash=7.
