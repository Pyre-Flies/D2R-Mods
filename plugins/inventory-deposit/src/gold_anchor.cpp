#include "gold_anchor.h"
#include "gold_anchor_policy.h"
#include "gold_anchor_profile.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
#include "native_identify_profile.h"
#include "aim/tree_layout_profile.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace InventoryDepositGoldAnchor {
namespace {
using Rect = D2RL::Widgets::Rect;
using Result = D2RL::Widgets::Result;
const D2RL::PluginContext* context{};
const D2RL::WidgetService* widgets{};
std::uintptr_t core{};
int gap{}, offsetY{};
bool admitted{};
std::array<bool, 2> unavailableReported{}, placementReported{};
const char* reason{};
constexpr char ChildName[] = "inventory-deposit/DepositAllButton";

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
            sizeof(QolNativeIdentifyProfile::FindPanel))) return false;
    for (const auto& site : InventoryDepositGoldProfile::Sites)
        if (!context->CheckExpectedBytes(site.rva, site.bytes, static_cast<uint32_t>(site.size))) return false;
    // These established geometry witnesses qualify the parent/anchor/scale
    // fields inspected below, as well as the native local-rectangle getter.
    return context->CheckExpectedBytes(0x8562A0, TreeLayoutNative::Rect, sizeof(TreeLayoutNative::Rect)) &&
        context->CheckExpectedBytes(0x856330, TreeLayoutNative::Origin, sizeof(TreeLayoutNative::Origin)) &&
        context->CheckExpectedBytes(0x1E6756, TreeLayoutNative::Scale, sizeof(TreeLayoutNative::Scale));
}
bool GuardSafely() noexcept {
    __try { return Guard(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Same(const Rect& a, const Rect& b) noexcept {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}
bool PlainChild(const void* child, const void* parent) noexcept {
    // Both controls must share the SDK rectangle's parent coordinate system.
    // A nested, fill-parent, independently scaled or anchored replacement is
    // not silently treated as an ordinary direct inventory child.
    return Read<const void*>(child, 0x30) == parent && Read<unsigned char>(child, 0x52) == 0 &&
        Read<float>(child, 0x48) == 0.0f && Read<float>(child, 0x4C) == 0.0f &&
        Read<float>(child, 0x80) == 1.0f;
}
bool PositionUnsafe(const char* panelName, D2RL::Widgets::WidgetHandle parentHandle,
    D2RL::Widgets::WidgetHandle buttonHandle) noexcept {
    reason = "native profile";
    if (!Guard()) {
        admitted = false;
        context->LogWarn("[InventoryDeposit] Gold-anchor native profile changed; anchored button disabled until restart.");
        return false;
    }
    // Resolve through the SDK to read current loaded geometry, including any
    // mod changes. No stock JSON, screen resolution or native widget address
    // is cached. Direct native lookup is only needed to call the setter.
    D2RL::Widgets::WidgetHandle goldHandle{};
    Rect gold{}, button{}, parent{}, target{};
    reason = "gold/widget rectangle lookup";
    if (widgets->findWidget(context, parentHandle, "gold_amount", &goldHandle) != Result::Success ||
        widgets->getWidgetRect(context, goldHandle, &gold) != Result::Success ||
        widgets->getWidgetRect(context, buttonHandle, &button) != Result::Success ||
        widgets->getWidgetRect(context, parentHandle, &parent) != Result::Success) return false;
    reason = "rectangle dimensions or panel bounds";
    if (!Compute(gold, button, parent, gap, offsetY, target)) return false;

    auto findPanel = At<void*(__fastcall*)(const char*)>(0x846170);
    auto findChild = At<void*(__fastcall*)(void*, const char*)>(InventoryDepositGoldProfile::FindChildRva);
    reason = "native panel/child lookup";
    void* root = findPanel(panelName);
    if (!root) return false;
    // The native lookup walks this direct-child vector and asserts on null
    // entries. Admit a small, complete vector before entering that loop.
    const auto count = Read<std::uint64_t>(root, 0x60);
    const auto children = Read<const void*>(root, 0x58);
    if (!children || !count || count > 256) return false;
    for (std::uint64_t i = 0; i < count; ++i)
        if (!Read<const void*>(children, static_cast<std::size_t>(i) * sizeof(void*))) return false;
    void* nativeGold = findChild(root, "gold_amount");
    void* nativeButton = findChild(root, ChildName);
    if (!nativeGold || !nativeButton || nativeGold == nativeButton) return false;
    reason = "child parent/anchor/scale";
    if (!PlainChild(nativeGold, root) || !PlainChild(nativeButton, root)) return false;
    reason = "button hit-test virtual method";
    const auto table = Read<std::uintptr_t>(nativeButton, 0);
    if (!table || Read<std::uintptr_t>(reinterpret_cast<const void*>(table), 0x40) !=
            context->exeBase + InventoryDepositGoldProfile::ContainsRva) return false;

    // Bind the path handles' geometry to the exact native controls just found.
    Rect checkedGold{}, checkedButton{}, checkedParent{};
    auto getRect = At<Rect*(__fastcall*)(void*, Rect*)>(0x8562A0);
    getRect(nativeGold, &checkedGold);
    getRect(nativeButton, &checkedButton);
    getRect(root, &checkedParent);
    reason = "native/SDK rectangle disagreement";
    if (!Same(gold, checkedGold) || !Same(button, checkedButton) || !Same(parent, checkedParent)) return false;
    reason = "native setter/read-back";
    if (button.x != target.x)
        At<void(__fastcall*)(void*, int32_t)>(InventoryDepositGoldProfile::SetXRva)(nativeButton, target.x);
    if (button.y != target.y)
        At<void(__fastcall*)(void*, int32_t)>(InventoryDepositGoldProfile::SetYRva)(nativeButton, target.y);
    Rect applied{};
    return widgets->getWidgetRect(context, buttonHandle, &applied) == Result::Success && Same(applied, target);
}
bool PositionSafely(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button) noexcept {
    __try { return PositionUnsafe(panelName, parent, button); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        admitted = false;
        context->LogWarn("[InventoryDeposit] Gold-anchor native access fault; anchored button disabled until restart.");
        return false;
    }
}
}

bool Initialize(const D2RL::PluginContext* ctx, int space, int dy) noexcept {
    Shutdown();
    if (!ctx || space < 0 || space > 4096 || dy < -4096 || dy > 4096) return false;
    context = ctx; gap = space; offsetY = dy;
    const auto module = GetModuleHandleW(L"D2RCore.dll");
    core = reinterpret_cast<std::uintptr_t>(module);
    if (ctx->QueryService(&widgets) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasWidgetServiceField(widgets, D2RL::WidgetServiceRequiredSize) ||
        !widgets->findWidget || !widgets->getWidgetRect ||
        !QolCore::VerifyFileHash(module, QolNativeProfile::CoreHash) || !GuardSafely()) {
        ctx->LogWarn("[InventoryDeposit] Gold anchoring unavailable: geometry service or native profile mismatch.");
        Shutdown(); return false;
    }
    admitted = true;
    ctx->LogInfo("[InventoryDeposit] Gold anchoring admitted; follow gold_amount right edge and vertical center.");
    return true;
}
bool Position(const char* panelName, D2RL::Widgets::WidgetHandle parent,
    D2RL::Widgets::WidgetHandle button) noexcept {
    if (!admitted || !context || !panelName || !parent || !button) return false;
    const std::size_t index = std::strcmp(panelName, "PlayerInventoryExpansionLayout") == 0 ? 0 : 1;
    const bool placed = PositionSafely(panelName, parent, button);
    char message[320]{};
    if (!placed && !unavailableReported[index]) {
        std::snprintf(message, sizeof(message), "[InventoryDeposit] %s gold anchor unavailable (%s); button hidden.",
            panelName, reason ? reason : "native access");
        context->LogWarn(message);
        unavailableReported[index] = true;
    } else if (placed && (unavailableReported[index] || !placementReported[index])) {
        std::snprintf(message, sizeof(message), "[InventoryDeposit] %s gold anchor placed and rectangle verified (gap=%d offsetY=%d).",
            panelName, gap, offsetY);
        context->LogInfo(message);
        unavailableReported[index] = false;
        placementReported[index] = true;
    }
    return placed;
}
void Shutdown() noexcept {
    admitted = false; unavailableReported = {}; placementReported = {}; reason = nullptr;
    context = nullptr; widgets = nullptr; core = 0; gap = 0; offsetY = 0;
}
}
