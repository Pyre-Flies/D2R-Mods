# Changelog

## Distribution documentation

- Package concise install/configuration guides, command examples and release notes;
  keep engineering reports in source documentation. Remove private paths and
  internal workflow references from distributable documentation.
- Packaging/privacy checks are automated; plugin DLLs and live-tested behavior
  are unchanged by this documentation update.

## 1.3.1+rev.12 - 2026-10-15 stable release staging

- Promote 0.1.0-beta.12 to the shared loader-version plus independent revision format.
  Plugin metadata, Windows resources, package names and documentation agree.
- Keep the previously released behavior, native guards and saved settings.
  This promotion adds no native hook or gameplay behavior. Existing live
  coverage and documented limitations are unchanged; rebuild/artifact and
  package checks qualify the new stable artifact separately.

## 0.1.0-beta.12 — 2026-10-12

- Add `resolution_scaling = true`. Treat pixel settings as base values at
  1920 × 1080 and uniformly scale font size, offsets, lanes, motion, outline,
  shadow and backing by `min(screenWidth / 1920, screenHeight / 1080)`.
  Use 2× at 4K and 4/3× at 1440p; bound the factor to 0.25–4 and use 1× for
  invalid dimensions. HP amounts, grouping, lifetime, fade timing and opacity
  are unchanged. Setting false retains the previous fixed-pixel rendering.
- Add `critical_style = "both"`: `"suffix"` uses normal-size `!!`, `"size"`
  enlarges without a suffix, and `"both"` retains enlarged `!!`. The existing
  1–2 `critical_size_scale` defaults to 1.35 and applies only to size/both;
  `critical_hits = false` disables both emphasis effects. All modes retain
  the damage-type color and the same native critical association.
- Existing saved settings are preserved. Omitted new keys select resolution
  scaling and the existing both-effects critical style; add
  `resolution_scaling = false` to retain fixed screen pixels. Reference configs
  stay under `defaults/`; the loader creates only a missing live config.
- Preserve native hooks, admission guards, observations, critical-marker
  association and gameplay semantics. Static inspection records the reviewed
  Core's inclusive 512-pixel draw/measure font limit; the maximum configured
  base size, viewport factor and critical multiplier fit it exactly.
- MSVC Release `/W4 /WX` and all seven automated suites pass, including
  viewport/mode/config cases, production drawing-adapter checks at 2× scale
  with dense outline/shadow/backing, and exact-DLL ABI/defaults/exports/version
  checks. 4K scaling and the critical-style modes still need live visual checks.

## 0.1.0-beta.11 — 2026-10-11

- Enlarge matched native Critical/Deadly Strike numbers by 35% and append `!!`,
  retaining their damage color, motion, outline, shadow and optional backing.
  Measure and render every layer at the same effective size. HP is not doubled.
- Add `critical_hits = true` and bounded `critical_size_scale = 1.35` (1–2).
  Critical association works independently of element coloring.
- Carry the shared native bit through exact owned HP endpoint chains, including
  supported immediate/delayed killing remainders. Fresh poison-only groups and
  later poison deaths do not inherit earlier weapon emphasis. Unknown gaps,
  healing, stale records and other identities/sessions decline association.
- Reuse the existing observer and independent critical guards; no additional
  native hook, gameplay write or damage-size inference.
- Seven automated MSVC Release suites pass, including exact-DLL ABI/defaults,
  critical model and actual drawing/call adapter checks. The native marker is
  established on controlled paths; enlarged `!!` presentation remains live pending.
- Publish the first Floating Damage runtime package in the five-plugin suite
  with reference-only configuration, licenses, palette documentation, curated
  native evidence and checksums. Preserve existing live settings on extraction.

## 0.1.0-beta.10 — 2026-10-11

- Use ivory/coral/ice-blue/gold/lavender/pale-green/rose colors with a silver
  fallback and charcoal outline. Add configurable translucent backing, enabled
  for poison-containing numbers by default; all-types mode is optional.
- Identify the guarded physical-times-two writer and shared Critical/Deadly
  Strike result bit (`uint16 +4`, mask `0x2000`). Capture the bit separately
  from neighboring flags; six marked weapon controls and unmarked spell/weapon
  controls establish its provenance without changing gameplay.
- Six automated suites validate backing geometry/fade, providers and result-word
  isolation. Additional backing visibility/performance coverage remains pending.

## 0.1.0-beta.9 — 2026-10-11

- Correct surviving-poison matching: nonnegative base regeneration plus the
  poison state rate must explain composite stat74, while observed HP loss uses
  the full poison rate. Retain frame, expiry, identity and exact-endpoint guards.
- Confirm surviving/merged poison colors, three guarded poison killing
  remainders, and mixed physical/fire/poison followed by fresh pure-poison groups.
  Poison expiration on a survivor remains pending.
- Six automated suites cover base-regeneration correction and refusal cases.

## 0.1.0-beta.8 — 2026-10-11

- Add magic/poison classification and configurable colors from controlled Teeth
  and Poison Nova component captures.
- Add guarded periodic-poison inference from state2, signed stat74, float expiry
  and game frames; magnitude remains observed HP loss. Copy owned evidence only.
- Validate the two layout witnesses without calling a new native helper. Six
  automated suites cover component sums, periodic/death endpoints and mismatches.

## 0.1.0-beta.7 — 2026-10-11

- Retain a bounded lethal candidate when post-call stats disappear; resolve
  color only on a matching delayed death or zero sample. Preserve actual HP
  remainder, and decline stale/replaced/foreign candidates.
- Add opt-in `/damagenumbers capture` with a 128-record cap and game-exit reset.
  Capture synchronous packet/vector copies and HP/death ordering without screen
  debug text or a competing probe.
- Set the reference animation lifetime to 1000 ms. Six automated suites pass;
  live delayed-death and correct fire/cold/lightning color behavior are confirmed.

## 0.1.0-beta.6 — 2026-10-11

- Add exact-build physical/fire/cold/lightning component correlation through a
  guarded observer that forwards unchanged arguments and return value once.
- Embed native chat registration support for actual slash commands, preserving
  the debug-console fallback. Native callbacks retain the DLL until process exit.
- Six automated suites cover component/model behavior, unchanged forwarding,
  bounded commands and retained callback lifetime.

## 0.1.0-beta.5 — 2026-10-11

- Accept quoted hex RGB colors while retaining legacy RGB-array support.
- Add bounded shadows and optional rise/fall/side drift; expose motion commands.
  Layer fade and movement remain shared. No new shader or drawing hook is used.
- Three automated suites cover parser bounds, motion and shadow normalization.

## 0.1.0-beta.4 — 2026-10-11

- Default screen diagnostics off. Add bounded local console toggles/status and
  configurable outline/fade controls; real transitions clear tracked HP/numbers
  and invalidate queued samples. Runtime toggles do not rewrite configuration.
- Three automated suites cover state transitions, configuration and unsupported
  build refusal. Later beta.6 supplies native game-chat routing.

## 0.1.0-beta.3 — 2026-10-11

- Strengthen fill brightness, continuous two-pixel outline and late shared fade.
  HP tracking and compatibility remain unchanged. Improved appearance confirmed.

## 0.1.0-beta.2 — 2026-10-11

- Display the last tracked HP remainder once on a confirmed monster death,
  including when its stats have disappeared. Keep terminal amounts separate
  from tick groups. A two-second cache excludes stale/foreign identities;
  disappearance alone generates nothing. Killing numbers confirmed live.

## 0.1.0-beta.1 — 2026-10-11

- Add floating hostile-enemy HP-loss numbers, target following, independent
  lanes, rise/fade, grouping and bounded configuration on an admitted local game.
- Use authoritative read-only snapshots and owned render data; guard exact host/
  Core identities and native projection. Unsupported builds stay inert.
- Model/reader/artifact checks pass. Ordinary numbers and projection confirmed
  live; subsequent beta.2 adds the missing terminal remainder behavior.
