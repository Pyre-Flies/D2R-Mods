#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>

namespace RunningDefense {

struct Point {
    float x{};
    float y{};
};

struct ScreenRect {
    float x{};
    float y{};
    float width{};
    float height{};
};

struct LayoutNode {
    float x{};
    float y{};
    float width{};
    float height{};
    float anchorX{};
    float anchorY{};
    float scale{1.0f};
    bool fill{};
};

inline bool IsFinitePoint(Point p) noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y);
}

inline bool LayoutBounds(std::span<const LayoutNode> chain, ScreenRect& output) noexcept {
    if (chain.empty() || chain.size() > 16) return false;
    const auto effective = [&](std::size_t i) noexcept -> const LayoutNode* {
        while (i < chain.size() && chain[i].fill) ++i;
        return i < chain.size() ? &chain[i] : nullptr;
    };

    float x = 0.0f;
    float y = 0.0f;
    float scale = chain[0].scale;
    for (std::size_t i = 0; i < chain.size(); ++i) {
        const auto& n = chain[i];
        if (!IsFinitePoint({n.x, n.y}) || !IsFinitePoint({n.width, n.height}) || !IsFinitePoint({n.anchorX, n.anchorY}) ||
            !std::isfinite(n.scale) || n.scale <= 0.0f || n.scale > 8.0f ||
            std::abs(n.x) > 100000.0f || std::abs(n.y) > 100000.0f ||
            n.width < 0.0f || n.height < 0.0f || n.width > 100000.0f || n.height > 100000.0f ||
            std::abs(n.anchorX) > 4.0f || std::abs(n.anchorY) > 4.0f) {
            return false;
        }
        if (!n.fill) {
            x += n.x;
            y += n.y;
        }
        if (i + 1 < chain.size()) {
            const auto* parent = effective(i + 1);
            if (!parent) return false;
            x = (x + parent->width * n.anchorX) * chain[i + 1].scale;
            y = (y + parent->height * n.anchorY) * chain[i + 1].scale;
            scale *= chain[i + 1].scale;
        }
    }

    const auto* rect = effective(0);
    if (!rect) return false;
    output = {x, y, rect->width * scale, rect->height * scale};
    return IsFinitePoint({output.x, output.y}) && IsFinitePoint({output.width, output.height}) &&
           output.width > 0.0f && output.height > 0.0f;
}

constexpr unsigned CountDigits(int value) noexcept {
    unsigned digits = 1;
    unsigned v = value < 0 ? static_cast<unsigned>(-static_cast<long long>(value)) : static_cast<unsigned>(value);
    while (v >= 10U && digits < 8U) {
        v /= 10U;
        ++digits;
    }
    return digits;
}

inline bool ComputeRunningIconPlacement(
    const ScreenRect& box,
    int displayedDefense,
    Point& centerOut,
    float& scaleOut) noexcept
{
    if (!IsFinitePoint({box.x, box.y}) || !IsFinitePoint({box.width, box.height}) ||
        box.width < 28.0f || box.height < 12.0f || box.width > 2000.0f || box.height > 1000.0f) {
        return false;
    }

    // Scale the running figure relative to the ArmorClass box height.
    const float iconRadius = std::clamp(box.height * 0.25f, 4.5f, 22.0f);
    const unsigned digits = CountDigits(displayedDefense);
    // MediumPanelFontSize digits inside ArmorClass (225x86 HD) have width ~0.27 * box.height each.
    const float estimatedTextHalfWidth = static_cast<float>(digits) * (box.height * 0.145f);
    const float boxCenterX = box.x + box.width * 0.5f;
    const float desiredX = boxCenterX + estimatedTextHalfWidth + iconRadius * 1.15f;
    const float maxRightX = box.x + box.width - iconRadius * 1.15f;
    const float minRightX = box.x + box.width * 0.58f;

    centerOut.x = std::clamp(desiredX, minRightX, std::max(minRightX, maxRightX));
    centerOut.y = box.y + box.height * 0.5f;
    scaleOut = iconRadius;
    return IsFinitePoint(centerOut) && std::isfinite(scaleOut) && scaleOut > 0.0f;
}

} // namespace RunningDefense
