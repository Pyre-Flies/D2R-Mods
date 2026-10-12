# Fixed and scaling affixes, revision30

Target: the admitted D2R 3.3.93787 and exact D2RCore provider profile already
recorded by this plugin. Source labels, native formatter calls, ownership and
input contracts are unchanged. New behavior has optional witness admission;
mismatches retain previous tier families/ranges. Function8 scalar admission
uses the existing UniqueRangeProfile Roll8 witness, not a new native call.

## Read-only observations, 2026-10-12

Game hosted in D2RLoader process3248, game base0x140000000 and
Core base0xC0DE5000000. The existing nine socket witnesses matched.
Raven Carapace is unit ID33, quality6, bank3, class435 (Boneweave, code`uhn `).
The first probe mistakenly searched class439/code`uld `; that was a research
filter error, not a runtime item mapping error. Active loaded rows/definitions:

| Source | Saved affix ID | Property / parameter / low / high | Observed native stats |
| --- | --- | --- | --- |
| Paleocene | 986 | 141 / 48 / 0 / 0 | stat214 coefficient48 |
| Loremaster's1 | 1272 | 252 / 0 / 1 / 1 | stat85 value1 |
| Artificer's | 1243 | 133 / 3 / 0 / 0 | intrinsic socket count3 |
| of the Giant | 231 | 7 / 0 / 10 / 15 | Strength13 |
| of Balance | 322 | 79 / 0 / 10 / 10 | stat99 FHR10 |
| of Chilling Armor | 600 | 260 / 60 / 100 / 17 | stat199 layer3857, chance100 |

Raven's primary Defense428 and stored coefficient48 produce the screenshot's
native Defense1022 at level99, without changing the normal Boneweave base
interval399-505. That header is separate from the level-scaled modifier.
The fixed FHR property uses handler8; the old ScalarBounds restriction accepted
that handler only for IAS stat93. The handler is generic for its declared stat
and rolls the source low/high before the shared scalar grant. Extending this
already guarded function covers FHR and other applicable speed scalars without
label-name matching or a stat-name allowlist.

Beast Shell is ID29, class360, quality6, bank3. It retains of Thorns ID338,
property165 / parameter8 / low0 / high0. Loaded rows with the same display name:

| ID | Affix level | Property | Parameter / bounds | Family |
| --- | --- | --- | --- | --- |
| 341 | 1 | 54 | 0 / 1-1 | flat damage |
| 334 | 14 | 54 | 0 / 1-3 | flat damage |
| 338 | 40 | 165 | 8 / 0-0 | damage per level |
| 339 | 67 | 165 | 12 / 0-0 | damage per level |

The old family comparison required identical parameters, so coefficient8 and
12 each appeared to be T1 in separate families. Function17 reads a nonzero
parameter as a coefficient. Compare positive coefficients within the same
property family, while retaining group, class-specific marker, property IDs,
eligibility/version/quality and distinct higher affix levels. Pure function17
properties permit this relaxation; mixed handlers, zero/random coefficients,
encoded groups and skill/class/layer selectors retain strict comparison.
Display names are never family keys. Flat Thorns and per-level Thorns remain
separate. With the stronger row eligible, coefficient8 is T2 and12 is T1.
This logic also applies to other coefficient properties and socket-jewel tier
definitions; it is not a special case for the name of Thorns.

## New native witnesses and metadata

The source ledger `scaling_profile.h` records these exact byte windows:

| Module / RVA | Contract |
| --- | --- |
| game0x3CC59B | handler17 reads specification parameter DWORD+4 and tests nonzero |
| game0x300C84 | Experience maximum-level getter recognizes class indices0-7 |
| game0x300CC8 | Experience vector at active bank+0x14E0 |
| game0x300CFF | row stride9 DWORDs (36 bytes), indexed class DWORD read |
| Core0x3DE41B | positive specification high DWORD+0xC is a fixed skill level |
| Core0x3DE4D5 | specification low DWORD+8 is chance (default5 only when nonpositive) |

Research also verified handler8 at game0x3D0EC0 and the existing roll window
0x3D0F07; handler17 at game0x3CC550 forwards the coefficient to the scalar
grant0x3D5940. Proc handler11's patched game entry0x3D12B0 delegates through
game slot0x3E2A488 to Core0x833F00, which calls Core0x3DE360. Positive level
and chance proof uses that loaded Core path. Automatically calculated levels
follow another branch containing99; revision30 does not infer their bounds.
No new hook, native mutation, alternate proc grant, or Item SDK is introduced.

Experience pointer is active bank+0x14E0, count+0x14E8. Current count102;
first36 bytes are eight MaxLvl100 columns and ExpRatio10. The native getters
use row zero for caps and subsequent rows for levels, hence require cap+2 <=
row count. Cap admission requires eight equal positive class caps <=255 and a
bounded count <=1024. Unequal caps need a separate player-class proof and are
declined, retaining native output. This does not hardcode Reimagined to100.

Loaded ItemStatCost stride0x144 metadata observed for stat214 and238:
ValShift byte+0x14 =0, descfunc byte+0x32 =19, operation byte+0x50 =4/2,
operation shift byte+0x51 =3, operation base WORD+0x52 =12 (character level).
These fields agree with the installed source definitions. The historical
[D2MOO layout](https://github.com/ThePhrozenKeep/D2MOO/blob/master/source/D2Common/include/DataTbls/ItemsTbls.h)
helped locate the metadata, but its offsets differ; those historical offsets
are not cast into current D2R. Current admission checks row size, stat identity,
known operation2/4/5, level basis12, shift0-15, scalar descfunc19 and ValShift0.
Other bases (Strength/Dexterity/time), layouts and transforms retain prior
native bounds. Their coefficients can still belong to one tier family.

Only complete fixed coefficients from source definitions agreeing with both
owned and aggregate raw stats are extended to the active cap. Unknown socket,
enchantment or corruption contributions to that coefficient refuse this
override. Low/high display units are integer-truncated coefficient*level /
2^operationShift. Native original scalar formatting supplies localized text,
signs, percentage units and numeric-shape agreement; actual item text is kept.
Examples: Thorns8 gives1-100, Thorns12 gives1-150, Paleocene48 gives6-600.
The independently proven prefix replaces the older native99 interval, rather
than simply replacing the number99 in arbitrary strings.

Fixed proc prefixes require one direct ordinary affix contributor, a valid
packed skill/positive level using current bit count/mask, owned and aggregate
chance agreement, and exactly two corresponding actual numbers. They display
`[100%]` here; level17 remains in the original line. Chance100 and level17 are
not min/max, and this is not a claim that the skill level is a variable roll.
Automatic/unknown levels, charged skills, ambiguous sources, mismatching
chance/level/text and additional numbers retain the existing native display.
Socketed count remains the native `Socketed [3]` structural line; the reported
FHR/proc fixes do not invent a separate random roll for that count.

## Validation boundary

Read-only receipts and disassemblies under outputs stay local; extracted game
tables/code and process addresses are not shipped artifacts. Meaningful
regressions cover all four same-name Thorns definitions, another scaling
property, selectors/mixed handlers, variable/fixed function8 scalars,
coefficient agreement, uniform/unequal caps, vanilla99 and Reimagined100,
metadata refusal, fixed proc chance/level packing, actual mismatch and
optional-guard fallback. Full production capture-to-label-to-merge fixtures
exercise FHR, proc, Paleocene and Thorns without gameplay writes.
All seven automated suites pass. All six new optional witnesses match the
loaded build. Revision30 restart rendering remains pending.

Revision30 was installed locally on 2026-10-12 with the game and loader closed,
at `<game-directory>/d2rloader\plugins\Item Roll Ranges.dll`.
The built and installed SHA256 agree:
`F5A266102048BEDE4267819DA1FD1EB49B8EB2F41E158375D0D665453D174175`.
The installed DLL passes the artifact suite (loader ABI manifest, exports,
metadata and role). Other DLLs and TOMLs retain their pre-install hashes.
Rollback is preserved as
`Documentation\item-roll-ranges\backups\Item Roll Ranges.rev29.before-rev30.20261012-140003.dll`
under that game installation. The local deployment receipt is
`outputs/item-roll-ranges-rev30-deployment.json`. This candidate is not published;
these deployment checks do not establish visible restart behavior.
