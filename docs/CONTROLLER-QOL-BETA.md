# Controller QOL 1.3.1+rev.79-beta.2

Testing prerelease for item transfers and identification on a separate remote
host. Please test and report all bugs; live remote behavior and save persistence
have not yet been validated for this beta.

## Changes

- Repair QOL actions blocked after unrelated Global Chat updates. Validate the
  actual non-chat forwarding path and its helpers, ownership and original Core
  continuation, allowing relative references to relocate safely.
- Cover remote LB+X stash transfers, LB+L3 advanced-storage deposits, ordinary
  LB+Y Cube transfers, Quick Identify and Identify All through their shared guard.
- Reject unknown forwarding behavior or invalid runtime memory even when the
  companion's disk file has a previously recognized hash.
- Log why the sender was refused without implying an already-sent request was
  never submitted. Existing item identity, session, destination, capacity and
  duplicate-submission safeguards remain in place.

The runtime repair is based on stable rev.78. Settings, embedded defaults,
controller mappings, offline/local routes, belt refill and ground pickup retain
their existing implementations. This beta publishes Controller QOL only.

## Remote-host test checklist

Use the normal launcher/server package update route for a managed pack. Restart
the client and confirm Controller QOL reports `1.3.1+rev.79-beta.2`. Keep existing
configuration; files under `defaults/` are references only. Coordinate any
matching host/client plugin version requirement through the pack maintainer.

1. **LB+X:** Start with a low-value item. Deposit and withdraw through Personal
   and the selected Shared page, then check supported advanced-storage and Cube
   contexts. Confirm the exact destination and unchanged item properties.
2. **LB+L3:** Deposit a small inventory batch of eligible materials, gems, runes
   or rejuvenations. Check both source removal and the actual destination totals.
3. **LB+Y:** Transfer an ordinary item into the carried Cube and back through the
   supported storage controls. The Cube itself must remain protected from insertion.
4. **LB+A identification:** Identify one eligible item, then use an eligible
   highlighted Identify Tome for Identify All. Confirm one charge is consumed per
   identified item and the batch covers main inventory only on a remote host.
   Supported remote targets/supplies are Inventory and open Personal Stash;
   Cube/Shared supplies and modded scroll stacks remain excluded.
5. **Refusals and regression:** Check a full destination and insufficient identify
   charges. Check that changing the stash tab or leaving the game stops queued
   work. Confirm existing LB+R3 belt refill still works.
6. **Persistence:** Leave and rejoin, then verify item locations, stored totals,
   identified state and remaining tome/scroll charges. Check the outcome before
   repeating an operation that stopped or timed out.

For reports, include the shortcut, source/destination and selected page, expected
and actual result, companion versions, whether it survives rejoining, and the
relevant QOL log excerpt. Redact account, connection and credential information.

## Validation and limits

All 33 local QOL suites passed, including 483 integration checks, 160,395 byte
mutation refusals, 1,275 executable forwarding cases, and real Windows image/page
ownership checks. The production guard accepts all three inspected Global Chat
images and rejects altered forwarding helpers. All 16 current runtime-packaging
checks passed. Automated tests do not establish
remote destination visibility, server persistence or live identification success.

New forwarding instructions, compiler output or additional detours can still
require a reviewed compatibility update. Unknown paths remain refused.
See the [compatibility record](https://github.com/Pyre-Flies/D2R-Mods/blob/beta-controller-qol-2026.10.15.1/plugins/controller-qol/docs/REMOTE-SENDER-COMPATIBILITY.md)
and [complete controls/configuration guide](https://github.com/Pyre-Flies/D2R-Mods/blob/beta-controller-qol-2026.10.15.1/plugins/controller-qol/README.md).
