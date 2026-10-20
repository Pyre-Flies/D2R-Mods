# Native RB ranges - rev.22

## Problem and scope

QOL uses LT/RT for outer menu navigation, making the native RT Show Ranges
binding unusable. Supply RB for the native range-selection query even when
Item Roll Ranges is absent. Keep its detailed-formatting integration intact.

## Evidence and interception decision

Verified current D2RCore file SHA256:
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2.
Current Item Roll Ranges provider_profile.h and live read-only inspection on
2026-09-27 agree on Core+0x819949 setup / +0x819954 indirect call /
+0x81995A return. RCX=input object, EDX=selected controller index,
R8D=0x800 (native digital RT). Indirect slot Core+0x6FE470 originally resolves
to game+0x13CA70, the existing QOL native input profile's button query.
The live slot was owned by Item Roll Ranges while the original game query
remained unmodified. Full caller witness:
48 89 F1 89 C2 41 B8 00 08 00 00 FF 15 16 4B EE FF 90 C7 85 EC 18
00 00 00 00 00 00 E9 CF FC FF.

Do NOT patch that call or its mask immediate: Item Roll Ranges validates
those bytes and scopes PadAdapter by the exact return address. Do NOT replace
or wrap its shared slot: a normal nested call changes the return address its
adapter sees. Instead add an SDK-owned inline hook at the native game query,
qualified by that original tooltip return address, mask 0x800, valid index<8,
and active QOL trigger-menu remap. Change only that argument to 0x200 (RB),
then invoke the original native query. ABI is bool __fastcall(void*,unsigned,
unsigned), independently used in controller_input.cpp. All other callers
and masks pass through, including native bridge state seeding.

Admission: exact Core hash, full caller bytes above, full existing ButtonBytes
in native_input_profile.h, then SDK inline hook with the first 15 bytes:
48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20.
Install after native input admission, so our own hook cannot invalidate its
initial query fingerprint. Disable on navigation shutdown and console
input/menu off; retain the original binding on admission failure. The hot
query path uses only atomic enable state and immutable addresses, taking no
navigation/input mutex (avoids input-to-navigation lock inversion).

With Item Roll Ranges: its scoped PadAdapter answers the tooltip query itself
without forwarding to the native query, so the detailed range request and
formatter state remain its responsibility. Its call site, slots and guard
bytes stay untouched. When it passes an unrelated query through, the new
native hook does not match its return address. This applies in either load
order and when its adapter is inactive; visible validation is still required.
QOL alone: native query receives RB and the native formatter renders ranges.
No dependency on Item Roll Ranges or physical XInput is introduced.

## Header and limitations

Within the exact rev.21 ControllerOverlay legend ancestry, replace the native
E00E + Show Ranges text with action glyph E028 + Show Ranges only when the
range hook and menu remap are active. It uses the same original rectangle,
style and scale. English text is matched exactly to avoid changing unrelated
RT legends; translated or truncated labels fail open. No spacing change in
this revision. Existing QOL shortcut additions retain their prior layout.

This adds ONE shared native query entry hook, scoped by caller. Another
plugin owning that same inline-hook entry can still prevent admission;
unknown owners are not overwritten. The source file/byte guards must be
revalidated after a provider update. No shared Core data slot is claimed.

## Validation

Live read-only query entry and Core caller bytes matched before editing.
Release build and all 19 suites pass, including exact caller/mask/index gates,
inactive/other-caller pass-through, header text qualification and DLL ABI.
Automated tests do not prove visual ranges or runtime plugin ordering.
Test QOL alone and QOL plus Item Roll Ranges: hold/release RB over an item,
confirm ordinary/detailed ranges respectively, and verify LT/RT still navigate.
Also verify Quest/Skills/Options/Chronicle bumper navigation remains normal.

Candidate 1.3.1+rev.22 SHA256:
60ED6A9202331C8EFFD5BD5AF04F8CB734FFBE838393ABAB420BAD7383195186.

Installed globally after user closure and process-absence verification.
Rev.21 backed up; deployed hash matches the candidate. Live checks pending.
