# Controller-only labels - rev.14

The label lock previously checked feature enablement, LB configuration, installed
hooks, in-game state and channel 0, but not active input mode. Consequently it
consumed keyboard Alt through the shared native press/release functions.

LabelLockActive now also uses the existing ControllerQoL::IsControllerUiActive
reader: admitted native game profile, manager pointer game +0x3440170, manager
+0xDC == 1, active controller index game +0x2A23704 < 8. This is the same reader
already used to hide controller inventory shortcuts in mouse mode. An unknown
reader returns false; controller connection alone does not qualify. No new RVA,
hook or native call introduced.

Every enforcement entry uses LabelLockActive: press/release interception, UI
message enforcement, explicit LB overlay refresh, and periodic rev.13 recovery.
Keyboard mode therefore forwards native Alt actions and performs no label writes.
Switching mode does not restore a saved keyboard label preference or overwrite a
native keyboard action; the current label state is left to native controls.
The existing game-thread periodic callback now calls EnsureLabels before recovery,
so returning to controller mode also restores a filtered flag left OFF by Alt.
Healthy flags still avoid native refresh. Unknown/inactive mode fails open.

Tests cover controller/keyboard ownership transitions, unfiltered channel and
out-of-game exclusions. User confirmed rev.13 repaired the intermittent display
issue. Rev.14 input-switching behavior awaits in-game validation. Personal Stash
withdrawal remains unverified from rev.12.

2026-09-27 deployment: user closed game; process absence verified. Installed
1.3.1+rev.14 in the global d2rloader/plugins folder. Built and deployed SHA256:
2DE44E383FB3ECECB62BE9B6929FE67AD991A2A1BBCE07E4B87A0F67009FCDB8.
Previous rev.13 DLL backed up under before-rev14-20260927-081018.
All 16 suites pass. Mixed-input behavior remains pending live validation.
User also confirmed the Personal Stash withdrawal fix works correctly.
