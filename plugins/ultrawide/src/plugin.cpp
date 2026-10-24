#include "bars_mouse.h"
#include "config.h"
#include "expansion.h"
#include "ground_supply.h"
#include "hash.h"
#include "monster_ai.h"
#include "monster_pop_in.h"
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/lifecycle_events.h>
#include <atomic>
#include <cstring>

extern "C" {
std::atomic<unsigned char> UltrawideInSession{};
void* UltrawideMenuContinue{};
void* UltrawideGameContinue{};
void* UltrawideMenuLabel{};
void* UltrawideMouseClampCheck{};
void* UltrawideMouseClampContinue{};
void* UltrawideMouseUnclampedExit{};
void* UltrawidePlacardUIRootOrigin{};
void* UltrawidePlacardUIRootSize{};
void* UltrawidePlacardFullWindowDims{};
void* UltrawidePlacardContinue{};
void* UltrawideAutomapContinue{};
void* UltrawideUIRootWidthContinue{};
void* UltrawideUIRootSub1{};
void* UltrawideUIRootSub2{};
void* UltrawideCornerHudUpdateContinue{};
void* UltrawideRoomDiffContinue{};
void* UltrawideRemovedUnitsContinue{};
void* UltrawideDirtyUnitsContinue{};
void UltrawideMenuEntry();
void UltrawideMouseEntry();
void UltrawidePlacardEntry();
void UltrawideAutomapEntry();
void UltrawideUIRootWidthEntry();
void UltrawideCornerHudUpdateEntry();
void UltrawideRoomDiffEntry();
void UltrawideRemovedUnitsEntry();
void UltrawideDirtyUnitsEntry();

static uintptr_t g_autoMapVtable{};
static uintptr_t g_hireablesVtable{};
static uintptr_t g_hudWarningsVtable{};
static bool g_cornerMinimap{};
static bool g_cornerMercenary{};
static bool g_cornerGameInfo{};

void __fastcall UltrawideUpdateCornerHudPanels(uintptr_t uiRoot) noexcept {
    Ultrawide::BarsAndMouse::UpdateCornerHudPanelsImpl(
        uiRoot,
        UltrawideInSession.load(std::memory_order_acquire) != 0,
        g_autoMapVtable,
        g_hireablesVtable,
        g_hudWarningsVtable,
        g_cornerMinimap,
        g_cornerMercenary,
        g_cornerGameInfo);
}
void __fastcall UltrawidePrepareClientRooms(uintptr_t pClient, uintptr_t pOldRoom, uintptr_t pNewRoom, uintptr_t rbp) noexcept {
    Ultrawide::MonsterPopIn::PrepareClientRoomsImpl(pClient, pOldRoom, pNewRoom, rbp);
}
void __fastcall UltrawideLoadClientActiveRooms(uintptr_t pPlayerRoom1, uintptr_t pClient, const uintptr_t** outRooms, uint32_t* outCount) noexcept {
    Ultrawide::MonsterPopIn::LoadClientActiveRoomsImpl(pPlayerRoom1, pClient, outRooms, outCount);
}
}

static_assert(sizeof(UltrawideInSession) == 1 && decltype(UltrawideInSession)::is_always_lock_free);

namespace {

const D2RL::PluginContext* g_context{};
bool g_fullLengthHud{};

void GameplayEventCallback(const D2RL::PluginContext*, const D2RL::Lifecycle::GameplayEvent* event, void*) noexcept {
    if (!event || event->structSize < D2RL::Lifecycle::GameplayEventRequiredSize) return;
    const bool joined = event->kind != D2RL::Lifecycle::GameplayEventKind::GameLeft;
    UltrawideInSession.store(joined ? 1 : 0, std::memory_order_release);
    TerrainProbe::GroundSupply::inGame.store(joined, std::memory_order_release);
    if (!joined) {
        Ultrawide::MonsterPopIn::ClearAllStates();
    }
    if (g_fullLengthHud && g_context && g_context->exeBase) {
        // Invalidate UIRootWidget's cached window width (+0xBC) so UIRootWidget::Update (0x8A5AA0)
        // runs UpdateLayout (0x8A64F0) and broadcasts SystemMessage:ResolutionChanged on the next frame.
        const uintptr_t uiRoot = *reinterpret_cast<const uintptr_t*>(g_context->exeBase + 0x3440170);
        if (uiRoot) {
            *reinterpret_cast<int32_t*>(uiRoot + 0xBC) = 0;
        }
    }
    if (g_context) {
        g_context->LogInfo(joined
            ? "Ultrawide: native session event entered game."
            : "Ultrawide: native session event left game.");
    }
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{
        .infoSize = D2RL::PluginInfoSize,
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .id = "ultrawide",
        .name = "Ultrawide",
        .version = "1.3.5+rev.2",
        .author = "PyreFly",
        .description = "Ultrawide gameplay support: pillarbox/vignette removal, mouse/placard/automap unclamp, centered/full-length HUD & corner HUD anchoring, 22x22 terrain expansion, 7x7 monster pop-in fix, and configurable AI range.",
        .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks
    };
    return &info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* ctx) noexcept -> bool {
    if (!ctx || !ctx->GetApi() || ctx->abiVersion != D2RL_PLUGIN_ABI_VERSION) return false;
    g_context = ctx;
    UltrawideInSession.store(0, std::memory_order_relaxed);
    TerrainProbe::GroundSupply::enabled.store(false, std::memory_order_relaxed);
    TerrainProbe::GroundSupply::inGame.store(false, std::memory_order_relaxed);
    Ultrawide::MonsterPopIn::enabled.store(false, std::memory_order_relaxed);
    Ultrawide::MonsterPopIn::ClearAllStates();

    if (!ctx->exeBase ||
        !Ultrawide::Hash(reinterpret_cast<HMODULE>(ctx->exeBase), "93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10") ||
        !Ultrawide::Hash(GetModuleHandleW(L"D2RCore.dll"), "2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2")) {
        ctx->LogWarn("Ultrawide: exact host/Core SHA256 guard refused; plugin inert.");
        return true;
    }

    const D2RL::MutationService* mutationService{};
    if (ctx->QueryService(&mutationService) != D2RL::ServiceQueryResult::Success || !mutationService) {
        ctx->LogWarn("Ultrawide: mutation service unavailable; plugin inert.");
        return true;
    }

    const D2RL::LifecycleService* lifecycleService{};
    if (ctx->QueryService(&lifecycleService) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasLifecycleServiceField(lifecycleService, D2RL::LifecycleServiceRequiredSize) ||
        !lifecycleService->registerGameplayEventListener) {
        ctx->LogWarn("Ultrawide: native lifecycle service missing; plugin inert.");
        return true;
    }

    UltrawideMenuContinue = reinterpret_cast<void*>(ctx->exeBase + 0x844174);
    UltrawideGameContinue = reinterpret_cast<void*>(ctx->exeBase + 0x844216);
    UltrawideMenuLabel = reinterpret_cast<void*>(ctx->exeBase + 0x1d70c58);
    UltrawideMouseClampCheck = reinterpret_cast<void*>(ctx->exeBase + 0x846210);
    UltrawideMouseClampContinue = reinterpret_cast<void*>(ctx->exeBase + 0x7f438);
    UltrawideMouseUnclampedExit = reinterpret_cast<void*>(ctx->exeBase + 0x7f48c);
    UltrawidePlacardUIRootOrigin = reinterpret_cast<void*>(ctx->exeBase + 0x8462a0);
    UltrawidePlacardUIRootSize = reinterpret_cast<void*>(ctx->exeBase + 0x8462e0);
    UltrawidePlacardFullWindowDims = reinterpret_cast<void*>(ctx->exeBase + 0x6545d0);
    UltrawidePlacardContinue = reinterpret_cast<void*>(ctx->exeBase + 0x1fa7e1);
    UltrawideAutomapContinue = reinterpret_cast<void*>(ctx->exeBase + 0x14e368e);
    UltrawideUIRootWidthContinue = reinterpret_cast<void*>(ctx->exeBase + 0x8a6644);
    UltrawideUIRootSub1 = reinterpret_cast<void*>(ctx->exeBase + 0x89f0c0);
    UltrawideUIRootSub2 = reinterpret_cast<void*>(ctx->exeBase + 0x89eb80);
    UltrawideCornerHudUpdateContinue = reinterpret_cast<void*>(ctx->exeBase + 0x8a6471);
    UltrawideRoomDiffContinue = reinterpret_cast<void*>(ctx->exeBase + 0x4872a8);
    UltrawideRemovedUnitsContinue = reinterpret_cast<void*>(ctx->exeBase + 0x48a15b);
    UltrawideDirtyUnitsContinue = reinterpret_cast<void*>(ctx->exeBase + 0x48a309);
    g_autoMapVtable = ctx->exeBase + 0x1fdc8a8;
    g_hireablesVtable = ctx->exeBase + 0x1cdda38;
    g_hudWarningsVtable = ctx->exeBase + 0x1fcfbe8;

    for (const auto kind : {
             D2RL::Lifecycle::GameplayEventKind::GameJoined,
             D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,
             D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .kind = kind,
            .callback = GameplayEventCallback
        };
        D2RL::Lifecycle::ListenerHandle handle{};
        if (lifecycleService->registerGameplayEventListener(ctx, &listener, &handle) != D2RL::Lifecycle::Result::Success || !handle) {
            ctx->LogWarn("Ultrawide: session listener registration failed; plugin refused.");
            return false;
        }
    }

    Ultrawide::Config::Settings settings{};
    bool configInvalid = false;
    char configBuffer[8192]{};
    uint32_t requiredBytes{};
    if (ctx->ReadConfig(configBuffer, sizeof(configBuffer), &requiredBytes) &&
        requiredBytes <= sizeof(configBuffer) &&
        strnlen_s(configBuffer, sizeof(configBuffer)) < sizeof(configBuffer)) {
        settings = Ultrawide::Config::Parse(configBuffer, configInvalid);
    }
    if (configInvalid) {
        ctx->LogWarn("Ultrawide: invalid or duplicate setting in ultrawide.toml; safe defaults applied for invalid entries.");
    }

    g_fullLengthHud = settings.enableFullLengthHud;
    g_cornerMinimap = settings.enableCornerMinimap;
    g_cornerMercenary = settings.enableCornerMercenary;
    g_cornerGameInfo = settings.enableCornerGameInfo;
    const bool enableCornerHud = settings.enableCornerMinimap || settings.enableCornerMercenary || settings.enableCornerGameInfo;

    // 1. Pillarboxes / Vignette, Mouse/Placard/Automap Unclamp, Full-Length HUD & Corner HUD Anchoring
    const auto barsMouseResult = Ultrawide::BarsAndMouse::Apply(
        ctx,
        mutationService,
        settings.enableBarsRemoval,
        reinterpret_cast<void*>(UltrawideMenuEntry),
        settings.enableMouseUnclamp,
        reinterpret_cast<void*>(UltrawideMouseEntry),
        settings.enablePlacardUnclamp,
        reinterpret_cast<void*>(UltrawidePlacardEntry),
        settings.enableAutomapUnclamp,
        reinterpret_cast<void*>(UltrawideAutomapEntry),
        settings.enableFullLengthHud,
        reinterpret_cast<void*>(UltrawideUIRootWidthEntry),
        enableCornerHud,
        reinterpret_cast<void*>(UltrawideCornerHudUpdateEntry));
    if (barsMouseResult == Ultrawide::BarsAndMouse::ApplyResult::RollbackFailed) {
        ctx->LogError("Ultrawide: bars/mouse/placard/automap/hud transaction rollback failed; loader cleanup required.");
        return false;
    }
    if (barsMouseResult == Ultrawide::BarsAndMouse::ApplyResult::Applied) {
        D2RL::LogInfoF(ctx,
            "Ultrawide: bars_removal=%u mouse_unclamp=%u placard_unclamp=%u automap_unclamp=%u full_length_hud=%u corner_minimap=%u corner_mercenary=%u corner_game_info=%u active.",
            settings.enableBarsRemoval ? 1U : 0U,
            settings.enableMouseUnclamp ? 1U : 0U,
            settings.enablePlacardUnclamp ? 1U : 0U,
            settings.enableAutomapUnclamp ? 1U : 0U,
            settings.enableFullLengthHud ? 1U : 0U,
            settings.enableCornerMinimap ? 1U : 0U,
            settings.enableCornerMercenary ? 1U : 0U,
            settings.enableCornerGameInfo ? 1U : 0U);
    } else if (barsMouseResult != Ultrawide::BarsAndMouse::ApplyResult::NothingToApply) {
        D2RL::LogWarnF(ctx, "Ultrawide: bars/mouse/placard/automap/hud mutation refused (%u); native behavior retained.",
            static_cast<unsigned>(barsMouseResult));
    }

    // 2. 22x22 Terrain Cache Expansion + GroundSupply
    if (settings.enableTerrainExpansion) {
        if (!TerrainProbe::Expansion::Try(ctx, true)) {
            ctx->LogError("Ultrawide: terrain expansion rollback uncertain; loader cleanup required.");
            return false;
        }
        if (TerrainProbe::Expansion::enabled) {
            TerrainProbe::GroundSupply::Install(ctx);
        }
    } else {
        ctx->LogInfo("Ultrawide: terrain expansion disabled by configuration.");
    }

    // 3. Monster Pop-In Fix (configurable 1-hop / 2-hop / 3-hop active room neighborhood)
    if (settings.enableMonsterPopInFix) {
        const auto popInResult = Ultrawide::MonsterPopIn::Apply(
            ctx,
            mutationService,
            true,
            reinterpret_cast<void*>(UltrawideRoomDiffEntry),
            reinterpret_cast<void*>(UltrawideRemovedUnitsEntry),
            reinterpret_cast<void*>(UltrawideDirtyUnitsEntry),
            settings.monsterPopInRadius);
        if (popInResult == Ultrawide::MonsterPopIn::ApplyResult::RollbackFailed) {
            ctx->LogError("Ultrawide: monster pop-in transaction rollback failed; loader cleanup required.");
            return false;
        }
        if (popInResult == Ultrawide::MonsterPopIn::ApplyResult::Applied) {
            const uint32_t side = settings.monsterPopInRadius * 2U + 1U;
            D2RL::LogInfoF(ctx, "Ultrawide: %u-hop (%ux%u) active room neighborhood admitted for monster pop-in fix.",
                settings.monsterPopInRadius, side, side);
        } else {
            D2RL::LogWarnF(ctx, "Ultrawide: monster pop-in fix refused (%u); native 3x3 room window retained.",
                static_cast<unsigned>(popInResult));
        }
    } else {
        ctx->LogInfo("Ultrawide: monster pop-in fix disabled by configuration.");
    }

    // 4. Monster AI Perception Range
    if (settings.enableMonsterAiRange) {
        const auto aiResult = Ultrawide::MonsterAi::Apply(ctx, mutationService, settings);
        if (aiResult == Ultrawide::MonsterAi::ApplyResult::RollbackFailed) {
            ctx->LogError("Ultrawide: monster AI range transaction rollback failed; loader cleanup required.");
            return false;
        }
        if (aiResult == Ultrawide::MonsterAi::ApplyResult::Applied) {
            D2RL::LogInfoF(ctx, "Ultrawide: monster AI perception range=%u player_slot_cap=%u scale_friendly=%u active.",
                settings.monsterAiRange,
                static_cast<unsigned>(Ultrawide::MonsterAi::ComputePlayerSlotCap(settings.monsterAiRange)),
                settings.scaleFriendlyAiRange ? 1U : 0U);
        } else {
            D2RL::LogWarnF(ctx, "Ultrawide: monster AI range mutation refused (%u); native AI distance retained.",
                static_cast<unsigned>(aiResult));
        }
    } else {
        ctx->LogInfo("Ultrawide: monster AI range scaling disabled by configuration.");
    }

    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    UltrawideInSession.store(0, std::memory_order_release);
    TerrainProbe::GroundSupply::enabled.store(false, std::memory_order_release);
    TerrainProbe::GroundSupply::inGame.store(false, std::memory_order_release);
    Ultrawide::MonsterPopIn::enabled.store(false, std::memory_order_release);
    Ultrawide::MonsterPopIn::ClearAllStates();
}
