# Item shortcut hints - 1.3.1+rev.10

QOL item tooltip contributions now require the current native controller UI mode.
Focus/identity tracking still runs for every tooltip; only the controller shortcut
text is gated. No item-transfer, input-remapping or other plugin tooltip code changes.

Reuses the existing admitted native input profile and NativeActiveIndex() query:
game RVA 0x3440170 holds the panel manager; manager+0xDC must equal 1, and
the active controller index at game RVA 0x2A23704 must be less than 8.
These are existing native bridge contracts, not newly discovered offsets.
Unknown/unavailable mode hides controller hints. Physical controller connectivity
and the cached input snapshot cannot establish whether the player is using mouse
and keyboard right now, so neither is used for this display decision.
No new hooks, patches, provider hashes or signatures.

Validation: build and existing regression suites; runtime mouse/controller switching
still requires user validation. Verify hints vanish on mouse hover with a controller
still connected, return when using controller, and normal item text/ranges remain.
