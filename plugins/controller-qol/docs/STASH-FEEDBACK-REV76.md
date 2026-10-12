# Controller Deposit All stash feedback - rev.76-beta.1

## Behavior and scope

`1.3.1+rev.76-beta.1` is a local test candidate. Controller Deposit All uses the
same supplemental tab notification handling as Inventory Deposit beta.13:
Gems, Materials, and Runes each start their existing animation once per batch
after a submitted source item is observed leaving inventory. Six different
materials still trigger one Materials animation. Different destination tabs
have independent timers and may animate together. A later batch may start each
animation again. Personal and Shared receive no supplemental notification.

The first confirmation starts the tab's timer. Further confirmations for that
destination neither restart nor extend it, even after its timer expires. The
existing animation's configured fade duration and repetition count are kept;
"once per batch" does not mean replacing it with one brighten-and-fade cycle.
Refused, skipped, or unconfirmed submissions produce no extra feedback. Source
removal is the existing batch verifier's observation, not an independent check
of destination counters or save persistence.

There are no transfer, eligibility, shortcut, configuration, or shipped-default
changes. In particular, the reference `prioritize_chests` default remains
`false`. This candidate does not include unrelated local default edits.

## Integration and ownership

`QolBulkStashFeedback` connects the existing `QolBulkStash::Observer` begin,
removed-item, and end callbacks to the Inventory Deposit feedback module.
Both authoritative local verification and remote source-removal verification
already feed this observer. Only copied item identities and class IDs enter
the feedback mailbox; no native item or widget pointer crosses threads.

Controller QOL compiles the existing
`plugins/inventory-deposit/src/stash_feedback.cpp` implementation and its
policy/profile headers into its own DLL. It does not load or depend on the
Inventory Deposit plugin at runtime, and does not include that plugin's button,
artwork, sprite resources, or gold-anchor placement behavior. The shared
implementation's existing child-lookup byte witness is reused as a native
profile dependency, not as a button placement operation.

Initialization admits feedback separately from bulk deposit. An unsupported
profile or missing feedback service does not disable the transfer feature.
`QolBulkStashFeedback::Refresh` runs from the existing 33 ms polling loop and
queues bounded UI work through `ThreadService::runOnUiThread`. Native panel
lookups, classification, visibility, and alpha writes remain in that UI
callback. The feedback module deduplicates queued work and schedules only while
confirmations, timers, or cleanup remain pending. It adds no polling thread.

The game owns one current notification tab. Supplemental timers leave that
tab native-driven and animate additional confirmed destinations. The existing
native producer can replace its current tab during ordinary deposits; this
change does not hook or suppress it. Consequently the once-per-batch rule
describes the supplemental timers, not exclusive control over every native
notification under arbitrary interleaved transactions.

**Coexistence limit (static review; not observed live):** each DLL owns its own
supplemental timers. Merely loading Controller QOL and Inventory Deposit together
does not duplicate notification writes: an idle instance has no feedback work.
Rapidly alternating mouse and controller batches within an animation's lifetime
could let the two instances compete over alpha or expiry if both resolve the
same live `BankTabs`. Cross-plugin timer coordination has not been established.

Stash closure and session changes cancel pending feedback; generation tokens
reject stale queued callbacks. A missing or replaced panel discards pending
state. Failed UI scheduling discards stale confirmations and retains only safe
cleanup for a later UI refresh. Cleanup hides only indicators owned by this
module that are not currently native-owned. Shutdown stops deposit callbacks
before shutting down feedback. Arbitrary off-thread hot-unload does not write
native UI state; an owned indicator can remain until panel reconstruction or a
native deposit. Restart remains the supported DLL update workflow.

## Controller layout evidence

The following local files were inspected read-only on 2026-10-11. Portable root
placeholders identify the mod source and the active game installation.

| File | SHA-256 |
| --- | --- |
| `<reimagined-source-root>/data/global/ui/layouts/controller/bankexpansionlayouthd.json` | `F90603FCF6351FA33B2C5574F9B4FA81AE359A0AF3C71FC92DC642FF1F8A197A` |
| `<game-root>/mods/Reimagined/Reimagined.mpq/data/global/ui/layouts/controller/bankexpansionlayouthd.json` | `8386DB3AB85FED1A4BC8A3F432F7962865532C7F545A87596A80544F0C9AC204` |
| `<reimagined-source-root>/data/global/ui/layouts/bankexpansionlayouthd.json` | `A5808D04B1FEFCCBCA1E403736E7182FB21C488A4235F580C3BB34A3FE4E9176` |

The controller source and installed files differ as whole files, but their
parsed root type, name, fields, and complete direct `BankTabs` objects are
identical. Source lines 2 and 30-50 establish:

- Root type `BankPanel`, name `BankExpansionLayout`.
- Direct child type `TabBarWidget`, name `BankTabs`, with five labels in
  Personal / Shared / Gems / Materials / Runes order (indices 0 through 4).
- Notification image template
  `Controller/Panel/Stash/V2/AdditionalStash/StashNotification`.
- Controller tab position `{x:82,y:-40}` and tab size `195 x 72`, versus mouse
  layout position `{x:82,y:152}` and tab size `196 x 72`.

The feedback adapter resolves the live notification image through the native
TabBar pointers. It does not hardcode either notification asset. Its geometry
guard compares fresh SDK and native rectangles instead of requiring mouse
coordinates, so controller placement needs no adapter change.

The controller layout supplies neither `tabNotificationDurationSeconds` nor
`tabNotificationFadeOccurrences`, and declares no `basedOn` layout. Existing
constructor evidence in [shared-page-disassembly.txt](shared-page-disassembly.txt)
at `0x23A9EB` and `0x23A9F8` writes float bits `0x3EA8F5C3` (0.33 seconds) to
Bank `+0x240` and integer 3 to `+0x244`. The feedback adapter reads the live
timing fields, preserving those defaults and any valid native overrides.
Static layout compatibility is not proof of visible controller rendering.

## Native contract and exact-build limits

No new game RVA, structure offset, callable ABI, or hook is introduced by this
integration. The full existing contract, expected byte arrays, private capture
provenance, setter probes, and lifecycle limits remain documented in
[Inventory Deposit's stash feedback record](../../inventory-deposit/docs/STASH-FEEDBACK.md).

The admitted target is D2R 3.3.93787 with D2RCore 1.3.1-beta SHA-256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Existing runtime byte guards, provider hash, native widget types, ownership,
geometry, and bounded timing checks remain required. Names alone do not admit
an arbitrary replacement layout or later game build.

| Existing contract | Use in the controller adapter |
| --- | --- |
| `0x846170`, `0x856220`, `0x8562A0` | Resolve panel, direct child, and current rectangles on UI callbacks |
| Bank vtable `0x1CE4A20`; TabBar vtable `0x1D75E00` | Require the previously reviewed native types |
| `0x15EF70` | Resolve native category from each confirmed class ID; categories 1/2/3 map to tabs 2/3/4 |
| `0x879470`, `0x879D20` | Set existing tab notification visibility and alpha |
| Bank `+0x240`, `+0x244`, `+0x260` | Read fade duration, fade count, and current native notification tab |
| `0xCE500`, stash mode `0x18` | Require the open stash context |

The older QOL [Materials contract](MATERIALS-NATIVE-CONTRACT.md) independently
established the same controller panel name, direct category paths, and five-tab
order. The selected-tab helper at `0x23AF50` remains part of deposit routing;
its index is a UI category, not a native storage page number.

## Validation

The 2026-10-11 Release build passed all 32 Controller QOL CTest suites, including
the production observer bridge, shared animator, once-per-batch policy, and
DLL artifact checks. All 12 Inventory Deposit suites passed after its shared
source gained the configurable diagnostic prefix. Both runtime packaging
regression tests also passed. These results do not substitute for live
controller validation.

The tested DLL reports `1.3.1+rev.76-beta.1` in plugin and Windows version
metadata. Its ABI manifest and loader exports passed the artifact fixture;
the import table contains only the existing Windows/VC runtime dependencies,
with no Inventory Deposit DLL dependency. SHA-256:
`25C03D583A3E511DBE708797E13F447F9A837BFC0045DC0E51A044549E1A801C`.

The same DLL was installed into the local Reimagined mod-scoped plugin folder
with D2R/loader closed. The previous rev.75 DLL was backed up, all four existing
QOL config/profile files were preserved byte-for-byte, and the installed hash
matches the tested artifact. Private deployment paths and rollback receipts
remain outside versioned documentation. No public release was made.

Inventory Deposit's previously confirmed animation behavior is evidence for
the reused implementation, not a live test of this new controller integration.

Controller checks still required:

1. Deposit several different Materials in one batch; verify one existing
   Materials animation without per-item restarts.
2. Deposit mixed Gems, Materials, and Runes; verify each affected label can
   animate, including while a different tab is selected.
3. Start another batch and verify it can trigger each affected tab again;
   skipped/refused/unconfirmed items must not create extra notifications.
4. Close/reopen the stash, change sessions, and switch input layouts during the
   effect; verify no stale animation replays or persistent extra indicators.
5. With both plugins loaded, alternate mouse-button and controller Deposit All
   batches before the previous animation finishes; check for flickering or
   prematurely hidden notifications.
6. Check both local and remote sessions and verify destination totals as well
   as visible labels. Existing deposit cancellation and refusal remain intact.
