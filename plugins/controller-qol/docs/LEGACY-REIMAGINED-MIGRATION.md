# Legacy Reimagined extraction and evidence migration

Audited: 2026-09-26.

## Purpose

Controller QOL was originally developed inside a D2R Reimagined checkout. Its maintained home is now `plugins/controller-qol` in the standalone D2R-Mods repository. This record preserves the relationship between those histories and the durable conclusions from local runtime logs that are intentionally excluded by repository policy.

No files were removed from the former checkout during this migration audit.

## Repository lineage

| Role | Repository state |
| --- | --- |
| Former development checkout | D2R Reimagined `next`, base `0148b1b9486d36640a48432921a82b203a7bad46`, head `2d29b71c06e81f301a1d249dad151893f6183e99` |
| Extracted repository | D2R-Mods `main`, initial import `98cb2305e0506d1f59aa2f0256e0041ae3397a0c` |
| Reimagined fork checkout retained separately | Pyre-Flies fork with official upstream configured; not the canonical Controller QOL source |

The former head is 46 commits after the base. Those commits are Controller QOL development: 99 changed paths are under `plugins/` and one is under `data/`. They must not be interpreted as 46 general Reimagined upstream commits.

The extracted tree preserves the substantive source, tests, tools and research. Core implementation files such as `src/controller_input.cpp`, `src/plugin_main.cpp`, `src/controller_qol.rc`, and `controller-qol-updates.toml` matched the former working tree byte-for-byte at audit time. Differences in retained research files replace private machine paths with placeholders. The extracted CMake project uses the monorepo's reviewed `third_party/D2RLoader-PluginSDK` submodule instead of carrying a second plugin-local SDK copy, and `tools/audit_native_controller.py` now requires explicit PID and module bases instead of hardcoded local values.

## Legacy commit sequence

Oldest to newest after base `0148b1b9`:

1. `c74c53c3` - release v1.0.0 with Left Trigger + A Quick Identify and container QoL
2. `aedb9d42` - native button legend widget and tooltip identify prompt
3. `94598761` - Description tooltip listener and unidentified-item coverage
4. `4156e20e` - native ground-loot pickup engine with action-table hooks
5. `5e5e88e8` - ground-item inspection diagnosis without vacuuming
6. `6f7ace20` - resolve real game via `GetGame(player)` for unit enumeration
7. `64ebe72a` - extract item quality and flags from native `UnitAny` item data
8. `6810a3ae` - multi-button selective ground pickup
9. `b805fcbc` - synchronized packet/game-thread pickup dispatch
10. `efa79275` - sticky loot assignments and LB tap pickup
11. `b49a2be0` - rarity-versus-distance mapping and R1 change
12. `f1a29216` - ground-item inspection and diagnostics
13. `6dfa2734` - placard layout fingerprint and `UnitAny` diagnostics
14. `44bfcfea` - pointer dereference and placard bytecode evidence
15. `027c3934` - ground-label button overlay and item-priority ranking
16. `666b4a8d` - face-button suppression experiment
17. `6685c323` - revert face-button suppression experiment
18. `048dfb3d` - native pickup suppression and stable input restoration
19. `c9a0ec4d` - dynamic placard tags while L1 is held
20. `9805d774` - seven ground-loot slots and golden image
21. `e6c907c0` - repair tooltip color encoding
22. `81b8ec34` - `CfgShowItems` live scan and caller diagnostics
23. `6fb3a072` - persistent ground-placard hook experiment
24. `b21b691c` - revert persistent ground-placard hook experiment
25. `e234127f` - configurable ground-pickup modifier
26. `b7951050` - modifier-based quick identify and quick move
27. `f908b399` - smart identify/move and inventory ground-loot pause
28. `e5b95090` - remove failed custom-move intercept and restore native transfer
29. `8c7c8b51` - robust item resolution and candidate destinations
30. `b271ca60` - gate quick move by active UI mode
31. `4494a99e` - controller grid focus-centering experiment
32. `733f823e` - revert controller grid focus-centering experiment
33. `e5b79a01` - modifier plus D-pad spatial inventory leap
34. `cc93d59d` - XInput caller diagnostics
35. `93740fea` - remove startup-crashing XInput trampoline hooks
36. `8f7bfe73` - guarded smart inventory traversal hooks
37. `52a32e4f` - strictly one-dimensional inventory raycasting
38. `d8c930b3` - repair pulse undershoot and preserve sub-cell coordinates
39. `8e45e47b` - separate bounds clamping from in-flight tooltip state
40. `256dc562` - faster spatial-leap pulse experiment
41. `7e4f63fe` - restore stable pulse timing
42. `7b67da08` - change quick-move shortcut while retaining identify
43. `a60bbbfe` - first-input handling, robust handles and multiline tooltip
44. `628a1009` - right-stick panel-flick experiment
45. `d206682d` - revert panel flick and repair cross-panel focus
46. `2d29b71c` - native Hold-X-to-Sell vendor integration

This lineage is historical provenance, not a compatibility promise. Current source, signature headers, research records and validation notes are authoritative.

## Distilled runtime evidence

Raw local logs were not copied because repository policy excludes local logs and machine-specific output. Their SHA-256 hashes preserve artifact identity.

| Legacy artifact | SHA-256 | Durable conclusion |
| --- | --- | --- |
| `1.5.1-runtime-excerpt.log` | `2BE652A11E43BECA04BEF4FD2F9E852A839FFBAAC321EB9C8E1216199E2A432E` | Native belt contract admitted. Stored-item submissions were followed by SDK observation in Belt: runtime ID 38 at 17:52:58, IDs 37/39 at 17:53:20, and full rejuvenations 51/52 at 17:53:33. Shared-stash quick moves also resolved stale handles and reported successful transfer. This validates the sampled inventory/belt and shared-transfer behavior, not Materials withdrawal or all item families. |
| `1.5.2-runtime-failure.log` | `59DE544C1B56C11C0401067EB048D795DAA0A1F1D2C7D757DF71143A441CD915` | Materials and belt profiles admitted at 18:15:22, but the tested advanced-stash actions followed legacy quick-move paths or stopped when the stash/tab changed. No advanced withdrawal was submitted. An ordinary belt placement later submitted and confirmed at 18:17:30. |
| `1.5.3-runtime.log` | `B66F5A683DA2140018C7BBFF414040BA273AD95DAFF335C9AA8F4F845787CCB6` | Materials layout binding succeeded. Rejuvenation refill submitted three times and ended with `confirmed=3`; advanced-stash withdrawals for observed item codes submitted and completed; inventory-to-Cube routing triggered while the Cube panel itself was not reported as a quick-move source. This is sampled live behavior, not exhaustive container qualification. |
| `1.5.6-runtime-failure.log` | `EFDB0AC845F0A8D0FFC8F89F7C333DEFF9AC7E193660C0F4AE72AF8208C83A35` | Guarded BankPanel remap installed at 18:50:57.749. LB+trigger input was captured upstream as loot-slot input before the tab handler received a mapped page press, distinguishing an input-routing conflict from a rejected native page call. |
| `ctest.log` | `5D56867013B3FED5E69A282510D0158DABCA0D60A20330EE08D9A936C700CA0A` | Historical generated test output only; excluded because current test execution and versioned test sources supersede it. |

## Reimagined gameplay context retained

The Controller QOL documents deliberately distinguish ordinary inventory, personal/shared stash, the Reimagined Gems/Materials surfaces, embedded Cube interactions, belt refill, vendors, ground labels and portals. These are separate player surfaces with different focus, ownership, page identity and native action paths. Do not generalize one successful path to every storage tab.

The Reimagined-specific conclusions are indexed in [README.md](README.md), especially the Materials, Cube, Shared deposit/page, refill and selling documents. Exact item, skill, recipe and current mod balance data should still be checked against the installed Reimagined revision or official live data rather than inferred from Controller QOL history.

## Excluded material

- Old release ZIPs, golden packages and packaged DLLs: obsolete release artifacts.
- `build-potions` and other build trees: generated output.
- Plugin-local `sdk/`: duplicated by the reviewed monorepo SDK submodule.
- Raw runtime and CTest logs: local/generated artifacts; durable conclusions and hashes are recorded above.
- `.vscode` state and machine-local paths: editor or host-specific configuration.

The former checkout should remain untouched until the user confirms that no additional non-Controller-QOL material is needed. This document is the migration record; it is not authorization to delete that checkout.
