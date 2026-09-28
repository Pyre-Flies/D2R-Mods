#pragma once

#include <cmath>
#include <cstdint>

namespace GuidedArrowProjection {

inline constexpr std::uint16_t SkillId = 22;
inline constexpr std::int32_t MaximumNearDistanceSquared = 64;
inline constexpr double ProjectedDistance = 20.0;

inline bool TryProject(
    std::uint16_t playerX,
    std::uint16_t playerY,
    std::uint16_t targetX,
    std::uint16_t targetY,
    std::uint16_t skillId,
    std::uint16_t& projectedX,
    std::uint16_t& projectedY) noexcept {
    if (skillId != SkillId) return false;

    const auto deltaX = static_cast<std::int32_t>(targetX) - playerX;
    const auto deltaY = static_cast<std::int32_t>(targetY) - playerY;
    const auto distanceSquared = deltaX * deltaX + deltaY * deltaY;
    if (distanceSquared <= 0 || distanceSquared >= MaximumNearDistanceSquared)
        return false;

    const auto scale = ProjectedDistance /
        std::sqrt(static_cast<double>(distanceSquared));
    const auto x = static_cast<std::int32_t>(std::lround(
        static_cast<double>(playerX) + deltaX * scale));
    const auto y = static_cast<std::int32_t>(std::lround(
        static_cast<double>(playerY) + deltaY * scale));
    if (x <= 0 || x >= 65536 || y <= 0 || y >= 65536) return false;

    projectedX = static_cast<std::uint16_t>(x);
    projectedY = static_cast<std::uint16_t>(y);
    return true;
}

}
