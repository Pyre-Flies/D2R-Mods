#pragma once
#include <D2RLPlugin/api.h>
#include "bulk_stash_policy.h"

namespace InventoryDepositEligibility {
using Info = D2RL::Items::ItemInfo;
enum class Result { Unavailable, Ineligible, Eligible };
using Inspect = Result (*)(const Info&) noexcept;

// UI-thread read only. Complete a bounded snapshot before inspecting any item;
// never call back into SDK item services from their enumeration callback.
// Reuse the batch's page-zero scope, deduplication and 256-item limit.
inline bool Any(const D2RL::PluginContext* context, const D2RL::InventoryService* inventory,
    const D2RL::ItemService* items, Inspect inspect) noexcept {
    if (!context || !inventory || !items || !inspect || !inventory->getLocalPlayer ||
        !inventory->forEachInventoryItem || !items->getItemInfo) return false;
    D2RL::PlayerHandle player{};
    if (inventory->getLocalPlayer(context, &player) != D2RL::Inventory::Result::Success ||
        player == D2RL::InvalidPlayerHandle) return false;
    struct Snapshot { QolBulkStash::Plan plan{}; bool invalid{}; } snapshot;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize, 0,
        D2RL::Items::ContainerBit(D2RL::Items::ItemContainer::Inventory), 0};
    const auto enumerated = inventory->forEachInventoryItem(context, player, &filter,
        [](const D2RL::PluginContext*, const Info* item, void* user) noexcept {
            auto& snapshot = *static_cast<Snapshot*>(user);
            if (!item || item->structSize < D2RL::Items::ItemInfoSize) {
                snapshot.invalid = true;
                return D2RL::Inventory::IterationAction::Stop;
            }
            snapshot.plan.Observe(*item);
            return snapshot.plan.overflow ? D2RL::Inventory::IterationAction::Stop
                : D2RL::Inventory::IterationAction::Continue;
        }, &snapshot);
    if (enumerated != D2RL::Inventory::Result::Success || snapshot.invalid || snapshot.plan.overflow)
        return false;
    for (unsigned i = 0; i < snapshot.plan.count; ++i) {
        const auto& expected = snapshot.plan.items[i];
        Info live{.structSize = D2RL::Items::ItemInfoSize};
        if (items->getItemInfo(context, expected.handle, &live) != D2RL::Items::Result::Success ||
            live.structSize < D2RL::Items::ItemInfoSize || !QolBulkStash::Source(live) ||
            !QolClientTransfer::Source(expected, live)) return false;
        const auto result = inspect(live);
        if (result == Result::Unavailable) return false;
        if (result == Result::Eligible) {
            D2RL::PlayerHandle current{};
            return inventory->getLocalPlayer(context, &current) == D2RL::Inventory::Result::Success &&
                current == player;
        }
    }
    return false;
}
}
