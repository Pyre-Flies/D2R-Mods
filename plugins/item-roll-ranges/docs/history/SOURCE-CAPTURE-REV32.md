# Revision32 source capture corrections

The revision32 socket-label interpretation below was disproved by the user's
live screenshot. See the revision33 correction at the end for the actual key
and header implementation; the old ItemStats1p insertion is removed.

Target: the existing qualified D2R 3.3.93787 and D2RCore provider profile.
All addresses below are module-relative and specific to that admission.
No input hooks, Item SDK dependency or gameplay writes are introduced.

## Live evidence and rejected fixture assumptions

Revision31 screenshots confirmed fixed light-blue source labels and range
coverage on Shadow Mantle without a native clone. They disproved the tested
Eagleeye/Trump and Godly/Silver-Wrought breakdowns and socket capacity suffix.
Read-only captures on 2026-10-12 establish the following corrections:

- Bramble Song's root stat list has flags `0xA0000000` at list+0x1C, whereas
  Beast Shell and Shadow Mantle have `0x80000000`. The original source capture
  rejected Bramble's flags before constructing any scaling components.
- Both observed flag values select the extended aggregate vector. Core's
  accessor at RVA0x3E839B uses a signed test of list+0x1C to choose primary
  vector+0x30 or aggregate+0xA8. The already guarded WideCopy path at
  Core RVA0x832940 likewise uses the extended layout. The extra bit's meaning
  is not established. Admission accepts only these two observed values;
  unknown flags, broken ownership/parent links and malformed vectors refuse.
- Active Properties records are stride0x30, not0x40. The initial stride0x40
  probe produced invalid metadata and was discarded. Corrected typed reads
  establish ac% property5: function2/stat16; dur property51: function1/stat73;
  thorns property54: function1/stat78; dmg/lvl property145: function17/stat218;
  att/lvl property151: function17/stat224; thorns/lvl property165:
  function17/stat238. No extracted table rows are distributed.

Source layout remains unit+0x88 -> root; root+0x90 -> owned leaves; leaf+0x78
must equal root, leaf owner must equal the source unit, and scalar leaves
retain flags0x40 and expiry-1. Vectors retain signed low-DWORD values, sorted
keys and bounds. Attached sources use the same two admitted extended flags
and retain root-owner, bank and insertion-order checks.

## Source records and expected text

Bramble Song has Eagleeye's coefficient16 on stat224. Trump has two property
specifications: coefficient8 on stat218 first and coefficient24 on stat224
second. Its owned/aggregate stat224 coefficient is40. Active stat224 operation
metadata is op2, shift1, base12 (character level), ValShift0. At level100 the
sources contribute800 and1200, totaling2000. Their bounds are8-800 and12-1200;
combined bounds are20-2000. Revision31's fixture used root flags0x80000000 and
put Trump's attack-rating specification first, missing the live capture refusal.
The corrected fixture follows the live flags and specification positions.
Existing rounding/uniqueness proof still gates splitting; arbitrary fractional
remainders or unexplained coefficients retain native combined text.

Shadow Mantle has Godly201-225 and Silver-Wrought16-20 Enhanced Defense.
Silver-Wrought also has durability16-20 as a second specification. The owned
leaf has stat16=230 and stat73=17; aggregate ED is consumed. The old fixture
used function1 for ED, whereas production ac% uses function2. The corrected
model admits function2 only for stat16/layer0 with the expected target stat.
Combined ED bounds are217-245. Several pairs sum to230, so individual source
actuals remain unknown while their ranges and the actual combined total remain.

Game function2 entry RVA0x3CFA60 reads min/max at specification+8/+0xC and
calls the known RNG helper RVA0x3D5860 at RVA0x3CFADC. It forwards the rolled
scalar to RVA0x3D5940 at RVA0x3CFB31. Its earlier native preparation call is
RVA0x3D5AC0 at RVA0x3CFACD; this plugin does not reproduce or invoke generation.
The additional byte witness at game RVA0x3CFAD2 is:
`44 8B 47 0C 48 8B CB 8B 57 08 E8 7F 5D 00 00`.
It is checked with the existing scalar-range witnesses before installation.

Shadow Mantle's flat Thorns is stat78=2, with known function1 bounds1-3.
The fallback scalar model omitted stat78 even though ScalarBounds understood
its property. Adding it removes that false unknown-range report. Flat and
level-scaled Thorns remain separate property/coefficient families.

## Socket capacity correction

The native Socketed line is built outside the property buffer. Historical
property diagnostics contain no Socketed line. Core's already-socketed branch
at RVA0x81AE64 leads to RVA0x81AE7B after its header builder; the unsocketed
capacity branch enters RVA0x81B234 and calls Core RVA0x3763A0 at RVA0x81B25F.
Consequently revision31's suffix matcher and fixture operated on a line that
the live property transformer never receives.

Revision32 emits the native localized `ItemStats1p` (Max Sockets) template into
the transformed property block for socketed items. The template key is11 bytes;
Core uses it at RVA0x81B28F, with key storage at RVA0x62BBC0. Lookup retains
the existing Core RVA0x387D60 ABI and d2r group. Exactly one integer placeholder
is required. Capacity comes from the already guarded read-only helper
RVA0x3763A0 (item RCX, return AL), not a guessed base item cap. The current
count must exist in primary stat194, lie in1-6, and not exceed capacity.
An existing Max Sockets line, unavailable service/template, invalid capacity
or insufficient buffer preserves previous text. Unsocketed native headers
are unchanged. No extra formatter hook is required. See revision31's ledger
for the helper guard and active-table/configured-capacity contract.

## Validation

All seven automated suites pass, including production adapter fixtures with
the observed root flags, multi-specification Trump/Silver-Wrought records,
function2 ED, flat Thorns without clone identities, and a property block with
no native Socketed line. Unknown root/attached flags, wrong handler target,
layer mismatches, duplicate text and invalid capacity refuse. Existing socket,
corruption, color, request identity/release and header checks also pass.

Read-only observations establish the live layout and operands. Visible
revision32 rendering, placement and loaded guard admission remain pending a
restart test. No release is published. Enchantments remain deferred.

Revision32 was installed with game and loader confirmed closed in the standalone
global folder on 2026-10-12:
`<game-directory>/d2rloader\plugins\Item Roll Ranges.dll`.
Built/installed SHA256:
`DD54D8338ED90982B2736BFCA4D33B76CF5BDD2349CB46E4E4E5286D30380EEF`.
The installed artifact passes its ABI manifest/export/metadata/role test.
Other DLLs/TOMLs retain their previous hashes. Rollback under the installation:
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev31.before-rev32.20261012-145838.dll`.
Local deployment receipt: `outputs/item-roll-ranges-rev32-deployment.json`.

## Revision33 header correction

The user screenshot shows revision32's extra `Required Level: 2` modifier row.
The active Reimagined `data/local/lng/strings/item-modifiers.json` identifies
ItemStats1p as Required Level. The earlier disassembly stopped before the
actual maximum-sockets string lookup and the mock assigned the wrong text to
that intermediate key. This was a faulty interpretation, not a capacity-math
failure. Revision33 removes that modifier insertion entirely.

Read-only loaded-code inspection on 2026-10-12 extends the branch record:

- Core RVA0x81B28F looks up ItemStats1p to derive a Required Level template
  prefix. Its continuation jumps to RVA0x81C016.
- At RVA0x81C221, Core references the key at RVA0x62BC90:
  `d2rloader:D2RLoaderMaximumSockets`, length33 excluding NUL.
- RVA0x81C238 calls Core RVA0x389E60 with NativeText* in RCX. That wrapper
  routes to the already qualified lookup RVA0x387D60 with d2r resource group,
  NativeText* key and required=true. No new lookup ABI is invoked by revision33.
- RVA0x81C264 loads the capacity byte stored at frame+0x1898 into R9D;
  RVA0x81C277 formats the actual MaximumSockets template through RVA0x1BEBE0.
  The preceding ItemStats1p is not the capacity label and must not be reused.

Additional optional guard at Core RVA0x81C221, 28 bytes:
`48 8D 05 68 FA E0 FF 48 89 45 E0 48 C7 45 E8 21 00 00 00 48 8D 4D E0 E8 23 DC B6 FF`.
The key including its NUL is guarded at RVA0x62BC90. These two new guards and
the existing helper-entry guard match the loaded build. The revision32 Roll2
game witness at RVA0x3CFAD2 was also compared and matches.

For an identified socketed source during the existing qualified hold request,
revision33 adds the native localized capacity label to the header builder's
owned text, below Required Level in displayed order (native lines are stored
bottom-to-top). It uses header white/color0. Current count remains primary
stat194; capacity still comes from the guarded read-only helper and must be
at least current and at most6. Unsocketed items retain Core's existing path.
No branch patch, item write, extra hook or input interception is added.

The existing receiver/actual-text publication proof now also retains this
addition for weapon headers without a Defense value. That matters because
Core may replace the header after restoring range TLS. No source pointer is
stored in the proof. Duplicate capacity headers, missing/malformed templates,
unknown ownership or invalid capacity retain native behavior. The property
formatter no longer injects any capacity or Required Level line.

Seven automated suites pass. Corrected fixtures distinguish ItemStats1p's
Required Level from MaximumSockets, verify header order/white color, translated
single-integer templates, duplicate refusal, invalid capacity, unsocketed
passthrough and retained weapon capacity through publication with restored
TLS. Visible placement and hold/release behavior await revision33 restart.
Nothing is published; only Item Roll Ranges is built.

Revision33 installed in the same standalone global plugin folder after closure
confirmation on 2026-10-12. Built/installed SHA256:
`BAF739EE0FD3F7E4EC0DDF3C9A77E08E7D8733EFA3DAB36F5205F5CF20C1BAB2`.
The installed artifact passes; other DLL/TOML hashes are unchanged.
Rollback under the game installation:
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev32.before-rev33.20261012-150922.dll`.
Local receipt: `outputs/item-roll-ranges-rev33-deployment.json`.

## Revision34 header placement correction

The user's revision33 screenshots confirm the correct MaximumSockets label
and capacity2 for Beast Shell, but show that label below all affixes. Shadow
Mantle's native unsocketed Max Sockets4 remains between Required Level and
the first affix. The screenshot also confirms its Godly/Silver-Wrought combined
bounds217-245 with separate unknown source actuals, and flat Thorns bounds1-3.
Thus the revision33 placement claim above is superseded; its localization
and read-only capacity contracts remain valid.

The game builder's returned text is the whole tooltip, including modifiers,
in bottom-to-top line order. Prepending a line puts it beneath those modifiers.
Revision33's fixture used only requirement/header lines, so it missed this
production layout. A bounded read-only stack sampling attempt did not capture
a complete receiver and adds no independent live-buffer evidence; the user's
screenshots establish the placement failure. No new RVA, native hook or ABI
is introduced by revision34.

The corrected insertion resolves ItemStats1p as the localized Required Level
anchor and inserts immediately before that unique line in buffer order. In
display order this puts capacity below Required Level. A weapon's Attack Speed
line stays below capacity. Without a level line, insertion uses the earliest
recognized header in buffer order, using localized keys ItemStats1e (Strength),
ItemStats1f (Dexterity), ItemStats1d (Durability), ItemStats1i (Quantity),
ItemStats1h (Defense), ItemStats1l/ItemStats1m (one-/two-hand damage) or
strItemStatThrowDamageRange. These keys use the existing lookup ABI and active
Reimagined item-modifiers.json contracts; no English matching is used. Existing
Base/unknown header annotations can extend a recognized numeric shape without
losing the anchor. Missing/duplicate anchors or capacity lines preserve native
text, including original trailing-newline behavior.

The native unsocketed capacity branch remains unchanged; socketed additions
still require the existing qualified hold scope, identified source, ownership
snapshot and valid native capacity. Release, source identity and publication
proof contracts are unchanged. No property-row capacity insertion remains.

Regression fixtures now include Socketed, modifier/breakdown rows, requirements
and weapon Attack Speed in the same complete bottom-to-top layout. They check
exact output placement, requirement ordering without level, durability and
annotated Defense fallbacks, translated anchors, duplicate/no-anchor refusals,
unchanged existing capacity and retained weapon publication after TLS restore.
All seven automated suites pass. Visible revision34 placement and hold/release
confirmation remain pending.

Revision34 installed in the standalone global plugin folder after the user
confirmed closure and the process check found no game/loader on 2026-10-12.
Built/installed SHA256:
`CC45F2305A6034CE80E08C74C412C4B381A28BF0C185F16C354E3E0D7EAC9DE7`.
The installed DLL passes its artifact test; other DLL/TOML hashes are unchanged.
Rollback under the game installation:
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev33.before-rev34.20261012-152323.dll`.
Local receipt: `outputs/item-roll-ranges-rev34-deployment.json`. Not published.

After installing revision34, the user replied "Looks good" to the requested
restart/header-position check. Record corrected socket-header placement as
user-confirmed live rendering. The reply did not provide separate per-item,
controller or additional quality coverage; automated fallback/refusal tests
remain distinct from that live confirmation. No further code changed.
