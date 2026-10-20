# Controller QOL Updates 1.3.1+rev.22

Promoted for production at the user's request on 2026-09-27. This release includes rev.12 through rev.22 fixes and features; see CHANGELOG.md and the accompanying revision records. Debug logging remains off by default.

Windows x64, D2RLoader 1.3.1 / plugin ABI4, SDK v0.3.0. Current guarded Core SHA256:
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.

The existing local rev.22 build passed all 19 suites during development. Publication does not imply additional live validation. User confirmed the Chronicle-to-Quest crash fix, Shared Stash transfers, controller-only labels, stat-70 identification and potion fixes in prior revision testing. The two-row shortcut header still needs spacing refinement. Standalone and combined-plugin native RB ranges require explicit live confirmation. See NATIVE-RANGES-REV22.md for the new caller-scoped query hook and shared-entry ownership limitations.

Close game/loader before replacement, back up the old DLL, keep one active copy and preserve user configuration. Hot reload unsupported. Unknown hook owners and unsupported native profiles fail open rather than being overwritten.

The source is maintained in the GitHub repository; release ZIPs contain installable runtime files and documentation, not proprietary game images or development dumps. GitHub's existing release workflow builds the tagged source and generates its own archive checksums.
