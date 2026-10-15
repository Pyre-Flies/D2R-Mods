# D2R Mods release 2026.10.14.2

Item Roll Ranges **1.3.1+rev.44** improves range and source attribution across
vanilla and modded items. Actual tooltip values and native localization stay
intact; definitions come from the current game tables.

- Resolve shared resistance rolls and overlapping single resistances, plus
  grouped unique Attributes and supported weighted property choices.
- Read all active runeword recipes from ordered sockets, eligible bases and
  word-owned stats. `[RW]` identifies word properties; runes keep socket sources.
  No vanilla/Reimagined runeword stats database is shipped.
- Show independently rolled elemental-damage endpoints and guarded recurring
  replenishment intervals from their active parameters.
- Label random skill/class/tab properties using their actual bonus strength.
  For example, Gnostics' selected Wake of Fire shows `[+5]`, with the source and
  tier. Whole-family constraints prevent attributing a bonus selected elsewhere.
- Retain native output for ambiguous, unsupported or budget-limited sources.
  No new hook or arbitrary formatter composition is introduced.

**Always-show remains false by default.** Hold Ctrl or RT/R2 (R1/RB with the
active Controller QOL range remap), or opt in with `/AlwaysShowRolls` for the
session. Saved settings are preserved. Reference TOMLs are under `defaults/`;
the archives contain no live `d2rloader/config/` paths.

| Plugin | Version |
| --- | --- |
| Item Roll Ranges | 1.3.1+rev.44 |
| Controller QOL Updates | 1.3.1+rev.78 |
| Map Assistance | 1.3.1+rev.1 |
| Inventory Deposit | 0.1.0-beta.13 |
| Floating Damage | 0.1.0-beta.12 |

The other four plugins retain their released source and versions. Each ZIP is
independently installable; Inventory Deposit and Floating Damage keep beta status.

## Validation and limits

Nine Item Roll Ranges suites pass, including artifact/ABI, cloned and clone-less
formatting, shared/ambiguous source proofs, selector families and refusal paths.
Private captured-data replays inspect2,026 affix rows, resolve219/219 enabled
Reimagined recipes and verify56 random-selector specifications. Exact captured
charm and amulet metadata replays pass. Raw captures are not included.

User checks confirm Prismatic overlaps, grouped Attributes, Bone, Lightning and
Honor in the Reimagined setup, Raven Song fire endpoints, replenishment display,
Ctrl/always-show behavior and Gnostics' Wake of Fire. Subsequent vanilla smoke
checks were also reported working. These sampled visible results are separate
from catalog lookup/model coverage; every item/localization/third-party plugin
combination and replenishment stopwatch timing are not exhaustively validated.
Changing enchantment recipes, conditional set bonuses, ambiguous conditional
groups and unsupported transforms retain their documented attribution limits.

Requires the qualified D2R3.3.93787 / D2RCore1.3.1-beta family described in the
included compatibility records. Exact artifact/code guards fail open on a
mismatch. Close the game before replacing DLLs; install one active copy and
back up the prior DLL. Managed mod packs distribute through their package route.
The release workflow rebuilds/tests all five packages and verifies archive
layout/checksums. Publication is blocked if a required check fails.
