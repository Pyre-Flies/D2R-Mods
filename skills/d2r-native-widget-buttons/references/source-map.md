# Inventory Deposit source map

All paths in this file are **relative to the verified D2R-Mods repository root**,
not to the installed skill. Use the user's checkout or an explicit supplied
repository path. The worked example was established on D2R 3.3.93787 with an
exact D2RCore 1.3.1-beta artifact. Those versions are evidence labels, not a
portable compatibility promise; obtain hashes and callable contracts from the
current records and guard headers below.

## Read by task

| Task | Canonical evidence | Implementation/test entry points |
| --- | --- | --- |
| Add a child without a panel replacement or MPQ | `plugins/inventory-deposit/docs/INVENTORY-DEPOSIT-BUTTON.md`, supported extension mechanism | `src/deposit_button.cpp`, `tests/deposit_button_tests.cpp` |
| Understand allowed SDK registration and threading | `third_party/D2RLoader-PluginSDK/README.md`; `include/D2RLPlugin/panels.h`, `resources.h`, `widgets.h`, `threads.h`, `shared_events.h`, `lifecycle.h` within that SDK | Check service size/capability before each adopted API; use the SDK pinned by the target checkout |
| Move relative to a live sibling | `plugins/inventory-deposit/docs/GOLD-ANCHOR.md`, SDK limits, lookup/hitbox evidence, placement/refusal behavior | `src/gold_anchor.cpp`, `src/gold_anchor_policy.h`, `src/gold_anchor_profile.h`, `tests/gold_anchor_policy_tests.cpp` |
| Fix input-disabled but still-gold artwork | `plugins/inventory-deposit/docs/BUTTON-STATE.md`, rejected ordering hypothesis and native transition contracts | `src/button_state.cpp`, `src/button_state_profile.h`, `tests/button_state_tests.cpp` |
| Change or embed button artwork | `plugins/inventory-deposit/docs/BUTTON-SPRITES.md`; `plugins/inventory-deposit/assets/art/PROMPTS.md` for original-art provenance | `tools/build_button_sprites.py`, `src/button_art.cpp`, `src/button_art_resource.h`, `CMakeLists.txt`, `tests/artifact_tests.cpp` |
| Keep visible while unavailable; avoid unsolicited clicks | `plugins/inventory-deposit/docs/INVENTORY-DEPOSIT-BUTTON.md`, held-item/empty-inventory sections; corrected visual result in `BUTTON-STATE.md` | `src/plugin_main.cpp`, `src/deposit_button.cpp`, `src/deposit_eligibility.h`, `tests/plugin_wiring_tests.cpp`, `tests/deposit_eligibility_tests.cpp` |
| Add action feedback once per destination per batch | `plugins/inventory-deposit/docs/STASH-FEEDBACK.md` | `src/stash_feedback.cpp`, `src/stash_feedback_policy.h`, `src/stash_feedback_profile.h`, corresponding feedback tests |

Unless prefixed with a full repository path, `src/`, `tests/`, `tools/` and
`CMakeLists.txt` in the table refer to `plugins/inventory-deposit/`. Follow the
canonical records' links to inherited Controller QOL witnesses; do not copy an
RVA without its admission contract. Historical sections retain old dimensions
and pending statuses; consult later validation records and current source.

## Useful boundaries from the worked example

- Four frame indices are normal 0, hover 1, pressed 2, disabled 3. The original
  PNGs have equal square canvases, while the approved logical/raster button is
  144 by 136 and low-end is 72 by 68. These are Inventory Deposit dimensions,
  not requirements for every future button.
- The encoder supports the documented uncompressed `SpA1` format 31 only. It
  writes original RGBA data and a new header; it does not need donor game pixels
  or a proprietary sprite header. Generated sprites, previews, manifests, and
  resource scripts belong in build output, not duplicated skill assets.
- Source alpha edges explain a visible gap even when a logical rectangle fits.
  Resizing the art and resizing the hitbox must stay coordinated, but art fit
  still needs visual review at the actual scale.
- The native state probe relocates complete guarded enable/disable bodies into
  an isolated allocation, substitutes external calls, and uses a synthetic
  widget. Its mutation-footprint assertions prevent unexplained writes from
  being accepted as a successful test. It does not validate real text layout.
- Supplemental stash feedback follows observed source removal, not request
  submission. Per-destination batch identity survives timer expiry so later
  items cannot restart that batch's animation. This is an optional feedback
  pattern, not part of the general button registration contract.

For artifact admission, deployment or release work, use the repository's broader
`skills/d2rloader-development/SKILL.md` and its release reference when that work
is in scope. This button skill does not grant publishing authorization.
