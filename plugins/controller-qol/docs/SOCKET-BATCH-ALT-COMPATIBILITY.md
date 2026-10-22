# Socket batching and ALT pickup compatibility (rev.89 / 1.2.16)

## Result and independence

UnHoarder combines neighboring witnesses inside each socket guard into one
fresh read per eligible pair. Its own properties and QOL's synchronous visibility
queries benefit. QOL's client-loot logic, controller pickup settings, optional
service leases and all existing per-item checks are unchanged. Both plugins
continue to work alone; no QOL service is required for UnHoarder's mouse policy.

The requested behavior change is separate: UnHoarder now blocks a proven
filter-hidden keyboard/mouse ground pickup unless either keyboard ALT key is
held. This covers requests from the unlabeled gear icon as well as other routes
through the existing dispatcher. Controller/unknown input passes through to the
existing native/QOL behavior. No key polling loop or cached ALT deadline exists.

## Preserved socket contract

The three pairs are Maps Head/Gate (`0x132F0`/`0x13373`), original/level slots
(`0xFEB70`/`0xFEB78` in the reviewed image), and Index/Negative
(`0x142D0`/`0x14342`). The pair reader uses the original image reader for one
covering span of at most 512 bytes on one hardware page. Full section, kind,
current protection, allocation owner and fault-contained copy remain; unusual
layout or failed covering reads retain the two original narrow reads.
All byte/operand/continuation comparisons remain. Nothing survives the call.
The outer two socket results, equality/range and unit identity checks, and four
complete before/after Maps/Core guards are unchanged. No native function is
bypassed. Quality/level/flag observation and ordered rule evaluation are unchanged.

Owned-process timing (7 samples x 2,000 iterations) measured four guards at
59.834 vs 44.654 us, a 25.4% reduction. It is not an FPS forecast. The latest live
baseline was rev.88/1.2.15 at user-reported 90-120 FPS with Wallpaper Engine closed;
this new pair still needs the same-pile before/after test.

## Exact new companion and hook contracts

UnHoarder **1.2.16** SHA256 `cc9dfc7430089a74d3732b2a3551d9cee05d3517940d34b4795058bf442bd4d7`.
Image size `0xec000`. Item-code wrapper `0x1fd40`, original slot
`0xb9b68`; the complete 19-byte unchanged-argument/return forwarding graph
is in `item_code_socket_batch_alt_contract.h`.

Dispatch wrapper `0x213f0`, original slot `0xe2e00`, game
base slot `0xb9b58`, qualifier `0x295d0`, vendor invalidation
`0x1ebb0`. `unhoarder_dispatch_socket_batch_alt_contract.h` verifies the
native entry tail, exact relay/destination/wrapper, writable original slot,
trampoline and native continuation, game-base pointer and full item-code graph.
The sole executable section is at `0x1000`, size `578847` with no
base relocations. Its SHA256 is `1a3d7aed115f6c5915aa6f948da1bdc8da47160881dc545ec7eccd153a00cc71`. Every dispatch admission
re-reads and hashes all executable bytes, including the filtering helper.
Disk identity alone never admits changed live code. All fourteen earlier item
profiles and earlier dispatcher profiles remain; unknown changes fail open.

The native dispatcher is game `0xFABE0`, x64
`void(uint32 action, void* player, uint32 type, uint32 id)`; pickup is action 22,
item type 4, mode 3 only. Existing callers `0xFA115`, `0xFBF30`, `0x101ADF` and
lookup edge `0xFACB9 -> 0x9A5D0` qualify installation, not caller permission.
UnHoarder now requires the complete captured original dispatcher from QOL's
`loot_approach_profile.h` instead of a permissive live prologue snapshot.
Its supported-build/PE/SDK ownership gates remain. SDK manages hook lifetime.

UnHoarder reuses game UI manager `0x3440170`, DWORD mode at manager `+0xDC`,
from the reviewed native-input contract. Only readable mode zero with a stable
manager pointer qualifies keyboard input. Other/missing state passes through.
ALT reads use only each key's current high bit. Stable item type/class/ID/mode is
checked after rules, then ALT/input mode is rechecked before blocking. No new
game addresses or runtime plugin dependencies were introduced. Labels and filter
configuration are unchanged. Automatic routes bypassing this dispatcher are not
claimed covered.

## Validation and diagnostics

The actual production pickup callback fixture covers hidden gear-icon requests
without label state, left/right ALT, release/late press, controller and unknown
input, null/unreadable state, identity reuse, visible/unknown rules and exactly
once forwarding. Socket tests cover all graph mutations, execution of captured
native instructions in a private process, fresh paired reads and live protection
changes. New QOL tests exercise the actual companion DLL, reject changed helper
code, every graph byte, incorrect ownership and continuations, and execute the
captured wrapper with spies. Previous companion admission tests remain active.
These checks are automated evidence, not live pickup or FPS confirmation.

Both perf commands remain. UnHoarder adds `pickup-guard` and
`pickup-hidden-blocked`; `LOOT_PICKUP_GUARD_READY` reports keyboard-only ALT
policy. Recheck direct and walk-to-item mouse pickup, both ALT keys, ALT release,
visible items and existing controller modifier/shortcut behavior in game.
