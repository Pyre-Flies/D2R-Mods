# Controller QOL configuration

## Find your active files

| Path under `d2rloader/config/` | Purpose |
| --- | --- |
| `controller-qol-updates.toml` | Item actions, general aim controls and reticle appearance. |
| `controller-qol-skills/<scope>.overrides.toml` | Skill choices for one mod or Vanilla. Read its `# Mod:` header to select the right profile. |
| Matching `<scope>.catalog.toml` | Generated reference only; editing it does not change your settings. |

Start the intended mod once to create its profile. Close the game before manual
edits and restart afterward. Change existing keys rather than adding duplicates.
Files under the ZIP's `defaults/` directory are examples, not active settings.
Existing settings survive updates. Profiles follow the mod/config location;
moving a mod directory can select a new profile. Keep backups before relocating
or restoring older versions.

## Common settings

In `controller-qol-updates.toml`, these examples change existing keys:

```toml
[qol]
modifier = "bumper"            # LB/L1 for item shortcuts
ground_pickup_button = "bumper" # LB/L1 for ground-loot assignments
block_filtered_pickup = true   # Controller A cannot collect hidden-label loot
quick_deposit = true           # Enable LB/L1 + L3 bulk deposit at the stash

[aim]
enabled = true                # Enable optional controller aiming
deadzone = 0.22                # Ignore small right-stick movement
initial_speed = 8.0            # Initial cursor speed in world tiles/second
maximum_speed = 48             # Maximum speed while the stick is held
overlay_smoothing_ms = 60.0    # Smooth reticle movement; 0 disables smoothing
ground_reticle_color = "#8FE8FF"
lock_reticle_color = "#FFD166"
```

Set `[aim] enabled = false` to disable right-stick aiming. Set `[qol] enabled =
false` to disable the plugin's actions and aiming. `modifier` and
`ground_pickup_button` are independent. The full reference TOML lists the
remaining action and appearance controls.

## Per-skill aiming

Focus a supported skill in its class or General Skills tab and press R3 to
toggle aim. A successful change applies immediately and saves to the active
`.overrides.toml`. A failed save leaves the previous state active.

For manual editing, use the existing numeric skill ID in that profile. Example
for vanilla Sorceress Teleport (ID 54):

```toml
[aim.sorceress]
"54" = true

[aim.targeting]
"54" = "ground"
```

`true` enables aiming, `false` leaves it off but permits R3 toggling, and
`"disabled"` locks it off. `"ground"` targets the cursor; `"snap"` uses an
eligible nearby enemy with cursor fallback where supported. Mods may replace
skills: consult the active profile/catalog names before editing an ID.
Targeting entries do not enable a skill by themselves. Prediction and
Whirlwind settings also belong in the skill profile, not the main file.

## Optional keyboard controls

These appear under **Controller QOL Aim** in the loader controls menu:

| Default key | Effect |
| --- | --- |
| F8 | Toggle aiming for this session, if allowed by configuration. |
| F9 | Recenter the cursor ahead of the character and release the target. |
| F10 | Invert right-stick Y for this session. |

These shortcuts are not chat commands. Manual config changes require a restart;
R3 saves skill choices immediately. Reticle visibility and aim enablement are
separate settings.
