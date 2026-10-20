# Ladder leave-game crash - 2026-10-08

User reports Telekinesis object interaction now works but needs two taps, followed
by a crash when leaving. Keep targeting timing and crash attribution separate.

## Recorded failure

- PID 35708, thread 42608; last-resort report Generated UTC
  `2026-10-08 11:39:13.646` (07:39:13 America/New_York). The filename's 11:36:01
  timestamp is process startup, not the crash time.
- Windows Application Error 1000 at 07:39:16 and WER 1001 at 07:39:19 confirm
  `C0000005`, `D2RLoader.exe +0x10A2540`. Dump:
  `D2RLoader.exe.35708.dmp`; local evidence under ignored
  `outputs/crash-review-20261008/`. No dump is committed or uploaded.
- Loaded QOL candidate was rev.65-beta.1, deployed SHA256
  `BC9307450EB588ECE22AC9363F7AB0D107ECA54CBE36B1AC2FFB3D0964778C9B`.
  Loader disk identity remains the reviewed 1.3.1.0 profile, SHA256
  `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`.
- Dump confirms instruction bytes `48 89 02 48 8B 01 48 FF 60 20 C3` at the
  fault. `mov [rdx],rax` attempts to use RDX `8B80E28B80E28B80`, a noncanonical
  pointer. The recorded exception parameter classifies it as read/address -1;
  retain that raw fact rather than inferring a valid accessed address from it.

## Static cleanup-path leads (build-specific)

Reviewed code-only capture SHA256
`246DCB55785662194EE1103EF159ABD2DD37DCE66A7711BDC33CC519AE15FDFF`:

| RVA | Observation |
|---|---|
| `0x10A2530..0x10A254A` | Null-check RDX; RCX becomes `[RCX+0x10]`; assign table pointer `game+0x1DA9C10` through RDX; tail-call `[RCX.vtable+0x20]`. Fault is the assignment at +10A2540. Exact object type/ownership unproven. |
| `0x107EEA7` | Dispatcher branch loads `[RDX+0x18]` as RDX and tail-calls virtual slot `+0x3C0` from the object behind RCX. This address appears in R8; not itself a proven stack frame. |
| `0x107EF50..0x107EFDF` | Iterates a doubly linked list, calls dispatcher `0x107EC30` at +107EF88, then unlinks and releases each node. First raw stack return value is +107EF8D. |
| `0x107F1F0..0x107F238` | Advances a ring index modulo a count and calls +107EF50 at +107F225; raw stack includes +107F22A. |
| `0xEF87B0` | Calls +107F1F0; raw stack includes return +EF87B5. |

These support a native cleanup/list-drain lead, not a symbolized call stack or a
proven owner. No native patch was added at these addresses. The code-only capture
lacks usable full unwind/type metadata; raw stack scans contain both code and
data addresses and cannot establish a complete caller chain. No QOL address
appeared in the inspected first 2 KiB of raw stack candidates, which does not
exclude earlier corruption caused by QOL or interactions with another plugin.

An earlier PID 56624 last-resort report at 07:08:17, before rev.65 deployment,
faulted at game+0x7C1A0 with RBX `80E28B80E28C80E2`. The prior day's report also
records an E28B80 repeating invalid-pointer pattern. This is supporting history,
not proof of the same root cause. Separate Supporter Portals fast-fails at
07:27:27/28 use C0000409/+54645; they are distinct from this access violation.

Supporter Portals logged connection=0 at 07:39:13.435; Global Chat logged game
ended at 07:39:13.613, immediately before the fault. This corroborates leave-game
timing. This run also contains Global Chat HTTP 401 and Supporter Portals missing
access-token warnings. Those warnings establish incomplete authentication for
this launch, not that authentication caused the invalid native cleanup pointer.

Root cause remains open. Useful next isolation is a same-profile leave-game
comparison with QOL enabled/disabled, keeping other variables fixed, plus a
symbol-aware first-fault/unwind capture if reproducible. No crash fix is claimed.

## 17:41 recurrence

PID 24680, thread 30240, last-resort Generated UTC 21:41:48.926 (17:41:48
Eastern); Windows event 1000 follows at 17:41:52. Exception C0000005 at
game/loader+0x15880C3, read/address -1, RCX and RBP both
`E28B80E28B80E28B`. This exactly matches the location, exception and invalid
register pattern in the October 7 PID 27600 report. It differs from this
morning's +0x10A2540 cleanup site, despite the related-looking repeating pointer
pattern. Same fault signature does not prove the same original corrupting write
or identify its owner.

The latest QOL log's final line is R3 enabling skill 84 at 17:41:47.497, about
1.43 seconds before the exception; timing alone does not establish causality.
Preserved the report and QOL log (`controller-qol-174148.log`) in the existing
ignored evidence directory. This was a read-only comparison; no plugin or config
change was made. Earlier events today also repeated +0x1175254 at 17:21:07 and
the Supporter Portals C0000409/+54645 signature at 17:26:13. Attribution remains
open across these distinct observed fault sites.

## User-requested two-plugin isolation

At 17:44 Eastern, with no D2R/D2RLoader process running, disabled only
`d2rl-supporter-portals.dll` and `d2rl-trade-notifications.dll` in the Ladder
`d2rloader/plugins` folder by appending `.disabled`. Copied recoverable originals
and a hash manifest to ignored `outputs/crash-review-20261008/disabled-plugins-174456/`;
verified hashes after renaming and absence of both original `.dll` paths.
Configurations and other plugins were not changed by this action. Removing the
`.disabled` suffix restores them. The next launch/test result remains pending;
disabling these two does not itself attribute the preceding access violations.
