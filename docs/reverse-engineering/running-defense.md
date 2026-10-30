# Running Defense & Block Native Contracts (`running-defense`)

**Date:** 2026-10-28  
**Consumer:** [Running Defense plugin](../../plugins/running-defense/README.md) (`Running Defense.dll`, id `running-defense`)  
**Target Build:**
- On-disk packed `D2R.exe` (`3.3.94995` / `3.3.93787`): SHA-256 `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10`
- Unpacked `.text` capture (`runtime-game-auto-deposit-20260927.exe`): SHA-256 `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D`
- `D2RCore.dll` (`1.3.1`): SHA-256 `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2`

---

## 1. Summary of Vanilla D2R Running Defense & Block Mechanics

In vanilla Diablo II: Resurrected, moving in `PLRMODE_WALK` (`2`) preserves 100% of Defense Rating and Shield Block chance, whereas moving in `PLRMODE_RUN` (`3`) applies three separate penalties:

1. **Melee Auto-Hit (`SUnitDmg_ResolveHitFlags`, RVA `0x44B720`):**
   - At `0x44B7D7..0x44B7E6`, when the defender is a player (`dwUnitType == 0`) and `UNITS_GetAnimMode(pDefender)` (`0x34AB60`) returns `3` (`PLRMODE_RUN`), `ebx` is set to `r13d` (`1`), bypassing `SUnitDmg_CalculateToHit` (`0x44BA60`) at `0x44B832` entirely.
2. **Character Sheet `"---"` Replacement (`CharacterStatsPanel::UpdateDefenseStat`, RVA `0x14E5EA0`):**
   - At `0x14E5F34..0x14E5F40`, when `UNITS_GetAnimMode(pPlayer)` returns `3` (`PLRMODE_RUN`), `UpdateDefenseStat` skips calling `UNITS_GetDefense` (`0x347B10`) and replaces the `"ArmorClass"` `TextBoxWidget` text with `"---"` (`@CharacterStatsNoDefense`, RVA `0x1FDCD00`).
3. **Shield Block 1/3 Penalty (`SUnitDmg_CheckBlock`, RVA `0x44B930`):**
   - At `0x44B982..0x44B9C8`, after calling `UNITS_GetBlockRate(pDefender, bExpansion)` (`0x347DA0`), if `pDefender` is a player (`dwUnitType == 0`), `UNITS_IsMoving(pDefender) != 0` (`0x34C390`), and `UNITS_GetAnimMode(pDefender) != 2` (`PLRMODE_WALK`), the block rate `esi` is divided by `3` (`0x44B9BB..0x44B9C8`, 14 bytes).

---

## 2. Discovered RVAs and Guard Contracts

| Module | RVA | Symbol / Role | Guard Bytes | Contract & Notes |
|---|---:|---|---|---|
| `D2R.exe` | `0x44B720` (`+0xB7` = `0x44B7D7`) | `SUnitDmg_ResolveHitFlags` running check | `48 8B CF E8 81 F3 EF FF 83 F8 03 75 03 41 8B DD` (16B at `0x44B7D7`) | `0x44B7E4` (`41 8B DD`, `mov ebx, r13d`) sets auto-hit when `dwAnimMode == 3`. Patching `0x44B7E4` to `90 90 90` routes running players through `SUnitDmg_CalculateToHit` (`0x44BA60`). |
| `D2R.exe` | `0x44BA60` | `SUnitDmg_CalculateToHit` | `4C 8B DC 53 55 56 41 56 41 57 48 81 EC 90 04 00 00` | `int __fastcall(D2UnitStrc* pAttacker, D2UnitStrc* pDefender, int nArBonus, int bMissile)`. Calls `UNITS_GetDefense(pDefender)` at `0x44BABF`. Also called by missile hit check at `0x463C5F`. |
| `D2R.exe` | `0x347B10` | `UNITS_GetDefense` | `40 53 48 83 EC 50 48 89 74 24 70 48 89 7C 24 48` (16B) | `int __fastcall(D2UnitStrc* pUnit)`. Computes total Defense Rating. Called by `SUnitDmg_CalculateToHit` (`0x44BABF`), `CharacterStatsPanel::UpdateDefenseStat` (`0x14E5FBC`), and `BuildDefenseTooltip` (`0x1514A3D`). |
| `D2R.exe` | `0x34AB60` | `UNITS_GetAnimMode` | `48 83 EC 28 48 85 C9 75 1A 88 4C 24 30 48 8D 4C 24 30` (18B) | `int __fastcall(D2UnitStrc* pUnit)`. Returns `pUnit->dwAnimMode` (`+0x0C`); for `UNIT_PLAYER` (`0`), `2 == PLRMODE_WALK` and `3 == PLRMODE_RUN`. |
| `D2R.exe` | `0x34C390` | `UNITS_IsMoving` | `40 53 48 83 EC 20 48 85 C9 74 4D 8B 11` (13B) | `int __fastcall(D2UnitStrc* pUnit)`. Returns non-zero when unit path/mode is moving. |
| `D2R.exe` | `0x34B9D0` | `UNITS_GetType` | `8B 01 C3` | Reads `dword ptr [rcx]` (`0 == UNIT_PLAYER`, `1 == UNIT_MONSTER`). |
| `D2R.exe` | `0x347DA0` | `UNITS_GetBlockRate` | `40 55 57 41 57 48 83 EC 30 33 ED 44 8B FA 48 8B F9` (17B) | `int __fastcall(D2UnitStrc* pUnit, int bExpansion)`. Returns capped shield block chance (`<= 75`). Called by `SUnitDmg_CheckBlock` (`0x44B982`) and `FormatDefenseAndBlockTooltip` (`0x1514CF7`). |
| `D2R.exe` | `0x44B930` (`+0x8B` = `0x44B9BB`) | `SUnitDmg_CheckBlock` running `/ 3` | `B8 56 55 55 55 F7 EE 8B F2 C1 EE 1F 03 F2` (14B at `0x44B9BB`) | Divides `esi` (`blockRate`) by 3 when a moving player is not walking (`dwAnimMode != 2`). |
| `D2R.exe` | `0x14E5EA0` | `CharacterStatsPanel::UpdateDefenseStat` | `40 53 56 57 48 83 EC 70 48 8B 05 19 54 4E 01` (15B) | `void __fastcall(void* pPanel, D2UnitStrc* pPlayer)`. Updates the `"ArmorClass"` `TextBoxWidget` on `CharacterStatsPanel` every UI update frame (called from `0x14E9FEF`). |
| `D2R.exe` | `0x14E5F34` (`0x14E5F3F`) | `UpdateDefenseStat` running branch | `48 8B CE E8 24 4C E6 FE 83 F8 03 75 78` (13B at `0x14E5F34`) | `0x14E5F3F` (`75 78`, `jne 0x14E5FB9`) falls through to format `"---"` when `dwAnimMode == 3`. Patching `0x14E5F3F` to `EB` (`jmp`) always formats the numerical Defense returned by `0x347B10`. |
| `D2R.exe` | `0x15149F0` / `0x1514CA0` / `0x14E8220` | `BuildDefenseTooltip` / `FormatDefenseAndBlockTooltip` | Calls `0x347B10` at `0x1514A3D` and `0x347DA0` at `0x1514CF7` | Automatically reflects the scaled running Defense and Block values from `UNITS_GetDefense` and `UNITS_GetBlockRate` in the Character Sheet tooltip. |
| `D2R.exe` | `0x856220` | `Widget::FindChild` | `48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20` (15B) | `void* __fastcall(void* pWidget, const char* name)`. Resolves `"ArmorClass"` child widget on `CharacterStatsPanel`. |
| `D2R.exe` | `0x908690` | `GetStatTextColor` | `40 53 57 48 83 EC 38 C6 44 24 58 00 41 8B D8` (15B) | `float* __fastcall(float outRgba[4], int colorIdx, int paletteIdx)`. `(1, 5)` returns the native red/penalty RGBA float tuple written to `TextBoxWidget+0xBC`. |

---

## 3. Coexistence & Failure Mode

- **Coexistence:** None of `0x347B10`, `0x347DA0`, `0x44B7D7`, `0x44B9BB`, or `0x14E5EA0` are hooked or patched by any other plugin in `plugins/`.
- **Fail-open behavior:** If `D2R.exe` or `D2RCore.dll` SHA-256 hashes or any expected byte guard fails, `Running Defense.dll` logs a warning and remains completely inert (`return true` from `D2RLoaderLoadPlugin`). All byte patches (`0x44B7E4`, `0x14E5F3F`, `0x44B9BB`) are restored on `D2RLoaderUnloadPlugin`.
