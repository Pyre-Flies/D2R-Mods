#pragma once
// Direct Automap & Corner-Minimap ground-item icon pipeline:
//   - 0xD76E0 (AutomapRenderUnit): Observes ground items (type == 4) directly on every automap
//     render pass and projects active markers to screen coordinates via 0xD4910.
//   - D2RL::OverlayService: Renders Circle, Diamond, Triangle, and Star icons inside D2RLoader's
//     native overlay pass with zero MinHook or Dear ImGui DX12 swapchain hooks.

#include "unit_reader.h"
#include <D2RLPlugin/api.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>

namespace PyreLootFilter::Minimap {

namespace Rva {
inline constexpr std::uintptr_t AutomapRenderUnit = 0xD76E0;
inline constexpr std::uintptr_t ProjectClientToAutomap = 0xD4910;
inline constexpr std::uintptr_t NativeUiStateTable = 0x2A2ADA0;
inline constexpr std::size_t NativeUiAutomapIndex = 10U;

// D2RCore 1.3.1's OverlayService dispatcher (0x4413DC) and pushClip bridge pass float2*
// pointers in RDX/R8 to native ImDrawList::PushClipRect (0xD03A30), which expects two
// packed float2 values by value in RDX/R8. Pushing the automap clip by value with
// intersect=false overrides the corrupted near-zero parent clip for our callback pass.
inline constexpr std::uintptr_t DrawListRva = 0xCEAED0;
inline constexpr std::uintptr_t PushClipRva = 0xD03A30;
inline constexpr std::uintptr_t PopClipRva = 0xD03760;

inline constexpr std::size_t ClipLeftOffset = 0x18U;
inline constexpr std::size_t ClipTopOffset = 0x1CU;
inline constexpr std::size_t ClipWidthOffset = 0x20U;
inline constexpr std::size_t ClipHeightOffset = 0x24U;
} // namespace Rva

namespace Witnesses {
inline constexpr std::array<std::uint8_t, 32> AutomapRenderUnitEntry{{
    0x48, 0x89, 0x6C, 0x24, 0x10, 0x57, 0x48, 0x83,
    0xEC, 0x40, 0x48, 0x8B, 0xFA, 0x4C, 0x8D, 0x44,
    0x24, 0x68, 0x48, 0x8D, 0x54, 0x24, 0x60, 0x48,
    0x8B, 0xE9, 0xE8, 0xF1, 0x01, 0x00, 0x00, 0x84,
}};
inline constexpr std::array<std::uint8_t, 32> ProjectClientToAutomapEntry{{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
    0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20,
    0x66, 0x0F, 0x6E, 0x41, 0x10, 0x4C, 0x8B, 0xFA,
}};
inline constexpr std::array<std::uint8_t, 36> DrawListBytes{{
    0x48, 0x8B, 0x0D, 0x79, 0x5F, 0x80, 0x02, 0x48, 0x8B, 0x81, 0x80, 0x1A,
    0x00, 0x00, 0xC6, 0x80, 0x80, 0x00, 0x00, 0x00, 0x01, 0x48, 0x8B, 0x81,
    0x80, 0x1A, 0x00, 0x00, 0x48, 0x8B, 0x80, 0xB0, 0x02, 0x00, 0x00, 0xC3,
}};
inline constexpr std::array<std::uint8_t, 69> PushClipBytes{{
    0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x40,
    0x48, 0x89, 0x54, 0x24, 0x50, 0x48, 0x8B, 0xF9, 0xF3, 0x0F, 0x10, 0x4C,
    0x24, 0x50, 0xF3, 0x0F, 0x10, 0x44, 0x24, 0x54, 0x4C, 0x89, 0x44, 0x24,
    0x20, 0xF3, 0x0F, 0x10, 0x5C, 0x24, 0x20, 0xF3, 0x0F, 0x10, 0x54, 0x24,
    0x24, 0xF3, 0x0F, 0x11, 0x48, 0xE8, 0xF3, 0x0F, 0x11, 0x40, 0xEC, 0xF3,
    0x0F, 0x11, 0x58, 0xF0, 0xF3, 0x0F, 0x11, 0x50, 0xF4,
}};
inline constexpr std::array<std::uint8_t, 32> PopClipBytes{{
    0x8B, 0x41, 0x60, 0x83, 0xE8, 0x01, 0x89, 0x41, 0x60, 0x75, 0x17, 0x48,
    0x8B, 0x41, 0x38, 0x48, 0x83, 0xC0, 0x1C, 0x0F, 0x10, 0x00, 0x0F, 0x11,
    0x81, 0x90, 0x00, 0x00, 0x00, 0xE9, 0xDE, 0x3B,
}};
} // namespace Witnesses

struct Point {
    std::int32_t x{};
    std::int32_t y{};
};

struct ClipRect {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t width{};
    std::int32_t height{};
};

[[nodiscard]] constexpr bool WorldSubtileToClient(std::int32_t worldX, std::int32_t worldY,
                                                  Point& out) noexcept {
    const auto x = std::int64_t{16} * (static_cast<std::int64_t>(worldX) - worldY);
    const auto y = std::int64_t{8} * (static_cast<std::int64_t>(worldX) + worldY);
    constexpr auto kMin = static_cast<std::int64_t>((std::numeric_limits<std::int32_t>::min)());
    constexpr auto kMax = static_cast<std::int64_t>((std::numeric_limits<std::int32_t>::max)());
    if (x < kMin || x > kMax || y < kMin || y > kMax) {
        out = {};
        return false;
    }
    out = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
    return true;
}

[[nodiscard]] constexpr std::uint64_t PackClientCoordinates(Point pt) noexcept {
    return static_cast<std::uint64_t>(static_cast<std::uint32_t>(pt.x)) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(pt.y)) << 32U);
}

[[nodiscard]] constexpr bool PlausibleClip(ClipRect clip) noexcept {
    return clip.width > 0 && clip.height > 0 &&
           clip.width <= 32768 && clip.height <= 32768 &&
           clip.left >= -32768 && clip.left <= 32768 &&
           clip.top >= -32768 && clip.top <= 32768;
}

[[nodiscard]] constexpr bool Contains(ClipRect clip, Point pt) noexcept {
    if (!PlausibleClip(clip)) return false;
    const auto right = static_cast<std::int64_t>(clip.left) + clip.width;
    const auto bottom = static_cast<std::int64_t>(clip.top) + clip.height;
    return pt.x >= clip.left && pt.y >= clip.top &&
           static_cast<std::int64_t>(pt.x) < right &&
           static_cast<std::int64_t>(pt.y) < bottom;
}

struct WorldPosition {
    std::int32_t worldX{};
    std::int32_t worldY{};
    std::int32_t clientX{};
    std::int32_t clientY{};
    bool hasClient{};
    bool plausible{};
};

[[nodiscard]] inline WorldPosition ReadUnitWorldPosition(const void* unit) noexcept {
    WorldPosition out{};
    if (!unit) return out;
    const auto unitAddr = reinterpret_cast<std::uintptr_t>(unit);
    std::uintptr_t pathAddr{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(unitAddr + 0x38U), &pathAddr,
                              sizeof(pathAddr)) ||
        !pathAddr)
        return out;
    std::array<std::uint8_t, 0x18> pathBytes{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(pathAddr), pathBytes.data(),
                              pathBytes.size()))
        return out;
    std::uint32_t wx{}, wy{};
    std::memcpy(&out.clientX, pathBytes.data() + 0x08U, 4);
    std::memcpy(&out.clientY, pathBytes.data() + 0x0CU, 4);
    std::memcpy(&wx, pathBytes.data() + 0x10U, 4);
    std::memcpy(&wy, pathBytes.data() + 0x14U, 4);
    out.hasClient = (out.clientX != 0 || out.clientY != 0);
    if (wx > 0 && wy > 0 && wx <= 0xFFFFU && wy <= 0xFFFFU) {
        out.worldX = static_cast<std::int32_t>(wx);
        out.worldY = static_cast<std::int32_t>(wy);
        out.plausible = true;
    } else if (out.hasClient) {
        out.plausible = true;
    }
    return out;
}

struct TrackedIconItem {
    std::uint32_t unitId{};
    std::uint32_t code{};
    std::uint64_t generation{};
    std::uint64_t observedMs{};
    std::int32_t worldX{};
    std::int32_t worldY{};
    std::int32_t clientX{};
    std::int32_t clientY{};
    bool hasClient{};
    bool clientConfirmed{};
    Schema::MinimapShape shape{Schema::MinimapShape::Diamond};
    std::array<float, 4> borderColor{};
    std::array<float, 4> fillColor{};
    float sizePx{Schema::DefaultMinimapSizePx};
};

struct ScreenMarker {
    float x{};
    float y{};
    std::uint32_t unitId{};
    Schema::MinimapShape shape{Schema::MinimapShape::Diamond};
    std::array<float, 4> borderColor{};
    std::array<float, 4> fillColor{};
    float sizePx{Schema::DefaultMinimapSizePx};
};

inline constexpr std::size_t MaxTrackedItems = 256U;
inline constexpr std::size_t MaxScreenMarkers = 128U;
inline constexpr std::uint64_t AutomapItemTtlMs = 5000U;
inline constexpr std::uint64_t MarkerFrameTtlMs = 250U;
inline constexpr std::uint64_t ClientTableScanIntervalMs = 100U;

// Client UnitAny hash table (D2R+0x2A23910): 128 buckets per unit type (1024 bytes per type).
// UnitTypeItem (4) starts at D2R+0x2A23910 + (4 << 10) = D2R+0x2A24910, chained via unit+0x158.
inline constexpr std::uintptr_t ClientItemHashTableRva = 0x2A23910U + (Reader::UnitTypeItem << 10U);
inline constexpr std::size_t ClientItemHashBucketCount = 128U;
inline constexpr std::size_t UnitHashNextOffset = 0x158U;

struct MarkerFrame {
    std::uint64_t publishedMs{};
    D2RL::Overlay::Rect clip{};
    std::size_t count{};
    std::array<ScreenMarker, MaxScreenMarkers> markers;
};

using AutomapRenderUnitFn = void(__fastcall*)(void* unit, void* automapContext) noexcept;
using ProjectClientToAutomapFn = Point*(__fastcall*)(void*, Point*, std::uint64_t) noexcept;

[[nodiscard]] inline void* LookupClientItemUnit(std::uintptr_t exeBase, std::uint32_t unitId,
                                                Reader::UnitHeader& outHdr) noexcept {
    outHdr = {};
    if (!exeBase || !unitId || unitId == UINT32_MAX) return nullptr;
    const std::size_t bucket = static_cast<std::size_t>(unitId & 0x7FU);
    std::uintptr_t nodeAddr = 0;
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(exeBase + ClientItemHashTableRva + bucket * sizeof(std::uintptr_t)),
            &nodeAddr, sizeof(nodeAddr))) {
        return nullptr;
    }
    for (unsigned step = 0; step < 32U && nodeAddr != 0; ++step) {
        void* const candidate = reinterpret_cast<void*>(nodeAddr);
        Reader::UnitHeader hdr{};
        if (!Reader::ReadUnitHeader(candidate, hdr)) break;
        if (hdr.unitId == unitId) {
            outHdr = hdr;
            return candidate;
        }
        std::uintptr_t nextAddr = 0;
        if (!Reader::CopyReadable(reinterpret_cast<const void*>(nodeAddr + UnitHashNextOffset),
                                  &nextAddr, sizeof(nextAddr)) ||
            nextAddr == nodeAddr) {
            break;
        }
        nodeAddr = nextAddr;
    }
    return nullptr;
}

class MinimapState {
public:
    void Clear() noexcept {
        {
            std::lock_guard lock(itemMutex_);
            itemCount_ = 0;
            activeItemCount_.store(0, std::memory_order_relaxed);
            items_.fill({});
        }
        {
            std::lock_guard lock(frameMutex_);
            publishedFrame_.publishedMs = 0;
            publishedFrame_.clip = {};
            publishedFrame_.count = 0;
            publishedCount_.store(0, std::memory_order_relaxed);
        }
        lastScanMs_.store(0, std::memory_order_relaxed);
        lastLocalPassMs_.store(0, std::memory_order_relaxed);
        loggedMatch_.store(false, std::memory_order_relaxed);
        loggedPublish_.store(false, std::memory_order_relaxed);
        loggedDraw_.store(false, std::memory_order_relaxed);
    }

    void ForgetUnit(std::uint32_t unitId) noexcept {
        if (!unitId || activeItemCount_.load(std::memory_order_relaxed) == 0 || !itemMutex_.try_lock()) return;
        for (std::size_t i = 0; i < itemCount_;) {
            if (items_[i].unitId != unitId) {
                ++i;
                continue;
            }
            if (i + 1 < itemCount_) items_[i] = items_[itemCount_ - 1];
            items_[itemCount_ - 1] = {};
            --itemCount_;
        }
        activeItemCount_.store(itemCount_, std::memory_order_relaxed);
        itemMutex_.unlock();
    }

    bool ObserveEvaluatedGroundUnit(void* unit, const Reader::UnitHeader& hdr,
                                    const Schema::ItemFacts& facts,
                                    const Schema::EvaluatedDecision& decision,
                                    std::uint64_t generation,
                                    const D2RL::PluginContext* logCtx = nullptr) noexcept {
        if (!unit || !decision.show || !decision.hasMinimapIcon) {
            ForgetUnit(hdr.unitId);
            return false;
        }
        const auto nowMs = GetTickCount64();

        if (!itemMutex_.try_lock()) return false;
        std::size_t slot = itemCount_;
        bool prevConfirmed = false;
        for (std::size_t i = 0; i < itemCount_; ++i) {
            if (items_[i].unitId == hdr.unitId) {
                slot = i;
                prevConfirmed = items_[i].clientConfirmed;
                if (items_[i].generation == generation && nowMs >= items_[i].observedMs &&
                    (nowMs - items_[i].observedMs) < 50U) {
                    itemMutex_.unlock();
                    return true;
                }
                break;
            }
        }
        itemMutex_.unlock();

        const auto pos = ReadUnitWorldPosition(unit);
        if (!pos.plausible) return false;

        if (!itemMutex_.try_lock()) return false;
        slot = itemCount_;
        for (std::size_t i = 0; i < itemCount_; ++i) {
            if (items_[i].unitId == hdr.unitId) {
                slot = i;
                prevConfirmed = items_[i].clientConfirmed;
                break;
            }
        }
        if (slot == itemCount_) {
            if (itemCount_ < MaxTrackedItems) {
                slot = itemCount_++;
            } else {
                slot = 0;
                for (std::size_t i = 1; i < itemCount_; ++i) {
                    if (items_[i].observedMs < items_[slot].observedMs) slot = i;
                }
            }
        }
        items_[slot] = TrackedIconItem{
            .unitId = hdr.unitId,
            .code = facts.code,
            .generation = generation,
            .observedMs = nowMs,
            .worldX = pos.worldX,
            .worldY = pos.worldY,
            .clientX = pos.clientX,
            .clientY = pos.clientY,
            .hasClient = pos.hasClient,
            .clientConfirmed = prevConfirmed,
            .shape = decision.minimapShape,
            .borderColor = decision.minimapBorderColor,
            .fillColor = decision.minimapFillColor,
            .sizePx = decision.minimapSizePx,
        };
        activeItemCount_.store(itemCount_, std::memory_order_relaxed);
        itemMutex_.unlock();

        if (logCtx && !loggedMatch_.exchange(true, std::memory_order_relaxed)) {
            char codeStr[5]{};
            Tables::CodeToText(facts.code, codeStr);
            char msg[240]{};
            std::snprintf(msg, sizeof(msg),
                          "[PyreLootFilter] Minimap icon observed: unitId=%u code='%s' mode=%u world=%d,%d client=%d,%d sizePx=%.0f",
                          hdr.unitId, codeStr, hdr.mode, pos.worldX, pos.worldY,
                          pos.clientX, pos.clientY, static_cast<double>(decision.minimapSizePx));
            logCtx->LogInfo(msg);
        }
        return true;
    }

    bool ObserveGroundUnit(void* unit, const Schema::FilterRuleTable* table,
                           const Reader::NativeReaders& readers,
                           Cache::UnitEvaluationCache& cache,
                           const D2RL::PluginContext* logCtx = nullptr) noexcept {
        if (!unit || !table || table->minimapIconRules == 0) return false;
        Reader::UnitHeader hdr{};
        if (!Reader::ReadUnitHeader(unit, hdr)) {
            return false;
        }
        if (!Reader::AllowsMode(hdr.mode, Reader::Purpose::VerifiedLabel)) {
            ForgetUnit(hdr.unitId);
            return false;
        }
        Schema::ItemFacts facts{};
        Schema::EvaluatedDecision decision{};
        if (!readers.EvaluateUnit(unit, 0, table, cache, Reader::Purpose::VerifiedLabel, facts,
                                  decision) ||
            !decision.show || !decision.hasMinimapIcon) {
            ForgetUnit(hdr.unitId);
            return false;
        }
        return ObserveEvaluatedGroundUnit(unit, hdr, facts, decision, table->generation, logCtx);
    }

    void ScanClientGroundItems(std::uintptr_t exeBase, const Schema::FilterRuleTable* table,
                               const Reader::NativeReaders& readers,
                               Cache::UnitEvaluationCache& cache,
                               const D2RL::PluginContext* logCtx = nullptr) noexcept {
        if (!exeBase || !table || table->minimapIconRules == 0) return;
        const auto nowMs = GetTickCount64();
        const auto lastMs = lastScanMs_.load(std::memory_order_relaxed);
        if (lastMs != 0 && nowMs >= lastMs && (nowMs - lastMs) < ClientTableScanIntervalMs) {
            return;
        }
        lastScanMs_.store(nowMs, std::memory_order_relaxed);

        std::array<std::uintptr_t, ClientItemHashBucketCount> buckets{};
        if (!Reader::CopyReadable(
                reinterpret_cast<const void*>(exeBase + ClientItemHashTableRva),
                buckets.data(), sizeof(buckets))) {
            return;
        }
        for (std::uintptr_t nodeAddr : buckets) {
            for (unsigned step = 0; step < 16U && nodeAddr != 0; ++step) {
                void* const unit = reinterpret_cast<void*>(nodeAddr);
                Reader::UnitHeader hdr{};
                if (!Reader::ReadUnitHeader(unit, hdr)) break;
                if (Reader::AllowsMode(hdr.mode, Reader::Purpose::VerifiedLabel)) {
                    (void)ObserveGroundUnit(unit, table, readers, cache, logCtx);
                }
                std::uintptr_t nextAddr = 0;
                if (!Reader::CopyReadable(
                        reinterpret_cast<const void*>(nodeAddr + UnitHashNextOffset),
                        &nextAddr, sizeof(nextAddr)) ||
                    nextAddr == nodeAddr) {
                    break;
                }
                nodeAddr = nextAddr;
            }
        }
    }

    void ProjectAndPublish(void* automapContext, ProjectClientToAutomapFn projectFn,
                           std::uintptr_t exeBase, std::uint64_t currentGeneration,
                           bool isLocalPlayerPass,
                           const D2RL::PluginContext* logCtx = nullptr) noexcept {
        if (!automapContext || !projectFn) return;
        const auto nowMs = GetTickCount64();
        if (isLocalPlayerPass) {
            lastLocalPassMs_.store(nowMs, std::memory_order_relaxed);
        } else {
            const auto lastLocal = lastLocalPassMs_.load(std::memory_order_relaxed);
            if (lastLocal != 0 && nowMs >= lastLocal && (nowMs - lastLocal) <= 50U) {
                return;
            }
        }

        if (activeItemCount_.load(std::memory_order_relaxed) == 0) {
            if (isLocalPlayerPass && publishedCount_.load(std::memory_order_relaxed) != 0) {
                std::lock_guard lock(frameMutex_);
                publishedFrame_.publishedMs = 0;
                publishedFrame_.clip = {};
                publishedFrame_.count = 0;
                publishedCount_.store(0, std::memory_order_relaxed);
            }
            return;
        }

        std::array<TrackedIconItem, MaxTrackedItems> active;
        std::size_t activeCount = 0;

        if (!itemMutex_.try_lock()) return;
        for (std::size_t i = 0; i < itemCount_;) {
            auto& it = items_[i];
            if (it.generation != currentGeneration) {
                if (i + 1 < itemCount_) items_[i] = items_[itemCount_ - 1];
                items_[itemCount_ - 1] = {};
                --itemCount_;
                continue;
            }
            if (exeBase) {
                Reader::UnitHeader liveHdr{};
                void* const liveUnit = LookupClientItemUnit(exeBase, it.unitId, liveHdr);
                if (liveUnit) {
                    if (!Reader::AllowsMode(liveHdr.mode, Reader::Purpose::VerifiedLabel)) {
                        // Item is now in inventory, stash, cube, belt, equipped, or on cursor.
                        if (i + 1 < itemCount_) items_[i] = items_[itemCount_ - 1];
                        items_[itemCount_ - 1] = {};
                        --itemCount_;
                        continue;
                    }
                    it.clientConfirmed = true;
                    it.observedMs = nowMs;
                    const auto livePos = ReadUnitWorldPosition(liveUnit);
                    if (livePos.plausible) {
                        it.worldX = livePos.worldX;
                        it.worldY = livePos.worldY;
                        it.clientX = livePos.clientX;
                        it.clientY = livePos.clientY;
                        it.hasClient = livePos.hasClient;
                    }
                } else if (it.clientConfirmed) {
                    // Previously confirmed in the client item table and now removed.
                    if (i + 1 < itemCount_) items_[i] = items_[itemCount_ - 1];
                    items_[itemCount_ - 1] = {};
                    --itemCount_;
                    continue;
                }
            }
            if (nowMs < it.observedMs || (nowMs - it.observedMs) > AutomapItemTtlMs) {
                if (i + 1 < itemCount_) items_[i] = items_[itemCount_ - 1];
                items_[itemCount_ - 1] = {};
                --itemCount_;
                continue;
            }
            active[activeCount++] = it;
            ++i;
        }
        activeItemCount_.store(itemCount_, std::memory_order_relaxed);
        itemMutex_.unlock();

        if (activeCount == 0) {
            if (isLocalPlayerPass && publishedCount_.load(std::memory_order_relaxed) != 0) {
                std::lock_guard lock(frameMutex_);
                publishedFrame_.publishedMs = 0;
                publishedFrame_.clip = {};
                publishedFrame_.count = 0;
                publishedCount_.store(0, std::memory_order_relaxed);
            }
            return;
        }

        const auto* ctxBytes = static_cast<const std::uint8_t*>(automapContext);
        ClipRect clip{};
        if (!Reader::CopyReadable(ctxBytes + Rva::ClipLeftOffset, &clip.left, 4) ||
            !Reader::CopyReadable(ctxBytes + Rva::ClipTopOffset, &clip.top, 4) ||
            !Reader::CopyReadable(ctxBytes + Rva::ClipWidthOffset, &clip.width, 4) ||
            !Reader::CopyReadable(ctxBytes + Rva::ClipHeightOffset, &clip.height, 4) ||
            !PlausibleClip(clip)) {
            return;
        }

        MarkerFrame frame{};
        frame.publishedMs = nowMs;
        frame.clip = {
            static_cast<float>(clip.left),
            static_cast<float>(clip.top),
            static_cast<float>(static_cast<std::int64_t>(clip.left) + clip.width),
            static_cast<float>(static_cast<std::int64_t>(clip.top) + clip.height),
        };

        if (!ProjectTrackedItems(automapContext, projectFn, clip, active.data(), activeCount,
                                 frame)) {
            return;
        }

        if (frame.count > 0 && logCtx && !loggedPublish_.exchange(true, std::memory_order_relaxed)) {
            char msg[240]{};
            std::snprintf(msg, sizeof(msg),
                          "[PyreLootFilter] Minimap frame published: count=%zu firstUnitId=%u x=%.1f y=%.1f clip=%d,%d,%d,%d",
                          frame.count, frame.markers[0].unitId,
                          static_cast<double>(frame.markers[0].x),
                          static_cast<double>(frame.markers[0].y),
                          clip.left, clip.top, clip.width, clip.height);
            logCtx->LogInfo(msg);
        }

        std::lock_guard lock(frameMutex_);
        if (frame.count > 0 || isLocalPlayerPass) {
            publishedFrame_.publishedMs = frame.publishedMs;
            publishedFrame_.clip = frame.clip;
            publishedFrame_.count = frame.count;
            if (frame.count > 0) {
                std::memcpy(publishedFrame_.markers.data(), frame.markers.data(),
                            frame.count * sizeof(ScreenMarker));
            }
            publishedCount_.store(frame.count, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] bool CopyFreshFrame(std::uint64_t nowMs, MarkerFrame& out) noexcept {
        if (publishedCount_.load(std::memory_order_relaxed) == 0) {
            return false;
        }
        std::lock_guard lock(frameMutex_);
        if (!publishedFrame_.count || !publishedFrame_.publishedMs ||
            nowMs < publishedFrame_.publishedMs ||
            (nowMs - publishedFrame_.publishedMs) > MarkerFrameTtlMs) {
            return false;
        }
        out.publishedMs = publishedFrame_.publishedMs;
        out.clip = publishedFrame_.clip;
        out.count = publishedFrame_.count;
        std::memcpy(out.markers.data(), publishedFrame_.markers.data(),
                    publishedFrame_.count * sizeof(ScreenMarker));
        return true;
    }

    [[nodiscard]] bool MarkFirstDrawLogged() noexcept {
        return !loggedDraw_.exchange(true, std::memory_order_relaxed);
    }

private:
    [[nodiscard]] static bool ProjectTrackedItems(void* automapContext,
                                                  ProjectClientToAutomapFn projectFn,
                                                  ClipRect clip,
                                                  const TrackedIconItem* active,
                                                  std::size_t activeCount,
                                                  MarkerFrame& frame) noexcept {
        __try {
            for (std::size_t i = 0; i < activeCount; ++i) {
                const auto& item = active[i];
                Point client{};
                bool hasClient = item.worldX > 0 && item.worldY > 0 &&
                                 WorldSubtileToClient(item.worldX, item.worldY, client);
                if (!hasClient && item.hasClient) {
                    client = {item.clientX, item.clientY};
                    hasClient = true;
                }
                if (!hasClient) continue;
                Point projected{};
                if (projectFn(automapContext, &projected, PackClientCoordinates(client)) !=
                    &projected)
                    continue;
                if (Contains(clip, projected) && frame.count < frame.markers.size()) {
                    auto& m = frame.markers[frame.count++];
                    m.x = static_cast<float>(projected.x);
                    m.y = static_cast<float>(projected.y);
                    m.unitId = item.unitId;
                    m.shape = item.shape;
                    m.borderColor = item.borderColor;
                    m.fillColor = item.fillColor;
                    m.sizePx = item.sizePx;
                }
            }
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    std::mutex itemMutex_;
    std::array<TrackedIconItem, MaxTrackedItems> items_{};
    std::size_t itemCount_{};
    std::atomic<std::size_t> activeItemCount_{0};

    std::mutex frameMutex_;
    MarkerFrame publishedFrame_{};
    std::atomic<std::size_t> publishedCount_{0};

    std::atomic<std::uint64_t> lastScanMs_{0};
    std::atomic<std::uint64_t> lastLocalPassMs_{0};
    std::atomic<bool> loggedMatch_{false};
    std::atomic<bool> loggedPublish_{false};
    std::atomic<bool> loggedDraw_{false};
};

namespace OverlayRender {

[[nodiscard]] constexpr D2RL::Overlay::Color ToOverlayColor(const std::array<float, 4>& c) noexcept {
    return {c[0], c[1], c[2], c[3]};
}

inline void DrawLine(const D2RL::PluginContext* ctx, const D2RL::OverlayService* overlay,
                     D2RL::Overlay::CanvasHandle canvas, float x0, float y0, float x1, float y1,
                     const D2RL::Overlay::Color& color, float thickness) noexcept {
    const D2RL::Overlay::LineRequest req{
        .structSize = sizeof(req),
        .flags = 0,
        .canvas = canvas,
        .start = {x0, y0},
        .end = {x1, y1},
        .color = color,
        .thickness = thickness,
        .reserved = 0,
    };
    overlay->drawLine(ctx, &req);
}

inline void DrawPolygonScanlines(const D2RL::PluginContext* ctx, const D2RL::OverlayService* overlay,
                                 D2RL::Overlay::CanvasHandle canvas,
                                 const D2RL::Overlay::Point* pts, std::size_t n,
                                 const D2RL::Overlay::Color& fill,
                                 const D2RL::Overlay::Color& border,
                                 float radiusPx) noexcept {
    if (n < 3) return;
    if (fill.alpha > 0.001f) {
        float minY = pts[0].y, maxY = pts[0].y;
        for (std::size_t i = 1; i < n; ++i) {
            minY = (std::min)(minY, pts[i].y);
            maxY = (std::max)(maxY, pts[i].y);
        }
        const float spanY = maxY - minY;
        if (spanY >= 1.0f) {
            // Use thicker horizontal scanlines (>= 2px step, capped at 18 rows max) so
            // standard icons use half as many drawLine calls and large icons (up to 128px)
            // never exceed 18 scanline steps.
            constexpr int kMaxScanlineRows = 18;
            const int numRows = std::clamp(
                static_cast<int>(std::ceil(spanY * 0.5f)), 2, kMaxScanlineRows);
            const float stepY = spanY / static_cast<float>(numRows);
            const float lineThickness = (fill.alpha < 0.99f) ? stepY : (stepY + 0.4f);
            std::array<float, 16> xHits{};
            for (int row = 0; row < numRows; ++row) {
                const float y = minY + (static_cast<float>(row) + 0.5f) * stepY;
                std::size_t hitCount = 0;
                for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
                    const float y0 = pts[j].y, y1 = pts[i].y;
                    if ((y0 <= y && y < y1) || (y1 <= y && y < y0)) {
                        const float t = (y - y0) / (y1 - y0);
                        if (hitCount < xHits.size()) {
                            xHits[hitCount++] = pts[j].x + t * (pts[i].x - pts[j].x);
                        }
                    }
                }
                std::sort(xHits.begin(), xHits.begin() + hitCount);
                for (std::size_t k = 0; k + 1 < hitCount; k += 2) {
                    if (xHits[k + 1] > xHits[k]) {
                        DrawLine(ctx, overlay, canvas, xHits[k], y, xHits[k + 1], y, fill,
                                 lineThickness);
                    }
                }
            }
        }
    }
    if (border.alpha > 0.001f) {
        const float borderThickness = std::clamp(radiusPx * 0.12f, 2.0f, 4.0f);
        for (std::size_t i = 0; i < n; ++i) {
            const auto& a = pts[i];
            const auto& b = pts[(i + 1) % n];
            DrawLine(ctx, overlay, canvas, a.x, a.y, b.x, b.y, border, borderThickness);
        }
    }
}

inline void DrawMarker(const D2RL::PluginContext* ctx, const D2RL::OverlayService* overlay,
                       D2RL::Overlay::CanvasHandle canvas, const ScreenMarker& m) noexcept {
    const float r = std::clamp(m.sizePx, Schema::MinMinimapSizePx, Schema::MaxMinimapSizePx) * 0.5f;
    const auto fill = ToOverlayColor(m.fillColor);
    const auto border = ToOverlayColor(m.borderColor);

    if (m.shape == Schema::MinimapShape::Square) {
        const D2RL::Overlay::Rect box{m.x - r, m.y - r, m.x + r, m.y + r};
        if (fill.alpha > 0.001f && overlay->drawFilledRectangle) {
            const D2RL::Overlay::FilledRectangleRequest fillReq{
                .structSize = sizeof(fillReq),
                .flags = 0,
                .canvas = canvas,
                .rect = box,
                .color = fill,
                .rounding = 0.0f,
                .reserved = 0,
            };
            overlay->drawFilledRectangle(ctx, &fillReq);
        }
        if (border.alpha > 0.001f && overlay->drawRectangle) {
            const D2RL::Overlay::RectangleRequest borderReq{
                .structSize = sizeof(borderReq),
                .flags = 0,
                .canvas = canvas,
                .rect = box,
                .color = border,
                .rounding = 0.0f,
                .thickness = std::clamp(r * 0.12f, 2.0f, 4.0f),
            };
            overlay->drawRectangle(ctx, &borderReq);
        }
        return;
    }

    static constexpr std::array<D2RL::Overlay::Point, 16> kUnitCircle{{
        { 1.0000000f,  0.0000000f},
        { 0.9238795f,  0.3826834f},
        { 0.7071068f,  0.7071068f},
        { 0.3826834f,  0.9238795f},
        { 0.0000000f,  1.0000000f},
        {-0.3826834f,  0.9238795f},
        {-0.7071068f,  0.7071068f},
        {-0.9238795f,  0.3826834f},
        {-1.0000000f,  0.0000000f},
        {-0.9238795f, -0.3826834f},
        {-0.7071068f, -0.7071068f},
        {-0.3826834f, -0.9238795f},
        { 0.0000000f, -1.0000000f},
        { 0.3826834f, -0.9238795f},
        { 0.7071068f, -0.7071068f},
        { 0.9238795f, -0.3826834f},
    }};

    if (m.shape == Schema::MinimapShape::Circle) {
        // D2RCore's OverlayService rectangle primitive does not forward corner rounding flags
        // to ImDrawList::AddRect/AddRectFilled, so render circles as a 16-segment polygon.
        std::array<D2RL::Overlay::Point, 16> pts{};
        for (std::size_t i = 0; i < kUnitCircle.size(); ++i) {
            pts[i] = {m.x + kUnitCircle[i].x * r, m.y + kUnitCircle[i].y * r};
        }
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Bullseye) {
        constexpr D2RL::Overlay::Color kTransparent{0.0f, 0.0f, 0.0f, 0.0f};
        if (border.alpha > 0.001f) {
            std::array<D2RL::Overlay::Point, 16> outerPts{};
            for (std::size_t i = 0; i < kUnitCircle.size(); ++i) {
                outerPts[i] = {m.x + kUnitCircle[i].x * r, m.y + kUnitCircle[i].y * r};
            }
            DrawPolygonScanlines(ctx, overlay, canvas, outerPts.data(), outerPts.size(),
                                 kTransparent, border, r);
        }
        if (fill.alpha > 0.001f) {
            const float dotR = r * 0.45f;
            std::array<D2RL::Overlay::Point, 16> innerPts{};
            for (std::size_t i = 0; i < kUnitCircle.size(); ++i) {
                innerPts[i] = {m.x + kUnitCircle[i].x * dotR, m.y + kUnitCircle[i].y * dotR};
            }
            DrawPolygonScanlines(ctx, overlay, canvas, innerPts.data(), innerPts.size(),
                                 fill, kTransparent, dotR);
        }
        return;
    }

    if (m.shape == Schema::MinimapShape::Diamond) {
        const std::array<D2RL::Overlay::Point, 4> pts{{
            {m.x, m.y - r},
            {m.x + r, m.y},
            {m.x, m.y + r},
            {m.x - r, m.y},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Kite) {
        const std::array<D2RL::Overlay::Point, 4> pts{{
            {m.x, m.y - r},
            {m.x + r * 0.80f, m.y - r * 0.30f},
            {m.x, m.y + r},
            {m.x - r * 0.80f, m.y - r * 0.30f},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Hexagon) {
        const float hx = r * 0.5f;
        const float hy = r * 0.8660254f;
        const std::array<D2RL::Overlay::Point, 6> pts{{
            {m.x + r, m.y},
            {m.x + hx, m.y + hy},
            {m.x - hx, m.y + hy},
            {m.x - r, m.y},
            {m.x - hx, m.y - hy},
            {m.x + hx, m.y - hy},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Triangle) {
        const float dx = r * 0.8660254f;
        const float dy = r * 0.5f;
        const std::array<D2RL::Overlay::Point, 3> pts{{
            {m.x, m.y - r},
            {m.x + dx, m.y + dy},
            {m.x - dx, m.y + dy},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::TriangleDown) {
        const float dx = r * 0.8660254f;
        const float dy = r * 0.5f;
        const std::array<D2RL::Overlay::Point, 3> pts{{
            {m.x, m.y + r},
            {m.x - dx, m.y - dy},
            {m.x + dx, m.y - dy},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Spark) {
        const float w = r * 0.28f;
        const std::array<D2RL::Overlay::Point, 8> pts{{
            {m.x, m.y - r},
            {m.x + w, m.y - w},
            {m.x + r, m.y},
            {m.x + w, m.y + w},
            {m.x, m.y + r},
            {m.x - w, m.y + w},
            {m.x - r, m.y},
            {m.x - w, m.y - w},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Cross) {
        const float w = r * 0.36f;
        const std::array<D2RL::Overlay::Point, 12> pts{{
            {m.x - w, m.y - r},
            {m.x + w, m.y - r},
            {m.x + w, m.y - w},
            {m.x + r, m.y - w},
            {m.x + r, m.y + w},
            {m.x + w, m.y + w},
            {m.x + w, m.y + r},
            {m.x - w, m.y + r},
            {m.x - w, m.y + w},
            {m.x - r, m.y + w},
            {m.x - r, m.y - w},
            {m.x - w, m.y - w},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    if (m.shape == Schema::MinimapShape::Chevron) {
        const std::array<D2RL::Overlay::Point, 6> pts{{
            {m.x, m.y - r},
            {m.x + r, m.y + r * 0.25f},
            {m.x + r * 0.65f, m.y + r * 0.75f},
            {m.x, m.y - r * 0.10f},
            {m.x - r * 0.65f, m.y + r * 0.75f},
            {m.x - r, m.y + r * 0.25f},
        }};
        DrawPolygonScanlines(ctx, overlay, canvas, pts.data(), pts.size(), fill, border, r);
        return;
    }

    // 5-pointed Star
    constexpr float kPi = 3.1415926535f;
    const float innerR = r * 0.45f;
    std::array<D2RL::Overlay::Point, 10> starPts{};
    for (std::size_t i = 0; i < 10; ++i) {
        const float angle = -kPi * 0.5f + static_cast<float>(i) * (kPi / 5.0f);
        const float radius = (i % 2 == 0) ? r : innerR;
        starPts[i] = {m.x + std::cos(angle) * radius, m.y + std::sin(angle) * radius};
    }
    DrawPolygonScanlines(ctx, overlay, canvas, starPts.data(), starPts.size(), fill, border, r);
}

[[nodiscard]] inline bool IsNativeAutomapVisible(std::uintptr_t exeBase) noexcept {
    if (!exeBase) return false;
    std::array<std::uint8_t, 0x34> table{};
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(exeBase + Rva::NativeUiStateTable),
            table.data(), table.size())) {
        return false;
    }
    std::uint32_t modeWord = 0;
    std::memcpy(&modeWord, table.data() + 0x30U, sizeof(modeWord));
    return table[Rva::NativeUiAutomapIndex] != 0U || table[31] != 0U || modeWord == 1U;
}

[[nodiscard]] inline void* BeginNativeAutomapClip(std::uintptr_t exeBase,
                                                  const D2RL::Overlay::Rect& rect) noexcept {
    if (!exeBase || !std::isfinite(rect.left) || !std::isfinite(rect.top) ||
        !std::isfinite(rect.right) || !std::isfinite(rect.bottom) ||
        rect.right <= rect.left || rect.bottom <= rect.top) {
        return nullptr;
    }
    __try {
        void* const drawList =
            reinterpret_cast<void*(__fastcall*)()>(exeBase + Rva::DrawListRva)();
        if (!drawList) return nullptr;
        struct Float2 {
            float x;
            float y;
        };
        const Float2 minPt{rect.left, rect.top};
        const Float2 maxPt{rect.right, rect.bottom};
        std::uint64_t packedMin = 0;
        std::uint64_t packedMax = 0;
        std::memcpy(&packedMin, &minPt, sizeof(packedMin));
        std::memcpy(&packedMax, &maxPt, sizeof(packedMax));
        reinterpret_cast<void(__fastcall*)(void*, std::uint64_t, std::uint64_t, bool)>(
            exeBase + Rva::PushClipRva)(drawList, packedMin, packedMax, false);
        return drawList;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

inline void EndNativeAutomapClip(std::uintptr_t exeBase, void* drawList) noexcept {
    if (!exeBase || !drawList) return;
    __try {
        reinterpret_cast<void(__fastcall*)(void*)>(exeBase + Rva::PopClipRva)(drawList);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

inline void RenderMarkerFrame(const D2RL::PluginContext* ctx, const D2RL::OverlayService* overlay,
                              const D2RL::Overlay::Frame* frame, std::uintptr_t exeBase,
                              bool clipCorrectionAdmitted, MinimapState& state) noexcept {
    if (!ctx || !overlay || !frame || !clipCorrectionAdmitted || !IsNativeAutomapVisible(exeBase)) {
        return;
    }
    MarkerFrame snapshot{};
    if (!state.CopyFreshFrame(GetTickCount64(), snapshot)) return;

    void* const drawList = BeginNativeAutomapClip(exeBase, snapshot.clip);
    if (!drawList) return;

    if (state.MarkFirstDrawLogged()) {
        char msg[240]{};
        std::snprintf(msg, sizeof(msg),
                      "[PyreLootFilter] Minimap overlay first draw: markers=%zu firstUnitId=%u x=%.1f y=%.1f viewport=%.0fx%.0f clipCorrection=1",
                      snapshot.count, snapshot.markers[0].unitId,
                      static_cast<double>(snapshot.markers[0].x),
                      static_cast<double>(snapshot.markers[0].y),
                      static_cast<double>(frame->screenWidth),
                      static_cast<double>(frame->screenHeight));
        ctx->LogInfo(msg);
    }

    for (std::size_t i = 0; i < snapshot.count; ++i) {
        DrawMarker(ctx, overlay, frame->canvas, snapshot.markers[i]);
    }

    EndNativeAutomapClip(exeBase, drawList);
}

} // namespace OverlayRender

} // namespace PyreLootFilter::Minimap
