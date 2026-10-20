# Tooltips without range clones and scaling breakdowns, revision31

Target: the existing exact D2R 3.3.93787 / D2RCore admission. This candidate
changes only Item Roll Ranges. It does not change input bindings, consume
buttons, manufacture a native range clone, or write item/stat/table memory.

## Read-only evidence, 2026-10-12

The standalone test game is hosted in D2RLoader PID22916. Game base is
0x140000000; Core base is 0xC0DE5000000. Bounded local observations are kept
under outputs, not committed or distributed as game assets.

- Pain Jack, ID45/class365/quality6, has Loremaster's1 ID1273 with fixed
  Experience2; the fixed-modifier shortcut emitted its source label without
  the light-blue selector. The same path affects of Speed ID319, property78,
  handler8/stat96/value30, on the ID48 rare boots. A single-source row now
  colors the source label explicitly and restores the original color after it.
- Bramble Song, ID52/class69/quality6, has Eagleeye ID951/property151,
  coefficient16, and Trump ID970 whose second specification uses the same
  property/coefficient24. Owned/aggregate stat224 is40. Native metadata is
  operation2, shift1, level basis12. At level100 the contributions are800 and
  1200, totaling2000. Their level1-100 bounds are8-800 and12-1200.
- Shadow Mantle, ID55/class326/quality6, has IDs998/1021/1294/334/133/139:
  Godly, Monk's, Silver-Wrought, of Thorns, of Anima and of Guarding.
  Its owned leaf stats include ED230, physical reduction14, magic reduction5,
  durability17, flat Thorns2 and Paladin skill stat83/layer3/value1.
  Godly ED201-225 and Silver-Wrought ED16-20 yield combined217-245.
  The individual ED rolls cannot be uniquely reconstructed from230; source
  rows retain '?' actuals while the combined230 stays intact.
- Sampling Core's existing TLS index at Core+0x7DF224, thread TLS slots at
  TEB+0x58, and block+0x1840 observed the source ID55 with clone=null while
  Ctrl was held (621 samples). Other sampled tooltips had distinct clones.
  The sampling races the formatter; unrelated/non-item transient pointers
  are excluded from the conclusion. The exact reason Core refuses this
  item's clone remains unestablished; no native selector behavior is altered.

## General fallback and validation limits

The prior PropertiesAdapter required a distinct clone. HeaderBuilderEntry
already observes Core's native keyboard modifier/controller range decision
at the qualified header caller (Core+0x81A360), including items with no clone.
That request is now carried as source pointer identity plus unit ID until the
qualified properties caller (Core+0x81D53B), where it is consumed once.
Each next qualified header clears it, including release. Wrong sources,
reused IDs, unidentified items, unrelated callers, nested rendering and
unsupported panels do not use this fallback. Existing clones still use the
existing overlay source resolution. No physical key polling is introduced.

When observers have no scalar identity, bounded owned leaf stats are rendered
with the existing native Single ABI and active ItemStatCost rows. Only
ungrouped descfunc19 scalars and typed stat83/descfunc13/class layers0-7 are
admitted. Each unique localized numeric shape must match one actual line and
one stat candidate. Function21 class selectors use the already documented
Properties.valN WORDs at+0x0A and target stat WORDs at+0x20; the current class
skill row's descfunc WORD at+0x32 is13. Ambiguous/repeated lines remain native.

Magic/rare scalar bounds require complete source definitions and agreement
with owned totals. Ordinary aggregate values must also agree; previously
documented consumed ED/computed physical damage operands use owned leaves.
This does not infer crafted fixed recipe, enchantment or unknown set sources.
The ordinary socket/corruption model can replace the fallback only after its
existing full ownership and source checks succeed.

Scaling breakdowns require the existing coefficient/active-cap proof. Every
level1-cap must produce the same sum when contributions are truncated
separately or as a combined coefficient. The displayed total must identify
unique component values across all possible levels. Rounding remainders,
ambiguous values or unexplained coefficients keep the combined native line;
they are not arbitrarily allocated to an affix. Non-scaling variable host
rolls can show unknown individual actuals with proven source bounds.

## Socket capacity contract

The optional read-only Core helper at RVA0x3763A0 takes an item in RCX and
returns capacity in AL. It is the same helper called from Core+0x81B25F for
the existing unsocketed header. Its first32 bytes are recorded and checked in
header_profile.h, in addition to the existing exact provider admission.
Core resolves the active bank, base Items row and ItemTypes row, item level,
base cap and inventory dimensions, quality and configured overrides:

- Items bank pointer/count: table+0x15A0/+0x15A8, stride448.
- ItemTypes pointer/count: table+0x1348/+0x1350, stride232.
- Items first type WORD+0x12E; width/height bytes+0x11E/+0x11F; cap byte+0x148.
- ItemTypes capacity bytes+0x18/+0x19/+0x1A, level thresholds+0x1B/+0x1C.
- Core+0x376473 limits area to6 before taking the minimum of capacities.
- Core+0x37658F calls the provider's configured-capacity evaluation at
  Core+0x37F6E0; Core+0x376594 compares its result with the quality fallback.

The game helper at RVA0x36EAD0 was also inspected but is not called: its
level-clamp path can write item level+0x38 when below1. The Core path above
avoids introducing that mutation and follows the displayed mod rules.
Socketable's localized template is resolved through the existing qualified
lookup ABI (Core RVA0x387D60, d2r resource group). Exactly one
native socket count must agree with owned primary stat194. Capacity must be
at least that count and at most6. The resulting suffix is `(Max: 4)`, for
example; it is capacity, not an invented random range. Missing helper/template,
duplicate lines, disagreement, invalid capacity or insufficient buffer space
retains existing text. Unsocketed headers are unchanged.

## Validation

Production adapter fixtures cover fixed-label color, Eagleeye/Trump separate
values and complete bounds, fractional-rounding refusal, absent native
identities, overlapping Godly/Silver-Wrought bounds, mismatched totals,
duplicate text and socket capacity refusal. Request tests cover held/released
and wrong source/ID. Automated suite results and restart confirmation are
recorded separately. All seven suites pass; visible restart rendering is pending.

Revision31 was installed in the standalone global plugin folder with the game
and loader closed on 2026-10-12:
`<game-directory>/d2rloader\plugins\Item Roll Ranges.dll`.
Built and installed SHA256 agree:
`4B36158219AE414A442B4DFA886A40E5F8C15D4DAD617EE6DA2514AD4B325DEE`.
The installed DLL passes the artifact suite. Other DLLs/TOMLs retain their
pre-install hashes. Rollback is
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev30.before-rev31.20261012-143805.dll`
under the game installation; the local receipt is
`outputs/item-roll-ranges-rev31-deployment.json`. Nothing is published.

The subsequent user screenshots confirmed label color and absent-clone
coverage, but rejected the source breakdowns and socket suffix above.
[Revision32 evidence](SOURCE-CAPTURE-REV32.md) records the production flag,
handler and buffer differences that the original fixtures missed, and the
corrected implementation. Those revision31 fixture passes did not establish
visible behavior for the affected paths.
