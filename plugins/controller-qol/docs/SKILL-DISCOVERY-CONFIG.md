# Shared preferences and per-mod skill profiles

Rev.75 separates general controller preferences from all per-skill choices.
This is the current configuration guide for both fresh installs and upgrades.

## Which file to edit

| File | Purpose | User editable? |
| --- | --- | --- |
| `config/controller-qol-updates.toml` | `[qol]` and general `[aim]`: speed, deadzone, acceleration, reticle appearance, snapping radius, logging and master toggles | Yes |
| `config/controller-qol-skills/<scope>.overrides.toml` | All skill enable states, targeting modes, lead estimates and prediction limits for the active mod or Vanilla | Yes; R3 saves here |
| Matching `<scope>.catalog.toml` | Generated source names/IDs and reference defaults | No; never read as active settings |
| `config/controller-qol-skills/migration/legacy-skills.toml` | Import archive from the old main file, used to seed profiles not yet created | Recovery/import only |
| Release files under `defaults/` | Examples of shipped shared and skill defaults | Reference only; do not copy over customized files |

Live `config/` paths above are relative to the loader's active `d2rloader/`
directory. Release `defaults/` paths are relative to the ZIP extraction root. The loader may
select different config scopes for global and pack-installed plugins. Shared
preferences apply to profiles using that main file; this change does not merge
separate loader installations' configs.

Open the skill profile whose `# Mod:` line identifies the pack or `Vanilla`.
Despite the `.overrides.toml` extension retained from rev.74, it now contains the
complete editable skill settings, not just R3 changes. Existing values win;
startup adds missing values without replacing choices already present. A profile
is shared by characters using the same mod and config scope, not stored per save.

The scope fingerprint uses the canonical mod directory and mod identity. Vanilla
uses the config directory and `Vanilla`. Pack revisions retain the same profile;
moving or renaming the directory selects a new one. Deliberately copy a profile
if it should follow that move. Do not rely on filenames alone to identify a pack.

## Everyday editing and updates

1. Close the game before manual edits. Keep speed, deadzone, colors, logging and
   display smoothing in the main file; these apply across profiles that use it.
2. Start the game with the intended mod once to create/migrate its skill profile.
   Close it, then find the `.overrides.toml` whose `# Mod:` header matches.
3. Edit the existing numeric skill key in that profile, or use R3 on the focused
   supported skill in the class/General Skills tab. Do not add duplicate keys.
4. Restart after manual edits. R3 saves and applies a successful toggle immediately.
   Editing `.catalog.toml` or the ZIP's `defaults/` files does not change gameplay.

For example, shared `[aim] overlay_smoothing_ms = 60.0` smooths the displayed
reticle as the character/camera moves. It does not change cast coordinates or
projectile leading; direct stick adjustments remain immediate. Zero disables it,
and larger values can add visible tracking lag. Rev.75 uses 60 ms for new configs
and omitted keys. An explicit previous value remains yours, as do custom colors,
speeds and every saved skill choice.

DLL updates add missing profile settings but do not refresh existing keys to new
defaults. To adopt a changed default, compare the release reference and change
only the desired active key. Do not copy the entire defaults file over a profile.

## Enable state, targeting and leading

`true` enables QOL aim, `false` keeps native targeting but permits R3, and
`"disabled"` locks aim off and removes the R3 hint. Class sections organize the
240 built-in numeric IDs; extra IDs belong under `[aim.custom]`. Equipment-granted
class skills use the same ID and setting on any class. Numeric IDs, not names or
button positions, identify a skill.

`[aim.targeting]` selects `"ground"` (cursor destination) or `"snap"` (eligible
nearby enemy, with cursor fallback). This does not enable the skill or bypass
shared `snapping_enabled`. `[aim.leading]` estimates milliseconds per tile;
zero turns prediction off. `[aim.leading_max_ms]` and `[aim.leading_max_tiles]`
limit prediction. These entries matter only for enabled skills that support
snapping/leading. Discovery does not calculate a projectile's flight time.

`[aim.whirlwind]` holds `whirlwind_pass_through_enabled` and
`whirlwind_pass_through_distance` (0–15 tiles). These skill-specific preferences
also migrate from their former location under shared `[aim]`. Defaults remain
`true` and `1.5`; false or zero distance targets the enemy directly.

For recognized Reimagined Warp, discovery supplies ID 429 with ground targeting.
It starts off unless a saved preference enables it. R3 writes `"429" = true`
under the profile's `[aim.custom]`; the profile retains its ground mode under
`[aim.targeting]`. A `false` entry in the generated catalog does not turn it off:
the catalog is not an effective-settings report and is never loaded as config.
Commented examples also have no effect. Packs without readable source data can
manually declare Warp's enable entry and ground mode in their skill profile.

Edit files with the game closed and restart to reload manual changes. R3 takes
effect immediately after a successful save. A failed save leaves runtime state
unchanged. Profiles reject duplicate keys/IDs, unknown sections and invalid
values. Invalid skill settings stop aim initialization with a warning; other QOL
features remain available. No automatic reset to an empty profile is attempted.

## Migration and recovery

When aim initializes, rev.75 loads the embedded skill defaults and optional mod
source, then prepares the active profile. For an old main config, the previous
main-file choices and mod R3 overrides determine the migration result. Previously
omitted options retain their old effective values; migration does not silently
adopt new leading estimates or enable a skill. R3 overrides take precedence over
the corresponding old main-file keys during this one-time import.

The plugin preserves an import archive, fills missing profile keys, saves the
profile, reads it back and compares every skill's enable/lock state, targeting,
lead estimate, prediction limits and Whirlwind preferences. Only after that comparison succeeds does
it remove the old skill sections from the main file. Remaining shared setting
bytes and comments are preserved. Comments attached to copied skill keys follow
them; complete original documents remain in backups.

The archive supplies the old shared skill choices when another mod's profile is
created later. A current schema-2 profile no longer reads the archive for active
settings. An interrupted migration can resume: a complete profile may exist
while the old main sections remain, and their effective settings must still
validate. Conflicting legacy snapshots stop migration instead of overwriting the
archive. Restoring an older DLL also requires restoring its matching main config
backup, since it does not understand the new profile layout.

Migration changes to main/profile files use a unique temporary file, flush, then
`ReplaceFileW` with a unique `.bak` copy beside the original. Snapshot checks
reject detected concurrent edits. This is not a multi-file filesystem transaction:
a failed main-file replacement can leave a verified new profile and the intact
old main config. Original files/backups permit recovery; aim does not initialize
with an incomplete migration. Do not edit while the game is running.

From rev.75-beta.2, R3 saves keep one rolling `<scope>.overrides.toml.r3.bak`
containing the immediately preceding saved profile. The backup and current
profile each use flushed temporary files and atomic replacement; no-op saves
do not rotate the backup. If the backup cannot be updated, the toggle is not
saved or applied. Migration recovery copies and the import archive are never
pruned by R3 saves. Earlier beta timestamped backups are not automatically deleted.

Shipped skill defaults are a separate private `RCDATA` resource, ID **4102**.
The loader's embedded default config remains resource **0x03EA** and contains
only shared preferences. The manifest remains **0x03E9**. These are plugin-owned
resource contracts, not game addresses. The artifact test reads and validates
both config resources from the built DLL.

## Discovery boundaries and evidence

SDK ABI 4 context fields `activeMod`, `modDirectory`, and `pluginConfigPath` are
accessed only after `HasContext`. Candidate source paths, in priority order:

1. `<modDirectory>/data/global/excel/skills.txt`;
2. `<modDirectory>/<activeMod>.mpq/data/global/excel/skills.txt`.

No other installed packs are searched. Compressed MPQs, vanilla without a loose
active-mod source, missing sources, unknown/malformed schemas, duplicate IDs,
or capacity overflow retain manual skill profiles; automatic discovery is skipped. The source is read once at plugin
startup (8 MiB maximum). The current settings union is bounded to 1024 entries;
config/profile/catalog documents remain bounded to 65535 bytes. No per-frame
file scans or verbose aim logging are introduced.

Source fields used are `skill`, `*Id`, `skilldesc`, `passive`, `warp`, and
`srvdofunc`. An ID with a description is a catalog candidate, not proof of a
player-castable skill. Native utility IDs 0-5, 357-364 and 370 are excluded.
New candidates default off, passive candidates are locked. Only the previously
reviewed Warp signature (429, name Warp, warp=1, srvdofunc=27) supplies a new
ground-target default. No generic projectile lead or replacement semantics are
guessed. Existing reviewed class policies remain unchanged, and explicit settings
win. This discovers available configuration candidates, not just equipped skills.

Automatic IDs additionally require runtime confirmation before aim/R3 admission:

- SDK `DataTableService` v1, `findRowById`, `TableId::Skills` (12). A successful
  row lookup must have a valid view, matching table/bank, non-null row, nonzero
  row size and revision. Rotw/Lod/Classic are checked; this confirms presence,
  not the character's bank or source/runtime semantic equivalence. No raw row
  field is decoded and no row pointer survives the call.
- `LifecycleService` v1 `DataTablesLoaded` invokes the check on its documented
  game thread, after loader table processing. An initial `runOnGameThread` is
  attempted for tables loaded earlier. Remote clients can lack this scheduler.
- Independently, the existing guarded, visible class/General Skills tree reader
  confirms copied IDs on the UI callback. Its native witnesses are recorded in
  [General Skills](GENERAL-SKILLS-OSKILLS.md). This allows remote use without an
  authoritative local scheduler.
- The existing guarded `ReadView` active-skill request also confirms its copied
  ID before policy checks and before preview fallback. Saved discovered skills
  therefore do not require opening the tree on every remote launch. This reuses
  `Native::ActiveSkillRva` (game RVA `0x1440D0`, existing exact-byte profile for
  3.3.93787); no additional native read/call or hook is installed.
  Manually declared IDs remain supported even without source data.

The SDK confirms IDs only. Labels/passive metadata remain source observations;
compiled-only changes by another plugin are not automatically decoded. A table
or widget mismatch cannot enable an unknown skill. Services/listeners are owned
by the loader, callbacks check shutdown, and published confirmation flags are
atomic; baseline settings remain immutable during gameplay.

## Release extraction and maintenance

The runtime ZIP contains `defaults/controller-qol-updates.toml` and
`defaults/controller-qol-skills.toml`, with no live config paths. The DLL remains
under `d2rloader/plugins/`. The loader creates a missing main config from embedded
shared defaults; QOL creates a missing skill profile when aim initializes.
Existing preferences are preserved during updates. Compare individual defaults
when opting into a changed preference; do not replace entire user documents.

`tools/expand_skill_catalog_local.py` now regenerates `skill-defaults.toml`, not
the shared config. It remains a developer tool that resets reviewed defaults;
it must not be run against deployed user profiles.

## Validation

Automated checks cover separate embedded resources, unchanged shared defaults,
profile migration equivalence, source/schema limits, restart persistence, R3,
vanilla and mod separation, preserved tuning/comments, backups, invalid files,
existing beta R3 preferences and recovery after interrupted main-file replacement.
Runtime ZIP checks verify extraction cannot overwrite live configs. The user
confirmed the profile layout, bounded R3 backup behavior and improved reticle
motion in live Ladder testing. General Skills Warp and Multi Shot were also
exercised. This is not exhaustive validation of every mod, offline combination,
manual recovery scenario or equipment layout. Full-Ladder exit crashes remain
unresolved; see the rev.75 production and crash-triage records.

Install matching QOL versions in loader locations that share a main config. An
older DLL will not read these complete skill profiles after migration; keep its
main-config backup if rolling back.
