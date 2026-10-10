# Save & Exit / Multi Shot follow-up - 2026-10-10

## Latest exit crash

The user confirms rev.74-beta.1 Warp works, reports Multi Shot only snapped after
an R3 off/on cycle despite both cursor and lock reticles being visible, and
reports a Save & Exit crash. These observations do not establish one shared cause.

Last-resort report PID 61976 was generated at **12:15:18.577 UTC / 08:15:18
America/New_York**. The report filename's 12:12:26 timestamp is startup time.
Windows dump `D2RLoader.exe.61976.dmp` confirms exception `C0000005` at
**nvwgf2umx.dll +0xA329C2**, rather than inside QOL or the earlier game cleanup
fault sites. The module's dump version is **32.0.16.1692**, image size 90284032;
the still-installed driver file SHA256 is
`670A1C125F8C3851A1AE7D23C0390CD879E5B9D0A423D99EDDFF37A8E2774BFF`.

Fault bytes from the dump: `8B 7C DE 08 44 8B C7 41 C1 E8 1C 45 85 C0 74 4F`.
The first instruction is `mov edi,dword ptr [rsi+rbx*8+8]`. RSI=0x0D980FD0,
RBX=0x8C80E28A, yielding the recorded unreadable address 0x4719F2428. The
E28x-looking register fragment resembles earlier invalid-value patterns but
is not proof of the same corrupting write or owner. No native patch is proposed.
The first 2 KiB of raw stack candidates are mostly NVIDIA addresses, plus ntdll;
this is not an unwound stack and does not exclude earlier plugin corruption.
Global Chat's game-ended event is at 08:15:18.556, 21 ms before the exception.

Dump identities: QOL version 1.3.1.74, loaded from the Ladder plugin folder;
matching installed SHA256
`8F94C535E73140ED4B3042009513161B248437919CBEC1F184DF45A5661366D4`.
D2RCore version 1.3.1.0 / SHA256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Build identity/layout limits remain those in
[GENERAL-SKILLS-OSKILLS.md](GENERAL-SKILLS-OSKILLS.md).

The dump explicitly locates Supporter Portals and Trade Notifications in the
**global** `d2rloader/plugins` folder. Renaming only the Ladder copies did not
isolate them. The user subsequently disabled the global copies; read-only disk
inspection confirms `.dll.disabled` names and no active DLL names for those two.
No disabling action was performed by this investigation. A new run is needed to
establish behavior with both absent from the loaded module list.

Other dumps from today are different failures:

- PID 43128, 08:05:58 dump: `C0000409` in
  `d2rl-supporter-portals.dll +0x54645`, matching the previously recorded
  Supporter Portals fast-fail signature. This predates rev.74 deployment.
- PID 57576, 07:55:36 dump: `C0000005` at game/loader +0x29394C;
  raw stack includes +0x2E08BC. Neither is a newly admitted call/hook.

The NVIDIA fault alone does not establish a driver bug; likewise the separately
observed Supporter Portals fast-fail does not prove it caused the NVIDIA failure.
No crash fix or clean-exit validation is claimed.

## Multi Shot startup state

User explicitly confirms both aim and snap reticles appeared before toggling.
This rules out the simple explanation that the stick was never used, but not
an active-skill transition or a mismatch between preview and actual routing.
The user cannot confirm whether the stick was moved again after the R3 cycle.

Semantic comparison of the pre-test backup and current Ladder TOML finds:
Multi Shot ID12=true, no ID12 targeting override, snapping_enabled=true in both.
The built-in catalog assigns ID12 snap mode. UI status and routing share the
same LiveSkills instance; no independent startup registration table was found.
R3 off/on also clears snap observations/markers and may clear the preview ID,
so recovery after toggling is not proof that configuration failed to load.
The existing quiet log has no per-cast diagnostics to discriminate these paths.

Preserved the relevant config/logs/report and compact, explicitly non-unwound
dump summaries under ignored `outputs/crash-review-20261010/`. No dump, binary,
local log or proprietary input is committed. With no game process running,
changed **only local aim.verbose=false to true** for one diagnostic run;
backup `controller-qol-updates.toml.before-multishot-trace` in that directory.
No DLL, shipped default, aiming policy, or other setting changed.

Requested sequence: fresh launch; right-stick aim and multiple Multi Shot casts
before touching R3; if failing, off/on and repeat; Save & Exit with the two global
plugins disabled. Compare requested skill, native selected-unit suppression,
lookup target/destination and R3 timing. Existing cast observer output remains
source-unclassified and may not expose authoritative remote cast packets.
Restore local verbose=false after capture, preserving any user R3 changes.
Evidence from that run is pending; no speculative targeting patch was made.

## Fresh-launch follow-up, 08:22-08:26 Eastern

User reports the Multi Shot issue no longer reproduces. Preserved verbose trace
`outputs/crash-review-20261010/multishot-startup-0826.log` contains **zero R3 toggle
events** and four skill-12 monster-target lookups. At 08:25:02.702 lookup 2 chose
monster 50; lookup destination (5645.50,4857.50) was followed by cast request
(5645,4857), temporal comparison snapped=1, age 15 ms, error 0.71 tiles.
This supports startup snapping without an R3 workaround on this run. Lookup 1
at 08:24:38 had no selected target and used the ground cursor; the trace does not
establish that an eligible enemy was present then. Do not treat normal ground
fallback alone as reproduction of the earlier symptom. No targeting fix made.

Restored **local aim.verbose=false** from the current post-test configuration;
semantic comparison verifies only that flag changed, preserving the user's
current skill toggles. The prior failure remains unreproduced, not proven fixed.

Two new Windows dumps, PID3136 (08:22:28 dump timestamp) and PID56048 (08:26:24
dump timestamp), show **C0000409 at d2rl-reimagined-online-count.dll +0x2CFA5**.
The latter dump confirms Supporter Portals and Trade Notifications are absent.
The newly faulting plugin loaded from the **Ladder** plugin folder, image size
364544, zeroed version fields, installed-file SHA256
`ABBF3DC0A8C9DF8CDCFDE852C896D9959EAFF6D5D13B49FBFD327AB64C599C5A`.
Instruction bytes at the fault are `CD 29` (`int 0x29`), with RCX=7, a fast-fail
termination. Raw stack candidates include plugin RVAs 0x2CD4A and 0x6CDF;
these are not unwound frames or admitted native functions. No new last-resort
text report was generated for these dumps. Do not equate C0000409 alone with
proof of a stack-buffer overrun or attribute the earlier NVIDIA AV to this DLL.
The online-count plugin is the next concrete isolation candidate; it was not
disabled or modified during this follow-up. No clean-exit/crash-fix claim follows.

## Online Count isolation, 08:30 Eastern

At the user's request, with no D2R/D2RLoader process running, renamed
`d2rl-reimagined-online-count.dll` to `.dll.disabled` in both the global
`d2rloader/plugins` and `mods/ReimaginedLadder/d2rloader/plugins` directories.
Both files were 338944 bytes and matched the SHA256 recorded above. Verified
that both active `.dll` paths are absent and both disabled files retain that
hash. No other plugins or configurations changed during isolation.

Preserved separate copies and a manifest under ignored
`outputs/crash-review-20261010/online-count-isolation-20261010T123048Z/`.
A fresh launch and Save & Exit remain necessary to test whether the fast-fail
recurs; disabling the faulting module does not establish the original cause.

## Further exit crashes and Kill Tracker isolation, 09:29–09:39 Eastern

Five new Windows dump summaries are preserved under the same ignored output
directory. PID 62692 (09:29:25 dump write), 37692 (09:34:01), 28052 (09:35:13)
and 43156 (09:37:38) all report C0000409 at
`d2rl-kill-tracker.dll +0x64F05`, with RCX=7. Raw stack candidates include
Kill Tracker +0x62286 and +0x1642F; these are not unwound frames or admitted
native functions. Supporter Portals, Trade Notifications and Online Count are
absent from all five dump module lists. Kill Tracker and QOL are present.

The latest dump, PID 55224 (09:39:39 dump write), instead reports C0000005 at
`nvwgf2umx.dll +0xA329C2`, the same fault location as PID 61976 above.
Its module records identify NVIDIA 32.0.16.1692, image size 0x561A000;
QOL 1.3.1.75, image size 0xD7000, and Kill Tracker (zero version fields),
image size 0xB4000. Both plugins loaded from the Ladder plugin directory.
RBX=0xE28B80E2. Global Chat records game-ended at 09:39:32.927; the QOL log
identifies rev.75-beta.2. These observations do not prove a shared cause or
exclude prior corruption by QOL or another module. No speculative native patch
or exit-crash fix is included with the reticle styling change in beta.3.

At the user's explicit request, with the client/loader closed, backed up and
renamed both global and Ladder `d2rl-kill-tracker.dll` copies to `.dll.disabled`.
Both match SHA256
`3E7A0382801484F8719D86B8B9E4DD413017E1A162ED8BD4C1858795F4687D44`.
Verified active filenames absent and disabled hashes unchanged. Backups and
manifest are under `outputs/crash-review-20261010/kill-tracker-isolation/`.
The next fresh-launch Save & Exit test will evaluate this isolation; it is not
evidence yet that the NVIDIA failure is resolved.
