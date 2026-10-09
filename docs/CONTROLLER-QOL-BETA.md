# Controller QOL 1.3.1+rev.72-beta.1

This is a **GitHub prerelease** of Controller QOL only.

## Rev.72 logging update

Detailed aim logging now requires `[aim] verbose = true` and a restart. The
packaged default is `false`; existing configs that omit it also stay quiet.
Quiet mode skips diagnostic formatting and logging-only counters/reads as well
as file writes. It preserves aiming, NPC selection checks and cast state.
Startup status and compatibility/save-failure warnings remain visible.
The debug HUD, general QOL debug logs and portal diagnostics are separate flags.

This targets unnecessary work on handhelds. Steam Deck frame-time improvement
has not yet been measured; the update is not a claim to resolve every slowdown.

## Included changes since rev.64 beta

- Improve first-cast Telekinesis object/item targeting and snapping for lobbed
  skills such as Fire Blast and Ice Barrage.
- Add bounded projectile leading from observed enemy motion, with independent
  per-skill travel estimates and limits. Ship 42 projectile estimates and explicit
  zero-lead baselines for the remaining 198 catalog skills.
- Preserve the tuned Ice Bolt/Ice Blast limits and Holy Bolt, Miasma Bolt,
  Blade Fury and Shock Web values. Add initial estimates for Inferno, Frozen Orb,
  Shield Throw, Winter's Gambit, Chasm Break, Firestorm, Molten Boulder,
  Echoing Strike and Miasma Chains.
- Restore nearby chest/object priority on native A and preserve native NPC
  scoring after snap aiming so interaction does not require a second press.
- Set aim defaults to deadzone 0.22, initial speed 8.0, maximum speed 48,
  acceleration 0.35 seconds, ground color `#C2B596`, lock color `#CC9C52F2`,
  and reticle thickness 2.0.

Remote item actions, vendor tome filling, offline routes and Ladder hook
coexistence from rev.64 remain included.

## Configuration and scope

Defaults are **Reimagined-oriented**. Numeric skill IDs do not detect whether a
mod replaced the skill. Automatic vanilla/Reimagined differentiation is deferred.
Replacement names are annotated; review those settings for other variants.
Four replaced passive IDs become off-but-toggleable, and Winter's Gambit/Chasm
Break receive snap overrides. The same ten skills remain enabled by default.
Leading values do not enable skills: use R3 or the class sections to opt in.

Frozen Orb's baseline adjusts parent direction, not burst placement; Molten
Boulder has no separate startup-delay model. Homing, movement, melee, stationary
area, summon and trap placement retain zero lead. Slow Missiles compensation is
outside scope. Leading falls back to current-position aim on unreliable motion.

## Validation and limitations

Local automated validation covers all 28 suites, DLL ABI/exports/defaults,
category isolation, prediction bounds, R3 persistence and package integrity.
Rev.72 also checks strict verbose parsing, quiet defaults, zero hot-path log
writes/counters, bounded verbose output and unchanged enemy/NPC scoring.
User testing confirmed improved Telekinesis/Fire Blast targeting, several
projectile families, chest priority, first-A NPC interaction and Shock Web lead.
The nine newly added estimates still need ordinary gameplay testing; these
checks do not establish all offline scenarios or long-term session stability.

Unexpected client exits in the full Ladder setup remain under investigation.
This revision makes no crash-fix claim. Native compatibility is limited to the
reviewed builds and guarded paths. Remote Identify All targets remain limited
to Inventory; Cube/Shared identification supplies remain unsupported.

## Installation

Close the client and back up the DLL/config. Extract the ZIP into the game
folder: DLL under `d2rloader/plugins`, config under `d2rloader/config`.
Merge new leading maps and desired defaults into existing customized TOML files;
do not overwrite personal skill toggles unintentionally. Restart after updating.
Pack-managed Ladder installations must use their approved revision process.

The release `SHA256SUMS` verifies the runtime ZIP; its internal manifest verifies
individual files. GitHub also supplies source archives for this tag. Game binaries,
local logs, crash dumps, backups and credentials are not included.
