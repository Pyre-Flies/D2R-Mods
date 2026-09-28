# Controller QOL Updates 1.3.1+rev.48

## Runes embedded-Cube routing

The Reimagined Gems, Materials and Runes pages share an embedded Horadric Cube
grid. Rev.47's policy admitted inventory LB+X to the advanced-storage-first,
Cube-fallback route only for selected tabs 2 and 3. Tab 4 therefore fell into
ordinary stash routing: native smart deposit accepted runes, while the Runes
page correctly rejected non-runes instead of reaching the Cube.

Rev.48 extends only `UseMaterialsRoute` to tab 4. Inventory LB+X now first asks
the existing native advanced-stash predicate and deposit helper. Eligible runes
retain native Rune storage. A noneligible item continues through the existing
SDK Cube transaction. A failed eligible deposit still stops without spilling
into the Cube or Personal Stash.

Cube-source moves, advanced-counter withdrawals, ordinary Personal/Shared
stash behavior, LB+Y, controller navigation and Guided Arrow are unchanged.
No hook, RVA, byte guard, packet, layout offset or native ABI was added or
changed.

## Validation boundary

All 21 automated Controller QOL suites pass, including materials routing,
Guided Arrow isolation and DLL ABI/export/version checks. The built and deployed
DLLs are byte-identical with SHA-256
`E4D2452B0B9E8BB4BD121E25A903254346B0FCE0B609AE19734C67785FC937D4`.
Rev.47 was preserved as `Controller QOL Updates.rev47.rollback.dll.disabled`
with SHA-256
`BCD569DF40630A6AC34F81549E5A17C6DD52D1732D17BD6B15527D89A3DA928A`.

The user confirmed the installed rev.48 behavior in game, including the merged
Guided Arrow correction and Runes embedded-Cube routing. This live result is
specific to the installed build and does not qualify unreviewed D2R builds or
different advanced-stash layouts.
