#include "identify_bulk.h"
#include "owned_payloads.h"
#include "bulk_stash.h"
#include "bulk_stash_feedback.h"
#include "aim/controller_aim.h"
#include "aim/config_sections.h"
#include <D2RLPlugin/api.h>
#include "controller_input.h"
#include "physical_input.h"
#include "qol_navigation.h"
#include "qol_glyphs.h"
#include "item_dimensions.h"
#include "d2r_defs.h"
#include "d2r_memory.h"
#include "native_d2r.h"
#include "belt_actions.h"
#include "materials_actions.h"
#include "custom_page_actions.h"
#include "identify_action.h"
#include "native_identify.h"
#include "client_transfer.h"
#include "storage_focus.h"
#include "identify_stat.h"
#include "move_identity.h"
#include "cube_shortcut_policy.h"
#include "materials_policy.h"
#include "materials_signatures.h"
#include "shared_owner_compatibility.h"
#include "shared_item_policy.h"
#include "shared_sdk_transfer.h"
#include "shared_sdk_selection.h"
#include "vendor_policy.h"
#include "vendor_compatibility.h"
#include "belt_policy.h"
#include "portal_priority.h"
#include "portal_policy.h"
#include "policy.h"
#include "ground_action_hooks.h"
#include "guided_arrow_projection.h"
#include "placard_call.h"
#include "pickup_calls.h"
#include "client_loot.h"
#include "gold_pickup.h"
#include "placard_text.h"
#include "plugin_compatibility.h"
#include <xinput.h>
#include <intrin.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <array>
#include <climits>
#include <vector>
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace PlacardOverlay {void ForgetSession() noexcept;}
namespace {

struct PluginSettings {
    bool enabled = true;
    bool quickIdentify = true;
    bool identifyAll = true;
    bool nativeIdentify = false;
    bool quickMove = true;
    bool quickDeposit = true;
    bool groundPickup = true;
    uint32_t groundPickupDistance = 6;
    uint32_t portalPriorityDistance = 10;
    char groundPickupButton[16] = "lb";
    bool requireTomeOrScroll = true;
    bool consumeTomeOrScroll = true;
    bool requireModifier = true;
    char modifier[16] = "bumper";
    uint8_t triggerThreshold = 30;
    bool debugLogging = false;
    bool blockFilteredPickup = true;
    bool prioritizePortals = true;
    bool prioritizeStashBoxes = true;
    bool prioritizeWaypoints = true;
    bool prioritizeShrines = true;
    bool prioritizeChests = true;
    bool portalDiagnostics = false;
};

static PluginSettings g_Settings{};

static const D2RL::ItemInteractionService* g_Interactions = nullptr;
static const D2RL::ItemService*            g_Items        = nullptr;
static const D2RL::InventoryService*       g_Inventory    = nullptr;
static const D2RL::ThreadService*          g_Threads      = nullptr;
static const D2RL::InputService*           g_Input        = nullptr;
static const D2RL::SharedEventService*     g_SharedEvents = nullptr;
static const D2RL::WidgetService*          g_Widgets      = nullptr;
static const D2RL::PluginContext*          g_PluginContext = nullptr;

static D2RL::ItemInteractions::ListenerHandle g_ListenerHandle      = D2RL::ItemInteractions::InvalidHandle;
static D2RL::Input::ActionHandle              g_ActionHandle        = D2RL::Input::InvalidHandle;
static D2RL::SharedEvents::ListenerHandle     g_TooltipActionHandle = D2RL::SharedEvents::InvalidHandle;

static std::atomic<bool> g_PollingRunning{false};
static std::thread g_PollingThread;
static std::atomic<ULONGLONG> g_LastInventoryInteractionTick{0};

static std::atomic<D2RL::ItemHandle> s_FocusedItem{D2RL::InvalidItemHandle};
static std::atomic<D2RL::Items::ItemContainer> s_FocusedContainer{D2RL::Items::ItemContainer::Unknown};
static std::atomic<uint32_t> s_FocusedCode{0};
static std::atomic<ULONGLONG> s_FocusedTick{0};
static std::mutex s_FocusIdentityMutex;
static D2RL::Items::ItemInfo s_FocusIdentity{};
static ULONGLONG s_FocusIdentityTick{};
static std::atomic<ULONGLONG> s_AdvancedBeltFocusTick{};
static std::atomic<ULONGLONG> s_SharedBeltFocusTick{};
static bool ReadFocusIdentity(D2RL::Items::ItemInfo& info) noexcept {
    std::lock_guard lock(s_FocusIdentityMutex);
    if(!s_FocusIdentity.handle || GetTickCount64()-s_FocusIdentityTick>1000)return false;
    info=s_FocusIdentity;return true;
}
static std::atomic<D2RL::PlayerHandle> s_LastPlayerHandle{D2RL::InvalidPlayerHandle};

enum class PotionType {
    None,
    Health,
    Mana,
    Rejuv,
    Utility, // stamina, thawing, antidote
    Scroll   // tsc, isc
};

inline PotionType GetPotionType(uint32_t code) noexcept {
    char c[5]{};
    c[0] = static_cast<char>(code & 0xFF);
    c[1] = static_cast<char>((code >> 8) & 0xFF);
    c[2] = static_cast<char>((code >> 16) & 0xFF);
    c[3] = static_cast<char>((code >> 24) & 0xFF);

    if (c[0] == 'h' && c[1] == 'p') return PotionType::Health;
    if (c[0] == 'm' && c[1] == 'p') return PotionType::Mana;
    if (c[0] == 'r' && c[1] == 'v') return PotionType::Rejuv;
    if (std::strcmp(c, "vps ") == 0 || std::strcmp(c, "wms ") == 0 || std::strcmp(c, "yps ") == 0) return PotionType::Utility;
    if (std::strcmp(c, "tsc ") == 0 || std::strcmp(c, "isc ") == 0) return PotionType::Scroll;
    return PotionType::None;
}

inline bool IsPotionOrBeltItem(uint32_t code) noexcept {
    return GetPotionType(code) != PotionType::None;
}

inline bool IsPotionItem(uint32_t code) noexcept {
    const PotionType pt = GetPotionType(code);
    return pt == PotionType::Health || pt == PotionType::Mana || pt == PotionType::Rejuv || pt == PotionType::Utility;
}

static constexpr D2RL::PluginInfo ControllerQoLPluginInfo {
    .infoSize    = D2RL::PluginInfoSize,
    .abiVersion  = D2RL_PLUGIN_ABI_VERSION,
    .id          = "controller-qol-updates",
    .name        = "Controller QOL Updates",
    .version     = "1.3.1+rev.78",
    .author      = "PyreFly",
    .description = "Direct controller looting with filtered labels, inventory shortcuts, and stash navigation.",
    .flags       = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

static void LoadConfiguration(const D2RL::PluginContext* context) noexcept {
    if (!context) return;

    char buffer[65536]{};
    uint32_t reqSize = 0;
    if (!context->ReadConfig(buffer, sizeof(buffer) - 1, &reqSize)) {
        context->LogInfo("[ControllerQoL] Using default settings (no custom toml found).");
        return;
    }

    std::string toml;
    if(!QolAim::Section(buffer,"qol",toml)) {
        context->LogWarn("[ControllerQoL] Duplicate qol section; using QOL defaults.");
        return;
    }
    auto parseBool = [&toml](const char* key, bool& target) {
        auto pos = toml.find(key);
        if (pos != std::string::npos) {
            auto eq = toml.find('=', pos);
            if (eq != std::string::npos) {
                auto val = toml.find_first_not_of(" \t", eq + 1);
                if (val != std::string::npos) {
                    if (toml.compare(val, 4, "true") == 0) target = true;
                    else if (toml.compare(val, 5, "false") == 0) target = false;
                }
            }
        }
    };

    parseBool("enabled", g_Settings.enabled);
    parseBool("quick_identify", g_Settings.quickIdentify);
    parseBool("identify_all", g_Settings.identifyAll);
    parseBool("native_identify", g_Settings.nativeIdentify);
    parseBool("quick_move", g_Settings.quickMove);
    parseBool("quick_deposit", g_Settings.quickDeposit);
    parseBool("ground_pickup", g_Settings.groundPickup);
    parseBool("require_tome_or_scroll", g_Settings.requireTomeOrScroll);
    parseBool("consume_tome_or_scroll", g_Settings.consumeTomeOrScroll);
    parseBool("require_modifier", g_Settings.requireModifier);
    parseBool("debug_logging", g_Settings.debugLogging);
    parseBool("block_filtered_pickup", g_Settings.blockFilteredPickup);
    parseBool("prioritize_portals", g_Settings.prioritizePortals);
    parseBool("prioritize_stash_boxes", g_Settings.prioritizeStashBoxes);
    parseBool("prioritize_waypoints", g_Settings.prioritizeWaypoints);
    parseBool("prioritize_shrines", g_Settings.prioritizeShrines);
    parseBool("prioritize_chests", g_Settings.prioritizeChests);
    parseBool("portal_diagnostics", g_Settings.portalDiagnostics);

    // Parse modifier string
    size_t searchPos = 0;
    while ((searchPos = toml.find("modifier", searchPos)) != std::string::npos) {
        if (searchPos == 0 || (toml[searchPos - 1] != '_' && !std::isalnum(static_cast<unsigned char>(toml[searchPos - 1])))) {
            auto eq = toml.find('=', searchPos);
            if (eq != std::string::npos) {
                auto q1 = toml.find('"', eq);
                if (q1 != std::string::npos) {
                    auto q2 = toml.find('"', q1 + 1);
                    if (q2 != std::string::npos && (q2 - q1 - 1) < sizeof(g_Settings.modifier)) {
                        std::string m = toml.substr(q1 + 1, q2 - q1 - 1);
                        std::strncpy(g_Settings.modifier, m.c_str(), sizeof(g_Settings.modifier) - 1);
                        g_Settings.modifier[sizeof(g_Settings.modifier) - 1] = '\0';
                        break;
                    }
                }
            }
        }
        searchPos += 8;
    }

    // Parse ground_pickup_button string
    size_t gpbPos = toml.find("ground_pickup_button");
    if (gpbPos != std::string::npos) {
        auto eq = toml.find('=', gpbPos);
        if (eq != std::string::npos) {
            auto q1 = toml.find('"', eq);
            if (q1 != std::string::npos) {
                auto q2 = toml.find('"', q1 + 1);
                if (q2 != std::string::npos && (q2 - q1 - 1) < sizeof(g_Settings.groundPickupButton)) {
                    std::string btn = toml.substr(q1 + 1, q2 - q1 - 1);
                    std::strncpy(g_Settings.groundPickupButton, btn.c_str(), sizeof(g_Settings.groundPickupButton) - 1);
                    g_Settings.groundPickupButton[sizeof(g_Settings.groundPickupButton) - 1] = '\0';
                }
            }
        }
    }

    g_Settings.portalPriorityDistance = QolPortal::ReadPriorityDistance(toml);

    // Parse ground_pickup_distance
    size_t gpdPos = toml.find("ground_pickup_distance");
    if (gpdPos != std::string::npos) {
        auto eq = toml.find('=', gpdPos);
        if (eq != std::string::npos) {
            auto numStart = toml.find_first_of("0123456789", eq + 1);
            if (numStart != std::string::npos) {
                g_Settings.groundPickupDistance = static_cast<uint32_t>(std::atoi(toml.c_str() + numStart));
                if (g_Settings.groundPickupDistance < 1) g_Settings.groundPickupDistance = 1;
                if (g_Settings.groundPickupDistance > 20) g_Settings.groundPickupDistance = 20;
            }
        }
    }

    char msg[256];
    std::snprintf(msg, sizeof(msg),
        "[ControllerQoL] Config loaded: quickIdentify=%d, groundPickup=%d, blockFiltered=%d, modifier=%s, dist=%u",
        g_Settings.quickIdentify, g_Settings.groundPickup, g_Settings.blockFilteredPickup, g_Settings.modifier, g_Settings.groundPickupDistance);
    context->LogInfo(msg);

    ControllerQoL::SetActiveModifier(g_Settings.modifier, g_Settings.triggerThreshold);
}

struct IdentifyRequest {
    QolIdentify::Request action{};
    ULONGLONG queuedAt{};
};
static std::atomic<bool> s_IdentifyPending{false};
static std::recursive_mutex s_IdentifyMutex;
static IdentifyRequest s_IdentifyRequest{};
static uint64_t s_IdentifyGeneration{};
static bool s_IdentifySessionEvents{};
static std::atomic<uint64_t> s_ActionGeneration{1};
static std::atomic<uint64_t> s_ClientLootRefreshToken{0};
static void CancelOwnedRequests() noexcept;
static void __cdecl IdentifySessionChanged(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent* event,void*) noexcept {
    if(event && !D2RL::Lifecycle::HasGameplayEventField(event,D2RL::Lifecycle::GameplayEventRequiredSize))return;
    {std::lock_guard lock(s_IdentifyMutex);++s_IdentifyGeneration;s_IdentifyPending=false;s_IdentifyRequest={};}
    ++s_ActionGeneration;
    s_ClientLootRefreshToken=0;
    CancelOwnedRequests();
    QolClientLoot::OnLifecycle(event);
    PlacardOverlay::ForgetSession(); // Never restore a buffer belonging to the old game.
    {std::lock_guard lock(s_FocusIdentityMutex);s_FocusIdentity={};s_FocusIdentityTick=0;}
    s_FocusedItem=0;s_FocusedCode=0;s_FocusedTick=0;s_LastPlayerHandle=0;
}
static void __cdecl GroundPlayerReady(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent* event,void*) noexcept {
    QolClientLoot::OnLifecycle(event);
}
static void InitializeIdentifyQueue(const D2RL::PluginContext* ctx) noexcept {
    IdentifySessionChanged(nullptr,nullptr,nullptr);s_IdentifySessionEvents=false;
    const D2RL::LifecycleService* lifecycle{};
    if(ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) || !lifecycle->registerGameplayEventListener)return;
    s_IdentifySessionEvents=true;
    for(auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
        const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize,0,kind,0,IdentifySessionChanged,nullptr};
        D2RL::Lifecycle::ListenerHandle handle{};
        if(lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success)s_IdentifySessionEvents=false;
    }
    const D2RL::Lifecycle::GameplayEventListener readyListener{D2RL::Lifecycle::GameplayEventListenerSize,0,
        D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,0,GroundPlayerReady,nullptr};
    D2RL::Lifecycle::ListenerHandle readyHandle{};
    const bool readyRegistered=lifecycle->registerGameplayEventListener(ctx,&readyListener,&readyHandle)==D2RL::Lifecycle::Result::Success;
    QolClientLoot::SetLifecycleAvailable(s_IdentifySessionEvents && readyRegistered);
}
static void __cdecl ExecuteIdentifyTask(const D2RL::PluginContext* context,void* userData) noexcept {
    std::lock_guard lock(s_IdentifyMutex);
    if(!s_IdentifyPending || reinterpret_cast<uintptr_t>(userData)!=s_IdentifyGeneration)return;
    const auto* req=&s_IdentifyRequest;
    const auto started=GetTickCount64();
    if(context && g_Settings.enabled && started-req->queuedAt<=2000) {
        const auto result=QolIdentify::Execute(context,g_Items,g_Inventory,req->action,QolIdentifyStat::Read,g_Settings.nativeIdentify?QolNativeIdentify::Request:nullptr);
        const auto elapsed=GetTickCount64()-started;
        // One bounded summary, never a synchronous per-item log dump. Failures
        // remain diagnosable with debug_logging=false; timings precede log I/O.
        if(result.status!=QolIdentify::Status::Success || g_Settings.debugLogging || result.nativeReads || elapsed>=50) {
            char message[512];
            std::snprintf(message,sizeof(message),
                "[QOL/Identify] result=%s scan=%u operation=%u scanned=%u tomes=%u scrolls=%u maxTomeQty=%d consumed=%u sdkQty=%d stat70=%d remaining=%d readFailures=%u mismatches=%u queueMs=%llu workMs=%llu runtimeId=%u route=%s targetContainer=%u sourceContainer=%u.",
                QolIdentify::Name(result.status),static_cast<unsigned>(result.scan),static_cast<unsigned>(result.operation),
                result.scanned,result.tomes,result.scrolls,result.maxTomeQuantity,result.consumed?1u:0u,result.sdkTomeQuantity,result.nativeTomeQuantity,result.remainingQuantity,result.nativeReadFailures,result.quantityMismatches,
                static_cast<unsigned long long>(started-req->queuedAt),static_cast<unsigned long long>(elapsed),req->action.target.runtimeId,result.route,static_cast<unsigned>(req->action.target.container),static_cast<unsigned>(result.sourceContainer));
            if(result.status==QolIdentify::Status::Success || result.status==QolIdentify::Status::NativePending)context->LogInfo(message);else context->LogWarn(message);
        }
    }
    s_IdentifyPending.store(false);
}

enum D2RUiMode : int {
    UI_MODE_INVENTORY = 0x01,
    UI_MODE_CHARACTER = 0x02,
    UI_MODE_VENDOR    = 0x0B,
    UI_MODE_STASH     = 0x18,
    UI_MODE_CUBE      = 0x19,
};

using TestUiModeFn = bool(__fastcall*)(int mode);

static TestUiModeFn GetTestUiModeFn() noexcept {
    if(!g_PluginContext || !g_PluginContext->exeBase)return nullptr;
    // Reuse the complete getter witness already admitted by materials actions.
    // The old unguarded D2RCore+0x680330 fallback is not a stable function slot.
    __try {
        const auto address=g_PluginContext->exeBase+0xce500;
        if(std::memcmp(reinterpret_cast<const void*>(address),QolMaterials::Signatures::UiMode,
                       sizeof(QolMaterials::Signatures::UiMode)))return nullptr;
        return reinterpret_cast<TestUiModeFn>(address);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}

static bool TestUiMode(int mode) noexcept {
    auto fn = GetTestUiModeFn();
    if (!fn) return false;
    __try {
        return fn(mode);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool IsVendorPanelOpen() noexcept {
    // Native quick-sell caller 0x2AA588 uses shop mode 0x0B.
    return TestUiMode(UI_MODE_VENDOR);
}

static void TriggerNativeSellOnFocusedItem(const D2RL::PluginContext* ctx,
    const D2RL::Items::ItemInfo& info) noexcept {
    if (!ctx || !ctx->exeBase || !QolVendor::Context(IsVendorPanelOpen(),TestUiMode(UI_MODE_STASH),
        TestUiMode(UI_MODE_CUBE),info.container)) return;
    static ULONGLONG lastSell{}; // UI-thread only; native immediate action also throttles at 500ms.
    const auto now=GetTickCount64();
    if (lastSell && now-lastSell<500) return;
    for (const auto& site:QolVendor::Sites) if (!(site.rva==0x10D160 ? QolVendor::ValidateTransaction(ctx->exeBase) : ctx->CheckExpectedBytes(site.rva,site.bytes,site.size))) {
        char msg[144]; std::snprintf(msg,sizeof(msg),"[QOL/Sell] Native profile mismatch at RVA 0x%X; refused.",site.rva);
        ctx->LogWarn(msg); return;
    }
    __try {
        const auto base=ctx->exeBase;
        void* panel=reinterpret_cast<void*(__fastcall*)(int)>(base+0x846190)(0x0B);
        void* player=D2R::Native::GetLocalPlayerUnit(base);
        void* item=reinterpret_cast<void*(__fastcall*)(uint32_t,uint32_t)>(base+0x09A5D0)(info.runtimeId,4);
        if (!panel || !player || !item ||
            reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x36EF50)(item)!=info.code ||
            reinterpret_cast<uint8_t(__fastcall*)(void*)>(base+0x36CFE0)(item)!=0 ||
            !reinterpret_cast<int(__fastcall*)(void*)>(base+0x374370)(item) ||
            reinterpret_cast<int(__fastcall*)(void*)>(base+0x34AB60)(item)==2) {
            ctx->LogWarn("[QOL/Sell] Shop/player/item unavailable or native eligibility refused; no hold/drop fallback."); return;
        }
        lastSell=now;
        QolVendor::Submit(reinterpret_cast<QolVendor::SellFn>(base+0x23FED0),panel,player,item);
        char msg[192]; std::snprintf(msg,sizeof(msg),"[QOL/Sell] Native quick-sell invoked runtimeId=%u code=0x%08X; game owns pricing, packet and acceptance.",info.runtimeId,info.code);
        ctx->LogInfo(msg);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ctx->LogWarn("[QOL/Sell] Native action fault; no retry or fallback.");
    }
}

struct MoveRequest {
    D2RL::PlayerHandle          player;
    D2RL::ItemHandle            targetItem;
    uint32_t                    targetCode;
    int32_t                     cellX;
    int32_t                     cellY;
    D2RL::Items::ItemContainer  sourceContainer;
    D2RL::Items::ItemContainer  primaryDestination;
    D2RL::Items::ItemContainer  secondaryDestination;
    QolSharedSdk::Selection sharedSelection{};
    bool sdkSharedRequest{};
    ULONGLONG sharedRequestedAt{};
    bool materialsFirst{};
    D2RL::Items::ItemInfo sharedIdentity{};
    D2RL::Items::ItemInfo identity{};
    bool cubeShortcut{};
    uint64_t actionGeneration{}, actionRequestedAt{};
};

static QolTasks::Payloads<MoveRequest> s_MoveRequests;
struct CubePending { D2RL::Items::ItemInfo info; uint64_t generation,requestedAt; };
static QolTasks::Payloads<CubePending> s_CubeRequests;

// Capture the chosen normal page and player before scheduling across threads.
static bool CaptureSharedRequest(const D2RL::PluginContext* ctx,MoveRequest& req) noexcept {
    if (!QolSharedSdk::Supported(g_Items)) return true; // Older loader retains its reviewed native path.
    req.sharedSelection=QolSharedSdk::Selected(ctx->exeBase);
    if (!req.sharedSelection.valid) {ctx->LogWarn("[QOL/SharedSDK] Selected Shared page unavailable; no move queued.");return false;}
    if (req.sharedSelection.previousSeason) return true; // SDK does not support remove-only pages.
    if (!g_Inventory || g_Inventory->getLocalPlayer(ctx,&req.player)!=D2RL::Inventory::Result::Success || !req.player) return false;
    req.sdkSharedRequest=true;req.sharedRequestedAt=GetTickCount64();
    return true;
}
static void ReportSharedMove(const D2RL::PluginContext* ctx,const QolSharedSdk::Outcome& result,uint32_t page,bool deposit) noexcept {
    char msg[224];std::snprintf(msg,sizeof(msg),"[QOL/SharedSDK] %s page=%u result=%u committed=%d failureIndex=%u; no native retry.",
        deposit?"Deposit":"Withdraw",page,static_cast<unsigned>(result.result),result.committed?1:0,result.failureIndex);
    if(result.committed)ctx->LogInfo(msg);else ctx->LogWarn(msg);
}

static void ExecuteMove(const D2RL::PluginContext* context, MoveRequest* req) noexcept {
    if (!req) return;

    if (!context || !g_Items || !g_Settings.enabled) return;
    if(req->cubeShortcut && (!QolCubeShortcut::Current(req->actionGeneration,s_ActionGeneration.load(),
        req->actionRequestedAt,GetTickCount64()) || !QolCubeShortcut::Context(req->sourceContainer,
        TestUiMode(UI_MODE_INVENTORY),TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE))))return;

    D2RL::PlayerHandle player = D2RL::InvalidPlayerHandle;
    const bool localResolved=g_Inventory && g_Inventory->getLocalPlayer(context,&player)==D2RL::Inventory::Result::Success && player!=D2RL::InvalidPlayerHandle;
    if (localResolved) {
        s_LastPlayerHandle.store(player);
    } else if (req->player != D2RL::InvalidPlayerHandle) {
        player = req->player;
    } else {
        player = s_LastPlayerHandle.load();
    }

    if(req->cubeShortcut && (!localResolved || player!=req->player))return;
    if (req->sdkSharedRequest) {
        D2RL::PlayerHandle livePlayer{};
        if (!g_Inventory || g_Inventory->getLocalPlayer(context,&livePlayer)!=D2RL::Inventory::Result::Success ||
            !livePlayer || livePlayer!=req->player || GetTickCount64()-req->sharedRequestedAt>3000 ||
            !QolSharedSdk::SameSelection(req->sharedSelection,QolSharedSdk::Selected(context->exeBase))) {
            context->LogWarn("[QOL/SharedSDK] Player/page changed or request expired; cancelled.");return;
        }
        player=livePlayer;
    }
    char startMsg[256];
    std::snprintf(startMsg, sizeof(startMsg),
        "[ControllerQoL] ExecuteMoveTask: player=%llu, initialTarget=%llu, cell=(%d,%d), code=0x%08X, src=%u, dest1=%u, dest2=%u",
        static_cast<unsigned long long>(player),
        static_cast<unsigned long long>(req->targetItem),
        req->cellX, req->cellY, req->targetCode,
        static_cast<uint32_t>(req->sourceContainer),
        static_cast<uint32_t>(req->primaryDestination),
        static_cast<uint32_t>(req->secondaryDestination));
    context->LogInfo(startMsg);

    D2RL::ItemHandle activeTarget = req->targetItem;
    D2RL::Items::ItemInfo checkInfo{ .structSize = D2RL::Items::ItemInfoSize };
    if (req->sourceContainer==D2RL::Items::ItemContainer::SharedStash) {
        // UI handles can expire across scheduling; only the same SDK identity
        // on the same Shared page is eligible, never a matching cell/code.
        if (req->sharedIdentity.structSize!=D2RL::Items::ItemInfoSize || !TestUiMode(UI_MODE_STASH)) {
            context->LogWarn("[QOL/Shared] Missing UI identity or closed stash; withdrawal refused.");
             return;
        }
        if (g_Items->getItemInfo(context,activeTarget,&checkInfo)!=D2RL::Items::Result::Success ||
            !QolShared::SameItem(req->sharedIdentity,checkInfo)) {
            struct Find { const D2RL::Items::ItemInfo* expected; D2RL::ItemHandle found{D2RL::InvalidItemHandle}; unsigned scanned{}; } find{&req->sharedIdentity};
            const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,
                D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::SharedStash),0};
            if (g_Inventory) g_Inventory->forEachInventoryItem(context,player,&filter,
                [](const D2RL::PluginContext*,const D2RL::Items::ItemInfo* item,void* user) noexcept {
                    auto& f=*static_cast<Find*>(user); ++f.scanned;
                    if (!QolShared::SameItem(*f.expected,*item)) return D2RL::Inventory::IterationAction::Continue;
                    f.found=item->handle; return D2RL::Inventory::IterationAction::Stop;
                },&find);
            activeTarget=find.found;
            char msg[192]; std::snprintf(msg,sizeof(msg),"[QOL/Shared] Resolve runtimeId=%u SDK-page=%u code=0x%08X scanned=%u resolved=%llu; no cell/code fallback.",
                req->sharedIdentity.runtimeId,req->sharedIdentity.sharedStashPage,req->sharedIdentity.code,find.scanned,
                static_cast<unsigned long long>(activeTarget)); context->LogInfo(msg);
        }
        if (activeTarget==D2RL::InvalidItemHandle) {  return; }
    } else {
        // Never resolve a stale handle by a cell/code in another container.
        if(req->identity.structSize!=D2RL::Items::ItemInfoSize ||
            req->identity.container!=req->sourceContainer) {return;}
        activeTarget=QolMove::Resolve(context,g_Items,g_Inventory,player,req->identity);
    }

    if (activeTarget == D2RL::InvalidItemHandle) {
        context->LogWarn("[ControllerQoL] Could not resolve live handle for move target item!");

        return;
    }

    // Enforce this gameplay rule even when the SDK/native route accepts a move.
    // Check freshly resolved state, not the request's cached targetCode, before
    // any ordinary Cube destination can reach SDK/native mutation.
    D2RL::Items::ItemInfo liveTarget{.structSize=D2RL::Items::ItemInfoSize};
    if(g_Items->getItemInfo(context,activeTarget,&liveTarget)!=D2RL::Items::Result::Success ||
        liveTarget.container!=req->sourceContainer)return;
    if(!QolCubeShortcut::Allows(liveTarget.code,req->primaryDestination) ||
        !QolCubeShortcut::Allows(liveTarget.code,req->secondaryDestination)) {
        context->LogWarn("[QOL/Cube] Refused moving the Horadric Cube into Cube storage; no move submitted.");
        return;
    }

    const uintptr_t exeBase = context->exeBase ? context->exeBase : reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    void* playerUnit = D2R::Native::GetLocalPlayerUnit(exeBase);

    // Materials LB+X shares the LB+Y queue and executor, with the existing
    // advanced-storage predicate/deposit tried first inside scoped native access.
    if (req->materialsFirst && (req->sourceContainer==D2RL::Items::ItemContainer::Inventory || req->sourceContainer==D2RL::Items::ItemContainer::Cube)) {
        if (!TestUiMode(UI_MODE_STASH)) {  return; }
        const bool fromCube=req->sourceContainer==D2RL::Items::ItemContainer::Cube;
        if(fromCube && !QolClientTransfer::AdvancedTab(D2R::Native::GetActiveStashTabIndex(exeBase))) {return;}
        struct Deposit { uintptr_t base; void* player; uint8_t page; bool inspected{},eligible{},submitted{}; } deposit{exeBase,playerUnit,static_cast<uint8_t>(fromCube?3:0)};
        g_Items->editNativeItem(context,activeTarget,[](const D2RL::PluginContext*,void* item,void* user) noexcept {
            auto& d=*static_cast<Deposit*>(user);
            d.inspected=item!=nullptr;
            d.eligible=D2R::Native::CanDepositToAdvancedStash(d.base,item);
            if (d.eligible && d.player) d.submitted=D2R::Native::DepositToAdvancedStash(d.base,item,d.player,d.page);
        },&deposit);
        if (!deposit.inspected || deposit.eligible || fromCube) {
            context->LogInfo(deposit.submitted ? "[QOL/Materials] Existing smart-deposit helper submitted storage move." :
                "[QOL/Materials] Storage inspection/deposit refused; no Cube or Personal Stash fallback.");
            if (deposit.submitted) { s_FocusedItem.store(D2RL::InvalidItemHandle); s_FocusedCode.store(0); }
             return;
        }
        context->LogInfo("[QOL/Materials] Not advanced-storage eligible; continuing existing LB+Y Cube move.");
    }

    // 2. Inventory -> Stash: Smart deposit to Advanced Stash or active Stash tab
    const bool isStashOpen = TestUiMode(UI_MODE_STASH);
    if (QolMaterials::UseStashDeposit(req->sourceContainer,req->primaryDestination,isStashOpen)) {
        struct Deposit { uintptr_t base; void* player; bool eligible{},submitted{}; } deposit{exeBase,playerUnit};
        g_Items->editNativeItem(context, activeTarget, [](const D2RL::PluginContext*, void* item, void* userData) noexcept {
            auto& d=*static_cast<Deposit*>(userData);
            d.eligible=item && d.player && D2R::Native::CanDepositToAdvancedStash(d.base,item);
            if(d.eligible)d.submitted=D2R::Native::DepositToAdvancedStash(d.base,item,d.player);
        }, &deposit);

        // A. Smart deposit check for Advanced Stash (runes, gems, stackables, essences)
        if (deposit.eligible) {
            if (deposit.submitted) {
                context->LogInfo("[ControllerQoL] Quick Move: Successfully deposited to Advanced Stash (smart deposit)!");
                s_FocusedItem.store(D2RL::InvalidItemHandle);
                s_FocusedCode.store(0);

                return;
            } else {
                context->LogWarn("[ControllerQoL] Advanced Stash deposit failed; routing to active stash tab...");
            }
        }

        // B. Deposit into the currently active Stash tab (Personal or selected Shared page)
        const uint32_t activeTabIndex = D2R::Native::GetActiveStashTabIndex(exeBase);
        char tabMsg[128];
        std::snprintf(tabMsg, sizeof(tabMsg), "[ControllerQoL] Quick Move: Active stash tab is %u (%s)",
            activeTabIndex, activeTabIndex == 0 ? "Personal" : activeTabIndex == 1 ? "Shared" : "Advanced/unknown");
        context->LogInfo(tabMsg);

        if (activeTabIndex==0xFFFFFFFF || activeTabIndex>1) {
            context->LogWarn("[QOL/Stash] No ordinary stash destination selected; refusing Personal fallback.");
             return;
        }
        if (activeTabIndex == 1) {
            if (req->sdkSharedRequest) {
                D2RL::Items::ItemInfo live{.structSize=D2RL::Items::ItemInfoSize};
                if (g_Items->getItemInfo(context,activeTarget,&live)==D2RL::Items::Result::Success) {
                    const auto result=QolSharedSdk::Move(context,g_Items,player,live,req->sharedSelection.page,true);
                    ReportSharedMove(context,result,req->sharedSelection.page,true);
                    if(result.committed){s_FocusedItem.store(D2RL::InvalidItemHandle);s_FocusedCode.store(0);}
                }
                return;
            }
            if (QolSharedSdk::Supported(g_Items) && !(req->sharedSelection.valid && req->sharedSelection.previousSeason)) {
                context->LogWarn("[QOL/SharedSDK] No captured normal destination; native fallback refused.");return;
            }
            if (!QolShared::ValidateOwner(exeBase) ||
                !context->CheckExpectedBytes(0x2EF880,SharedOwnerIdBytes,sizeof(SharedOwnerIdBytes)) ||
                !context->CheckExpectedBytes(0x09A5D0,SharedUnitLookupBytes,sizeof(SharedUnitLookupBytes))) {
                context->LogWarn("[QOL/Stash] Selected Shared owner profile mismatch; deposit refused.");
                 return;
            }
            // Shared Stash tab: use native client transfer with resolved shared stash container unit
            struct SharedDeposit { uintptr_t base; bool submitted{}; } shared{exeBase};
            g_Items->editNativeItem(context,activeTarget,[](const D2RL::PluginContext*,void* item,void* user) noexcept {
                auto& d=*static_cast<SharedDeposit*>(user);
                d.submitted=D2R::Native::DepositToActiveStashPage(d.base,item,1);
            },&shared);
            if (shared.submitted) {
                context->LogInfo("[ControllerQoL] Quick Move: Submitted deposit to selected Shared Stash page.");
                s_FocusedItem.store(D2RL::InvalidItemHandle);
                s_FocusedCode.store(0);

                return;
            } else {
                context->LogWarn("[ControllerQoL] Quick Move: Failed to deposit into Shared Stash tab (tab full or error).");

                return;
            }
        } else {
            // Personal Stash tab: Use D2RLoader's reliable server-side item transaction
            D2RL::Items::ExistingItemOperation op{};
            op.structSize = D2RL::Items::ExistingItemOperationSize;
            op.flags = 0;
            op.kind = D2RL::Items::ExistingItemOperationKind::Move;
            op.item = activeTarget;
            op.move.destination.structSize = D2RL::Items::ItemDestinationSize;
            op.move.destination.flags      = 0;
            op.move.destination.container  = D2RL::Items::ItemContainer::PersonalStash;
            op.move.destination.placement  = D2RL::Items::Placement::Automatic;

            const D2RL::Items::ExistingItemTransaction txn{
                .structSize     = D2RL::Items::ExistingItemTransactionSize,
                .player         = player,
                .operationCount = 1,
                .operations     = &op,
            };
            D2RL::Items::ExistingItemTransactionResult res{ .structSize = D2RL::Items::ExistingItemTransactionResultSize };
            auto txnRes = g_Items->executeExistingItemTransaction(context, &txn, &res);
            if (txnRes == D2RL::Items::Result::Success) {
                context->LogInfo("[ControllerQoL] Quick Move: Successfully deposited to Personal Stash via transaction!");
                s_FocusedItem.store(D2RL::InvalidItemHandle);
                s_FocusedCode.store(0);

                return;
            } else {
                char msg[128];
                std::snprintf(msg, sizeof(msg), "[ControllerQoL] Quick Move: Personal Stash transaction failed (code %u, failIdx %u)",
                    static_cast<uint32_t>(txnRes), res.failureIndex);
                context->LogWarn(msg);

                return;
            }
        }
    }

    // 3. Shared Stash -> Inventory: Native client transfer
    if (req->sourceContainer == D2RL::Items::ItemContainer::SharedStash) {
        if (req->sdkSharedRequest) {
            D2RL::Items::ItemInfo live{.structSize=D2RL::Items::ItemInfoSize};
            if (g_Items->getItemInfo(context,activeTarget,&live)==D2RL::Items::Result::Success && QolShared::SameItem(req->sharedIdentity,live)) {
                const auto result=QolSharedSdk::Move(context,g_Items,player,live,req->sharedSelection.page,false,req->primaryDestination);
                ReportSharedMove(context,result,req->sharedSelection.page,false);
                if(result.committed){s_FocusedItem.store(D2RL::InvalidItemHandle);s_FocusedCode.store(0);}
            }
            return;
        }
        if (QolSharedSdk::Supported(g_Items) && !(req->sharedSelection.valid && req->sharedSelection.previousSeason)) {
            context->LogWarn("[QOL/SharedSDK] No captured normal source; native fallback refused.");return;
        }
        if(req->primaryDestination!=D2RL::Items::ItemContainer::Inventory) {
            context->LogWarn("[QOL/Shared] Direct Cube withdrawal unavailable on this authoritative route; no Inventory fallback.");return;
        }
        struct Withdrawal { uintptr_t base; void* player; bool submitted{}; } withdrawal{exeBase,playerUnit};
        g_Items->editNativeItem(context,activeTarget,[](const D2RL::PluginContext*,void* item,void* user) noexcept {
            auto& w=*static_cast<Withdrawal*>(user);
            w.submitted=D2R::Native::TransferFromStashToInventory(w.base,item,w.player);
        },&withdrawal);
        if (withdrawal.submitted) {
            context->LogInfo("[ControllerQoL] Quick Move: Submitted Shared Stash withdrawal (native page 4 -> 0); awaiting game update.");
            s_FocusedItem.store(D2RL::InvalidItemHandle);
            s_FocusedCode.store(0);
        } else {
            context->LogWarn("[ControllerQoL] Quick Move: Shared Stash withdrawal refused; no fallback.");
        }

        return;
    }

    // 4. Default destination loop (Cube, PersonalStash fallback, Inventory fallback)
    D2RL::Items::ItemContainer candidateDests[4] = {
        req->primaryDestination,
        req->secondaryDestination,
        D2RL::Items::ItemContainer::Unknown,
        D2RL::Items::ItemContainer::Unknown
    };

    bool moveSucceeded = false;
    for (int i = 0; i < 4; ++i) {
        D2RL::Items::ItemContainer dest = candidateDests[i];
        if (dest == D2RL::Items::ItemContainer::Unknown) continue;

        D2RL::Items::ExistingItemOperation op{};
        op.structSize = D2RL::Items::ExistingItemOperationSize;
        op.flags = 0;
        op.kind = D2RL::Items::ExistingItemOperationKind::Move;
        op.item = activeTarget;
        op.move.destination.structSize = D2RL::Items::ItemDestinationSize;
        op.move.destination.flags = 0;
        op.move.destination.container  = dest;
        op.move.destination.placement  = D2RL::Items::Placement::Automatic;

        const D2RL::Items::ExistingItemTransaction txn {
            .structSize     = D2RL::Items::ExistingItemTransactionSize,
            .player         = player,
            .operationCount = 1,
            .operations     = &op,
        };

        D2RL::Items::ExistingItemTransactionResult res {
            .structSize = D2RL::Items::ExistingItemTransactionResultSize,
        };

        auto result = g_Items->executeExistingItemTransaction(context, &txn, &res);
        char logBuf[192];
        std::snprintf(logBuf, sizeof(logBuf),
            "[ControllerQoL] Move to container %u result: code=%u, failIndex=%u, committed=%u",
            static_cast<uint32_t>(dest), static_cast<uint32_t>(result),
            res.failureIndex, res.committedOperationCount);
        context->LogInfo(logBuf);

        if (result == D2RL::Items::Result::Success) {
            context->LogInfo("[ControllerQoL] Quick-moved item successfully!");
            moveSucceeded = true;
            s_FocusedItem.store(D2RL::InvalidItemHandle);
            s_FocusedCode.store(0);
            break;
        }
    }

    if (!moveSucceeded) {
        context->LogWarn("[ControllerQoL] Quick-move failed for all candidate destinations.");
    }


}

static void __cdecl ExecuteMoveTask(const D2RL::PluginContext* ctx,void* token) noexcept {
    auto request=s_MoveRequests.Take(reinterpret_cast<uintptr_t>(token));
    if(request)ExecuteMove(ctx,request.get());
}
static D2RL::Threads::Result QueueMoveTask(const D2RL::PluginContext* ctx,MoveRequest* raw) noexcept {
    std::unique_ptr<MoveRequest> request(raw);
    if(!s_IdentifySessionEvents)return D2RL::Threads::Result::Unavailable;
    const auto token=s_MoveRequests.Put(std::move(request));
    if(!token)return D2RL::Threads::Result::Busy;
    const auto result=g_Threads->runOnGameThread(ctx,ExecuteMoveTask,reinterpret_cast<void*>(token));
    if(result!=D2RL::Threads::Result::Success)(void)s_MoveRequests.Take(token);
    return result;
}

static void QueueLegacyQuickMove(const D2RL::Items::ItemInfo& info,bool cubeDestination) noexcept {
    if(!g_PluginContext || !g_Threads || !g_Settings.quickMove)return;
    const bool cube=cubeDestination || TestUiMode(UI_MODE_CUBE);
    const bool stash=TestUiMode(UI_MODE_STASH);
    if(!cube && !stash)return;
    using C=D2RL::Items::ItemContainer;
    const auto destination=info.container==C::Inventory ? (cube?C::Cube:C::PersonalStash) : C::Inventory;
    if(!QolCubeShortcut::Allows(info.code,destination))return;
    auto* req=new MoveRequest{};
    req->targetItem=info.handle;req->targetCode=info.code;req->cellX=info.x;req->cellY=info.y;
    req->sourceContainer=info.container;req->primaryDestination=destination;
    req->secondaryDestination=info.container==C::Inventory && !cube?C::SharedStash:C::Unknown;
    req->identity=info;req->sharedIdentity=info;
    if (!cube && TestUiMode(UI_MODE_STASH) && D2R::Native::GetActiveStashTabIndex(g_PluginContext->exeBase)==1 &&
        !CaptureSharedRequest(g_PluginContext,*req)) {delete req;return;}
    const auto scheduled=QueueMoveTask(g_PluginContext,req);
    if(scheduled!=D2RL::Threads::Result::Success) {
        if(scheduled==D2RL::Threads::Result::Unavailable)
            (void)QolClientTransfer::Request(g_PluginContext,info,cube);
    }
}

static void QueueQuickMoveToCube(bool materialsFirst,const D2RL::Items::ItemInfo* captured=nullptr) noexcept;
static void TriggerQuickMoveToCubeOnFocusedItem() noexcept;

static void TriggerQuickMoveOnFocusedItem() noexcept {
    if (!g_PluginContext || !g_Threads || !g_Settings.enabled || !g_Settings.quickMove) return;
    static std::atomic<ULONGLONG> last{0};
    const auto now=GetTickCount64();
    if (now-last.exchange(now)<250) return;
    // Advanced slots are UI-owned proxies: route them before entering the
    // authoritative legacy resolver, which cannot resolve that proxy handle.
    (void)g_Threads->runOnUiThread(g_PluginContext, [](const D2RL::PluginContext* ctx,void*) noexcept {
        if (QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy() || QolNativeIdentify::Busy() || QolClientTransfer::Busy()) return;
        const auto tab=QolMaterials::SelectedStashTab(ctx);
        if (TestUiMode(UI_MODE_STASH) && tab==0xFFFFFFFF) {
            ctx->LogWarn("[QOL/Stash] Selected tab unavailable; refusing an ordinary stash fallback.");
            return;
        }
        auto source=s_FocusedContainer.load();
        D2RL::Items::ItemInfo focused{};
        bool hasFocused=false;
        if (g_Items) {
            D2RL::Items::ItemInfo info{.structSize=D2RL::Items::ItemInfoSize};
            if (g_Items->getItemInfo(ctx,s_FocusedItem.load(),&info)==D2RL::Items::Result::Success) {
                source=info.container;focused=info;hasFocused=true;
                if (GetTickCount64()-s_FocusedTick.load()>1000) return;
                if(TestUiMode(UI_MODE_STASH) && !QolStorageFocus::Matches(ctx,info)) {
                    ctx->LogInfo("[QOL/Stash] Controller selection is empty, changed, or unsupported; ignored stale tooltip transfer.");
                    return;
                }
                if (QolMove::CustomRoute(TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE),IsVendorPanelOpen()) &&
                    QolCustomPage::TryTransfer(ctx,info)) return;
                if (QolVendor::Context(IsVendorPanelOpen(),TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE),source)) {
                    TriggerNativeSellOnFocusedItem(ctx,info); return;
                }
                if (TestUiMode(UI_MODE_STASH) && QolMaterials::TryWithdrawFocused(ctx,info)) return;
                if (tab==1 && source==D2RL::Items::ItemContainer::SharedStash && TestUiMode(UI_MODE_STASH)) {
                    auto* req=new MoveRequest{};
                    req->targetItem=info.handle; req->targetCode=info.code;
                    req->cellX=info.x; req->cellY=info.y;
                    req->sourceContainer=source;
                    req->primaryDestination=D2RL::Items::ItemContainer::Inventory;
                    req->sharedIdentity=info;
                    if (!CaptureSharedRequest(ctx,*req)) {delete req;return;}
                    const auto scheduled=QueueMoveTask(ctx,req);
                    if(scheduled!=D2RL::Threads::Result::Success) {
                        if(scheduled==D2RL::Threads::Result::Unavailable)(void)QolClientTransfer::Request(ctx,info,false);
                    }
                    return;
                }
            }
        }
        if (QolMove::CustomRoute(TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE),IsVendorPanelOpen()) && QolCustomPage::Visible(ctx)) return; // Unresolved focus must not target another container.
        if (tab>=2 && tab<=4 && source!=D2RL::Items::ItemContainer::Inventory &&
            source!=D2RL::Items::ItemContainer::Cube) {
            ctx->LogWarn("[QOL/Materials] Advanced focus unresolved; refusing ordinary stash lookup.");
            return;
        }
        if (source==D2RL::Items::ItemContainer::Cube) {
            if(QolClientTransfer::EmbeddedCube(TestUiMode(UI_MODE_STASH),tab,source)) {
                QueueQuickMoveToCube(true);return;
            }
            TriggerQuickMoveToCubeOnFocusedItem(); return;
        }
        if (QolMaterials::UseMaterialsRoute(tab,source)) {
            ctx->LogInfo("[QOL/Materials] LB+X -> existing Cube action with smart storage first.");
            QueueQuickMoveToCube(true); return;
        }
        if (tab==1 && source==D2RL::Items::ItemContainer::SharedStash) {
            ctx->LogWarn("[QOL/Shared] Focus snapshot unavailable; no cross-page fallback."); return;
        }
        if(hasFocused)QueueLegacyQuickMove(focused,TestUiMode(UI_MODE_CUBE));
        else ctx->LogWarn("[QOL/Move] Current UI identity unavailable; no cell/code fallback.");
    },nullptr);
}

static bool PlayerHasCube(const D2RL::PluginContext* context, D2RL::PlayerHandle player) noexcept {
    if (TestUiMode(UI_MODE_CUBE)) return true;
    if (!g_Inventory || player == D2RL::InvalidPlayerHandle) return false;

    static std::atomic<bool> s_CachedHasCube{false};
    static std::atomic<ULONGLONG> s_LastCubeCheckTick{0};
    const ULONGLONG now = GetTickCount64();
    if (now - s_LastCubeCheckTick.load() < 750) {
        return s_CachedHasCube.load();
    }
    s_LastCubeCheckTick.store(now);

    bool found = false;
    D2RL::Inventory::ItemFilter filter{
        .structSize    = D2RL::Inventory::ItemFilterSize,
        .flags         = 0,
        .containerMask = D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory) |
                         D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::PersonalStash) |
                         D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::SharedStash) |
                         D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Cube),
        .reserved      = 0,
    };
    g_Inventory->forEachInventoryItem(context, player, &filter, [](const D2RL::PluginContext*, const D2RL::Items::ItemInfo* item, void* u) noexcept -> D2RL::Inventory::IterationAction {
        if (QolCubeShortcut::IsCube(item->code) || item->container == D2RL::Items::ItemContainer::Cube) {
            *static_cast<bool*>(u) = true;
            return D2RL::Inventory::IterationAction::Stop;
        }
        return D2RL::Inventory::IterationAction::Continue;
    }, &found);

    s_CachedHasCube.store(found);
    return found;
}

// UI thread only: capture the source, local SDK owner and Shared page before dispatch.
static void QueueQuickMoveToCube(bool materialsFirst,const D2RL::Items::ItemInfo* captured) noexcept {
    if(!g_PluginContext || !g_Threads || !g_Settings.enabled || !g_Settings.quickMove || !s_IdentifySessionEvents)return;
    if(QolNativeIdentify::Busy() || QolClientTransfer::Busy())return;
    D2RL::Items::ItemInfo focused{};
    if(captured)focused=*captured;else if(!ReadFocusIdentity(focused))return;
    const auto destination=focused.container==D2RL::Items::ItemContainer::Cube?
        D2RL::Items::ItemContainer::Inventory:D2RL::Items::ItemContainer::Cube;
    if(!QolCubeShortcut::Allows(focused.code,destination) || !QolCubeShortcut::Context(focused.container,
        TestUiMode(UI_MODE_INVENTORY),TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE)) ||
        !QolStorageFocus::Matches(g_PluginContext,focused))return;
    static uint64_t last{}; // All callers are on the UI thread.
    const auto now=GetTickCount64();
    if(last && now-last<250)return;
    last=now;
    auto request=std::make_unique<MoveRequest>();
    request->targetItem=focused.handle;request->targetCode=focused.code;
    request->cellX=focused.x;request->cellY=focused.y;request->sourceContainer=focused.container;
    request->primaryDestination=destination;
    request->secondaryDestination=D2RL::Items::ItemContainer::Unknown;
    request->materialsFirst=materialsFirst;request->sharedIdentity=focused;request->identity=focused;
    request->cubeShortcut=true;request->actionGeneration=s_ActionGeneration.load();request->actionRequestedAt=now;
    if(!g_Inventory || g_Inventory->getLocalPlayer(g_PluginContext,&request->player)!=D2RL::Inventory::Result::Success || !request->player)return;
    if(focused.container==D2RL::Items::ItemContainer::SharedStash && !CaptureSharedRequest(g_PluginContext,*request))return;
    const auto scheduled=QueueMoveTask(g_PluginContext,request.release());
    if(scheduled==D2RL::Threads::Result::Unavailable)
        (void)QolClientTransfer::Request(g_PluginContext,focused,!materialsFirst);
}

static void TriggerQuickMoveToCubeOnFocusedItem() noexcept {
    if(!g_PluginContext || !g_Threads || !g_Settings.enabled || !g_Settings.quickMove || !s_IdentifySessionEvents)return;
    // Copy at the physical press. A delayed UI callback must not substitute a new tooltip.
    D2RL::Items::ItemInfo captured{};
    if(!ReadFocusIdentity(captured))return;
    const auto token=s_CubeRequests.Put(std::make_unique<CubePending>(CubePending{captured,s_ActionGeneration.load(),GetTickCount64()}));
    if(!token)return;
    const auto result=g_Threads->runOnUiThread(g_PluginContext,[](const D2RL::PluginContext* ctx,void* user) noexcept {
        auto pending=s_CubeRequests.Take(reinterpret_cast<uintptr_t>(user));
        if(!pending || !g_Settings.enabled || !QolCubeShortcut::Current(pending->generation,s_ActionGeneration.load(),pending->requestedAt,GetTickCount64()))return;
        if(QolNativeIdentify::Busy() || QolClientTransfer::Busy() || QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy())return;
        const auto& info=pending->info;
        D2RL::Items::ItemInfo current{};
        if(!ReadFocusIdentity(current) || !QolIdentify::Same(info,current) || !QolCubeShortcut::Context(info.container,
            TestUiMode(UI_MODE_INVENTORY),TestUiMode(UI_MODE_STASH),TestUiMode(UI_MODE_CUBE)) || !QolStorageFocus::Matches(ctx,info))return;
        if(TestUiMode(UI_MODE_STASH) && QolMaterials::TryWithdrawToCube(ctx,info))return;
        QueueQuickMoveToCube(false,&info);
    },reinterpret_cast<void*>(token));
    if(result!=D2RL::Threads::Result::Success)(void)s_CubeRequests.Take(token);
}
static bool TriggerAdvancedSingleBelt() noexcept {
    const auto advancedStamp=s_AdvancedBeltFocusTick.load();
    const auto sharedStamp=s_SharedBeltFocusTick.load();
    const auto stamp=advancedStamp>sharedStamp?advancedStamp:sharedStamp;
    if(!g_Settings.enabled || !g_Settings.quickMove || !stamp || GetTickCount64()-stamp>250 ||
       !TestUiMode(UI_MODE_STASH) || QolNativeIdentify::Busy() || QolClientTransfer::Busy() || QolBelt::Busy() || QolBulkStash::Busy())return false;
    D2RL::Items::ItemInfo info{};
    if(!ReadFocusIdentity(info))return false;
    if(sharedStamp && sharedStamp>=advancedStamp) {
        // Shared grid activation can bypass ItemInteraction. The tooltip has
        // qualified the real controller cell; the UI belt callback rechecks it.
        if(info.container!=D2RL::Items::ItemContainer::SharedStash || !QolBelt::BeltCandidate(info.code))return false;
        return QolBelt::RequestSingle(s_LastPlayerHandle.load(),info,true);
    }
    return QolMaterials::RequestSingleBelt(info);
}

static bool VendorRefillContext() noexcept {
    return IsVendorPanelOpen() && !TestUiMode(UI_MODE_STASH) && !TestUiMode(UI_MODE_CUBE);
}
static void TriggerAutoFillBelt() noexcept {
    if (!g_PluginContext || !g_Settings.enabled || !g_Settings.quickMove || !g_Threads) return;
    (void)g_Threads->runOnUiThread(g_PluginContext,[](const D2RL::PluginContext* ctx,void*) noexcept {
        if (QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy() || QolNativeIdentify::Busy() || QolClientTransfer::Busy()) return;
        const bool stash=TestUiMode(UI_MODE_STASH);
        if (VendorRefillContext()) {
            D2RL::Items::ItemInfo focus{};
            if (!ReadFocusIdentity(focus) || GetTickCount64()-s_FocusedTick.load()>1000) return;
            if (focus.container!=D2RL::Items::ItemContainer::Inventory) {
                if ((!IsPotionItem(focus.code) && !QolVendor::TomeForScroll(focus.code)) || !QolBelt::RequestVendorRefill(ctx,focus,VendorRefillContext))
                    ctx->LogWarn("[QOL/VendorBelt] Highlighted merchant potion/scroll could not be bound; no purchase.");
                return;
            }
        }
        if (!stash && !TestUiMode(UI_MODE_INVENTORY) && !TestUiMode(UI_MODE_CUBE)) return;
        if (stash && QolMaterials::SelectedStashTab(ctx)==3 && g_Items) {
            D2RL::Items::ItemInfo focus{.structSize=D2RL::Items::ItemInfoSize};
            if (g_Items->getItemInfo(ctx,s_FocusedItem.load(),&focus)==D2RL::Items::Result::Success &&
                IsPotionItem(focus.code)) {
                if (QolMaterials::RequestFocusedRefill(ctx,focus)) return;
                if (focus.container!=D2RL::Items::ItemContainer::Inventory &&
                    focus.container!=D2RL::Items::ItemContainer::Cube) {
                    ctx->LogWarn("[QOL/Materials] Focused refill binding unavailable; no different potion selected.");
                    return;
                }
            } else {
                ctx->LogInfo("[QOL/Materials] Highlight a potion before Materials refill.");
                return;
            }
        }
        if (!QolBelt::RequestRefill(stash))
            ctx->LogWarn("[QOL/Belt] Refill unavailable; inspect native contract/service diagnostics.");
    },nullptr);
}

static auto __cdecl OnItemInteraction(
    const D2RL::PluginContext* context,
    const D2RL::ItemInteractions::ItemInteractionEvent* event,
    void* userData
) noexcept -> D2RL::ItemInteractions::Decision {
    (void)userData;

    if (!context || !event || event->structSize < D2RL::ItemInteractions::ItemInteractionEventRequiredSize) {
        return D2RL::ItemInteractions::Decision::Continue;
    }

    if (QolNativeIdentify::Forwarding() || !g_Settings.enabled) {
        return D2RL::ItemInteractions::Decision::Continue;
    }

    if (event->player != D2RL::InvalidPlayerHandle) {
        s_LastPlayerHandle.store(event->player);
    }

    const bool isController = (event->inputSource == D2RL::ItemInteractions::InputSource::Controller);
    // This listener owns controller shortcuts only. Preserve native Ctrl+click,
    // including advanced-stash deposits and ordinary keyboard item actions.
    if (!isController) return D2RL::ItemInteractions::Decision::Continue;

    if (g_Settings.debugLogging) {
        char logMsg[256];
        std::snprintf(logMsg, sizeof(logMsg),
            "[ControllerQoL] Item activated: item=%llu inputSource=%u (%s) mod=0x%X container=%u cell=(%d,%d)",
            static_cast<unsigned long long>(event->item),
            static_cast<uint32_t>(event->inputSource),
            isController ? "Controller" : "KBM",
            event->modifiers,
            static_cast<uint32_t>(event->container),
            event->cellX, event->cellY);
        context->LogInfo(logMsg);
    }

    if (!g_Items || !g_Threads) {
        return D2RL::ItemInteractions::Decision::Continue;
    }

    // Inspect the clicked item
    D2RL::Items::ItemInfo itemInfo{
        .structSize = D2RL::Items::ItemInfoSize,
    };
    const auto infoRes = g_Items->getItemInfo(context, event->item, &itemInfo);
    if (infoRes != D2RL::Items::Result::Success) {
        char errBuf[128];
        std::snprintf(errBuf, sizeof(errBuf), "[ControllerQoL] getItemInfo failed with code %u", static_cast<uint32_t>(infoRes));
        context->LogWarn(errBuf);
        return D2RL::ItemInteractions::Decision::Continue;
    }

    const bool isUnidentified = (itemInfo.stateFlags & D2RL::Items::ItemStateIdentified) == 0;
    if (g_Settings.debugLogging) {
        char inspectLog[256];
        std::snprintf(inspectLog, sizeof(inspectLog),
            "[ControllerQoL] Item inspected: code=0x%08X, quality=%u, unidentified=%d",
            itemInfo.code, static_cast<uint32_t>(itemInfo.quality), isUnidentified ? 1 : 0);
        context->LogInfo(inspectLog);
    }

    // Check if modifier is active
    bool modifierActive = false;
    const bool btnA = isController && ControllerQoL::IsButtonAPressed();
    const bool btnX = isController && ControllerQoL::IsButtonXPressed();

    if (isController) {
        if (!g_Settings.requireModifier) {
            modifierActive = true;
        } else {
            const uint8_t ltVal = ControllerQoL::GetMaxLeftTriggerValue();
            const bool lbVal = ControllerQoL::IsLeftBumperPressed();
            const bool bracketVal = (GetAsyncKeyState(VK_OEM_4) & 0x8000) != 0;
            modifierActive = ControllerQoL::IsControllerModifierActive(g_Settings.modifier, g_Settings.triggerThreshold);

            uint8_t cachedLT = 0, cachedRT = 0;
            uint16_t cachedBtns = 0;
            uint64_t cacheAge = 0;
            ControllerQoL::GetCachedControllerState(cachedLT, cachedRT, cachedBtns, cacheAge);
            const uint64_t hookCalls = ControllerQoL::GetHookCallCount();

            if (g_Settings.debugLogging) {
                const auto bridgeDiag = ControllerQoL::QueryNativeBridgeDiagnostics();
                char modLog[384];
                std::snprintf(modLog, sizeof(modLog),
                    "[ControllerQoL] [BRIDGE-VALIDATION] Item Interaction: item=%llu modActive=%d | Bridge(d2rCore=%d, mgr=%d, mode=%u, altHold=(res=%d,st=%u), isAltActive=%d, steam=%d) | XInput(LT=%u, LB=%d, [=%d, calls=%llu)",
                    static_cast<unsigned long long>(event->item),
                    modifierActive ? 1 : 0,
                    bridgeDiag.d2rCoreFound ? 1 : 0, bridgeDiag.mgrFound ? 1 : 0, bridgeDiag.controllerMode,
                    bridgeDiag.altHoldRes, bridgeDiag.altHoldState, bridgeDiag.isAltActive ? 1 : 0,
                    bridgeDiag.steamControllers,
                    ltVal, lbVal ? 1 : 0, bracketVal ? 1 : 0, static_cast<unsigned long long>(hookCalls));
                context->LogInfo(modLog);
            }
        }
    } else {
        // Keyboard Ctrl+click
        modifierActive = (event->modifiers & D2RL::ItemInteractions::ModifierBit(D2RL::ItemInteractions::Modifier::Control)) != 0;
    }

    g_LastInventoryInteractionTick.store(GetTickCount64());

    if(modifierActive && (QolNativeIdentify::Busy() || QolClientTransfer::Busy()))
        return D2RL::ItemInteractions::Decision::Consume;

    if(g_Settings.quickMove && modifierActive && TestUiMode(UI_MODE_STASH) && QolBelt::BeltCandidate(itemInfo.code)) {
        if(QolMaterials::Busy() || QolBelt::Busy() || QolBulkStash::Busy())return D2RL::ItemInteractions::Decision::Consume;
        if(QolMaterials::TryWithdrawToBelt(context,itemInfo))return D2RL::ItemInteractions::Decision::Consume;
    }

    if(Probe::BatchFeatureEnabled(g_Settings.quickIdentify,g_Settings.identifyAll) &&
       modifierActive && !btnX && QolIdentify::BulkTome(itemInfo)) {
        if(QolClientTransfer::Busy() || QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy())return D2RL::ItemInteractions::Decision::Consume;
        (void)QolNativeIdentify::RequestAll(context,event->player,itemInfo,g_Settings.nativeIdentify,isController && g_Settings.consumeTomeOrScroll);
        return D2RL::ItemInteractions::Decision::Consume;
    }
    if (isUnidentified && g_Settings.quickIdentify && modifierActive) {
        // Prefer authoritative SDK work. Only an Unavailable scheduler permits
        // read-only client preparation and a native request on the UI thread.
        if(QolNativeIdentify::Busy() || QolClientTransfer::Busy() || QolBelt::Busy() || QolMaterials::Busy() || QolBulkStash::Busy())return D2RL::ItemInteractions::Decision::Consume;
        std::lock_guard lock(s_IdentifyMutex);
        if(!s_IdentifySessionEvents)return D2RL::ItemInteractions::Decision::Continue;
        if(s_IdentifyPending.exchange(true))return D2RL::ItemInteractions::Decision::Consume;
        s_IdentifyRequest={
            .action={event->player,itemInfo,g_Settings.requireTomeOrScroll,g_Settings.consumeTomeOrScroll,QolNativeIdentify::PersonalStashOpen()},
            .queuedAt=GetTickCount64(),
        };

        const auto scheduleResult = g_Threads->runOnGameThread(context, ExecuteIdentifyTask, reinterpret_cast<void*>(++s_IdentifyGeneration));
        if (scheduleResult == D2RL::Threads::Result::Success) {
            return D2RL::ItemInteractions::Decision::Consume;
        } else {
            s_IdentifyPending.store(false);
            if(isController && scheduleResult==D2RL::Threads::Result::Unavailable && g_Settings.consumeTomeOrScroll &&
               QolNativeIdentify::RequestRemote(context,event->player,itemInfo))return D2RL::ItemInteractions::Decision::Consume;
            char line[160];std::snprintf(line,sizeof(line),"[QOL/Identify] scheduling=%u remote request unavailable; preserving native input.",static_cast<unsigned>(scheduleResult));context->LogWarn(line);
            return D2RL::ItemInteractions::Decision::Continue;
        }
    }

    // Consume accepted belt work; the module resolves this exact identity on the game thread.
    if (g_Settings.quickMove && modifierActive && QolBelt::BeltCandidate(itemInfo.code) &&
        QolBelt::SupportedSource(itemInfo.container)) {
        if (QolBelt::RequestSingle(event->player, itemInfo))
            return D2RL::ItemInteractions::Decision::Consume;
        context->LogWarn("[QOL/Belt] Shortcut unavailable; inspect native contract/service diagnostics.");
    }
    return D2RL::ItemInteractions::Decision::Continue;
}

static auto __cdecl OnInputAction(
    const D2RL::PluginContext* context,
    const D2RL::Input::ActionEvent* event,
    void* userData
) noexcept -> D2RL::Input::ActionResult {
    (void)userData;
    if (!context || !event || event->kind != D2RL::Input::ActionEventKind::Pressed) {
        return D2RL::Input::ActionResult::Ignored;
    }
    context->LogInfo("[ControllerQoL] Controller QoL action pressed.");
    return D2RL::Input::ActionResult::Handled;
}

static void __cdecl OnItemTooltipCallback(
    const D2RL::PluginContext* context,
    D2RL::SharedEvents::ItemTooltipEvent* event,
    void* userData
) noexcept {
    (void)userData;
    QolGlyphs::PublishHeader(nullptr,nullptr,nullptr,nullptr,nullptr);
    s_AdvancedBeltFocusTick.store(0);
    s_SharedBeltFocusTick.store(0);
    if (!g_Settings.enabled || !event) return;

    if (g_Items && event->item != D2RL::InvalidItemHandle) {
        D2RL::Items::ItemInfo info{ .structSize = D2RL::Items::ItemInfoSize };
        if (g_Items->getItemInfo(context, event->item, &info) == D2RL::Items::Result::Success) {
            g_LastInventoryInteractionTick.store(GetTickCount64());

            s_FocusedItem.store(event->item);
            s_FocusedContainer.store(info.container);
            s_FocusedCode.store(info.code);
            s_FocusedTick.store(GetTickCount64());
            { std::lock_guard lock(s_FocusIdentityMutex);s_FocusIdentity=info;s_FocusIdentityTick=GetTickCount64(); }


            // Keep focus tracking above intact; only controller hint rendering is gated.
            if (!ControllerQoL::IsControllerUiActive()) return;
            if (!event->text || event->capacity == 0) return;

            const bool isUnidentified = ((info.stateFlags & D2RL::Items::ItemStateIdentified) == 0);
            const bool isCubeOpen     = TestUiMode(UI_MODE_CUBE);
            const bool isStashOpen    = TestUiMode(UI_MODE_STASH);
            const auto stashTab=isStashOpen?QolMaterials::SelectedStashTab(context):UINT32_MAX;
            const bool advancedFocus=isStashOpen && QolMaterials::AdvancedProxyCandidate(info.container) &&
                QolMaterials::Category(stashTab) && QolStorageFocus::Matches(context,info);
            const bool advancedPotion=advancedFocus &&
                (info.code==QolMaterials::RejuvenationCode(0) || info.code==QolMaterials::RejuvenationCode(1));
            if(advancedPotion)s_AdvancedBeltFocusTick.store(GetTickCount64());
            if(QolBelt::SharedInputCandidate(info.container,stashTab,info.code) && QolStorageFocus::Matches(context,info))
                s_SharedBeltFocusTick.store(GetTickCount64());
            const bool isVendorOpen   = IsVendorPanelOpen();
            // Merchant stock must not advertise player-storage transfers.
            const bool vendorStock=isVendorOpen && !isStashOpen && !isCubeOpen &&
                info.container!=D2RL::Items::ItemContainer::Inventory;


            D2RL::PlayerHandle player = s_LastPlayerHandle.load();
            if (player == D2RL::InvalidPlayerHandle && g_Inventory) {
                g_Inventory->getLocalPlayer(context, &player);
                if (player != D2RL::InvalidPlayerHandle) s_LastPlayerHandle.store(player);
            }
            const bool playerHasCube  = PlayerHasCube(context, player);

            const bool canMove        = !vendorStock && g_Settings.quickMove && (
                (info.container == D2RL::Items::ItemContainer::Inventory && (isCubeOpen || isStashOpen || isVendorOpen || QolCustomPage::Visible(context))) ||
                (info.container == D2RL::Items::ItemContainer::PersonalStash ||
                 info.container == D2RL::Items::ItemContainer::SharedStash ||
                 info.container == D2RL::Items::ItemContainer::CustomPage ||
                 info.container == D2RL::Items::ItemContainer::Cube)
            ) && !(info.container==D2RL::Items::ItemContainer::Inventory && isCubeOpen && QolCubeShortcut::IsCube(info.code));

            const auto cubeSharedSelection=info.container==D2RL::Items::ItemContainer::SharedStash?
                QolSharedSdk::Selected(context->exeBase):QolSharedSdk::Selection{};
            const bool cubeSourceSupported=info.container!=D2RL::Items::ItemContainer::SharedStash ||
                (QolSharedSdk::Supported(g_Items) && cubeSharedSelection.valid && !cubeSharedSelection.previousSeason);
            const bool canMoveToCube  = cubeSourceSupported && !vendorStock && g_Settings.quickMove && playerHasCube && !QolCubeShortcut::IsCube(info.code) && (!isCubeOpen) && (
                info.container == D2RL::Items::ItemContainer::Inventory ||
                info.container == D2RL::Items::ItemContainer::PersonalStash ||
                info.container == D2RL::Items::ItemContainer::SharedStash || advancedFocus
            );

            const bool isPotionItem   = QolBelt::BeltCandidate(info.code) && QolBelt::SupportedSource(info.container);
            const bool canMoveToBelt  = !vendorStock && (isPotionItem || advancedPotion);
            const bool canIdentifyAll = !vendorStock &&
                Probe::BatchFeatureEnabled(g_Settings.quickIdentify,g_Settings.identifyAll) && QolIdentify::BulkTome(info);
            const bool canIdentify    = !vendorStock && isUnidentified && g_Settings.quickIdentify;
            const bool vendorScroll=vendorStock && QolVendor::TomeForScroll(info.code)!=0;
            const bool canAutoFill    = g_Settings.quickMove && (vendorScroll || (IsPotionItem(info.code) && (vendorStock || isStashOpen || isCubeOpen || info.container == D2RL::Items::ItemContainer::Inventory ||
                                         info.container == D2RL::Items::ItemContainer::PersonalStash ||
                                         info.container == D2RL::Items::ItemContainer::SharedStash)));

            const bool canBulkStash=isStashOpen &&
                Probe::BatchFeatureEnabled(g_Settings.quickMove,g_Settings.quickDeposit);
            if (canBulkStash || canIdentifyAll || canIdentify || canMoveToBelt || canMove || canMoveToCube || canAutoFill) {
                char prompt[256]{};
                const char* moveAction = "Transfer";
                if (info.container == D2RL::Items::ItemContainer::Inventory && isVendorOpen && !isStashOpen && !isCubeOpen) {
                    moveAction = "Sell";
                }

                const char* modStr = "LT";
                if (g_Settings.requireModifier) {
                    if (std::strcmp(g_Settings.modifier, "[") == 0 || _stricmp(g_Settings.modifier, "l4") == 0 ||
                        _stricmp(g_Settings.modifier, "bracket") == 0 || _stricmp(g_Settings.modifier, "leftbracket") == 0) {
                        modStr = "[";
                    } else if (_stricmp(g_Settings.modifier, "lb") == 0 || _stricmp(g_Settings.modifier, "bumper") == 0) {
                        modStr = "LB";
                    } else if (_stricmp(g_Settings.modifier, "l3") == 0) {
                        modStr = "L3";
                    } else if (_stricmp(g_Settings.modifier, "rb") == 0) {
                        modStr = "RB";
                    } else if (_stricmp(g_Settings.modifier, "rt") == 0) {
                        modStr = "RT";
                    } else {
                        modStr = "LT";
                    }
                }

                QolGlyphs::PublishHeader(g_Settings.requireModifier?modStr:"",
                    canIdentifyAll?"Identify All":canIdentify?"Identify":(canMoveToBelt?"To Belt":nullptr),
                    canMove?moveAction:nullptr,canMoveToCube?"To Cube":nullptr,
                    canAutoFill?(vendorScroll?"Fill Tome":vendorStock?"Refill / Buy":"Fill Belt"):nullptr,canBulkStash?"Stash All":nullptr);
                int promptLen = 0;
                auto appendLine = [&](const char* btn, const char* act) {
                    if (promptLen > 0 && promptLen < static_cast<int>(sizeof(prompt)) - 1) {
                        prompt[promptLen++] = '\n';
                        prompt[promptLen] = '\0';
                    }
                    if (g_Settings.requireModifier) {
                        if (std::strcmp(modStr, "[") == 0) {
                            promptLen += std::snprintf(prompt + promptLen, sizeof(prompt) - promptLen, "[ + %s] %s", btn, act);
                        } else {
                            promptLen += std::snprintf(prompt + promptLen, sizeof(prompt) - promptLen, "[%s + %s] %s", modStr, btn, act);
                        }
                    } else {
                        promptLen += std::snprintf(prompt + promptLen, sizeof(prompt) - promptLen, "[%s] %s", btn, act);
                    }
                };

                if (canIdentifyAll) {
                    appendLine("A", "Identify All");
                } else if (canIdentify) {
                    appendLine("A", "Identify");
                } else if (canMoveToBelt) {
                    appendLine("A", "To Belt");
                }
                if (canMove) {
                    appendLine("X", moveAction);
                }
                if (canMoveToCube) {
                    appendLine("Y", "To Cube");
                }
                if (canAutoFill) {
                    appendLine("R3", vendorScroll?"Fill Tome":vendorStock ? "Refill Belt / Buy Missing" : "Fill Belt");
                }

                const size_t len = std::strlen(prompt);
                if (len < event->capacity) {
                    std::memcpy(event->text, prompt, len + 1);
                    event->length = static_cast<uint32_t>(len);
                }
            }
        }
    }
}

} // namespace

namespace PlacardOverlay {
    void UnregisterPlacard(uint32_t guid);
    bool IsPlacardActive(uint32_t guid);
    size_t GetActivePlacardCount();
}

namespace GroundLoot {
    constexpr std::uintptr_t PickupRva = 0x471950;
    constexpr std::uintptr_t GetItemCodeRva = 0x36EF50;

    constexpr uint8_t GetItemCodeExpected[32] = {
        0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83,
        0xEC, 0x20, 0x48, 0x8B, 0xF9, 0x48, 0x85, 0xC9,
        0x75, 0x13, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D,
        0x4C, 0x24, 0x30, 0xE8, 0x80, 0x83, 0xFF, 0xFF,
    };

    using PickupFn = bool(__fastcall*)(void*, std::uint32_t, bool, std::uint32_t, bool, bool);
    using GetSelectedSkillFn = void*(__fastcall*)(void*);

    static uint8_t* s_Base = nullptr;
    static bool s_FingerprintValid = false;

    static PickupFn s_Pickup = nullptr;
    static std::atomic<bool> s_PickupCallsActive{false};
    static GetSelectedSkillFn s_GetSelectedSkill = nullptr;

    static bool LocalActor(void* player,uint32_t& id) noexcept;
    static bool BlockClientPickup(uint32_t guid,uint32_t code) noexcept {
        if(!g_Settings.enabled || !g_Settings.blockFilteredPickup ||
           !ControllerQoL::IsControllerUiActive() || QolClientLoot::IsGoldCode(code))return false;
        const bool blocked=!PlacardOverlay::IsPlacardActive(guid);
        if(blocked && g_PluginContext && g_Settings.debugLogging) {
            char message[128];std::snprintf(message,sizeof(message),"[QOL/ClientLoot] Suppressed filtered pickup GUID=%u before submission.",guid);
            g_PluginContext->LogInfo(message);
        }
        return blocked;
    }

    static bool ProjectGuidedArrowPacket(void* player, const void* packet,
        int32_t size, std::array<uint8_t,5>& projectedPacket) noexcept {
        uint32_t actorId{};
        if (!g_Settings.enabled || !LocalActor(player,actorId) || !ControllerQoL::IsControllerUiActive() ||
            !player || !packet || size != static_cast<int32_t>(projectedPacket.size()) ||
            !s_GetSelectedSkill)
            return false;

        __try {
            std::memcpy(projectedPacket.data(), packet, projectedPacket.size());
            if (projectedPacket[0] != 0x05) return false;

            void* const selectedSkill = s_GetSelectedSkill(player);
            void* const skillRecord = selectedSkill
                ? *reinterpret_cast<void**>(selectedSkill)
                : nullptr;
            void* const path = *reinterpret_cast<void**>(
                static_cast<uint8_t*>(player) + 0x38);
            if (!skillRecord || !path) return false;

            const auto skillId = *reinterpret_cast<uint16_t*>(skillRecord);
            if (skillId==GuidedArrowProjection::SkillId && QolAim::OwnsGuidedArrow()) return false;
            const auto playerX = *reinterpret_cast<uint16_t*>(
                static_cast<uint8_t*>(path) + 0x02);
            const auto playerY = *reinterpret_cast<uint16_t*>(
                static_cast<uint8_t*>(path) + 0x06);
            uint16_t targetX{};
            uint16_t targetY{};
            std::memcpy(&targetX, projectedPacket.data() + 1, sizeof(targetX));
            std::memcpy(&targetY, projectedPacket.data() + 3, sizeof(targetY));

            uint16_t projectedX{};
            uint16_t projectedY{};
            if (!GuidedArrowProjection::TryProject(
                    playerX, playerY, targetX, targetY, skillId,
                    projectedX, projectedY))
                return false;

            std::memcpy(projectedPacket.data() + 1, &projectedX, sizeof(projectedX));
            std::memcpy(projectedPacket.data() + 3, &projectedY, sizeof(projectedY));
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool LocalActor(void* player,uint32_t& id) noexcept {
        __try {
            if(!player)return false;
            const auto* words=static_cast<const uint32_t*>(player);
            id=words[2];return QolClientLoot::IsLocalActor(words[0],id);
        } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
    }

    __declspec(noinline) bool __fastcall HookPickup(void* player, uint32_t guid, bool arg3, uint32_t dist, bool arg5, bool arg6) {
        if (!s_Pickup) return false;
        if (!s_PickupCallsActive.load())
            return s_Pickup(player, guid, arg3, dist, arg5, arg6);

        uint32_t actorId{};
        if(!g_Settings.enabled || !QolClientLoot::Ready() || !LocalActor(player,actorId) || QolClientLoot::TakeNativePickupPermit(actorId,guid))
            return s_Pickup(player,guid,arg3,dist,arg5,arg6);
        const bool shortcutsEnabled = Probe::GroundShortcutsEnabled(g_Settings.enabled, g_Settings.groundPickup);
        const bool modifierHeld = shortcutsEnabled &&
            ControllerQoL::IsGroundPickupActive(g_Settings.groundPickupButton);
        const bool blockFilteredPickup = g_Settings.blockFilteredPickup && ControllerQoL::IsControllerUiActive();
        const bool placardActive = !blockFilteredPickup || PlacardOverlay::IsPlacardActive(guid);
        if (Probe::BlockNativePickup(shortcutsEnabled, modifierHeld,
                blockFilteredPickup, placardActive)) {
            // Gold is exempt even when its label is hidden or LB is held.
            // Native capacity/automatic-pickup settings still decide acceptance.
            if(QolGoldPickup::IsGold(g_PluginContext,player,guid))
                return s_Pickup(player,guid,arg3,dist,arg5,arg6);
            if (g_PluginContext && g_Settings.debugLogging) {
                char supMsg[128];
                std::snprintf(supMsg, sizeof(supMsg),
                    "[ControllerQoL] Suppressed native game pickup for GUID=%u (%s)", guid,
                    modifierHeld ? "ground modifier held" : "filtered by loot filter");
                g_PluginContext->LogInfo(supMsg);
            }
            return false;
        }
        return s_Pickup(player, guid, arg3, dist, arg5, arg6);
    }

    static bool InstallPickupCalls(const D2RL::PluginContext* context) noexcept {
        if (!context || !context->exeBase) return false;
        HMODULE pinned{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&HookPickup), &pinned)) return false;
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        const uintptr_t step = info.dwAllocationGranularity;
        const uintptr_t aligned = (context->exeBase + QolPickupCalls::Calls[0].rva) & ~(step - 1);
        unsigned char* relay = nullptr;
        for (uintptr_t offset = step; offset < 0x40000000 && !relay; offset += step)
            relay = static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(aligned + offset),
                64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!relay) return false;
        relay[0] = 0xff;
        relay[1] = 0x25;
        std::memset(relay + 2, 0, 4);
        const auto destination = &HookPickup;
        std::memcpy(relay + 6, &destination, sizeof(destination));
        DWORD old{};
        if (!VirtualProtect(relay, 64, PAGE_EXECUTE_READ, &old) ||
            !FlushInstructionCache(GetCurrentProcess(), relay, 64)) {
            VirtualFree(relay, 0, MEM_RELEASE);
            return false;
        }
        // Keep the relay and pinned wrapper for process lifetime once publication
        // begins. A partial failure remains harmless because active stays false.
        const bool published = QolPickupCalls::Publish(context->exeBase,
            reinterpret_cast<uintptr_t>(relay),
            [context](uintptr_t rva, const unsigned char* expected, const unsigned char* replacement) {
                return context->PatchBytes(rva, expected, 5, replacement, 5);
            });
        if (published) s_PickupCallsActive.store(true);
        return published;
    }

    static constexpr size_t NUM_STICKY_SLOTS = 7;
    static const char* const s_SlotNames[NUM_STICKY_SLOTS] = { "A", "X", "Y", "B", "R1", "R2", "L2" };

    struct PrefixInfo {
        const wchar_t* pfxW;
        const char* pfxA;
        size_t len;
    };

    static constexpr PrefixInfo s_SlotPrefixes[NUM_STICKY_SLOTS] = {
        { L"(A) ",  "(A) ",  4 },
        { L"(X) ",  "(X) ",  4 },
        { L"(Y) ",  "(Y) ",  4 },
        { L"(B) ",  "(B) ",  4 },
        { L"(R1) ", "(R1) ", 5 },
        { L"(R2) ", "(R2) ", 5 },
        { L"(L2) ", "(L2) ", 5 },
    };

    static int GetItemCodeRank(const char* code, uint32_t quality) noexcept {
        int baseRank = 400; // default equipment gear (e.g. bld, dr8, etc.)
        if (code && code[0] != '\0') {
            // Runes: 'r01' - 'r33'
            if (code[0] == 'r' && code[1] >= '0' && code[1] <= '3' && code[2] >= '0' && code[2] <= '9') {
                int runeNum = (code[1] - '0') * 10 + (code[2] - '0');
                if (runeNum >= 20) baseRank = 2000 + runeNum; // High Runes (r20-r33) highest priority
                else if (runeNum >= 10) baseRank = 1200 + runeNum; // Mid Runes (r10-r19)
                else baseRank = 600 + runeNum; // Low Runes (r01-r09)
            }
            // Charms ('cm1', 'cm2', 'cm3') & Jewels ('jew')
            else if ((code[0] == 'c' && code[1] == 'm') ||
                     (code[0] == 'j' && code[1] == 'e' && code[2] == 'w')) {
                baseRank = 1500;
            }
            // Rings ('rin') & Amulets ('amu')
            else if ((code[0] == 'r' && code[1] == 'i' && code[2] == 'n') ||
                     (code[0] == 'a' && code[1] == 'm' && code[2] == 'u')) {
                baseRank = 1400;
            }
            // Keys ('pk1'-'pk3') & Essences/Tokens ('tes', 'ceh', 'bet', 'fed')
            else if ((code[0] == 'p' && code[1] == 'k') ||
                     (code[0] == 't' && code[1] == 'e' && code[2] == 's')) {
                baseRank = 1300;
            }
            // Gems ('gsv', 'gsa', 'skz', etc.)
            else if (code[0] == 'g' && (code[1] == 's' || code[1] == 'p' || code[1] == 'f' || code[1] == 'c')) {
                baseRank = 500;
            }
            else if (code[0] == 's' && code[1] == 'k') { // Skulls
                baseRank = 500;
            }
            // Consumables & Low Junk
            else if (code[0] == 'r' && code[1] == 'v') { // Full/Rejuv potions
                baseRank = 250;
            }
            else if (code[0] == 'h' && code[1] == 'p') { // Healing potions
                baseRank = 150;
            }
            else if (code[0] == 'm' && code[1] == 'p') { // Mana potions
                baseRank = 150;
            }
            else if (code[0] == 't' && code[1] == 's' && code[2] == 'c') { // TP Scroll
                baseRank = 80;
            }
            else if (code[0] == 'i' && code[1] == 's' && code[2] == 'c') { // ID Scroll
                baseRank = 80;
            }
            else if (code[0] == 'k' && code[1] == 'e' && code[2] == 'y') { // Keys
                baseRank = 70;
            }
            else if (code[0] == 'a' && code[1] == 'q' && code[2] == 'v') { // Arrows
                baseRank = 60;
            }
            else if (code[0] == 'c' && code[1] == 'q' && code[2] == 'v') { // Bolts
                baseRank = 60;
            }
            else if (code[0] == 'g' && code[1] == 'l' && code[2] == 'd') { // Gold
                baseRank = 10;
            }
        }

        // Quality modifier: add bonus for Unique (7), Set (5), Rare (6), Magic (4), etc.
        int qualBonus = 0;
        switch (quality) {
            case 7: qualBonus = 350; break; // Unique
            case 5: qualBonus = 300; break; // Set
            case 8: qualBonus = 250; break; // Crafted
            case 6: qualBonus = 200; break; // Rare
            case 4: qualBonus = 100; break; // Magic
            case 3: qualBonus = 30;  break; // Superior
            case 2: qualBonus = 20;  break; // Normal
            default: qualBonus = 0;  break;
        }

        return baseRank + qualBonus;
    }

    static bool InstallHooks(const D2RL::PluginContext* context, uint8_t*) noexcept {
        s_GetSelectedSkill = nullptr;
        QolCompat::ActionHooks::Projector projector = nullptr;
        bool guidedArrowGuardMatched = false;
        if (context && context->CheckExpectedBytes(
                QolCompat::Native::ControllerSelectedSkill.rva,
                QolCompat::Native::ControllerSelectedSkill.bytes,
                sizeof(QolCompat::Native::ControllerSelectedSkill.bytes))) {
            s_GetSelectedSkill = reinterpret_cast<GetSelectedSkillFn>(
                context->exeBase + QolCompat::Native::ControllerSelectedSkill.rva);
            projector = &ProjectGuidedArrowPacket;
            guidedArrowGuardMatched = true;
        } else if (context) {
            context->LogWarn("[ControllerQoL] Guided Arrow selected-skill guard mismatch; projection disabled, other controller features retained.");
        }
        const bool installed = QolCompat::ActionHooks::Install(
            context, nullptr, projector);
        if (installed && guidedArrowGuardMatched && context)
            context->LogInfo("[ControllerQoL] Guided Arrow controller projection enabled at 20 tiles.");
        return installed;
    }
    static void UninstallHooks(const D2RL::PluginContext*) noexcept {
        s_PickupCallsActive.store(false);
        QolCompat::ActionHooks::Shutdown();
        s_GetSelectedSkill = nullptr;
    }

}

static bool ClientGroundHeld() noexcept;

namespace PlacardOverlay {
    constexpr std::uintptr_t BuildGroundItemTooltipRva = 0xCBEB0;
    typedef int64_t (__fastcall *BuildGroundItemTooltipFn)(void* unit, void* textBuffer, size_t bufferSize, void* colorCode);
    inline BuildGroundItemTooltipFn s_OriginalBuildGroundItemTooltip = nullptr;
    inline std::atomic<uint32_t> s_HookCallCount{0};
    inline bool s_Active=false; // guarded by s_PlacardMutex

    struct PlacardTrack {
        void* textBuffer = nullptr;
        size_t bufferSize = 0;
        bool isWide = true;
        uint32_t guid = 0;
        ULONGLONG lastSeenTick = 0;
        QolPlacardText::Lease lease{};
    };

    inline std::recursive_mutex s_PlacardMutex;
    inline std::unordered_map<uint32_t, PlacardTrack> s_TrackedPlacards;

    static bool HasVisiblePlacardText(const void* buffer, size_t bufferSize) noexcept {
        if (!buffer || bufferSize < 2) return false;
        __try {
            const auto* raw = reinterpret_cast<const uint8_t*>(buffer);
            if (raw[0] != 0 && raw[1] == 0) {
                // Wide UTF-16
                const auto* w = reinterpret_cast<const wchar_t*>(buffer);
                for(size_t i=0;i<bufferSize && w[i];++i) {
                    if(w[i]==0xFF && i+2<bufferSize && w[i+1]==L'c' && w[i+2]) {i+=2;continue;}
                    if(w[i]>32) return true;
                }
                return false;
            } else {
                // Narrow / UTF-8
                const auto* c = reinterpret_cast<const char*>(buffer);
                for(size_t i=0;i<bufferSize && c[i];++i) {
                    if(static_cast<unsigned char>(c[i])==0xFF && i+2<bufferSize && c[i+1]=='c' && c[i+2]) {i+=2;continue;}
                    if(static_cast<unsigned char>(c[i])>32) return true;
                }
                return false;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline void RegisterPlacard(uint32_t guid, void* textBuffer, size_t bufferSize, bool isWide) {
        if (guid == 0 || !textBuffer || bufferSize < 8) return;
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        for(auto it=s_TrackedPlacards.begin();it!=s_TrackedPlacards.end();) {
            if(it->first!=guid && it->second.textBuffer==textBuffer) {
                QolClientLoot::Track(it->first,false);
                it=s_TrackedPlacards.erase(it);
            }
            else ++it;
        }
        s_TrackedPlacards[guid] = PlacardTrack{
            .textBuffer = textBuffer,
            .bufferSize = bufferSize,
            .isWide = isWide,
            .guid = guid,
            .lastSeenTick = GetTickCount64()
        };
    }

    inline void UnregisterPlacard(uint32_t guid) {
        if (guid == 0) return;
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        QolClientLoot::Track(guid,false);
        s_TrackedPlacards.erase(guid);
    }

    inline bool IsPlacardActive(uint32_t guid) {
        if (guid == 0) return false;
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        auto it = s_TrackedPlacards.find(guid);
        if (it == s_TrackedPlacards.end()) return false;
        return (GetTickCount64() - it->second.lastSeenTick < 1500);
    }

    inline size_t GetActivePlacardCount() {
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        const ULONGLONG now = GetTickCount64();
        size_t count = 0;
        for (const auto& [g, track] : s_TrackedPlacards) {
            if (now - track.lastSeenTick < 1500) {
                ++count;
            }
        }
        return count;
    }

    static bool StripPlacardBuffer(PlacardTrack& track) noexcept {
        __try {return track.lease.Restore(track.textBuffer,track.bufferSize*(track.isWide?sizeof(wchar_t):1));}
        __except(EXCEPTION_EXECUTE_HANDLER) {track.lease.active=false;return false;}
    }
    static bool ApplyPlacardBuffer(PlacardTrack& track,const wchar_t* pfxW,const char* pfxA,size_t length) noexcept {
        __try {
            return track.isWide
                ? track.lease.Apply(static_cast<wchar_t*>(track.textBuffer),track.bufferSize,pfxW,length)
                : track.lease.Apply(static_cast<char*>(track.textBuffer),track.bufferSize,pfxA,length);
        } __except(EXCEPTION_EXECUTE_HANDLER) {track.lease.active=false;return false;}
    }

    void ForgetSession() noexcept {
        std::lock_guard lock(s_PlacardMutex);
        s_TrackedPlacards.clear(); // Do not dereference expired UI buffers.
    }

    inline void ClearPlacardOverlays() {
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        const ULONGLONG now = GetTickCount64();
        for (auto it = s_TrackedPlacards.begin(); it != s_TrackedPlacards.end(); ) {
            PlacardTrack& track = it->second;
            if (!track.textBuffer || track.bufferSize < 8 || (now - track.lastSeenTick > 3000)) {
                it = s_TrackedPlacards.erase(it);
                continue;
            }

            if (!StripPlacardBuffer(track)) {
                it = s_TrackedPlacards.erase(it);
            } else {
                ++it;
            }
        }
    }

    inline void ApplyPlacardOverlays() {
        ClearPlacardOverlays();

        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        for (size_t i = 0; i < GroundLoot::NUM_STICKY_SLOTS; ++i) {
            const uint32_t guid = QolClientLoot::Slot(static_cast<unsigned>(i)).guid;
            if (guid == 0) continue;

            auto it = s_TrackedPlacards.find(guid);
            if (it == s_TrackedPlacards.end()) continue;

            PlacardTrack& track = it->second;
            if (!track.textBuffer || track.bufferSize < 8) continue;

            ApplyPlacardBuffer(track,
                GroundLoot::s_SlotPrefixes[i].pfxW,
                GroundLoot::s_SlotPrefixes[i].pfxA,
                GroundLoot::s_SlotPrefixes[i].len);
        }
    }

    static uint32_t ReadPlacardGuid(void* unit) noexcept {
        __try {return unit ? static_cast<const uint32_t*>(unit)[2] : 0;}
        __except(EXCEPTION_EXECUTE_HANDLER) {return 0;}
    }
    inline int64_t __fastcall HookBuildGroundItemTooltip(void* unit, void* textBuffer, size_t bufferSize, void* colorCode) {
        ++s_HookCallCount;
        int64_t result = 0;
        if (s_OriginalBuildGroundItemTooltip) {
            result = s_OriginalBuildGroundItemTooltip(unit, textBuffer, bufferSize, colorCode);
        }

        std::lock_guard<std::recursive_mutex> processing(s_PlacardMutex);
        if (!s_Active || !unit || !textBuffer || bufferSize < 8) return result;

        const uint32_t guid=ReadPlacardGuid(unit);

        if (guid == 0) return result;

        if (!HasVisiblePlacardText(textBuffer, bufferSize)) {
            QolClientLoot::Track(guid,false);
            UnregisterPlacard(guid);
            return result;
        }

        const uint8_t* rawBytes = reinterpret_cast<const uint8_t*>(textBuffer);
        bool isWide = (rawBytes[0] != 0 && rawBytes[1] == 0);

        RegisterPlacard(guid, textBuffer, bufferSize, isWide);
        QolClientLoot::Track(guid,true);

        // If ground pickup modifier is currently held down, apply the prefix immediately
        if (Probe::GroundShortcutsEnabled(g_Settings.enabled,g_Settings.groundPickup) &&
            ControllerQoL::IsGroundPickupActive(g_Settings.groundPickupButton)) {
            for (size_t i = 0; i < GroundLoot::NUM_STICKY_SLOTS; ++i) {
                if (QolClientLoot::SlotFor(guid)==static_cast<int>(i)) {
                    ApplyPlacardBuffer(s_TrackedPlacards.at(guid),
                        GroundLoot::s_SlotPrefixes[i].pfxW,
                        GroundLoot::s_SlotPrefixes[i].pfxA,
                        GroundLoot::s_SlotPrefixes[i].len);
                    break;
                }
            }
        }

        return result;
    }

    bool Install(const D2RL::PluginContext* context) noexcept {
        using namespace QolPlacardCall;
        if(!context->CheckExpectedBytes(WitnessRva,Witness,sizeof(Witness))) return false;
        HMODULE pinned{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&HookBuildGroundItemTooltip),&pinned)) return false;
        SYSTEM_INFO info{};GetSystemInfo(&info);
        const uintptr_t step=info.dwAllocationGranularity;
        const uintptr_t aligned=(context->exeBase+Site)&~(step-1);
        unsigned char* relay=nullptr;
        for(uintptr_t offset=step;offset<0x40000000 && !relay;offset+=step)
            relay=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(aligned+offset),64,
                MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        if(!relay) return false;
        relay[0]=0xff;relay[1]=0x25;std::memset(relay+2,0,4);
        const auto destination=&HookBuildGroundItemTooltip;std::memcpy(relay+6,&destination,8);
        unsigned char replacement[5]{};DWORD old{};
        if(!QolGlyphCalls::Encode(context->exeBase+Site,reinterpret_cast<uintptr_t>(relay),replacement) ||
            !VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old)) {
            VirtualFree(relay,0,MEM_RELEASE);return false;
        }
        FlushInstructionCache(GetCurrentProcess(),relay,64);
        s_OriginalBuildGroundItemTooltip=reinterpret_cast<BuildGroundItemTooltipFn>(context->exeBase+Target);
        // Retain relay after attempted publication, including ambiguous failures.
        // SDK owns patch cleanup; the pinned wrapper becomes passthrough on shutdown.
        const bool installed=context->PatchBytes(Site,Expected,5,replacement,5);
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        s_Active=installed;return installed;
    }
    void Shutdown() noexcept {
        std::lock_guard<std::recursive_mutex> lock(s_PlacardMutex);
        s_Active=false;s_TrackedPlacards.clear();
    }
}


struct PickupTaskArgs {
    QolClientLoot::Request clientRequest{};
};

static QolTasks::Payloads<PickupTaskArgs> s_PickupRequests;
namespace {
static void CancelOwnedRequests() noexcept {
    s_MoveRequests.Reset();s_CubeRequests.Reset();s_PickupRequests.Reset();
}
}

static bool ClientGroundHeld() noexcept {
    return g_PollingRunning.load() && Probe::GroundShortcutsEnabled(g_Settings.enabled,g_Settings.groundPickup) &&
        ControllerQoL::IsControllerUiActive() && QolNavigation::GroundInputActive() &&
        ControllerQoL::IsGroundPickupActive(g_Settings.groundPickupButton);
}
static void __cdecl RefreshClientGroundTask(const D2RL::PluginContext*,void* user) noexcept {
    if(QolClientLoot::Epoch()==reinterpret_cast<uintptr_t>(user) && g_PollingRunning.load() && QolClientLoot::Active()) {
        const bool held=ClientGroundHeld();
        if(QolClientLoot::Refresh(std::max(g_Settings.groundPickupDistance,10U),held)) {
            PlacardOverlay::ClearPlacardOverlays();
            if(held)PlacardOverlay::ApplyPlacardOverlays();
            QolNavigation::RefreshGroundLabels();
        }
    }
    auto token=static_cast<uint64_t>(reinterpret_cast<uintptr_t>(user));
    (void)s_ClientLootRefreshToken.compare_exchange_strong(token,0);
}
static void QueueClientGroundRefresh(const D2RL::PluginContext* context) noexcept {
    if(!g_Threads || !g_Threads->runOnUiThread)return;
    const auto epoch=QolClientLoot::Epoch();uint64_t empty=0;
    if(!s_ClientLootRefreshToken.compare_exchange_strong(empty,epoch))return;
    if(g_Threads->runOnUiThread(context,RefreshClientGroundTask,reinterpret_cast<void*>(static_cast<uintptr_t>(epoch)))!=D2RL::Threads::Result::Success) {
        auto owned=epoch;(void)s_ClientLootRefreshToken.compare_exchange_strong(owned,0);
    }
}
static void __cdecl PickupClientGroundTask(const D2RL::PluginContext*,void* userData) noexcept {
    auto args=s_PickupRequests.Take(reinterpret_cast<uintptr_t>(userData));
    if(args) {
        QolClientLoot::Submit(args->clientRequest,std::max(g_Settings.groundPickupDistance,10U),ClientGroundHeld());
    }
}

static void QueueGroundPickup(const D2RL::PluginContext* context,unsigned slot) noexcept {
    if(!g_Threads || !g_Threads->runOnUiThread || !s_IdentifySessionEvents || !ClientGroundHeld())return;
    const auto request=QolClientLoot::Capture(slot);
    if(!request.item.guid)return;
    const auto token=s_PickupRequests.Put(std::make_unique<PickupTaskArgs>(PickupTaskArgs{request}));
    if(token && g_Threads->runOnUiThread(context,PickupClientGroundTask,reinterpret_cast<void*>(token))!=D2RL::Threads::Result::Success)
        (void)s_PickupRequests.Take(token);
}

static void ScanDecryptedMemory(const D2RL::PluginContext* context, uint8_t* base) noexcept {
    if (!context || !base) return;

    __try {
        const uintptr_t targetRva = 0x178BF68;
        const uintptr_t targetVa = reinterpret_cast<uintptr_t>(base) + targetRva;
        const uint8_t* textStart = base + 0x1000;
        const size_t textLen = 0x1500000;

        int ripMatches = 0;
        for (size_t i = 0; i < textLen - 4 && ripMatches < 8; ++i) {
            int32_t disp = *reinterpret_cast<const int32_t*>(textStart + i);
            uintptr_t nextRip = reinterpret_cast<uintptr_t>(textStart + i + 4);
            if (nextRip + disp == targetVa) {
                ripMatches++;
                uintptr_t instrRva = (reinterpret_cast<uintptr_t>(textStart + i) - reinterpret_cast<uintptr_t>(base)) - 3;
                char ripMsg[256];
                std::snprintf(ripMsg, sizeof(ripMsg),
                    "[ControllerQoL] Decrypted memory: Found RIP ref to CfgShowItems at RVA 0x%llX (bytes: %02X %02X %02X %02X %02X %02X %02X)",
                    static_cast<unsigned long long>(instrRva),
                    textStart[i - 3], textStart[i - 2], textStart[i - 1],
                    textStart[i], textStart[i + 1], textStart[i + 2], textStart[i + 3]);
                context->LogInfo(ripMsg);
            }
        }

        int vaMatches = 0;
        for (size_t i = 0; i < textLen - 8 && vaMatches < 8; ++i) {
            uint64_t val = *reinterpret_cast<const uint64_t*>(textStart + i);
            if (val == targetVa) {
                vaMatches++;
                uintptr_t matchRva = reinterpret_cast<uintptr_t>(textStart + i) - reinterpret_cast<uintptr_t>(base);
                char vaMsg[256];
                std::snprintf(vaMsg, sizeof(vaMsg),
                    "[ControllerQoL] Decrypted memory: Found 8-byte VA pointer to CfgShowItems at RVA 0x%llX",
                    static_cast<unsigned long long>(matchRva));
                context->LogInfo(vaMsg);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        context->LogWarn("[ControllerQoL] Exception during decrypted memory scan for CfgShowItems.");
    }
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &ControllerQoLPluginInfo;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) {
        return false;
    }

    g_PluginContext = context;
    context->LogInfo("[ControllerQoL] Initializing Controller QoL plugin (PluginSDK ABI 4)...");

    // Load config from TOML
    LoadConfiguration(context);
    ControllerQoL::SetInputFilteringEnabled(g_Settings.enabled);
    ControllerQoL::SetActiveModifier(g_Settings.modifier,g_Settings.triggerThreshold);

    // Initialize controller input subsystem
    ControllerQoL::InitControllerInput();
    // Resolve admission before installing input callbacks or starting workers.
    (void)ControllerQoL::IsNativeAltModifierActive();
    context->LogInfo("[QOL/Input] Private core bridge admission completed; unsupported cores use physical XInput.");

    // Query core services
    if (context->QueryService(&g_Interactions) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasItemInteractionServiceField(g_Interactions, D2RL::ItemInteractionServiceRequiredSize)) {
        context->LogWarn("[ControllerQoL] ItemInteractionService unavailable.");
    }

    if (context->QueryService(&g_Items) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasItemServiceField(g_Items, D2RL::ItemServiceRequiredSize)) {
        context->LogWarn("[ControllerQoL] ItemService unavailable.");
    }

    context->LogInfo(QolSharedSdk::Supported(g_Items)
        ? "[QOL/SharedSDK] SharedStashWrite available; normal LB+X transfers use SDK transactions."
        : "[QOL/SharedSDK] SharedStashWrite unavailable; reviewed native transfer path retained.");

    if (context->QueryService(&g_Inventory) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasInventoryServiceField(g_Inventory, D2RL::InventoryServiceRequiredSize)) {
        context->LogWarn("[ControllerQoL] InventoryService unavailable.");
    }

    if (context->QueryService(&g_Threads) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(g_Threads, D2RL::ThreadServiceRequiredSize)) {
        context->LogWarn("[ControllerQoL] ThreadService unavailable.");
    }

    if (context->QueryService(&g_Widgets) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasWidgetServiceField(g_Widgets, D2RL::WidgetServiceRequiredSize)) {
        context->LogWarn("[ControllerQoL] WidgetService unavailable.");
    } else {
        context->LogInfo("[ControllerQoL] WidgetService acquired successfully.");
    }

    auto inputRes = context->QueryService(&g_Input);
    auto seRes = context->QueryService(&g_SharedEvents);
    char srvMsg[256];
    std::snprintf(srvMsg, sizeof(srvMsg),
        "[ControllerQoL] QueryService: Input=%d (ptr=%p), SharedEvents=%d (ptr=%p), Widgets=%p",
        static_cast<int>(inputRes), static_cast<const void*>(g_Input),
        static_cast<int>(seRes), static_cast<const void*>(g_SharedEvents),
        static_cast<const void*>(g_Widgets));
    context->LogInfo(srvMsg);

    // Register Item Interaction Listener
    if (g_Interactions) {
        const D2RL::ItemInteractions::ItemInteractionListener listener{
            .structSize = D2RL::ItemInteractions::ItemInteractionListenerSize,
            .flags      = 0,
            .priority   = 100,
            .reserved   = 0,
            .callback   = OnItemInteraction,
            .userData   = nullptr,
        };

        const auto regRes = g_Interactions->registerListener(context, &listener, &g_ListenerHandle);
        if (regRes == D2RL::ItemInteractions::Result::Success) {
            context->LogInfo("[ControllerQoL] Registered item interaction listener successfully.");
        } else {
            char msg[128];
            std::snprintf(msg, sizeof(msg), "[ControllerQoL] Failed to register item interaction listener (%u).",
                static_cast<uint32_t>(regRes));
            context->LogError(msg);
        }
    }

    // Register ActionFooter Tooltip Listener for Quick Identify
    if (g_SharedEvents) {
        const bool hasField = D2RL::HasSharedEventServiceField(g_SharedEvents, D2RL::SharedEventServiceRequiredSize);
        std::snprintf(srvMsg, sizeof(srvMsg),
            "[ControllerQoL] SharedEventService: ver=%u, size=%u, reqSize=%u, hasField=%d",
            g_SharedEvents->serviceVersion, g_SharedEvents->serviceSize,
            D2RL::SharedEventServiceRequiredSize, static_cast<int>(hasField));
        context->LogInfo(srvMsg);

        if (hasField) {
            const D2RL::SharedEvents::ItemTooltipListener actionTooltip{
                .structSize = D2RL::SharedEvents::ItemTooltipListenerSize,
                .flags      = 0,
                .priority   = 100,
                .slot       = 0,
                .region     = D2RL::SharedEvents::ItemTooltipRegion::ActionFooter,
                .position   = D2RL::SharedEvents::ItemTooltipPosition::Bottom,
                .anchor     = D2RL::SharedEvents::ItemTooltipAnchor::None,
                .fallback   = D2RL::SharedEvents::ItemTooltipFallback::Omit,
                .callback   = OnItemTooltipCallback,
                .userData   = nullptr,
            };
            const auto regRes = g_SharedEvents->registerItemTooltipListener(context, &actionTooltip, &g_TooltipActionHandle);
            std::snprintf(srvMsg, sizeof(srvMsg),
                "[ControllerQoL] registerItemTooltipListener (ActionFooter) res=%u, handle=%llu",
                static_cast<uint32_t>(regRes), static_cast<unsigned long long>(g_TooltipActionHandle));
            context->LogInfo(srvMsg);
        }
    } else {
        context->LogWarn("[ControllerQoL] SharedEventService is null!");
    }

    // Initialize Ground Loot Native Subsystem
    GroundLoot::s_Base = reinterpret_cast<uint8_t*>(context->exeBase);
    // Filtered A-button blocking is independent of the optional direct-loot
    // shortcuts, but both rely on this guarded native/placard observation path.
    if (g_Settings.enabled && GroundLoot::s_Base && Probe::GroundLootHooksRequired(
            g_Settings.groundPickup, g_Settings.blockFilteredPickup)) {
        const bool match = context->CheckExpectedBytes(
            GroundLoot::GetItemCodeRva,
            GroundLoot::GetItemCodeExpected,
            sizeof(GroundLoot::GetItemCodeExpected));

        char fpMsg[128];
        std::snprintf(fpMsg, sizeof(fpMsg),
            "[ControllerQoL] Native Ground Loot fingerprint match: %d (exeBase=%p)",
            static_cast<int>(match), reinterpret_cast<void*>(context->exeBase));
        context->LogInfo(fpMsg);

        if (match) {
            GroundLoot::s_FingerprintValid = true;
            GroundLoot::s_Pickup = reinterpret_cast<GroundLoot::PickupFn>(GroundLoot::s_Base + GroundLoot::PickupRva);

            if (g_Settings.groundPickup) {
                const bool installed = GroundLoot::InstallHooks(context, GroundLoot::s_Base);
                if (installed) {
                    context->LogInfo("[ControllerQoL] Local Guided Arrow packet hook installed; loot requests use the local UI path.");
                } else {
                    context->LogWarn("[ControllerQoL] Guided Arrow packet-hook admission failed; native projection retained.");
                }
            } else {
                context->LogInfo("[QOL/Loot] Direct ground shortcuts disabled; filtered A-button guard remains eligible.");
            }

            // Diagnostic Ground Label Placard Engine match
            constexpr std::uintptr_t LabelCapRva = 0x1516EBE;
            constexpr uint8_t ExpectedLabelCap[4] = { 0x48, 0x83, 0xF8, 0x20 };
            uint8_t labelCapBytes[sizeof(ExpectedLabelCap)]{};
            SIZE_T labelCapRead{};
            const bool labelCapReadable = ReadProcessMemory(GetCurrentProcess(),
                GroundLoot::s_Base + LabelCapRva, labelCapBytes, sizeof(labelCapBytes), &labelCapRead)
                && labelCapRead == sizeof(labelCapBytes);
            const bool labelCapMatch = labelCapReadable &&
                std::memcmp(labelCapBytes, ExpectedLabelCap, sizeof(ExpectedLabelCap)) == 0;

            constexpr std::uintptr_t LayoutLimitRva = 0x1519AF9;
            constexpr uint8_t ExpectedLayoutLimit[6] = { 0x49, 0x83, 0x7C, 0x24, 0x10, 0x20 };
            uint8_t layoutBytes[sizeof(ExpectedLayoutLimit)]{};
            SIZE_T layoutRead{};
            const bool layoutReadable = ReadProcessMemory(GetCurrentProcess(),
                GroundLoot::s_Base + LayoutLimitRva, layoutBytes, sizeof(layoutBytes), &layoutRead)
                && layoutRead == sizeof(layoutBytes);
            const bool layoutMatch = layoutReadable &&
                std::memcmp(layoutBytes, ExpectedLayoutLimit, sizeof(ExpectedLayoutLimit)) == 0;

            char lblLog[256];
            std::snprintf(lblLog, sizeof(lblLog),
                "[ControllerQoL] Informational label limits: LabelCap(0x1516EBE)=%s, LayoutLimit(0x1519AF9)=%s; no QOL patch or admission depends on these sites.",
                !labelCapReadable ? "unreadable" : labelCapMatch ? "original" : "modified",
                !layoutReadable ? "unreadable" : layoutMatch ? "original" : "modified");
            context->LogInfo(lblLog);

            if (GroundLoot::s_Base) {
                char dumpBuf[256];
                int off = std::snprintf(dumpBuf, sizeof(dumpBuf), "[ControllerQoL] LabelCap asm [0x1516EB0]: ");
                const uint8_t* pCode = GroundLoot::s_Base + 0x1516EB0;
                for (int i = 0; i < 32 && (off + 4 < sizeof(dumpBuf)); ++i) {
                    off += std::snprintf(dumpBuf + off, sizeof(dumpBuf) - off, "%02X ", pCode[i]);
                }
                context->LogInfo(dumpBuf);

                off = std::snprintf(dumpBuf, sizeof(dumpBuf), "[ControllerQoL] LayoutLimit asm [0x1519AE0]: ");
                pCode = GroundLoot::s_Base + 0x1519AE0;
                for (int i = 0; i < 36 && (off + 4 < sizeof(dumpBuf)); ++i) {
                    off += std::snprintf(dumpBuf + off, sizeof(dumpBuf) - off, "%02X ", pCode[i]);
                }
                context->LogInfo(dumpBuf);

                off = std::snprintf(dumpBuf, sizeof(dumpBuf), "[ControllerQoL] GetItemCode asm [0x36EF50]: ");
                pCode = GroundLoot::s_Base + 0x36EF50;
                for (int i = 0; i < 40 && (off + 4 < sizeof(dumpBuf)); ++i) {
                    off += std::snprintf(dumpBuf + off, sizeof(dumpBuf) - off, "%02X ", pCode[i]);
                }
                context->LogInfo(dumpBuf);
                // Scan decrypted memory for RIP references and pointers to CfgShowItems (RVA 0x178BF68)
                ScanDecryptedMemory(context, GroundLoot::s_Base);
            }

            const bool hookOk=PlacardOverlay::Install(context);
            context->LogInfo(hookOk
                ? "[QOL/Placards] Scoped label call installed at game+0x1FAA18; shared builder+0xCBEB0 untouched."
                : "[QOL/Placards] Label caller guard/patch failed; controller label hints unavailable.");

            const bool pickupCallsOk = GroundLoot::InstallPickupCalls(context);
            context->LogInfo(pickupCallsOk
                ? "[QOL/Loot] Seven guarded pickup CALL patches installed; shared game+0x471950 entry left untouched."
                : "[QOL/Loot] Pickup CALL-site guard mismatch/publication failure; native pickup behavior preserved.");
        }
    }

    QolIdentifyStat::Initialize(context);
    QolNativeIdentify::Initialize(context,g_Items,g_Threads);
    QolClientLoot::Initialize(context,&GroundLoot::GetItemCodeRank,&GroundLoot::BlockClientPickup,&ClientGroundHeld);
    InitializeIdentifyQueue(context);
    if(!QolClientTransfer::Initialize(context))context->LogWarn("[QOL/ClientTransfer] Profile/services unavailable; remote transfers disabled.");
    (void)QolCustomPage::Initialize(context);
    if (!QolMaterials::Initialize(context))
        context->LogWarn("[QOL/Materials] Advanced withdrawals unavailable; inspect native admission diagnostics.");
    if (!QolBelt::Initialize(context))
        context->LogWarn("[QOL/Belt] Potion shortcuts disabled: required SDK services or native contract unavailable.");

    // Register controller shortcut callbacks and hooks
    ControllerQoL::SetTriggerPassThroughPredicate(+[]() noexcept {
        return TestUiMode(UI_MODE_STASH) && QolNavigation::SharedPageInputActive();
    });
    QolGlyphs::SetBulkHeaderContext(+[]() noexcept -> const char* {
        if(!g_Settings.enabled || !Probe::BatchFeatureEnabled(g_Settings.quickMove,g_Settings.quickDeposit) || !TestUiMode(UI_MODE_STASH))return nullptr;
        const auto* mode=g_Settings.modifier;
        if(_stricmp(mode,"bumper")==0 || _stricmp(mode,"lb")==0 || _stricmp(mode,"l1")==0)return "LB";
        if(_stricmp(mode,"l3")==0)return "L3";
        if(_stricmp(mode,"[")==0 || _stricmp(mode,"bracket")==0 || _stricmp(mode,"leftbracket")==0 || _stricmp(mode,"l4")==0)return "[";
        return "LT";
    });
    QolBulkStashFeedback::Initialize(context,+[]() noexcept {
        return g_Settings.enabled && Probe::BatchFeatureEnabled(g_Settings.quickMove,g_Settings.quickDeposit) && ControllerQoL::IsControllerUiActive() &&
            TestUiMode(UI_MODE_STASH) && !QolBelt::Busy() && !QolMaterials::Busy() && !QolNativeIdentify::Busy() && !QolClientTransfer::Busy();
    });
    ControllerQoL::SetBulkStashCallback(QolBulkStash::Request);
    ControllerQoL::SetAdvancedBeltCallback(TriggerAdvancedSingleBelt);
    ControllerQoL::SetQuickMoveCallback(TriggerQuickMoveOnFocusedItem);
    ControllerQoL::SetQuickMoveCubeCallback(TriggerQuickMoveToCubeOnFocusedItem);
    ControllerQoL::SetAutoFillBeltCallback(TriggerAutoFillBelt);
    ControllerQoL::SetVendorContextPredicate(nullptr); // Native LB+X sale; no synthetic X hold.
    const bool nativeInputOk=ControllerQoL::InstallNativeInputHook(context,g_Threads);
    const bool hooksOk = nativeInputOk || ControllerQoL::InstallXInputHooks();
    char hookLog[128];
    std::snprintf(hookLog, sizeof(hookLog), "[ControllerQoL] Controller input hooks installed: %s", hooksOk ? "SUCCESS" : "FAILED");
    context->LogInfo(hookLog);
    if(!nativeInputOk) context->LogInfo(ControllerQoL::GetXInputHookReport());

    ControllerQoL::SetGroundPickupPredicate([]() noexcept {
        return Probe::GroundShortcutsEnabled(g_Settings.enabled,g_Settings.groundPickup) &&
            QolClientLoot::Ready() && ControllerQoL::NativeInputInstalled() && ControllerQoL::IsControllerUiActive() &&
            QolNavigation::GroundInputActive() && ControllerQoL::IsGroundPickupActive(g_Settings.groundPickupButton);
    });
    // Start background controller polling thread for ground item pickup & inspection
    if (g_Settings.enabled) {
        g_PollingRunning.store(true);
        g_PollingThread = std::thread([context]() {
            static ULONGLONG s_LastPickupTick = 0;
            static bool s_LastMod = false;
            static bool s_LastA = false;
            static bool s_LastX = false;
            static bool s_LastY = false;
            static bool s_LastB = false;
            static bool s_LastNativeAlt = false;
            static bool s_FirstNativePoll = true;

            while (g_PollingRunning.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 FPS
                if (!g_Settings.enabled) continue;

                QolBulkStashFeedback::Refresh();
                ControllerQoL::PumpNativeInput();
                QolNavigation::PumpLabels(context,g_Threads);
                if(Probe::GroundShortcutsEnabled(g_Settings.enabled,g_Settings.groundPickup) && QolClientLoot::Ready())
                    (void)QolClientLoot::Activate();
                if(QolClientLoot::Active()) {
                    static ULONGLONG lastClientLootRefresh=0;
                    const auto now=GetTickCount64();
                    if(now-lastClientLootRefresh>=100) {
                        lastClientLootRefresh=now;
                        QueueClientGroundRefresh(context);
                    }
                }
                if(g_Settings.debugLogging) {
                    static ULONGLONG lastInputReport=0;
                    const auto now=GetTickCount64();
                    if(now-lastInputReport>=5000) {
                        lastInputReport=now;
                        const auto input=ControllerQoL::ReadControllerInput();
                        uint8_t lt{},rt{}; uint16_t buttons{}; uint64_t age{};
                        ControllerQoL::GetCachedControllerState(lt,rt,buttons,age);
                        char message[256];
                        std::snprintf(message,sizeof(message),"[QOL/RemoteTrace] nativeInstalled=%d controllerUi=%d valid=%d providers=0x%X buttons=0x%04X cached=0x%04X age=%llu ms modifier=%d",
                            ControllerQoL::NativeInputInstalled(),ControllerQoL::IsControllerUiActive(),input.valid,input.providers,input.buttons,buttons,
                            static_cast<unsigned long long>(age),ControllerQoL::IsControllerModifierActive(g_Settings.modifier,g_Settings.triggerThreshold));
                        context->LogInfo(message);
                    }
                }
                QolBelt::Pump(context);
                QolMaterials::Pump(context);

                // Live Native Bridge validation monitor: logs whenever Native L2 state transitions
                const bool currentNativeAlt = ControllerQoL::IsNativeAltModifierActive();
                if (currentNativeAlt != s_LastNativeAlt) {
                    s_LastNativeAlt = currentNativeAlt;
                    if (g_Settings.debugLogging) {
                        const auto diag = ControllerQoL::QueryNativeBridgeDiagnostics();
                        char valBuf[320];
                        std::snprintf(valBuf, sizeof(valBuf),
                            "[ControllerQoL] [BRIDGE-VALIDATION] State Transition: Native L2 (ControllerAltHold) is now %s! (mgr=%d, mode=%u, altHold=(res=%d,st=%u), isAltActive=%d, steamCtrls=%d)",
                            currentNativeAlt ? "HELD / ACTIVE" : "RELEASED / INACTIVE",
                            diag.mgrFound ? 1 : 0, diag.controllerMode,
                            diag.altHoldRes, diag.altHoldState, diag.isAltActive ? 1 : 0,
                            diag.steamControllers);
                        if (context) {
                            context->LogInfo(valBuf);
                        }
                    }
                }

                // The active input provider owns each physical shortcut press.

                if ((GetTickCount64() - g_LastInventoryInteractionTick.load()) < 500) {
                    continue;
                }

                if (!Probe::GroundShortcutsEnabled(g_Settings.enabled,g_Settings.groundPickup)) {
                    continue;
                }

                const bool modBtn = ControllerQoL::IsGroundPickupActive(g_Settings.groundPickupButton);
                const bool lt = ControllerQoL::IsLeftTriggerPressed(30);
                const bool rb = ControllerQoL::IsRightBumperPressed();
                const bool rt = ControllerQoL::IsRightTriggerPressed(30);
                const bool btnA = ControllerQoL::IsButtonAPressed();
                const bool btnX = ControllerQoL::IsButtonXPressed();
                const bool btnY = ControllerQoL::IsButtonYPressed();
                const bool btnB = ControllerQoL::IsButtonBPressed();

                static ULONGLONG s_ModPressTick = 0;
                static bool s_ModUsedWithFace = false;
                static bool s_LastRb = false;
                static bool s_LastRt = false;
                static bool s_LastLt = false;

                const bool modDown = modBtn && !s_LastMod;
                const bool modUp = !modBtn && s_LastMod;
                const bool aDown = btnA && !s_LastA;
                const bool xDown = btnX && !s_LastX;
                const bool yDown = btnY && !s_LastY;
                const bool bDown = btnB && !s_LastB;
                const bool rbDown = rb && !s_LastRb;
                const bool rtDown = rt && !s_LastRt;
                const bool ltDown = lt && !s_LastLt;

                s_LastMod = modBtn;
                s_LastA = btnA;
                s_LastX = btnX;
                s_LastY = btnY;
                s_LastB = btnB;
                s_LastRb = rb;
                s_LastRt = rt;
                s_LastLt = lt;

                if (!QolNavigation::GroundInputActive()) {
                    s_LastMod=false; // Refresh mappings when returning to the world with LB held.
                    s_ModUsedWithFace=false;
                    continue; // Track edges above, but menu bumpers must not queue world-loot work.
                }
                const ULONGLONG now = GetTickCount64();

                if ((modDown || modUp) && g_Settings.debugLogging) {
                    auto physical=ControllerQoL::ReadControllerInput();
                    char diagnostic[160];
                    std::snprintf(diagnostic,sizeof(diagnostic),"[QOL/Loot] modifier=%s held=%d providers=0x%X buttons=0x%04X",
                        g_Settings.groundPickupButton,modBtn,physical.providers,physical.buttons);
                    context->LogInfo(diagnostic);
                }
                if (modDown) {
                    s_ModPressTick = now;
                    s_ModUsedWithFace = false;
                    QueueClientGroundRefresh(context);
                }

                // If modifier is held, it acts as the modifier for shortcuts (A, X, Y, B, R1, R2, L2)
                if (modBtn && (aDown || xDown || yDown || bDown || rbDown || rtDown || ltDown ||
                               btnA || btnX || btnY || btnB || rb || rt || lt)) {
                    s_ModUsedWithFace = true;
                }

                int targetSlot = -1;

                if (modBtn) {
                    if (aDown) targetSlot = 0;       // Button A  -> Slot 0
                    else if (xDown) targetSlot = 1;  // Button X  -> Slot 1
                    else if (yDown) targetSlot = 2;  // Button Y  -> Slot 2
                    else if (bDown) targetSlot = 3;  // Button B  -> Slot 3
                    else if (rbDown) targetSlot = 4; // Button R1 -> Slot 4
                    else if (rtDown) targetSlot = 5; // Button R2 -> Slot 5
                    else if (ltDown) targetSlot = 6; // Button L2 -> Slot 6
                }

                if (modUp) {
                    // Immediately clear placard overlays upon releasing modifier (never auto-pickup)
                    if (g_Threads) {
                        QueueClientGroundRefresh(context);
                    }
                }

                if (targetSlot >= 0 && (now - s_LastPickupTick >= 150)) {
                    s_LastPickupTick = now;
                    if (context) {
                        char trigMsg[128];
                        if (targetSlot == 0xFF) {
                            std::snprintf(trigMsg, sizeof(trigMsg),
                                "[ControllerQoL] Controller input: Ground modifier tap triggered Quick Pickup");
                        } else {
                            std::snprintf(trigMsg, sizeof(trigMsg),
                                "[ControllerQoL] Controller input: triggered pickup for slot [%s] (mod=%s)",
                                (targetSlot < GroundLoot::NUM_STICKY_SLOTS) ? GroundLoot::s_SlotNames[targetSlot] : "?",
                                g_Settings.groundPickupButton);
                        }
                        context->LogInfo(trigMsg);
                    }
                    QueueGroundPickup(context,static_cast<unsigned>(targetSlot));
                }
            }
        });
        context->LogInfo("[ControllerQoL] Started controller ground item pickup & inspection thread (30 FPS).");
    }

    // Register Controls Menu Action
    if (g_Input && D2RL::HasInputServiceField(g_Input, D2RL::InputServiceRequiredSize)) {
        const D2RL::Input::ActionRegistration actionReg{
            .structSize       = D2RL::Input::ActionRegistrationSize,
            .flags            = 0,
            .logicalId        = "controller-qol-modifier",
            .displayName      = "Controller QoL Quick Action",
            .category         = "Controller QoL",
            .defaultPrimary   = { .key = D2RL::Input::Key::None, .modifier = D2RL::Input::Modifier::None },
            .defaultSecondary = { .key = D2RL::Input::Key::None, .modifier = D2RL::Input::Modifier::None },
            .callback         = OnInputAction,
            .userData         = nullptr,
        };
        g_Input->registerAction(context, &actionReg, &g_ActionHandle);
    }

    if (!QolNavigation::Initialize(context, g_Settings.enabled, g_Settings.debugLogging)) {
        context->LogWarn("[QOL] Navigation/label module unavailable; inspect QOL diagnostics. Item features remain loaded.");
    }
    QolPortal::Initialize(context,
        g_Settings.enabled && (g_Settings.prioritizePortals || g_Settings.prioritizeStashBoxes ||
            g_Settings.prioritizeWaypoints || g_Settings.prioritizeShrines || g_Settings.prioritizeChests),
        g_Settings.groundPickup, g_Settings.prioritizePortals, g_Settings.prioritizeStashBoxes,
        g_Settings.prioritizeWaypoints, g_Settings.prioritizeShrines, g_Settings.prioritizeChests,
        g_Settings.debugLogging || g_Settings.portalDiagnostics,
        g_Settings.groundPickupButton, g_Settings.portalPriorityDistance);
    if(!QolAim::Initialize(context,g_Settings.enabled))
        context->LogWarn("[QOL/Aim] Aim unavailable; other QOL features remain loaded.");
    context->LogInfo("[QOL] QOL v1.3.1+rev.78 loaded: controller item features and integrated v0.6 navigation/label hooks.");
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    QolAim::Shutdown();
    g_PollingRunning.store(false);
    if (g_PollingThread.joinable()) {
        g_PollingThread.join();
    }
    ControllerQoL::UninstallXInputHooks(); // Drain input before destroying callback dependencies.
    ControllerQoL::SetGroundPickupPredicate(nullptr);
    QolClientLoot::Shutdown();
    PlacardOverlay::Shutdown();
    QolBulkStashFeedback::Shutdown();
    ControllerQoL::SetBulkStashCallback(nullptr);
    ControllerQoL::SetAdvancedBeltCallback(nullptr);
    QolNativeIdentify::Shutdown();
    IdentifySessionChanged(nullptr,nullptr,nullptr);
    QolClientTransfer::Shutdown();
    QolPortal::Shutdown();
    QolBelt::Shutdown();
    QolMaterials::Shutdown();
    QolCustomPage::Shutdown();
    ControllerQoL::SetAutoFillBeltCallback(nullptr);
    QolNavigation::Shutdown();
    ControllerQoL::SetTriggerPassThroughPredicate(nullptr);
    ControllerQoL::SetQuickMoveCallback(nullptr);
    ControllerQoL::SetQuickMoveCubeCallback(nullptr);
    ControllerQoL::SetVendorContextPredicate(nullptr);


    GroundLoot::UninstallHooks(nullptr);

    g_Interactions = nullptr;
    g_Items = nullptr;
    g_Inventory = nullptr;
    g_Threads = nullptr;
    g_Input = nullptr;
    g_SharedEvents = nullptr;
    g_ListenerHandle = D2RL::ItemInteractions::InvalidHandle;
    g_ActionHandle = D2RL::Input::InvalidHandle;
    g_TooltipActionHandle = D2RL::SharedEvents::InvalidHandle;
}
