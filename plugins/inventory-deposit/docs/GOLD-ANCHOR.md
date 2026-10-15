# Inventory Deposit: gold-relative button placement

## Beta.8 top-edge clearance, 2026-10-11

The user reported that beta.7 still obscured the gold lip in
`screenshot-dcaaecda-cf92-4c59-af9c-8f77d64e96df.png` (124-by-114).
The runtime log confirms beta.7 loaded at 08:44:31 and gap 85/offset-1 placement
was verified at 08:44:48; installed DLL SHA-256 matches the beta.7 record.
Screenshot matching also agrees with the revised gold-inlay artwork.

At 144-by-140, all four rasterized frames contain faint alpha beginning at
local y4 while their fully opaque pixels start at y7. A roughly 95-by-93 canvas
at crop origin `(19,9)` puts that fringe near the gold lip's y13. The previous
centered trim moved the top by only 2 UI units, leaving insufficient clearance.
This is screenshot/alpha evidence, not a new native-renderer contract.

Beta.8 uses 144-by-136 with gap 85 and offsetY 1. The reference gold rectangle
`(492,1656,246,48)` resolves to `(823,1613,144,136)`: its top moves down 4 UI
units, about 2.7 screenshot pixels, while its bottom remains 1749 and x/width
remain unchanged. The resampled alpha and opaque bottom bounds are preserved
in all four states; their top edges move down. No source artwork is edited.

The requested live config changes only `gold_offset_y` from -1 to 1 after backup;
explicit settings remain preserved by parsing. HD144-by-136/low-end72-by-68
resources match the click rectangle. Native addresses and anchor calculations
are unchanged. Final lip clearance remains pending live-game review.

## Beta.7 tiny height trim, 2026-10-11

The user confirmed beta.6's left alignment and requested a 1-2 pixel reduction
because the upper gold lip was obscured in
`screenshot-f9b4aec7-234c-4df0-b4d8-c02f35b330de.png` (427-by-152).
The beta.7 button was 144-by-140 UI units, with unchanged `gold_gap=85` and
`gold_offset_y=-1`. Reference gold `(492,1656,246,48)` now resolves to
`(823,1609,144,140)`: the top moves down 2 UI units, the bottom moves up 2,
and both horizontal edges remain fixed. This exposes roughly 1.2 screen pixels
at each edge at that screenshot scale. It is an estimate pending live review.
HD144-by-140 and low-end72-by-70 resources match the click rectangle.

The revised fine gold frame inlay keeps near-identical source alpha bounds;
normal alpha128 bounds change from `(58,62)-(1197,1193)` to
`(59,61)-(1197,1193)` on the 1254-square source. This subpixel-at-native-size
variation does not cancel the height trim. Config and native contracts remain
unchanged. The square source is vertically resampled by 140/144 in the build.

## Beta.6 lip fit, 2026-10-11

The beta.6 button used a 144-by-144 rectangle, `gold_gap=85` and
`gold_offset_y=-1`. Reference gold `(492,1656,246,48)` resolves to
`(823,1607,144,144)`, versus beta.5's `(848,1614,132,132)`: its layout origin
moves 25 UI units left and 7 up, and its lower edge extends 5 units down.
The existing gold-relative calculation and native geometry guards are unchanged.

The user's annotated 202-by-196 screenshot
`screenshot-caa0475a-ccfe-4fdd-82f2-23f3ba41ab8c.png` marks the top gap in
red and the left background strip in green. Matching the current normal sprite
to that image gives an approximately 88-pixel canvas at `(50,73)`, or two-thirds
screen pixels per UI unit. At alpha128 the 132-pixel normal sprite has bounds
`(6,7)-(126,126)`; the roughly 5% transparent margin explains why logical bounds
alone overestimated the visible coverage. The proposed visible frame is about
`(38,73)-(125,160)` in that crop, reaching the stone lips and covering the green
strip. This is a screenshot-derived estimate, not a live beta.6 observation.
The square artwork is resampled without cropping or stretching its aspect ratio.

HD144/low-end72 sprites and the 144-unit click rectangle change together.
Explicit settings remain preserved; the requested local fit updates only
`gold_gap` (110 to 85) and `gold_offset_y` (0 to -1) after backup. Manual x/y
defaults remain `(860,1620)`. No new native address or structure offset was used.

## Beta.5 footer fit, 2026-10-11

The beta.5 button used a 132-by-132 rectangle and default `gold_gap=110`,
`gold_offset_y=0`. From the reference gold rectangle `(492,1656,246,48)`, the
resolved button rectangle is `(848,1614,132,132)`. Compared with the earlier
`(860,1620,120,120)`, the left edge extends 12 UI units left, the top extends
6 units up, and the right edge remains 980. This enlarges the visible button
within the footer lip and covers the nearby background pattern seam. These
measurements come from the user's cropped screenshot and the existing layout
contract; final visual fit requires an in-game view.

The existing native setters, inherited hit testing, geometry checks and
gold-center calculation all read the button's current dimensions. No new
native address or structure field is required. HD132/low-end66 sprites and
the native click rectangle agree. Explicit spacing/manual coordinates are
preserved by config parsing; this user's requested adjustment changes their
installed `gold_gap` from 122 to 110 after backing it up. Manual x/y defaults
remain `(860,1620)` and are used only when gold anchoring is disabled.

The sections below record the earlier beta.2 investigation and its original
120-unit/gap122 defaults.

Recorded 2026-10-11 for `0.1.0-beta.2`. The button follows the loaded inventory's
`gold_amount` rectangle, replacing the default fixed-position behavior. It still
uses the existing embedded child layout and game TransmuteButton artwork; no
stock panel JSON, texture, or MPQ is replaced.

## Target and evidence provenance

All native facts below are specific to the reviewed D2R **3.3.93787** and
D2RCore **1.3.1-beta** profile. These are RVAs relative to the loaded **game
image**, not D2RCore, unless explicitly stated otherwise.

| Artifact | SHA-256 | Evidence |
|---|---|---|
| Installed `D2R.exe` | `1E2AC459FEB3F4BBFA818CDFF49800480502BEAE9F90CFA4CBA9E7E1F8BFA3B7` | Disk hash reread during this investigation |
| Installed `D2RCore.dll` | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` | Disk hash reread; implementation verifies the loaded Core module's file hash |
| Private code capture `runtime-game-auto-deposit-20260927.exe` | `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D` | Bounded static instruction analysis and isolated setter/getter probe |

The installed files are under
`<game-root>/`. The private capture is
`<research-root>/inputs/captures/runtime-game-auto-deposit-20260927.exe`.
It contains the relevant code but lacks the runtime `.rdata`/vtable contents.
The installed disk image also returns zeroes at the previously known runtime
vtable locations inspected here. A matching installed disk hash is not a new
observation of loaded instructions.

The prior lead was the native chat label/input sizing path in
`<d2rl-plugins-root>/docs/reverse-engineering/chatpanel-native-layout.md`, particularly
the 2026-10-04 label and active-caller records. Existing geometry evidence is in
[Controller QOL's skill-tree record](../../controller-qol/docs/SKILL-TREE-AIM-TOGGLE.md).
This investigation narrowed those leads to setters, the rectangle getter,
origin calculation, and point containment. No game binary, extracted layout,
or proprietary texture is included in this change.

## Supported SDK behavior and its limit

The pinned SDK's `WidgetService` ABI 1 provides path-based widget handles,
`findPanel`, `findWidget`, and `getWidgetRect`. Rectangle coordinates are local
to the widget's parent. It has no rectangle/position setter. Widget calls run
through the SDK UI-thread queue.

`PanelService::registerChildLayout` adds the plugin-owned child to
`StockPanel::PlayerInventory`, with `KeyboardMouseOnly` excluding controller
layouts. The layout remains an embedded `ResourceService` resource at
`data/global/ui/layouts/inventory-deposit/DepositAllButtonhd.json`, with root
name `inventory-deposit/DepositAllButton`. The child is initially hidden.

Native layout examples establish fractional **parent** anchors and shared
`$...` variable references, not a sibling-name anchor. No supported
`relativeTo`, `anchorTo`, or equivalent sibling-position field was found in the
inspected layouts or SDK. Re-registering a resource after reading the gold
rectangle is not a supported live layout reload: existing widgets can remain
cached until the panel is reloaded or the game restarts. The implementation
therefore reads geometry through the SDK and uses narrowly guarded native
position setters for the plugin's own button.

Static Reimagined layout evidence, used only as an example and initial spacing
reference, is `<reimagined-source-root>/data/global/ui/layouts/`:

- `playerinventoryoriginallayouthd.json` names `gold_amount` with local rectangle
  `(492,1656,246,48)` and `gold_button` at `(412,1644)`.
- `playerinventoryexpansionlayouthd.json` inherits that original HD layout.
- `_profilehd.json` defines numeric `LeftPanelAnchor`/`RightPanelAnchor` values;
  `mainmenuloadingpanelhd.json` uses child anchor `(1,1)` with negative offsets.

Those coordinates are not read from disk or assumed by automatic placement.
The active game's loaded `gold_amount` widget supplies the geometry.

## Native entry points and exact instruction witnesses

Full-body guards for direct child lookup, SetX, SetY and point containment are
versioned in [gold_anchor_profile.h](../src/gold_anchor_profile.h). The established
rectangle/origin/scale witnesses are reused from
[tree_layout_profile.h](../../controller-qol/src/aim/tree_layout_profile.h).
The existing FindPanel witness is reused from
[native_identify_profile.h](../../controller-qol/src/native_identify_profile.h).
The profile arrays are the authoritative guard bytes; a prologue match alone
does not admit the new setters.

| Game RVA and bounded span | ABI or observed behavior | Adoption |
|---|---|---|
| `0x846170..0x846185`, 0x16-byte body | `void* FindPanel(const char* name)`: moves name from RCX to RDX, loads manager into RCX, tail-jumps to `0x89F760`; null manager returns zero | Existing guarded native lookup, no handle-to-pointer cast |
| `0x856220..0x85629D`, 0x7E-byte body | `void* FindChild(void* parent, const char* name)`: RCX parent, RDX null-terminated name; iterates immediate children and returns the first matching pointer or zero | Complete new body guard |
| `0x8562A0..0x8562E8`, 0x49-byte body | `Rect* GetRect(void* widget, Rect* output)`: RCX widget, RDX caller-owned output; returns output in RAX | Existing complete getter witness; native read-back is compared with SDK geometry |
| `0x857570..0x8575B4`, 0x45-byte body | `void SetX(void* widget, int32_t x)`: RCX widget, EDX signed local x; normal path writes only `+0x70` | Complete new body guard and isolated execution probe |
| `0x8575C0..0x857604`, 0x45-byte body | `void SetY(void* widget, int32_t y)`: RCX widget, EDX signed local y; normal path writes only `+0x74` | Complete new body guard and isolated execution probe |
| `0x8562F0..0x856476`, 0x187-byte body | Current origin computation using local rectangles, ancestry, anchors and scale; output is a caller-owned integer x/y pair | Static consumer witness; not called by this adapter |
| `0x8566B0..0x8567E8`, 0x139-byte body | Point containment returns boolean AL, comparing the supplied packed x/y against freshly derived edges | Complete new body guard; actual button vtable slot `+0x40` must equal this entry |
| `0x857520..0x857564`, 0x45-byte body | RCX widget, EDX width; writes `+0x78` on its normal path | Static supporting lead only; not called |
| `0x8574D0..0x857511`, 0x42-byte body | RCX widget, RDX points to width/height pair; writes `+0x78/+0x7C` | Adjacent static size-setter witness only; not called |
| `0x8580D0..0x8580EA`, 0x1B-byte body | Writes visibility byte `+0x51`; changed state tail-calls virtual `+0x30` or `+0x38` | Rejected as a geometry helper; not called |

FindPanel's load at `0x846173` is `48 8B 0D F6 9F BF 02`. RIP-next
`0x84617A` plus displacement `0x2BF9FF6` resolves the manager global to
**`0x3440170`**. `0x3430170` is a rejected transcription, not another supported
global. The reused FindPanel guard includes two trailing `0xCC` bytes beyond
its 0x16-byte body.

The setters' C++ `void` return declarations intentionally consume no return
value; no success value in RAX is established. The supported Windows x64 input
ABI is RCX plus EDX. Both setters first inspect the fill-parent flag at `+0x52`.
Their nonzero branches call diagnostic-like helpers (`0x854B30` for X,
`0x854BC0` for Y), can execute `INT3`, and still reach a store. These branches
are excluded by admission, not adopted as usable layout behavior. The width
setter similarly references `0x854910`; the paired-size setter references
`0x854BF0`. These diagnostics are not called independently or given a complete
public ABI claim.

The chat caller proves the SetX argument meaning without guessing from the
setter's shape. At `0x2E386C` and `0x2E387F`, it reads the label rectangle;
`0x2E3884..0x2E388A` loads the InputBox into RCX and computes
`EDX = label.x + label.width`. `0x2E388C` calls `0x857570`. The neighboring
width calls are `0x2E3854` and `0x2E38B5`. A second previously documented
caller at `0x1562557` performs the equivalent input move. The new adapter
does not hook either caller or change chat widgets.

## Widget fields, lookup and hitbox evidence

| Widget field | Bounded interpretation |
|---|---|
| `+0x00` | Vtable pointer; inspect slot `+0x40` for the admitted containment implementation |
| `+0x08` | Child name pointer compared by native `0x12277A0` in FindChild |
| `+0x30` | Parent pointer |
| `+0x38/+0x3C/+0x40/+0x44` | Additional edge/margin-like fields consumed by point containment; no full public margin contract adopted |
| `+0x48/+0x4C` | Normalized parent-anchor floats |
| `+0x50/+0x51` | Existing enabled/visible byte witnesses |
| `+0x52` | Fill-parent flag |
| `+0x58/+0x60` | Immediate child pointer array / 64-bit child count |
| `+0x70/+0x74/+0x78/+0x7C` | Signed local x/y/width/height, four int32 values |
| `+0x80` | Local float scale |

FindChild uses the array/count at `+0x58/+0x60`, reads each child's name through
`+0x08`, compares it to the requested name, and returns that immediate child.
It is not recursive. Null child entries take a diagnostic path through
`0x1E5D50`, including a possible `INT3`; the helper does not itself provide a
bounded traversal guarantee for corrupt counts. This is a native-object lookup
contract, not permission to reinterpret SDK handles as native pointers.

For ordinary widgets, GetRect copies all four integers directly from `+0x70`.
Fill-parent widgets instead obtain their parent's rectangle and synthesize
zero x/y with inherited dimensions. The origin routine reads the current local
rectangle at `0x856353`, walks parents, combines anchors at `+0x48/+0x4C`
with parent dimensions, and applies parent scale. The reused origin guard
covers `0x856330..0x85640B`; the existing scale witness begins at `0x1E6756`
inside the ancestor-scale helper `0x1E6750`.

The containment routine reads current rectangle data at `0x8566FA`, current
scale, and those additional edge fields. It calls GetOrigin at `0x85679D`.
Comparisons at `0x8567AD..0x8567C8` implement left/top-inclusive and
right/bottom-exclusive containment. Thus this implementation derives its
hitbox from the same canonical position fields that SetX/SetY update, rather
than a separately cached position. The normal setters have no invalidation
call or other normal-path layout side effect to reproduce.

The adapter additionally requires the **actual injected button's** virtual slot
`+0x40` to equal `game base + 0x8566B0`. This ties the consumer witness to the
control being moved and refuses alternate implementations. It is a runtime
guard, not a claim that the currently installed button's vtable was observed
during development. Drawing and complete native click delivery still require
live validation.

## Placement and refusal behavior

[gold_anchor.cpp](../src/gold_anchor.cpp) runs in the existing UI callback and
reads `gold_amount`, the button, and the inventory parent through SDK handles.
The handles resolve by path; native widget addresses and screen resolution are
not cached. The same refresh runs immediately before accepting a queued click.

[gold_anchor_policy.h](../src/gold_anchor_policy.h) computes, in parent-local
units:

```text
button.x = gold.x + gold.width + gold_gap
button.y = gold.y + (gold.height - button.height) / 2 + gold_offset_y
```

Defaults are a gap of **122** and vertical adjustment **0**, preserving the
requested horizontal spacing and centering the button against the gold text
rectangle. Integer division follows C++ truncation toward zero. The current
button dimensions are preserved. With Reimagined's example gold rectangle and
a 120-by-120 button, the result is `(860,1620)`.

Both widgets must be direct children of the same native inventory panel, with
zero individual anchors, scale exactly 1, and fill-parent disabled. SDK and
native rectangle reads must agree for gold, button and panel before moving.
Before invoking direct child lookup, the adapter validates the panel's child
vector at `+0x58`, count at `+0x60` in `1..256`, and every child pointer as nonnull.
This bounds the native loop and avoids its null-child diagnostic path. The
limit is a conservative adapter policy, not a discovered engine capacity.
The policy requires positive dimensions, a contained gold rectangle, and a
target fully inside the panel; calculations use int64 intermediates and reject
out-of-range results. Gap must be `0..4096`; vertical adjustment must be
`-4096..4096`. Parent x/y are not added again because child rectangles already
share its coordinate system.

The adapter verifies the Core identity and required game instructions before
admission, and checks the instruction witnesses again when positioning.
Unresolved controls, unsupported geometry, missing space, read-back mismatch,
or failed positioning leave the button hidden and disabled. An instruction
mismatch or native access exception makes the adapter unavailable until
restart. Automatic placement does not silently fall back to guessed coordinates.
The existing explicit manual mode remains available separately.

SetX/SetY are invoked only when the corresponding coordinate differs; a final
SDK read must equal the intended full rectangle. Only the plugin's button is
moved. The gold widget, inventory parent, stock behavior and deposit policy are
unchanged. No new inline hook, code patch, input synthesis, panel replacement,
or direct write to stock-widget geometry is introduced.

## Validation and remaining live checks

Before integration, [probe_gold_anchor.py](../tools/probe_gold_anchor.py) verified
the private capture hash and executed its actual normal SetX, SetY and GetRect
machine-code paths against a synthetic 256-byte widget buffer in a separate
Python process. It passed all five positions `(860,1620)`, `(300,700)`, `(0,0)`,
`(-20,-30)` and `(4096,4096)`: only x/y changed, width/height and all other bytes
were preserved, and GetRect returned the requested values and output pointer.
Fill-parent remained zero throughout, so diagnostic/external-call paths were
never executed. The probe never opened or wrote a game process. It validates
this normal-path ABI and mutation footprint, not game drawing or interaction.

To reproduce locally with the private capture and `pefile` available:

```powershell
$env:PYTHONPATH = '<optional-python-libs>'
python plugins/inventory-deposit/tools/probe_gold_anchor.py <research-root>/inputs/captures/runtime-game-auto-deposit-20260927.exe
```

The integrated candidate built in Release with MSVC **19.44.35220**, `/W4`, with
zero warnings. All **six CTest suites passed**, including gold-anchor policy,
UI refresh/click refusal, settings, existing batch/transfer policy and DLL
artifact admission checks. Build/test output is in ignored
`outputs/inventory-deposit-button/build-gold-anchor/`. Bounded disassembly and
investigation notes are in ignored
`outputs/inventory-deposit-button/gold-anchor-research/`; the versioned profiles
and this record retain the adopted contracts.

No D2R or D2RLoader process was running when optional live inspection was
attempted, and neither was started. Automatic placement, actual button
vtable admission, hover/click hitbox alignment, multiple resolutions/UI scales,
panel reopen and changed inventory layouts remain **live-pending**. The earlier
user-confirmed fixed-position beta does not establish those new behaviors.

## Local candidate and deployment - 2026-10-11

The final beta.2 DLL passed all six suites after the child-vector guard was
added. The artifact verifier confirmed the one-DWORD ABI manifest, config,
exports and beta.2 metadata. Imports remain only `KERNEL32.dll` and `bcrypt.dll`.
Strict JSON parsing checked the actual registered default and manual layout
payloads. Package integrity, exact DLL identity, reference config, text privacy
and the pre-task Controller QOL baseline comparison passed.

- DLL SHA-256: `238B8015AAD41BD9FFA35F2EE59201D330FF4750A06C55F24D1CE8EDEE10CD45`.
- Local runtime ZIP: `outputs/inventory-deposit-button/gold-anchor/Inventory-Deposit-0.1.0-beta.2-runtime.zip`.
- ZIP SHA-256: `876CC2D9C9CB4413CA24EECDFFF8974F3C1EF8097C22D9D749F0B6688B16BA0A`.

With D2R/D2RLoader confirmed closed, the global installed DLL was updated at
`<game-root>/d2rloader/plugins/Inventory Deposit.dll`.
Built and installed hashes match. The global `d2rloader/config/inventory-deposit.toml`
adds `anchor_to_gold = true`, `gold_gap = 122`, and `gold_offset_y = 0`. Parsed
existing settings, including `button_x = 860`, `button_y = 1620` and enablement,
are preserved. Comments now describe when manual coordinates apply.

Rollback copies of both prior files are under the game root at
`d2rloader/backups/inventory-deposit-gold-anchor-20261011-063408/`.
With the game closed, restore its `Inventory Deposit.dll` to
`d2rloader/plugins/` and `inventory-deposit.toml` to `d2rloader/config/`.
The original beta.1 ZIP remains untouched. No commit or public release was
created. Deployment/hash verification does not establish live admission,
placement, hover, click, panel reopening or multiple-resolution behavior.

## Subsequent beta.2 runtime evidence - 2026-10-11

During the custom-art follow-up, the global `d2rloader/logs/inventory-deposit.log`
was inspected with the beta.2 DLL still installed and its recorded hash matching.
At `06:37:34.341` the native gold-anchor profile was admitted. At `06:37:50.876`
`PlayerInventoryExpansionLayout` placement and rectangle read-back succeeded
with gap 122 and offsetY 0. This means the actual button passed the point-test
virtual-method guard, direct-parent/transform checks and SDK/native geometry
comparison used by that build. At `06:38:25.640` the batch completed with snapshot
18, submitted 6, removedFromInventory 6, skipped 12.

These are runtime-observed admission, position/read-back and batch observations.
They do not measure visual pixel alignment, prove destination counters/save
persistence, or qualify other resolutions and the new custom-art build.
