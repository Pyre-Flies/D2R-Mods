# Controller QOL 1.3.1+rev.64-beta.1

This is a **beta / GitHub prerelease**, not a stable release. It contains only
Controller QOL; Item Roll Ranges and Map Assistance are not updated here.

## Changes

- Add guarded remote-client ground pickup, item identification, stash/Cube
  transfers, advanced storage actions and deposit-all. Preserve existing
  authoritative/offline routes.
- Support the reviewed Ladder Global Chat and Maps hooks without replacing or
  bypassing them. Offline and non-Ladder installations require neither plugin.
- Fix competing ordinary A pickup, empty-cell targeting, Shared potion input,
  embedded Cube actions and queued-request cleanup.
- Add vendor LB+R3 on Identify and Town Portal scrolls to fill matching carried
  tomes through native purchasing. Gold and native capacity rules still apply.
- Fix slow right-stick movement when repeated clock timestamps reset acceleration.

## Validation and known limitations

All 28 automated suites pass locally, including DLL ABI/exports, forwarding
guards, original paths without Ladder plugins, and acceleration regression tests.
Private-server testing confirmed many ID/storage actions, improved cursor
movement and vendor tome filling; a successful fill was observed from 91 to 100
charges. This does not establish every edge case or long-term stability.

**Unexpected client crashes with the full Ladder setup remain under investigation.**
Recent crash records name game/loader code, Trade Notifications and Supporter
Portals. A causal connection to QOL has neither been established nor excluded.
The final beta still needs an offline smoke test and broader session/persistence
qualification. Hook compatibility is limited to the exact reviewed plugin builds.
Remote Identify All targets remain limited to Inventory; Cube/Shared ID supplies
are unsupported.

## Installation

Close the client and back up the existing DLL. Extract the ZIP into the game
directory; it installs under `d2rloader/plugins` and `d2rloader/config`. Preserve
customized TOML settings instead of overwriting them with the packaged defaults.
For a pack-managed Ladder installation, use the pack author's approved update
process; local package modifications may fail integrity checks.

`SHA256SUMS` verifies the ZIP, and the ZIP contains a second checksum manifest for
its files. No game binaries, crash dumps, local logs or credentials are included.
