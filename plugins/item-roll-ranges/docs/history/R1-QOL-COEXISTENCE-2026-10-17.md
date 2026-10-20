# R1 range-query coexistence diagnosis - 2026-10-17

## Observed installation

The Reimagined client startup at 12:03:49 local time loaded Controller QOL
`1.3.1+rev.79-beta.4` and Item Roll Ranges `1.3.1+rev.52`. Core item stat
ranges were enabled. QOL logged successful native tooltip RT-to-RB query
remapping, and Item Roll Ranges logged native Ctrl/controller hold mode.
The active Item Roll Ranges config had `always_show_ranges = false`.
No startup refusal explains the missing R1 behavior. The game had exited
before this investigation, so individual button events were not observed.

Installed artifact SHA256, matched to existing deployment receipts:

- QOL: `9bb0a83e6078c5a5d58665ed13e2d6710987da46923471ac9a42ce040eaecfa9`.
- Item Roll Ranges: `e0c5186a41b60ca51f07cd442953f8bf65172200d4f84d8619b5e7466759f0ee`.
- QOL's log confirms Core admission against
  `2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2`.

## Exact call-chain conflict

For this build, Core's tooltip controller query returns to Core+`0x81995A`
and uses provider slot Core+`0x6FE470`, originally game+`0x13CA70`.
QOL hooks that game entry. Its `QolRangePolicy::Mask` remaps requested mask
`0x800` (RT) to `0x200` (RB) only when the immediate return address equals
Core+`0x81995A`, the remap is active, and the user index is below eight.

Item Roll Ranges' `InstallAlways` replaces that provider slot with
`AlwaysButtonAdapter`, even when always-show is disabled. That adapter calls
`originalButtons(device,user,mask)` before applying its optional always-show
policy. The original pointer still reaches QOL's detoured native entry, but
QOL now observes a return address inside Item Roll Ranges rather than Core.
Its exact caller qualification therefore does not remap RT to RB. Disabling
the always-show setting does not remove this intermediate call.

This is an input-query composition failure, not evidence that physical R1 is
consumed or that Item Roll Ranges failed to load. No unknown predecessor was
admitted and no guard was weakened during diagnosis.

## Reproduction and limits

A private, standalone MSVC fixture includes the deployed QOL source's
`range_input_policy.h` and rev.52's `policy.h`, reproduces the QOL hook and
Item Roll Ranges adapter call sequence, and supplies deterministic native input.
It uses actual return addresses and disables tail-call elimination. Running
`python outputs/r1-ranges-20261017/run.py` produced exit 1:

```text
R1 direct QOL query: PASS
R1 through rev.52 wrapper (always off): FAIL
Caller changed by wrapper: yes
RT through wrapper: PASS
```

The fixture demonstrates the source-level failure mechanism; it is not an
in-game button trace. RT is a supported workaround prediction from this fixture,
not yet user-confirmed in this session. No DLL/config edits, deployment, runtime
patching, or publication occurred. A repair must preserve the qualified tooltip
context across the two integrations without accepting arbitrary callers or
altering general controller queries; it needs a composed-call regression test
and a live R1/RT retest with both plugins loaded.

## Rev.53-beta.1 repair

Replace only Item Roll Ranges' controller provider-slot wrapper with an
assembly entry that selects a target and tail-forwards to it. For normal,
disabled, unsupported-scope or unrelated queries, the target is the existing
`originalButtons` pointer. The original return address and device/user/mask
arguments reach the native entry, including QOL's existing detour. QOL's
exact caller check, all native guards and the game/Core call sites are unchanged.

For an admitted tooltip query with always-show enabled, a valid device/user,
RT mask and eligible foreground panel, the selected target calls the same
predecessor exactly once and ORs bit 0 into its full 64-bit return value.
The decision is sampled before dispatch; a concurrent setting change applies
to the next query. The command/config persistence implementation and keyboard
adapter are unchanged. No raw input, physical button, provider admission or
global controller state is rewritten.

Local assembly ABI: `AlwaysButtonAdapter` receives RCX=device, EDX=user,
R8D=mask and returns the native uint64 result in RAX. Its unwind-described
0x48-byte frame provides shadow space and saves RCX/RDX/R8/R9 at +0x20/28/30/38.
The original return address at +0x48 is the selector's fourth argument. After
restoring argument registers and RSP, the gate tail-dispatches via
`__guard_dispatch_icall_fptr`, preserving CFG. No nonvolatile register is
modified. The slot publication/restoration still uses the existing
compare-exchange ownership protocol. No new game/Core RVA is introduced.

The added `range-controller` suite includes the real plugin adapters and
QOL's production caller policy. Before the repair it failed the composed R1
assertion (line 56); afterward all 61 checks passed. Coverage includes
always-show on/off, R1 and RT with QOL enabled/disabled/absent, unrelated
callers and masks, foreground/panel/lifecycle refusal, device/user bounds,
full return-bit and argument preservation, one predecessor call, and owned
versus later-owner slot restoration. This is a deterministic production-adapter
test, not a live-game input trace. Live R1 and always-show retesting is pending.

Candidate validation: all ten CTest suites pass, including the composed-input,
configuration, existing adapter and DLL artifact suites. Built DLL SHA256:
`d74b09527bd034736e01e78cc298be7caebe31cbf1b14bd39d9f13656778aebe`.
The initial full-suite run found a stale rev.52 test-version expectation;
updating it to rev.53-beta.1 produced the complete pass.

## Live follow-up

On 2026-10-17 the user confirmed restored R1 functionality after restarting
with rev.53-beta.1. The client log confirms that version loaded at 12:23:42,
then records AlwaysShowRolls ON (saved) at 12:24:54 and OFF (saved) at 12:25:01.
The active TOML reads `always_show_ranges = false`, matching the last command.
These observations establish the repaired R1 behavior and command persistence
to disk, not a separate restart readback of a changed setting.

The repair is entirely in the client tooltip query path; it adds no network
messages or server-owned item/state changes. Hosted operation is expected
with the supported client plugin/Core combination, without a server-side fix.
A distinct hosted retest of this revision has not been identified or confirmed.
