# 0.4.7: contribution layout and native unique scalar bounds

2026-09-24, same qualified Core1.3.1-beta / D2R3.3.93787. Read-only PID37712;
game base0x140000000 and Core base0xC0DE5000000. Controller QOL unchanged.

## Visual ordering

The user's0.4.6 screenshot confirms native property strings are displayed in
reverse line order: the last emitted contribution appeared above the first,
and the combined total appeared below both. Multi-source blocks now emit
reversed children followed by the total. On screen this places the combined
total first, then its contributions in source order. Full child lines use native
color5 (gray), and reset to3 afterward; a single-source line keeps prior styling.
No draw hook or change to unrelated property ordering is introduced.

## Loaded Renewed Black Cleft definition

Unique record437, internal name Crafted Black Cleft, flags8. Existing tables:
bank3 table from D2R+0x2A9A580+3*16; unique pointer/count at table+0x13C8/+0x13D0,
stride348; definitions from row+152, twelve16-byte entries. Properties pointer
table+0x240, stride48; group pointer table+0x258, stride200. These were already
qualified and are reused, not re-created from an external chart.

| Definition | Selected property on this item | Verified bounds |
|---|---|---|
|property274|stat193, Sunder|300..300, fixed|
|property37|stat37, Magic Resist|-45..-45, fixed|
|group7, property278|stat358, Enemy Magic Resistance|5..10 internally; native template renders -10%..-5%|
|group13, property59|stat80, Magic Find|14..25%|
|group19, property13|stat7, Life|10..65 display units|
|group25, property76|stat96, Faster Run/Walk|5..10%|
|group31, property3|stat34, Damage Reduced|5..10|

Every listed group has mode2 and the containing unique spec has min=max=1.
Entries carry fixed parameter0 and positive weights. Alternative entries target
different stats; the observed tooltip identifies the selected stat. We do not
label an absent alternative or call random-selection code. The linked Maxroll
page was unavailable to the browser (robots restriction); no limits were copied
from it. All numbers above came from the loaded game tables.

## Native semantic proof and patch landmarks

* D2R+0x3D4EA3..0x3D4EEA dispatches property-group mode2 to+0x3D18F0.
* D2R+0x3D1949..0x3D1975 reads the containing spec's min/max as selection
  count. Equal1 selects one weighted entry. Existing+0x3D1A31..0x3D1A7F
  copies that entry's property and bounds into the applied spec.
* Property function1 is D2R+0x3CF960. +0x3CF9D7..0x3CF9E6 reads spec+8/+C
  as roll bounds and invokes+0x3D5860.
* Function8 is D2R+0x3D0EC0 (used by FRW). +0x3D0F07..0x3D0F21 similarly
  supplies the spec bounds unless an already-computed value is supplied.
* Both delegate the resulting scalar to D2R+0x3D5940. Its+0x3D5A0C..0x3D5A2A
  applies ItemStatCost+0x14 fixed-point shift when storing. Life's shift is8:
  stored10496 means41 display units. Definition limits10..65 are already in
  display units, so passing2560..16640 to the display formatter would be wrong.
* Existing SingleFn D2R+0x2D6520, descfunc19 branch+0x2D732E..0x2D7347,
  formats the supplied integer through the native positive/negative template.
  This provides the correct sign for enemy resistance without English matching.

`tools/audit_sunder_ranges.py <pid>` reads the unique/group/property metadata.
`--generate` records reviewed witness windows in `src/unique_range_profile.h`.
These additional witnesses gate activation together with existing profiles.
The research handlers above are NEVER invoked by the plugin; only the existing
read-only SingleFn formatter is reused to render endpoints into local buffers.

## Range implementation and exclusions

Unique source expansion retains whether each scalar definition came directly
from the unique or through verified mode2/exactly-one groups. Other group modes
remain eligible for source labels but not numerical bounds. A numeric fallback
requires one contributing definition, property handler1 or8, ungrouped layer0,
and stat description function19. Fixed limits are intentionally omitted.

The plugin formats low and high through SingleFn using the original item and
read-only native row. It requires matching normalized text identity, exactly
one integer in each endpoint and actual line, and the actual value within the
resulting bounds. Inverted displayed endpoints are sorted. Multiple contributors,
unsupported parameterization/handlers, paired numbers and grouped lines fail
closed. Existing native ranges take precedence; fallback is used only where
none is present. No range clone, stat mutation or RNG replay is performed.

This supports the current Sunder's five variable lines without inventing unique
tiers. It is not universal support for every unique/set/grouped property.

## Validation

All five CTest suites pass. Tests cover reverse buffer/display order, gray child
colors with blue reset, signed enemy resistance, FRW, Life display-unit bounds,
fixed/incorrect/out-of-range endpoints, native-range precedence, exactly-one
group eligibility, overlapping-source rejection and native formatter invocation.
Existing ABI/slot ownership tests also pass. Real visual verification is still
required after installation, particularly child ordering/color and Sunder bounds.
