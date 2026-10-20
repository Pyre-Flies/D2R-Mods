# Chronicle inner navigation - rev.19

Extend the existing guarded TabBar hook at game RVA 0x878D30; no new hook,
patch, plugin dependency or native widget-binding write is introduced.

Read-only live inspection on 2026-09-27 found visible ChroniclePanel containing
ChronicleTabs. The tab widget's vtable is game+0x1D75E00 and its +0x20 message
slot points to game+0x878D30. Switch-enabled byte +0x16A0 was 1 and left/right
bindings +0x16A4/+0x16A8 were 7/8 (LT/RT). Names (+8), parent (+0x30), and
active/visible (+0x50/+0x51) match the existing Options widget contract.
Discovery reused panel manager game+0x3440170, children +0x58/count +0x60;
those enumeration fields are research-only, not new production dependencies.

Track ChroniclePanel open/close via the existing SDK UI-message listener
(submenu bit 8). Require visible exact ChronicleTabs/ChroniclePanel names,
active remap admission, enabled switching and native 7/8 bindings. Translate
copied LB/RB actions 19/20 to inner 7/8; suppress inner handling of original
triggers so existing outer menu navigation receives them. Other widgets pass
through. Reuse the scoped glyph renderer to show LB/RB in Chronicle, gated
by the same submenu state. No changes to Chronicle Ground Flag are needed:
this is the game's Chronicle UI, not that plugin's ground-label rendering.

Regression checks cover exact names/visibility, lifecycle bit, action mapping,
and left/right glyph scoping with the feature active/inactive. Live controller
behavior remains pending candidate installation and user validation.

Release build and all 19 suites pass, including scoped Chronicle glyph and
menu regression checks and DLL artifact validation. Candidate SHA256:
AA8806A0BDB2A9D21D8825A0A530A3D5CF7BF51AD0B9F102C8F567FB97B1C8FB.
Installation and live navigation validation pending.


Installed rev.19 in the global plugins folder on 2026-09-27 after user closure
and process-absence verification. Rev.18 was backed up. Installed SHA256
matches the tested candidate above. Live Chronicle navigation and indicator
validation remains pending.

