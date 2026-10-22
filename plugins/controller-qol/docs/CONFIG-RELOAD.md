# Controller QOL configuration reload

Rev.82 reads the existing active `controller-qol-updates.toml` and, when aim
prepared one at startup, the active `controller-qol-skills/<scope>.overrides.toml`.
No new configuration file or loader API is required. See the
[player setting table](../README.md#reload-settings) for the exact live/restart scope.

## Reading and validation

`config_reload.cpp` owns one worker. It observes the combined file-existence,
size and last-write stamps every 200 ms, then waits for 650 ms without a change.
`/qol reload` requests a read without that delay. Existing `/qol` navigation
commands retain their handler; root command registration also works when native
navigation admission fails. Configuration paths come from the loader context
and the already selected profile, never a reference/default file.

The worker caps each file at 65,535 bytes and parses a complete candidate before
publishing anything. `live_config.h` checks exact QOL names, sections, types,
button aliases, ranges and duplicates. Existing aim/skill parsers validate their
schemas. Missing, empty, unreadable, oversized or malformed active files refuse
the whole attempt. The main TOML accepts a UTF-8 BOM; CRLF and comments remain
supported. File stamps are
checked again after reading both files; an editor/R3 write during preparation
discards the candidate and retries. A refused automatic version logs once;
another save or explicit reload retries it. Removing a valid key selects its
ordinary default. Reload never writes configuration, makes backups, recreates
files, refreshes catalogs or repeats migration.

## Applying and lifetime

Root settings and aim motion settings use owned immutable snapshots. Callbacks
copy their current settings; startup settings remain immutable. `LiveSkills`
publishes a replacement baseline while retaining runtime confirmation bits for
discovered skills. Unknown/unconfirmed IDs gain no native admission from reload.

The worker waits for queued item requests, identify/transfer/belt/material/bulk
operations and synthetic holds. The input mutex then admits a commit only when
cached physical buttons/triggers, native buttons, raw button histories and
relevant keyboard/mouse modifiers are released. It reads published input samples;
it does not acquire the native callback gate while holding the input mutex.
Queued pickups retain their captured distance. No file I/O runs under the input
mutex or inside native detours.

Profile reads, R3 saves and reload publication share the profile mutex and a
revision counter. A successful R3 save publishes its toggle under that mutex.
Commit uses a nonblocking profile lock so input never waits for an R3 file write.
If it overtakes a prepared reload, the stale candidate is discarded and the newer
file is read again. Clearing obsolete session overrides retains confirmation
flags. Motion/root publication follows successful profile publication; all parsing
and allocation finishes before commit. Settings readers may finish with their
previous owned snapshot safely.

Restart-only fields are compared with the immutable startup values, retained
at those values and listed in the success log. Reload never calls initialization,
native admission, hook installation/removal, service registration or profile
migration. Existing exact-build native guards and pass-through failure behavior
are unchanged. No new native address, RVA, layout or ABI was discovered.

The reload worker joins before aim/input/item teardown. Published settings remain
owned for late resident pass-through callbacks. If worker startup fails, the
startup configuration stays active and the log asks for a restart after edits.

## Validation boundaries

`qol-config-reload` exercises the actual coordinator and worker with isolated
existing-file fixtures: stable automatic saves, console reload, root/profile
error retention, deleted/recreated files, held controls, busy actions, stale R3
publication, restart masks and joined shutdown. It creates no extra config file.
`qol-aim-gating` checks the production aim prepare/apply path against hook-count
fixtures. `qol-skill-discovery` checks production profile publication, targeting,
leading, Whirlwind, preserved confirmation, no reload writes and R3 revision races.
The complete compatibility suite remains required; see [validation](VALIDATION.md).

These are offline automated checks. Live automatic/console edits, reticle changes,
held-button release, item operations, hosted play and save persistence require
separate game acceptance. Replacing the DLL is still a closed-game operation.
