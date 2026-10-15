# Counter focus and aim startup - 2026-10-15

## Observed failures

The user joined a separate TCP/IP host running locally with rev.79-beta.2 on
both sides. Ordinary inventory/stash/Cube moves and Materials rejuvenation
withdrawals produced client-state confirmations. The user reported gem LB+X
withdrawal failing, with a visible Chipped Topaz tooltip. Rune withdrawal was
not tested. The log refused advanced focus before submitting a withdrawal.

A read-only UI sample found the selected `gcy` slot with a display item whose
runtime ID was zero and a distinct stocked bound item with runtime ID 28. The
existing action required a successful SDK tooltip-item lookup before routing
to the native counter. The fix observes the actual controller slot independently
of SDK tooltip delivery. A tooltip/display object is never submitted as stock.

The user also reported no aim reticle and no R3 skill toggling. A fresh client
and host, followed by a successful first join, reproduced an SDK allocation
failure at the second aim hook. The first hook remained installed pass-through;
the next lifecycle callback then rejected its own modified prefix. Reticle
updates and skill-tree input share the complete-aim-installation gate.

## Exact environment

| Artifact | SHA-256 |
| --- | --- |
| D2RLoader.exe, content 3.3.93787 | `93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10` |
| D2RCore.dll | `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2` |
| Published QOL rev.79-beta.2 | `2a11bcc2c4920e0be1d3aa16c5d8b3c850d4e6fa1911d1e82e9fa02a4c017e58` |
| Local QOL rev.79-beta.3 | `63387afc98248ce7a257ad2ab1567407b985c1b21a96dc2d53154b7a2950ba2f` |
| Local QOL rev.79-beta.4 | `9bb0a83e6078c5a5d58665ed13e2d6710987da46923471ac9a42ce040eaecfa9` |

Local diagnostic sources are retained outside version control under
`outputs/qol-gem-withdrawal-20261015/`. No server implementation, binary, save,
or unredacted runtime log belongs in the distributable.

## Controller counter contract

This reuses the reviewed game-relative witnesses in `storage_focus_profile.h`,
`filter_focus_signatures.h` and `materials_signatures.h`; no new game hook or
packet format is introduced. All addresses remain exact-build-specific.

| Existing path | Use and qualification |
| --- | --- |
| game `+0x3440170` -> manager `+0xD0` -> focus `+0x190` | Actual controller widget; focus getter `0x846020` and controller-focus witness `0x876160` remain exact |
| widget `+0x50/+0x51`, vtable `+0xC8` | Require enabled/visible AdvancedStashSlotWidget with getter `game+0x2CE900` |
| widget `+0x600/+0x608` | Display and bound stock respectively; the live display had ID 0 while the bound stock had nonzero ID and nonzero native availability |
| item `+0`, getter `0x34A330`, getter `0x36EF50` | Require native item type 4 and copy nonzero bound runtime ID plus code; no SDK handle is invented |
| `0x846170`, `0x23AF50`, `0x856220` | Resolve BankExpansionLayout, selected category 2/3/4, and code-named child; the result must be the same selected widget and bound item |
| `0x23B980`, `0x46D9A0`, `0x2CF680` | Preserve season selection, native stock availability, and the native single withdrawal wrapper (page 0 inventory / page 3 Cube) |

Idle UI observations are scheduled at most every 50 ms while controller stash
context is active; tooltip callbacks may refresh the same observation. A
physical LB+X/LB+Y press copies an observation at most 250 ms old. The mailbox
stores values only: bound identity, selected tab, season, SDK player and time.
Before submission the UI callback reacquires and matches the selected widget,
bound identity, native profile, tab, season, player, cursor and destination
baseline. A request expires before submission after 750 ms. Lifecycle resets
invalidate queued callbacks. Empty stock, changed selection or unavailable
observation never uses a same-code ordinary stash fallback.

The belt-only guards at `0x3862D0` (free-slot planner), `0x34A360` (unit inventory)
and `0x2CF88B` (belt caller witness) remain mandatory for Destination::Belt.
Inventory/Cube's exact `WithdrawWidget`/`WithdrawOne` route does not use those
three direct dependencies; its guard set retains all other existing witnesses.
A belt-only refusal leaves common counter admission intact. This reduces the
impact of the remaining Potion Auto Pickup version pin without admitting an
unreviewed belt implementation.

The existing one-request/confirmation behavior remains: native submission is
not success; observe an increase at the requested destination, or time out
without resending. Cube withdrawal additionally requires a carried Cube. No
pointer crosses UI callbacks and no counter, item or network state is edited.

## Aim allocation finding and correction

These are read-only findings from the exact Core above, not new callable APIs.

| Core RVA / offset | Evidence |
| --- | --- |
| `0x47D960..0x47DC78` | SDK inline-hook registration validates expected bytes, creates/enables a managed hook, records ownership, and reports errors |
| `0x8FE00..0x90090` | Hook creation calls the bounded trampoline allocator; allocation/record-allocation failures map to memory-allocation status |
| `0x129A80..0x12A088` | Trampoline allocator searches target +/- `0x40000000` (1 GiB), uses existing free slots, otherwise requests a `0x1000` page with allocation-granularity-aligned reservation |
| `0x7132A8` | Root of this allocator's pool list; private observation only |
| pool `+0/+8/+0x10` | Next pool, free-slot pointer, used-slot count; 64-byte slots start at `+0x40`, giving 63 slots in a 4 KiB page |

Live inspection after the failure found one eligible pool, all 63 slots used,
no free-slot pointer, and no suitable 64 KiB-aligned reservation in the permitted
range. Smaller free fragments were present. Process dynamic-code policy was zero;
that mitigation was not the blocker. The UnitTest entry retained pristine bytes,
while the Lookup entry led to QOL's installed relay. This supports bounded
trampoline exhaustion/fragmentation, not a foreign detour on UnitTest.

QOL now attempts its essential aim hooks during plugin initialization, after
native/profile/service admission, before entering gameplay. Native wrappers
remain pass-through outside an active enabled session. No expected bytes,
contact-relay ownership checks, Core identity, or optional-observer rules are
relaxed. An atomic one-attempt gate prevents duplicate/partial retry on repeated
join/ready events. Incomplete hook sets remain inert under SDK ownership until
loader teardown; QOL neither overwrites them nor implements an unsupported
per-hook uninstall. A startup failure explicitly names the unavailable features.

Earlier installation must be checked with the entire package: allocating the
hooks earlier is not proof of sufficient capacity on every pack or machine.
Inspect all plugin startup failures as well as QOL, then test reticle, R3 saved
settings, leaving/rejoining, and native aim when QOL aim is disabled.

## Full-pack beta.3 result and beta.4 allocation revision

On the same exact Core/game/Maps artifacts, beta.3 was loaded by both the local
TCP/IP host and the separately connected client. The user confirmed the aim
reticle, R3 skill toggles, Chipped Topaz withdrawal to Inventory and Cube with
correct one-item stock decrements, and single identification with exactly one
tome charge consumed. Logs independently observed the destination increments
and identify charge change 93 -> 92. This does not establish persistence or
actual remote-host latency/save behavior.

The full-pack startup check found new Maps allocation failures at game
`+0x4C2410` and `+0x49E470`. Read-only client inspection found both entries
pristine, both QOL aim entries installed, and the Core pool again full at 63/63.
The separately sampled TCP/IP host had both Maps entries hooked; these two
failures were observed in the client rather than the server.
Earlier aim installation alone therefore did not satisfy the coexistence gate.
The candidate was not published.

QOL's existing six small CALL relay allocations all searched upward inside
1 GiB of their call sites. The live contact relay was at an owned 4 KiB
allocation, and a large private reservation occupied the address space beyond
the last nearby relay. Reservation ownership beyond the queried allocation is
not attributed to a particular plugin. Dynamic addresses are diagnostic only.

Beta.4 changes allocation placement only. `relay_allocation.h` first searches
1.25..1.75 GiB above, then below each call's next-instruction address; these
ranges are outside the observed Core +/-1 GiB search and inside signed rel32
reach. It falls back to the remaining reachable user address range. VirtualQuery
skips occupied reservations, every attempted start is allocation-granularity
aligned, arithmetic is bounded, and the OS must return the exact requested
address. A reservation race advances to the next candidate rather than looping.

The existing pickup, placard, client update/run, contact and glyph CALL patches
retain their sites, guards, wrappers, FF25 relay instructions, argument/return
behavior, RX sealing, SDK publication and lifetime rules. Every actual CALL
displacement is still checked before publication. No Core allocator/pool state,
other plugin, or native entry trampoline is modified. This reduces address
competition but cannot guarantee capacity for arbitrary future plugin packs.
Every future pack still requires startup and actual allocation-headroom checks.

### Beta.4 local TCP/IP validation

After verified process closure, only QOL was replaced in the active Reimagined
installation, with beta.3 and current logs/configs backed up. The installation
inventory confirmed all other audited plugin/config hashes unchanged during the
swap. Both new processes loaded the exact beta.4 hash above and unchanged Core
and Maps artifacts. Startup and first-lobby join completed. No hook-allocation
errors appeared; unrelated existing pack resource/table diagnostics remain.

Read-only inspection at 18:45 Eastern confirmed QOL ownership of all four aim
relays at the documented Lookup, UnitTest, PointScore and Selected entries in
both processes. Both previously failing game entries `+0x4C2410` and `+0x49E470`
led through relays into the loaded Maps image in both processes. The client now
had two eligible Core pools using 63 and 4 slots, leaving 59 free slots, versus
zero in beta.3. The server used 58 of 63 slots. QOL's contact CALLs led to its own
farther relay, preserving the previously documented contact-wrapper ownership.

The user confirmed successful join, visible aim reticle and R3 toggles. The log
observed a Chipped Topaz Inventory increase 0 -> 1 and the user reconfirmed gems
working. No stocked rune was available, so live rune coverage remains open.
Beta.3's Inventory/Cube gem counter and single-identify charge checks remain
separate evidence; beta.4's new allocation placement has not established broader
item behavior, skill-setting persistence or actual remote-host/save behavior.
Beta.3 was not published. Beta.4 is the tested candidate selected for this release.

An 18:50 Eastern read-only inventory checked every source-declared QOL SDK
entry-hook site in the same beta.4 client. It found 21 entry relays owned by QOL:
five aim hooks (including the enabled optional cast observer), one client loot,
two native input, one Guided Arrow, one tooltip input query, four interaction
priority, one glyph widget and six navigation hooks. The native menu entry
fallback was unused because the guarded route-aware pointer patch was active.
This counts direct SDK entry hooks, not every QOL listener or byte/pointer patch.

The shared pools still used 67 of 126 slots, leaving 59 free. Beta.3 used all
63 slots in its single pool. Beta.4 did not remove 59 QOL hooks: moving six
separate CALL relay reservations allowed a second 63-slot Core pool to exist,
of which four slots were used. QOL's inline-hook count was unchanged by beta.4.
The other occupied slots were not individually attributed in this inventory.

## Remaining compatibility work

- The rev.79-beta.2 sender contract already tolerates unrelated Global Chat
  changes while validating the complete non-chat forwarding path. Preserve it.
- Maps quantity-reader coexistence was separately reviewed and hardened in
  this candidate; see [the restricted item-quantity proof](ITEM-QUANTITY-COEXISTENCE.md).
  Its proof is limited to QOL's stat-70/layer-0 calls, independently of Chat.
- Inventory/belt/vendor helpers, ground-pickup hooks, input hooks, and native
  provider slots have their own ownership/ABI checks. Keep an inventory of
  overlaps and test the actual pack; do not admit arbitrary detours or bypass
  companion hooks to avoid refusal.
- Exercise failed joins, disconnect during pending work, reconnect, delayed
  destination updates, empty/full storage, tab/season changes and both host
  roles. Submitted-but-unconfirmed requests must never automatically retry.
- Keep the host/client shared-plugin fingerprint requirement intact. Matching
  file/version/config inventories belong in the test setup, not a bypass.
- Prefer future supported SDK counter-focus/remote-transaction and hook-capacity
  facilities when available; current private witnesses remain build-specific.

## Validation status

Production counter-focus/mailbox fixtures cover zero-ID displays without any SDK
tooltip event, all three advanced categories and Inventory/Cube destinations,
identity/focus/context changes, full/empty/unavailable states, age, lifecycle
cancellation, confirmation and no-retry timeouts. Aim fixtures inject failure at
each required hook and exercise repeated lifecycle events and optional observer
refusal. There are 115 production counter-focus checks. Beta.3 passed all 35 QOL
suites and 16 packaging checks. Beta.4 passes all 36 QOL suites, including 27
allocation checks covering address bounds, occupied regions, races, invalid
queries, RX sealing and execution of a real allocation, plus 16 packaging checks.
The qualified beta.4 live results and beta.3's release-blocking failures are above.
