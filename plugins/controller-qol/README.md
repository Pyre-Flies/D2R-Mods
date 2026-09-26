# Controller QOL Updates

**Made by PyreFly for D2RLoader**  
Version **1.3.1+rev.11** | Windows x64 | Diablo II: Resurrected

Pick the loot you want directly from its ground label. Controller QOL Updates gives nearby items their own controller shortcuts, keeps those assignments stable while you loot, and respects the active loot filter. Inventory, Cube, stash, potion and shop shortcuts extend the same LB-based controls to item management.

## Direct controller looting

1. Stand near the items you want to collect and **hold LB**.
2. Up to **seven eligible nearby ground items** receive button indicators on their labels.
3. Keep holding LB and press the button shown beside the item to pick up that specific item.

The available shortcuts are **LB+A, LB+X, LB+Y, LB+B, LB+RB, LB+RT and LB+LT**. These are ground-looting controls; inventory and storage screens use the contextual actions below. On a PlayStation-style controller, LB is L1; A/X/Y/B correspond to Cross/Square/Triangle/Circle.

- **Choose an item by its label.** You do not need to cycle ground targets to reach the item assigned to a shortcut.
- **Stable assignments while holding LB.** An item keeps its shortcut while it remains eligible. Vacant slots refill as you collect items or move; pressing LB again refreshes the assignments.
- **Priority and range.** Available slots are assigned by item type/quality priority, then distance. Candidates must be within pickup range and pass the collision check. The default range is 6 game distance units.
- **Loot-filter awareness.** Only items with active visible labels are assigned shortcuts. With the default `block_filtered_pickup = true`, native pickup attempts for items without an active label are blocked too.
- **Persistent filtered labels.** The integrated label handling keeps filtered ground labels visible and prevents the modifier from dismissing them. Releasing LB removes the shortcut overlays.

Holding or releasing LB alone does **not** automatically collect items. This plugin uses your existing loot filter; it does not provide a filter editor.

### Neutral A: nearby portal priority

With `prioritize_portals = true` (default), the native Interact target comparison prefers an eligible nearby portal over ground loot. The independent `portal_priority_distance = 10` setting controls the range (default **10 game units**, inclusive; supported values 1â€“20). `ground_pickup_distance` remains 6 by default. Version 1.5.19 extends the portal-only candidate contact gate as well as scoring and Interact distance checks. Actual portal use remains native. Native visibility, angle and skill checks still apply. `portal_diagnostics = false` controls optional portal-only tracing independently of general debug logging. Restart the game after editing the TOML. The game retains its normal portal eligibility and entry handling. Bare A still picks up items when no eligible portal wins that comparison; **LB+A and the other direct-loot chords remain unchanged**. NPCs and other objects keep their normal comparisons. Set `prioritize_portals = false` to restore the original targeting behavior.

## Inventory, storage and shop controls

| Shortcut | Action |
|---|---|
| LB + A | Identify the focused item using an available identify charge |
| LB + X | Transfer the focused item between inventory and the open storage context; sell an inventory item when an NPC shop is open |
| LB + Y | Transfer an item using the existing inventory/Cube action |
| LB + R3 | Refill belt potions, including supported stash rejuvenation sources |
| LB + LT / LB + RT | Previous / next Shared stash sub-page |
| LT / RT | Switch main stash tabs |

Gems and Materials LB+X use smart storage for eligible items and the embedded Horadric Cube for other items. Shared stash transfers use the active page and preserve the focused item's identity. Selling uses the game's native eligibility, pricing and transaction path. Controller glyphs and menu navigation are also included.

## Install or update

1. Close Diablo II: Resurrected.
2. Back up the old DLL and configuration. **Move the old `QOL.dll` out of the plugins directory** before installing this renamed release; it has a new plugin ID, so leaving both DLLs there would load both.
3. Copy `d2rloader/plugins/Controller QOL Updates.dll` into the active loader's plugins directory. The current installation uses the game's global `d2rloader/plugins` folder; use a mod-specific folder only when that loader configuration actually loads it.
4. For an existing installation, rename `d2rloader/config/qol.toml` to **`controller-qol-updates.toml`** to preserve your settings. Keep the `[qol]` section inside the file unchanged. If the destination already exists, back it up and merge your settings instead of overwriting it.
5. For a new installation, launch through D2RLoader: it creates `d2rloader/config/controller-qol-updates.toml` from the embedded defaults if missing. A loose default copy is supplied under `configuration/controller-qol-updates.toml` for reference or manual installation.

D2RLoader requires lowercase plugin IDs without spaces and derives configuration filenames from those IDs. The DLL uses the full display name; the loader ID and configuration use `controller-qol-updates`. The native loader still creates, reads and preserves configuration files. If you explicitly configured a loader load-order entry for `qol`, update that entry to `controller-qol-updates`.

## Defaults and configuration

**Debug logging is OFF by default: `debug_logging = false`.** This is true of both the supplied TOML and the copy embedded in the DLL. Renaming an existing configuration preserves its current value; set it to `false` if that file still has debugging enabled.

Ground looting, quick identify, quick move and filtered-pickup blocking are enabled by default. The modifier is LB (`"bumper"`); identify requires and consumes an available identify charge. Edit `controller-qol-updates.toml` with the game closed. The `[qol]` section remains for settings compatibility.

## Compatibility and verification

Requires D2RLoader. Earlier gameplay features were developed and manually qualified with the current Reimagined installation. Native code profiles target that tested game build; other patches and mod configurations have not been qualified. Item roll-range display is separate research and is not included.

All 12 automated suites passed. The user confirmed Chronicle Ground Flag coexistence in rev.6 and portal behavior in rev.7. Earlier native-controller validation covered Battle.net DualShock without Steam and Steam Controller/DualShock on Steam, with Stash Search and Potion Auto Pickup. See `docs/PRODUCTION-1.3.1-rev.7.md` for precise validation scope. `SHA256SUMS` lists package hashes.

The monorepo references a shared, pinned SDK submodule and includes tests and patch-recovery documents. Start with `docs/README.md` for the research map. See `docs/PORTAL-PRIORITY.md` for the new hook and patch recovery, `docs/PACKAGING.md` for naming evidence, and the feature-specific documents for native contracts. `docs/LEGACY-REIMAGINED-MIGRATION.md` records the extraction from the former Reimagined working tree and the evidence distilled from excluded local logs. Historical documents retain their original filenames and version-specific observations. The 1.5.13 and 1.5.19 golden checkpoints remain separate.

## Build from source

Use an x64 Visual Studio Developer Prompt with CMake 3.29+ and the Windows SDK:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

The output is **`Controller QOL Updates.dll`**. `tools/build_local.py` is the developer machine's compiler-discovery helper. See `docs/NPC-SELLING.md`, `docs/SHARED-DEPOSIT.md` and the other feature-specific research for native contracts.

### Plugin cooperation in 1.5.20

QOL leaves the shared action table available to RuffnecKk Potion Auto Pickup 1.3.3, honors direct plugin pickup requests, and recognizes its reviewed belt hook. Game-originated pickups retain QOL filtering. Optional label-limit observations now report modified code without a misleading patch-failure error (including the report associated with Stash Search). See docs/POTION-AUTO-PICKUP-COMPATIBILITY.md for evidence, addresses, scope and remaining runtime checks. Neither third-party plugin needs editing.


### 1.5.21 startup compatibility test build

Private D2RCore controller calls now share the exact-build guard used by navigation. Unknown loader builds use physical XInput instead of old core addresses. Native navigation/label/glyph features that fail existing guards remain disabled; this build does not claim full 1.3.1 feature compatibility. Ten automated suites cover the startup fallback and existing contracts; game startup confirmation is pending.

For a launch with no active mod, install in the game's **d2rloader/plugins** folder and use **d2rloader/config/controller-qol-updates.toml**. This test deployment preserves the global configuration and leaves mods/Reimagined untouched.

### 1.3.1+rev.4 focused potion refill

Highlight a potion stack in Materials and press LB+R3 to refill from that exact stack. It does not switch to another potion type when stock runs out. Fill Belt appears only on potions. Existing inventory refill remains available. See docs/FOCUSED-REFILL-1.5.25.md.

See docs/PRODUCTION-1.3.1.0.md for the production snapshot and version policy.

XInput hardening: see docs/XINPUT-HARDENING-1.3.1.1.md. Shutdown retains a transparent hook until process exit; restart to change DLL versions.

1.3.1.2: holding the ground-loot modifier excludes portals from Interact selection; neutral A retains portal priority.

1.3.1.3 moves controller prompt text interception into widget call sites, preserving the renderer entry checked by Stash Search. See docs/GLYPH-CALLS-1.3.1.3.md.

Version policy: `1.3.1+rev.N` identifies the qualified loader target and the independent plugin revision. Build metadata does not participate in SemVer precedence. Native fingerprints remain authoritative.

### Native controller input in 1.3.1+rev.5

The qualified D2RLoader 1.3.1 build uses the game's normalized controller input for loot modifiers, shortcuts and Shared page chords. Steam translation is not required for a controller the game itself recognizes. QOL applies shortcut suppression at that same native layer and does not install XInput hooks when the native hook is admitted. An unsupported/conflicting native profile falls back to XInput with an explicit coverage warning; native navigation guards remain independent. Digital native triggers follow the game's pressed state rather than a raw analog threshold. See [native input evidence and verification](docs/NATIVE-INPUT-1.3.1-rev.5.md). Production validation: Battle.net DualShock with Steam closed, Steam Controller on Steam, and DualShock on Steam, all tested by the user with Stash Search and Potion Auto Pickup enabled. See [production release notes](docs/PRODUCTION-1.3.1-rev.5.md) for scope and remaining risks.

### Production 1.3.1+rev.7

Ground-label and portal-contact interception now uses scoped call sites, leaving their shared entries available to other plugins. See [release notes](docs/PRODUCTION-1.3.1-rev.7.md) for validation and compatibility limits. Version-specific sections above describe historical changes.

## Latest compatibility improvements

Charm Inventory menu switching and LB+X transfers now cooperate with the
loader's registered custom page. Item shortcut hints appear only in controller
mode. These changes reuse native routing and transfer semantics; custom-page
transfers add no hooks. See [current release record](docs/PRODUCTION-1.3.1-rev.11.md)
for build requirements, validation scope and limitations.

Rev.11 removes synchronous per-item identification logging, validates the exact
item and consumables in one game-thread scan, and reports distinct failure
reasons without enabling verbose logging. See docs/IDENTIFY-REV11.md.
