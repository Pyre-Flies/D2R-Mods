# Rev.71 Reimagined leading baselines

Current configuration (rev.75): all per-skill targeting, leading and prediction
limits live in the active mod's `.overrides.toml` profile. Shipped skill defaults
are maintained in `skill-defaults.toml`; the generated catalog is reference-only.
See [configuration and migration](SKILL-DISCOVERY-CONFIG.md). The revision-specific
trials and source evidence below are historical; discovery does not infer new
lead estimates for arbitrary mod replacements.

This beta supplies an explicit baseline for all 240 catalog IDs: 42 nonzero
travel estimates and 198 zero-lead entries. Zero means current-position aiming,
not disabling the skill. Existing R3 enable defaults remain ten enabled skills.
No automatic game/mod identity detection is introduced in this revision.

`leading-baselines.json` is the reviewed semantic ledger. It contains names,
numeric IDs, values and exclusion reasons, not extracted game records. The
`tools/leading_defaults.py` renderer keeps the shipped config and catalog
generator consistent. Runtime tuning remains ordinary TOML; no JSON dependency
is added to the plugin.

## Evidence and new estimates

Reverified installed Reimagined Ladder tables on 2026-10-08:
skills SHA-256 `708DA23BF078CC3FC4D2EF901C7480869E18206ABA8CBC91E4030D5CBA93FEF3`;
missiles SHA-256 `432CC9EA7CE03D2C6D65370E7412B4C077ED1BD17000902F5AB173AE03A9FAED`.
The empirical estimate `round(1200 / Vel)` supplies new starting values. This is
not a proven native world-speed conversion. Existing 33 trial values are retained,
including Holy/Miasma Bolt 68, Blade Fury 69 and Shock Web 110 ms/tile.

| ID | Skill in inspected Reimagined data | New ms/tile | Evidence and limits |
|---|---|---|---|
| 41 | Inferno | 92 | infernoflame1/2 Vel/MaxVel 13/13; channel direction estimate only |
| 64 | Frozen Orb | 100 | frozenorb 12/12; predicts parent direction, not final burst position or child timing |
| 116 | Shield Throw | 48 | pala_shieldthrow 25/25; initial projectile only, not subsequent bounces |
| 136 | Winter's Gambit | 50 | wgambit_spawn 24/24; not later wgambit_hit effects |
| 141 | Chasm Break | 12 | chasm_break 100/100; not its secondary effects |
| 225 | Firestorm | 100 | firestormmaker 12/12; stream direction, not each wandering flame |
| 229 | Molten Boulder | 100 | moltenboulder 12/12; emergence stage 3/3 with Range 5 has no separately modelled delay |
| 388 | Echoing Strike | 50 | echoingstrike 24/24; initial projectile, no later path modelling |
| 399 | Miasma Chains | 34 | zero-speed miasmachainsmaker SubMissile1 is miasmachains at 35/35; maker/secondary behavior not modelled |

New values retain the 600 ms / three-tile limits. Only previously tested Ice
Bolt/Ice Blast retain 1200 ms / six tiles. These bounds and motion-confidence
checks are unchanged. New estimates are not live-validated.

## Explicit zero-lead decisions

Melee, movement, self-centered, passive, corpse and buff skills retain zero.
Homing Guided Arrow/Bone Spirit/Unstoppable Force do not receive speculative
destination offsets. Shock Wave stays zero at the user's request. Telekinesis
uses native unit/item/object selection and remains outside coordinate leading.

Ground area/summon/trap skills retain current-position placement: the speed of a
later summoned projectile does not establish where its summon should be placed.
Meteor/Blizzard and similar delayed effects have no distance-based projectile
estimate; their existing snap aim still works. Mirrored Blades is a mixed
weapon attack with no direct reviewed missile mapping, so its baseline is zero.
Blade Warp retains its movement destination. Reimagined Arctic Blast uses a
stationary starter and radial child, so vanilla channel timing is not applied.
Eldritch Blast's nova and Cleave's melee range do not justify ranged lead.

## Variant and configuration boundaries

These are **Reimagined-oriented defaults**. IDs remain authoritative; comments
include both installed labels and catalog labels where they differ. The plugin
does not discover replacement behavior automatically. A vanilla or different-mod
installation should review the replacement IDs before opting those skills in.
Variant-specific behavior detection is deferred at the user's request.

Reimagined replaces vanilla passive IDs 18, 134, 136 and 141 with active skills.
Their packaged settings become `false` (off, R3-toggleable) instead of locked
`"disabled"`. IDs 136 and 141 also receive explicit snap targeting overrides.
Actual passive IDs such as Critical Strike remain locked. No skill is newly
enabled by default. Existing local user enable states and explicit targeting
choices should be preserved when merging the package.

The requested defaults are deadzone 0.22, initial speed 8.0, maximum speed 48,
acceleration 0.35 seconds, ground color `#C2B596`, lock color `#CC9C52F2`, and
reticle thickness 2.0. Compiled motion/style fallbacks match these values.
Missing leading entries in an existing config still mean zero; new baselines
are not silently imposed on a user's saved configuration.

## Validation

The user confirmed rev.70's first-A NPC interaction fix and improved Shock Web
leading. Existing confirmations apply to the tested skills and scenarios, not
every skill, offline combination or session-exit path. Automated build/package
results are recorded in the changelog. No crash-fix claim is made.
