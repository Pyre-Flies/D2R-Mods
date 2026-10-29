# Stash Search (`1.10.12` by `yinyin333333`) Decompilation & Native Contracts

## Overview & Build Identity

- **Date:** 2026-10-23
- **Subject Binary:** `StashSearch.dll` (`id = "stash-search"`, `name = "Stash Search"`, `version = "1.10.12"`, `author = "yinyin333333"`)
- **Artifact Size & SHA-256:** `112,128` bytes, `D2305982248980583C54E4DB73C9CF7973DD7DCD1C71E2EF7EB9CA8F794E1A20`
- **Validated Game / Core Profiles:**
  - `D2R.exe` `3.3.94995` (Steam update `2026-10-22`, SHA-256 `5EFC64B7B3D5E6642227930AF536D158CADFF859E4986B5FC297632E087E0E22`)
  - `D2R.exe` `3.3.93847` (Battle.net installation, SHA-256 `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`)
  - `D2RCore.dll` (`1.3.1`, SHA-256 `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`)

## Key Findings on Game Update `3.3.94995`

1. **No Version-String Check:** `StashSearch.dll` does **not** check the `D2R.exe` product/file version string (`3.3.94995` vs `3.3.93847`). Instead, during `D2RLoaderLoadPlugin` (`SS+0x11800` -> `SS+0xc5a0`), it executes two binary admission validators:
   - `SS+0x10500`: Compares **26 hardcoded RVAs** in the decrypted in-memory `D2R.exe` image against exact byte sequences in `StashSearch.dll`'s `.rdata` (`SS+0x14520..0x14720`).
   - `SS+0x10220`: Compares **7 internal `D2RCore.dll` pointer-table entries** (`D2RCore+0x7004a8`, `+0x7004d8`, `+0x700dc0`, `+0x700dd8`, `+0x700df0`, `+0x700e08`, `+0x700e20`) against expected `D2R.exe` RVAs and checks that `D2RCore+0x700e38` is readable.
2. **All 26 Game RVAs and 7 `D2RCore.dll` Offsets Match `3.3.94995`:** In live process memory of `D2R.exe` `3.3.94995` (verified against live PID `46968`), all 26 `D2R.exe` `.text` witnesses and all 7 `D2RCore.dll` pointer table entries are 100% unchanged. `StashSearch.dll` passes both validators and installs its 17-byte SDK inline hook on `D2R+0x23aa60` (`BankPanel::Draw`).
3. **Why Stash Search Fails or Disappears After Updates / Across Modes:**
   - **Pre-Hook Prologue Guard & `Ultrawide` `1.3.5+rev.0`/`rev.1` Conflict (`0x8562F0`):** Because `SS+0x10500` checks the raw bytes of all 26 `D2R.exe` RVAs (`0x23aa60`, `0x8562f0`, etc.) in memory, any plugin that patches or hooks one of those 26 RVAs causes `StashSearch.dll` to fail admission (`D2RLoaderLoadPlugin` returns `false`, or if `Ultrawide` loaded after `StashSearch` and hooked the prologue of `0x8562f0` (`Widget::GetScreenPosition`), `Ultrawide` `1.3.5+rev.0`/`rev.1` collided directly with `StashSearch`'s 16-byte prologue guard at `0x8562f0`: `48 8B C4 48 89 70 20 57 48 81 EC A0 00 00 00 0F`, just as it did with `gem-bag-ui`! And in `Ultrawide` `1.3.5+rev.0`, `0x8562f0` also had a stack corruption crash at `[rsp+0x28]` when `Widget::GetScreenPosition` was called). `Ultrawide` `1.3.5+rev.2` fixed this by removing all hooks from `0x8562F0..0x856476` and moving corner HUD anchoring to `0x8A6461` (`UIRootWidget::Update`).
   - **Layout / Input-Mode Gating (`StashSearchAnchor`):** During every `BankPanel::Draw` frame (`SS+0x3e30` -> `SS+0x87a0` -> `SS+0x5210`), `SS+0x5210` looks up the child widget named `"StashSearchAnchor"` (`SS+0x149e0`) on `BankPanel` via `D2R+0x855c90` (`Widget::FindChild`). If `"StashSearchAnchor"` is missing, has `width <= 0` or `height <= 0`, or is hidden (`opacity <= 0.0f` at `widget+0x80` along the parent chain `widget+0x30`), `SS+0x5210` sets `valid = 0` (`[out+0x118] = 0`) and `SS+0x87a0` immediately disables and hides the search bar (`SS+0xdab0`).
     - In `Reimagined.mpq`, `"StashSearchAnchor"` (`rect: {x: 1096, y: 1506, width: 562, height: 93}`) exists **only** in the MKB layout (`data\global\ui\layouts\bankexpansionlayouthd.json`) and is **absent** in the controller layout (`data\global\ui\layouts\controller\bankexpansionlayouthd.json`) as well as unpatched vanilla/other mod layouts (e.g., `F:\Diablo II Resurrected\mods\Reimagined\Reimagined.mpq\data\global\ui\layouts\bankexpansionlayouthd.json` where `StashSearchAnchor` is missing).
   - **Window Client-Origin Check (`GetClientRect` / `ClientToScreen`):** `SS+0x5210` (`0x54d2..0x5514`) enumerates process windows (`SS+0x9c30`) to find the largest visible top-level window for the current process ID, calls `GetClientRect`, and requires `rect.left == 0` and `rect.top == 0` (`[rbp+0xc0] == 0 && [rbp+0xc4] == 0`) with ` anchor_rect` strictly inside `[0..clientWidth, 0..clientHeight]`.

---

## Complete Validated RVA & Byte-Guard Table (`SS+0x10500` & `SS+0x10220`)

### 1. `D2R.exe` Guarded RVAs (`SS+0x10500`)

| # | `D2R.exe` RVA | Cached Global in `StashSearch.dll` | Guard Size | Expected Bytes (`SS+.rdata`) | Reconstructed Symbol / Role |
|---|---:|---|---:|---|---|
| 1 | `0x23AA60` | Hooked via SDK (`trampoline` -> `SS+0x1b510`) | 17 B (`SS+0x14520`) | `40 53 55 56 57 48 83 EC 58 4C 89 B4 24 98 00 00 00` | `BankPanel::Draw(BankPanel* this)` — main render hook detouring to `SS+0x3e30` |
| 2 | `0x08B2D0` | `SS+0x1b518` (also `D2RCore+0x7004a8`) | 16 B (`SS+0x14538`) | `8B 05 AA B7 C5 01 C3 CC CC CC CC CC CC CC CC CC` | `GetLocalPlayerUnitId() -> uint32_t` (reads `dword ptr [D2R+0x1ce6a80]`) |
| 3 | `0x09A480` | `SS+0x1b520` (also `D2RCore+0x7004d8`) | 16 B (`SS+0x14548`) | `83 F9 FF 74 26 48 8B 05 C4 CC C4 01 8B D1 81 E2` | `GetClientPlayerUnitById(uint32_t unitId) -> UnitAny*` |
| 4 | `0x09A5D0` | `SS+0x1b528` | 16 B (`SS+0x14558`) | `83 FA FF 74 2E 83 F9 06 72 1A 8D 41 FA 83 F8 01` | `GetClientUnitByTypeAndId(uint32_t type, uint32_t unitId) -> UnitAny*` (called with `type=0` for shared-stash pseudo-player units) |
| 5 | `0x34A360` | `SS+0x1b530` | 16 B (`SS+0x14568`) | `48 85 C9 75 03 33 C0 C3 48 8B 81 90 00 00 00 C3` | `Unit_GetInventory(UnitAny* unit) -> Inventory*` (reads `[unit + 0x90]`) |
| 6 | `0x388C10` | `SS+0x1b538` | 16 B (`SS+0x14578`) | `48 85 C9 74 0F 81 39 06 05 04 10 75 07 48 8B 41` | `Inventory_GetFirstItem(Inventory* inv) -> UnitAny*` (validates magic `0x10040506`, returns `[inv + 0x20]`) |
| 7 | `0x38ABA0` | `SS+0x1b540` | 16 B (`SS+0x14588`) | `48 85 C9 74 14 83 39 04 75 0F 48 8B 41 10 48 85` | `Item_GetNextInventoryItem(UnitAny* item) -> UnitAny*` (checks `unit->dwUnitType == 4`, reads `item->pItemData` at `+0x10`) |
| 8 | `0x36CFE0` | `SS+0x1b548` | *(un-guarded)* | *(resolved at `SS+0xc9ad` without byte guard)* | `Item_GetLocationPage(UnitAny* item) -> uint8_t` |
| 9 | `0x3E2B634` | `SS+0x1b550` | 16 B (`SS+0x14598`) | `FF 25 86 F6 FF FF FF 25 88 F6 FF FF FF 25 8A F6` | Import/delay thunk -> `D2RCore.dll+0x817c40`: `BuildItemFullDescription(UnitAny* item, wchar_t* outBuf, uint64_t maxChars)` |
| 10 | `0x300A90` | `SS+0x1b558` | 16 B (`SS+0x145a8`) | `48 8B 05 19 F0 9F 01 C3 CC CC CC CC CC CC CC CC` | `GetDataTables() -> sgptDataTables*` (reads `[D2R+0x1cffab0]`) |
| 11 | `0x3E2B730` | `SS+0x1b560` | 6 B (`SS+0x145b8`) | `FF 25 DA F6 FF FF` | Import/delay thunk -> `D2RCore.dll+0x81f290`: `LookupStringByIndex(uint16_t stringId) -> const char*` |
| 12 | `0x2C49F0` | `SS+0x1b568` | 32 B (`SS+0x145c0`) | `48 89 5C 24 20 56 41 56 41 57 48 83 EC 40 4C 8B FA 48 8B F1 33 DB E8 05 2E FE FF 4C 8B F0 48 85` | `InventoryGridWidget::GetItemAtCell(InventoryGridWidget* grid, const int32_t cellCoord[2]) -> UnitAny*` |
| 13 | `0x5F4B90` | `SS+0x1b570` | *(un-guarded)* | *(resolved at `SS+0xca2a` without byte guard)* | `GetLocalizedStringByKey(const KeyView* key) -> const char*` (looks up `"LobbySearch"`, `"personal"`, `"shared"`) |
| 14 | `0x855C90` | `SS+0x1b578` | 16 B (`SS+0x145e0`) | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57` | `Widget::FindChild(Widget* parent, const char* childName) -> Widget*` |
| 15 | `0x23AF50` | `SS+0x1b580` (also `D2RCore+0x700dc0`) | 16 B (`SS+0x145f0`) | `48 89 5C 24 08 57 48 83 EC 20 48 8D 15 3F 9C AA` | `BankPanel::GetSelectedTab(BankPanel* panel) -> uint8_t` (`0` = Personal, `1` = Shared, `2` = Gems, `3` = Materials, `4` = Runes) |
| 16 | `0x878FB0` | `SS+0x1b588` | 16 B (`SS+0x14600`) | `48 89 5C 24 18 89 54 24 10 55 56 57 48 83 EC 30` | `TabBarWidget::SelectTab(TabBarWidget* tabBar, int tabIndex, bool notify)` |
| 17 | `0x8562F0` | `SS+0x1b5d8` | 16 B (`SS+0x14610`) | `48 8B C4 48 89 70 20 57 48 81 EC A0 00 00 00 0F` | `Widget::GetScreenPosition(Widget* widget, int32_t outPos[2]) -> int32_t*` |
| 18 | `0x8562A0` | `SS+0x1b5e0` | 16 B (`SS+0x14620`) | `40 53 48 83 EC 30 80 79 52 00 48 8B DA 74 2A 48` | `Widget::GetRectSize(Widget* widget, int32_t outRect[4]) -> int32_t*` (reads `[widget + 0x70]`) |
| 19 | `0x86D1B0` | `SS+0x1b5b8` | 32 B (`SS+0x14630`) | `40 53 48 83 EC 20 C7 41 30 00 00 80 3F 33 C0 89 41 2C 0F 57 C0 89 41 3C 48 8B D9 88 41 46 89 41` | `TextStyle::Init(TextStyle* style, uint32_t fontType, const void* colorData)` |
| 20 | `0x902E20` | `SS+0x1b5c0` | 32 B (`SS+0x14650`) | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 70 0F 29 74 24 60 49 8B F0 0F 28 F3 48 8B FA 48 8B D9` | `FontRenderer::DrawStyledText(const char* utf8Text, const IntRect* rect, const void* styleBuf, float scale)` |
| 21 | `0x903CD0` | `SS+0x1b5c8` | 32 B (`SS+0x14670`) | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 50 0F 29 74 24 40 49 8B F9 0F 28 F2 48 8B F2 48 8B D9` | `FontRenderer::MeasureStyledText(const char* utf8Text, const void* styleBuf, float scale, const IntRect* rect) -> uint64_t` |
| 22 | `0x657B90` | `SS+0x1b5d0` | 32 B (`SS+0x14690`) | `44 3B C1 0F 8C D3 02 00 00 4C 8B DC 55 41 54 41 55 41 57 49 8D 6B A8 48 81 EC 38 01 00 00 48 8B` | `DrawSolidRect(int left, int top, int right, int bottom, const float* color4f, int flag)` (draws search box, borders, dropdown, & dimming quads) |
| 23 | `0x86D489` | *(Interior witness only)* | 31 B (`SS+0x146b0`) | `8B 97 B0 00 00 00 4C 8D 87 B4 00 00 00 48 8D 4C 24 50 4C 89 B4 24 F8 01 00 00 E8 08 FD FF FF 80` | Verifies `TextBoxWidget` style offsets `+0xb0` (dword) and `+0xb4` (struct) passed to `0x86d1b0` |
| 24 | `0x888B8D` | *(Interior witness only)* | 29 B (`SS+0x146d0`) | `FF 90 C8 00 00 00 0F 28 DF 4C 8D 87 58 06 00 00 48 8D 54 24 48 48 8B 08 E8 76 A2 07 00 48 B8 00` | Verifies renderer vtable call `[rax + 0xc8]` before `0x902e20` (`FontRenderer::DrawText`) |
| 25 | `0x23B060` | `SS+0x1b590` (also `D2RCore+0x700df0`) | 16 B (`SS+0x146f0`) | `48 83 EC 28 E8 A7 08 00 00 84 C0 74 2D E8 5E 02` | `BankPanel::GetSharedStashPageCount(BankPanel* panel) -> uint64_t` |
| 26 | `0x23B910` | `SS+0x1b598` (also `D2RCore+0x700dd8`) | 16 B (`SS+0x14700`) | `40 57 48 83 EC 20 48 8D 15 7B 93 AA 01 E8 6E A3` | `BankPanel::IsShowingPreviousLadderSeason(BankPanel* panel) -> bool` (checks `"ShowPreviousLadderSeasonToggle"`) |
| 27 | `0x23AD80` | `SS+0x1b5a0` (also `D2RCore+0x700e08`) | 16 B (`SS+0x14710`) | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57` | `BankPanel::GetSelectedSharedStashRecord(BankPanel* panel) -> SharedStashRecord*` |
| 28 | `0x23E630` | `SS+0x1b5a8` (also `D2RCore+0x700e20`) | 16 B (`SS+0x14720`) | `48 89 5C 24 10 55 56 57 41 54 41 55 41 56 41 57` | `BankPanel::UpdateActiveStashView(BankPanel* panel, UnitAny* playerUnit)` |
| 29 | `0x2F5020` | `g_Native.fnGetUnitStat` (extension) | 12 B (`+0x2F502A` tail) | `44 8B C2 48 8B 91 88 00 00 00 33 C0` | `STATLIST_GetUnitStatSigned(UnitAny* unit, uint32_t statId, uint16_t layer) -> int32_t` (tail-guarded at `+0x0A` to coexist with `D2RLoader` entry detour; queried with `statId = 194` (`STAT_ITEM_NUMSOCKETS`), `layer = 0` when `ITEMFLAG_SOCKETED` (`0x800`) is set on `[itemUnit + 0x10] + 0x18`) |

*(Note: 26 RVAs are byte-checked in `SS+0x10500`, and 2 additional RVAs `0x36cfe0` and `0x5f4b90` are resolved into `SS+0x1b548` and `SS+0x1b570` without byte guards.)*

### 2. `D2RCore.dll` Guarded Offsets (`SS+0x10220`)

In `SS+0x10220`, `StashSearch.dll` verifies that `D2RCore.dll`'s internal hook/function descriptor table contains the exact runtime addresses of the `D2R.exe` functions above, and caches `&D2RCore.dll[0x700e38]` in `SS+0x1b5b0`:

| `D2RCore.dll` Offset | Expected Value (`gameBase + RVA`) | Purpose in `D2RCore.dll` / `StashSearch.dll` |
|---:|---|---|
| `+0x7004A8` | `D2R + 0x08B2D0` | `GetLocalPlayerUnitId` descriptor target |
| `+0x7004D8` | `D2R + 0x09A480` | `GetClientPlayerUnitById` descriptor target |
| `+0x700DC0` | `D2R + 0x23AF50` | `BankPanel::GetSelectedTab` descriptor target |
| `+0x700DD8` | `D2R + 0x23B910` | `BankPanel::IsShowingPreviousLadderSeason` descriptor target |
| `+0x700DF0` | `D2R + 0x23B060` | `BankPanel::GetSharedStashPageCount` descriptor target |
| `+0x700E08` | `D2R + 0x23AD80` | `BankPanel::GetSelectedSharedStashRecord` descriptor target |
| `+0x700E20` | `D2R + 0x23E630` | `BankPanel::UpdateActiveStashView` descriptor target |
| `+0x700E38` | `int32_t` (default `-1` / `0xFFFFFFFF`, stored at `SS+0x1b5b0`) | `D2RCore` active shared-stash unit ID override state (written during `SS+0x7980` when switching shared stash pages) |

---

## Decompiled Architecture of `StashSearch.dll`

### 1. Plugin Lifecycle (`D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`, `D2RLoaderUnloadPlugin`)

```cpp
// RVA 0x117F0
extern "C" const PluginInfo* D2RLoaderGetPluginInfo() {
    static const PluginInfo kInfo = {
        "stash-search",   // id (0x144A0)
        "Stash Search",   // name (0x144B0)
        "1.10.12",        // version (0x144C0)
        "yinyin333333",   // author (0x144C8)
        nullptr,
        0x00010002
    };
    return &kInfo;
}

// RVA 0x11800 -> 0xC5A0
extern "C" bool D2RLoaderLoadPlugin(const PluginContext* ctx) {
    g_isShuttingDown = false; // SS+0x1B26B
    if (!ctx || ctx->structSize < 0xC8 || !ctx->hooks || ctx->hooks->structSize < 0xC0)
        return false;
    if (!ctx->hooks->installInlineHook || !ctx->hooks->removeHook)
        return false;

    uintptr_t gameBase = ctx->gameBase; // [ctx + 0x18]
    if (!gameBase) {
        gameBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        if (!gameBase) return false;
    }

    HMODULE coreMod = GetModuleHandleW(L"D2RCore.dll");
    uintptr_t coreBase = reinterpret_cast<uintptr_t>(coreMod);
    if (!coreBase) return false;

    g_gameBase = gameBase; // SS+0x1B500
    g_coreBase = coreBase; // SS+0x1B508

    // Validate all 26 D2R.exe byte signatures and 7 D2RCore.dll pointer-table entries
    if (!ValidateGameSignatures(gameBase) || !ValidateCorePointers(gameBase, coreBase)) {
        ClearResolvedPointers(); // SS+0x4EA0
        return false;
    }

    // Populate SS+0x1B518 .. SS+0x1B5E0 with resolved function pointers
    ResolveFunctionPointers(gameBase, coreBase);

    // Spawn worker thread (SS+0xBEC0) that creates a hidden message window,
    // hidden Win32 EDIT control, and installs WH_MOUSE_LL (14) + WH_KEYBOARD_LL (13) hooks.
    // Waits up to 5000ms (0x1388) on startup event handle (SS+0x1B3C8).
    if (!StartInputWorkerThread()) {
        StopInputWorkerThread(); // SS+0x4F70
        ClearResolvedPointers(); // SS+0x4EA0
        return false;
    }

    // Install 17-byte inline hook at D2R+0x23AA60 -> Detour_BankPanelDraw (SS+0x3E30)
    HookHandle handle = 0;
    if (!ctx->hooks->installInlineHook(
            ctx,
            "stash-search.sub_14023aa60",
            reinterpret_cast<void*>(gameBase + 0x23AA60),
            reinterpret_cast<void*>(&Detour_BankPanelDraw),
            reinterpret_cast<void**>(&g_origBankPanelDraw), // SS+0x1B510
            0x11, // 17 bytes
            &handle) || !handle || !g_origBankPanelDraw) {
        RemoveBankPanelHook();   // SS+0xF680
        StopInputWorkerThread(); // SS+0x4F70
        ClearResolvedPointers(); // SS+0x4EA0
        return false;
    }

    g_hookCtx = ctx;       // SS+0x1B4F0
    g_hookHandle = handle; // SS+0x1B4F8
    return true;
}
```

### 2. `BankPanel::Draw` Hook (`SS+0x3E30` -> `SS+0x87A0` & `SS+0x68D0`)

Every frame that `BankPanel::Draw` (`D2R+0x23aa60`) runs:
1. Calls the original `BankPanel::Draw` trampoline (`SS+0x1B510`).
2. Updates the last-seen tick count (`GetTickCount64()` -> `SS+0x1B100`) so the worker thread knows the Stash panel is currently open (`now - lastDrawTick <= 250 ms`).
3. Calls `RenderSearchBarAndDropdown(bankPanel)` (`SS+0x87A0`):
   - Calls `ComputeSearchBarLayout(bankPanel)` (`SS+0x5210`), which:
     - Looks up child widget `"StashSearchAnchor"` (`SS+0x149E0`) on `bankPanel` via `D2R+0x855c90`.
     - Queries its screen position (`D2R+0x8562f0`) and size (`D2R+0x8562a0`).
     - Walks up the parent chain (`[widget + 0x30]`, up to 64 levels) multiplying each widget's opacity float at `[widget + 0x80]`. If any parent has `opacity <= 0.0f` or `NaN`, aborts.
     - Finds the game's top-level `HWND` (`SS+0x9c30`), checks `GetClientRect`, and verifies the anchor rect is inside the client area.
   - Resolves text style from `"StashSearchAnchor"` (or falls back to `"BankTabs"`, `"gold_amount"`, `"title"`), reading font/color fields at `widget + 0xb0` and `widget + 0xb4` and initializing a `TextStyle` struct via `D2R+0x86d1b0`.
   - Draws the search box background and borders using `D2R+0x5f4b90`, and renders the current search query (or localized `"Search"` placeholder + match count `(current/total)`) using `D2R+0x902e20` (`FontRenderer::DrawText`).
   - When the dropdown is open, renders up to `N` matching stash items with a scrollbar.
4. Calls `DimNonMatchingGridItems(bankPanel)` (`SS+0x68D0`):
   - When a search filter is active (`hasQuery != 0`), looks up child `"grid"` (`InventoryGridWidget`) on `bankPanel` via `D2R+0x855c90`.
   - Reads the grid's screen position (`D2R+0x8562f0`), size (`D2R+0x8562a0`), unscaled integer cell size (`int32_t` `cellSizeX` at `grid + 0x5b8`, `int32_t` `cellSizeY` at `grid + 0x5bc`, e.g., `98x98`), cell counts (`int32_t` `cols` at `grid + 0x628`, `int32_t` `rows` at `grid + 0x62c`), and calls `D2R+0x2c49f0` (`InventoryGridWidget::GetItemAtCell(grid, const int32_t cellCoord[2])`).
   - For each item in the current stash tab, builds its full item tooltip/description string via `D2R+0x3e2b634` (`D2RCore+0x817c40`), strips D2R color codes (`ÿc...`), and checks if it matches the search query.
   - For every item that **does not** match the query, draws a semi-transparent dark overlay rectangle (`0xB4000000`) over its grid cells via `D2R+0x5f4b90`.

### 3. Cross-Tab Stash Scanning & Page Switching (`SS+0x41E4`, `SS+0xD800`, `SS+0x7980`)

- **Scanning All Personal & Shared Tabs (`SS+0x41E4` & `SS+0x9A90`):**
  - Resolves the local player unit via `D2R+0x08b2d0` -> `D2R+0x09a480`, and gets the Personal Stash `Inventory*` via `D2R+0x34a360`.
  - Iterates items in the Personal Stash (`D2R+0x388c10` / `D2R+0x38aba0` where `Item_GetLocationPage` `D2R+0x36cfe0 == 4`).
  - Walks the linked list of Shared Stash tab records at `[inventory + 0x68]` (next pointer at `[node + 0x10]`, active flags at `[node + 0x08] == 0` and `[node + 0x0a] != 1`, pseudo-unit ID at `dword ptr [node + 0x00]`).
  - Resolves each Shared Stash pseudo-unit via `D2R+0x09a5d0(0, sharedUnitId)` -> `D2R+0x34a360` and scans all items in each shared tab (`SS+0x9A90` -> `SS+0x37E0`).
  - Also resolves base item names via `GetDataTables()` (`D2R+0x300a90`): reads `itemsTxt = [dataTables + 0x13a8]` (stride `0x1cc`), gets `namestr` index at `word ptr [record + 0x30]`, looks up the string key in `[dataTables + 0x1388]` (stride `0x160`), and calls `D2R+0x3e2b730` (`D2RCore+0x81f290`) to resolve localized item base names (`SS+0xF050`).
- **Jumping to a Matching Tab (`SS+0xD800` & `SS+0x7980`):**
  - If the selected match is in the **Personal** stash (`isShared == 0`), finds `"BankTabs"` (`D2R+0x855c90`) and calls `TabBarWidget::SelectTab(bankTabs, 0, 0)` (`D2R+0x878fb0`).
  - If the selected match is in a **Shared** stash tab (`isShared == 1`):
    1. If `"BankTabs"` is not currently on tab `1` (`BankPanel::GetSelectedTab` `D2R+0x23af50 != 1`), first calls `TabBarWidget::SelectTab(bankTabs, 1, 0)` (`D2R+0x878fb0`) and retries on the next frame (up to 3 frames).
    2. Once on tab `1`, calls `SwitchSharedStashSubpage(bankPanel, pageIndex)` (`SS+0x7980`):
       - Checks `BankPanel::GetSharedStashPageCount(bankPanel)` (`D2R+0x23b060`) and `BankPanel::IsShowingPreviousLadderSeason(bankPanel)` (`D2R+0x23b910`).
       - Writes `pageIndex` into `bankPanel + 0x168` (current season) or `bankPanel + 0x170` (previous ladder season).
       - Calls `BankPanel::GetSelectedSharedStashRecord(bankPanel)` (`D2R+0x23ad80`), reads the selected shared unit ID (`dword ptr [record + 0x00]`), writes it to `D2RCore.dll+0x700e38` (`SS+0x1B5B0`), and calls `BankPanel::UpdateActiveStashView(bankPanel, localPlayerUnit)` (`D2R+0x23e630`).

### 4. Input Worker Thread & Win32 Hooks (`SS+0xBEC0`, `SS+0x3080`)

- Spawns a background thread (`SS+0xBEC0`) that registers window class `"StashSearchHiddenHostWindow"` (`SS+0x14AC8`), creates a message-only host window and a child Win32 `EDIT` control (`"EDIT"`, `SS+0x14B00`) for IME/clipboard/text-editing support, and installs global low-level hooks:
  - `SetWindowsHookExW(WH_MOUSE_LL /*14*/, LowLevelMouseProc /*SS+0xBA90*/, hInst, 0)`
  - `SetWindowsHookExW(WH_KEYBOARD_LL /*13*/, LowLevelKeyboardProc /*SS+0xBC20*/, hInst, 0)`
- **When `BankPanel::Draw` has run within the last `250 ms` (`0xFA` ms check at `SS+0xCB90`)**, clicking inside the computed search box rectangle focuses the hidden `EDIT` control and intercepts keyboard/mouse wheel input to update the search string and scroll/select dropdown results.

### 5. Shared-Page Slash Command (`SS+0xD650`, `SS+0xD3C0`)

- In addition to substring item search, `StashSearch.dll` implements a `/` slash-command parser (`SS+0xD650`, `ParseSlashSharedPageCommand`) that allows jumping directly to a 1-based Shared Stash subpage:
  - Query does not start with `'/'` (`0x2f`) -> `SlashCommandState::None` (`0`): normal substring item search across Personal + Shared tabs and non-matching grid dimming (`SS+0x68D0`).
  - Query is `"/"` (or `"/"` followed only by whitespace) -> `SlashCommandState::MissingNumber` (`1`): dropdown shows single row `L"Enter shared page number"` (`SS+0x14930`).
  - Query is `"/0"` or contains non-digits / overflow (`> 999999`) -> `SlashCommandState::InvalidNumber` (`2`): dropdown shows single row `L"Invalid page number"` (`SS+0x14968`).
  - Query is `"/<N>"` (`1 <= N <= 999999`) -> `SlashCommandState::ValidPage` (`3`): dropdown shows single row `L"Press Enter to jump to shared page <N>"` (`SS+0x14990`), suppresses non-matching grid dimming in `SS+0x68D0`, and pressing `Enter` or clicking the dropdown row queues a jump to 0-based shared page `N - 1`.

---

## Complete Internal `StashSearch.dll` Function RVA Map

| `StashSearch.dll` RVA | Reconstructed Symbol / Responsibility |
|---:|---|
| `0x3080` | `HiddenHostWndProc(HWND, UINT, WPARAM, LPARAM)` — handles custom `WM_APP+1` (`0x8001` focus edit) and `WM_APP+2` (`0x8002` unfocus edit) |
| `0x3190` | `IsPointInRect(POINT, const RectF&)` — hit-test helper (`x in [l, r)`, `y in [t, b)`) |
| `0x31E0` | `GetOrCreateEditFont(int pixelHeight)` — caches `CreateFontW("Microsoft YaHei UI", ...)` in `SS+0x1B3D8` |
| `0x37E0` | `ScanSingleInventoryItems(Inventory*, bool isShared, int sharedTabIndex, const wstring& query, vector<DropdownEntry>&)` |
| `0x3E30` | `Detour_BankPanelDraw(void* bankPanel)` — main `BankPanel::Draw` hook detour |
| `0x4190` | `FormatDisplayLabelWithCount(const wstring& baseText, size_t matchCount, bool isSharedCommand)` |
| `0x4480` | `PointInRectXY(int x, int y, const RectF&)` |
| `0x44D0` | `ClampScrollOffset(int requested, int totalItems, int maxVisibleRows)` |
| `0x4510` | `GetScrollbarMetrics(const DropdownLayout&, int totalItems, int maxVisibleRows, int scrollOffset)` |
| `0x4630` | `ComputeScrollOffsetFromThumbY(int mouseY, float trackTop, float trackHeight, float thumbHeight, int totalItems, int maxVisible)` |
| `0x46E0` | `ComputeDropdownLayout(const SearchBarLayout&, size_t rowCount)` |
| `0x4890` | `GetHoveredDropdownRow(const DropdownLayout&, int mouseX, int mouseY, size_t visibleRows)` |
| `0x4930` | `DrawSolidRectHelper(void* renderCtx, float l, float t, float r, float b, float scaleX, float scaleY, uint32_t color)` |
| `0x4AF0` | `UpdateInputFocusAndEditWindow()` — synchronizes hidden `EDIT` focus and text when search bar focus changes |
| `0x4EA0` | `ClearResolvedPointers()` — zeroes `SS+0x1B500..0x1B5E0` |
| `0x4F70` | `StopInputWorkerThread()` — signals shutdown, posts `WM_QUIT` (`0x12`), joins worker thread (`SS+0x1B3C0`) |
| `0x5060` | `DestroyHiddenWindowsAndHooks()` — unhooks `WH_MOUSE_LL` / `WH_KEYBOARD_LL`, destroys `EDIT` and host `HWND`, deletes `HFONT` |
| `0x5210` | `ComputeSearchBarLayout(void* bankPanel)` — resolves `"StashSearchAnchor"`, checks parent opacity chain, computes UI rects |
| `0x5DC0` | `ResolveFontAndTextStyle(void* bankPanel, void* anchorWidget, uint32_t* outFont, uint8_t* outColor)` |
| `0x60A0` | `DrawFittedTextInRect(...)` — measures UTF-8 string (`D2R+0x903CD0`), binary-searches suffix/prefix truncation with `"..."` if needed, draws via `D2R+0x902E20` |
| `0x68D0` | `DimNonMatchingGridItems(void* bankPanel)` — dims non-matching items in `"grid"` (`InventoryGridWidget`) with `0xB4000000` |
| `0x7980` | `SwitchSharedStashSubpage(void* bankPanel, int pageIndex)` — switches shared stash page and updates `D2RCore.dll+0x700E38` |
| `0x7BA0` | `ReadEditControlText(HWND editHwnd)` — reads `GetWindowTextLengthW` / `GetWindowTextW`, strips `\r`/`\n`, caps at 80 chars |
| `0x8120` | `SyncQueryStateFromEditControl(HWND editHwnd)` — updates `SS+0x1B140` (`searchQuery`) under `SRWLOCK` when `EDIT` text changes |
| `0x87A0` | `RenderSearchBarAndDropdown(void* bankPanel)` — full search box + dropdown list + scrollbar renderer |
| `0x9A90` | `ScanAllStashesForMatches(const wstring& query, vector<DropdownEntry>&)` — scans Personal Stash + all Shared Stash tabs |
| `0x9C30` | `EnumWindowsCallback_FindGameWindow(HWND, LPARAM)` — finds largest visible top-level window belonging to current PID |
| `0x9D40` | `FindGameMainWindow()` — cached wrapper (250 ms TTL at `SS+0x1B4D8`/`0x1B4E0`) around `EnumWindows` |
| `0x9E40` | `GetLocalizedSearchPlaceholder()` — caches localized `"Search"` string via `D2R+0x657B90` (`SS+0x1B470..0x1B4C8`) |
| `0xA8E0` | `HandleMouseEvent(UINT msg, const MSLLHOOKSTRUCT*)` — processes `WM_LBUTTONDOWN`, `WM_LBUTTONUP`, `WM_MOUSEMOVE`, `WM_MOUSEWHEEL` |
| `0xB560` | `HandleKeyboardEvent(UINT msg, const KBDLLHOOKSTRUCT*)` — processes `VK_RETURN`, `VK_ESCAPE`, `VK_BACK`, `Ctrl+A/C/V/X`, and character keys |
| `0xBA90` | `LowLevelMouseProc(int nCode, WPARAM, LPARAM)` — `WH_MOUSE_LL` callback |
| `0xBC20` | `LowLevelKeyboardProc(int nCode, WPARAM, LPARAM)` — `WH_KEYBOARD_LL` callback |
| `0xBEC0` | `InputWorkerThreadProc(LPVOID)` — hidden window & hook message pump thread |
| `0xC5A0` | `LoadPluginInternal(const D2RL_PluginContext*)` — main plugin initialization |
| `0xCB90` | `IsStashPanelRecentlyDrawn()` — checks `GetTickCount64() - g_lastBankDrawTick <= 250` |
| `0xCC20` | `StripColorCodesAndNormalize(wstring_view)` — strips `ÿc` (`\xFF\x63`) color tags and normalizes control chars |
| `0xD020` | `ToLowerFolded(wstring_view)` — ASCII + `CharLowerBuffW` lowercase folding |
| `0xD3C0` | `BuildSlashCommandDropdownEntry(SlashCommandState, int targetPage1Based)` |
| `0xD650` | `ParseSlashSharedPageCommand(wstring_view, int* outPage1Based)` |
| `0xD800` | `ProcessPendingTabSwitch(void* bankPanel)` — executes queued Personal / Shared tab + subpage navigation |
| `0xDAB0` | `DeactivateSearchAndCloseDropdown()` — clears focus, dropdown, and drag state when stash panel closes |
| `0xEB00..0xF2B0` | `__try / __except(EXCEPTION_EXECUTE_HANDLER)` safe native call and memory access wrappers (`SafeCall_GetLocalPlayerUnitId`, `SafeGetItemBaseName`, etc.) |
| `0xF680` | `RemoveBankPanelHook()` — calls `ctx->hooks->removeHook(ctx, g_hookHandle)` |
| `0x10220` | `VerifyD2RCoreExports(uintptr_t exeBase, uintptr_t coreBase)` |
| `0x10500` | `VerifyAllGameSignatures(uintptr_t exeBase)` |
| `0x117F0` | `D2RLoaderGetPluginInfo()` |
| `0x11800` | `D2RLoaderLoadPlugin(const D2RL_PluginContext*)` |
| `0x11850` | `D2RLoaderUnloadPlugin()` |

---

## Reconstructed & Extended C++20 Source Codebase (`pyre-stashsearch`)

A complete, buildable C++20 implementation (`pyre-stashsearch.dll` `1.0.0`) resides in `I:\D2R-Mods\plugins\pyre-stashsearch\`:
- `I:\D2R-Mods\plugins\pyre-stashsearch\CMakeLists.txt` — CMake build target (`pyre-stashsearch` -> `pyre-stashsearch.dll`) plus `pyre-stashsearch-unit` and `pyre-stashsearch-artifact` test targets.
- `I:\D2R-Mods\plugins\pyre-stashsearch\src\version.rc` — Embedded `D2RL_PLUGIN_MANIFEST_RESOURCE_ID` (`1001`, `RT_RCDATA`, ABI v3) and `VS_VERSION_INFO` (`1.0.0.0`).
- `I:\D2R-Mods\plugins\pyre-stashsearch\src\stash_search.h` — Constants, UI layout/scroll math, color-code stripping (`0xCC20`), case folding (`0xD020`), slash-command parser (`0xD650`), and `StashSearchConfig` TOML persistence.
- `I:\D2R-Mods\plugins\pyre-stashsearch\src\native.h` & `I:\D2R-Mods\plugins\pyre-stashsearch\src\native.cpp` — All `D2R.exe` byte guards (`0x10500`), 7 `D2RCore.dll` pointer table checks (`0x10220`), and SEH-protected native wrappers (`0xEB00..0xF2B0`).
- `I:\D2R-Mods\plugins\pyre-stashsearch\src\plugin.cpp` — Full plugin lifecycle, `BankPanel::Draw` hook (`0x3E30`), standalone `"BankTabs"` fallback layout (`0x5210`), search bar & dropdown renderer (`0x87A0`), matching item grid highlighter (`0x68D0`), stash scanner (`0x9A90`), shared-stash page jumper (`0x7980`, `0xD800`), expandable Side Tab Flyout with right-click custom tab names, and Win32 input thread (`0xBEC0`).
- `I:\D2R-Mods\plugins\pyre-stashsearch\tests\unit_tests.cpp` & `I:\D2R-Mods\plugins\pyre-stashsearch\tests\artifact_tests.cpp` — Unit and DLL artifact contract tests.

---

## Newly Discovered `D2R.exe` Item Tooltip & Affix Builder RVAs (Enhanced Stash Search)

| `D2R.exe` RVA | Prologue Guard (16 bytes) | Signature & Role |
|---:|---|---|
| `0x02BD480` | `48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56` | `BuildFullHoverItemTooltip(uintptr_t playerUnit, uintptr_t itemUnit, ...)` — Top-level hover tooltip builder; calls `0x3E2B634` (`BuildItemFullDescription`), `0x2DA0E0` (`BuildItemStatsString` at `0x2BE16C`), and `0x2BF1B0` (`FormatItemTooltipSection` at `0x2BE6A0`). |
| `0x02BF1B0` | `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48` | `FormatItemTooltipSection(...)` — Formats tooltip sections and invokes `0x2DA310` (`BuildSocketableStatsString` at `0x2BFD68`) for socketable items. |
| `0x02DA0E0` | `40 53 55 56 57 41 55 41 56 41 57 48 81 EC 80 04` | `BuildItemStatsString(uintptr_t itemUnit, char* outBuf, int capacity, uint8_t flag1, uintptr_t prefixStr, uintptr_t arg6, ItemStatOptArg* optArg)` — Invokes `D2R.exe+0x3E2B832` (`D2Client_BuildItemPropertiesDescription`) to format all magic/rare/unique/set/crafted/runeword affix lines into `outBuf` (`char[1024]`). |
| `0x02DA310` | `48 85 D2 0F 84 F0 01 00 00 44 89 4C 24 20 4C 89` | `BuildSocketableStatsString(uintptr_t playerUnit, uintptr_t itemUnit, char* outBuf, int capacity)` — Formats Weapon / Armor / Helm / Shield socketable bonuses for runes, gems, and jewels (`itemType == 4 && itemKind == 5`) into `outBuf` (`char[1024]`). |
| `0x02F5020` | *(Tail guard at `0x2F502A`: `48 85 C9 74 09 0F B7 D2 FF E0 33 C0 C3`)* | `STATLIST_GetUnitStatSigned(uintptr_t unit, uint32_t statId, uint16_t layer) -> int32_t` — Reads any unit stat (such as `STAT_ITEM_NUMSOCKETS = 194`) even on background shared-stash items whose `BuildItemFullDescription` does not emit `"Socketed (N)"`. |
| `0x023A730` | `40 55 53 56 57 41 56 48 83 EC 30 45 0F B6 F0 0F` | `BankPanel::StepSharedStashSubpage(uintptr_t bankPanel, uint8_t forward, uint8_t wrap)` — Native shared stash subpage stepper that updates `bankPanel + 0x168` (`0x170` for previous ladder season) and calls `D2R.exe+0x3E2B2F2` (hooked by `D2RCore.dll+0x1C9C09`). |
| `0x0877D20` | `4C 8B DC ...` | `TabBarWidget::ResolveEnabledTabIndex(uintptr_t tabBar, uint32_t desiredTab, uint8_t forward)` — Reads total tab count at `tabBar + 0x558` (`int32_t`), current selected tab at `tabBar + 0x16C4` (`int32_t`), and per-tab enabled flags at `tabBar + 0x1788 + tabIdx` (`uint8_t`). Called by `TabBarWidget::SelectTab` (`0x878FB0`), which optionally plays UI sound `*(uint32_t*)(tabBar + 0x1550)` via `0x1A0780` when `playSound != 0`. Note that `0x878FB0` executes `int 3` (`swi(3)`) if `desiredTab < 0` or `desiredTab >= *(int32_t*)(tabBar + 0x558)`, and invokes the tab-changed std::function callback at `tabBar + 0x1758` (`target` at `+0x1760`, hooked by `D2RCore.dll+0x1C9D40`). |
| `0x023E630` | `48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48` | `BankPanel::UpdateActiveView(uintptr_t bankPanel, uintptr_t playerUnit)` — Rebinds the active stash view (`0=Personal`, `1=Shared`, `2=Gems`, `3=Materials`, `4=Runes`), toggling `"BankPages"` (`*(uintptr_t*)(bankPanel + 0x278)`), `"grid"` (`*(uintptr_t*)(bankPanel + 0x2A0)`, calling `InventoryGridWidget::ClearAndRebuild` at `0x2AAB40` and `0x2AD2B0`), and `"advancedstash_gems"` / `"advancedstash_materials"` / `"advancedstash_runes"`. Must be invoked **before** `BankPanel::Draw` (`0x23AA60`) on a given frame so child widgets and async HD item texture loads stay coherent. |




