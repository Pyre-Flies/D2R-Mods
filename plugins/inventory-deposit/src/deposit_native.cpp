#include "deposit_native.h"
#include "deposit_eligibility.h"
#include "client_transfer.h"
#include "client_transfer_profile.h"
#include "client_request_profile.h"
#include "core_compatibility.h"
#include "materials_actions.h"
#include "materials_signatures.h"
#include "native_d2r.h"
#include "native_identify_profile.h"
#include "native_input_profile.h"
#include "belt_signatures.h"
#include "vendor_signatures.h"
#include "identify_probe_profile.h"
#include "shared_page_signatures.h"
#include "shared_owner_compatibility.h"
#include "storage_focus_profile.h"
#include <mutex>

namespace {
using Info = D2RL::Items::ItemInfo;
using Container = D2RL::Items::ItemContainer;
using Result = QolClientTransfer::BatchDepositResult;

std::mutex nativeMutex;
const D2RL::PluginContext* pluginContext{};
const D2RL::ItemService* itemService{};
const D2RL::InventoryService* inventoryService{};
uintptr_t coreBase{};
bool supported{};
bool faulted{};

template<class T> T At(uintptr_t rva) noexcept {
    return reinterpret_cast<T>(pluginContext->exeBase + rva);
}

void Disable(const char* reason) noexcept {
    supported = false;
    if (!faulted && pluginContext) pluginContext->LogWarn(reason);
    faulted = true;
}

// Mirror client_transfer.cpp::Guard, including the full transfer bodies and
// provider redirection contracts. None of these are new native discoveries.
bool Guard() noexcept {
    if (!pluginContext || !pluginContext->exeBase || !coreBase ||
        reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll")) != coreBase) return false;
    for (const auto& site : QolClientTransfer::Profile::Sites) {
        if (site.size > UINT32_MAX ||
            !pluginContext->CheckExpectedBytes(site.rva, site.bytes, static_cast<uint32_t>(site.size))) return false;
    }
    using namespace QolBelt::Signatures;
    using namespace QolNativeIdentifyProfile;
    const QolClientTransfer::Profile::Site helpers[] = {
        {0x8b2d0, LocalContext, sizeof(LocalContext)},
        {0x9a480, LocalPlayer, sizeof(LocalPlayer)},
        {0x9a5d0, QolVendor::ClientUnitBytes, sizeof(QolVendor::ClientUnitBytes)},
        {0x36cfe0, QolVendor::ItemPageBytes, sizeof(QolVendor::ItemPageBytes)},
        {0x36ef50, Code, sizeof(Code)},
        {0x14f1e0, Cursor, sizeof(Cursor)},
        {0x14f210, Mode, sizeof(Mode)},
        {0x1c7360, QolIdentifyProbe::Blocked, sizeof(QolIdentifyProbe::Blocked)},
        {0xce500, QolMaterials::Signatures::UiMode, sizeof(QolMaterials::Signatures::UiMode)},
        {0x846170, FindPanel, sizeof(FindPanel)},
        {0x23af50, SharedPageNative::SelectedTab, sizeof(SharedPageNative::SelectedTab)},
        {0x2ef880, SharedOwnerIdBytes, sizeof(SharedOwnerIdBytes)},
    };
    for (const auto& site : helpers) {
        if (site.size > UINT32_MAX ||
            !pluginContext->CheckExpectedBytes(site.rva, site.bytes, static_cast<uint32_t>(site.size))) return false;
    }
    return QolShared::ValidateOwner(pluginContext->exeBase) &&
        QolClientRequest::Guard(pluginContext->exeBase, coreBase);
}

bool DepositGuard() noexcept {
    using namespace QolMaterials::Signatures;
    return pluginContext->CheckExpectedBytes(0x15a0b0, QolStorageProfile::DepositEligible, sizeof(QolStorageProfile::DepositEligible)) &&
        pluginContext->CheckExpectedBytes(0x46da50, CurrentOwner, sizeof(CurrentOwner)) &&
        pluginContext->CheckExpectedBytes(0x23b980, PreviousSeason, sizeof(PreviousSeason)) &&
        pluginContext->CheckExpectedBytes(0x1a0780, FinishInteraction, sizeof(FinishInteraction));
}

bool ProfileAvailable() noexcept {
    if (!supported || faulted || !pluginContext) return false;
    if (Guard() && DepositGuard()) return true;
    Disable("[InventoryDeposit] Native profile changed; deposit disabled until plugin reload.");
    return false;
}

bool UiMode(uint32_t mode) noexcept {
    return At<bool(__fastcall*)(uint32_t)>(0xce500)(mode);
}

uint32_t SelectedTabUnsafe() noexcept {
    if (!ProfileAvailable() || !UiMode(0x18)) return UINT32_MAX;
    // Both calls used by the helper are admitted by Guard above. A missing
    // bank panel is a normal unavailable state, not a compatible zero tab.
    const auto tab = D2R::Native::GetActiveStashTabIndex(pluginContext->exeBase);
    return tab <= 4 ? tab : UINT32_MAX;
}

bool AvailableUnsafe(bool requireEmptyCursor) noexcept {
    if (SelectedTabUnsafe() == UINT32_MAX || UiMode(0x0b) ||
        At<bool(__fastcall*)()>(0x23b980)() ||
        (requireEmptyCursor && (At<void*(__fastcall*)()>(0x14f1e0)() ||
        At<int(__fastcall*)()>(0x14f210)() == 5))) return false;
    D2RL::PlayerHandle player{};
    return inventoryService->getLocalPlayer(pluginContext, &player) == D2RL::Inventory::Result::Success &&
        player != D2RL::InvalidPlayerHandle &&
        D2R::Native::GetLocalPlayerUnit(pluginContext->exeBase) != nullptr;
}

bool AvailableGuarded(bool requireEmptyCursor) noexcept {
    __try {
        return AvailableUnsafe(requireEmptyCursor);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Disable("[InventoryDeposit] Native UI preflight fault; deposit disabled until plugin reload.");
        return false;
    }
}

InventoryDepositEligibility::Result InspectEligibilityUnsafe(const Info& live) noexcept {
    using Eligibility = InventoryDepositEligibility::Result;
    // Same admitted client lookup and identity/page checks as BatchDepositUnsafe.
    // This is a synchronous read: no transfer or finish-interaction call.
    auto* item = static_cast<uint32_t*>(At<void*(__fastcall*)(uint32_t, uint32_t)>(0x9a5d0)(live.runtimeId, 4));
    if (!item || item[0] != 4 || item[1] != live.classId || item[2] != live.runtimeId || item[3] != 0 ||
        At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(item) != live.code ||
        At<uint8_t(__fastcall*)(const void*)>(0x36cfe0)(item) != 0) return Eligibility::Unavailable;
    if (At<int(__fastcall*)(const void*)>(0x1c7360)(item)) return Eligibility::Unavailable;
    return At<bool(__fastcall*)(const void*)>(0x15a0b0)(item) ? Eligibility::Eligible : Eligibility::Ineligible;
}

bool HasEligibleItemsGuarded() noexcept {
    __try {
        return AvailableUnsafe(true) &&
            InventoryDepositEligibility::Any(pluginContext, inventoryService, itemService, InspectEligibilityUnsafe) &&
            AvailableUnsafe(true);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Disable("[InventoryDeposit] Native eligibility read fault; deposit disabled until plugin reload.");
        return false;
    }
}

uint32_t SelectedTabGuarded(const D2RL::PluginContext* context) noexcept {
    __try {
        return context == pluginContext ? SelectedTabUnsafe() : UINT32_MAX;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Disable("[InventoryDeposit] Stash selection fault; deposit disabled until plugin reload.");
        return UINT32_MAX;
    }
}

bool AdmissionGuarded() noexcept {
    __try {
        return Guard() && DepositGuard();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

Result BatchDepositUnsafe(const D2RL::PluginContext* context, const Info& expected) noexcept {
    __try {
        if (context != pluginContext || expected.structSize < D2RL::Items::ItemInfoSize ||
            expected.container != Container::Inventory || !AvailableUnsafe(true)) return Result::Refused;
        Info live{.structSize = D2RL::Items::ItemInfoSize};
        if (itemService->getItemInfo(context, expected.handle, &live) != D2RL::Items::Result::Success ||
            !QolClientTransfer::Source(expected, live)) return Result::Refused;
        auto* player = D2R::Native::GetLocalPlayerUnit(context->exeBase);
        auto* item = static_cast<uint32_t*>(At<void*(__fastcall*)(uint32_t, uint32_t)>(0x9a5d0)(live.runtimeId, 4));
        if (!player || !item || item[0] != 4 || item[1] != live.classId || item[2] != live.runtimeId || item[3] != 0 ||
            At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(item) != live.code ||
            At<uint8_t(__fastcall*)(const void*)>(0x36cfe0)(item) != 0 ||
            At<int(__fastcall*)(const void*)>(0x1c7360)(item) ||
            At<void*(__fastcall*)()>(0x14f1e0)() || At<int(__fastcall*)()>(0x14f210)() == 5) return Result::Refused;
        if (!At<bool(__fastcall*)(const void*)>(0x15a0b0)(item)) return Result::Ineligible;
        auto* destination = At<void*(__fastcall*)(void*)>(0x46da50)(player);
        if (!destination) return Result::Refused;
        alignas(8) unsigned char placement[16]{};
        // Submitted includes a native false return: the unchanged queue owns
        // observation and must never retry an uncertain client request.
        (void)At<D2R::Native::TransferItemToInventoryPageFn>(0x15f8b0)(item, destination, 4, 0, true, placement);
        At<D2R::Native::FinishInventoryInteractionFn>(0x1a0780)(3, nullptr, 0, 0, false);
        return Result::Submitted;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Disable("[InventoryDeposit] Native submission fault; batch stopped without retry and deposit disabled until plugin reload.");
        return Result::Refused;
    }
}
}

namespace InventoryDepositNative {
bool Initialize(const D2RL::PluginContext* context) noexcept {
    std::lock_guard lock(nativeMutex);
    pluginContext = context;
    itemService = nullptr;
    inventoryService = nullptr;
    coreBase = 0;
    supported = false;
    faulted = false;
    const D2RL::ThreadService* threads{};
    const D2RL::LifecycleService* lifecycle{};
    if (!context || !context->exeBase ||
        context->QueryService(&itemService) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasItemServiceField(itemService, D2RL::ItemServiceRequiredSize) ||
        !itemService->getItemInfo || !itemService->editNativeItem ||
        context->QueryService(&inventoryService) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasInventoryServiceField(inventoryService, D2RL::InventoryServiceRequiredSize) ||
        !inventoryService->getLocalPlayer || !inventoryService->forEachInventoryItem ||
        context->QueryService(&threads) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(threads, D2RL::ThreadServiceRequiredSize) ||
        !threads->runOnUiThread || !threads->runOnGameThread ||
        context->QueryService(&lifecycle) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasLifecycleServiceField(lifecycle, D2RL::LifecycleServiceRequiredSize) ||
        !lifecycle->registerGameplayEventListener) {
        Disable("[InventoryDeposit] Required SDK services unavailable; deposit disabled.");
        return false;
    }
    const auto core = GetModuleHandleW(L"D2RCore.dll");
    coreBase = reinterpret_cast<uintptr_t>(core);
    if (!QolCore::VerifyFileHash(core, QolNativeProfile::CoreHash) || !AdmissionGuarded()) {
        Disable("[InventoryDeposit] Existing deposit profile rejected; deposit disabled.");
        return false;
    }
    supported = true;
    context->LogInfo("[InventoryDeposit] Existing guarded deposit profile admitted; no controller hooks installed.");
    return true;
}

void Shutdown() noexcept {
    std::lock_guard lock(nativeMutex);
    supported = false;
    pluginContext = nullptr;
    itemService = nullptr;
    inventoryService = nullptr;
    coreBase = 0;
}

bool Supported() noexcept {
    std::lock_guard lock(nativeMutex);
    return supported && !faulted;
}

bool Visible() noexcept {
    std::lock_guard lock(nativeMutex);
    return AvailableGuarded(false);
}

bool Available() noexcept {
    std::lock_guard lock(nativeMutex);
    return AvailableGuarded(true);
}

bool HasEligibleItems() noexcept {
    std::lock_guard lock(nativeMutex);
    return HasEligibleItemsGuarded();
}
}

// Only the two adapter symbols required by the unchanged bulk queue. This DLL
// does not link the QOL controller/input/material-withdrawal implementations.
namespace QolMaterials {
uint32_t SelectedStashTab(const D2RL::PluginContext* context) noexcept {
    std::lock_guard lock(nativeMutex);
    return SelectedTabGuarded(context);
}
}

namespace QolClientTransfer {
BatchDepositResult BatchDeposit(const D2RL::PluginContext* context, const Info& expected) noexcept {
    std::lock_guard lock(nativeMutex);
    return BatchDepositUnsafe(context, expected);
}
}
