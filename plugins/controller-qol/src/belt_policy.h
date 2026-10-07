#pragma once
#include <D2RLPlugin/item.h>

namespace QolBelt {
using Container = D2RL::Items::ItemContainer;
constexpr bool SupportedSource(Container c) noexcept {
    return c == Container::Inventory || c == Container::PersonalStash || c == Container::SharedStash || c==Container::Cube;
}
constexpr bool ClientBeltSource(Container container,uint32_t page) noexcept {
    // SDK inventoryPage is not the native source-page byte. Native resolution
    // separately checks page 0/3/4 and the visible stash/Cube owner context.
    return container==Container::Inventory?page==0:
        container==Container::PersonalStash || container==Container::SharedStash || container==Container::Cube;
}
// Preserve the mod's hp/mp families (including its extra tiers). The native
// free-slot function remains the final authority on belt eligibility/capacity.
constexpr bool BeltCandidate(uint32_t code) noexcept {
    const auto a = code & 255U, b = (code >> 8) & 255U;
    return ((a == 'h' || a == 'm') && b == 'p') ||
        code == D2RL::Items::MakeItemCode("rvs") || code == D2RL::Items::MakeItemCode("rvl") ||
        code == D2RL::Items::MakeItemCode("vps") || code == D2RL::Items::MakeItemCode("wms") ||
        code == D2RL::Items::MakeItemCode("yps");
}
constexpr bool SharedInputCandidate(Container container,uint32_t tab,uint32_t code) noexcept {
    return container==Container::SharedStash && tab==1 && BeltCandidate(code);
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
inline bool BindShared(const D2RL::Items::ItemInfo& requested,const D2RL::Items::ItemInfo& live,uint32_t selectedPage,bool exactFocus) noexcept {
    return exactFocus && SameItem(requested,live) && requested.container==Container::SharedStash &&
        live.container==Container::SharedStash && selectedPage!=UINT32_MAX && live.sharedStashPage==selectedPage;
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
