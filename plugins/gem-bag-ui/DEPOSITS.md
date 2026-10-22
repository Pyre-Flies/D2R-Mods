# Gem Bag deposits (beta.17)

Shift + right-click a stored bag deposits eligible gems and clusters from that
bag's current storage page. Plain right-click and LB+B retain the withdrawal
picker. This first candidate adds no picker button. Drag-to-bag was cancelled by the user; Shift + right-click is the deposit input.

## Recipe evidence

The engine/provider admission in [NATIVE-CONTRACT.md](NATIVE-CONTRACT.md) remains
unchanged. No additional native entry point or structure offset is introduced.
The October 21 normal-package compiled capture under the ignored Gem Bag scratch
directory supplies complete CubeMain rows 1008 (cluster) and 1107 (chipped gem),
now retained in `tests/recorded_deposits.h`. Corresponding installed TXT rows are
944 and 1043; TXT row numbers cannot be used as compiled indices. The compiled
row itself, including its output index, is the discovery authority.

The two rows use operation 0, unrestricted difficulty FF, two inputs, expansion
version 100, exact bag input flag 1, and either exact item flag 1 or item-type
flag 2 for input two. The second input's quantity byte at +0x25 is 1. Output A
at +0x5A is usetype/mod (flag WORD +4 = 1, kind byte +13 = FF), preserves bag
quality, and has one certain flat credit property at +0x77. Its signed WORD
minimum/maximum are +0x7D/+0x7F; chance +0x81 is 100. Outputs B/C are absent;
all unused property IDs are FFFFFFFF. Every other byte must match the bounded
zero/default shape, not merely the fields mentioned here.

The observed cluster input is class 793/code `1gc`, credit range 20..30; the
chipped input type is 92/code `gem0`, credit range 1..1. Those identities are
observations, not hard-coded admission requirements. Bounded ItemTypes ancestry
resolves type recipes into concrete item classes. Exact item recipes also allow
clusters that have no Gems row. Captured Lod/Rotw tables each discover 44 input
classes. Unsupported restrictions, extra effects and overlapping input rules
disable deposits for that bag while preserving its withdrawal definitions.
Selected input/recipe rows join exact local witnesses and the cross-peer
fingerprint. Channel token advances to v3; request/reply sizes stay 72/224.
Both peers require beta.17. Localized name references retain beta.16's exclusion
from cross-peer comparison only.

## Authority and conservation

The gesture captures a fresh bag hover with the existing 400ms/four-pixel rule,
queues UI work, and consumes its matched right-button release. Control, Alt and
other mouse buttons do not activate deposits. It does not open a native panel.
The client inspects the bag, validates the host definition, then sends Deposit
with the resulting lease. Only bag location and action are sent; no client item
list, quantity, random value or credit amount authorizes consumption.

The host resolves the sending peer in the existing authoritative callback,
requires an empty cursor and unchanged leased bag, then enumerates only that
storage container. Inventory page and shared-stash page must also match. Normal,
unsocketed items without affixes/ethereal state are eligible; equipped, cursor,
trade, other pages and the bag itself are excluded. Enumeration is bounded at
256 entries and total consumption at 4096 logical units per request. A stack's
whole quantity is deposited or left in place. Full grids are allowed.

Before sampling, maximum per-unit credit times stack quantity must fit below
65535. Oversized stacks remain untouched, avoiding capped/lost credits and
rerolls near capacity. Each cluster unit receives an independent uniform roll
in its one-unit recipe range using server BCryptGenRandom with rejection
sampling. This reproduces repeated one-unit recipe ranges, not the distribution
or PRNG seed sequence of a multi-cluster Cube recipe. Random-source failure
stops before that stack changes. Processing order is storage row then column.

Each stack uses a compensating operation, not a new SDK atomic transaction:
qualify the existing credit contribution and publication path; add its sampled
credits; destroy the entire owned stack through public ItemService; require the
input handle to be absent and the bag total to agree; publish the bag. A clean
destruction refusal refunds only after confirming the input identity/quantity
is unchanged. CallbackFault, uncertain setter/removal, failed compensation or
post-consumption verification/publication failure halt this owner. There is no
automatic retry or guessed item recreation. Earlier successful stacks remain
deposited if a later stack fails. Session/sequence caching covers deposits as
mutations, including dropped replies and dedicated-host frame routing.

CaptureJob intentionally clears the contribution pointer before returning from
the synchronous native callback. Preflight uses its source index/fingerprint,
never that cleared pointer. The setter reacquires and qualifies the source in
its own callback. Bags without a provable existing contribution refuse deposits;
this candidate does not fabricate a stat list for a fresh empty bag.

## Evidence and remaining checks

Twelve automated suites cover existing withdrawals, copied real deposit rows,
actual normal-package replay, native deposit execution with owned SDK/setter/
publisher substitutes, full grids, cap skip, clean refusal refund, uncertain
failure, shared-page exclusion, offline/hosted RPC routing, duplicate deposit
requests and moved/replaced-bag refusal. UI lifecycle tests cover an inspection
followed by one deposit without opening a panel and mismatch cancellation.

These checks do not prove actual input delivery, native destruction/publication,
network visibility, empty-bag insertion or save persistence. Before accepting
the candidate, capture a bag plus one eligible gem, Shift + right-click once,
and compare client/server credit, item removal, bag identity and unrelated
items. Repeat with a cluster and check its 20..30 range in this pack. Separately
check hosted mode, other storage pages, ordinary withdrawals and save/rejoin.

On October 21, beta.17 was verified loaded from the mod-local folder in PID48548
(hash 9ab52ff3150ecba4b8ba1b09089de5597380f5215fdf5894d5d085e06bff665c).
One offline Shift+right-click consumed Chaos Onyx168 from normal inventory and
raised bag167 from1491 to1492 on client and local server. The bag stayed at
(1,5); Cube gems164/165 and every other sampled item record were unchanged.
The user confirmed visible success. Sequential snapshots and the runtime log
are retained in ignored beta17-onyx-deposit-validation.json. This is not a full
item-stat or persistence audit; clusters and separate-host deposits remain pending.

A second controlled offline test consumed cluster203/code1gc, quantity1 in the
host log, and raised the same bag from1492 to1513 (+21, within20..30) on client
and local server. The bag stayed at its new baseline cell(2,5); all other
sampled records, including both Cube gems, were unchanged. The user reported
1513. Evidence: ignored beta17-cluster-deposit-validation.json. This single
roll does not establish the random distribution or persistence.

## Shared stash investigation (beta.18 diagnostics)

The user confirmed offline persistence at 1513 and hosted withdrawals/deposits.
The October 21 12:41 hosted log records a Skull withdrawal into Shared Stash
(1515 to 1514), followed by repeated deposit refusals at 1514. Beta.17's generic
refusal message did not distinguish failed credit writes from compensated item
removal refusals. Do not infer the SDK destruction result from that text.

Beta.18 records the failing stage, SDK destroy result, item/bag IDs, storage
pages, quantity, attempted credit and confirmed compensation/halt state. A
result of Unavailable before the consume stage is the uncalled default, not an
observed SDK refusal. No native contract, authority, mutation ordering or guard
changes are included. This diagnostic candidate is not a shared-stash fix.

## Shared-stash repair candidate (beta.19)

Live beta.18 hosted attempts at 12:54:13..15 confirmed stage=consume,
sdkDestroyResult=2 (Unsupported), sharedPage=0, quantity=1, creditsBefore=1514,
attemptedCredit=1, restored=1 and halted=0. Both processes loaded the admitted
beta.18 hash. Evidence: ignored beta18-shared-refusal.log and client baseline.
The first earlier click had no hover; subsequent attempts reached the host.

Beta.19 uses the public existing-item Move transaction, as Controller QOL does,
to stage each shared input into normal inventory with automatic placement. The
loader checks ownership, active writable shared page and available space. The
item's handle, identity, quantity, quality and flags are checked after moving.
Only then is the public destroy call made. No native hooks or offsets change.

A clean destroy refusal restores the same item to its exact shared page/cell
before refunding credits. CallbackFault, changed identity, unexpected commit
count, uncertain move or failed restoration halts the owner without blindly
refunding. Successful destruction followed by failed publication still halts.
There is no second consumption route or automatic retry. This remains a
compensating sequence, not one SDK atomic transaction.

Shared deposits now require free normal-inventory space for one whole input
stack at a time. A full inventory safely refuses; the shared page itself may
be full. Other containers retain direct consumption. Tests exercise staging,
no-space refusal, exact restoration, restoration failure and uncertain move
using owned SDK substitutes. Hosted visibility/persistence remain to be tested.

### Beta.19 hosted observation

October 21: both processes loaded beta.19 hash
a5889352f335119f9bdd8b9387271fbe34230bdcbc574d82fcb7b21a24a73a5c.
The host logged Skull1 consumption at 13:03:23, 1514 to1515, an empty repeat
without credit change, Ruby withdrawal201 at 13:03:29 (1515 to1514), and
Ruby201 consumption at 13:03:31 (1514 to1515). Client memory before/after
shows only original Skull1 absent, bag2 unmoved at (8,6), 1515 credits and
all other sampled item records identical. The user confirmed visible success.
Evidence: ignored beta19-shared-validation.json and beta19-shared-success.log.
This is client memory plus host logs, not a separate server-memory comparison
or a single-action snapshot interval. Hosted save/rejoin, cluster staging and
full-inventory refusal remain live-test gaps.

The user subsequently confirmed the requested hosted save/leave/rejoin check
with the shared bag expected at1515 (response: "Looks good"). Persistence is
user-confirmed, not an independent post-rejoin memory capture. Full-inventory
refusal and shared cluster staging remain automated-only coverage.
