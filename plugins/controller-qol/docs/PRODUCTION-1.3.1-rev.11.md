# Controller QOL Updates 1.3.1+rev.11

Promoted for production at the user's request. Includes all rev.10 behavior.
Identification now removes the unconditional per-item log dump and duplicate UI
scan, resolves exact item identity, and uses one authoritative SDK inventory scan.
Checked SDK edits/transactions replace the raw native flag fallback. No new hooks
or RVAs. Debug logging remains off by default.

Fifteen automated suites pass, including authoritative handle resolution,
consumables, failed enumeration, exact identity and charge-boundary simulations.
The local rev.10 logs showed 360-460 ms identification attempts with 29-39 item
log lines. Quantified live improvement and the external reporter's mod-specific
tome failure remain unverified; production promotion is not evidence of those
results. See IDENTIFY-REV11.md for investigation and test scope.

Quantity>1 identify/debit is atomic via the SDK. Final tome/scroll boundary paths
use checked sequential operations with compensating edits, not an atomic
transaction; compensation failures produce an explicit diagnostic.

Windows x64, D2RLoader 1.3.1 / ABI4. Existing private native features retain their
build guards for Core SHA256
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Identification itself adds no native address dependencies. Hot reload unsupported.
Back up the DLL, close the game/loader, install one copy and retain user config.
