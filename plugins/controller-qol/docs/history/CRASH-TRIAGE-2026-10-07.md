# Full Ladder crash triage - 2026-10-07

User reports unexpected exits shortly after joining or while idle, rather than
intentional shutdown. No DLL/configuration/package changes were made for this
inspection. Windows Application Error events (1000) confirm four distinct
D2RLoader processes failed after noon (America/New_York):

| Time | PID | Exception | Fault location |
| --- | --- | --- | --- |
| 12:29:38 | 29120 | C0000005 | D2RLoader.exe +1175254 |
| 12:38:36 | 27600 | C0000005 | D2RLoader.exe +15880C3 |
| 12:43:59 | 25888 | C0000409 | d2rl-trade-notifications.dll +30F95 |
| 12:51:28 | 42604 | C0000409 | d2rl-supporter-portals.dll +54645 |

The loader last-resort reports independently capture the first two exception
locations at 12:29:35 and 12:38:33; their filenames contain earlier timestamps,
so use their internal Generated UTC and PID to correlate. The first is a write
to address 4; the second a read fault with RCX/RBP E28B80E28B80E28B. Stack slots
are raw values and must not be presented as a symbolized/unwound call stack.
The associated game image identity is the existing Loader 1.3.1.0 profile,
disk SHA256 93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10.

Windows CrashDumps contains all four PID-specific dumps. Minimal read-only
parsing of exception/module/thread streams confirms the latter two instruction
locations. Current plugin artifact disassembly at each fault shows
`B9 07 00 00 00` (ECX=7) followed by `CD 29` (fast fail); the Windows SDK names
7 FAST_FAIL_FATAL_APP_EXIT. This is a deliberate runtime termination site, not
by itself proof of a buffer overrun despite the generic C0000409 exception.

- Trade Notifications SHA256:
  85C9DA3C73EDB8B4623C179E2958CF8AA406EB56670B4D1D5BB20CB63205D8EB.
  Raw stack contains +2E956 and +778F; the code before +778F calls +2E938
  conditionally after a boolean helper. The caller/termination cause remains
  unproven without unwind/symbol analysis.
- Supporter Portals SHA256:
  15A522351AE7914ADD409E8ED0EB6FE82E43A6691E9FDEAE0B41AB6E87C23A51.
  Raw stack includes +53E3E and +B7AF. These are leads, not proven frames.

The latest QOL log ends with vendor tome charges observed 91 -> 100 at
12:42:41.815. It contains no QOL fatal diagnostic. This and the named faulting
modules do not exclude earlier corruption or a cross-plugin interaction.
Supporter Portals logged connection=0 at 12:43:53.708 before the Trade
Notifications crash; it may be a consequence of disconnection rather than its
cause. Multiple processes existed, so cross-file correlation needs PID care.

Evidence retained locally under outputs/crash-review-20261007: events.json,
PID-specific dump summaries, copied last-resort reports and three plugin logs.
Raw dumps stay in the local Windows CrashDumps folder; none are committed or
uploaded. WER archive access was partially denied; the Application events and
local dumps were readable. No symbol-aware unwinder was available in the
initial check. Root cause remains open, particularly for the earlier game
access violations. Do not attribute or exonerate QOL solely from fault module.

## Coexistence follow-up

The user wants to distinguish QOL interactions from an imperfect local Ladder
simulation. A narrow source review found no QOL reference to the Supporter
Portals visual hook at game RVA 0x779103 and no QOL code managing either
plugin's worker threads. Trade Notifications and Supporter Portals each own
std::thread workers and join them in their unload routines. The fast-fail sites
and raw stack leads are compatible with runtime termination during destruction,
but the reason for entering termination is unproven. Source checkout behavior
alone is not an exact symbol mapping for the loaded binaries. These may be
secondary exit failures; the original trigger could lie elsewhere.

The earlier shortcut regression is a different established mechanism: QOL's
strict admission rejected Global Chat's send detour and Maps' stat-slot owner.
The corrected candidate validates and preserves both chains; it installs no
replacement hook at either conflict site. That fixes demonstrated admission
incompatibilities, but is not proof of overall crash freedom. A controlled
same-environment comparison with QOL present/absent (using an accepted test
package if integrity checks require it), and a symbolized exception stack or
first-fault capture, remain the useful isolation evidence. No package or
worker behavior was modified during this review.
