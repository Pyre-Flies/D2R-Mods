# Auto Deposit pickup compatibility - rev.42

## Problem and ownership decision

With Auto Deposit 0.1.4 loaded first, its MinHook detour owns game RVA
`0x471950`. Controller QOL rev.41 attempted a second inline hook at the same
entry and received `MH_ERROR_ALREADY_CREATED`. This is a real byte-ownership
collision, not a `ground_pickup` policy-gating issue.

Rev.42 does not hook or restore `0x471950`. It redirects all seven observed
direct game CALLs to a QOL wrapper. A permitted pickup calls the current shared
entry, allowing Auto Deposit's detour to run. Filtered native calls return false
before reaching that entry. QOL's own optional LB pickup calls the shared entry
directly and is still gated by `ground_pickup`.

## Exact-build evidence

Runtime capture was taken from the executable image hosted by `D2RLoader.exe`
at base `0x140000000`; the loader file was version `1.3.1-beta`, SHA-256
`93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`.
The loader log identified the game build as `3.3.93847`. These contracts are
build-specific and are not portable merely because an RVA happens to exist.

| CALL RVA | Original bytes | Resolved target | Nearby source evidence |
| --- | --- | --- | --- |
| `0x410005` | `E8 46 19 06 00` | `0x471950` | `D2Game\\src\\Player\\PlrMsgCheats.cpp` |
| `0x4112F5` | `E8 56 06 06 00` | `0x471950` | `D2Game\\src\\Player\\PlrMsgCheats.cpp` |
| `0x416126` | `E8 25 B8 05 00` | `0x471950` | `D2Game\\src\\Player\\PlrMsgCheats.cpp` |
| `0x41738D` | `E8 BE A5 05 00` | `0x471950` | `D2Game\\src\\Player\\PlrMsgCheats.cpp` |
| `0x4BA0EF` | `E8 5C 78 FB FF` | `0x471950` | normal player-message region; exact source string not established |
| `0x4BBAD0` | `E8 7B 5E FB FF` | `0x471950` | `D2Game\\src\\Player\\PlrMsg.cpp` |
| `0x55496C` | `E8 DF CF F1 FF` | `0x471950` | `D2Game\\src\\Skills\\SkillSor.cpp` |

All seven calls use the existing six-argument pickup ABI:
`bool __fastcall(void* player, uint32_t guid, bool arg3, uint32_t distance,
bool arg5, bool arg6)`. The relay is an indirect JMP, so it preserves the
native CALL return address, argument registers and stack arguments.

## Admission and failure behavior

- Every original five-byte CALL is checked by D2RLoader `PatchBytes`.
- Every replacement displacement is calculated before publication.
- The wrapper remains passthrough until all seven patches publish.
- A partial publication therefore still reaches the unmodified shared entry;
  no filtered behavior is claimed in that failure state.
- The executable relay and containing DLL are retained for process lifetime
  once publication starts, matching the established scoped-CALL pattern.

## Validation boundary

Automated tests verify all seven original CALL displacements resolve to
`0x471950`, relay displacement encoding/overflow behavior, filtered-only policy
gating, artifact versioning, and the existing plugin suites. Live validation
must still confirm bare A rejects hidden items and accepts visible items while
Auto Deposit is loaded, and that `ground_pickup = false` leaves LB inactive.
