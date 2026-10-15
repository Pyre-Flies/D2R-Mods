# Item quantity coexistence - 2026-10-15

## Scope and evidence

`identify_stat.cpp`, `native_identify.cpp` and `vendor_buy.cpp` use the game
aggregate stat reader only for item quantity: stat **70**, layer **0**. Each
caller retains its exact game entry, Core wrapper/body, item identity and
context guards. This contract cannot qualify another stat or layer.

Read-only inspection of the current successful TCP/IP client session found the
game provider slot `game+0x3E2A218` pointing to Maps `+0x132F0`. Its original
reader pointer was exactly Core `+0x831DE0`. The previous QOL guard recognized
only two older file hashes and a hook at `+0x106D0`, so it would refuse the
installed chain. This is a confirmed admission gap, not a live identification
success/failure test.

| Artifact | SHA-256 |
| --- | --- |
| Game image, content 3.3.93787 | `93021dad48533bcfca8a95c69a0cc00cdb9008a3ce6ceba1d04c7aca71985a10` |
| Core | `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2` |
| Installed Maps 0.11.7 | `c459f2092a6b35e5465999484e0f232c60712bccf13977ee4fe399fe8cb892b6` |
| Retained older Maps | `fe4dcbb368a09e85c3c618f8da68f95156295b94c48deacbaf04ec4c9e6cf7d1` |

The newer Maps source's `RMap::PlayerPenalty::Read` corroborates the intended
semantics. Admission is based on the inspected binary path, not source version,
exported ownership claims or an assumed general hook-chain ABI.

## Reviewed route

All RVAs below are observations for the installed Maps hash above. Production
resolves references from the actual provider target; it does not scan for code
or hardcode these companion RVAs.

| Maps RVA / relative offset | Contract |
| --- | --- |
| `0x132F0..0x1333C`, hook `+0..+0x4C` | Save RCX/unit, EDX/stat and R8W/layer; reserve `0x98` stack bytes; load original pointer; call it once with unchanged arguments; preserve its full RAX result; stat 70 takes the branch past stat-127 instrumentation |
| `0xFCB70` | Writable, non-executable image-owned original-reader slot; must equal the separately guarded Core `+0x831DE0` |
| `0x13373..0x133BE`, hook `+0x83..+0xCE` | Null-unit/layer/level-provider gates, stat classifier call, and negative-classifier return; both return edges must reach the same checked epilogue |
| `0xFCB78` | Distinct aligned image-owned level-provider slot; only its null/non-null value is tested on this route; it is never called |
| `0x142D0..0x1430C` | Classifier subtracts 17, checks unsigned upper bound 110, resolves image base and uses two immutable lookup tables |
| `0x14370+53` | Byte-table entry for `70-17`; observed choice 8, with the chosen jump-table entry separately validated |
| `0x1434C+8*4` | Image-relative destination of the selected classifier branch; observed leaf `0x14342` |
| `0x14342..0x1434B` | Exact negative-result leaf: EAX=-1, restore `0x18` stack bytes, return |
| `0x1359D..0x135A4` | Restore the outer `0x98` stack bytes and return the preserved Core result |

`maps_quantity_profile.h` records all operative instruction bytes. Only decoded
relative-reference operands may vary. `maps_quantity_contract.h` validates
every reachable block, both lookup entries, the exact negative-result leaf,
original continuation and data ownership. The image-base calculation must
resolve to this Maps image. Executable code and tables must be readable,
non-writable committed image pages. Data slots must be readable writable
non-executable pages in that same image. Arithmetic overflow and missing reads
refuse admission. The Windows memory reader is shared with the reviewed Chat
contract; its code/data protection requirements remain unchanged.

The classifier returns negative for stat 70, so player penalties, map lookups,
stat-127 counters and their helpers are unreachable for QOL's quantity call.
Unrelated bytes and the overall file hash may change. Changing the operative
route, its tables or its ownership requires a new review. Existing caller/Core
guards remain required; this proof does not bless Core, the whole Maps plugin,
other inputs, or any future compiler output.

QOL calls through the installed game entry/provider slot after validation. It
never invokes the original pointer directly to bypass Maps. There is no cached
admission, hash override, new detour, packet change or charge write. As with the
existing native contracts, another component modifying code concurrently with
a native call is outside the supported runtime contract.

## Validation

- 2,035 standalone checks, including mutation of each bit in every operative
  code/reference/table/continuation byte, memory-role refusal, address overflow,
  unrelated-code tolerance and relocated fixtures.
- 16 calls execute the validated forwarding machine code in private test memory;
  null/non-null unit/provider combinations preserve the full original return
  value and exactly one original call with unchanged arguments.
- The production Windows image reader accepts both DLL hashes listed above in
  private mappings created without running their initialization. Each artifact
  run adds seven checks, including rejection when its code page becomes writable.
- The previous intermediate `717cbd31...` image is documented historically but
  was not available for a new artifact check in this pass; do not claim that run.
- All 35 QOL suites pass. The current candidate still requires live quantity,
  identification, vendor and persistence tests with the full host/client pack.

Private diagnostic scripts/results remain under
`outputs/qol-gem-withdrawal-20261015/`; binaries and logs are not versioned or
included in runtime packages.
