# Inventory Deposit 0.1.0-beta.13

First public prerelease of the standalone **Deposit All** inventory button.

- Deposit eligible gems, materials, runes and rejuvenation potions into advanced
  storage from a mouse button beside the gold display while the stash is open.
- Custom stone-and-gold artwork is embedded in the DLL. No MPQ, loose image
  files or Controller QOL runtime dependency.
- Gold-relative placement follows the loaded inventory layout and leaves the
  existing Charm Inventory panel resources intact.
- The button greys out with no eligible items, while holding an item, or during
  a batch; it highlights on hover when available.
- Each affected stash tab receives the existing supplemental animation once
  per batch. Tooltip: **Deposit All Gems, Materials and Runes**.

The user confirmed these UI behaviors and deposits in the reviewed Reimagined
setup. Release builds run the plugin's twelve automated suites plus package
checks. Remote play, cross-plugin simultaneous batches, low-end rendering,
multiple resolutions and persistence are not fully validated.

Requires Windows x64 and the reviewed D2R `3.3.93787` / D2RLoader `1.3.1`
profile, including Core SHA-256
`2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`.
Native guards disable the optional feature on an unsupported profile.

Close the game and extract the runtime ZIP into the game directory, or install
the DLL into one active mod scope. The DLL belongs in `d2rloader/plugins/`;
reference settings stay under `defaults/`, preserving existing live configs.
The archive includes installation, rollback and compatibility instructions.

Download the ZIP and verify it against the attached `SHA256SUMS`. Source and
original editable artwork are included in the tagged repository source archive.
