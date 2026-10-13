# Controller QOL 1.3.1+rev.77-beta.1 - hosted compatibility fix

Please test this release and report all bugs. This Controller QOL-only prerelease
builds on stable rev.76; it does not replace the latest stable suite.

## Changes

- Freeze ground-loot shortcut assignments for each LB/modifier hold. Disappearing
  labels and new drops cannot replace original slots. Release and press again to
  refresh, including when the initial snapshot was empty.
- Recognize the inspected Ladder Global Chat and Maps forwarding hooks. The old
  guards blocked remote single/batch identify, ordinary transfers and stash-all
  after those plugins loaded. Preserve hooks, server validation, older supported
  builds and fail-open handling of unknown profiles.

## Offline compatibility and validation

Offline identify/transfer implementations, routing, configuration defaults,
Cube safeguards and Shared-page handling are unchanged from stable rev.76.
The intentional snapshot behavior applies to offline and hosted sessions.
All 33 local automated suites passed for the repair candidate, covering unhooked
routes, forwarding guards, local/guest isolation, input/session handling and DLL
ABI/exports. Release-source checks also run before publication. These tests do not
establish fresh live offline or hosted acceptance of this build.

Read-only live inspection confirmed compatibility mismatches and forwarding
contracts. Personal-stash LB+L3 captured 23 items and stopped before submission
on the existing build. Candidate success and persistence require testing through
an approved Ladder package. Walking to loot remains unresolved; this release
does not add an approach step or claim to fix pickup reach.

## Installation and reports

Pack-managed Ladder users must wait for the approved launcher/server package.
This GitHub prerelease alone does not authorize a local override or server use.
For independently managed installations, close the client, back up the current
DLL, and extract into the applicable loader root. The DLL is under
`d2rloader/plugins/`; reference settings are under `defaults/`, preserving live
configuration. Restore the prior DLL with the client closed to roll back.

Report game/mod/loader versions, offline versus hosted, controller type, action,
source/destination tab and relevant QOL log lines. Test single actions before
batches and check item state after rejoining. Do not include credentials or saves.

`SHA256SUMS` verifies the ZIP; its internal manifest verifies each packaged file.
GitHub supplies source archives for the tag. No game binaries, local logs, backups
or extracted assets are included. See `docs/HOSTED-QOL-2026-10-12.md` for evidence.
