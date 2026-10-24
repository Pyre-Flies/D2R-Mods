#pragma once
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/mutations.h>
#include <array>
#include <cstdint>

namespace Ultrawide::BarsAndMouse {

// Canonical witnesses: docs/reverse-engineering/ultrawide-bars.md
inline constexpr std::array<unsigned char, 5> Left{0xe8, 0xaf, 0x39, 0xe1, 0xff};
inline constexpr std::array<unsigned char, 5> Right{0xe8, 0x8b, 0x39, 0xe1, 0xff};
inline constexpr unsigned char Aspect[]{0x72, 0x1c, 0x07, 0x40};
inline constexpr unsigned char Black[]{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0x3f};
inline constexpr unsigned char ColorLoad[]{0x0f, 0x28, 0x05, 0xc4, 0xeb, 0x4a, 0x01, 0x0f, 0x11, 0x45, 0xb0};
inline constexpr unsigned char Width[]{0xf3, 0x0f, 0x59, 0x05, 0x49, 0x67, 0x47, 0x01, 0xf3, 0x0f, 0x2c, 0xc8, 0x2b, 0xc1};
inline constexpr unsigned char LeftSetup[]{0x33, 0xd2, 0xd1, 0xf8, 0x8b, 0xf8, 0x48, 0x8d, 0x45, 0xb0, 0x44, 0x8b, 0xc7, 0x48, 0x89, 0x44, 0x24, 0x20};
inline constexpr unsigned char RightSetup[]{0x44, 0x8b, 0x4c, 0x24, 0x64, 0x48, 0x8d, 0x45, 0xb0, 0x8b, 0xcb, 0xc7, 0x44, 0x24, 0x28, 0x01, 0, 0, 0, 0x2b, 0xcf, 0x48, 0x89, 0x44, 0x24, 0x20, 0x44, 0x8b, 0xc3, 0x33, 0xd2};

// Separate aspect limit supplied by the vignette pass, not the shared input constant.
inline constexpr unsigned char ShaderArgs[]{0x8b, 0x81, 0x34, 0x17, 0x01, 0x00, 0xc7, 0x42, 0x08, 0x72, 0x1c, 0x07, 0x40, 0xf3, 0x48, 0x0f, 0x2a, 0xc0, 0xf3, 0x0f, 0x11, 0x0a, 0xf3, 0x0f, 0x11, 0x42, 0x04};
// Finite 32:1 ceiling for the vignette shader mask.
inline constexpr unsigned char ShaderWide[]{0x00, 0x00, 0x00, 0x42};
inline constexpr unsigned char MenuGate[]{0x84, 0xc0, 0x0f, 0x84, 0xa9, 0x00, 0x00, 0x00, 0x4c, 0x8d, 0x05, 0xe4, 0xca, 0x52, 0x01};

// Mouse horizontal coordinate clamp witnesses inside 0x7F350:
// 0x7F427: call 0x846210; test al,al; je 0x7F48C; movd xmm2,r15d; cvtdq2ps xmm2,xmm2 (17 bytes)
inline constexpr unsigned char MouseGate[]{
    0xe8, 0xe4, 0x6d, 0x7c, 0x00,
    0x84, 0xc0,
    0x74, 0x5c,
    0x66, 0x41, 0x0f, 0x6e, 0xd7,
    0x0f, 0x5b, 0xd2
};
// 0x7F438: movd xmm1, ebx; mulss xmm2, [0x1CBA908] (12 bytes)
inline constexpr unsigned char MouseClampMul[]{
    0x66, 0x0f, 0x6e, 0xcb,
    0xf3, 0x0f, 0x59, 0x15, 0xc4, 0xb4, 0xc3, 0x01
};
// 0x7F48C: mov rbx, [rsp+0x68]; mov rax, rdi; add rsp, 0x20; pop r15..rbp; ret (20 bytes)
inline constexpr unsigned char MouseUnclampedExitBytes[]{
    0x48, 0x8b, 0x5c, 0x24, 0x68,
    0x48, 0x8b, 0xc7,
    0x48, 0x83, 0xc4, 0x20,
    0x41, 0x5f, 0x41, 0x5e, 0x5f, 0x5e, 0x5d, 0xc3
};

// Ground loot item placard horizontal bounds inside 0x1FA7B0 (GetPlacardHorizontalScreenBounds):
// 0x1FA7CB (22 bytes):
//   e8 d0 ba 64 00    call 0x8462A0 (GetUIRootOrigin)
//   89 06             mov [rsi], eax (outLeftBound = left)
//   e8 c9 ba 64 00    call 0x8462A0 (GetUIRootOrigin)
//   48 8b d8          mov rbx, rax
//   e8 01 bb 64 00    call 0x8462E0 (GetUIRootSize)
//   03 c3             add eax, ebx (eax = left + cappedWidth)
inline constexpr unsigned char PlacardBoundsGate[]{
    0xe8, 0xd0, 0xba, 0x64, 0x00,
    0x89, 0x06,
    0xe8, 0xc9, 0xba, 0x64, 0x00,
    0x48, 0x8b, 0xd8,
    0xe8, 0x01, 0xbb, 0x64, 0x00,
    0x03, 0xc3
};
// 0x1FA7E1 (10 bytes): lea rcx, [rip+0x1ac81b8]; mov [r14], eax (outRightBound = eax)
inline constexpr unsigned char PlacardBoundsContinueGuard[]{
    0x48, 0x8d, 0x0d, 0xb8, 0x81, 0xac, 0x01,
    0x41, 0x89, 0x06
};
// 0x6545D0 (14 bytes): GetFullWindowDimensions (returns packed width:height in rax)
inline constexpr unsigned char GetFullWindowDimsGuard[]{
    0x48, 0x83, 0xec, 0x38,
    0xe8, 0x97, 0xd1, 0x2a, 0x00,
    0x48, 0x8d, 0x54, 0x24, 0x20
};

// Full-screen Automap overlay bounds inside 0x14E3480 (AutoMapPanel::Draw):
// 0x14E367C (18 bytes):
//   e8 1f 2c 36 ff       call 0x8462A0 (GetUIRootOrigin -> rax = x | (y << 32))
//   48 8b d8             mov rbx, rax
//   48 89 44 24 28       mov [rsp + 0x28], rax
//   e8 52 2c 36 ff       call 0x8462E0 (GetUIRootSize -> rax = w | (h << 32))
inline constexpr unsigned char AutomapBoundsGate[]{
    0xe8, 0x1f, 0x2c, 0x36, 0xff,
    0x48, 0x8b, 0xd8,
    0x48, 0x89, 0x44, 0x24, 0x28,
    0xe8, 0x52, 0x2c, 0x36, 0xff
};
// 0x14E368E (10 bytes): mov r8d, [rbp-0x7d]; mov rcx, rax; mov [rbp-0x51], eax
inline constexpr unsigned char AutomapBoundsContinueGuard[]{
    0x44, 0x8b, 0x45, 0x83,
    0x48, 0x8b, 0xc8,
    0x89, 0x45, 0xaf
};

// UIRootWidget::UpdateLayout (0x8A64F0) horizontal 19:9 (0x11D0 = 4560) clamp gate:
// 0x8A6635 (15 bytes):
//   b8 d0 11 00 00       mov eax, 0x11d0
//   8b d6                mov edx, esi
//   3b f0                cmp esi, eax
//   48 8b cf             mov rcx, rdi
//   0f 4f d0             cmovg edx, eax
inline constexpr unsigned char UIRootWidthClampGate[]{
    0xb8, 0xd0, 0x11, 0x00, 0x00,
    0x8b, 0xd6,
    0x3b, 0xf0,
    0x48, 0x8b, 0xcf,
    0x0f, 0x4f, 0xd0
};
// 0x8A6644 (13 bytes): call 0x857520; lea rdx, [rsp+0x30]; mov rcx, rdi
inline constexpr unsigned char UIRootWidthClampContinueGuard[]{
    0xe8, 0xd7, 0x0e, 0xfb, 0xff,
    0x48, 0x8d, 0x54, 0x24, 0x30,
    0x48, 0x8b, 0xcf
};

// UIRootWidget::Update (0x8A5AA0) post-children-update gate:
// 0x8A6461 (16 bytes):
//   48 8b cb             mov rcx, rbx
//   e8 57 8c ff ff       call 0x89f0c0
//   48 8b cb             mov rcx, rbx
//   e8 0f 87 ff ff       call 0x89eb80
inline constexpr unsigned char UIRootUpdateCornerHudGate[]{
    0x48, 0x8b, 0xcb,
    0xe8, 0x57, 0x8c, 0xff, 0xff,
    0x48, 0x8b, 0xcb,
    0xe8, 0x0f, 0x87, 0xff, 0xff
};
// 0x8A6471 (14 bytes): test r13b, r13b; mov r13, [rsp+0x5e8]; je 0x8a64ce
inline constexpr unsigned char UIRootUpdateCornerHudContinueGuard[]{
    0x45, 0x84, 0xed,
    0x4c, 0x8b, 0xac, 0x24, 0xe8, 0x05, 0x00, 0x00,
    0x74, 0x50
};

inline bool BarsGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x844165, MenuGate, sizeof(MenuGate)) &&
        ctx->CheckExpectedBytes(0xe19df2, ShaderArgs, sizeof(ShaderArgs)) &&
        ctx->CheckExpectedBytes(0x1cba908, Aspect, sizeof(Aspect)) &&
        ctx->CheckExpectedBytes(0x1cf2d50, Black, sizeof(Black)) &&
        ctx->CheckExpectedBytes(0x844185, ColorLoad, sizeof(ColorLoad)) &&
        ctx->CheckExpectedBytes(0x8441b7, Width, sizeof(Width)) &&
        ctx->CheckExpectedBytes(0x8441ca, LeftSetup, sizeof(LeftSetup)) &&
        ctx->CheckExpectedBytes(0x8441e1, RightSetup, sizeof(RightSetup)) &&
        ctx->CheckExpectedBytes(0x8441dc, Left.data(), static_cast<uint32_t>(Left.size())) &&
        ctx->CheckExpectedBytes(0x844200, Right.data(), static_cast<uint32_t>(Right.size()));
}

inline bool MouseGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x7f427, MouseGate, sizeof(MouseGate)) &&
        ctx->CheckExpectedBytes(0x7f438, MouseClampMul, sizeof(MouseClampMul)) &&
        ctx->CheckExpectedBytes(0x7f48c, MouseUnclampedExitBytes, sizeof(MouseUnclampedExitBytes)) &&
        ctx->CheckExpectedBytes(0x1cba908, Aspect, sizeof(Aspect));
}

inline bool PlacardGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x1fa7cb, PlacardBoundsGate, sizeof(PlacardBoundsGate)) &&
        ctx->CheckExpectedBytes(0x1fa7e1, PlacardBoundsContinueGuard, sizeof(PlacardBoundsContinueGuard)) &&
        ctx->CheckExpectedBytes(0x6545d0, GetFullWindowDimsGuard, sizeof(GetFullWindowDimsGuard));
}

inline bool AutomapGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x14e367c, AutomapBoundsGate, sizeof(AutomapBoundsGate)) &&
        ctx->CheckExpectedBytes(0x14e368e, AutomapBoundsContinueGuard, sizeof(AutomapBoundsContinueGuard)) &&
        ctx->CheckExpectedBytes(0x6545d0, GetFullWindowDimsGuard, sizeof(GetFullWindowDimsGuard));
}

inline bool FullLengthHudGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x8a6635, UIRootWidthClampGate, sizeof(UIRootWidthClampGate)) &&
        ctx->CheckExpectedBytes(0x8a6644, UIRootWidthClampContinueGuard, sizeof(UIRootWidthClampContinueGuard));
}

inline bool CornerHudGuards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x8a6461, UIRootUpdateCornerHudGate, sizeof(UIRootUpdateCornerHudGate)) &&
        ctx->CheckExpectedBytes(0x8a6471, UIRootUpdateCornerHudContinueGuard, sizeof(UIRootUpdateCornerHudContinueGuard));
}

namespace Detail {

inline void UpdateSingleCornerPanelAnchor(
    uintptr_t panel,
    uintptr_t autoMapVtable,
    uintptr_t hireablesVtable,
    uintptr_t hudWarningsVtable,
    bool cornerMinimap,
    bool cornerMercenary,
    bool cornerGameInfo,
    float anchorDelta) noexcept
{
    if (!panel) return;
    const uintptr_t vt = *reinterpret_cast<const uintptr_t*>(panel);
    if (vt == autoMapVtable) {
        if (!cornerMinimap) return;
        float& anchorX = *reinterpret_cast<float*>(panel + 0x48);
        if (anchorX <= 0.25f) {
            anchorX = -anchorDelta;
        } else if (anchorX >= 0.75f) {
            anchorX = 1.0f + anchorDelta;
        }
    } else if (vt == hireablesVtable) {
        if (!cornerMercenary) return;
        *reinterpret_cast<float*>(panel + 0x48) = -anchorDelta;
    } else if (vt == hudWarningsVtable) {
        if (!cornerGameInfo) return;
        *reinterpret_cast<float*>(panel + 0x48) = 1.0f + anchorDelta;
    }
}

} // namespace Detail

// Updates m_anchor.x (+0x48) on top-corner HUD panels (AutoMapPanel, HireablesPanel, HUDWarningsPanel)
// immediately after UIRootWidget updates its children in UIRootWidget::Update (0x8A5AA0 -> 0x8A6461).
// Because Widget::GetScreenPos (0x8562F0) computes screenX = uiRoot.x + (uiRoot.width * panel.m_anchor.x + panel.x) * uiRoot.scale,
// shifting m_anchor.x by ±anchorDelta where anchorDelta = uiRoot.x / (uiRoot.width * uiRoot.scale)
// places those panels and all of their children at the true ultrawide screen corners without hooking 0x8562F0.
inline void UpdateCornerHudPanelsImpl(
    uintptr_t uiRoot,
    bool inSession,
    uintptr_t autoMapVtable,
    uintptr_t hireablesVtable,
    uintptr_t hudWarningsVtable,
    bool cornerMinimap,
    bool cornerMercenary,
    bool cornerGameInfo) noexcept
{
    if (!uiRoot || (!cornerMinimap && !cornerMercenary && !cornerGameInfo)) {
        return;
    }

    float anchorDelta = 0.0f;
    if (inSession) {
        const int32_t rootOffsetX = *reinterpret_cast<const int32_t*>(uiRoot + 0x70);
        const int32_t rootWidth = *reinterpret_cast<const int32_t*>(uiRoot + 0x78);
        const float rootScale = *reinterpret_cast<const float*>(uiRoot + 0x80);
        const float scaledRootWidth = static_cast<float>(rootWidth) * rootScale;
        if (rootOffsetX > 0 && rootWidth > 0 && scaledRootWidth > 1.0f) {
            anchorDelta = static_cast<float>(rootOffsetX) / scaledRootWidth;
        }
    }

    const uintptr_t* children = *reinterpret_cast<const uintptr_t* const*>(uiRoot + 0x58);
    const uint64_t childCount = *reinterpret_cast<const uint64_t*>(uiRoot + 0x60);
    if (!children || childCount == 0 || childCount > 512) {
        return;
    }

    for (uint64_t i = 0; i < childCount; ++i) {
        const uintptr_t child = children[i];
        if (!child) continue;
        Detail::UpdateSingleCornerPanelAnchor(
            child,
            autoMapVtable,
            hireablesVtable,
            hudWarningsVtable,
            cornerMinimap,
            cornerMercenary,
            cornerGameInfo,
            anchorDelta);

        // Also inspect 1 level of sub-panels in case a container panel wraps any HUD panel.
        const uintptr_t* subChildren = *reinterpret_cast<const uintptr_t* const*>(child + 0x58);
        const uint64_t subCount = *reinterpret_cast<const uint64_t*>(child + 0x60);
        if (subChildren && subCount > 0 && subCount <= 256) {
            for (uint64_t j = 0; j < subCount; ++j) {
                Detail::UpdateSingleCornerPanelAnchor(
                    subChildren[j],
                    autoMapVtable,
                    hireablesVtable,
                    hudWarningsVtable,
                    cornerMinimap,
                    cornerMercenary,
                    cornerGameInfo,
                    anchorDelta);
            }
        }
    }
}

enum class ApplyResult { Applied, NothingToApply, GuardRefused, ServiceUnavailable, StageFailed, CommitFailed, RollbackFailed };

inline ApplyResult Apply(
    const D2RL::PluginContext* ctx,
    const D2RL::MutationService* svc,
    bool enableBars,
    void* menuEntry,
    bool enableMouse,
    void* mouseEntry,
    bool enablePlacard = false,
    void* placardEntry = nullptr,
    bool enableAutomap = false,
    void* automapEntry = nullptr,
    bool enableFullLengthHud = false,
    void* uiRootWidthEntry = nullptr,
    bool enableCornerHud = false,
    void* cornerHudUpdateEntry = nullptr) noexcept
{
    using namespace D2RL::Mutations;
    if (!enableBars && !enableMouse && !enablePlacard && !enableAutomap && !enableFullLengthHud && !enableCornerHud) {
        return ApplyResult::NothingToApply;
    }
    if ((enableBars && (!BarsGuards(ctx) || !menuEntry)) ||
        (enableMouse && (!MouseGuards(ctx) || !mouseEntry)) ||
        (enablePlacard && (!PlacardGuards(ctx) || !placardEntry)) ||
        (enableAutomap && (!AutomapGuards(ctx) || !automapEntry)) ||
        (enableFullLengthHud && (!FullLengthHudGuards(ctx) || !uiRootWidthEntry)) ||
        (enableCornerHud && (!CornerHudGuards(ctx) || !cornerHudUpdateEntry))) {
        return ApplyResult::GuardRefused;
    }
    if (!D2RL::HasMutationServiceField(svc, D2RL::MutationServiceRequiredSize) ||
        !svc->stageInlineHook || !svc->beginTransaction || !svc->stageBytePatch || !svc->commit || !svc->cancelTransaction) {
        return ApplyResult::ServiceUnavailable;
    }

    TransactionHandle transaction{};
    if (svc->beginTransaction(ctx, &transaction) != Result::Success || !transaction) {
        return ApplyResult::StageFailed;
    }

    if (enableBars) {
        const InlineHookRequest gate{
            .structSize = InlineHookRequestSize,
            .rva = 0x844165,
            .expected = MenuGate,
            .expectedSize = sizeof(MenuGate),
            .target = menuEntry
        };
        OperationHandle gateOperation{};
        if (svc->stageInlineHook(ctx, transaction, &gate, &gateOperation) != Result::Success || !gateOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
        const BytePatchRequest shader{
            .structSize = BytePatchRequestSize,
            .rva = 0xe19dfb,
            .expected = Aspect,
            .expectedSize = sizeof(Aspect),
            .bytes = ShaderWide,
            .size = sizeof(ShaderWide)
        };
        OperationHandle shaderOperation{};
        if (svc->stageBytePatch(ctx, transaction, &shader, &shaderOperation) != Result::Success || !shaderOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    if (enableMouse) {
        const InlineHookRequest mouseHook{
            .structSize = InlineHookRequestSize,
            .rva = 0x7f427,
            .expected = MouseGate,
            .expectedSize = sizeof(MouseGate),
            .target = mouseEntry
        };
        OperationHandle mouseOperation{};
        if (svc->stageInlineHook(ctx, transaction, &mouseHook, &mouseOperation) != Result::Success || !mouseOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    if (enablePlacard) {
        const InlineHookRequest placardHook{
            .structSize = InlineHookRequestSize,
            .rva = 0x1fa7cb,
            .expected = PlacardBoundsGate,
            .expectedSize = sizeof(PlacardBoundsGate),
            .target = placardEntry
        };
        OperationHandle placardOperation{};
        if (svc->stageInlineHook(ctx, transaction, &placardHook, &placardOperation) != Result::Success || !placardOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    if (enableAutomap) {
        const InlineHookRequest automapHook{
            .structSize = InlineHookRequestSize,
            .rva = 0x14e367c,
            .expected = AutomapBoundsGate,
            .expectedSize = sizeof(AutomapBoundsGate),
            .target = automapEntry
        };
        OperationHandle automapOperation{};
        if (svc->stageInlineHook(ctx, transaction, &automapHook, &automapOperation) != Result::Success || !automapOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    if (enableFullLengthHud) {
        const InlineHookRequest uiRootHook{
            .structSize = InlineHookRequestSize,
            .rva = 0x8a6635,
            .expected = UIRootWidthClampGate,
            .expectedSize = sizeof(UIRootWidthClampGate),
            .target = uiRootWidthEntry
        };
        OperationHandle uiRootOperation{};
        if (svc->stageInlineHook(ctx, transaction, &uiRootHook, &uiRootOperation) != Result::Success || !uiRootOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    if (enableCornerHud) {
        const InlineHookRequest cornerHudHook{
            .structSize = InlineHookRequestSize,
            .rva = 0x8a6461,
            .expected = UIRootUpdateCornerHudGate,
            .expectedSize = sizeof(UIRootUpdateCornerHudGate),
            .target = cornerHudUpdateEntry
        };
        OperationHandle cornerHudOperation{};
        if (svc->stageInlineHook(ctx, transaction, &cornerHudHook, &cornerHudOperation) != Result::Success || !cornerHudOperation) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    CommitResult result{.structSize = CommitResultSize};
    const auto committed = svc->commit(ctx, transaction, &result);
    if (committed == Result::Success && result.result == Result::Success) {
        return ApplyResult::Applied;
    }
    const auto canceled = svc->cancelTransaction(ctx, transaction);
    if (!HasFlag(result.flags, CommitFlags::OriginalStateRestored) && canceled != Result::Success) {
        return ApplyResult::RollbackFailed;
    }
    return ApplyResult::CommitFailed;
}

}
