# Proc range precedence and headerless amulet report - 2026-10-17

User screenshot: 100% Chance to cast level48 Nova when you Die, source of Novas
T1, but leading bounds +36..+48. Existing FixedProcRange can prove chance100
from saved owned/aggregate stat, explicit function11 chance/level operands and
actual displayed numbers. Its output was only a fallback, so Core's native
skill-level range won. The adapter now marks a successfully verified proc chance
as replacing native bounds. It does not infer chance from the displayed level.
No native hook, offset or ABI changes.

Regression tests cover competing native level12..17 and36..48 spans, including
stat197 death proc, actual level preservation and [100%] prefix. All nine suites
pass in the local build/irr51 development directory. Version remains rev50 until
the candidate is finalized. No deployment or publication made for this fix.

Separate open issue: screenshot of a magic Inner Sight amulet without numeric
requirements shows Ilvl/Qlvl above its name. LevelInfo currently appends on the
no-anchor path, based on an incomplete assumption that this buffer excludes
item names. Existing synthetic tests did not include that full tooltip case.
Read-only line-order capture requested before replacing the placement rule.
Private observation helper outputs/observe_irr51_header.py reuses the documented
rev34 qualified frame/receiver path; no new addresses introduced. Do not claim
the header issue fixed until full-buffer proof and live confirmation exist.

## Header evidence and candidate completion

Read-only PID52104, Core base0xc0de5000000 and installed rev50 at
0x7ffac8530000 (addresses specific to this process). The short-lived native
frame scan returned no samples. A bounded read-only scan of committed writable
private regions retained this final line order, matching the user screenshot:
proc + labels / Amulet of Inner Sight (99) / (Ilvl:99,Qlvl:1).
Private raw capture: <workspace>/outputs/irr51-header-retained.json. Copied fragments
can begin mid-string; only the complete title/newline ordering is used as evidence.
No native functions were invoked and no memory was written. Earlier frame paths
were reused, not newly inferred. Heap addresses are ephemeral, not runtime guards.

LevelInfo no longer appends to an unanchored full tooltip. AnnotateLevelHeader
qualifies the last line as a single-line normal/magic title (quality1..4) only
when its sole numeric token equals stored Ilvl and its exact suffix is (Ilvl).
Color tokens are ignored for this proof, with original text preserved. Placement
is immediately before that title in bottom-to-top storage. Numeric requirement,
defense, durability and weapon anchors retain their existing behavior.

Limit: no numeric anchors plus a multi-line rare/set/unique title, disabled native
item-level suffix, mismatched suffix or otherwise unrecognized title causes the
extra level line to be omitted. Do not guess these boundaries or append at the
end. This supersedes rev45's unproven upper-body-boundary fallback, including its
synthetic empty-body tests. No English item-name matching is used.

The exact amulet layout, trailing-newline variant, duplicate suppression, wrong
level and unsupported quality regressions pass. Both proc and header changes are
now rev51. All nine suites pass. Live validation and deployment are pending;
PID52104 was still running at final inspection. Installer prepared with candidate
hash verification and retained config/rollback behavior.

Rev51 installed in standalone Reimagined after closure/process and exclusive-file
checks passed. Rev50 rollback backup retained; 121 DLL/config inventory entries
verified, with only the intended DLL changed. Installed SHA256:
4A7C4A1093C530B785CE0182FBDAE5FE1D27658B4E05CC38185AA580D27C7411.
Receipt: <workspace>/outputs/item-roll-ranges-test-rev51-standalone-deployment.json.
Live confirmation remains pending; no publication performed.
