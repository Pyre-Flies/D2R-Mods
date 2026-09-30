# ReimaginedLadder compatibility investigation - rev.12 candidate

## Evidence and scope

2026-09-26 global Controller QOL rev.11 with mod-scoped Charm Inventory 1.0.1,
BNetSimulation and other ReimaginedLadder plugins; global Chronicle Ground Flag,
Stash Search and Potion Auto Pickup. Canonical source is this repository.
Core SHA256: 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Global QOL logs, rather than the mod log directory, contain its transfer evidence.

## Personal Stash wrong-item transfer

At 22:11:57 and repeatedly afterwards, source container 7, cell (5,0), code
0x2077656A reported a stale UI handle followed by resolution by position.
ExecuteMoveTask enumerated PersonalStash, SharedStash, CustomPage and Cube together
and accepted the first overlapping cell without checking source or runtime ID.
This directly permits the reported Charm Inventory selection. A same-code fallback
was also unsafe. A visible custom-page binding additionally claimed transfers
while the standard stash was open (to-custom log entries at 22:12:11 and 22:12:25).

Candidate changes: carry a coherent full ItemInfo from UI focus into queued work;
require runtime ID, code, container, inventory page, coordinates and Shared page
where applicable. Stale-handle recovery enumerates only the source container and
rejects incomplete scans. Stash/Cube/vendor contexts precede CustomPage routing.
Cube queue now also carries the identity rather than rebuilding it from independent
atomics. Existing native custom-page, advanced storage and Shared routes remain.
No new native hooks or addresses are introduced by the transfer fix.

Validation: Release MSVC build and 16 CTest suites pass, including colliding
container/cell/code, reused handles, failed scans and custom-page route precedence.
Live candidate transfer behavior is not yet verified; rev.11 remains installed.

## Label investigation - unresolved

Read-only live capture PID 35612: game base 0x140000000, Core base 0xC0DE5000000.
Game +0x235D360 points to 0x14235D378; +0x235D368 count is 2;
filtered/unfiltered bytes are 01/00. Press +0xC66A0 and release +0xC6E90
still branch through MinHook relay destinations into the loaded QOL module.
Native setter +0x1FAE90 and refresh +0xCE450 are unchanged at their entries.

Core IsInGame export +0x1F99B0 reads singleton pointer +0x700480 through
+0x1F9A10 and tests singleton +0x2E0 for non-null; both observed non-null.
Thus the observed state does not support a lost hook or false in-game predicate.
Refresh +0xCE450 calls aggregate-label query +0x1FAD80, tests controller mode
+0x77E10, obtains game TLS context through +0x144640, and assigns context +0x1F60
from controller eligibility +0x13DE60. This is disassembly evidence only: the
render-context flag was not directly sampled and no new callable contract is
admitted. More visible reproduction evidence is needed to select a label fix.

Other startup refusals: belt +0x15F660 and portal +0x349860 guards saw modified
bytes. They are separate disabled features; guard bypass is not a label solution.

## Options submenu navigation

User confirmed desired LB/RB for Video/Audio/Gameplay categories, LT/RT for outer
menu navigation. Read-only live inspection PID 3604 identified visible
SettingsPanel -> OptionsTabs -> TabLeftIndicator / TabRightIndicator. OptionsTabs
vtable game +0x1D75E00, slot +0x20, points to the existing hooked TabBar handler
+0x878D30. Its switch-enabled byte +0x16A0 is 1 and actions +0x16A4/+0x16A8 are
7/8. Widget name pointer +8, parent +0x30, visibility +0x50/+0x51 match the
existing scoped glyph contract. No new detour or mutable native binding is used.

Discovery path: FindTopLevelPanelByName +0x846170 loads manager from game
+0x3440170 and forwards to +0x89F760. Manager children vector +0x58/count +0x60
held the active panels; deferred lists +0x88/+0x90 and +0xA0/+0xA8 were empty.
FindChildWidgetByName +0x856220 confirms child vector and name layout. These
manager offsets are research evidence only, not added runtime dependencies.

SettingsPanel open/close lifecycle sets a separate submenu bit. The existing
TabBar hook requires exact OptionsTabs/SettingsPanel names, visible widget/parent,
and native 7/8 tab bindings. It gives this widget copied messages with 19/20
(bumpers) mapped to 7/8, and releases original 7/8 triggers to the outer menu.
No shared input is changed. Settings glyph replacement is active only with this
submenu bit. Other settings widgets and unrelated tab bars remain native.
Existing signature guards and submenu admission conditions still apply.

Release build and all 16 suites pass after this addition. Tests cover the names,
hidden/unrelated widget refusal, action mapping and scoped Settings indicators.
Actual controller navigation remains pending user validation after installation.

Installed candidate in global d2rloader/plugins after process-closed verification. SHA256 E0C0CF43C6E2E46E231F5877491EBEC96F669F931DA5C1CEE11B50D9927234C3 matches build and deployed DLL. Previous DLL retained in local backups/before-rev12-20260926-232124. Not published; live behavior pending.

2026-09-27 user validation: Options inner LB/RB and outer LT/RT navigation work.
Personal Stash withdrawal remains untested. User reports intermittent missing
labels for a set Military Pick and magic Bone Shield; pressing LB restores them.
Source inspection confirms both LB press (RefreshStickySlotsTask) and release
(ClearPlacardsTask) explicitly call RefreshGroundLabels, which forces native
setter +0x1FAE90 and refresh +0xCE450 even when the filtered flag is already ON.
Ordinary EnsureLabels skips refresh while that flag is ON and no refresh is
pending. This supports a stale display/refresh hypothesis but does not establish
what invalidates the display, nor exclude filtering/label-cap interactions.
Read-only PID 34552 capture still showed filtered/unfiltered 01/00 and installed
label hook entries; it was not a confirmed capture during the missing-label state.
Awaiting release-behavior detail and a reproduction left in the failing state.

2026-09-27: User confirmed Personal Stash withdrawal works correctly. This supersedes the earlier pending stash validation entries.
