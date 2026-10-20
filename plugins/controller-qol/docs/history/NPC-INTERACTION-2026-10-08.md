# NPC interaction after snap aiming (rev.70)

The user reports that native A needs two presses to interact with NPCs after a
snap-aim skill. Static review found that idle preview keeps `manualAim` armed,
and `ReadMonster` accepts UnitType 1 without distinguishing NPCs, friendly units
or other native categories. `PointScore` previously replaced geometry for every
category reached through that reader. The first disabled/interaction skill then
releases aim, potentially allowing the second press to use normal geometry.
This is a plausible cause, not a confirmed runtime diagnosis.

The Ladder log at 2026-10-08 19:29:24 shows reticle scoring for categories 2 and 5
during idle preparation (profiles 8 and 5). It proves category 5 was overridden,
but does not identify the unit or prove that particular NPC failed.

## Build-specific evidence

Captured code PE SHA-256:
`246DCB55785662194EE1103EF159ABD2DD37DCE66A7711BDC33CC519AE15FDFF`.
All addresses below are RVAs in that artifact. The captured executable is not
committed. Expected byte sequences and hashes are recorded in
`target-category-evidence.json` and `src/aim/target_category_profile.h`.

| Path / field | Static observation |
|---|---|
| `0x18B350` | Existing ScoreUnit; entry is already QOL-owned. No new hook added. |
| `0x18B45B..0x18B480` (38 bytes) | ScoreUnit supplies controller/player/candidate to classifier at call `0x18B464 -> 0x18E390`; `0x18B469` copies returned EAX into R9D, the fourth PointScore argument. Call `0x18B47C -> 0x18AF30` receives native XY and stack profile. Exact byte witness is outside the entry patch. |
| `0x18E390..0x18E5F0` (609 bytes) | Native classifier: Windows x64 `(controller, player, candidate) -> int`. Entire body through RET guarded read-only; QOL does not invoke or hook it. |
| `0x18E41E..0x18E459` | UnitType 1 has special category 6 and category 5 branches before ordinary monster fallback. Category names are not assumed from the numeric values. |
| `0x18E426 -> 0x9ABA0` | Predicate selects category 6 when nonzero. Reviewed type-1 tail `0x9AC59..0x9AC96` calls `0x349860`, then `0x98540` with selector `0x14`, and compares mode with `0xC`; type-0 tail compares mode with `0x11`. Likely death-related, not a newly proven general API. No direct calls added. |
| `0x18E43C -> 0x34F5A0`, `0x18E448 -> 0x3500E0` | Category 5 requires first predicate false and second true. Second helper is null-guarded and returns `(Unit+0x124 >> 1) & 1`. Flag name and complete first-helper contract remain unproven; QOL does not read this flag or call either helper. |
| `0x18E45B..0x18E54D` | Additional player/monster branches select categories 7, 8, 9, 10 using native predicates. Calls include `0x34A330`, `0x13ABA0`, `0x13AB80`, `0x136A70`, `0x34F5A0`, and `0x971E0`; broader ownership/team semantics remain unproven and are not implemented in QOL. |
| `0x18E54F..0x18E584` | Remaining UnitType 0/1/2/4/5 map to categories 0/1/2/3/4 respectively. Unknown types keep -1. Category 1 is therefore the ordinary monster fallback, not every UnitType-1 unit. |
| `0x18E5E4` | Classifier copies EBX to EAX for return. |

## Change and limits

The existing PointScore wrapper preserves native scores for every category
except 1 (ordinary monster), and 2/3 (object/item) inside the existing
Telekinesis native-unit scoring window. No interaction priority, native input,
server transaction, skill enable state, or targeting override is changed.

Enemy caching additionally requires at least one category-1 score and no other
category during that candidate's native enumeration. Missing/mixed categories
fail open to native selection; an existing retained identity is invalidated.
This also avoids projectile leading toward noncombat units. Unknown categories
retain native geometry; no claim is made about custom hostile-player categories.

The classifier and argument witnesses are required during aim admission before
installing aim hooks. Mismatch leaves the aim module inert and logs the site;
normal game targeting and the independently admitted QOL modules remain active.
There are no added hooks, native helper calls, target pointers, or asynchronous
work. Category metadata follows the existing nested thread-local scoring scope.
Bounded `[QOL/AimTrace]` candidate diagnostics now include the native category.

All 28 automated suites pass. Policy checks cover native category preservation, Telekinesis
object/item isolation, absent/mixed classification and retained-target removal.
Live admission, first-A NPC interaction after Fireball/Telekinesis, enemy snapping,
and Telekinesis chest/item behavior still require gameplay confirmation.

Subsequent user test: rev.70 resolved the reported first-A NPC interaction issue.
This confirms the tested path; other NPC/mod combinations remain unqualified.
