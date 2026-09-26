# Materials and Cube shared action — 1.5.5-materials

This supersedes the remembered Cube-return destination in 1.5.4. The user reported it still failed and clarified the desired rule: Materials contains both storage counters and the Cube; LB+X must reuse the existing send-to-storage and LB+Y Cube actions. No previous withdrawal or focus history is required.

## Routing

- Inventory + Materials tab (BankTabs index 3): QueueQuickMoveToCube(true), the same queue/debounce/item resolver used by LB+Y. ExecuteMoveTask invokes the existing CanDepositToAdvancedStash and DepositToAdvancedStash helpers first. Eligible items go to advanced storage. Noneligible items continue through the existing Cube transaction. A failed eligible deposit stops instead of depositing into Personal Stash or Cube.
- Cube source: TriggerQuickMoveToCubeOnFocusedItem -> QueueQuickMoveToCube(false), exactly the LB+Y callback. Its existing source switch chooses Inventory for Cube contents.
- Advanced proxy source: existing QolMaterials::TryWithdrawFocused counter withdrawal remains first.
- LB+Y: its registered callback remains TriggerQuickMoveToCubeOnFocusedItem, now a thin wrapper over QueueQuickMoveToCube(false); it always retains the explicit Cube behavior.
- Other stash tabs keep the normal smart-stash action. Cube destinations cannot enter Personal Stash deposit logic.

CubeReturnTab and PreferCube were removed. Materials routing depends only on the current selected tab and source container. No alternate Cube move implementation or new hook was added. SDK UI scheduling resolves the tab; SDK game scheduling and executeExistingItemTransaction perform the shared Cube move. The Materials smart-deposit pointer is used only inside editNativeItem's synchronous callback. The earlier Cube-only stale-handle filter remains in both the shared queue and executor.

## Existing native path registry

These are reused addresses from include/native_d2r.h, not newly discovered or changed RVAs. Address form is D2R.exe base + RVA. The predicate is the existing generic advanced-storage eligibility check, so advanced-eligible runes/gems also use native storage rather than Cube.

| RVA | Existing helper and role |
| --- | --- |
| 0x846170 | FindTopLevelPanelByName("BankExpansionLayout") |
| 0x23AF50 | BankPanel selected tab; 3 selects Materials |
| 0x15A0B0 | CanDepositToAdvancedStash(item*) |
| 0x46DA50 | GetAdvancedStashDestination(player*) |
| 0x15F8B0 | TransferItemToInventoryPage(item,destination,4,0,true,placement) inside existing deposit helper |
| 0x1A0780 | Existing deposit helper completion with (3,null,0,0,false) |

For original image hashes and locator/admission tooling, see MATERIALS-NATIVE-CONTRACT.md and BELT-NATIVE-CONTRACT.md. The deposit helper above predates these fixes; reusing it is not a new live verification of its return semantics. Logs say submitted, not confirmed. After a patch, verify this inherited predicate/deposit chain as well as the Materials lookup profile; the Materials profile does not guard every byte of the inherited predicate.

## Verification

Merged MSVC Release build and all 6 CTest suites passed; mirror Release and 2 suites passed. Materials/policy retains 29 checks, replacing remembered-destination tests with direct Materials-tab routing checks. Existing older-code warnings remain. Automated tests do not run the game. **1.5.5 still needs in-game qualification.**

Restart, open Materials, and immediately LB+X an ordinary inventory item: it should use the same Cube path as LB+Y without a prior Cube withdrawal. Test an eligible material goes to storage; Cube LB+X returns to inventory; LB+Y still sends explicitly to Cube; full Cube and failed storage deposits do not spill into Personal Stash. Look for LB+X -> existing Cube action with smart storage first, followed by either smart-deposit submitted or continuing existing LB+Y Cube move. Existing move logs show source/destination enums: Inventory=4, Cube=5.
