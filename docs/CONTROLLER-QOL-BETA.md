# Controller QOL 1.3.1+rev.76-beta.4 — testing release

**Please test this release and report all bugs, including minor issues and
inconsistent behavior. Online play has not yet been validated.**

This is a Controller QOL-only **prerelease**. Non-online gameplay was accepted
after single-client Reimagined testing. Automated tests and that acceptance
do not establish hosted/guest correctness, every storage combination or all
game/mod/controller environments.

## Changes

- Block moving the Horadric Cube into itself through LB+X, LB+Y and ordinary
  Cube transfer routes. Keep valid Cube -> Inventory withdrawals available.
- Keep overlapping LB+LT/RT gestures inside Shared Stash, with no escape to
  the main stash tabs. Fully release both triggers before changing direction.
- Bind ground-loot requests to the current local player/session. Prevent old
  requests from crossing games and keep host filtering away from guest actors.
- Preserve consumed button/trigger ownership through modifier-first release;
  serialize shortcut state and make master-disable input pass through.
- Revalidate Cube focus/context before execution; capture normal Shared page,
  season and local owner for direct Shared -> Cube transactions.
- Remove packet-driven diagnostic ground scans. Retain existing aim behavior
  with local-player qualification; this is not full Guided Arrow consolidation.
- Include controller Deposit All stash-tab feedback and LB/RB Mod Information
  tabs, with scoped blocking of underlying LT/RT navigation.
- New/omitted `prioritize_chests` settings default to `true`. Existing explicit
  settings and customized configuration files remain preserved.

## Validation and known limits

- Release build: **33/33 CTest suites passed**, including **424 production
  integration checks** and **61 Shared-page/glyph checks**.
- User-confirmed: Cube self-insertion prevented, Shared Stash escape fixed,
  Cube behavior works through rejoins, and non-online gameplay accepted.
- Pickup retains its current reach and does not automatically approach loot.
  A reported two-of-three pickup required slight movement for the third item;
  no new native walking or pickup-reach fix is claimed.
- **Online play still requires testing.** Host and every joining client must
  use matching plugin sets, versions and DLLs. Mismatched peers cannot be used
  as a test. Distinct-peer ownership, latency and hosted persistence remain
  unverified. A single-client host is not a substitute for a second actor.
- Native features admit only their documented exact builds/byte guards and
  fail open on mismatches. This release makes no broad Ladder crash-fix claim.

## Report bugs

[Open an issue](https://github.com/Pyre-Flies/D2R-Mods/issues/new) for every bug
you encounter. Include:

- This plugin version, game/loader version, mod and offline/host/joined role.
- Controller/device and exact buttons, panels and steps used.
- Expected result, actual result and whether it repeats or survives a rejoin.
- Relevant QOL log excerpts and screenshots where useful; omit account/session
  secrets and private paths. For online issues, include matching plugin versions
  on both sides.

See the [single-client acceptance cases](https://github.com/Pyre-Flies/D2R-Mods/blob/beta-controller-qol-2026.10.11.1/plugins/controller-qol/docs/AUDIT-REPAIRS-IN-GAME-TESTS.md)
and [repair evidence](https://github.com/Pyre-Flies/D2R-Mods/blob/beta-controller-qol-2026.10.11.1/plugins/controller-qol/docs/AUDIT-LIVE-FOLLOWUP-BETA4.md).

## Install and preserve settings

Close D2R/D2RLoader and back up the existing DLL. Extract the ZIP into the
appropriate loader root: `d2rloader/plugins/Controller QOL Updates.dll` is
the only installed plugin. Keep existing configs and skill profiles.

The TOMLs under `defaults/` are references; the ZIP contains no live
`d2rloader/config/` paths and does not overwrite customized settings.
The loader creates missing main settings from embedded defaults. Review the
included configuration guide before changing per-mod skill profiles.

The adjacent `SHA256SUMS` verifies the ZIP; its internal manifest verifies
individual files. Source archives are supplied by GitHub. Other plugins and
the current stable suite release are unchanged.
