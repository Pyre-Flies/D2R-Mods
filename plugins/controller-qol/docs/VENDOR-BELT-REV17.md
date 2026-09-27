# Vendor potion refill - revision 17 candidate

## Scope and build

D2RLoader 1.3.1 / SDK ABI 4, current reviewed Core SHA256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
RVAs below are relative to the game image, not Core. All are build-specific.
Investigation used bounded, read-only live disassembly on 2026-09-27 with the
merchant panel open. No remote native invocation or test purchase was made.

## Evidence and call contract

- Native shop handler `0x23FF90`, branch `0x2400EE..0x240128`, resolves the
  selected stock through its grid and calls `0x23FED0(panel, player, item,
  sell=false, immediate, forceShift=false)`.
- Existing reviewed `0x23FED0` has ABI `void __fastcall(void*,void*,void*,bool,
  bool,bool)`. It combines key 0x10 state (via `0x120A100`) with the sixth
  argument, then invokes transaction `0x10D160`. The new shortcut passes
  **false, true, true** (buy, immediate, force Shift). Selling retains its
  existing **true, true, false** arguments.
- Buy branch `0x10D612` performs native checks/pricing. At `0x10D72C..0x10D73A`
  it forwards the Shift bit in R8B to `0x1114C0`.
- `0x1114C0` stores this bit at stack +0x40. Its purchase path includes
  `0x111FCD` (Shift test), `0x112129` (native belt slot lookup `0x3862D0`),
  and subsequent native transaction/send processing. These are research
  landmarks, not additional hooks or direct plugin calls.
- User independently confirmed normal **Shift+right-click** on vendor potions
  fills empty belt slots in this mod configuration. This validates the intended
  native action; it does not yet validate QOL's new invocation.

## Stock resolution and admission

- `0x846190(0x0B)` resolves the vendor panel. Live UI tree identified
  `VendorPanelLayout` and its child `grid`; visibility bytes are +0x50/+0x51.
- `0x856220(panel, "grid")` resolves the child; `0x2C49F0(grid, &cell)` accepts
  a pointer to two int32 coordinates, not packed coordinates.
- `0x34A330(item)` must match captured runtimeId and `0x36EF50(item)` must
  match the captured potion code. Thus a stale SDK container classification
  cannot select Personal Stash or another inventory for a purchase.
- Reuse `vendor_signatures.h`, `vendor_compatibility.h` (including only its
  already-reviewed namespaced-string call relocations), and existing game
  witnesses in `custom_page_profile.h`. Checks are read-only/quiet, not SDK
  patch attempts. Vendor validation currently retains the full existing
  selling profile as a conservative superset.
- Belt admission rechecks the local-player/inventory/slot contracts, including
  the reviewed Potion Auto Pickup and Auto Belt Refill compatibility paths.
  Those plugins remain optional. No new hooks are installed.

## Ordering, ownership and failure behavior

A UI-thread request snapshots ItemInfo and SDK player identity, verifies shop
context and stock binding, and begins a lifecycle-bounded belt batch. Collect
only Inventory items with the exact highlighted stock code. Existing belt
moves must be observed in the belt before advancing; timeout/failure cancels
without buying. When enumeration completes, schedule one UI task to recheck
shop context, player, stock identity, contract and compatible free belt space.
Only then invoke native Shift-buy once. Native code owns pricing, available
gold, stock, accepted quantity and placement. There is no blind retry and no
claim of purchase confirmation from a void native return.

No native item/player/widget pointer is retained across tasks. Requests expire
at ten seconds. GameJoined/GameLeft reset the batch; monotonically changing
request generations reject stale UI/game callbacks after reset or timeout.
Inventory refills outside the vendor keep their existing behavior. A full
belt causes no purchase. Closing the shop before the purchase task cancels it.
The same merchant stock must still occupy the captured cell; changing highlight
alone does not retarget an already requested refill.

## Tooltip behavior

When shopping, non-player-inventory stock no longer advertises player-storage
Transfer, To Cube, Identify or To Belt. Potions get `Refill Belt / Buy Missing`
on R3 with the configured modifier. Player inventory retains Sell. Existing
controller-mode gating remains in effect.

## Validation and remaining live checks

Build and existing suites plus buy ABI/null-dependency/single-invocation
regressions are required before installation. Automated tests do not simulate
native purchasing, gold deduction, multiplayer acceptance or stock changes.
Live candidate checks: selected potion with matching inventory supply and
empty slots; no inventory supply; full belt; mixed potion columns; insufficient
gold; ordinary non-potion vendor prompts; LB+X Sell; closing shop during refill.
The new purchase path remains a candidate until these are exercised in game.

## Candidate installation

All 18 automated suites passed, including the re-enabled DLL artifact test
(metadata version, ABI manifest, lifecycle exports and embedded defaults).
Installed rev.17 after observing game and loader closed; backed up rev.16.
Built and installed SHA256:
`70BB9A21E399D3A10ADFC2C4E2266B6ACC8ABCE095F2A4E4693DA80E9EB1C756`.
Plugin-driven live behavior remains pending user verification.
