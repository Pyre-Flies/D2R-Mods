# Controller label recovery - rev.13 candidate

## Live evidence, 2026-09-27

User reproduced missing ground labels after LB+A identification, then left the
failure untouched. PID 34552, game base 0x140000000, Core SHA256
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Before LB: filtered/unfiltered bytes at game +0x235D378 = 01/00;
controller label display byte at game +0x2A50A10 = 00.
After user tapped/released LB to restore labels: filtered/unfiltered still 01/00;
display byte = 01. Both label action hook entry jumps remained installed.
This proves a stale controller display state in this reproduction, not a toggled
filtered-label flag. The code/plugin that cleared the display byte is not proven.

## Native path and guards

Existing native refresh +0xCE450 queries aggregate labels +0x1FAD80, controller
mode +0x77E10, obtains display context +0x144640, obtains input context +0x13CE90,
and computes eligibility via +0x13DE60(input). It writes the resulting boolean
to display context +0x1F60. +0x144640 returns static game +0x2A4EAB0 after lazy
initialization (TLS is for initialization, not a separate render context per
thread). Thus +0x1F60 is game +0x2A50A10 for this build. +0x13CE90 similarly returns
static +0x2A4DC20. Eligibility requires input +0xE40 == 0 and native +0x847160 true.
No direct writes to these offsets are introduced.

label_recovery_signatures.h records live exact bytes for +0x144640 (0x7A bytes),
+0x13CE90 (0x7A), +0x13DE60 (0x24), +0x77E10 (0x39). Existing guards cover
+0xCE450 and the setter +0x1FAE90. Recovery admission depends on existing label
hooks; new witnesses are also rechecked before each recovery query. A changed
witness or memory exception disables only recovery. All RVAs are build-specific.

## Implementation

The existing polling worker schedules at most one SDK game-thread check every
250ms. It does not read native controller/display state on the worker. The game
task checks the existing enabled/LB-mode/in-game policy, controller UI mode,
native label eligibility, filtered state exactly 1, and display state exactly 0.
Only that mismatch invokes the existing setter/refresh path. Healthy labels are
not rebuilt; keyboard mode and intentional menu suppression are left alone.
No new detour, filter bypass, item scan, renderer patch, or raw state write.
Trace logging is opt-in. Pending tasks are guarded against shutdown by context
and readiness checks; no heap request survives a dropped SDK task. A dropped
accepted task leaves the single pending gate closed until plugin restart rather
than accumulating replacement tasks.

## Validation and limits

MSVC Release build and all 16 suites pass. Policy regression checks cover stale
versus healthy display, disabled state, keyboard mode, menu suppression and
invalid state bytes. Before/after byte change is live verified on rev.12;
automatic recovery in rev.13 requires user testing. This is bounded recovery of
the verified invalid state, not a claim to identify the original clearing writer.
Options navigation remains user-confirmed on rev.12; stash withdrawal still awaits
testing. No GitHub production release was published for this candidate.
`nCandidate installed globally after user closed game and process absence was verified; previous rev.12 DLL backed up locally. Built and installed SHA256 matched. User reproduction test pending.
