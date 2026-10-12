# Validation — 1.5.14-native-sell

Local **rev.76-beta.4** follow-up: Release build and **33/33 suites passed**.
Real Cube codes now exercise legacy/explicit queues, live executor boundaries,
Shared SDK refusal and both overlapping-trigger orders through BankPanel.
On 2026-10-11, the user confirmed the beta.4 Cube self-insertion fix, Shared
Stash escape fix and Cube behavior through rejoins. Beta.3 T04 was user-cleared;
T01/T03 were partial, and T05 actually moved the Cube into itself with LB+X.
The user subsequently accepted non-online gameplay as ready. Online/hosted
qualification remains open; unreported matrix steps are coverage limits, and
pickup reach/no-approach behavior remains unchanged and documented. See
[the follow-up evidence](AUDIT-LIVE-FOLLOWUP-BETA4.md).
Beta.4 is installed locally after verified backup, with all 28 config files and
39 other plugins unchanged. Installed/build hash is recorded in that evidence.

Local **rev.76-beta.3 audit repair candidate**: Release build and **33/33 CTest
suites passed**, including production handler regressions for all eight
findings. Built under `outputs/controller-qol-repair-20261011-build` and now
installed locally in Reimagined at the user's request after verified beta.2
backup. Installed/build SHA256 matches
`19064D851E752BF7A9935636B7D9FA4A9B01BF41139A675FA48E7610CC312759`.
All 28 configuration files and 39 other plugin files were unchanged during
installation. A guarded rollback script/private receipt is retained under
`outputs/`. No commit, push, publication or game launch. In-game/hosted acceptance,
persistence and performance remained pending at build time; subsequent
beta.3 live failures are recorded in the beta.4 follow-up above. See the
[per-issue reassessment](AUDIT-REPAIRS-REV76.md) and
[single-client live cases](AUDIT-REPAIRS-IN-GAME-TESTS.md). The tester can run
only one client: distinct-peer/network qualification remains unverified and is
not a required manual step in this checklist.

For current rev.75 release coverage and live-test limits, see the
[production record](PRODUCTION-1.3.1-rev.75.md) and [suite release notes](../../../docs/RELEASE-NOTES.md).
The dated entries below retain their original validation scope. Current config
editing/migration is documented in [the profile guide](SKILL-DISCOVERY-CONFIG.md).

Local rev.76-beta.2 Mod Information navigation candidate: Release build and all
32 Controller QOL suites passed. The tested build is now installed locally in
Reimagined at the user's request, with a rollback backup and all existing
configuration/profile files preserved. L1/R1 information tabs,
LT/RT underlying-panel blocking, and live prompt/scroll/close behavior remain
unverified in game. See [the scoped validation record](MOD-INFORMATION-NAVIGATION.md).

Release build and 7/7 CTest suites passed on 2026-09-23 after fixing a test-local variable naming conflict. Added vendor native-call flags/null-context and shop/source routing tests. Eight native byte profiles match the reviewed runtime PE. Source mirror synced. No live item sale performed; gold/item confirmation pending.

Installed with D2R stopped. DLL SHA256: 6FD2B9BDA2AC3E6F24E88BFF463F9A272CAD5F124A74120C44EA9C6D5142A609

Installed: <game-root>\mods\Reimagined\d2rloader\plugins\QOL.dll

Golden rollback: <private-workspace>\outputs\QOL-1.5.13-golden.zip
Immediate DLL backup: <private-workspace>\outputs\QOL-native-sell-rollback\QOL.dll
Close D2R before restoring. Configuration/layout unchanged.

See NPC-SELLING.md for ABI, native pricing/packet evidence, and patch recovery.
