# D2RLoader Plugin Hot-Path Performance Audit & Optimization (2026-10-26)

This document records the live profiler captures, reverse-engineered hot paths,
anti-patterns, and verified fixes from the 2026-10-26 40-plugin performance
optimization pass on the non-ladder **D2R Reimagined** profile.

## 1. Live Profiler Benchmark Summary (`120 FPS` cap, 40 plugins loaded)

Captured via the `D2RL Performance Profiler` (`d2rloader/logs/perf/`):

| Metric / Plugin | Baseline (`perf-20261026-065600.json`) | After `Buff HUD` Fix (`perf-20261026-073550.json`) | After All 4 Plugin Fixes (`perf-20261026-092726.json`) | Total Change |
|---|---:|---:|---:|---:|
| **All 40 Plugins Combined (`ms/frame`)** | `0.5143 ms` | `0.2966 ms` | **`0.1467 ms`** | **`-71.5%`** |
| **Plugin Share of Frame Budget** | `6.14%` | `3.56%` | **`1.76%`** | **`-4.38%` of frame** |
| **Average FPS** | `119.34 FPS` | `119.99 FPS` | **`119.79 FPS`** | Locked at cap |
| **1% Low FPS** | `66.16 FPS` | `97.69 FPS` | **`108.50 FPS`** | **`+64.0%`** |
| **0.1% Low FPS** | `29.74 FPS` | `51.83 FPS` | **`59.48 FPS`** | **`+100.0%` (2x)** |
| **1. `Buff HUD` (`d2rl-buff-panel.dll`)** | `0.2186 ms/f` (`42.5%`) | `0.0063 ms/f` | **`0.0060 ms/f`** | **`-97.3%`** |
| **2. `Pyre Loot Filter` (`pyre-lootfilter.dll`)** | `0.0897 ms/f` (`17.4%`) | `0.0928 ms/f` | **`0.0176 ms/f`** | **`-80.4%`** |
| **3. `Projectile Perf Optimizer`** | `0.0598 ms/f` (`11.6%`) | `0.0493 ms/f` | **`0.0060 ms/f`** | **`-90.0%`** |
| **4. `Controller QOL Updates`** | `0.0498 ms/f` (`9.7%`) | `0.0468 ms/f` | **`0.0163 ms/f`** | **`-67.3%`** |
| **5. `Bind and Summon` (`BindAndSummon.dll`)** | `0.0369 ms/f` (`7.2%`) | `0.0389 ms/f` | **`0.0398 ms/f`** | Closed-source (untouched) |

---

## 2. Six Recurring Hot-Path Anti-Patterns & Solutions

### Pattern 1: Per-Frame Kernel Syscalls (`VirtualQuery`) for Pointer Probing
- **Symptom:** Calling `VirtualQuery` on every native pointer dereference inside a `120 Hz` UI callback or `25 Hz` game-thread loop (`Buff HUD` `ReadMemorySafe` at `RVA 0xC120`). Each `VirtualQuery` transitions into the Windows kernel (`NtQueryVirtualMemory`), adding hundreds of microseconds per frame when reading dozens of states/stats.
- **Fix:** Replace per-read `VirtualQuery` calls with a lightweight null/range check (`address >= 0x10000`) and a Structured Exception Handling (`__try` / `__except`) guard in an isolated non-unwinding helper function. On x64 Windows, table-based SEH has zero runtime overhead when no exception occurs.

### Pattern 2: Always-On `QueryPerformanceCounter` Scopes Inside Per-Item / Per-Unit Hooks
- **Symptom:** Wrapping nested per-item ground label evaluation hooks (`HookBuildGroundItemName`, `FormatItemFromUnit`, `EvaluateUnitLabelAndRule`, `ReadUnitFacts`, `EvaluateItemFilter`) in an unconditional `PerfScope` that calls `QueryPerformanceCounter` on entry and exit (`Pyre Loot Filter` `src/plugin.cpp`). When 20–50 items are on the ground, hundreds of QPC timers fire every frame, consuming **`~0.050 ms/frame` (`56%` of the entire loot filter's CPU time)**.
- **Fix:** Gate internal `PerfScope` timing behind `std::atomic<bool> g_perfTimingEnabled{false}` (`memory_order_relaxed`), and enable it on demand when the user runs `/filter perf`, `/filter perf on`, or `/filter perf reset`.

### Pattern 3: Guard-Order Inversion (Locking / Byte-Checking / OS Window Queries Before the Cheap Filter)
- **Symptom A — Locking settings mutexes before checking widget identity:**
  - In `Controller QOL Updates` (`src/qol_glyphs.cpp`), `IsRuleReorderLegend` called `QolNavigation::LootFilterReorderEnabled()` (which locks `stateMutex`) on *every* widget drawn by the game before checking if the widget's parent vtable was actually `ControllerLegendWidget` (`D2R.exe + 0x1d76550`).
  - Similarly, `HookWidget` called 6 separate `QolNavigation::*RemapEnabled()` functions (6 `stateMutex` lock/unlock pairs per widget) before checking whether the widget even contained any controller glyph names (`QolGlyphPolicy::Classify`).
- **Symptom B — Byte-guarding `D2RCore.dll` on idle ticks:**
  - In `Controller QOL Updates` (`src/client_loot.cpp`), `HookClientUpdate` called `ValidateUpdate()` (running 4 `CheckExpectedBytes` memory comparisons against `D2RCore.dll`) on every single frame before checking `!queuedApproach.request.item.guid`.
- **Symptom C — Querying OS window focus before checking hotkey state:**
  - In `Pyre Loot Filter` (`PollLiveReloadAndHotkeys`) and `Projectile Performance Optimizer` (`PollOverlayHotkeys`), `IsGameWindowFocused()` (`GetForegroundWindow()` + `GetWindowThreadProcessId()`) ran on every `120 Hz` frame *before* `GetAsyncKeyState(VK_F9 / VK_F10)`.
- **Fix:** Always order guards from cheapest/most selective to most expensive:
  1. Local scalar/pointer/vtable/atomic flag check (`~1 ns`).
  2. `GetAsyncKeyState(vk)` check (only true for the 1 frame a key is pressed).
  3. Mutex acquisition, `D2RCore.dll` byte verification, or Win32 window focus queries.

### Pattern 4: Unconditional `120 Hz` Widget Tree Churn & Multi-Argument `snprintf`
- **Symptom:**
  - `Buff HUD` (`RVA 0x13D80` / `0x10DF0`) walked all 21 buff widget slots every render frame and unconditionally called `WidgetService::SetVisible` and `WidgetService::SetEnabled` (`42+` native calls/frame) even when no buffs changed, and polled game state at `120 Hz` despite `D2R` updating authoritative client/server state at `25 Hz` (`currentGameFrame` at `+0x54`).
  - `Projectile Performance Optimizer` (`src/plugin.cpp`) ran two 16-argument floating-point `std::snprintf` calls on every `120 Hz` overlay frame to format HUD counters that only change at `25 Hz`.
- **Fix:**
  - Gate `PumpOnGameThread` state scans to `25 Hz` (`currentGameFrame != lastPolledGameFrame`).
  - Track per-slot visibility/enabled state (`appliedVisible`, `appliedEnabled`) and a lightweight widget canary (slots `0`, `10`, `20`) so native `WidgetService` calls only fire on state transitions.
  - Cache formatted HUD strings and refresh them at `150 ms` (~6.6 Hz) or when revision counters advance.

### Pattern 5: Hidden `memset` Cost on Fixed-Capacity Embedded Arrays
- **Symptom:** Zero-initializing structs with large embedded fixed arrays on every call even when the array is unused or only partially populated:
  - `Pyre Loot Filter`: `ItemFacts` (`src/filter_schema.h`) embeds `StatFact stats[64]` (`512 bytes`). Value-initializing `ItemFacts{}` and copying all 64 entries in `ReadUnitFacts` zeroed and copied `1,024 bytes` per item read even when an item only had 3–8 stats.
  - `Pyre Loot Filter`: `MinimapPipeline` (`src/minimap_pipeline.h`) allocated and zeroed `MarkerFrame` (`6,728 bytes` on stack) and locked `mutex_` in `ForgetUnit`, `ProjectAndPublish`, and `CopyFreshFrame` on every frame even when `0` minimap-tracked items were on the ground.
  - `Controller QOL Updates`: `RegisterPlacard` (`src/plugin_main.cpp`) ran `existing->second.lease = {};` (`1,040-byte` `memset` of `char text[1024]`) for every ground item every frame when resetting an inactive lease.
  - `Controller QOL Updates`: `HookWidget` (`src/qol_glyphs.cpp`) zero-initialized `char names[12][64]{}` (`768 bytes`) and `DrawScoped` copied `currentHeaderLabel` (`1,024 bytes`) on every widget draw even when `currentHeaderLabel[0] == '\0'`.
- **Fix:**
  - Provide scalar-only reset helpers (e.g. `ItemFacts::ResetScalars()`, `lease.bytes = 0; lease.active = false;`) and copy only `0 .. statCount - 1` elements.
  - Maintain `std::atomic<uint32_t>` counters (`activeItemCount_`, `publishedCount_`, `g_hasActiveToast`) to fast-return before locking mutexes or allocating stack buffers when queues are empty.

### Pattern 6: Repeated `std::atomic<std::shared_ptr<T>>::load()` Calls in MSVC STL
- **Symptom:** Calling a helper like `ReadSettings()` that returns `std::atomic<std::shared_ptr<const Settings>>::load()` multiple times inside the same expression or hot function (`Controller QOL Updates` `ObserveCompletedName`, `SetGroundPickupPredicate`). In MSVC's STL (`<memory>`), `std::atomic<std::shared_ptr<T>>` is **not lock-free**—every `.load()` acquires a global spinlock and performs an atomic reference-count increment/decrement pair.
- **Fix:** Call `const auto settings = ReadSettings();` **once** at the top of the function scope and read fields from the local `shared_ptr`.

---

## 3. Reverse-Engineered Third-Party / Closed-Source Plugin Ledgers

### 3.1 `Buff HUD` (`d2rl-buff-panel.dll` v1.1.7 baseline -> v1.1.8 optimized)
- **Baseline binary:** `F:\SteamLibrary\steamapps\common\Diablo II Resurrected\mods\Reimagined\d2rloader\plugins\d2rl-buff-panel.dll.bak-20261026`
- **Upstream PR:** `Lukaszpg/d2rl-buff-hud#9`
- **Reimagined Port PR:** `D2R-Reimagined/d2rl-plugins#10` (`plugins/buff-panel`)

| Baseline RVA | Function / Role | Finding |
|---:|---|---|
| `0xC120` | `ReadMemorySafe(uintptr_t address, void* out, size_t bytes)` | Called `VirtualQuery` on every single memory read (`playerUnit`, `pStatListEx`, bitmasks, stat arrays), causing dozens of kernel transitions per frame. Replaced with `__try` / `__except` safe copy. |
| `0xCB70` | `BuffStateScanner::ScanActiveStates` | Reads `PlayerUnit` (`+0x10` unitId, `+0x90` pStatListEx), `StatListEx` (`+0x40` full stats array, `+0x90` child StatList linked list, `+0xAC` `192-bit` / `6 x uint32_t` active state bitmask). |
| `0xE190` | `BuffPanelPlugin::PumpOnGameThread` | Ran full state & stat-list scans every `120 Hz` callback instead of gating on `GameStateSnapshot::currentGameFrame` (`+0x54`, `25 Hz`). |
| `0x10DF0` | `BuffSlotWidgetManager::ApplySnapshots` | Iterated 21 slots (`0x88` bytes/slot) every frame, calling `WidgetService::SetVisible` (`+0x50`) and `SetEnabled` (`+0x58`) unconditionally even when slot visibility and texture were unchanged. |
| `0x13D80` | `BuffSlotWidgetManager::EnsureWidgets` | Walked all 21 slots calling `FindChild` every frame to detect UI destruction. Replaced with a 3-point canary (`slots[0, 10, 20]`) + `25 Hz` full-check cadence. |

### 3.2 `Bind and Summon` (`BindAndSummon.dll`, closed-source)
- **Binary path:** `F:\SteamLibrary\steamapps\common\Diablo II Resurrected\mods\Reimagined\d2rloader\plugins\BindAndSummon.dll` (`144,896 bytes`)
- **Measured cost:** `0.0369 - 0.0398 ms/frame` (`~0.47%` of a `120 FPS` frame budget).
- **Decision:** Left untouched. At `0.039 ms/frame`, binary-patching a closed-source DLL carries high maintenance risk for negligible frame-time gain (`< 0.5%` of a frame). Recorded below so we do not need to re-decompile it when a future D2R patch requires a clean-room rewrite.

| RVA | Function / Role | Decompiled Behavior |
|---:|---|---|
| `0x18D0` | `D2RL_CreatePlugin` / `OnTick` | Main per-frame callback (`PumpOnGameThread` / `OverlayService`). Checks `GetForegroundWindow() == g_hwnd` (`0x26C00`), reads key states (`GetAsyncKeyState`), and dispatches `0x3E80` and `0x3F20`. |
| `0x3E80` | `ValidatePlayerAndScanSummons` | Calls `0x34A0` to validate the local player unit, then calls `0x2260` to scan active pets/summons. |
| `0x34A0` | `ValidatePlayerUnitPtr` | Calls `VirtualQuery` on the player unit pointer (`0x26C08`) once per frame to verify `State == MEM_COMMIT` (`0x1000`) and readable protection (`0x66`). |
| `0x2260` | `ScanRosterPets` | Walks the client pet/roster linked list (`D2R.exe` global via `0x26BF8`), calling `VirtualQuery` on each node (`10–30` nodes/frame) and checking monster class IDs against `417` (Shadow Warrior), `418` (Shadow Master), `357` (Valkyrie), `290..292` (Spirits/Vines), `289` (Clay Golem), `290` (Blood Golem), `291` (Iron Golem), `292` (Fire Golem), `363` (Skeleton), `364` (Skeleton Mage), `420` (Grizzly), etc. |
| `0x3F20` | `DrawSummonOverlay` | Iterates tracked summon slots and submits overlay text/icons via `D2RL::OverlayService`. |
| `0x2F50` | `OnGameJoinOrLeave` | Resets cached unit pointers and reloads per-character bind config when session state transitions. |

---

## 4. Note on Shared `native_commands.cpp`
Several plugins (`pyre-lootfilter`, `projectile-perf`, `floating-damage`, `always-show-sockets`, `day-night-cycle`, `gem-bag-ui`, `Damage Meter`) vendor a copy of `native_commands.cpp` to hook `D2R.exe`'s slash-command dispatcher.
- In `EnsureInstalled()`, `native_commands.cpp` acquires a local `std::mutex` once per frame to check `state.hookInstalled`.
- Because `state.hookInstalled` is an uncontended local mutex checking a boolean and returning immediately, its total cost across all loaded plugins is **`< 0.002 ms/frame`** (`2 microseconds`).
- We intentionally keep `native_commands.cpp` **100% stock** across all repositories so shared command-hook lifecycle code remains identical and untouched.
