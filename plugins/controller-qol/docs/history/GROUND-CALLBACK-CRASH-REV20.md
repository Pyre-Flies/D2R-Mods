# Ground-loot callback crash - rev.20

## Evidence (2026-09-27)

Two rev.19 crash reports, UTC 13:15:31 (pid 39248) and 13:16:23
(pid 540), record an access violation at game RVA 0x34B461 while reading
address 0x2. RCX and RBX are 0x2. This instruction is inside the existing
GetGame(UnitAny*) entry at game RVA 0x34B440. Static disassembly identifies
Core RVA 0x4944A2 as the return address after its task callback invocation
at 0x4944A0. These are diagnostic witnesses, not new hooks or guards.
The installed Core SHA256 is
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
An older report (2026-09-26 19:11:36, pid 39580) has the same game fault
with RCX=1, demonstrating that this signature predates the Chronicle change.

User reproduction: open Chronicle, press LB/RB repeatedly, switch to Quest,
then press LB/RB. Both attempts crashed. First-chance stack records do not
establish a complete unwound call chain. A second fault at an unidentified
module's RVA 0x1587A remains unattributed; QOL ownership is not established.

## Source defect and correction

ScanGroundTask, RefreshStickySlotsTask and PickupGroundTask used the SDK
getLocalPlayer result as a native player pointer when no native action had
provided one. PlayerHandle is an opaque SDK identifier, not UnitAny*. Values
1/2 fit the observed invalid arguments. Remove all three conversions and skip
when native context is unavailable. Do not substitute another unverified
pointer source. Existing native-action observation remains the context source.

Ground shortcut polling previously excluded the stash but not all tracked
menus. Require no dedicated panel and no tracked submenu before queuing world
loot work. Repeat the check in queued refresh/pickup and native pending-pickup
consumption, so opening a menu after queuing cannot perform that pickup.
Track input edges before skipping and clear pending pickup on menu entry.
Quest, Skills, Options and Chronicle retain their menu remaps.

No new native hooks, code patches, dependencies or native constants are added.
This does not redesign cached native-context lifetime across game sessions.

## Validation

Release build and all 19 automated suites pass, including panel/submenu ground
shortcut policy checks and DLL ABI/version/export validation. Candidate
1.3.1+rev.20 SHA256:
AE0C3BEEC765E15C5AFFECF1F966F6E82A8A96CAFC274C232EB5C764CA932829.
Live crash reproduction and ground-loot validation remain pending.

Installed globally after verifying game and loader were closed. Previous rev.19
was backed up before replacement; installed SHA256 matches the candidate.
User validation remains pending.

