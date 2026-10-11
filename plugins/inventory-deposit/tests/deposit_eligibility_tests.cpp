#include "deposit_eligibility.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

namespace {
using Info = D2RL::Items::ItemInfo;
using Container = D2RL::Items::ItemContainer;
using ScanResult = D2RL::Inventory::Result;
using ItemResult = D2RL::Items::Result;
using Eligibility = InventoryDepositEligibility::Result;

void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

Info Item(uint32_t id, Container container = Container::Inventory, int32_t page = 0) {
    Info item{};
    item.structSize = D2RL::Items::ItemInfoSize;
    item.handle = id;
    item.runtimeId = id;
    item.code = D2RL::Items::MakeItemCode("r01");
    item.classId = 100;
    item.container = container;
    item.inventoryPage = page;
    item.generationSeed = id + 1000;
    item.itemSeed = id + 2000;
    return item;
}

struct Fixture;
Fixture* fixture{};

// Supply copied SDK values, with every mutation entry point left null. SDK
// reads and native inspection fail the test if called during enumeration.
struct Fixture {
    D2RL::PluginContext context{};
    D2RL::InventoryService inventory{};
    D2RL::ItemService items{};
    std::vector<Info> snapshot;
    std::map<D2RL::ItemHandle, Info> live;
    std::map<uint32_t, Eligibility> eligibility;
    ScanResult scanResult{ScanResult::Success}, firstPlayerResult{ScanResult::Success},
        finalPlayerResult{ScanResult::Success};
    ItemResult itemResult{ItemResult::Success};
    D2RL::PlayerHandle firstPlayer{10}, finalPlayer{10};
    bool enumerating{}, nullItem{};
    unsigned playerReads{}, scans{}, visits{}, itemReads{}, inspections{};
    std::vector<uint32_t> inspected;

    Fixture() {
        fixture = this;
        inventory.getLocalPlayer = [](const D2RL::PluginContext* ctx, D2RL::PlayerHandle* player) noexcept {
            auto& f = *fixture;
            Check(ctx == &f.context && !f.enumerating, "player reads stay outside inventory enumeration");
            const bool first = f.playerReads++ == 0;
            *player = first ? f.firstPlayer : f.finalPlayer;
            return first ? f.firstPlayerResult : f.finalPlayerResult;
        };
        inventory.forEachInventoryItem = [](const D2RL::PluginContext* ctx, D2RL::PlayerHandle player,
            const D2RL::Inventory::ItemFilter* filter, D2RL::Inventory::ItemCallback callback, void* user) noexcept {
            auto& f = *fixture;
            Check(ctx == &f.context && player == f.firstPlayer && !f.enumerating,
                "enumeration uses the captured player and does not reenter");
            Check(filter && filter->structSize == D2RL::Inventory::ItemFilterSize && !filter->flags &&
                filter->containerMask == D2RL::Items::ContainerBit(Container::Inventory),
                "SDK enumeration requests inventory only");
            ++f.scans;
            f.enumerating = true;
            if (f.nullItem) {
                ++f.visits;
                (void)callback(ctx, nullptr, user);
            } else for (const auto& entry : f.snapshot) {
                ++f.visits;
                if (callback(ctx, &entry, user) == D2RL::Inventory::IterationAction::Stop) break;
            }
            f.enumerating = false;
            return f.scanResult;
        };
        items.getItemInfo = [](const D2RL::PluginContext* ctx, D2RL::ItemHandle handle, Info* result) noexcept {
            auto& f = *fixture;
            Check(ctx == &f.context && !f.enumerating, "SDK item reads cannot reenter inventory enumeration");
            Check(result && result->structSize == D2RL::Items::ItemInfoSize,
                "item refresh supplies the current SDK structure size");
            ++f.itemReads;
            if (f.itemResult != ItemResult::Success) return f.itemResult;
            const auto found = f.live.find(handle);
            if (found == f.live.end()) return ItemResult::StaleHandle;
            *result = found->second;
            return ItemResult::Success;
        };
    }

    void Add(Info item, Eligibility result = Eligibility::Ineligible) {
        snapshot.push_back(item);
        live[item.handle] = item;
        eligibility[item.runtimeId] = result;
    }

    static Eligibility Inspect(const Info& item) noexcept {
        auto& f = *fixture;
        Check(!f.enumerating && f.visits == f.snapshot.size(),
            "native eligibility runs only after the complete successful snapshot");
        ++f.inspections;
        f.inspected.push_back(item.runtimeId);
        const auto found = f.eligibility.find(item.runtimeId);
        return found == f.eligibility.end() ? Eligibility::Unavailable : found->second;
    }

    bool Any() { return InventoryDepositEligibility::Any(&context, &inventory, &items, Inspect); }
};

void AvailabilityTests() {
    {
        Fixture f;
        Check(!f.Any() && f.scans == 1 && !f.itemReads && !f.inspections,
            "empty inventory disables without inspecting an item");
    }
    {
        Fixture f;
        f.Add(Item(1)); f.Add(Item(2));
        Check(!f.Any() && f.inspections == 2, "all ineligible items disable the button");
    }
    {
        Fixture f;
        f.Add(Item(1)); f.Add(Item(2), Eligibility::Eligible); f.Add(Item(3), Eligibility::Unavailable);
        Check(f.Any() && f.visits == 3 && f.inspections == 2 && f.itemReads == 2 && f.playerReads == 2,
            "complete snapshot stops native inspection at the first eligible item and confirms the player");
    }
    {
        Fixture f;
        for (auto container : {Container::Cube, Container::Belt, Container::Equipment, Container::Cursor,
            Container::PersonalStash, Container::SharedStash, Container::CustomPage, Container::Ground})
            f.Add(Item(static_cast<uint32_t>(f.snapshot.size()) + 1, container), Eligibility::Eligible);
        f.Add(Item(20, Container::Inventory, 1), Eligibility::Eligible);
        f.Add(Item(21));
        Check(!f.Any() && f.itemReads == 1 && f.inspections == 1 && f.inspected[0] == 21,
            "only ordinary inventory page zero is a source even if other containers are returned");
    }
    {
        Fixture f;
        f.Add(Item(1)); f.Add(Item(1)); f.Add(Item(2), Eligibility::Eligible);
        Check(f.Any() && f.itemReads == 2 && f.inspections == 2,
            "duplicate runtime identities are inspected only once");
    }
    {
        Fixture f;
        for (uint32_t id = 1; id <= 256; ++id)
            f.Add(Item(id), id == 256 ? Eligibility::Eligible : Eligibility::Ineligible);
        Check(f.Any() && f.inspections == 256, "the final item within the supported bound can enable");
    }
    {
        Fixture f;
        for (uint32_t id = 1; id <= 258; ++id) f.Add(Item(id), Eligibility::Eligible);
        Check(!f.Any() && f.visits == 257 && !f.itemReads && !f.inspections,
            "snapshot overflow stops enumeration and never admits even an early eligible item");
    }
}

void FailureTests() {
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible); f.scanResult = ScanResult::Unavailable;
        Check(!f.Any() && !f.itemReads && !f.inspections,
            "a failed enumeration cannot enable from a partial successful callback");
    }
    {
        Fixture f;
        f.nullItem = true;
        Check(!f.Any() && !f.itemReads, "null callback data is rejected");
    }
    {
        Fixture f;
        auto malformed = Item(1); --malformed.structSize;
        f.Add(malformed, Eligibility::Eligible);
        Check(!f.Any() && !f.itemReads, "short snapshot structures are rejected before item access");
    }
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible); --f.live.at(1).structSize;
        Check(!f.Any() && !f.inspections, "short refreshed structures cannot reach native inspection");
    }
    for (unsigned change = 0; change < 9; ++change) {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible);
        auto& live = f.live.at(1);
        switch (change) {
        case 0: ++live.runtimeId; break;
        case 1: ++live.code; break;
        case 2: ++live.classId; break;
        case 3: ++live.generationSeed; break;
        case 4: ++live.itemSeed; break;
        case 5: live.container = Container::Cube; break;
        case 6: ++live.inventoryPage; break;
        case 7: ++live.x; break;
        case 8: ++live.y; break;
        }
        Check(!f.Any() && !f.inspections, "a changed identity or moved source cannot reach native inspection");
    }
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible); f.itemResult = ItemResult::StaleHandle;
        Check(!f.Any() && !f.inspections, "stale SDK handles disable rather than using the snapshot as live");
    }
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Unavailable); f.Add(Item(2), Eligibility::Eligible);
        Check(!f.Any() && f.inspections == 1, "unavailable native inspection aborts before a later candidate");
    }
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible); f.finalPlayer = 11;
        Check(!f.Any() && f.playerReads == 2, "session or player change invalidates a positive candidate");
    }
    {
        Fixture f;
        f.Add(Item(1), Eligibility::Eligible); f.finalPlayerResult = ScanResult::Unavailable;
        Check(!f.Any(), "failed final player confirmation invalidates a positive candidate");
    }
    {
        Fixture f;
        f.firstPlayer = D2RL::InvalidPlayerHandle;
        Check(!f.Any() && !f.scans, "missing local player prevents enumeration");
    }
    {
        Fixture f;
        f.firstPlayerResult = ScanResult::Unavailable;
        Check(!f.Any() && !f.scans, "failed local-player capture prevents enumeration");
    }
    {
        Fixture f;
        Check(!InventoryDepositEligibility::Any(nullptr, &f.inventory, &f.items, Fixture::Inspect) &&
            !InventoryDepositEligibility::Any(&f.context, nullptr, &f.items, Fixture::Inspect) &&
            !InventoryDepositEligibility::Any(&f.context, &f.inventory, nullptr, Fixture::Inspect) &&
            !InventoryDepositEligibility::Any(&f.context, &f.inventory, &f.items, nullptr),
            "missing dependencies are rejected");
        f.items.getItemInfo = nullptr;
        Check(!f.Any() && !f.playerReads, "missing required SDK function disables before reads");
    }
}
}

int main() {
    AvailabilityTests();
    FailureTests();
    std::puts("inventory eligibility scope, bounded SDK reads, identity, failure, and session regressions passed");
}
