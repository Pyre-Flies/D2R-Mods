# Controller QOL Updates 1.5.19 — golden checkpoint

Made by PyreFly for D2RLoader. User confirmed the portal-priority fix works and requested preservation.

This checkpoint contains the exact installed DLL and TOML, the verified release and source archives, native hook/RVA documentation, signatures, and portal-only runtime evidence. The previous 1.5.13 golden checkpoint remains untouched.

Preserved settings: portal_priority_distance = 10, ground_pickup_distance = 6, debug_logging = false, portal_diagnostics = true. Diagnostic settings were intentionally preserved exactly; no settings were changed while making this checkpoint.

## Restore

1. Close Diablo II: Resurrected.
2. Back up your current DLL and configuration.
3. Copy the contents of restore/d2rloader into mods/Reimagined/d2rloader, replacing the matching Controller QOL Updates.dll and controller-qol-updates.toml files. Keep only one active copy of this plugin in the plugins folder.
4. Restart the game. Confirm version 1.5.19 and the configured radius in the plugin log.

Use the restore configuration to return to the tested snapshot; the release archive's configuration is the default reference file and differs in diagnostic settings. The source archive includes build instructions, tests and SDK files; see tools/build_local.py and docs/PORTAL-PRIORITY.md inside it. Native signatures are game-build-specific, so this checkpoint is a rollback for the current game build, not automatic compatibility with future patches.

DLL SHA256: 98f36a9aa2d54b05168194529de957d46fcb06748813a10b0dc5f9c7280bdb63

SHA256SUMS covers all checkpoint files. The sibling ZIP.sha256 file covers the complete archive.

Primary archive: `<private-workspace>\outputs\Controller-QOL-Updates-1.5.19-golden.zip`

Local recovery copy: `<game-root>\mods\Reimagined\d2rloader\backups\Controller-QOL-Updates-1.5.19-golden.zip`
