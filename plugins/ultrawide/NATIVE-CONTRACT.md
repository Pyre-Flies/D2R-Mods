# Ultrawide Native Contract (`1.3.5+rev.2`)

## 1. Host & Provider Admission

`Ultrawide.dll` requires exact SHA256 verification before installing any hook or byte mutation:

- `D2R.exe`: `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` (file size `41,693,976` bytes; runtime capture image `AC72D9C38488C2BC5B44831E7EBE51FC75EA44B3B48DD741A607CCE03C6279F9`)
- `D2RCore.dll`: `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` (D2RLoader `1.3.1`, Plugin SDK ABI version `4`)

If either image hash or any subsystem byte witness mismatches, the affected subsystem logs a warning and remains inert without modifying game memory.

---

## 2. Subsystem 1: Pillarbox & Vignette Removal (`enable_bars_removal`)

| RVA | Size | Role | Expected Bytes / Contract |
| --- | --- | --- | --- |
| `0x844165` | `15` | `InlineHookRequest` -> `UltrawideMenuEntry` | `84 C0 0F 84 A9 00 00 00 4C 8D 05 E4 CA 52 01` (`test al, al; je 0x844216; lea r8, [0x1D70C58]`). In menus (`UltrawideInSession == 0`), replays `r8 = exeBase + 0x1D70C58` and resumes at `0x844174`. In gameplay (`UltrawideInSession != 0`), jumps to `0x844216` to skip the left/right black bar draws. |
| `0xE19DFB` | `4` | `BytePatchRequest` (vignette shader aspect ceiling) | Replaces `72 1C 07 40` (`2.1111112f`) inside `0xE19DF2` (`mov dword ptr [rdx+8], 0x40071C72`) with `00 00 00 42` (`32.0f`). |
| `0x1CBA908` | `4` | Read-only witness | `72 1C 07 40` (`2.1111112f` = 19:9 aspect constant). Preserved untouched in `.rdata`. |
| `0x1CF2D50` | `16` | Read-only witness | `00..00 80 3F` (`{0, 0, 0, 1}` black color constant). |
| `0x844185`, `0x8441B7`, `0x8441CA`, `0x8441DC`, `0x8441E1`, `0x844200` | — | Read-only witnesses | Full `0x844140` pillarbox draw block verified before staging. |

---

## 3. Subsystem 2: Horizontal Mouse, Ground Loot Placard, Automap & HUD Layout Unclamp (`enable_mouse_unclamp`, `enable_placard_unclamp`, `enable_automap_unclamp`, `enable_full_length_hud`, `enable_corner_minimap`, `enable_corner_mercenary`, `enable_corner_game_info`)

| RVA | Size | Role | Expected Bytes / Contract |
| --- | --- | --- | --- |
| `0x7F427` | `17` | `InlineHookRequest` -> `UltrawideMouseEntry` | `E8 E4 6D 7C 00 84 C0 74 5C 66 41 0F 6E D7 0F 5B D2` (`call 0x846210; test al, al; je 0x7F48C; movd xmm2, r15d; cvtdq2ps xmm2, xmm2`). When `UltrawideInSession != 0`, jumps to `0x7F48C` (`UltrawideMouseUnclampedExit`) so raw cursor X in `[0, windowWidth]` is preserved for world picking and targeting across the full ultrawide viewport. When `UltrawideInSession == 0` (menus), calls `0x846210`, replays `movd xmm2, r15d; cvtdq2ps xmm2, xmm2`, and resumes at `0x7F438`. |
| `0x7F438` | `12` | Read-only witness | `66 0F 6E CB F3 0F 59 15 C4 B4 C3 01` (`movd xmm1, ebx; mulss xmm2, [0x1CBA908]`). |
| `0x7F48C` | `20` | Read-only witness | `48 8B 5C 24 68 48 8B C7 48 83 C4 20 41 5F 41 5E 5F 5E 5D C3` (epilogue of `0x7F350`). |
| `0x1FA7CB` | `22` | `InlineHookRequest` -> `UltrawidePlacardEntry` | `E8 D0 BA 64 00 89 06 E8 C9 BA 64 00 48 8B D8 E8 01 BB 64 00 03 C3` inside `0x1FA7B0` (`GetPlacardHorizontalScreenBounds`, called from `0x15170E4` and `0x151A1F5`). Native code initializes `*outLeftBound ([rsi]) = GetUIRootOrigin().x` (`0x8462A0`) and `*outRightBound ([r14]) = GetUIRootOrigin().x + GetUIRootSize().w` (`0x8462E0`), which restricts ground item placards (`0x1FA9F0`, `0x151742F..0x1517633`, `0x15177DC..0x151784E`, `0x15190A0`, `0x151A2E6..0x151A2F9`) to the 19:9 inner box. When `UltrawideInSession != 0`, `UltrawidePlacardEntry` sets `[rsi] = 0`, calls `0x6545D0` (`GetFullWindowDimensions`) to put `fullWindowWidth` into `eax`, and resumes at `0x1FA7E1` (`mov [r14], eax`), preserving open left/right UI panel narrowing at `0x1FA822..0x1FA8C8`. |
| `0x1FA7E1` | `10` | Read-only witness | `48 8D 0D B8 81 AC 01 41 89 06` (`lea rcx, [0x1CBA9A0]; mov [r14], eax`). |
| `0x14E367C` | `18` | `InlineHookRequest` -> `UltrawideAutomapEntry` | `E8 1F 2C 36 FF 48 8B D8 48 89 44 24 28 E8 52 2C 36 FF` inside `0x14E3480` (`AutoMapPanel::Draw`). Native code calls `0x8462A0` (`GetUIRootOrigin`) and `0x8462E0` (`GetUIRootSize`) to build the full-screen Automap `clipRect` (`[rbp-0x59] = {left, top, cappedWidth, cappedHeight}`), which both sets the GPU scissor rectangle via `0x79D920` (`0xD26AA`) and culls automap tiles (`0xD1940` / `0xD6F10`) and icons (`0xD76E0`) to the 19:9 inner box. When `UltrawideInSession != 0`, `UltrawideAutomapEntry` zeroes `rbx` and `[rsp+0x28]` (`origin = {0, 0}`), calls `0x6545D0` (`GetFullWindowDimensions`) to load `{fullWindowWidth, fullWindowHeight}` into `rax`, and resumes at `0x14E368E` (`44 8B 45 83 48 8B C8 89 45 AF`). |
| `0x14E368E` | `10` | Read-only witness | `44 8B 45 83 48 8B C8 89 45 AF` (`mov r8d, [rbp-0x7D]; mov rcx, rax; mov [rbp-0x51], eax`). |
| `0x8A6635` | `15` | `InlineHookRequest` -> `UltrawideUIRootWidthEntry` | `B8 D0 11 00 00 8B D6 3B F0 48 8B CF 0F 4F D0` (`mov eax, 0x11D0; mov edx, esi; cmp esi, eax; mov rcx, rdi; cmovg edx, eax`) inside `0x8A64F0` (`UIRootWidget::UpdateLayout`). When `enable_full_length_hud = true`, `UltrawideInSession != 0`, and `byte [rdi + 0xD8] == 0` (`m_isInFrontEnd == 0`), bypasses the `0x11D0` (`4560` virtual width = 19:9 at 2160p) clamp and passes the full virtual width `edx = esi` to `0x857520` (`Widget::SetWidth`), setting `UIRootWidget.x` (`[rdi + 0x70]`) to `0` and broadcasting `"SystemMessage:ResolutionChanged"` across all in-game panels. In front-end menus (`[rdi + 0xD8] != 0` or `UltrawideInSession == 0`), preserves the `0x11D0` clamp. Session transitions invalidate `[uiRoot + 0xBC] = 0` (`m_cachedWindowWidth`) so `UIRootWidget::Update` (`0x8A5AA0`) immediately re-runs `0x8A64F0`. |
| `0x8A6644` | `13` | Read-only witness | `E8 D7 0E FB FF 48 8D 54 24 30 48 8B CF` (`call 0x857520; lea rdx, [rsp+0x30]; mov rcx, rdi`). |
| `0x8A6461` | `16` | `InlineHookRequest` -> `UltrawideCornerHudUpdateEntry` | `48 8B CB E8 57 8C FF FF 48 8B CB E8 0F 87 FF FF` (`mov rcx, rbx; call 0x89F0C0; mov rcx, rbx; call 0x89EB80`) inside `0x8A5AA0` (`UIRootWidget::Update`), immediately after `UIRootWidget` updates its child panels (`0x8A645C`). Replays `0x89F0C0(rbx)` and `0x89EB80(rbx)`, then calls `UltrawideUpdateCornerHudPanels(rbx)` and resumes at `0x8A6471`. When `UltrawideInSession != 0` and Centered HUD is active (`rootOffsetX = [uiRoot + 0x70] > 0`), computes `anchorDelta = rootOffsetX / (uiRoot.width * uiRoot.scale)` and updates `m_anchor.x` (`[panel + 0x48]`) on `AutoMapPanel` (`0x1FDC8A8`), `HireablesPanel` (`0x1CDDA38`), and `HUDWarningsPanel` (`0x1FCFBE8`) to `-anchorDelta` (top-left) or `1.0f + anchorDelta` (top-right). Because `Widget::GetScreenPos` (`0x8562F0`) multiplies `uiRoot.width * panel->m_anchor.x * uiRoot.scale` (`0x8563E1..0x8563F8`), this places the panels and all of their child widgets/hitboxes at the true ultrawide screen corners while leaving `0x8562F0` (`Widget::GetScreenPos`) 100% unmodified for cross-plugin coexistence (`gem-bag-ui`). |
| `0x8A6471` | `13` | Read-only witness | `45 84 ED 4C 8B AC 24 E8 05 00 00 74 50` (`test r13b, r13b; mov r13, [rsp+0x5E8]; je 0x8A64CE`). |
| `0x6545D0` | `14` | Read-only witness | `48 83 EC 38 E8 97 D1 2A 00 48 8D 54 24 20` (`GetFullWindowDimensions` prologue). |

---

## 4. Subsystem 3: 22x22 Terrain Cache Expansion & GroundSupply (`enable_terrain_expansion`)

- **Arena & 135 Patch Spans (`src/expansion_profile.h`, `src/expansion.h`)**:
  - Allocates a `0x60000`-byte private arena at positive module-relative offset (`>= 0x8000000`) from `D2R.exe` so `[module + idx*8 + disp32]` addressing at `0xE7B21B` and `0xE7B239` remains valid.
  - Stages all 135 terrain cache/ring/dimension patches and the `0xD967A8` shader-prefix selector (`E8 ... 66 90`) in a single atomic `MutationService` transaction before the terrain cache or renderer is occupied.
  - Registers unwind metadata (`RtlAddFunctionTable`) for every control point of the selector stub at `arena + 0x5E000`.
- **Cross-Mod & Vanilla Shader Path Resolution**:
  - Verifies all 14 paired 22x22 shader `.bin` files (`tc22sh`) by exact byte size and SHA256 hash and keeps read-only file handles open for the lifetime of the process.
  - Searches candidate directories in order:
    1. `<gameRoot>mods\<activeMod>\<activeMod>.mpq\data\hd\tc22sh\` (uses relative prefix `"data/hd/tc22sh/"`)
    2. `<gameRoot>mods\<activeMod>\data\hd\tc22sh\`
    3. `<gameRoot>mods\<activeMod>\d2rloader\data\hd\tc22sh\`
    4. `<loaderBase>data\hd\tc22sh\` and `<loaderBase>plugins\data\hd\tc22sh\`
    5. `<pluginDllDir>data\hd\tc22sh\`
    6. `<gameRoot>d2rloader\data\hd\tc22sh\` and `<gameRoot>data\hd\tc22sh\`
  - For non-MPQ fallback directories (including Vanilla), writes the UTF-8 absolute path with forward slashes and trailing `/` (under 240 bytes) into `arena + 0x5F000`. Native `0xA00C60` checks `0x122A980` (`IsAbsolutePath`) and routes absolute paths to `0x1152EE0 -> 0x115F800` (direct OS file open).
- **GroundSupply (`src/ground_supply.h`, `src/ground_supply_model.h`)**:
  - Installs a guarded inline hook at `0x6B4A30` (`19`-byte prologue, verified against 8 function body SHA256 hashes and caller return `0x6B3FED`) to extend the fallback-ground room view with up to 2-hop existing-room neighbors of already-represented levels that intersect the 22x22 cache window, under an 8ms QPC read budget and double-snapshot stability check.

---

## 5. Subsystem 4: 2-Hop / 3-Hop (`5x5` / `7x7`) Active Room Neighborhood / Monster Pop-In Fix (`enable_monster_pop_in_fix`, `monster_pop_in_radius`)

| RVA | Size | Role | Expected Bytes / Contract |
| --- | --- | --- | --- |
| `0x487254` | `15` | `InlineHookRequest` -> `UltrawideRoomDiffEntry` | `45 33 FF 4C 89 7D F0 45 8B C7 44 89 7D 38 4D 85 F6` inside `0x4871A0` (`ServerUpdateClientActiveRooms`). Calls `UltrawidePrepareClientRooms(pClient, pOldRoom, pNewRoom, rbp)` after native `0x2EF2F0(act, pOldRoom, pNewRoom)` (`0x48723D`) has already updated DRLG Stage 0..3 refcounts. Walks the DRLG `D2RoomEx` neighbor lists (`[pRoomEx + 0x10..0x18]`) up to `monster_pop_in_radius` hops (`5x5` for `2`, `7x7` default for `3`), invokes `0x3289A0(act, pNeighborRoomEx)` (`RoomExEnsureActive`) on any Stage 2/3 room whose `D2Room1` (`[pRoomEx + 0x58]`) has not yet been materialized by the 5-tick background throttle (`0x328A00`), and calls `0x4042A0(pGame, pRoom1)` (`ServerPopulateRoom1`) on any room whose population flags (`[pRoom1 + 0x54] & 3`) are not yet set so preset/inactive monsters exist in `pRoom1->pUnitFirst` before `ClientAddRoom` (`0x489DB0`) runs. Populates `[rbp-0x10]`/`[rbp+0x38]` with the active new room array and `[rbp-0x18]`/`[rbp+0x40]` with the previous active room array, and resumes the native room-diff loop at `0x4872A8`. |
| `0x48A149` | `18` | `InlineHookRequest` -> `UltrawideRemovedUnitsEntry` | `4C 8D 44 24 60 48 8B CB 48 8D 54 24 68 E8 85 5C E6 FF` inside `0x48A120` (`ServerReplicateRemovedUnitsInRooms`). Replaces the native 1-hop `0x2EFDE0(pPlayerRoom1, &rsp[0x68], &rsp[0x60])` call with `UltrawideLoadClientActiveRooms(pPlayerRoom1, pClient, &rsp[0x68], &rsp[0x60])` and resumes at `0x48A15B`, ensuring S2C `0x0A` (`RemoveUnit`) is sent for deleted units in `[pRoom1 + 0x28]` across all active 2-hop/3-hop rooms before `0x488310` clears `[pRoom1 + 0x28]`. |
| `0x48A2F4` | `21` | `InlineHookRequest` -> `UltrawideDirtyUnitsEntry` | `4C 8D 84 24 88 00 00 00 48 8B CE 48 8D 54 24 20 E8 D7 5A E6 FF` inside `0x48A270` (`ServerReplicateDirtyUnitsInRooms`). Replaces the native 1-hop `0x2EFDE0(pPlayerRoom1, &rsp[0x20], &rsp[0x88])` call with `UltrawideLoadClientActiveRooms(pPlayerRoom1, pClient, &rsp[0x20], &rsp[0x88])` and resumes at `0x48A309`, ensuring newly spawned or dirty units in `[pRoom1 + 0x30]` across all active 2-hop/3-hop rooms are replicated (`0x538690` -> S2C `0xAC`) and updated (`0x59D400`) before `0x488410` clears unit flag `0x10` (`0x48F860`) and wipes `[pRoom1 + 0x30]`. |
| `0x48A200` | `16` | `InstallInlineHook` -> `HookDisconnectCleanup` | `48 89 5C 24 10 57 48 83 EC 20 48 8B FA 48 8B D9 48 85 C9` (`ServerRemoveClientFromActiveRooms`). Removes the disconnecting client via `0x2F0390` ONLY from tracked active rooms that are NOT in `pRoom`'s 1-hop neighbor list (since `0x2F0390` unconditionally decrements `[pRoom1 + 0x70]` and native `0x48A200` removes the 1-hop neighbors immediately afterward). |
| `0x3289A0`, `0x4042A0`, `0x2EFDE0`, `0x2F0390`, `0x487265`, `0x4872A8` | — | Read-only witnesses | Verified before installing hooks. |

---

## 6. Subsystem 5: Monster AI Perception Range (`enable_monster_ai_range`, `monster_ai_range`, `scale_friendly_ai_range`)

All patches target `0x595750` (`AiFindTarget`) in a single atomic `MutationService` transaction:

| RVA | Size | Role | Expected Bytes / Replacement |
| --- | --- | --- | --- |
| `0x595923` | `6` | `BytePatchRequest` (default hostile perception range) | Expected: `41 BE 23 00 00 00` (`mov r14d, 35`). Replaced with `41 BE <imm32>` (`monster_ai_range`, clamped to `[35, 100]`, default `65`). |
| `0x5959AF` | `18` | `BytePatchRequest` (`max(monster_ai_range, monstats.aidist)`) | Expected: `84 D2 74 19 49 8B CF 89 54 24 48 44 8B F2 89 54 24 3C`. Replaced with `41 3B D6 76 18 89 54 24 48 44 8B F2 89 54 24 3C EB 0B` (`cmp edx, r14d; jbe 0x5959CC; mov [rsp+0x48], edx; mov r14d, edx; mov [rsp+0x3C], edx; jmp 0x5959CC`), so non-zero `monstats.txt` `aidist` values smaller than `monster_ai_range` scale up to `monster_ai_range` while larger boss `aidist` values are preserved. |
| `0x595B19` | `3` | `BytePatchRequest` (player-slot distance ceiling) | Expected: `83 FE 37` (`cmp esi, 55`). Replaced with `83 FE <cap>` where `<cap> = max(75, min(120, monster_ai_range + 20))` (`85` when `monster_ai_range = 65`). |
| `0x595D61` | `6` | Optional `BytePatchRequest` (`scale_friendly_ai_range = true`) | Expected: `41 BE 23 00 00 00` (`mov r14d, 35`). Replaced with `41 BE <imm32>` (`monster_ai_range`) only when `scale_friendly_ai_range = true`. |
