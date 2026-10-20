# Rev.52: skill isolation and explicit aim intent

2026-10-03. Reuses the admitted rev.50/51 executable and D2RCore profile;
no new RVA, native entry point, offset, signature or ABI is introduced.

Inspection found idle `UnitTestSelect` observations opened the same custom
PointScore scope as active casts. The original enumeration is stateful and owns
native selection caches, so idle geometry replacement can affect subsequent
skills even though Selected/Lookup guard their requested skill. Rev.52 calls the
original enumeration once and leaves PointScore unchanged while idle. Candidate
copies may still feed the overlay, but native idle rejection is respected.
This deliberately limits idle preview coverage; independent arbitrary enemy
enumeration/eligibility is not claimed or implemented.

Custom geometry is admitted only for an enabled active snap skill after radial
right-stick intent. Lookup and Selected require an armed, enabled active skill;
Selected additionally matches the requested skill. Disabled/unlisted active
skills clear the copied candidate book, retained lock and recent lookup/marker,
and disarm manual aiming. The existing local controller cast observer also
catches unsupported casts too short for a UI tick when their copied player ID
matches a local controller observation from the last 250 ms. Cast's source/thread
remains unclassified; it calls no new native getters and retains no unit pointers.
Other player IDs cannot reset that state. A controller-to-mouse switch within
that short window may reset the dormant manual intent, but never rewrites the
mouse cast. A held stick must return inside
the dead zone before rearming after such a cast. Lifecycle and F8 reset intent.
Guided Arrow's legacy correction yields only while installed, enabled, in-session,
armed and Guided Arrow is enabled in the per-skill settings.

Class headings contain stable built-in numeric ID toggles with name comments,
never character-class checks. Legacy name keys remain readable for migration;
mixing a name and ID for the same skill is rejected as a duplicate.
Custom entries declare additional numeric IDs and ground/snap/disabled mode;
bounded storage holds ten built-ins and 32 additions. No names are discovered
automatically, no active mod table binding is guessed, and custom modes do not
establish native skill coordinate compatibility. Configuration assignment is
transactional; malformed values, duplicate IDs/sections, unknown aim headings,
unknown names and out-of-bound custom IDs disable only aim.

Validation: all 23 QOL suites passed, including automated configuration, routing,
idle-score policy, bounded custom entries and the DLL ABI/artifact test. These
do not prove live native call order, immediate skill transitions, idle marker
coverage or any mod-added skill. Live validation remains pending.

Local deployment: confirmed no D2R/loader processes, installed rev.52 at
`<game directory>/d2rloader\plugins\Controller QOL Updates.dll`.
Built and installed SHA256 match:
`E086A29BE8654E6E4451E2260CB63F022492D80963413D0DC653411013AE71B8`.
Prior DLL and TOML are preserved in ignored
`local rollback controller-qol-rev52-20261003-060857`.
Appended the class/custom sections to the deployed TOML; preserved deadzone .22,
initial speed 25, maximum speed 100, acceleration .05 and all other prior tuning.
Python TOML parsing also confirmed the deployed file is valid and the illustrative
custom IDs remain commented out. This is installation evidence, not live validation.

Numeric-config refinement, 2026-10-03: updated source and deployed class keys to
quoted numeric IDs with name comments, preserving all enable states and tuning.
Class boolean keys still require one of the ten supported IDs; other vanilla/mod
IDs require an explicit ground/snap/disabled mode in `[aim.custom]`.
All 23 suites passed again, including numeric/legacy duplicate detection and
embedded numeric config checks. Game/loader were closed for replacement;
updated built/deployed SHA256 is
`AA9D1402423A933821514204645CC72F1FF4B060B40A99B7DC01A7896DBD44E5`.
Previous rev.52 DLL/config preserved in ignored
`local rollback controller-qol-numeric-config-20261003-061547`.
Deployed TOML parses successfully. Live validation remains pending.
