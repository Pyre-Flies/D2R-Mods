#pragma once
// Ground item label pipeline (keyboard & controller):
//   - 0x1FA9F0 (LabelFormatter): Evaluates unit into UnitEvaluationCache and sets active scope.
//   - 0xCBEB0  (InnerNameWriter): Swaps custom rule name & stack quantity before glyph measurement.
//   - 0x1FA8E0 (SharedLabelPaint): Applies custom background tint or conceals show:false labels.
//   - 0x658510 (CorrectedGlyphB): Overrides glyph color for rules with textColor.

#include "unit_reader.h"
#include <D2RLPlugin/api.h>
#include <intrin.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace PyreLootFilter::Labels {

namespace Rva {
inline constexpr std::uintptr_t LabelFormatter = 0x1FA9F0;
inline constexpr std::uintptr_t LabelFormatterCall1 = 0x15171E5;
inline constexpr std::uintptr_t LabelFormatterCall2 = 0x1517783;
inline constexpr std::uintptr_t ControllerFormatterReturn = 0x151A2B2;

inline constexpr std::uintptr_t InnerNameWriter = 0xCBEB0;
inline constexpr std::uintptr_t InnerNameWriterCaller = 0x1FAA18;

inline constexpr std::uintptr_t SharedLabelPaint = 0x1FA8E0;
inline constexpr std::uintptr_t KeyboardPaintReturn = 0x1517AF6;
inline constexpr std::uintptr_t ControllerPaintReturn = 0x1519E46;
inline constexpr std::uintptr_t SharedPaintIsHdCall = 0x1FA919;
inline constexpr std::uintptr_t SharedPaintLegacyCoordCall = 0x1FA949;
inline constexpr std::uintptr_t SharedPaintRectCall = 0x1FA9AD;
inline constexpr std::uintptr_t IsHdMode = 0x846210;
inline constexpr std::uintptr_t LegacyCoordToScreen = 0x65B030;
inline constexpr std::uintptr_t DrawUiFilledRect = 0x657B90;

inline constexpr std::uintptr_t CorrectedGlyphB = 0x658510;
inline constexpr std::uintptr_t CorrectedGlyphBCaller = 0x90857B;

inline constexpr std::uintptr_t ShowItemsStatesPtr = 0x235D360;
inline constexpr std::uintptr_t ShowItemsStatesCount = 0x235D368;
inline constexpr std::uintptr_t SetShowItemsState = 0x1FAE90;
inline constexpr std::uintptr_t RefreshShowItemsUi = 0xCE450;
inline constexpr std::uintptr_t IsControllerMode = 0x77E10;
inline constexpr std::uintptr_t IsLoadingScreenActive = 0x8D3D0;
inline constexpr std::uintptr_t FindUiStatePanel = 0x846190;
inline constexpr std::uintptr_t NativeUiStateTable = 0x2A2ADA0;
inline constexpr std::size_t NativeUiItemLabelPanelIndex = 0xCU;
inline constexpr std::uintptr_t OffsetWidgetIsVisible = 0x51U;

inline constexpr std::size_t DirectCallBytes = 5U;
inline constexpr std::size_t InnerNameBufferBytes = 0x80U;
inline constexpr std::size_t InnerNamePrefixBytes = 4U;
inline constexpr std::uintptr_t GroundRecordIdOffset = 0x10U;
inline constexpr std::uintptr_t GroundColorOffset = 0x14U;
inline constexpr std::uintptr_t GroundTextOffset = 0x24U;
} // namespace Rva

namespace Witnesses {
inline constexpr std::array<std::uint8_t, 16> LabelFormatterEntry{{
    0x40, 0x55, 0x53, 0x56, 0x41, 0x54, 0x41, 0x57,
    0x48, 0x8B, 0xEC, 0x48, 0x83, 0xEC, 0x50, 0x45,
}};
inline constexpr std::array<std::uint8_t, 5> LabelFormatterCall1Bytes{{
    0xE8, 0x06, 0x38, 0xCE, 0xFE,
}};
inline constexpr std::array<std::uint8_t, 5> LabelFormatterCall2Bytes{{
    0xE8, 0x68, 0x32, 0xCE, 0xFE,
}};
inline constexpr std::array<std::uint8_t, 19> InnerNameWriterEntry{{
    0x40, 0x53, 0x55, 0x56, 0x57, 0x48, 0x81, 0xEC, 0x68, 0x04,
    0x00, 0x00, 0x48, 0x8B, 0x05, 0x05, 0xF4, 0x8F, 0x02,
}};
inline constexpr std::array<std::uint8_t, 5> InnerNameCallBytes{{
    0xE8, 0x93, 0x14, 0xED, 0xFF,
}};
inline constexpr std::array<std::uint8_t, 17> SharedLabelPaintEntry{{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18,
    0x57, 0x48, 0x83, 0xEC, 0x40, 0x49, 0x8B,
}};
inline constexpr std::array<std::uint8_t, 5> KeyboardPaintCallBytes{{
    0xE8, 0xEA, 0x2D, 0xCE, 0xFE,
}};
inline constexpr std::array<std::uint8_t, 5> ControllerPaintCallBytes{{
    0xE8, 0x9A, 0x0A, 0xCE, 0xFE,
}};
inline constexpr std::array<std::uint8_t, 16> CorrectedGlyphBEntry{{
    0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x18, 0x48,
    0x89, 0x78, 0x20, 0x55, 0x48, 0x8D, 0x68, 0xA8,
}};
inline constexpr std::array<std::uint8_t, 5> CorrectedGlyphBCallBytes{{
    0xE8, 0x90, 0xFF, 0xD4, 0xFF,
}};
inline constexpr std::array<std::uint8_t, 5> SharedPaintIsHdCallBytes{{
    0xE8, 0xF2, 0xB8, 0x64, 0x00,
}};
inline constexpr std::array<std::uint8_t, 5> SharedPaintLegacyCoordCallBytes{{
    0xE8, 0xE2, 0x06, 0x46, 0x00,
}};
inline constexpr std::array<std::uint8_t, 5> SharedPaintRectCallBytes{{
    0xE8, 0xDE, 0xD1, 0x45, 0x00,
}};
inline constexpr std::array<std::uint8_t, 16> DrawUiFilledRectEntry{{
    0x44, 0x3B, 0xC1, 0x0F, 0x8C, 0xD3, 0x02, 0x00,
    0x00, 0x4C, 0x8B, 0xDC, 0x55, 0x41, 0x54, 0x41,
}};
inline constexpr std::array<std::uint8_t, 19> SetShowItemsStateEntry{{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x30, 0x0F, 0xB6, 0xDA, 0x0F,
    0xB6, 0xD1, 0x48, 0x3B, 0x15, 0xC5, 0x24, 0x16, 0x02,
}};
inline constexpr std::array<std::uint8_t, 18> RefreshShowItemsUiEntry{{
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0x27, 0xC9, 0x12, 0x00, 0x84,
    0xC0, 0x74, 0x54, 0xE8, 0xAE, 0x99, 0xFA, 0xFF,
}};
inline constexpr std::array<std::uint8_t, 17> IsControllerModeEntry{{
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0xF7, 0xE1, 0xFF, 0xFF, 0x80,
    0xB8, 0xC8, 0x0D, 0x00, 0x00, 0x00, 0x75,
}};
inline constexpr std::array<std::uint8_t, 8> IsLoadingScreenActiveEntry{{
    0x0F, 0xB6, 0x05, 0x0E, 0x60, 0x99, 0x02, 0xC3,
}};
inline constexpr std::array<std::uint8_t, 19> FindUiStatePanelEntry{{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x83, 0x3D, 0xD4, 0x9F, 0xBF,
    0x02, 0x00, 0x74, 0x18, 0xE8, 0xFD, 0x18, 0x88, 0xFF,
}};
} // namespace Witnesses

// Accepts any valid native black-label opacity (0 < alpha <= 1).
[[nodiscard]] inline bool IsKnownNativeBlackBackground(const std::array<float, 4>& rgba) noexcept {
    constexpr float kChannelEpsilon = 0.001f;
    for (float c : rgba) {
        if (!std::isfinite(c)) return false;
    }
    return std::fabs(rgba[0]) <= kChannelEpsilon &&
           std::fabs(rgba[1]) <= kChannelEpsilon &&
           std::fabs(rgba[2]) <= kChannelEpsilon &&
           rgba[3] > 0.0f && rgba[3] <= 1.001f;
}

// Helper for verifying whether a ground label formatter invocation has a paired record.
[[nodiscard]] inline void* ResolveEffectiveFormatterRecord(
    bool firstCall, bool secondCall, bool controllerCall,
    void* dest, void* record, bool& outPaired) noexcept {
    const auto destAddr = reinterpret_cast<std::uintptr_t>(dest);
    const auto recAddr = reinterpret_cast<std::uintptr_t>(record);
    if ((secondCall || controllerCall) && dest && destAddr >= Rva::GroundTextOffset &&
        destAddr <= UINTPTR_MAX - Rva::InnerNameBufferBytes) {
        outPaired = (secondCall ? (record != nullptr) : true);
        return reinterpret_cast<void*>(destAddr - Rva::GroundTextOffset);
    }
    if (firstCall && record && dest && recAddr <= UINTPTR_MAX - Rva::GroundTextOffset &&
        destAddr == recAddr + Rva::GroundTextOffset) {
        outPaired = true;
        return record;
    }
    outPaired = false;
    return record;
}

// Resets a candidate ground-label record to its unpopulated frame-init state when
// LabelFormatter (0x1FA9F0) rejects a hidden item before calling the original function,
// preventing hidden items from consuming D2R's 32-label cap (0x1517149 / 0x151A23D)
// or occupying boxes in D2R's 16-step screen-space collision solver (0x1516A90 / 0x15190A0).
inline void ResetRejectedFormatterRecord(void* effectiveRecord, void* dest,
                                         bool resetRecordHeader) noexcept {
    constexpr char kNullChar = '\0';
    (void)Reader::WriteWritable(dest, &kNullChar, sizeof(kNullChar));
    if (resetRecordHeader && effectiveRecord) {
        const auto recAddr = reinterpret_cast<std::uintptr_t>(effectiveRecord);
        if (recAddr <= UINTPTR_MAX - Rva::GroundTextOffset) {
            constexpr std::uint32_t kZeroDim[2]{0U, 0U};
            constexpr std::uint32_t kInvalidId = 0xFFFFFFFFU;
            (void)Reader::WriteWritable(reinterpret_cast<void*>(recAddr + 0x08U),
                                        kZeroDim, sizeof(kZeroDim));
            (void)Reader::WriteWritable(
                reinterpret_cast<void*>(recAddr + Rva::GroundRecordIdOffset),
                &kInvalidId, sizeof(kInvalidId));
        }
    }
}

using LabelFormatterFn = std::uint8_t(__fastcall*)(
    void*, void*, void*, std::uint32_t, std::uint64_t, std::uint64_t) noexcept;
using InnerNameWriterFn = std::uint64_t(__fastcall*)(
    void*, void*, std::uint32_t, void*) noexcept;
using SharedLabelPaintFn = void(__fastcall*)(void*, void*, void*) noexcept;
using CorrectedGlyphBFn = std::uint64_t(__fastcall*)(
    void*, float, float, const float*) noexcept;
using IsHdModeFn = bool(__fastcall*)() noexcept;
using LegacyCoordToScreenFn = std::uint64_t(__fastcall*)(std::uint64_t) noexcept;
using DrawUiFilledRectFn = void(__fastcall*)(
    std::int32_t, std::int32_t, std::int32_t, std::int32_t, const float*, std::int32_t) noexcept;
using SetShowItemsStateFn = void(__fastcall*)(std::uint8_t, bool) noexcept;
using RefreshShowItemsUiFn = void(__fastcall*)() noexcept;
using IsControllerModeFn = bool(__fastcall*)() noexcept;
using IsLoadingScreenActiveFn = bool(__fastcall*)() noexcept;
using FindUiStatePanelFn = void*(__fastcall*)(int) noexcept;

[[nodiscard]] inline bool IsLoadingScreenActive(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    const auto isLoading =
        reinterpret_cast<IsLoadingScreenActiveFn>(exeBase + Rva::IsLoadingScreenActive);
    return isLoading();
}

[[nodiscard]] inline bool IsControllerUiMode(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    const auto isController =
        reinterpret_cast<IsControllerModeFn>(exeBase + Rva::IsControllerMode);
    return isController();
}

[[nodiscard]] inline int ReadFilteredLabelState(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return -1;
    std::uintptr_t count = 0;
    const std::uint8_t* statesPtr = nullptr;
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(exeBase + Rva::ShowItemsStatesCount),
            &count, sizeof(count)) ||
        count != 2U ||
        !Reader::CopyReadable(
            reinterpret_cast<const void*>(exeBase + Rva::ShowItemsStatesPtr),
            &statesPtr, sizeof(statesPtr)) ||
        !statesPtr) {
        return -1;
    }
    std::uint8_t states[2]{0xFFU, 0xFFU};
    if (!Reader::CopyReadable(statesPtr, states, sizeof(states)) ||
        states[0] > 1U || states[1] > 1U) {
        return -1;
    }
    return static_cast<int>(states[0]);
}

[[nodiscard]] inline bool IsItemLabelDisplayActive(std::uintptr_t exeBase) noexcept {
    if (ReadFilteredLabelState(exeBase) != 1) return false;
    if (IsControllerUiMode(exeBase)) {
        return true;
    }
    std::uint8_t panelOpen = 0;
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(
                exeBase + Rva::NativeUiStateTable + Rva::NativeUiItemLabelPanelIndex),
            &panelOpen, sizeof(panelOpen)) ||
        panelOpen == 0U) {
        return false;
    }
    const auto findPanel =
        reinterpret_cast<FindUiStatePanelFn>(exeBase + Rva::FindUiStatePanel);
    const void* const showItemsPanel = findPanel(static_cast<int>(Rva::NativeUiItemLabelPanelIndex));
    std::uint8_t isVisible = 0;
    if (!showItemsPanel ||
        !Reader::CopyReadable(
            reinterpret_cast<const std::uint8_t*>(showItemsPanel) + Rva::OffsetWidgetIsVisible,
            &isVisible, sizeof(isVisible))) {
        return false;
    }
    return isVisible != 0U;
}

// Enables D2R's filtered ground item label placards (channel 0) and refreshes the active
// HUD/ShowItemsPanel state on the UI thread. Leaves channel 1 (unfiltered bypass) untouched.
[[nodiscard]] inline bool EnableShowItemsPlacards(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    if (ReadFilteredLabelState(exeBase) < 0) return false;
    if (IsItemLabelDisplayActive(exeBase)) return true;
    const auto setShowItems =
        reinterpret_cast<SetShowItemsStateFn>(exeBase + Rva::SetShowItemsState);
    const auto refreshShowItems =
        reinterpret_cast<RefreshShowItemsUiFn>(exeBase + Rva::RefreshShowItemsUi);
    setShowItems(0U, false);
    refreshShowItems();
    setShowItems(0U, true);
    refreshShowItems();
    return IsItemLabelDisplayActive(exeBase);
}

// Draws a 2px border around a ground label rectangle using D2R's native UI quad batcher
// (0x657B90, the same primitive SharedLabelPaint 0x1FA8E0 calls at 0x1FA9AD).
inline void DrawLabelBorder(
    const void* rect, const std::array<float, 4>& borderRgba,
    IsHdModeFn isHdMode, LegacyCoordToScreenFn legacyCoordToScreen,
    DrawUiFilledRectFn drawUiFilledRect) noexcept {
    if (!rect || !isHdMode || !legacyCoordToScreen || !drawUiFilledRect) return;
    if (!std::isfinite(borderRgba[3]) || borderRgba[3] <= 0.001f) return;

    std::int32_t rawRect[4]{};
    if (!Reader::CopyReadable(rect, rawRect, sizeof(rawRect))) return;

    std::int32_t left = rawRect[0];
    std::int32_t top = rawRect[1];
    const std::int32_t width = rawRect[2];
    const std::int32_t height = rawRect[3];
    if (width <= 2 || height <= 2) return;

    if (!isHdMode()) {
        const auto centerX = static_cast<std::uint32_t>(left + (width / 2));
        const auto centerY = static_cast<std::uint32_t>(top + (height / 2));
        const std::uint64_t packedCenter =
            (static_cast<std::uint64_t>(centerY) << 32U) | static_cast<std::uint64_t>(centerX);
        const std::uint64_t screenCenter = legacyCoordToScreen(packedCenter);
        const auto screenX = static_cast<std::int32_t>(screenCenter & 0xFFFFFFFFULL);
        const auto screenY = static_cast<std::int32_t>(screenCenter >> 32U);
        left = screenX - (width / 2);
        top = screenY - (height / 2);
    }

    const std::int32_t right = left + width;
    const std::int32_t bottom = top + height;

    // 2px border straddling the label perimeter (1px outset, 1px inset) so glyphs inside
    // the box are untouched and adjacent stacked labels (3px vertical gap in 0x1516A90)
    // still maintain a 1px separation.
    const std::int32_t outerL = left - 1;
    const std::int32_t outerT = top - 1;
    const std::int32_t outerR = right + 1;
    const std::int32_t outerB = bottom + 1;
    const std::int32_t innerL = left + 1;
    const std::int32_t innerT = top + 1;
    const std::int32_t innerR = right - 1;
    const std::int32_t innerB = bottom - 1;

    drawUiFilledRect(outerL, outerT, outerR, innerT, borderRgba.data(), 1);
    drawUiFilledRect(outerL, innerB, outerR, outerB, borderRgba.data(), 1);
    if (innerB > innerT) {
        drawUiFilledRect(outerL, innerT, innerL, innerB, borderRgba.data(), 1);
        drawUiFilledRect(innerR, innerT, outerR, innerB, borderRgba.data(), 1);
    }
}

struct ActiveFormatterScope {
    const void* unit{};
    const void* dest{};
    std::uint32_t unitId{};
    std::uint32_t quantity{1};
    bool hasDecision{};
    Schema::EvaluatedDecision decision{};
};

inline thread_local ActiveFormatterScope g_activeFormatter{};
inline thread_local const float* g_activeGlyphColor = nullptr;

inline constexpr std::uint64_t kShowItemsRetryIntervalMs = 250ULL;
inline constexpr std::uint64_t kShowItemsRetryDeadlineMs = 10000ULL;
inline constexpr std::uint64_t kJoinSettleWindowMs = 1000ULL;
inline constexpr std::uint32_t kMaxJoinSettleRecoveries = 4U;

enum class ShowItemsStepEvents : std::uint8_t {
    None = 0U,
    EnabledOnJoin = 1U << 0U,
    JoinTimedOut = 1U << 1U,
    SwapTimedOut = 1U << 2U,
    AttemptedJoinEnable = 1U << 3U,
};

struct ShowItemsJoinSwapTracker {
    std::atomic<bool> pendingJoin{true};
    std::atomic<bool> pendingControllerSwap{false};
    std::atomic<std::uint64_t> joinReadyStartTickMs{0};
    std::atomic<std::uint64_t> joinSettleStartTickMs{0};
    std::atomic<std::uint64_t> joinLastAttemptTickMs{0};
    std::atomic<std::uint32_t> joinSettleRecoveries{0};
    std::atomic<bool> joinObservedAltUp{false};
    std::atomic<bool> joinLoggedShowItems{false};
    std::atomic<std::uint64_t> swapRetryStartTickMs{0};
    std::atomic<std::uint64_t> swapLastAttemptTickMs{0};
    std::atomic<int> lastObservedControllerMode{-1};

    void ClearJoinPending() noexcept {
        pendingJoin.store(false, std::memory_order_release);
        joinReadyStartTickMs.store(0, std::memory_order_release);
        joinSettleStartTickMs.store(0, std::memory_order_release);
        joinLastAttemptTickMs.store(0, std::memory_order_release);
        joinSettleRecoveries.store(0, std::memory_order_release);
        joinObservedAltUp.store(false, std::memory_order_release);
        joinLoggedShowItems.store(false, std::memory_order_release);
    }

    void ClearSwapPending() noexcept {
        pendingControllerSwap.store(false, std::memory_order_release);
        swapRetryStartTickMs.store(0, std::memory_order_release);
        swapLastAttemptTickMs.store(0, std::memory_order_release);
    }

    void ResetForGameJoin() noexcept {
        pendingJoin.store(true, std::memory_order_release);
        joinReadyStartTickMs.store(0, std::memory_order_release);
        joinSettleStartTickMs.store(0, std::memory_order_release);
        joinLastAttemptTickMs.store(0, std::memory_order_release);
        joinSettleRecoveries.store(0, std::memory_order_release);
        joinObservedAltUp.store(false, std::memory_order_release);
        joinLoggedShowItems.store(false, std::memory_order_release);
        ClearSwapPending();
        lastObservedControllerMode.store(-1, std::memory_order_release);
    }

    void ClearAll() noexcept {
        ClearJoinPending();
        ClearSwapPending();
        lastObservedControllerMode.store(-1, std::memory_order_release);
    }

    [[nodiscard]] bool HasPendingWork() const noexcept {
        return pendingJoin.load(std::memory_order_acquire) ||
               pendingControllerSwap.load(std::memory_order_acquire);
    }

    void ObserveControllerMode(std::uintptr_t exeBase, bool showItemsAdmitted) noexcept {
        if (!showItemsAdmitted || !exeBase) {
            return;
        }
        const int curMode = IsControllerUiMode(exeBase) ? 1 : 0;
        const int prevMode = lastObservedControllerMode.exchange(curMode, std::memory_order_acq_rel);
        if (curMode == 1) {
            ClearSwapPending();
            return;
        }
        if (prevMode == 1 && curMode == 0 && ReadFilteredLabelState(exeBase) == 1) {
            // Native OnInputModeChanged closes ShowItemsPanel when switching
            // from Controller -> Keyboard+Mouse while leaving ShowItemsState[0] == 1.
            swapRetryStartTickMs.store(0, std::memory_order_release);
            swapLastAttemptTickMs.store(0, std::memory_order_release);
            pendingControllerSwap.store(true, std::memory_order_release);
        }
    }

    [[nodiscard]] std::uint8_t StepOnUiThread(std::uintptr_t exeBase, bool showItemsAdmitted,
                                              bool localPlayerReady, bool shouldAutoShow,
                                              std::uint64_t rawNowMs,
                                              bool altKeyHeld = false) noexcept {
        if (!exeBase) return 0U;
        if ((showItemsAdmitted && IsLoadingScreenActive(exeBase)) || !localPlayerReady) {
            joinReadyStartTickMs.store(0, std::memory_order_release);
            joinSettleStartTickMs.store(0, std::memory_order_release);
            joinLastAttemptTickMs.store(0, std::memory_order_release);
            joinSettleRecoveries.store(0, std::memory_order_release);
            joinObservedAltUp.store(false, std::memory_order_release);
            joinLoggedShowItems.store(false, std::memory_order_release);
            return 0U;
        }
        const std::uint64_t nowMs = rawNowMs != 0U ? rawNowMs : 1U;
        std::uint8_t events = 0U;

        if (pendingJoin.load(std::memory_order_acquire)) {
            if (!shouldAutoShow) {
                ClearJoinPending();
                events |= static_cast<std::uint8_t>(ShowItemsStepEvents::AttemptedJoinEnable);
            } else {
                auto readyStartMs = joinReadyStartTickMs.load(std::memory_order_acquire);
                if (readyStartMs == 0U) {
                    readyStartMs = nowMs;
                    joinReadyStartTickMs.store(readyStartMs, std::memory_order_release);
                }
                const auto lastAttemptMs = joinLastAttemptTickMs.load(std::memory_order_acquire);
                if (lastAttemptMs != 0U && !altKeyHeld) {
                    joinObservedAltUp.store(true, std::memory_order_release);
                }
                if (lastAttemptMs != 0U && altKeyHeld &&
                    joinObservedAltUp.load(std::memory_order_acquire)) {
                    // Player manually pressed Alt after our initial activation attempt.
                    ClearJoinPending();
                    events |= static_cast<std::uint8_t>(ShowItemsStepEvents::AttemptedJoinEnable);
                } else if (IsItemLabelDisplayActive(exeBase)) {
                    events |= static_cast<std::uint8_t>(ShowItemsStepEvents::AttemptedJoinEnable);
                    auto settleStartMs = joinSettleStartTickMs.load(std::memory_order_acquire);
                    if (settleStartMs == 0U) {
                        settleStartMs = nowMs;
                        joinSettleStartTickMs.store(settleStartMs, std::memory_order_release);
                        if (!joinLoggedShowItems.exchange(true, std::memory_order_acq_rel)) {
                            events |= static_cast<std::uint8_t>(ShowItemsStepEvents::EnabledOnJoin);
                        }
                    }
                    if (nowMs >= settleStartMs && (nowMs - settleStartMs) >= kJoinSettleWindowMs) {
                        ClearJoinPending();
                    }
                } else {
                    const bool wasSettling =
                        joinSettleStartTickMs.exchange(0, std::memory_order_acq_rel) != 0U;
                    if (nowMs >= readyStartMs &&
                        (nowMs - readyStartMs) >= kShowItemsRetryDeadlineMs) {
                        ClearJoinPending();
                        events |=
                            static_cast<std::uint8_t>(ShowItemsStepEvents::AttemptedJoinEnable) |
                            static_cast<std::uint8_t>(ShowItemsStepEvents::JoinTimedOut);
                    } else {
                        const auto recoveries =
                            joinSettleRecoveries.load(std::memory_order_acquire);
                        const bool allowImmediateSettleRecovery =
                            wasSettling && recoveries < kMaxJoinSettleRecoveries;
                        if (lastAttemptMs == 0U || allowImmediateSettleRecovery ||
                            (nowMs >= lastAttemptMs &&
                             (nowMs - lastAttemptMs) >= kShowItemsRetryIntervalMs)) {
                            if (allowImmediateSettleRecovery) {
                                joinSettleRecoveries.store(recoveries + 1U,
                                                           std::memory_order_release);
                            }
                            joinLastAttemptTickMs.store(nowMs, std::memory_order_release);
                            if (!altKeyHeld) {
                                joinObservedAltUp.store(true, std::memory_order_release);
                            }
                            const bool active = EnableShowItemsPlacards(exeBase);
                            events |=
                                static_cast<std::uint8_t>(ShowItemsStepEvents::AttemptedJoinEnable);
                            if (active) {
                                joinSettleStartTickMs.store(nowMs, std::memory_order_release);
                                if (!joinLoggedShowItems.exchange(true,
                                                                  std::memory_order_acq_rel)) {
                                    events |= static_cast<std::uint8_t>(
                                        ShowItemsStepEvents::EnabledOnJoin);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (pendingControllerSwap.load(std::memory_order_acquire)) {
            if (!showItemsAdmitted || IsControllerUiMode(exeBase) ||
                ReadFilteredLabelState(exeBase) != 1 || IsItemLabelDisplayActive(exeBase)) {
                ClearSwapPending();
            } else {
                auto swapStartMs = swapRetryStartTickMs.load(std::memory_order_acquire);
                if (swapStartMs == 0U) {
                    swapStartMs = nowMs;
                    swapRetryStartTickMs.store(swapStartMs, std::memory_order_release);
                }
                const auto lastAttemptMs = swapLastAttemptTickMs.load(std::memory_order_acquire);
                if (nowMs >= swapStartMs && (nowMs - swapStartMs) >= kShowItemsRetryDeadlineMs) {
                    ClearSwapPending();
                    events |= static_cast<std::uint8_t>(ShowItemsStepEvents::SwapTimedOut);
                } else if (lastAttemptMs == 0U ||
                           (nowMs >= lastAttemptMs &&
                            (nowMs - lastAttemptMs) >= kShowItemsRetryIntervalMs)) {
                    swapLastAttemptTickMs.store(nowMs, std::memory_order_release);
                    if (EnableShowItemsPlacards(exeBase)) {
                        ClearSwapPending();
                    }
                }
            }
        }

        return events;
    }
};

} // namespace PyreLootFilter::Labels
