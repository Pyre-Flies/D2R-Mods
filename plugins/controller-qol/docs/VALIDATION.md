# Validation — 1.5.14-native-sell

For current rev.75 release coverage and live-test limits, see the
[production record](PRODUCTION-1.3.1-rev.75.md) and [suite release notes](../../../docs/RELEASE-NOTES.md).
The dated entries below retain their original validation scope. Current config
editing/migration is documented in [the profile guide](SKILL-DISCOVERY-CONFIG.md).

Release build and 7/7 CTest suites passed on 2026-09-23 after fixing a test-local variable naming conflict. Added vendor native-call flags/null-context and shop/source routing tests. Eight native byte profiles match the reviewed runtime PE. Source mirror synced. No live item sale performed; gold/item confirmation pending.

Installed with D2R stopped. DLL SHA256: 6FD2B9BDA2AC3E6F24E88BFF463F9A272CAD5F124A74120C44EA9C6D5142A609

Installed: <game-root>\mods\Reimagined\d2rloader\plugins\QOL.dll

Golden rollback: <private-workspace>\outputs\QOL-1.5.13-golden.zip
Immediate DLL backup: <private-workspace>\outputs\QOL-native-sell-rollback\QOL.dll
Close D2R before restoring. Configuration/layout unchanged.

See NPC-SELLING.md for ABI, native pricing/packet evidence, and patch recovery.
