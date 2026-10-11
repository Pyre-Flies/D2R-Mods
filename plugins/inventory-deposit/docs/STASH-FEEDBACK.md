# Advanced stash tab notifications

## Behavior and evidence boundary

Beta.9 gives each advanced destination tab an independent notification timer
after the batch verifier observes a submitted item leave inventory. It uses the
game's existing notification image and native visibility/alpha setters. No
panel, MPQ, custom overlay image, input hook, packet, transfer pacing, or item
transaction changes are involved. The first confirmed removal for each
destination starts its supplemental animation once per batch. Additional items
for that tab, including different material classes and late confirmations after
expiry, cannot restart or extend it. A later batch can start it again. Gems,
Materials and Runes can all animate together. Personal and Shared are not
advanced destinations and are not marked. Existing fade duration and cycle
count are preserved; the user explicitly requested one existing animation per
batch, not a single brighten-and-fade cycle. Beta.4 through beta.8 restarted
the timer on every later confirmation.

The game has one current flashing-tab slot. Successive native deposit requests
replace that slot and clear the other tab notification images. This explains why
the whole-inventory batch can finish with only one label flashing. This is a
static explanation of the native mechanism, not a live capture of each overwrite.
The user reported the corresponding one-tab visual symptom. Existing beta.3 logs
at 2026-10-11 07:29:34, 07:29:42 and 07:32:14 report complete batches of 4, 3 and
4 source removals; the user also confirmed the custom button artwork works.

Feedback follows **observed source removal**, never submission alone. Existing
verification is not an independent advanced-storage counter or save-persistence
check. Skipped/refused/unconfirmed items cause no extra notification. Runtime IDs
are deduplicated within each batch; only copied class/runtime IDs and a plugin
batch tag cross the game-to-UI boundary. Native item pointers are never retained.

`Mailbox::Begin` issues a monotonically increasing tag and preserves tags while
compacting undrained confirmations from older batches. Each tab's `lastBatches`
entry remembers the batch that triggered it independently of its timer's active
state. Per-tab expiry clears the visual timer without clearing this tag; full
cancellation clears both mailbox and timer tags. Tag exhaustion refuses new
feedback until cancellation rather than recycling a live tag. Deposit remains
independent. These are plugin-owned fields, not new native structure offsets.

## Exact artifact and static witnesses

Target D2R is 3.3.93787 with the admitted D2RCore 1.3.1-beta SHA-256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Code provenance is the private, text-section-only capture
`<research-root>/inputs/captures/runtime-game-auto-deposit-20260927.exe`,
30,081,024 bytes, SHA-256
`02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D`.
It contains already-installed runtime redirects and is not a clean on-disk PE.
The capture stays outside versioned source. Full callable body guards and the
field/type witnesses are in `src/stash_feedback_profile.h`; they must match the
loaded image before feedback is admitted. All addresses below are D2R.exe RVAs.

| RVA/offset | Bounded finding |
| --- | --- |
| `0x159A30` | Existing native advanced-deposit producer, inspected only. |
| `0x159A4D` | Calls class-ID getter `0x349860`. |
| `0x159A54` | Calls category lookup `0x15EF70` with that class ID. |
| `0x159A5C` | Calls notification producer `0x23BB50` before packet helper `0xEC880`; this notification itself is not confirmation of destination state. |
| `0x15EF70..0x15F080` | Complete `uint8_t(uint32_t classId)` category lookup body, 0x111 bytes. Traverses the active native class/category map and returns zero on no match. Reused on UI, no invented item-code list. |
| `0x15EF7F/0x15EF92` | Lookup reads native category-map pointer/count globals at `0x22BE698/0x22BE690`; these are provenance only, not directly read by the plugin. |
| `0x23BB50..0x23BCEB` | Complete notification producer body, 0x19C bytes. Maps category 1/2/3 to tab 2/3/4 at `0x23BBB0..0x23BBCD`. Skips the currently selected tab and the already-current notification tab. |
| Bank `+0x240/+0x244` | Float fade duration in seconds and signed fade occurrence count. Constructor defaults 0.33 and 3 at `0x23A9EB/0x23A9F8`; producer reads them at `0x23BBF6/0x23BC51`. Plugin accepts 0.1..2 seconds, 1..10 occurrences. |
| Bank `+0x250/+0x258` | Native start/end timestamps, written `0x23BC0B/0x23BC60`; never written by plugin. |
| Bank `+0x260` | Single current tab byte; value 5 means no current tab. Producer stores one tab at `0x23BBEB`. Plugin reads only to avoid cancelling native-owned notification state. |
| Bank `+0x268/+0x270` | Native accumulated fade time and rising/falling direction, consumed by `0x23E3BF..0x23E3FA`. Neither is written by plugin. |
| `0x23BCC0..0x23BCDA` | Loops all five tabs and calls `0x879470` with enabled true only for Bank `+0x260`, clearing other tab notification images. |
| `0x23E2EC..0x23E45D` | Bank update computes the single triangle fade, expires/hides its current notification and sends alpha to `0x879D20`. |
| `0x879470..0x8794D4` | Complete `void(TabBar*, int index, bool enabled)` body, 0x65 bytes. Writes enabled byte at TabBar `+0x1790+index`; resolves image from `+0x1748+8*index`; requires the additional native per-tab available byte at `+0x1788+index` to show it. Resets image alpha `+0x94` to 1.0 then invokes the image's existing vtable slots `+0x48/+0x50`. No bounds check in native function: plugin only passes 2..4. |
| `0x879D20..0x879D67` | Complete `void(TabBar*, int index, float alpha)` body, 0x48 bytes, Windows x64 RCX/EDX/XMM2 ABI. Resolves the same image, clamps alpha and writes image `+0x94`. Null image is a no-op. |
| `0x1CBA904` | RIP-relative upper-bound float read by `0x879D46`. Explicit live guard requires bytes `00 00 80 3F` (1.0). Not present in the text-only capture. |
| `0x23A99E..0x23A9A7` | Constructor stores Bank vtable `exeBase+0x1CE4A20`; exact ten-byte witness admitted before using Bank fields. |
| `0x1D75E00` | Existing TabBar vtable identity, reused from Controller QOL `LADDER-CONFLICTS-REV12.md`; plugin requires this exact type before calling setters. |

The category-map global addresses above are derived from the RIP-relative
instructions in the captured lookup, not a promise of a portable data layout.
All code bytes, constants, module identities and layouts are build-specific.

## Resource paths, lookup and ownership

Current mod layout witness:
`<reimagined-source-root>/data/global/ui/layouts/bankexpansionlayouthd.json`.
Its root is `BankExpansionLayout`, type `BankPanel`; its direct `BankTabs` child
is `TabBarWidget`, has five tabs in Personal/Shared/Gems/Materials/Runes order,
and declares `notificationIconTemplate.filename` as
`PANEL\\Stash\\AdditionalStash\\StashNotification`.
The same root supplies `tabNotificationDurationSeconds: 0.33` and
`tabNotificationFadeOccurrences: 3`. Nothing in this file is modified.

Every UI update resolves SDK path handles afresh and binds their rectangles to
the native controls located by the previously admitted native helpers
`0x846170` (panel name), `0x856220` (direct child name), and `0x8562A0`
(rectangle). Bank direct-child vector `+0x58/+0x60` is checked for a complete,
non-null, bounded list before calling the child finder. Native Bank and TabBar
vtable identities and the TabBar's parent at `+0x30` must match. These lookup and
geometry contracts are inherited from `GOLD-ANCHOR.md` and existing Controller
QOL records. WidgetService does not expose a native-widget edit callback; the
SDK path is therefore a geometry/ownership witness, not a cast to a pointer.

The plugin uses the native timing fields to reproduce three 0.33-second rise /
0.33-second fall cycles by default, with independent per-tab clocks. It leaves
the game's current notification tab entirely native-driven. The additional
confirmed tabs are reasserted during UI refresh because another ordinary native
deposit can clear their visibility. A tab is hidden on expiry only if this
plugin previously enabled it and it is not now the game's current notification.
The entire panel and all raw widget pointers are local to one UI callback.

The once-per-batch rule governs the plugin's supplemental timers. It does not
hook or suppress the native notification producer: alternating native deposit
categories can still replace the game's current-tab slot. The producer already
skips its currently flashing tab. Preserving that ownership avoids changing
ordinary native notifications; automated tests do not establish strict control
over every native notification during arbitrary interleaved transactions.

Closing stash cancels queued feedback and hides owned indicators on the UI
thread while the panel still exists. Session changes invalidate scheduled task
generations, copied identities and timers. Missing/replaced panels discard
pending state. Failed classification or an unsupported category creates no
flash. Byte-profile failure disables only feedback and preserves deposit.
Failed UI scheduling cancels pending feedback, retaining only safe cleanup of
previously owned indicators for a later refresh. It never accesses widgets on
the game thread or replays stale deposit confirmations after recovery. Partial
listener registration is rolled back on initialization failure.

Normal session exit cleans up through the UI callback. Arbitrary hot-unload on
another thread during the brief active effect cannot safely perform native UI
writes: SDK unload discards queued UI tasks, and no off-thread cleanup is
attempted. An extra indicator may then remain until panel reconstruction or
the next native deposit clears it. Restart remains the supported DLL update
workflow. This limitation is intentionally preferable to unsafe UI access.

## Validation

`tools/probe_stash_feedback.py` executes the two captured native setter bodies
against synthetic TabBar/image objects. It checks each supported tab, native
availability gating, the two virtual callbacks, null images, alpha clamps and
exact mutated bytes. The probe relocates the alpha upper-bound reference to a
synthetic 1.0 constant; this proves the bounded setter ABI/footprint, not the
live game's data constant or visual rendering.

`stash_feedback_policy_tests.cpp` covers confirmation mailbox bounds/deduplication
and independent timed pulses. `stash_feedback_tests.cpp` exercises the actual
module with a substituted native boundary so scheduling, lifecycle, refusal
and cleanup behavior can be checked without opening the game. Source/artifact
tests do not qualify the live custom notification effect, destination storage
counters or save persistence. Live mixed Gems/Materials/Runes label behavior
and the beta.9 once-per-batch behavior require live verification. The user
reported visible flashing on the earlier build; that observation does not
independently establish all mixed-destination cases.

## Beta.9 validation and local deployment, 2026-10-11

Release build and all nine suites passed. New policy tests cover retained batch
tags during mailbox compaction, duplicate destinations before/after expiry,
fresh-batch restart, cancellation reset and tag exhaustion. The actual module
fixture sends six distinct Materials classes across multiple UI updates and
beyond the original animation expiry: all are classified, only one animation
starts, and its three configured fade cycles retain their original phase and
expiry. New batches, mixed destinations and partially drained preceding batches
keep independent timing. Native current-tab ownership tests remain unchanged.

Artifact/package checks verified version, ABI, exports, imports, layout,
embedded sprite bytes and packaged DLL identity. Source and sprite comparisons
confirm button fit, artwork, configuration and transfer code are unchanged.
The latest beta.8 log records verified gap85/offset1 placement and a completed
four-source-removal batch at 09:06:24 on 2026-10-11. The user confirmed that
button fit; this is not live validation of beta.9 notification deduplication.

| Artifact | SHA-256 |
| --- | --- |
| `Inventory Deposit.dll`, 616448 bytes | `63E22F72A25C4DBD4C34496EA9574E207382D621AE983C9DD073BD5D3116167A` |
| `Inventory-Deposit-0.1.0-beta.9-runtime.zip` | `F941757353E6453604212D4A6C6C0C8DD73A937F8B8F18C9B081D912AD2A3594` |

Installed with game and loader closed; deployed DLL matches the tested build.
Previous DLL/config backup is under the game root at
`d2rloader/backups/inventory-deposit-batch-tab-feedback-20261011-091430/`.
Configuration remains byte-identical, SHA-256
`154283F7EBD5CCB13D8056D533E8E8F4C487594BB7211040B3723B8241571E2A`.
Restore the previous DLL to roll back. New once-per-batch supplemental feedback
requires a live check; no public release or commit was made.

## Beta.4 build and local deployment, 2026-10-11

Release build passed all nine CTest suites: sprite-codec, UI, batch-policy,
transfer-policy, artifact, settings, gold-anchor, feedback-policy and feedback.
The feedback fixture covers independent destination timers, rapid consecutive
batches, failed reads/scheduling, stale callbacks, session cancellation, partial
service registration, and fresh batches arriving before old visual cleanup.
The isolated setter probe also verified all five new expected-byte arrays
against the pinned private capture. No game process was accessed by the probe.

| Artifact | SHA-256 |
| --- | --- |
| `Inventory Deposit.dll` (512512 bytes) | `FB698CA443B813CAA77C4E392BA997F4D5796EE9BF7AF405ADFAB93E4B95C12E` |
| `Inventory-Deposit-0.1.0-beta.4-runtime.zip` | `0788E51D65B1AB73FA85AEAAC96095B512FAC605566E997A927E972661819D7B` |

Actual DLL checks passed for the ABI manifest, exports, version and exact
embedded sprites. Imports remain KERNEL32.dll and bcrypt.dll. The runtime ZIP
contains the verified DLL under `d2rloader/plugins/`, reference settings under
`defaults/`, instructions and licenses. Integrity, exact entries, DLL equality
and absence of external PNG/sprite/MPQ dependencies were checked.

D2R and D2RLoader were closed before installation. The previous beta.3 DLL and
configuration were copied to
`d2rloader/backups/inventory-deposit-stash-feedback-20261011-074609/` in the
active local game root. The installed beta.4 hash matches the artifact above.
Configuration remained byte-identical, SHA-256
`1DFE1C543CC6465A0ABE6A2D63708905C4F2B0950014C5D47F74B3CF3E5F5090`.
Controller QOL's installed DLL was not changed. This is a local candidate,
not a published release or confirmation of live multi-tab rendering.
