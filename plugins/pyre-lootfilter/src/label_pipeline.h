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

} // namespace PyreLootFilter::Labels
