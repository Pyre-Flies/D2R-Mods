# Controller item header - rev.21 candidate

## Observed path and scope

Read-only inspection on 2026-09-27, current D2RLoader 1.3.1 / SDK 0.3.0,
found ControllerOverlay > Anchor > legendBG > Legend. The active mod's
controlleroverlayhd.json names Legend as ButtonLegendScrollWidget with eight
subText templates, 360x103 logical pixels each within a 2880x103 strip.
Production does not modify assets or depend on those dimensions.

Live Legend vtable: game RVA 0x1D76550. Draw slot +0x18 is game+0x87AB30;
its loop draws embedded Text widgets at Legend+0x680, stride 0x230. Each
embedded widget's parent (+0x30) is Legend, name (+8) is Text, and draw entry
is game+0x86D410, which QOL already intercepts. Thus no new inline hook or
call-site patch is needed. Exact full ancestry is required for header work;
other widgets, panel labels and other plugins' unrelated legends pass through.

Runtime Legend+0x88 points to 0x40-byte entries, count +0x90. Entry label
strings identify Pick Up, Drop (Hold to Move to Shared Stash), Compare,
Open Cube, Hold to Sort, Show Ranges, Close Menu in the inspected state.
These offsets are research-only. Observed leading glyphs:
E00F=A, E011=X, E012=Y, E008=L3, E00A=R3, E00E=RT, E010=B.
Native runtime widget text can already be horizontally scrolled/truncated;
classify only known leading glyphs, never English substring matches.

## Implementation

The existing SDK ActionFooter item-tooltip callback publishes a copied,
mutex-protected four-entry presentation snapshot using the SAME computed
availability predicates as its tooltip. It includes configured modifier,
Identify/To Belt, Transfer/Sell, To Cube, and potion Fill Belt/Refill-Buy.
No item handle or native pointer is retained by the header snapshot. Clear
on a new callback; expire after 250 ms to avoid stale hints after focus loss.
Rendering additionally requires controller UI mode and the existing glyph
hook admission. Disabled actions and non-potion refill hints remain absent.

For an eligible A/X/Y/R3 entry, draw the existing native text in the top half
of its original rectangle and the QOL chord beneath it. Other entries retain
native rendering. Use the game's controller glyph renderer, so Xbox and
DualShock artwork follow native device selection. Text buffers, widget
rectangles, bindings, native legend entries and gameplay inputs stay untouched.
The native Show Ranges RT entry is not remapped by this change; Item Roll
Ranges' separate RB behavior is not assumed to be installed/active by QOL.

Fit both lines with the native text-measure helper at game+0x903CD0:
uint64_t __fastcall(const char* utf8, void* style, float scale,
                   const int* widthHeightLimit).
Packed return is low32 width, high32 height. Its existing caller at
0x86D508 supplies this ABI, then compares eax and rax>>32 to the limits.
Live bytes match the static witness and production admission guard:
48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 50 0F 29 74 24 40
49 8B F9 0F 28 F2 48 8B F2 48 8B D9.
Measurement and draw execute synchronously on the native rendering thread.
Reject invalid rectangles/results or excessively small fitted text and retain
native output. A guard mismatch disables only header additions. Existing
scoped draw call patches at 0x86D6A9 and 0x86D6F3 remain unchanged.

## Validation and limits

Release build and all 19 suites pass, including exact header ancestry,
unrelated widget rejection, glyph classification, snapshot expiration and
DLL artifact validation. Runtime ancestry/vtables/glyphs/measurement bytes
were inspected without changing game state. No remote calls were made.
Visual two-row spacing, tooltip update freshness and all vendor/stash header
variants still require live testing. A shortcut is added only when a matching
native A/X/Y/R3 entry exists; this iteration does not synthesize missing slots.
Localization of original text is preserved; new labels follow QOL's existing
English tooltip labels. No claim of visual validation yet.

Candidate 1.3.1+rev.21 SHA256:
667977C44F1326B97F77F2C20894764136B21D511D8C6EF201F8964D67812F60.
User confirmed rev.20 cleaned up the Chronicle-to-Quest crash reproduction.

Installed in global plugins after user closure and process-absence check.
Rev.20 backed up; installed hash matches the tested candidate. Live visual
validation is pending.
