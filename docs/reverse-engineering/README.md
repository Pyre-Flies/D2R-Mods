# Reverse-engineering registry

This directory is the cross-project index for derived D2R and D2RCore native
contracts. Feature-specific evidence remains beside the plugin that consumes
it; this registry prevents useful paths and RVAs from being rediscovered.

See [Finding RVAs and interception points](RVA-AND-INTERCEPTION-WORKFLOW.md) for
the static-analysis, live-observation, byte-guard, validation, and publication
workflow used to produce these records.

Cross-project summaries:

- [Inventory Deposit native button and batch integration](../../plugins/inventory-deposit/docs/INVENTORY-DEPOSIT-BUTTON.md),
  [gold-relative geometry](../../plugins/inventory-deposit/docs/GOLD-ANCHOR.md),
  [embedded sprites](../../plugins/inventory-deposit/docs/BUTTON-SPRITES.md),
  [independent stash-tab feedback](../../plugins/inventory-deposit/docs/STASH-FEEDBACK.md),
  and [input versus disabled-artwork state](../../plugins/inventory-deposit/docs/BUTTON-STATE.md).

- [Autosort transaction research](autosort-transactions.md)
- [Floating Damage HP and projection](floating-damage-numbers-feasibility.md): authoritative HP layout, owned snapshots and admitted projection.
- [Floating Damage components and poison](floating-damage-elemental-probe.md): guarded combat ABI, six components and periodic state/endpoint matching.
- [Floating Damage Critical/Deadly Strike](floating-damage-critical-strike-probe.md): shared native marker and display association.
- [Floating Damage native commands](floating-damage-chat-command.md): parser registration, callback lifetime and local toggles.
- [Floating Damage crash triage](floating-damage-crash-triage-2026-10-11.md): recurring browser read faults, separate teardown failures and attribution limits.
- [Controller QOL native-contract registry](controller-qol-native-contracts.md)
- [Guided Arrow controller targeting candidate](guided-arrow-controller-targeting.md)
- [Integrated controller aim](../../plugins/controller-qol/docs/history/PRODUCTION-1.3.1-rev.50.md): opt-in cursor/snapping, native guards, render projection and archived prototype evidence; combined-runtime validation pending.

## Tested compatibility profiles

| Consumer | Module | Required SHA-256 / profile | Evidence |
|---|---|---|---|
| Production baseline (`3.3.94995` / `1.7.91735`) | D2R.exe (on-disk packed) | `93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10` | [Ultrawide](../../plugins/ultrawide/NATIVE-CONTRACT.md), [Day Night Cycle](../../plugins/day-night-cycle/NATIVE-CONTRACT.md), [Pyre Loot Filter](../../plugins/pyre-lootfilter/README.md) |
| Production baseline (`3.3.94995` / `1.7.91735`) | D2R.exe (unpacked `.text` snapshot) | `02C32A05F182C3F1F1B29D58B2B8DA735EADC080B5E13ACF8EA721A0648CE60D` | Static Ghidra/disassembly reference for decrypted `.text` RVAs |
| Production baseline (D2RLoader `1.3.1`) | D2RCore.dll | `2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2` | [autosort-transactions.md](autosort-transactions.md), [Item Roll Ranges](../../plugins/item-roll-ranges/DEVELOPMENT.md), [Day Night Cycle](../../plugins/day-night-cycle/NATIVE-CONTRACT.md) |
| Item Roll Ranges (legacy provider) | D2RCore.dll | `AE1EA9B7F97AF5B89A550281E6A6C6B6E9C74E73AC8759E6558B40E751428CD0` | [Item Roll Ranges docs](../../plugins/item-roll-ranges/docs/README.md) |
| Controller QOL Updates | D2R.exe and related modules | Multiple guarded feature profiles | [Controller QOL hook inventory](../../plugins/controller-qol/docs/HOOK-INVENTORY.md) and feature documents |

Hashes identify only the files that were actually tested. A matching product
version without a matching hash is not considered equivalent.

## Recording format

Add discoveries to a feature document or table with:

| Field | Required content |
|---|---|
| Date | Discovery or validation date |
| Consumer | Plugin/feature using the contract |
| Module | PE module containing the target |
| Build identity | SHA-256 and useful file/product version |
| Location | RVA, never a session-only absolute address by itself |
| Guard | Expected bytes, layout check, or exported ABI version |
| Contract | Inputs, outputs, side effects, thread/lifetime requirements |
| Evidence | Disassembly/audit path and reproducible command |
| Validation | Unit, integration, and live-game results listed separately |
| Failure mode | Required fail-open/disable behavior |

If a runtime absolute address is useful, record the module base and derived RVA
alongside it. Never assume ASLR-stable process addresses.

## Known high-value entries

| Module | RVA / layout | Purpose | Qualification |
|---|---:|---|---|
| D2RCore.dll | `0x422500` | Public ItemService existing-item transaction entry | Guard this public entry, not inner wrapper `0x427440`; autosort tested profile only |
| D2RCore.dll | `0x427440` | Inner existing-item transaction wrapper | Reference only; not the public admission guard |
| D2R.exe | `0x878D30` | TabBar controller navigation target | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x27DF80` | UI switcher path | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x14C6810` | Skill Tree controller path | Controller probe build only; exact-byte guarded |
| D2R.exe | `0xC66A0` / `0xC6E90` | Ground-label press/release paths | Controller probe build only; exact-byte guarded |
| D2R.exe | `0x1FAE90` | Label setter | Controller probe build only; exact-byte guarded |
| D2R.exe | `0xCE450` | Label refresh | Controller probe build only; exact-byte guarded |
| Installed provider ABI | operation `64` bytes, page `+56`; destination `40` bytes, page `+32` | Explicit Shared Stash transaction page | Additive ABI observed in tested provider; legacy non-shared layouts remain 56/32 bytes |
| D2R.exe | `0x15F660` | Stored-item belt placement action | Controller QOL tested profile; native asynchronous submission requires SDK observation |
| D2R.exe | `0x159B30` / `0x2CF680` | Advanced-stash withdrawal sender / widget wrapper | Reimagined Materials/Gems/Runes surfaces; UI-thread and layout-qualified |
| D2R.exe | `0x23FED0` | Native VendorPanel quick-sell wrapper | Caller-derived six-argument contract; invocation is not sale confirmation |
| D2R.exe | `0x23AD80` | Selected Shared-owner record resolver | Convert record to unit through ID getter/client lookup; category index is not storage page |
| D2R.exe | `0x14E3340` / `0x14E3480` | `AutomapPanel` constructor / `AutomapPanel::Render` | Stores `D2R+0x2A2ADA0 + 10` (`+0x90`); checks corner mini-map mode via `0xD3480` (`*(uint32_t*)(D2R+0x2A2ADD0) == 1`) or full automap state (`table[10] != 0`) before calling `0xD2600` (`AutomapRender`) |
| D2R.exe | `0xCBE90` / `0xD2600` / `0xD76E0` | Automap visibility helper / `AutomapRender` / `AutomapRenderUnit` | `0xCBE90` checks `table[9] != 0 \|\| table[31] != 0` at `D2R+0x2A2ADA0`; `0xD2600` iterates automap units and calls `0xD76E0` (`AutomapRenderUnit`, projecting via `0xD4910`) |
| D2RCore.dll / D2R.exe | `D2RCore+0x4411C0` / `0x4430B0` -> `D2R+0xDF1B0` | `D2RL::OverlayService` (`Phase::AfterGameUi`) host | Hooks `D2R+0xDF1B0` inside `D2R.exe`'s native ImGui UI pass (`0xCEAED0` `GetForegroundDrawList`, `0xD03A30` `PushClipRect`, `0xD03760` `PopClipRect`, `0xCFE590` `AddLine`, `0xCFF290` `AddRect`, `0xCFF760` `AddRectFilled`). Call `0xCEAED0` + `0xD03A30` (`full-screen clip`) + `0xD03760` around `OverlayService` draws to bypass `D2RCore`'s `float2` clip-rect ABI bug |
| D2RCore.dll | `0x706110` (`+0x706130`..`+0x706168`) | Steam `3.3.94995` `gameBuild` string table | D2RLoader 1.3.1 on Steam leaves `gameBuild` empty (no `.build.info`), causing `D2RCore+0x75270` to reject `compiler/3.3.94995/manifest.json` and `D2RCore+0x1CE540` to skip `sounds.txt` bin compilation; populate `"94995"` / `L"94995"` when `verChars == "3.3.94995"` and `buildLen == 0` |
| D2R.exe | `0x1FA9F0` / `0xCBEB0` / `0x1FA8E0` (`0x657B90`, `0x846210`, `0x65B030`) / `0x658510` | Ground label formatter / inner name writer / shared label paint (with native UI quad batcher `0x657B90` for `tooltip.borderColor`) / glyph B | Used by [Pyre Loot Filter](../../plugins/pyre-lootfilter/README.md) with caller guards at `0x15171E5`, `0x1517783`, `0x151A2B2`, `0x1FAA18`, `0x1517AF6`, `0x1519E46`, `0x90857B` |
| D2R.exe | `0x1516D50` (`0x1517149`, `0x15171E5`, `0x15176D9`, `0x1517783`, `0x1516A90`) & `0x151A1F5` (`0x151A23D`, `0x151A2AD`, `0x15190A0`) | Ground label 32-item array cap & 16-step screen-space collision solver (KB/M & controller) | `0x1516D50` and `0x151A1F5` cap ground labels at `[r14+8] == 32` (`0x144`/`0x148`-byte records) and increment `r12d` whenever `0x1FA9F0` (`LabelFormatter`) returns `1`, then run a 16-attempt collision solver (`0x1516A90` / `0x15190A0`). Returning `0` from `0x1FA9F0` (and resetting `record+0x08..0x10` to `0, 0, 0xFFFFFFFF`) prevents hidden items from consuming the 32 label slots or blocking visible labels |
| D2R.exe | `0x193010` -> `0x1932CC` (`0xFE3B0` action `7`) -> `0x101ADF` (`0xFABE0` action `22`) & `0xCBEB0` (`+0x78` at `0xCBF28`) | `AutoGoldManager::UpdateAutoGold` walk-over pickup & ground Gold `STAT_GOLD` (`14`) reader | Native walk-over Auto-Gold checks `STAT_GOLD` (`14`) at `0x1930BD` and dispatches `0xFABE0(22, player, 4, goldUnitId)` at `0x101ADF`; `0xCBEB0` reads `STAT_GOLD` (`14`) at `0xCBF28` for `ItemType == 4` (`gold`) and formats `"<amount> Gold"`. Any `0xFABE0` pickup filter ([Pyre Loot Filter](../../plugins/pyre-lootfilter/README.md)) must exempt `code == "gld"` so hidden gold piles still auto-pickup |
| D2R.exe | `0x1FAA1D` -> `0x846210` | Post-name placard query call site | Patched by Controller QOL (`QolPlacardCall::Site`) via a near-relay so both `0x1FAA18` (Pyre Loot Filter) and `0xCBEB0` (Chronicle) remain untouched |
| D2R.exe | `0x410005`, `0x4112F5`, `0x416126`, `0x41738D`, `0x4BA0EF`, `0x4BBAD0`, `0x55496C` -> `0x471950` | Seven direct callers of `Pickup` (`0x471950`) | Patched by Controller QOL (`QolPickupCalls::Calls`) via a near-relay so shared `0x471950` entry remains untouched for `Potion Auto Pickup` / `UnHoarder` |
| D2R.exe | `0x858990` (`+0x17` interior witness) | `LoadSprite` (`UI_BuildItemSocketOverlay` helper) | Shared by [Always Show Sockets](always-show-sockets.md) (caller `0x1493DFB` / `0x14941A5` patched) and `d2rl-item-quality-backgrounds.dll` (entry inline-hooked); qualify via interior bytes `+0x17` (`17` bytes) when entry bytes are detour-owned |
| D2R.exe | `0x44CE80` (`SUnitDmg_Apply`) / `0x462E40` (`MISSILE_SrvApplyDamage`) | Server damage application / missile hit dispatcher | [Floating Damage](floating-damage-elemental-probe.md) hooks `0x44CE80` for elemental/crit attribution + 30 ms HP polling; research `projectile-perf` hooks caller `0x462E40` |
| D2R.exe | `0x9EBEE0`, `0x9F7B00`, `0x13D850`, `0xCFF170`, `0x826E60` | Ultrawide pillarbox, mouse/placard/automap unclamp, HUD, terrain cache (`0xB34200`), monster pop-in (`0x317410`), AI (`0x366970`) | Full RVA and vtable ledger in [Ultrawide native contract](../../plugins/ultrawide/NATIVE-CONTRACT.md) |
| D2R.exe / D2RCore.dll | `D2R+0x833F9` (`0x2A23B1A`) / `D2RCore+0x1CC970` (`0x706BA0`) | Day/Night cycle gate and renderer lighting state (`D2R+0x1EDEB20`) | Full RVA and rollback ledger in [Day Night Cycle native contract](../../plugins/day-night-cycle/NATIVE-CONTRACT.md) |
| D2R.exe | `0x2F5020` (`.maho` `0x3E2A1B0`), `0x2F2CB0` (`.maho` `0x3E2A208`), `0x2F3440` (`.maho` `0x3E2A280`), `0x2F6E50` (`.maho` `0x3E2A278`) | `STATLIST_GetUnitStat`, `STATLIST_PostStatList`, `STATLIST_SuppressStateStats` (toggles `StatList+0x1C` bit `0x2000`), `STATLIST_Unlink` (`uint8_t dataContext` from `0x34A0E0` `UNITS_GetDataContext`, `StatList*`) | Resolved via `ResolveRipIndirectBridge` (`FF 25 rel32` + `±0x10000` `.maho` drift tolerance) in [Buff HUD `buffhud.toml`](../../outputs/companions/d2rl-buff-hud/docs/reverse-engineering/buffhud.toml) |
| D2R.exe | `0x3351B0` (`STATES_CheckState`), `0x436A16` (`call` `0x436A1C`), `0x436A80`/`0x436A98` (`SpendMana`, `call` `0x436A9E`), `0x2188B0`/`0x218942` (`ClientPredictMana`, `call` `0x21894A`), `0x437972` (`14x CC` relay cave) | Authoritative state check (`int32(unit, state)`) and the three `blood_mana` (`state 0x72 = 114`) mana-vs-life callsites (server skill cost, server `SpendMana`, client prediction) | Staged atomically via `MutationService` (`stageBytePatch` at `0x437972` + 3× `stageRel32Patch`) in [Buff HUD `buffhud.toml`](../../outputs/companions/d2rl-buff-hud/docs/reverse-engineering/buffhud.toml) |
| D2R.exe | `0x48FF00` (`UNITS_GetGame`), `0x406210` (`InitClientTable` -> `hGameArray` `0x2A9C020`), `0x4062F0` (`GetGameByHandle`), `0x48FE80` (`GetServerUnit`, `Unit+0x158`), `0x4925B0` (`UnitListBucket`, `Game+0x2230`), `0x34B240` (`GetPlayerData`, `Unit+0x10` 16-byte name) | Server game instance lookup (`Game+0x170` 25 Hz frame clock; returns `nullptr` on joined multiplayer clients) and server player unit table traversal (`1024` game handles × `128` player hash buckets) | Used by Buff HUD (`1.3.1+rev.3`) to stream server buff timers and apply `blood_mana` suppression across local games, TCP/IP hosts, and `d2rds` dedicated servers |
| D2R.exe | `0x347B10` (`UNITS_GetDefense`), `0x347DA0` (`UNITS_GetBlockRate`), `0x34AB60` (`UNITS_GetAnimMode`), `0x34C390` (`UNITS_IsMoving`), `0x44B720`/`0x44B7D7` (`SUnitDmg_ResolveHitFlags`), `0x44B930`/`0x44B9BB` (`SUnitDmg_CheckBlock`), `0x44BA60` (`SUnitDmg_CalculateToHit`), `0x14E5EA0`/`0x14E5F34` (`CharacterStatsPanel::UpdateDefenseStat`), `0x15149F0`/`0x1514CA0` (`BuildDefenseTooltip`) | Running Defense & Shield Block combat calculation + Character Sheet Defense display & tooltip | Used by [Running Defense](running-defense.md) to replace vanilla 100% running Defense loss (`PLRMODE_RUN == 3`) and `"---"` Character Sheet display with a configurable penalty rate and running indicator icon |

Feature documents contain the authoritative signatures and surrounding context.
This table is an index, not sufficient justification for an unguarded hook.

- [Floating Damage viewport scaling and native font-size bounds](floating-damage-resolution-scaling.md).

- [Always Show Sockets](always-show-sockets.md): exact icon/scale/ring contracts,
  source guards, appearance controls and stable-suite validation limits.
- [Stash Search 1.10.12 decompilation and native contracts](stash-search-1.10.12-decompilation.md):
  26 guarded `D2R.exe` RVAs, 7 `D2RCore.dll` pointer-table checks, `BankPanel::Draw`
  hook, `StashSearchAnchor` layout gating, and cross-tab shared-stash switching.
- [D2RLoader plugin hot-path performance audit (2026-10-26)](plugin-hotpath-performance-2026-10-26.md):
  40-plugin live profiler benchmarks (`-71.5%` total plugin CPU time, `0.5143 ms/f` -> `0.1467 ms/f`),
  six recurring hot-path anti-patterns and fixes across `Buff HUD`, `Pyre Loot Filter`,
  `Projectile Performance Optimizer`, and `Controller QOL Updates`, plus `d2rl-buff-panel.dll`
  and `BindAndSummon.dll` RVA ledgers.
- [Running Defense & Block native contracts](running-defense.md):
  `UNITS_GetDefense` (`0x347B10`), `SUnitDmg_ResolveHitFlags` running auto-hit gate (`0x44B7D7`),
  `SUnitDmg_CheckBlock` running `/ 3` block penalty (`0x44B9BB`), `UNITS_GetBlockRate` (`0x347DA0`),
  and `CharacterStatsPanel::UpdateDefenseStat` (`0x14E5EA0`) running indicator integration.
