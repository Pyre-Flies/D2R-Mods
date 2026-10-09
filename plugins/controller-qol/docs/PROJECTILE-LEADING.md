# Experimental projectile leading: rev.67

## Rev.71 packaged baselines

The user confirmed the rev.70 NPC first-press fix and Shock Web tuning.
[Rev.71](LEADING-BASELINES-REV71.md) supersedes the earlier commented-only and
deferred-trial status below: 42 active estimates, a complete 240-ID ledger, and
explicit zero-lead choices. Historical trial records remain for provenance.

## Rev.70 local tuning feedback

Remove Shock Wave (243) from the leading trial at the user's request; preserve
its skill enable/targeting settings. Increase Shock Web (256) from 100 to 110
ms/tile (+10%) with the same 600 ms / three-tile limits. The increase only affects
predictions below those caps. Tornado, Flame Wave, Charged Bolt and Double Throw
feel good in user testing; Twister's wide coverage makes the benefit difficult
to judge, so leave its trial unchanged. Other estimates remain unchanged.
The user subsequently confirmed Shock Web feels better.

## Evidence and scope

The user reports moving enemies outrunning current-position snap casts,
especially the lobbed Fire Blast and Reimagined Ice Barrage. This prototype
adjusts the destination after choosing an enemy by its actual position. It uses
existing copied monster observations, adds no native hook or scheduler, and
retains no native unit pointer between calls.

Inspected installed Reimagined Ladder data on 2026-10-08:

| File under the mod's `data/` directory | SHA-256 |
|---|---|
| `global/excel/skills.txt` | `708DA23BF078CC3FC4D2EF901C7480869E18206ABA8CBC91E4030D5CBA93FEF3` |
| `global/excel/missiles.txt` | `432CC9EA7CE03D2C6D65370E7412B4C077ED1BD17000902F5AB173AE03A9FAED` |
| `hd/global/excel/controllerskillsettings.json` | `4B5E7902F5E6169342D624D956469E5B4B56BA76E94C5333632776EF0C0FBA79` |

Skills use the `*Id` column. Fire Ball (47) and Bone Spear (84) have native
`usePredictiveTargeting=true`; Fire Trauma/Fire Blast (251) and Ice Barrage (253)
have it false. The sampled `fireball`, `bonespear`, `magicarrow`,
`multipleshotarrow` and `glacialorbarrow` missile rows have Vel/MaxVel 24/24.
`bomb in air` and `icebarrage_throw` have 12/12. This supports different tuning
for these two groups, but does not establish equal speed for all projectiles or
an absolute conversion to world tiles per second. Lob trajectory and cast
animation can also affect arrival time. Shock Web remains the distinct ID 256.

Initial local trial values are 50 ms per tile for 47/84 and bow IDs
6/7/11/12/16/21/26/27/31, and 100 for 251/253. These are tuning estimates,
not native speed conversions. Homing Guided Arrow (22) is omitted. The settings
do not enable any previously disabled skill; shipped configuration only contains
commented examples.

### Expanded local trial, 2026-10-08

The user reports that leading feels better and requested additional cold/fire
bolts and thrown-javelin skills. The same installed skills/missiles hashes above
were reverified before extending the active Ladder `[aim.leading]` configuration:

| Skill ID | Installed skill | Missile | Vel / MaxVel | Trial ms per tile |
|---|---|---|---|---|
| 15 | Poison Javelin | poisonjav | 24 / 24 | 50 |
| 20 | Lightning Bolt | lightningjav | 30 / 30 | 40 |
| 25 | Plague Javelin | plaguejavelin | 24 / 24 | 50 |
| 35 | Lightning Fury | lightningfury | 30 / 30 | 40 |
| 36 | Fire Bolt | firebolt | 20 / 20 | 60 |
| 39 | Ice Bolt | icebolt | 12 / 12 | 100 |
| 45 | Ice Blast | iceblast | 12 / 12 | 100 |
| 55 | Glacial Spike | glacialspike | 18 / 18 | 67 |

Values scale the current Fireball estimate by `24 / Vel`, rounded to an integer.
They are empirical trial settings, not proven native speed conversions. For
javelins, leading adjusts the initial projectile destination; it does not predict
subsequent clouds or secondary lightning. Generic Throw (2) is not added as a
custom skill because it also covers weapons outside this requested skill group.

Existing leading entries, targeting overrides and enabled/disabled states remain
unchanged. Ice Bolt, Ice Blast and Glacial Spike were off at the time of the edit;
the user can enable them with R3. The distributed config/generator gains commented
examples only. Parsed TOML comparison verifies exactly these eight new active
entries and no other semantic changes. No new DLL, native path or ABI is needed;
restart loads the settings. Live accuracy of these additions remains pending.

## Rev.69: slow ice projectiles and changing speeds

### Further projectile coverage after rev.69 testing

The user reports the expanded ice limits feel better and explicitly excludes
Slow Missiles compensation from scope. No debuff monitoring is planned here.
The next local configuration trial adds these installed-table mappings (the
skills/missiles hashes above were reverified):

| ID | Skill | Missile | Vel / MaxVel | Trial ms per tile |
|---|---|---|---|---|
| 49 | Lightning | lightningbolt | 35 / 35 | 34 |
| 53 | Chain Lightning | chainlightning | 35 / 35 | 34 |
| 67 | Teeth | teeth | 20 / 20 | 60 |
| 101 | Holy Bolt | holybolt | 20 / 20 | 60 |
| 266 | Blade Fury | ri_bladefury | 22 / 22 | 55 |
| 395 | Miasma Bolt | miasmabolt | 20 / 20 | 60 |

Estimates use rounded `1200 / Vel`, matching the prior Fireball baseline. The
listed projectile rows have no populated Accel/VelLev modifier. Teeth's other
`bonecast` entry is not used as its travelling projectile speed. Leading affects
initial targeting, not Chain Lightning's subsequent bounces, projectile spread,
or Holy Bolt's native target eligibility. All six retain default 600 ms /
three-tile caps. The earlier ice overrides and all enable/targeting states remain
unchanged. New coverage totals 27 configured IDs and requires live validation.

Charged Bolt, Frozen Orb, Firestorm, Twister/Tornado, channelled attacks, movement
skills, homing projectiles and weapon-dependent Double Throw remain outside this
increment: a nominal missile speed alone does not describe those trajectories,
secondary effects or weapon choices well enough to claim the same benefit.

The active leading entries are now grouped with class comments and readable skill
names (including installed Ladder renames). TOML sections remain independent:
`[aim.class]` owns enabled/locked state, `[aim.targeting]` owns target mode, and
the leading maps own tuning. This avoids changing boolean/`"disabled"` values to
compound objects or migrating the R3 writer. Same numeric ID joins all maps;
leading never enables a skill. Stock config/generator adds commented examples.
Parsed TOML comparison permits only the six added settings; the remaining edit
is comments/order. No new DLL, native path or ABI was introduced.

### Next trajectory trial group

The next local expansion adds seven IDs, bringing configured leading coverage to
34. Installed skills/missiles hashes remain those recorded above. Weapon data
was also inspected: `weapons.txt` SHA-256
`FA8FEAD7B845A81BBD62C329719C80E5B2E97721AB44316AAC065F305F41BCEC`.

| ID | Skill | Relevant missile/table evidence | Trial ms/tile |
|---|---|---|---|
| 38 | Charged Bolt | `chargedbolt`, Vel/MaxVel 12/12; skill spawns multiple bolts | 100 |
| 140 | Double Throw | Throwing weapon types tkni/taxe/jave/ajav reference missile IDs 1/35/36/37/371: javelin/throwaxe/throwknife/glaive/pilum, all 24/24 | 50 |
| 240 | Twister | `twister`, 14/14, multiple projectiles via skill srvdofunc 118 | 86 |
| 243 | Shock Wave | `shockwave_spawn` has no speed; its HitSubMissile1 is `shockwave_prime`, 24/24 | 50 |
| 245 | Tornado | `tornado`, 12/12, missile pSrvDoFunc 27 | 100 |
| 256 | Shock Web | Internal skill name Shock Field; `shock field in air`, 12/12, hits `shock field on ground` | 100 |
| 398 | Flame Wave | `flamewaveunveiling` spawns `flamewave`; both 24/24; later fire children are separate | 50 |

These start with rounded `1200 / Vel`; the expanded ice caps are not copied.
All seven use the existing 600 ms / three-tile defaults. No enable states were
changed (all seven were off); R3 can enable each for testing. Existing tuned
Holy Bolt/Miasma Bolt/Blade Fury settings and ice overrides are preserved.

The first useful checks are Double Throw, Shock Wave and Flame Wave, followed by
Shock Web's initial landing. Charged Bolt, Twister and Tornado are exploratory
direction trials: their spread/path behavior is not solved by velocity-leading
one enemy. Test whether the overall stream follows sideways motion better;
do not interpret an individual wandering projectile's miss as proof that more
lead is required. Shock Web's later field is not retargeted after landing.

Frozen Orb and Molten Boulder remain deferred. Frozen Orb's parent travels at
12, emits frozenorbbolt children at 18 and ends in frozenorbnova at 24. Optimizing
burst placement cannot be inferred from the parent speed alone. Molten Boulder
starts with moltenboulderemerge at speed 3 / Range 5, then its hit child
moltenboulder travels at 12 / Range 60; the initial stage needs separate timing
evidence. No absolute time conversion from these Range fields is claimed.

Observed tuning now supplies useful starting families, not a universal speed
law: Fireball/bows use 50, Fire Bolt 60, Holy/Miasma 68 and Blade Fury 69; slow
ice/lobbed projectiles start at 100. User-requested changes to the last three
remain estimates until retested. Ice demonstrated that capped displacement can
dominate the travel estimate. Preserve independent per-skill limits and tune
similar trajectories together before broadening them.

TOML semantic comparison verifies only seven new entries. Stock examples and
generator syntax validate; the DLL and native code are unchanged. These seven
skills' leading behavior remains unverified in game. Slow Missiles remains out
of scope.

### Earlier ice and bolt tuning evidence

Further user tuning on 2026-10-08 requests 10..15% more leading for Holy Bolt
and Miasma Bolt, and about 25% more for Blade Fury. Active travel estimates and
commented stock/generator examples are now 68 ms/tile for IDs 101/395 (from 60,
+13.33%) and 69 ms/tile for ID 266 (from 55, +25.45%). The table above records
the initial trial. Time/distance caps, other skills and enable states are
unchanged; the percentage increase applies before those caps. Parsed config
comparison verifies only these three value changes. No DLL change is needed;
restart loads the new estimates, which still await live confirmation.

The user reports Ice Bolt/Ice Blast landing behind moving enemies, while Fire
Bolt and the Amazon additions feel fine. At 18:49:56.840 on 2026-10-08, the local
Ice Bolt trace reports `travelMsPerTile=100` and `lead=3.00`, proving the old
displacement cap was reached in at least that cast. It does not prove every miss
was capped or that Ice Blast reached it. Raising just the travel estimate would
not overcome an already saturated cap.

Optional integer maps `[aim.leading_max_ms]` (100..1500) and
`[aim.leading_max_tiles]` (1..8) now override each skill's 600 ms / three-tile
defaults. The active Ice Bolt/Ice Blast trial uses 1200 ms and six tiles, keeping
100 ms per tile. Other skills keep their previous settings and limits. All
predictions still require fresh consistent motion, valid world coordinates and
the existing 30-tile player radius. R3 preserves both maps. Configuring only a
limit does not enable either a skill or its leading.

The model already measures enemy movement. A chilled enemy's lower displacement
reduces estimated velocity; a large speed change drops prediction confidence
until samples agree, and a stationary/frozen sample clears it. Detection can
take a sampling window (50..150 ms), and moderate changes are averaged with the
previous velocity. New automated checks cover halved speed, a larger slowdown,
confidence recovery and freezing. This is tested motion policy, not a live cold
debuff test.

Slow Missiles affects a different input: projectile flight time. Verified local
data, same skill/missile hashes as above:

- `itemstatcost.txt`: `skill_handofathena` has ID 161; SHA-256
  `B45FFB70EC46DE189C6F3B5F94944AD74948976F53FCD97E6590F0E8C1D074F1`.
- `states.txt`: `slowmissiles` has ID 87 and names that stat; SHA-256
  `A89EE2CE15E2C96369D3EDE45CFD629938EEFAE6D9B0D4B920E7A0FCD788ADE1`.
- Skill 17 targets state `slowmissiles` and sets `aurastat1=skill_handofathena`,
  `aurastatcalc1=100-dm12`, with parameters 25 and 75.
- Sampled `icebolt`, `iceblast`, `firebolt`, `lightningjav` and `plaguejavelin`
  missile rows each have `CanSlow=1`.

These table links do not establish the native runtime stat encoding, where the
modifier is sampled, or whether an already flying missile can change speed. No
stat multiplier is applied in rev.69. Before implementing one, trace the native
consumer of stat 161, confirm effective missile speed with and without the
debuff on this build, and establish which projectile families inherit it. Do not
multiply by a guessed reduction percentage or infer a curse state from observed
enemy slowing. No native path/layout/hook changes were required for rev.69.

All 28 automated suites pass, including new limit bounds, defaults/isolation,
R3 rewrites, expanded prediction, hard range fallback and changed enemy speed.
Live ice accuracy and debuffed projectile behavior remain unverified.

## Native path review (rev.67)

All addresses below are game-relative and apply only to the captured executable
whose SHA-256 is
`246DCB55785662194EE1103EF159ABD2DD37DCE66A7711BDC33CC519AE15FDFF`.
The local captured code image is evidence, not a redistributable artifact.

| RVA / layout | Observation |
|---|---|
| `0x190440` | Existing LookupSkillTarget hook; original executes before QOL supplies coordinate destinations. |
| `0x19057A -> 0x18DDE0` | Selected-target call; result retained in RBX. |
| `0x1905A7` | Branch on selected unit; following path reads current dynamic-path coordinates. |
| `0x190739 -> 0x18D3E0` | Adjustment-branch call observed. Its complete purpose and ABI are not established; this prototype does not invoke it as a prediction API. |
| `Unit+0x38`, dynamic path `+0/+4` | Existing monster reader copies unsigned 16.16 X/Y coordinates; identity is the existing `Unit+8` ID and local player ID. No new layout introduced. |

QOL's coordinate route suppresses the selected unit and replaces Lookup output.
Consequently a native predictive flag alone does not prove prediction survives
that route. Rather than invoke an unproven native helper, this trial estimates
velocity from positions already observed in the natural UnitTest callback.
Native rejection invalidates the observation and its movement history.

## Prediction policy

`[aim.leading]` maps catalog or declared custom skill IDs to integer estimated
travel milliseconds per world tile, 0..200. Missing/zero disables. The parser
rejects invalid values, duplicate numeric aliases and undeclared IDs. R3 rewrites
only the enabled setting and preserves leading preferences.

Each enemy history samples movement over 50..150 ms windows. Two agreeing
velocity windows are required: direction cosine at least .75 and speed within
half to twice the previous speed. Agreeing velocities are averaged. Stationary
movement below .25 tiles/second, implausible speed above 40, time reversal,
observation gaps above 150 ms, rejection and identity replacement reset history.
A turn can establish a new velocity, but must again agree before prediction.

Only an active snapped coordinate cast uses prediction. Estimated travel time is
distance times configured milliseconds per tile, capped by default at 600 ms.
Displacement is capped by default at three tiles; rev.69 can override these limits
per skill as described above. Observations must be at most 75 ms old; invalid world
coordinates or destinations beyond the existing 30-tile player radius fall back
to the observed position. The lock marker and target ranking stay on the enemy's
actual position. Idle preview, Telekinesis's native unit route, ground-only casts
and Whirlwind do not receive leading.

This is a bounded velocity extrapolation, not a full ballistic/interception
solver. It does not compensate for cast animation, network latency, acceleration,
walls or future turns. Motion changes can take one sample window to be detected.

## Validation

All 28 automated suites pass, including configuration bounds, R3 preservation,
confidence warmup, stops, turns, jumps, stale observations, identity changes,
distance scaling and displacement/range caps. DLL version/export checks pass.
Live accuracy, practical tuning and offline/Ladder comparison remain unverified.
The existing bounded lookup log emits `[QOL/AimLead]` when a nonzero lead is used.
This change is not a crash fix.
