# Armor/weapon quality and fixed modifier display, revision29

Target: the currently admitted D2R 3.3.93787 / D2RCore profile. This change
reuses the existing guarded tooltip adapter, bank row getter, ItemData quality
DWORD at +0, flags DWORD at +0x18, and primary Defense stat31. No native hook,
table layout, input mapping, Item SDK dependency, or game write is added.

## Read-only controls (2026-10-12)

The standalone game was hosted by D2RLoader process46056. All nine existing
socket witnesses matched. Active bank3 class360 Serpentskin Armor has minac111
and maxac126. Inventory page0 controls:

| Item ID | Quality | Intrinsic / displayed Defense | Own modifiers | Base interval |
| --- | --- | --- | --- | --- |
| 44 | normal2 | 121 / 121 | none | 111–126 |
| 47 | crude1 | 87 / 87 | none | 83–94 |
| 48 | superior3 | 118 / 118 | durability stat75 = 11 | 111–126 |

The user requested ignoring Cube controls. Page3 IDs40/41 were excluded from
the inventory sample conclusion; ID40 additionally corroborates the existing
ED path: intrinsic127, own ED8%, durability14%, displayed137. IDs with native
mode3 were also excluded. No item was moved or edited by the probe.

For location filtering only, existing game getter0x36CFE0 was read: its
successful branch0x36D01E calls0x34A500 to obtain ItemData, branches at0x36D029
to0x36D046, then reads byte ItemData+0x55. Tail bytes at0x36D046:
`0F B6 40 55 48 83 C4 20 5B C3`. Page0 is inventory; page3 is Cube in this
capture. This is research evidence only, not a new runtime ABI or guard.
Local receipts/probes are under outputs and are not committed.

## Rules and limits

[Blizzard's item basics](https://classic.battle.net/diablo2exp/items/basics.shtml)
documents low-quality armor at floor(normal Defense * 3/4), with the normal
base roll still varying. The active crude control is consistent with this
rule: floor(111*3/4)=83, floor(126*3/4)=94. Revision29 explicitly recognizes
quality1 before normal-range matching and requires observed primary Defense
inside that reduced interval. Unexpected or ethereal low-quality cases are
unresolved. A single live
control establishes consistency, not exhaustive generation behavior across
all modified item definitions.

### Weapon controls

Same process/build/bank and guarded page filtering: inventory Long Swords,
class31, with active one-hand definition bytes3/19 at0x10F/0x110. Two-hand
and throw endpoints are0/0 for this base; no such mode is inferred.

| Item ID | Quality | Intrinsic damage | Displayed damage | Own quality operands |
| --- | --- | --- | --- | --- |
| 57 | low1 | 2 / 14 | 2 / 14 | none for physical damage |
| 62 | superior3 | 3 / 19 | 3 / 20 | ED stats17/18 = 6 each; AR stat19 = 1 |
| 67 | normal2 | 3 / 19 | 3 / 19 | none for physical damage |

All three also contain layered stat97 for a skill; it is unrelated to base
damage. The superior sword does not use armor's maxac+1 rule: native primary
damage stays3/19, and floor(19*106/100)=20. The low sword confirms the documented
75% reduction, floor(3*3/4)=2 and floor(19*3/4)=14. Weapon base endpoints are
fixed minimum/maximum damage, not two independent random roll intervals.

Revision29 now checks low-quality one-hand/two-hand/throw modes with reduced
endpoints clamped to at least1, requiring exact agreement with native primary
operands. Low-quality ethereal and custom/mismatched bases remain unresolved.
Only one-hand Long Sword has a live sample; other modes and the clamp are
automated coverage backed by the documented rule, not live confirmation.
Receipt `outputs/flatmax-quality-weapon-controls.json` stays local.

Superior durability alone preserves the ordinary base interval. Generated ED
armor uses maxac+1 (127 here), including superior armor, before staged ethereal
and percentage truncation. That existing path is retained. The percentage ED
modifier's roll and the header's intrinsic Base interval are separate. No
hardcoded superior affix range is introduced: native modifier ranges retain
their current definitions and behavior.

The user chose to retain the existing Base wording without a Superior marker.
Rare Beast Shell quality6 and Saintly ED94% keep `(Base: 127 - 127)`.
Superior armor can have a variable base (durability only) or fixed max+1 base
(ED); a fixed base alone is not evidence of Superior quality. No source marker
or hold/release change is introduced.

Fixed modifier bounds now display a singleton: `[+38]`, `[+50%]`, or separate
fixed paired endpoints `[15 / 20]`. Base headers retain explicit endpoints,
including `(Base: 127 - 127)`. Plain fixed P/S affixes omitted by the native
range formatter gain a prefix only when the captured definitions and the
single actual number agree exactly. Loremaster's1 ID1273 has property252,
parameter0, min=max2; stat85 reports2. It therefore shows `[+2%]` alongside the
existing P/name/tier label. Proc/multi-number, mismatched and capacity-limited
cases retain the prior behavior; no fixed unique range is guessed.

All seven automated suites pass. Automated coverage includes the real control
shapes, low interval boundaries,
out-of-range and ethereal refusal, integer limits, superior ED/durability,
socket ownership, fixed percentage/scalar/paired formatting, numeric mismatch,
multi-number refusal and buffer fallback. Revision29 visual validation is
pending. The user separately confirmed revision28 restores Beast Shell's
`Defense: 246 (Base: 127 - 127)` and preserves its Life breakdown.

Final local revision29 candidate SHA256:
`970AA353EBBD57BB461E7162C6356A29ABD61749C67109FD7E05C8EBC8B13E80`.
Seven suites pass after retaining the original header wording. After the user
closed the game and loader, revision29 was installed in the standalone global
`<game-directory>/d2rloader/plugins/Item Roll Ranges.dll`.
Built and installed hashes match. Revision28 rollback is retained under the
game's `Documentation/item-roll-ranges/backups/Item Roll Ranges.rev28.before-rev29.20261012-132104.dll`.
Other runtime DLLs and TOMLs match their before/after hash inventories. Local
receipt: `outputs/item-roll-ranges-rev29-deployment.json`. Restart/visual checks
for the fixed bounds and low-quality corrections remain pending. No publication.
