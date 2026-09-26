#pragma once
#include <D2RLPlugin/item.h>

namespace QolBelt {
using Container = D2RL::Items::ItemContainer;
constexpr bool SupportedSource(Container c) noexcept {
    return c == Container::Inventory || c == Container::PersonalStash || c == Container::SharedStash;
}
// Preserve the mod's hp/mp families (including its extra tiers). The native
// free-slot function remains the final authority on belt eligibility/capacity.
constexpr bool BeltCandidate(uint32_t code) noexcept {
    const auto a = code & 255U, b = (code >> 8) & 255U;
    return ((a == 'h' || a == 'm') && b == 'p') ||
        code == D2RL::Items::MakeItemCode("rvs") || code == D2RL::Items::MakeItemCode("rvl") ||
        code == D2RL::Items::MakeItemCode("vps") || code == D2RL::Items::MakeItemCode("wms") ||
        code == D2RL::Items::MakeItemCode("yps") || code == D2RL::Items::MakeItemCode("tsc") ||
        code == D2RL::Items::MakeItemCode("isc");
}
inline bool SameItem(const D2RL::Items::ItemInfo& a, const D2RL::Items::ItemInfo& b) noexcept {
    // Runtime unit identity within the lifecycle-bounded session, not a UI
    // handle or PRNG seed (the SDK does not promise cross-thread seed stability).
    return a.runtimeId == b.runtimeId && a.code == b.code && a.classId == b.classId;
}
inline bool SameSource(const D2RL::Items::ItemInfo& a, const D2RL::Items::ItemInfo& b) noexcept {
    return SameItem(a, b) && a.container == b.container && a.x == b.x && a.y == b.y &&
        a.inventoryPage == b.inventoryPage && a.sharedStashPage == b.sharedStashPage;
}
enum class Phase { Ready, AwaitInventory, AwaitBelt };
enum class Observation { Wait, Confirmed, TimedOut, Invalid };
constexpr Observation Observe(Phase phase, Container container, bool found, bool expired) noexcept {
    if (found && ((phase == Phase::AwaitBelt && container == Container::Belt) ||
                 (phase == Phase::AwaitInventory && container == Container::Inventory)))
        return Observation::Confirmed;
    if (found && !SupportedSource(container)) return Observation::Invalid;
    return expired ? Observation::TimedOut : Observation::Wait;
}
}
