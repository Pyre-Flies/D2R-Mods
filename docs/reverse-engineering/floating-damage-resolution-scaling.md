# Floating Damage: viewport scaling and native text-size behavior

Reviewed 2026-10-12 for `0.1.0-beta.12`. This record documents supported SDK
coordinates, current Core text-size forwarding/validation and the presentation
scale. Inspection was static/read-only; no helper was called and no game memory
was changed. Live 4K rendering and extreme-size performance remain unverified.

## Exact artifacts and public SDK contract

| Artifact | Identity |
|---|---|
| Pinned PluginSDK | Commit `bb0b48e080c51c37fc8296ee415d9daaba7ee33d` |
| Reviewed `D2RCore.dll` | SHA-256 `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` |
| Admitted loader-hosted game | SHA-256 `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` |

The Core file's hash matched before PE/disassembly inspection. Native addresses
below are Core-relative RVAs. Static decoded bytes are exact-file witnesses,
not independently compared live windows or new plugin admission gates. Existing
host/Core admission and native projection/clip/combat guards are unchanged.

Pinned [`overlay.h`](../../third_party/D2RLoader-PluginSDK/include/D2RLPlugin/overlay.h)
lines 61–72 defines callback-scoped top-left coordinates and float `screenWidth`,
`screenHeight`, `deltaTimeSeconds` and `defaultTextSize`. Text/measurement request
sizes are floats; textSize zero selects the default. UTF-8 input is bounded at
4096 bytes. The header declares no numeric maximum text size. The
[SDK README](../../third_party/D2RLoader-PluginSDK/README.md)'s overlay section
requires callback/thread ownership and describes screen coordinates, without
promising an OS physical-pixel/DPI conversion or native inventory-layout units.

Use the callback's own dimensions for this presentation policy. Do not use
monitor resolution, Windows DPI or widget-layout coordinates as substitutes,
and do not scale projected enemy positions a second time.

## Viewport provenance and service table

The reviewed Core OverlayService table is at `0x634C70`: 88-byte service size,
version 1, with drawText pointer at `+56` and measureText at `+64`.
Draw wrapper `0x4424E0` calls implementation `0x443D00`; measure wrapper
`0x442520` calls `0x444400`. The metric wrapper `0x4423E0` calls `0x442EE0`.

Dispatcher `0x4411C0` reads a pointer through Core slot `0x709A00`, then copies
the source float2 at `+0x10` and delta-time float at `+0x18`. It obtains default
text size through slot `0x709DB8`. The float2 becomes metric width/height at
Core `0x711B08`, with delta time/default text size at `0x711B10/0x711B14`.
The 32-byte Metrics record starts at `0x711AF8`; callback dispatch is through
`0x441D30`. Its saved Metrics pointer supplies default size from `+0x1C` at
`0x441E2C..0x441E38`. These are investigation references, not new native calls.

| Core RVA / length | Exact bytes | Witness |
|---|---|---|
| `0x4411F4` / 25 | `488B0505882C00488B004885C0741EF30F7E7010660F6E7818` | Slot/pointer and source dimension/delta reads. |
| `0x44131A` / 25 | `660F1335E6072D00660F7E3DE6072D0066440F7E05E1072D00` | Stores width/height, delta time and default text size. |
| `0x4424E0` / 22 | `554883EC20488D6C2420E811180000904883C4205DC3` | Draw wrapper calls `0x443D00`. |
| `0x442520` / 22 | `554883EC20488D6C2420E8D11E0000904883C4205DC3` | Measure wrapper calls `0x444400`. |

The SDK establishes shared screen-coordinate space. Static copying and earlier
reviewed overlay-window dimensions support viewport-relative geometry, but do
not establish every physical-DPI, render-resolution or display-scale combination.

## Explicit size is honored and limited to 512

Draw copies the 64-byte TextRequest. At `0x444089` it reads request textSize
from its copied `+0x28`. Bit checks at `0x44408E..0x4440CF` reject negative/nonfinite
values, with zero accepted for default selection. It compares against float32
512 at Core `0x634C64` (`00000044`); `0x4440E0` rejects only requested sizes greater
than 512. Equality is accepted. `0x44412F..0x444142` selects the callback default
only for zero; a nonzero requested size remains the effective size in XMM6.
At `0x444191` it moves that scalar to XMM2 before native drawing via slot
`0x709E90` at `0x444195`. Color conversion at `0x4434B0` is separate from size.

Measure reads MeasureTextRequest textSize at `+0x10` into XMM6 at `0x444497`.
The finite/nonnegative check at `0x44453C..0x44457C` and upper-bound comparison at
`0x44457E..0x444589` use the same 512 constant. Native base text metrics come
through slot `0x709F68` at `0x4445DE..0x4445F9`. Zero selects the callback default;
otherwise the requested size/default-size ratio multiplies both metric
components at `0x4445FC..0x444626`. Thus measured centering/backing and native
requested drawing size follow the same effective scalar on this Core.

| Core RVA / length | Exact bytes | Witness |
|---|---|---|
| `0x444089` / 5 | `660F6E4528` | Draw copied textSize read. |
| `0x4440D5` / 17 | `F30F100D870B1F000F2EC80F8241FFFFFF` | Draw compares requested size with 512; greater values branch to refusal. |
| `0x44412F` / 20 | `0F57F6F30FC2F000660FDBCE660FDFF0660FEBF1` | Draw chooses default only when requested size equals zero. |
| `0x44418F` / 9 | `31D2660F6FD641FFD0` | Effective size copied to XMM2 before native call. |
| `0x444497` / 5 | `660F6E7210` | Measure request textSize read. |
| `0x44457E` / 13 | `F30F1005DE061F000F2EC67285` | Measure uses the same 512 upper limit. |
| `0x4445FC` / 43 | `0F57C0F30FC2C600660FDBF8660FDFC6660FEBC7F30F5E450049C70610000000F20F104DF80F14C00F59C1` | Default/request selection, ratio and metric multiplication. |

The shared canvas validator at `0x443410..0x4434A3` checks active canvas/thread/
owner/draw-list and supplies default text size; this inspected path has no
per-glyph command counter. That does not promise unlimited rendering resources.
Runtime return success is submission evidence, not proof of visible appearance.
The static bridge does not explain an unchanged-size user report by itself;
active-scope configuration, restart and live presentation must be checked.

## Scaling policy and geometry

`resolution_scaling = true` uses the uniform scale
`min(screenWidth/1920, screenHeight/1080)`, clamped to 0.25–4. Disabled scaling,
nonfinite dimensions or dimensions at/below zero retain scale 1. The smaller
ratio keeps ultrawide width from inflating text at unchanged height. Examples:
1920×1080 → 1; 3840×2160 → 2; 2560×1440 and 3440×1440 → 4/3;
1280×720 → 2/3; 1920×1200 → 1. This is an explicit presentation choice,
not an SDK-imposed design resolution.

Font size, shadow offsets/softness sample spacing, backing padding/rounding,
vertical offset, lane spacing, rise/fall/side distances and screen-cull margin
share that scale. Native screen/world projection and clip dimensions continue
to use the actual unscaled viewport. Lifetimes, age, fade, opacity, colors, HP
amounts and classification remain unchanged. Measure and every glyph pass use
the same effective size/string; measured bounds must not be multiplied again.

Outline uses dense one-pixel-offset glyph passes with an upward-rounded scaled
radius, capped at 12. This avoids holes caused by spacing a small fixed outline
kernel several pixels apart. Fractional scaling can round the stroke upward;
zero outline remains zero. Shadow sampling keeps its existing normalized fixed
kernel/count and scales its spatial offsets, so broad sampled shadows are an
approximation rather than a blur shader.

Configured text_size is 12–64; critical multiplier is 1–2. At the maximum
scale 4, effective font size is at most `64×4×2 = 512`, exactly the reviewed
native limit. Selectable critical styles are suffix, size or both, all subject
to the existing master critical_hits gate and native marker association.

Dense outline cost grows with radius: `(2r+1)^2−1` outline glyph passes plus
shadow passes and fill. Default radius 2 uses 34 glyph passes including its
nine shadows and fill; 4K radius 4 uses 90. Worst radius 12 plus 25 shadows and
fill uses 650 per number, bounded by the existing 128 active-number cap.
High-resolution dense-combat performance requires a live check.

All seven MSVC Release `/W4 /WX` suites pass. Automated fixtures cover
malformed/scaled viewports, independent critical modes/master disable, identical
draw/measure size/string, exactly scaled motion, dense outline coverage/count,
shadow offsets/weights, backing padding/rounding/centering and the maximum
512-size contract. The nineteen packaging regressions also pass. Static native
forwarding and simulated geometry do not establish live 4K visibility or
rendering cost.
