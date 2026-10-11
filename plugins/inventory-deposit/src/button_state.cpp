#include "button_state.h"
#include "button_state_profile.h"
#include "gold_anchor_profile.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
#include "native_identify_profile.h"
#include "aim/tree_layout_profile.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace InventoryDepositButtonState {
namespace {
namespace Profile = InventoryDepositButtonStateProfile;
using Rect = D2RL::Widgets::Rect;
using Result = D2RL::Widgets::Result;
constexpr char ChildName[] = "inventory-deposit/DepositAllButton";
const D2RL::PluginContext* context{};
const D2RL::WidgetService* widgets{};
std::uintptr_t core{};
bool admitted{};
unsigned transitionLogs{};
std::array<bool, 2> unavailableReported{};
std::array<int, 2> reportedState{-1, -1};
const char* reason{};

template<class T> T Read(const void* object, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, static_cast<const unsigned char*>(object) + offset, sizeof(value));
    return value;
}
template<class T> T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(context->exeBase + rva);
}
bool Guard() noexcept {
    if (!context || !context->exeBase || !core ||
        reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"D2RCore.dll")) != core) return false;
    if (!context->CheckExpectedBytes(0x846170, QolNativeIdentifyProfile::FindPanel,
            sizeof(QolNativeIdentifyProfile::FindPanel)) ||
        !context->CheckExpectedBytes(InventoryDepositGoldProfile::FindChildRva,
            InventoryDepositGoldProfile::FindChild, sizeof(InventoryDepositGoldProfile::FindChild)) ||
        !context->CheckExpectedBytes(0x8562A0, TreeLayoutNative::Rect, sizeof(TreeLayoutNative::Rect))) return false;
    for (const auto& site : Profile::Sites)
        if (!context->CheckExpectedBytes(site.rva, site.bytes, static_cast<uint32_t>(site.size))) return false;
    return true;
}
bool GuardSafely() noexcept {
    __try { return Guard(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Same(const Rect& a, const Rect& b) noexcept {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}
bool NamedChild(const void* child) noexcept {
    const auto name = Read<const char*>(child, 0x08);
    if (!name) return false;
    // Bounded comparison includes the terminating null and never reads past it.
    for (std::size_t i = 0; i < sizeof(ChildName); ++i)
        if (name[i] != ChildName[i]) return false;
    return true;
}
bool ApplyUnsafe(const char* panelName, D2RL::Widgets::WidgetHandle parentHandle,
    D2RL::Widgets::WidgetHandle buttonHandle, bool enabled) noexcept {
    reason = "native profile";
    if (!Guard()) { admitted = false; return false; }
    reason = "SDK path or rectangle lookup";
    D2RL::Widgets::WidgetHandle resolved{};
    Rect parent{}, button{}, resolvedButton{};
    if (widgets->findWidget(context, parentHandle, ChildName, &resolved) != Result::Success ||
        !resolved ||
        widgets->getWidgetRect(context, resolved, &resolvedButton) != Result::Success ||
        widgets->getWidgetRect(context, parentHandle, &parent) != Result::Success ||
        widgets->getWidgetRect(context, buttonHandle, &button) != Result::Success ||
        !Same(button, resolvedButton) ||
        parent.width <= 0 || parent.height <= 0 || button.width != 144 || button.height != 136) return false;

    reason = "native panel and unique child";
    void* root = At<void*(__fastcall*)(const char*)>(0x846170)(panelName);
    if (!root) return false;
    const auto count = Read<std::uint64_t>(root, 0x60);
    const auto children = Read<const void*>(root, 0x58);
    if (!children || !count || count > 256) return false;
    unsigned matching{};
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto candidate = Read<const void*>(children, static_cast<std::size_t>(i) * sizeof(void*));
        if (!candidate) return false;
        if (NamedChild(candidate)) ++matching;
    }
    if (matching != 1) return false;
    void* native = At<void*(__fastcall*)(void*, const char*)>(InventoryDepositGoldProfile::FindChildRva)(root, ChildName);
    if (!native || Read<const void*>(native, 0x30) != root || Read<unsigned char>(native, 0x52) != 0) return false;

    reason = "exact ButtonWidget type";
    const auto table = Read<std::uintptr_t>(native, 0);
    if (table != context->exeBase + Profile::ButtonVtableRva ||
        Read<std::uintptr_t>(reinterpret_cast<const void*>(table), 0x48) != context->exeBase + Profile::SetInputEnabledRva ||
        Read<std::uintptr_t>(reinterpret_cast<const void*>(table), 0x50) != context->exeBase + Profile::SetVisibleRva) return false;

    Rect nativeParent{}, nativeButton{};
    const auto getRect = At<Rect*(__fastcall*)(void*, Rect*)>(0x8562A0);
    getRect(root, &nativeParent); getRect(native, &nativeButton);
    reason = "native/SDK rectangle disagreement";
    if (!Same(parent, nativeParent) || !Same(button, nativeButton)) return false;

    reason = "button frames, text layout, or input state";
    // Native visual transitions adjust the embedded text position. Its
    // fill-parent assertion path is excluded, even for our textless button.
    if (Read<unsigned char>(native, 0x818 + 0x52) != 0 ||
        Read<int>(native, 0x7E8) != 0 || Read<int>(native, 0x7EC) != 2 ||
        Read<int>(native, 0x7F0) != 1 || Read<int>(native, 0x7F4) != 3 ||
        Read<unsigned char>(native, 0x50) != static_cast<unsigned char>(enabled)) return false;
    const auto disabled = Read<unsigned char>(native, 0x67A);
    if (disabled > 1) return false;
    // The pressed-state helper used while enabling obtains this manager.
    // Refuse its null-global diagnostic path and null nested manager access.
    const auto managerOwner = Read<const void*>(At<const void*>(Profile::ManagerGlobalRva), 0);
    if (!managerOwner || !Read<const void*>(managerOwner, 0xD0)) return false;
    reason = "visual transition/read-back";
    if (disabled != static_cast<unsigned char>(!enabled)) {
        const auto rva = enabled ? Profile::EnableRva : Profile::DisableRva;
        At<void(__fastcall*)(void*)>(rva)(native);
    }
    const auto frame = Read<int>(native, 0x710);
    return Read<unsigned char>(native, 0x67A) == static_cast<unsigned char>(!enabled) &&
        Read<unsigned char>(native, 0x50) == static_cast<unsigned char>(enabled) &&
        (enabled ? (frame >= 0 && frame <= 2) : frame == 3);
}
bool ApplySafely(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button, bool enabled) noexcept {
    __try { return ApplyUnsafe(panelName, parent, button, enabled); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        admitted = false;
        reason = "native access fault; unavailable until restart";
        return false;
    }
}
}

bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    Shutdown();
    if (!ctx) return false;
    context = ctx;
    const auto module = GetModuleHandleW(L"D2RCore.dll");
    core = reinterpret_cast<std::uintptr_t>(module);
    if (ctx->QueryService(&widgets) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasWidgetServiceField(widgets, D2RL::WidgetServiceRequiredSize) ||
        !widgets->findWidget || !widgets->getWidgetRect ||
        !QolCore::VerifyFileHash(module, QolNativeProfile::CoreHash) || !GuardSafely()) {
        ctx->LogWarn("[InventoryDeposit] Button artwork state unavailable: widget service or native profile mismatch.");
        Shutdown(); return false;
    }
    admitted = true;
    ctx->LogInfo("[InventoryDeposit] Native ButtonWidget artwork-state profile admitted.");
    return true;
}
bool Apply(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button, bool enabled) noexcept {
    if (!admitted || !context || !panelName || !parent || !button) return false;
    const bool expansion = std::strcmp(panelName, "PlayerInventoryExpansionLayout") == 0;
    if (!expansion && std::strcmp(panelName, "PlayerInventoryOriginalLayout") != 0) return false;
    const std::size_t index = expansion ? 0 : 1;
    const bool applied = ApplySafely(panelName, parent, button, enabled);
    char message[320]{};
    if (!applied && !unavailableReported[index]) {
        std::snprintf(message, sizeof(message), "[InventoryDeposit] %s button artwork state unavailable (%s); button hidden.",
            panelName, reason ? reason : "native access");
        context->LogWarn(message);
        unavailableReported[index] = true;
    } else if (applied && reportedState[index] != static_cast<int>(enabled)) {
        std::snprintf(message, sizeof(message), "[InventoryDeposit] %s button artwork verified: %s.",
            panelName, enabled ? "enabled" : "disabled frame 3");
        if (transitionLogs < 32) { context->LogInfo(message); ++transitionLogs; }
        reportedState[index] = static_cast<int>(enabled);
        unavailableReported[index] = false;
    }
    return applied;
}
void Shutdown() noexcept {
    admitted = false; context = nullptr; widgets = nullptr; core = 0;
    unavailableReported = {}; reportedState = {-1, -1}; transitionLogs = 0; reason = nullptr;
}
}
