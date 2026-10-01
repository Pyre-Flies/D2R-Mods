## Controller QOL Updates 1.3.1+rev.51

- Integrate the controller aim prototype into QOL with a right-stick cursor,
  circular enemy snapping and subtle ground/enemy reticles.
- Support Teleport, Meteor, Blizzard, Hydra, Fire Wall, Guided Arrow, Multi Shot,
  Teeth, Leap and Whirlwind. Teleport/Leap remain ground-only; their idle marker
  previews the next snap-capable spell. Whirlwind supports configurable retained
  enemy pass-through, including an off toggle and extension distance.
- Preserve the existing QOL shortcuts and Guided Arrow correction when aim is off.
  Aim owns short-distance Guided Arrow coordinates only while active.
- **Aim now defaults enabled**, including existing configs with no aim section.
  Set `[aim] enabled = false` in `controller-qol-updates.toml` to disable it.
  Existing explicit false values remain respected. Cursor response, snapping, overlay and pass-through are
  configurable. Invalid aim settings disable only aim; exact native guards remain.

## Validation

Local automated suites pass: Controller QOL **23/23**, Item Roll Ranges **5/5**,
Map Assistance **2/2**. Prototype gameplay, reticles and post-Teleport lock preview
were user-tested. The newly integrated runtime still needs in-game validation;
automated checks do not establish compatibility with unreviewed game builds.

## Included unchanged

- Item Roll Ranges 1.3.1+rev.13
- Map Assistance 1.3.1+rev.1

## Installation and prototype migration

Close the game and loader, back up existing DLLs/configuration, and extract the
wanted runtime ZIPs into the game directory. Keep one active copy of each plugin.
Preserve customized configuration; the ZIP's `configuration/` file is a reference.

Aim is enabled unless explicitly disabled. Tune its `[aim]` section in
`d2rloader/config/controller-qol-updates.toml`; see the README for all tuning keys.
If migrating from Controller Aim Test, disable its DLL and copy its `[aim]` values
into QOL's configuration, adding `enabled = true`. Do not run both aim versions.
The old prototype config is no longer read. F8 toggles aim, F9 recenters, and F10
inverts Y while aim is enabled. Restart after configuration edits.

Target: Windows x64, D2RLoader 1.3.1 / ABI4; pinned SDK v0.3.0. See the bundled
rev.51 production record for exact-build guards and validation limits. Hot reload
is unsupported. Item Roll Ranges requires `d2rcore.items.item_stat_ranges = true`.
