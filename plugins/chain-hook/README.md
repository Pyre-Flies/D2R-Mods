# Universal Chain Hook

**1.3.1+rev.8 · by PyreFly · Windows x64 / D2RLoader 1.3.1 / plugin ABI 4**

Lets multiple D2RLoader plugins that hook the same underlying game functions, item grid panels, tooltip pipelines, or hotkeys run together cleanly in any load order:

- **Load-Order Independent Hook Chaining**: Coordinates overlapping native hooks across active plugins automatically - including function prologues, call-site patches, padding jump caves, tooltip builders, and lazy UI grid wrappers - so companion mods don't block each other at startup, overwrite shared jump caves, or break when disabled at runtime.
- **Shared Plugin Hotkey Coordination**: Allows multiple plugins bound to the same hotkey (such as `F10`) to each receive the keypress cleanly instead of the first plugin swallowing it and blocking the others.
- **Zero Gameplay Hot-Path Overhead**: Builds direct cooperative call chains at load time so chained functions run without extra per-frame wrapper overhead during combat or UI rendering.
- **Early Startup Coordination**: Ships as `00-d2rl-chain-hook.dll` so it sorts first in `d2rloader/plugins/` and coordinates every plugin loaded alongside it from startup.
- **Closed-Source Runtime Release**: Distributed as a standalone runtime DLL (`00-d2rl-chain-hook.dll`) with this player guide.

## Installation and updates

1. **Install via D2R Reimagined Launcher or manual extraction:**
   - **Launcher:** Use **Install Plugin/Zip** on `Universal-Chain-Hook-1.3.1+rev.8-PyreFly.zip`.
   - **Manual extraction:** Close the game and extract `Universal-Chain-Hook-1.3.1+rev.8-PyreFly.zip` into your game directory (for global/Vanilla use) or your active mod directory (for example `mods/Reimagined/`), placing `00-d2rl-chain-hook.dll` inside `d2rloader/plugins/`.
   - **Active mod folder note:** When you launch a mod folder such as `mods/Reimagined/`, D2RLoader loads plugins from `mods/<Mod>/d2rloader/plugins/` rather than the root `d2rloader/plugins/` folder - make sure `00-d2rl-chain-hook.dll` is placed inside that active mod's `d2rloader/plugins/` directory.
   - **Keep the `00-` filename prefix intact** so D2RLoader discovers `00-d2rl-chain-hook.dll` first.
2. Start the game through D2RLoader. If `d2rloader/config/chain-hook.toml` doesn't exist yet, it's created automatically on first launch.

## Console commands (`/chainhook`)

Enter commands in the D2RLoader console (or in normal game chat when [Controller QOL Updates](../controller-qol/README.md) is installed):

| Command | What it does |
| --- | --- |
| `/chainhook` or `/chainhook status` | Show whether Universal Chain Hook is enabled and how many active hook sites and multi-plugin chains are running. |
| `/chainhook list` | List active multi-plugin hook chains and the call order of plugins participating in each site (`/chainhook list all` also includes single-plugin sites). |
| `/chainhook test` | Run a quick non-destructive live chaining self-check and print the result to the D2RLoader console. |

## Configuration (`chain-hook.toml`)

All keys belong under `[chain-hook]`:

| Setting | Default | Meaning / accepted values |
| --- | --- | --- |
| `enabled` | `true` | Enable (`true`) or disable (`false`) automatic multi-plugin hook chaining at startup. |

### Default configuration example

```toml
[chain-hook]
enabled = true
```

## Compatibility notes

- **Recommended Companion**: Highly recommended whenever running multiple native-hook plugins together - especially [Pyre Performance Optimizations](../pyre-perf/README.md), [Controller QOL Updates](../controller-qol/README.md), [Item Roll Ranges](../item-roll-ranges/README.md), [Running Defense](../running-defense/README.md), [Ultrawide](../ultrawide/README.md), `'Player Page Host'`, `'Grail On'`, `'Compare Indicators'`, and `'Perfect Frame'`.
- **Shared Page Host Plugins**: When using page mods built on `'Player Page Host'` (such as `'Grail On'`), keep `'Player Page Host'` installed in the same `d2rloader/plugins/` folder and remove competing standalone custom-page plugins that claim the same inventory page slot on their own.
- **Safe Build Guard**: Replaces startup disk hashing with fast in-memory validation across supported D2R `3.3` and `3.4` setups and stays safely inert on unsupported versions.
