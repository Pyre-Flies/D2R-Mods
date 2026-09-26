#pragma once
#include <cstdint>
namespace RollRanges {
struct Panels { bool inventory{}, stash{}, cube{}, vendor{}; };
constexpr bool Allowed(bool foreground, Panels panels) noexcept {
    return foreground && (panels.inventory || panels.stash || panels.cube || panels.vendor);
}
// Only D2RCore's specific tooltip caller receives a substituted Ctrl bit.
constexpr std::uint64_t Keyboard(std::uint64_t original, bool tooltipCaller,
                                bool allowed, bool ctrlHeld) noexcept {
    return tooltipCaller ? ((original & ~std::uint64_t{2}) | (allowed && ctrlHeld ? 2 : 0)) : original;
}
constexpr std::uint64_t Controller(std::uint64_t original, bool tooltipCaller,
                                  bool allowed, bool connected, bool r1Held) noexcept {
    return tooltipCaller ? static_cast<std::uint64_t>(allowed && connected && r1Held) : original;
}
}
