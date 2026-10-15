# Changelog

## Documentation

- Make the plugin README the complete user reference for commands, settings
  and editing examples. Package that same README to keep instructions aligned.
- Controller QOL has a dedicated full controls/skill-profile guide; release
  overviews link to it. Plugin DLLs and defaults are unchanged.

## 1.3.1+rev.2 - 2026-10-15

- Save default/always/off and bare/on/toggle visibility commands, as well as
  existing appearance settings. Restore saved visibility at startup and retain
  the last styled mode across off/on toggles. New installs still use default.
- Store mode in `enabled` and `always_visible` under `[always-show-sockets]`.
  Status stays read-only; save failures remain clearly reported.
- Four suites pass, including retained native chat dispatch, real startup
  settings reloads and save-failure checks. Live save/restart confirmation remains
  pending. Native hooks, resources and rendering guards are unchanged.

## Distribution documentation

- Keep runtime ZIPs minimal: one DLL, reference defaults and a usage README
  with command examples and required notices. Keep detailed guides, release
  history and engineering reports in the repository. Sanitize public documents.
- Packaging/privacy checks are automated; plugin DLLs and live-tested behavior
  are unchanged by this documentation update.

## 1.3.1+rev.1 — 2026-10-15 stable release staging

- Promote the accepted socket feature set to the shared D2RLoader version plus
  plugin revision scheme. Plugin metadata, Windows file/product versions, CMake,
  distribution docs and package names now identify 1.3.1+rev.1.
- Keep appearance autosave, default startup mode, 80% ring / 40% center / 65%
  artwork defaults and the existing exact executable/Core guards unchanged.
  This promotion adds no native hook, rendering or persistence behavior change.
- Allow stable loader+revision versions in packaging while rejecting beta-tag/
  stable-version mismatches. Stage the stable runtime/source/guide/checksums for
  the upcoming combined release; this does not publish a separate release.
- Automated persistence fixtures remain distinct from live validation: actual
  loader-backed saving and restart behavior still need a live check.

## 0.1.0-beta.15 — 2026-10-15 staged appearance autosave

- Automatically save `size`, `opacity`, `ring`, `center` and `lift` through the
  loader's config API. Update only the selected value while retaining comments,
  line endings, other appearance values and other sections. Insert missing keys
  inside the plugin's section. Save failures leave the session usable and report
  a warning instead of silently claiming persistence.
- Keep display modes session-only and start each launch in `/sockets default`.
  Remove the obsolete outline command and configuration-key alias; the supported
  outline control is `ring`. Refresh the guide and release documentation.
- Keep the accepted 80% ring / 40% center / 65% artwork defaults, inserted
  opacity 65%, lift 1 and all existing native rendering/compatibility guards.
- Four native suites pass, including SDK config callbacks, saved-value parsing,
  comment/BOM/CRLF preservation, no mode/status/invalid-command writes, rejected
  aliases and read/write/oversized-config failures. Repackage locally for the
  combined release; actual loader-backed saving still needs live validation.

## 0.1.0-beta.14 — 2026-10-15 staged combined-release candidate

- Default to 80% ring opacity and 40% black-center opacity. Keep artwork size
  at 65%, derived circle size at 80%, inserted opacity at 65%, lift at 1 pixel
  and startup mode at `default`. Preserve explicit saved values.
- Add a copy-ready configuration guide covering display modes, commands,
  opacity versus size, command defaults and saved settings.
  Package it with the runtime DLL and a reference TOML under `defaults/`.
- The user confirms beta.13's modes work. The live log records default/always/
  off transitions and the requested size 65/ring 80/center 40 combination.
  This candidate changes defaults and distribution docs, with native hooks,
  guards, ABI, resources and rendering calculations unchanged.
- Stage locally for a future combined release; no publication or tag is made.
- Release build and four native suites pass, including embedded defaults and
  fallback parsing. Six packaging regressions pass, including reference-only
  config layout, guide inclusion, safe paths, privacy and checksum enforcement.

## 0.1.0-beta.13 — 2026-10-15 local display-mode candidate

- Add `/sockets default` for normal native hover/selection visibility with the
  existing appearance controls, and `/sockets always` for continuous styled
  sockets. `/sockets off` restores native visibility and appearance. All modes
  retain the same session size, opacity, ring, center and lift values.
- Start each session in `default`, as requested. Keep `/sockets on` as an alias
  for `always`; bare `/sockets` or `toggle` disables/resumes the selected mode.
  Report the selected mode in status/log output. Keep native help below 64 bytes.
- Separate forced post-icon drawing from appearance enablement using the
  existing hooks. Default mode only styles native socket draws; always mode
  keeps its matching-duplicate suppression. No new native patch or resource.
- Release build and four suites pass. Retained chat dispatch exercises all
  three modes through original socket-call scope and real assembly adapters,
  including empty/filled slots, native-off passthrough, loose items, paired lift,
  draw counts, setting retention and unsupported-build refusal. The user accepts
  beta.12's full-size ring correction and subsequently confirms beta.13's modes
  work in game.

## 0.1.0-beta.12 — 2026-10-15 local full-size-ring candidate

- Keep the +15-point ring-size margin through the entire `/sockets size` range.
  Remove the native-size cap that stopped circles growing at artwork size 85:
  artwork 85/90/100 now gives circles 100/105/115. Apply this to both circle
  layers and empty/filled slots. Startup defaults remain 65% artwork, 80%
  circles, 1-pixel lift, 100% ring opacity and 35% black-center opacity.
- Restrict above-native scaling to the owned ring layers. Ordinary inserted-art
  controls remain 10–100 and native-circle fallback remains at most 100%.
  Keep existing center/pivot fitting, scale bounds, guards and hook sites.
- Release build and four suites pass, including retained chat dispatch through
  size 85/90/100, both ring layers, empty/filled slots and paired full-size rune
  alignment. Geometry covers circles through 115% across UI scales and atomic
  refusal above the size/scale bounds. The user's screenshot/log establish the
  prior cap's visible problem. The user subsequently confirms the correction
  works great; new native-hover styling is a separate beta.13 live test.

## 0.1.0-beta.11 — 2026-10-15 local chat-registration repair

- Fix beta.10 rejecting its entire `/sockets` chat command: the added lift help
  made the description 72 bytes, exceeding the native backend's 63-byte limit.
  Use one shorter shared help string and enforce its size at compile time.
  The existing render/lift calculations and debug-console controls are unchanged.
- Include the backend's specific refusal reason in registration warnings.
- Extend the command suite through real backend registration, deferred UI work,
  retained native dispatch and the assembly/render adapter. Verify chat-driven
  lift 0/1/2/8 moves inserted artwork while both circle layers remain fixed,
  and retain rejection of descriptions exceeding the native capacity.
- Release build and all four automated suites pass. The beta.10 live log records
  the registration warning and no command dispatch, matching the user's report
  that lift 0 and 8 look identical. The user confirms beta.11's lift command
  works in game and prefers 1 pixel; retain the existing 1-pixel default.

## 0.1.0-beta.10 — 2026-10-15 local terminology/alignment candidate

- Rename the displayed outline control to `ring`: `/sockets ring N`,
  status/help and saved `ring_opacity_percent`.
- Lift paired inserted artwork one rendered pixel by default, leaving both
  circle layers fixed. Add `/sockets lift N` (whole pixels, 0–8) and saved
  `inserted_item_lift_pixels`. Zero restores beta.9 geometric alignment; 1 and
  2 allow direct visual comparison. The screenshots suggest a small optical
  correction but do not establish a universal exact value for all artwork.
- Keep the existing four hooks, +15-point circle sizing, independent opacity,
  loose-item behavior and Quality Backgrounds admission contracts.
- Release build and four automated suites pass, including command-driven
  0/1/2/8-pixel paired-art movement with stationary circle layers, bounded
  configuration and native adapter preservation.
  The user accepts beta.9's appearance; its log records layered draws and
  opacity commands. Beta.10's optical correction needs visible live comparison.
  Quality Backgrounds remained disabled, so there is no new combined live test.

## 0.1.0-beta.9 — 2026-10-15 local independent-opacity candidate

- Increase the derived circle-size margin to +15 percentage points, capped at
  100%: the default 65% artwork now gets 80% circles. Keep paired alignment,
  which the user confirmed looks good in beta.8.
- Default to an opaque rim and 35% black center. Draw rim and center as separate
  native layers so the outline and center commands adjust each opacity
  independently from 0–100 in the current session, including invisible zero.
- Add independent saved outline (100) and center (35) opacity settings.
  Existing inserted-art size/opacity controls remain independent.
- Reuse the four existing hook sites and native sprite renderer. Guard the new
  direct center draw and use owned copies of settings/position; unknown guards
  or unavailable layers retain native circles and the prior centering fallback.
  No texture regeneration, new cache entries or extra resource registrations
  occur when commands change opacity.
- Release build and all four suites pass: artifact/config, real native assembly
  adapters/commands, geometry/config policy and layer pixels/resource rollback.
  Tests cover independent live values, 0/100, private draw-argument isolation,
  retained alignment and missing-layer/guard refusal. Layered beta.9 drawing
  and actual slash-command presentation still need a live check.

## 0.1.0-beta.8 — 2026-10-15 local shaded-ring/alignment candidate

- Restore the original procedural socket design for both empty and filled slots,
  with a 100%-opaque rim and a black center at 20% opacity. The outer area stays
  transparent. Ring shading is independent of inserted-item opacity.
- Center each inserted sprite on the immediately preceding circle's copied
  center. This addresses the reported rune/circle displacement using native
  dimensions and pivots rather than item-type or fixed screen-pixel offsets.
  Pairing is consumed once and isolated across nested socket draws.
- Retain artwork size +10-point circle sizing, capped at 100%, the saved size
  key, loose-item behavior, exact guards and existing four native hook sites.
  Restore beta.6's bounded resource-loader/known Quality Backgrounds forwarding
  contract. Unsupported custom-ring paths retain native sprites/opacity and
  the prior artwork-centering behavior.
- Release build and four suites pass: artifact/config admission, executable
  adapters/commands, geometry/config, and ring pixels/resource ownership. The
  actual generated sprite preview was inspected. Tests check exact 20% black
  pixels, opaque rim, pairing/no reuse/nesting and non-square rune centering at
  multiple UI scales. The user confirms alignment looks good in beta.8; actual
  paired-center execution/corrections are present in the log. This does not
  establish new combined Quality Backgrounds coverage.

## 0.1.0-beta.7 — 2026-10-15 local native-circle correction

- Restore D2R's original socket-overlay sprite as requested. Empty circles use
  35% opacity for a faint center; filled-circle opacity remains native. Remove
  beta.6's procedural ring, resource registrations and sprite-loader integration.
- Derive circle size from the existing artwork setting plus 10 percentage
  points, capped at native 100%. `/sockets size 65` now means 65% inserted art
  and 75% circles, preserving their centers and native socket spacing.
- Show separate artwork/circle percentages in command status. Keep the existing
  saved size key, inserted-art opacity, loose items and Quality hook sites.
- Release build and all three automated suites pass: DLL/config admission,
  native assembly/commands and geometry/config policy. Tests verify original
  sprite-pointer preservation, 35% empty alpha, filled native alpha, size offset,
  100% cap and disabled/classic/refused-feature passthrough. The obsolete custom
  asset suite is removed. The user reports off-center runes and insufficient
  center shading in screenshots; native admission/execution is present in the
  log. Appearance is revised in beta.8 rather than considered accepted.

## 0.1.0-beta.6 — 2026-10-15 local outline correction

- Replace the empty-slot full-opacity native sprite with an original thin ring
  whose center and exterior are transparent. This addresses the user's report
  that beta.5's whole-sprite opacity produced a solid black hole over the art.
- Generate HD/low-end RGBA sprites inside the DLL and register unique loader
  resource paths. Fit the ring to native bounds/center using the existing shared
  size command and saved setting. No extra runtime files or native hooks.
- Preserve filled-circle artwork/opacity and inserted-art controls. If outline
  resources, loader admission or geometry fail, retain dim native circles.
- Call the existing sprite-loader entry, including the exact admitted Quality
  Backgrounds forwarding detour. Unknown entry changes decline the custom ring;
  the original socket/scaling paths remain available.
- Release build and all four automated suites pass: DLL admission, executable
  native adapters/commands, geometry/config and outline pixels/resource rollback.
  Native asset loading and display are observed in the log and user screenshots.
  The user prefers the original D2R look, wants a faint center and reports the
  inserted artwork hiding the ring. The custom look is superseded by beta.7;
  this does not establish full beta.6 coexistence/visual validation.

## 0.1.0-beta.5 — 2026-10-15 local circle candidate

- Link socket-circle size to the existing `/sockets size` command and saved
  `inserted_item_scale_percent` setting. No separate circle-size control.
- Keep empty socket circles fully opaque while enabled, using the native
  per-position inserted-entry predicate. Filled circles retain native opacity;
  `/sockets opacity` continues to affect inserted gem/rune/jewel artwork only.
- Center circles with native sprite dimensions/pivots without changing socket
  spacing, item data, loose-item drawing or Quality Backgrounds' hook sites.
- Add a guarded circle-call adapter at 0x2a76ea. If that feature is unavailable,
  existing inserted-art controls and normal native circles remain available.
- Release build and all three suites pass, including executable circle adapter,
  filled/empty occupancy, shared command, full-size, disabled/classic and guard
  fallback checks. The user reports the full-opacity native sprite has an
  unwanted solid black center; this emphasis is superseded by beta.6.

## 0.1.0-beta.4 — 2026-10-15 local opacity candidate

- Fade inserted gem/rune/jewel artwork through the same temporary sprite-opacity
  input used by native ethereal drawing. Default: 65% opacity relative to native.
- Add independent `inserted_item_opacity_percent` (10–100) and session command
  `/sockets opacity 65`. 100 restores native opacity, including any existing fade.
- Retain 65% centered size, loose-item/ring appearance, item flags and Quality
  Backgrounds' integration points. No extra hook or shared opacity-state override.
- Three automated suites pass, including native-stack opacity forwarding,
  loose/disabled/classic fallback, multiplicative native fade and saved settings.
  User confirms transparency looks good, loose items remain normal, and the
  previously tested Quality Backgrounds combination continues to work.

## 0.1.0-beta.3 — 2026-10-15 local scaling candidate

- Center inserted gem/rune/jewel artwork at 65% of native HD size by default.
  Leave loose items, socket circles and classic-mode drawing at native sizes.
- Embed `[always-show-sockets] inserted_item_scale_percent` (10–100); accept
  `/sockets size 65` for a session adjustment. 100 restores native-size art.
  Existing on/off/toggle/status behavior remains available.
- Scope the lower sprite adjustment to synchronous socket rendering; preserve
  Quality Backgrounds' icon entry and caller semantics. Unsupported scale
  guards retain native-size socket rendering.
- Release build and all three automated suites passed: DLL/config admission,
  command/assembly adapters and scale/config/centering policy. Beta.3 loader
  admission and the scaling path are observed in the loader log. User confirms
  65% looks good and loose items remain normal size. Combined Quality Backgrounds
  behavior in this version remains unconfirmed. Opacity is unchanged in beta.3.

## 0.1.0-beta.2 — 2026-10-13

- Move socket augmentation from the item-icon function entry to its post-draw
  interior site, avoiding Quality Backgrounds' entry hook and preserving its
  original caller return-address checks. No Quality Backgrounds changes required.
- Preserve native volatile registers, SIMD registers, flags and stack alignment
  around the added socket draw. Retain duplicate suppression and `/sockets`.
- Release build, DLL admission, command and executable assembly-adapter checks
  passed (2/2). User confirms Quality Backgrounds and sockets render together,
  and `/sockets` toggles correctly in game.

## 0.1.0-beta.1 — 2026-10-12

- First standalone prerelease of the live-tested socket renderer and local
  `/sockets` chat-toggle candidate. No rendering changes from probe.2.
- Live-confirmed: empty inventory sockets, gem/jewel artwork on equipped armor,
  Cube, personal stash and shared stash. Chat toggle remains live-unconfirmed.
- Package only this plugin, installation/rollback instructions and checksums.

## 0.1.0-probe.2 — 2026-10-12

- Add native local chat `/sockets` to toggle, plus `on`, `off`, `toggle` and
  `status`. Bare debug-console `sockets` now toggles as well.
- Reuse Floating Damage's guarded native parser backend, including UI-thread
  registration retries and retained-callback shutdown. No external DLL needed.
- Preserve socket rendering hooks and session default enabled. Status output
  uses the existing debug console; state changes are also logged.
- MSVC Release build and both artifact/command adapter checks passed (2/2).
  Socket-specific native chat registration and live slash dispatch remain untested.

## 0.1.0-probe.1 — 2026-10-12

- Experimental native socket visibility after inventory item icon drawing.
- Reuse native empty socket and inserted-item art without changing selection.
- Guard exact loader/Core SHA-256 and both renderer entry byte windows; preserve
  native behavior if admission or either hook fails.
- Debug-console `sockets on/off/status` for live comparison.
- MSVC Release build and artifact admission/refusal test passed (1/1). Live rendering,
  controller, stash, vendors and scaling remain untested.
- User screenshot confirms automatically visible filled sockets on equipped
  armor without a tooltip. A second user-confirmed screenshot shows three empty
  socket circles on an unhovered inventory sword.
- User confirms the equipped armor displays both a gem and a jewel, and socket
  visuals also work in the Cube, personal stash and shared stash. Mixed inserted
  item types are confirmed; empty and filled sockets together on one item remain
  unverified. Controller, vendors, trade, classic mode and scaling remain untested.
