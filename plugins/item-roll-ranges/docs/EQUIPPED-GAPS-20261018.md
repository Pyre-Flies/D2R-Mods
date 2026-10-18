# Equipped modifier gaps - October 18, 2026

Read-only live inspection requested by user, who clarified equipped gear. PID12740, D2RLoader.exe, Reimagined Item Roll Ranges1.3.1+rev.54. Module paths confirmed through process modules. No game memory writes, native calls, configuration changes or deployments. Private item/table snapshots and detailed report retained under outputs/inventory-review-20261018.

172 loaded item records include21 mode1 equipped records, potentially companion and alternate gear.18 of those carry upgrade markers somewhere in their host-owned lists. Unidentified inventory items are not the reported scope.

## Shared refusals

13 equipped records carry upgrade_minor/medium/major/uber (392..395) in state0 or171. GatherSourceGroups refuses the entire source reconstruction when own has any such marker; HasRecipeExtras also blocks its owned fallback. This is a source-reconstruction refusal, not proof that every label or native range on the item is absent. Examples: Sandstorm Trek, Dracul's Grasp, String of Ears, Raven Frost, Plantar Enlightenment, Lachdanan's Bracers and modified runewords.

All five equipped Tal Rasha pieces also contain upgrade/corruption markers, but inside state166..168 rather than state0. Rev52 classified these states as activated set bonuses and preserves their native output without annotating them. The capture shows those lists cannot be assumed to contain only original set-definition bonuses:

- Lidless Eye state168: mastery/pierce stats, corruption361=2/362=123, upgrade_major394=5.
- Horadric Crest state167: FHR, poison-length reduction, elemental/all skills, Cannot Be Frozen, corruption361=2/362=189, upgrade_medium393=3.
- Adjudication state167: resists/max-resists, MF25, class skills2, corruption361=2/362=173, upgrade_minor392=2.
- Guardianship state167: life, block, physical resistance, FBR, corruption361=2/362=172, upgrade_major394=5.
- Fine-Spun Cloth state166: max-life15, FCR15, corruption361=2/362=139, upgrade_medium393=3.

These are runtime observations of list contents, not a proven universal recipe-placement rule. Future reconstruction needs property-level attribution using active definitions across eligible host-owned lists. Simply removing guards or treating every state as permanent would mix original, socket, upgrade and conditional contributions.

Wisp Projector is an equipped unupgraded/uncorrupted control. A current hovered-tooltip capture is still required to correlate individual missing visible lines with these model refusals. No new implementation or universal coverage claim is made by this inspection.

## Build-specific inventory scoping discovery

Existing native item-page accessor Game+0x36CFE0 checks type4 and calls0x34A500. At0x34A52B this returns unit+0x10 (ItemData). At0x36D046 bytes0FB640554883C4205BC3 load byte[ItemData+0x55], then return. Verified live. Observed mode0/page0 inventory36, mode0/page4 Cube55, mode0/page6 custom15, mode1/page255 equipped21, mode6/page255 socketed30, mode2/page255 belt14, and mode3/page0 ground1. Do not identify carried inventory from page0 alone. No new owner-identity mapping inferred.

Artifact D2RCore.dll SHA256 2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2.

Artifact Item Roll Ranges.dll SHA256 76126bd612a715950a4f44aa68735fb4f70e308bc9cc57fa0e41c12f52789d91.
