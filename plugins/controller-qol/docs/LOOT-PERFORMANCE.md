# Ground-loot performance

## Rev.91: synchronous label-unit reuse and held-slot `CannotHideLabel` preflight

In rev.89/rev.90, two remaining QOL-side hot paths still performed avoidable native and broker work during label rendering and `LB` hold refreshes:

1. **Duplicate `0x9A5D0` unit lookups in `Track -> HiddenForPickup`:** When `QolLootVisibility::CannotHideLabel(context, labelUnit, guid)` returned `false` (potentially hidden equipment), `Track` called `HiddenForPickup(guid)` without forwarding `labelUnit`. `HiddenForPickup` then called `ReadPickupCode` (`0x9A5D0` lookup #1) and `HiddenCandidate` (`0x9A5D0` lookup #2) for the same unit already passed into `ObserveCompletedName`. Rev.91 retains `CurrentLabelGuid`/`CurrentLabelUnit` in `thread_local` scope across the synchronous fallback call so `ReadPickupCode` and `HiddenCandidate` reuse the verified `labelUnit` without changing `HiddenForPickup(uint32_t)`'s signature or running `CannotHideLabel` twice.
2. **Held-slot & candidate-read `CannotHideLabel` preflight (`Read` & `HiddenUnit`):** During `LB` holds, `Refresh` (10 Hz across up to 7 slots) and initial `Read` previously called `QolLootVisibility::Hidden` unconditionally, running full socket/property evaluations on every held slot even for runes, gems, and charms that `CannotHideLabel` can prove non-hideable in ~2 us. Rev.91 checks `CannotHideLabel` first in `Read` and `HiddenUnit` (when not already checked on the synchronous label path) and records `LabelFastPath` skips.
3. **Next-release backlog note:** Default `ground_pickup_distance` (`10` minimum floor in `RefreshClientGroundTask`/`QueueGroundPickup`, `6` in default TOML) is planned to be raised to **`14`** in the next Controller QOL release (`rev.92+`).

Live in-game verification (`1.3.1+rev.91` + UnHoarder `1.2.16`, 20.8s window with 12 ground items and 5 held `LB` slots): `label-visibility-preflight` averaged `0.002074 ms` (`37,042` calls, `19,414` skipped via `CannotHideLabel`), `completed-label` averaged `0.037152 ms` at `1,751.09 calls/sec`, `held-refresh` averaged `0.105003 ms` (`0.892 ms/sec`), and `fallbackSinceLoad` remained `0`.

## Rev.89: paired socket observations and keyboard pickup

UnHoarder 1.2.16 reduces socket guard cost for its own labels and the visibility
queries requested by QOL. QOL adds exact item-code and pickup-dispatch
compatibility; its controller policy and per-label admissions are unchanged.
See [the paired-socket/ALT contract](SOCKET-BATCH-ALT-COMPATIBILITY.md) for
artifact identity, all retained guards and validation limits. Both perf commands remain.

## Rev.88: fresh resident-page queries

The new independent page-query path reduces per-label compatibility and native
property guard cost without caching successful admission. The original reader
remains the fallback. See [current-page validation](CURRENT-PAGE-VALIDATION.md)
for API predicates, mutation coverage, benchmark limits and the exact 1.2.15
companion contract. Both perf commands remain; live acceptance is pending.

## Rev.86 label and compatibility work

The latest mixed-loot interval ending 2026-10-21 20:09:42 EDT exposes the limit
of rev.85's code-only preflight: 25% of labels skip the full query, versus 100%
in the gem sample. QOL averages 0.254705 ms per label at 808.52 calls/s; full
visibility runs 606.39 times/s and compatibility consumes 79.531 elapsed ms/s.
UnHoarder properties consume 308.764 elapsed ms/s, including socket observations
at 140.775 ms/s. Inventory polling is 0.831 ms/s. Do not add these nested scopes
or interpret them as frame times. The same filter includes equipment rules for
quality, sockets and ethereal state; these still require native observations.

Rev.86 combines the three usual PE header reads into one fresh 4 KiB API
snapshot per item-code compatibility check. All header/section validation
remains, and uncommon layouts or unavailable optional tails retain the narrow
reads. Metadata is invocation-local; no successful admission is cached. All
live graph bytes, sections, current page protections, ownership and continuation
checks remain. Other users of ImageMemory retain the default original mode.
No client-loot, native pickup, proof lifetime, provider query or reticle logic
changes in this revision.

UnHoarder 1.2.13 also batches PE metadata for every Maps socket guard, retaining
both stat observations and all four before/after guards. Quality reads retain
both 24-byte unit and 64-byte data observations; the data copies now use fresh
full-span page qualification plus SEH copying at each observation. Identity,
field range/equality and requested flag comparisons remain. Ordinary labels
skip minimap position work after resolving no icon; icon-bearing paths retain
the four reads and all identity/path/coordinate checks. No mutable properties
are reused across callbacks. Existing native fields remain build-specific.

New exact companion: UnHoarder **1.2.13**, SHA256
`9412c5f6175918b5b11ba7de631d4f795ee87cde37cab9b9b5fd35328aac32f8`;
image `0xE9000`, wrapper RVA `0x1F530`, original slot `0xB6A18`.
Release link-map symbols and artifact bytes establish the same forwarding ABI:
`48 83 EC 28 48 8B 05 DD 74 09 00 FF D0 90 48 83 C4 28 C3`.
The additional exact validator retains all eleven older profiles. No new game
RVA or hook is introduced.

Red/green fixtures verify 7,200 to zero unused position reads for 1,800 ordinary
labels, three to one header API reads, fresh mutation/protection refusals, and
unchanged icon/quantity/reload behavior. An owned-memory benchmark measured
quality median 2.373 to 1.925 us and headers 1.384 to 1.014 us (9 x 5,000 calls).
Those are isolated routine savings, not FPS forecasts. The combined callback
fixture retains 5,416 checks. Both perf commands remain; UnHoarder additionally
measures native quality, minimap position, and no-icon position skips. Mixed-loot
in-game acceptance of rev.86/1.2.13 is pending.


## Rev.85 shared-label preflight

The user isolated a later regression to **both** plugins enabled: 15 town gems
cost about 35 FPS when labels were shown, while either plugin alone did not.
This is a stronger combined-path reproducer than the earlier inventory-poll
hypothesis. The old live reports show costly nested QOL label checks, but do
not independently establish how much of the total FPS loss they explain.

The shared path is `ObserveCompletedName -> QolClientLoot::Track ->
HiddenForPickup -> Validate/ReadPickupCode/HiddenCandidate ->
unhoarder-visibility.query -> GroundRuleItem`. It runs even in keyboard/mouse
mode. `Track` must maintain the existing same-session, 250 ms Hidden proof:
`HookPickup` uses it to prevent filtered items being taken with controller A.
Deleting that refresh or caching successful native admission is not safe.

Rev.85 passes the already-borrowed label unit to a separate leased service,
`unhoarder-label-visibility` v1, owned by provider `unhoarder`. Its standard-layout
16-byte table contains uint32 size/version at offsets 0/4 and one cdecl noexcept
callback at offset 8: `Decision(const void* unit, uint32_t guid)`. The uint32
enum is `RequireQuery=0`, `CannotHide=1`. The callback and unit are borrowed only
for the synchronous call; no table or native pointer is retained. Acquire and
release use the existing SDK broker lifetime. Publication/unload follows the
existing provider service lifecycle. Either load order and an older provider
work: the service is acquired afresh and absence falls back to the full query.

The new service can only prove **no Hide rule is possible**. It cannot create a
Hidden proof or authorize an action. UnHoarder re-reads the 16-byte native header
(type/class/id/mode), requires item type 4, supplied id and ground mode 3, reads
the current canonical native code, and evaluates all Hide rules with other
properties unknown. Type/base selectors are already expanded to code sets by
the existing parser. Any Match or Unknown keeps the full query required;
rule order and Continue never justify an early skip. A positive proof rechecks
code, complete header, rule snapshot and presentation arming. The original
inactive-Hide/geometry-off Visible outcome also clears the proof. No mutable
property, successful admission, page information or rule result is cached.

`CannotHide` only erases an old QOL Hidden proof and updates the same label
observation timestamp. Every other result (including unknown enum, missing or
invalid service and missing unit) runs the original `HiddenForPickup` unchanged.
Native A-button enforcement, full service v1, Submit, session/age/epoch fences,
local-actor and one-use permits, gold exemptions and all action guards remain.
Rendering, rule output, sounds, polling cadence and reticle behavior are unchanged.
The user has now confirmed the reticle works; the earlier gate-2 diagnostic was
captured in keyboard/mouse mode and is no longer an unresolved reticle defect.

This reduces work for gems and other codes excluded by all active Hide rules.
Potentially hidden equipment and generic property-only Hide rules still take
the full path (plus a small preflight); no universal FPS improvement is claimed.
The original full validator remains mandatory whenever it is needed.

### Exact paired artifact

UnHoarder **1.2.12**, SHA256
`2c8aa235f55990250fcf9da03e611591fad1485286494a6a2e90683a4c9f6f29`;
image size `0xE8000`, forwarding wrapper RVA `0x1F4E0`, original slot `0xB5938`.
Release link-map symbols and DLL bytes establish the same one-argument,
unchanged-return forwarding contract:

```text
48 83 EC 28 48 8B 05 4D 64 09 00 FF D0 90 48 83 C4 28 C3
```

[The additional validator](../src/item_code_label_preflight_contract.h) retains
the full game `0x36EF50` entry/tail, relay, wrapper, original slot, trampoline,
continuation and page/ownership checks. Only reviewed coordinates/bytes differ;
all ten older exact artifact profiles remain. There are no new game RVAs or hooks.

### Evidence and monitoring

The extracted production Track regression failed before the optimization:
1,800 fallback queries for 15 labels over 120 iterations. The green test skips
all 1,800, exercises leased-provider refusal and keeps proof/session behavior.
The independent provider test covers 7,680 completed-rule parity cases and
changed identity, code, snapshot and arming. An owned combined fixture compiles
both actual callbacks through the SDK lease helper: 1,800 legacy queries versus
zero for no-Hide gems, with full-query fallbacks on reload, identity mutation,
provider loss and potentially hidden equipment. Native full-query execution is
a spy in that fixture; these counts are not measured game frames or FPS.

`qol perf` adds `label-visibility-preflight` and `label-no-hide-query-skipped`;
`unhoarder perf` adds `label-visibility-preflight` and `label-no-hide-proven`.
The former timings include nested work. Skipped/proven are event counts with
zero-duration records. Only active Rules proofs increment UnHoarder's proven
counter; QOL's skip count also includes inactive filtering. Existing counters
and command interval semantics are unchanged. Prime both reports, show the
same 15 gems for 30 seconds, report both again and record actual FPS separately.
The no-Hide skip count should track gem-label observations, while full
`visibility-service` and compatibility counts should fall sharply when idle.

Build, artifact and automated checks are recorded with the local candidate.
Live combined FPS recovery, controller hidden pickup, visible styling and
filter/session transitions still require acceptance with this exact pair.

## Rev.84 active-filter follow-up

A later live capture with UnHoarder 1.2.10/rev.83 and an active 13-rule filter
reported about 40 FPS. Its approximately 115-second window measured inventory
polling at 0.773 elapsed ms/sec; UnHoarder name, identity and visibility at
205.834, 153.746 and 164.798; QOL completed-label at 259.085, including its
compatibility and provider visibility work. These are nested elapsed scopes,
not additive CPU samples or an isolated cause of the entire FPS loss.

Rev.84 opts into fault-contained local copies only **after** the existing
`ImageMemory::Read` region loop has checked the complete current span, memory
kind, protection, image section, allocation owner and bounds. Opening the live
PE still uses the original three `ReadProcessMemory` calls on every admission.
No successful live guard, PE header or page result is cached. All byte, relay,
wrapper, original slot, trampoline and continuation predicates are unchanged.
Other users of `ImageMemory` retain their default copy path.

The distinction follows Windows' documented
[ReadProcessMemory range-read contract](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory)
and [page protections](https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants).
Unqualified header reads retain that API. Only a reader that has already
performed its explicit, complete page/ownership proof uses the local copy.

The exact additional UnHoarder 1.2.11 artifact is SHA256
`bc98e70a31c7765b9e25219857aed623eca507a27da3a95cbafb0241a9ce723c`.
Image size is `0xE7000`; its one-argument forwarding wrapper is at `0x1EC30`,
original pointer slot `0xB48B8`, with the full 19-byte witness:

```text
48 83 EC 28 48 8B 05 7D 5C 09 00 FF D0 90 48 83 C4 28 C3
```

[The new contract](../src/item_code_native_reads_contract.h) keeps the prior
complete validator with only these reviewed coordinates/bytes changed. All
nine preceding exact artifact identities remain. Actual-DLL tests mutate every
wrapper/trampoline byte, clear the original slot, change code protection,
remove slot readability, guard the slot page and corrupt live PE metadata
after warming disk identity. Every unsupported state is refused; the page
guard is not touched. Restoring the graph restores admission.

An owned fixture measured the full matching QOL guard at 26.456 us before and
23.114 us after, with 3 rather than 8 cross-process API reads and the same 5
live page queries. This is an isolated measurement, not a game FPS prediction.
All 69 CTest cases pass, including ten actual companion artifacts; the existing
skill-discovery file test used normal Windows temporary-folder access.

`qol perf` retains its existing interval metrics and adds `QOL_AIM_STATE`, a
read-only snapshot of installed/enabled/session state, overlay callbacks, last
UI tick age, stick values, projection state and ground draw attempts. It does
not invoke native game calls or change aim settings. `viewStage` identifies the
last preview gate: 1 global/session/foreground, 2 controller context, 3 player,
4 aim object, 5 disabled active skill, 6 no eligible preview skill, 7 world
state, 8 stick input, 9 manual activation, 10 complete, 11 caught native fault.
`frames` is cumulative, not an FPS figure; `drawSeen=0` means no normal ground
draw attempt has been recorded. Draw attempts do not prove visible pixels.

At the time of the rev.84 build, the missing idle reticle was not reproduced. Settings
enable aim and overlay; the new snapshot is intended to distinguish absent
callbacks, an idle-skill gate, F8/session state and projection failure without
changing those guards. The new pair requires live performance/behavior testing.

Rev.83 preserves the native entry, page, byte, pointer, trampoline and return
proofs. It changes which known profile is tried and avoids computing an
assignment that the existing hold policy discards. It introduces no new
settings or native action path. The pre-existing rev.82 reload work is retained.

## Exact artifact selection

[item_code_compatibility.h](../src/item_code_compatibility.h) hashes the loaded
UnHoarder file once per observed module identity and classifies that digest
against the same eight historical hashes, plus the exact 1.2.10 candidate.
Unknown or unreadable files remain refused. Classification does not cache
positive live admission or image metadata.

Every call opens the live PE image and executes the complete matching
validator. Expected native entry/tail bytes, relay target and opcode, full
wrapper, original slot, page protections, image ownership, trampoline and
native continuation are still checked on every use. A cached file identity
cannot admit a changed live graph. Existing Check/Match fail-open behavior
and native/SDK diagnostic fallback are retained. Other users of the historical
artifact predicates retain their same exact-hash answers.

The former dispatcher ran historical live validators before reaching 1.2.9.
The owned 1.2.9 fixture measured 108.078 us per current-dispatch call before
the change, versus 25.536 us afterward including monitoring on the same host, approximately 76%
less time. ReadProcessMemory calls fell from 25 to 8 and VirtualQuery calls
from 22 to 5. The full current live proof remains; disk classification alone
never returns compatibility. These are isolated guard costs, not game FPS.

## New build-specific record

The locally built UnHoarder 1.2.10 payload is admitted only by SHA256
`3e5f3ed2c5819035ab376dff60b1597a70a0710447370aa30dab3e5aa2262e88`.
Its image size is `0xE7000`. The Release link map identifies the unchanged
one-argument forwarding wrapper at DLL RVA `0x1EA40`, original slot `0xB47B8`:

```text
48 83 EC 28 48 8B 05 6D 5D 09 00 FF D0 90 48 83 C4 28 C3
```

The pointer operand resolves to that slot; its indirect call forwards the
original item pointer once and returns the original value. The game entry
is still `0x36EF50`, with the existing exact native original/tail witness.
[The new contract](../src/item_code_polling_contract.h) retains the complete
prior live graph validator with only the reviewed wrapper RVA/bytes updated.
No arbitrary jump, extra helper, new provider ABI or relaxed ownership check
is accepted. Existing eight profiles remain independently available.

## Frozen hold refresh

[client_loot.cpp](../src/client_loot.cpp) gathers, checks and sorts candidates
on the first refresh of a new hold. Subsequent refreshes retain the original
seven identities instead of rebuilding a replacement set. They continue
pruning expired observations, so the bounded observed-item map retains room
for new labels, and perform live hidden checks on every assigned target.
New drops never fill held holes. Release/repress still starts a new snapshot,
including an initially empty hold.

`Submit` is unchanged: before a native action it revalidates the session,
hold/request epoch, age, native profile, assigned identity, observation
freshness, visibility, distance/collision, approach/update graph and repeat
permissions. Skipping a discarded candidate set does not skip action admission.

The owned production-refresh fixture records 100 native candidate reads on
the first hold, zero on the next steady refresh, and all seven assigned-target
hidden checks. It also exercises expiry, changing priorities, new hidden items,
holes, release/repress, invalid sessions and empty holds. Its redundant-read
assertion failed against the original function.

## Performance reports

Enter `qol perf` in the loader console (or `/qol perf` through its chat route).
The same bounded report is written to the plugin log as `QOL_PERF`. It shows
completed call counts, calls/second, total and mean elapsed milliseconds,
elapsed milliseconds/second, and the maximum call since plugin load. Counts
and totals cover the interval since the previous report, or since load for
the first report. Prime a report before each 30-60 second comparison and
request another afterward. A report does not change settings, rules or input.

Metrics cover item-code compatibility, completed-label processing, hold refresh
and the visibility service including provider acquisition. Hold refresh also
counts attempted native candidate reads. The first hold may have many reads;
steady holds should add none while visibility calls continue. These timings
include called functions and lock waits and overlap: do not add them together.
They are elapsed scope costs, not CPU samples or frame-time measurements.
Concurrent completions at a report boundary can land in adjacent intervals.
There is no per-item logging, allocation, new polling or new native hook.

Matching UnHoarder's `unhoarder perf` report adds current feature demand,
scheduler outcomes, actual inventory enumerations and carried-item counts,
ground label/rule/identity/visibility work and existing minimap rendering.
The rendering metric excludes the original Present call and its VSync wait.
An empty scoped timer measured about 0.05 us per completion on this host in
an isolated aggregation fixture; live monitoring overhead remains unmeasured.

For FPS comparisons retain the same graphics settings, frame cap, plugin set,
filter and scene. Use a frame-time overlay for FPS and stutter evidence, save
both performance reports and note whether labels and the pickup modifier were
held. Test UnHoarder plus Charm Inventory first, then the matching QOL pair.
No-filter/text-only configurations should report zero actual inventory scans.
Sound, icon or price features retain the original 150 ms observation cadence.
Filter watcher filesystem checks are unchanged and are not inventory scans.

## Validation and limitations

All 67 QOL CTest cases pass, including nine actual companion fixtures. The
existing skill-discovery file-migration test required normal Windows temporary
file access after sandbox fixture creation/cleanup failed; its unchanged test
passes with that access. Other 66 cases passed in the sandbox.

The 1.2.9 and 1.2.10 actual-DLL tests count the full live reads and reject
each changed wrapper byte, a cleared original slot and every changed
trampoline/continuation byte even after warming artifact identity. Historical
byte/ABI tests and the nine actual artifacts remain covered. The full suite
also covers item routing, lifecycle, configuration reload and aim behavior.

No game files, live configuration or signed Ladder bundle were changed.
The new DLL pair is a local candidate. FPS improvement, dense-map behavior,
audible/visible results, hosted play and persistence need their own game
acceptance evidence. Functional tests and isolated guard timings do not
establish the cause or resolution of the reported 120 to 30 FPS regression.

## rev.87 / UnHoarder 1.2.14: optional formatter observation

UnHoarder can use the fully guarded result of QOL's existing visibility request
for the same formatter call's presentation identity. QOL `Track`,
`HiddenForPickup`, native profile/code/identity checks, service lease, cached
proof lifetime and action submission are unchanged. The only runtime QOL change
is a new exact companion profile, alongside all twelve older profiles. Both
plugins remain independently usable; no hook-chain or new service is assumed.

The combined actual-callback fixture retains 1,800 QOL admissions/native-code
calls while reducing the overlap from 3,600 to 1,800 property/socket observations.
The 3,840 old/new rule cases have identical results. Native QOL entry execution
and socket getters are owned fixture spies; this does not measure game FPS.

The owning independent fork records the full operation lifetime and exclusions in
`docs/formatter-observation.md`. The new exact artifact's hash, forwarding RVAs,
wrapper bytes and image layout are recorded in `item_code_formatter_observation_contract.h`
and the candidate validation manifest. No prior profile is changed. Keep
`qol perf` and compare with the new UnHoarder formatter counters in a fresh
mixed-loot interval; live acceptance remains required.

Exact candidate UnHoarder 1.2.14: SHA256 `2ea089cc7e698285d374bea859829053ec1c1ce3ed69e7b130c930b0df01066f`,
image size `0xea000`, HookGetItemCode `0x1f9c0`,
OriginalGetItemCode slot `0xb7ab8`, wrapper
`48 83 ec 28 48 8b 05 ed 80 09 00 ff d0 90 48 83 c4 28 c3`. The existing full one-argument, unchanged-return
forwarding proof and all live section/page/ownership/continuation checks remain.
