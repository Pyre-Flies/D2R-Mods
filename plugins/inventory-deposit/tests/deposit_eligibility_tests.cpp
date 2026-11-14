#include "deposit_eligibility.h"
#include "bulk_stash_policy.h"
#include "native_d2r.h"
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
        Check(!f.Any() && f.inspections == 2, "all ineligible items (including full-stack materials) disable the button");
    }
    {
        Fixture f;
        // Full-stack or item-scoped locked material (Ineligible) before a non-full material (Eligible)
        // must not abort the scan; the button stays enabled for the remaining eligible item.
        f.Add(Item(1), Eligibility::Ineligible);
        f.Add(Item(2), Eligibility::Ineligible);
        f.Add(Item(3), Eligibility::Eligible);
        Check(f.Any() && f.inspections == 3,
            "full-stack or item-locked ineligible entries are skipped so later eligible materials keep the button enabled");
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

void StackCapacityTests() {
    using Plan = QolBulkStash::Plan;
    Check(!Plan::HasStackRoom(99, 99, false, 1) && !Plan::HasStackRoom(98, 99, true, 1),
        "already-full or nativeFull advanced-stash counter has no stack room");
    Check(Plan::HasStackRoom(98, 99, false, 1) && Plan::HasStackRoom(98, 99, false, 0) &&
        Plan::HasStackRoom(98, 99, false, -3),
        "single-unit and non-positive stack quantities normalize to 1 and fit at 98/99");
    Check(!Plan::HasStackRoom(98, 99, false, 5) && !Plan::HasStackRoom(95, 99, false, 5),
        "quantity-5 stack against 98/99 or 95/99 is refused instead of overshooting 99");
    Check(Plan::HasStackRoom(94, 99, false, 5) && Plan::HasStackRoom(98, 500, false, 5),
        "quantity-5 stack fits when remaining capacity is at least 5");

    Plan plan{};
    auto first = Item(1); first.classId = 610u; first.quantity = 5;
    auto second = Item(2); second.classId = 610u; second.quantity = 5;
    auto third = Item(3); third.classId = 610u; third.quantity = 4;
    plan.Observe(first);
    plan.Observe(second);
    plan.Observe(third);

    Check(plan.EffectiveCount(610u, 90u) == 90u &&
        Plan::HasStackRoom(plan.EffectiveCount(610u, 90u), 99u, false, first.quantity),
        "first quantity-5 stack fits at 90/99");
    plan.Submitted(0);
    Check(plan.SubmittedForClass(610u) == 5u && plan.EffectiveCount(610u, 90u) == 95u,
        "submitted quantity-5 stack projects counter to 95/99");
    Check(!Plan::HasStackRoom(plan.EffectiveCount(610u, 90u), 99u, false, second.quantity),
        "second quantity-5 stack against projected 95/99 is refused");
    Check(Plan::HasStackRoom(plan.EffectiveCount(610u, 90u), 99u, false, third.quantity),
        "later quantity-4 stack against projected 95/99 still fits");

    // Server/client stack cap mismatch (e.g. client limit 255 vs server limit 99):
    // when a submitted deposit expires while the item remains unchanged in inventory,
    // CheckRemote returns Rejected and Plan skips that item class while still attempting other classes.
    Plan mismatchPlan{};
    auto cappedA = Item(10); cappedA.classId = 610u; cappedA.quantity = 1;
    auto cappedB = Item(11); cappedB.classId = 610u; cappedB.quantity = 1;
    auto otherRune = Item(12); otherRune.classId = 611u; otherRune.quantity = 1;
    mismatchPlan.Observe(cappedA);
    mismatchPlan.Observe(cappedB);
    mismatchPlan.Observe(otherRune);

    Check(QolBulkStash::CheckRemote(cappedA, &cappedA, true, true) == QolBulkStash::RemoteStep::Rejected,
        "expired remote submission with unchanged source is treated as per-class rejection rather than batch abort");
    auto movedCapped = cappedA; ++movedCapped.x;
    Check(QolBulkStash::CheckRemote(cappedA, &movedCapped, true, true) == QolBulkStash::RemoteStep::Stop,
        "moved source during remote wait still stops the batch");

    mismatchPlan.Submitted(0);
    Check(mismatchPlan.submitted == 1u && mismatchPlan.SubmittedForClass(610u) == 1u,
        "initial capped item submission recorded before server rejection");
    mismatchPlan.RejectSubmitted(0);
    Check(mismatchPlan.submitted == 0u && mismatchPlan.skipped == 1u &&
        mismatchPlan.SubmittedForClass(610u) == 0u &&
        mismatchPlan.IsClassRejected(610u, 1) && !mismatchPlan.IsClassRejected(611u, 1),
        "RejectSubmitted rolls back unremoved submission, marks classId 610 rejected, and leaves classId 611 eligible");

    Check(Plan::IsServerCapCandidate(99u, 255u, 1, 99u) &&
        Plan::IsServerCapCandidate(97u, 255u, 5, 99u) &&
        !Plan::IsServerCapCandidate(50u, 255u, 1, 99u) &&
        !Plan::IsServerCapCandidate(99u, 99u, 1, 99u),
        "only remote timeouts against the default 99 server cap when local limit > 99 qualify as server-cap rejections");
    Check(Plan::ServerCapRejectedQuantity(99u, 99u) == 1u &&
        Plan::ServerCapRejectedQuantity(97u, 99u) == 3u,
        "ServerCapRejectedQuantity computes the overflow threshold quantity for both full and near-full counters");

    Plan multiUnitMismatchPlan{};
    auto stackFive = Item(20); stackFive.classId = 610u; stackFive.quantity = 5;
    multiUnitMismatchPlan.Observe(stackFive);
    multiUnitMismatchPlan.Submitted(0);
    multiUnitMismatchPlan.RejectSubmitted(0, static_cast<int32_t>(Plan::ServerCapRejectedQuantity(99u, 99u)));
    Check(multiUnitMismatchPlan.IsClassRejected(610u, 1),
        "multi-unit stack rejected at count 99 also marks single-unit stacks of the same class rejected");

    {
        auto* fakeImage = static_cast<unsigned char*>(
            VirtualAlloc(nullptr, D2R::Native::FindAdvancedStashCounterRva + 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        Check(fakeImage != nullptr, "synthetic native image allocated");
        const auto fakeBase = reinterpret_cast<uintptr_t>(fakeImage);
        uint32_t fakeItem[4]{4u, 610u, 1u, 0u};
        uint32_t fakePlayer[4]{0u, 0u, 1u, 0u};

        Check(!D2R::Native::HasVerifiedAdvancedStashCounterContract(fakeBase) &&
            !D2R::Native::DepositToAdvancedStash(fakeBase, fakeItem, fakePlayer, 0, 1),
            "unwitnessed exeBase refuses DepositToAdvancedStash before calling native functions");

        std::memcpy(fakeImage + D2R::Native::FindAdvancedStashCounterRva,
            D2R::Native::FindAdvancedStashCounterWitness,
            sizeof(D2R::Native::FindAdvancedStashCounterWitness));
        std::memcpy(fakeImage + D2R::Native::GetAdvancedStashCounterByteRva,
            D2R::Native::GetAdvancedStashCounterByteWitness,
            sizeof(D2R::Native::GetAdvancedStashCounterByteWitness));
        Check(D2R::Native::HasVerifiedAdvancedStashCounterContract(fakeBase),
            "exact 0x46D8F0 + 0x46D9A0 witnesses admit advanced-stash counter contract");

        Check(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase) == 0,
            "unwitnessed D2RCore base falls back safely to 0 (default 99 cap, no D2RCore calls)");
        std::memcpy(fakeImage + D2R::Native::CoreGetMaterialsLimitRva,
            D2R::Native::CoreMaterialsLimitWitness,
            sizeof(D2R::Native::CoreMaterialsLimitWitness));
        std::memcpy(fakeImage + D2R::Native::CoreGetCounterFullStatusRva,
            D2R::Native::CoreCounterFullWitness,
            sizeof(D2R::Native::CoreCounterFullWitness));
        Check(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase) == fakeBase,
            "exact 0x1BB960 + 0x3A0F20 witnesses admit D2RCore stack-limit queries");
        fakeImage[D2R::Native::CoreGetMaterialsLimitRva] ^= 0xFFu;
        Check(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase) == 0,
            "mutated D2RCore byte immediately rejects D2RCore stack-limit queries");

        VirtualFree(fakeImage, 0, MEM_RELEASE);
    }

    D2R::Native::ClearAdvancedStashRejections();
    Check(!D2R::Native::IsAdvancedStashCountRejected(610u, 99u, 1),
        "no class is rejected before a deposit failure is observed");
    D2R::Native::RecordAdvancedStashRejection(610u, 50u, 1);
    Check(!D2R::Native::IsAdvancedStashCountRejected(610u, 50u, 1),
        "failures below the default 99 server cap are not cached as server-cap rejections");
    D2R::Native::RecordAdvancedStashRejection(610u, 99u, 5);
    Check(D2R::Native::IsAdvancedStashCountRejected(610u, 99u, 1) &&
        !D2R::Native::IsAdvancedStashCountRejected(611u, 99u, 1),
        "session cap cache remembers server rejection for classId 610 at count 99 (even from a quantity-5 stack) without blocking classId 611");
    Check(!D2R::Native::IsAdvancedStashCountRejected(610u, 98u, 1) &&
        !D2R::Native::IsAdvancedStashCountRejected(610u, 99u, 1),
        "changing counter count (e.g. withdrawing from stash) clears the cached rejection for classId 610");
    D2R::Native::RecordAdvancedStashRejection(610u, 97u, 5);
    Check(D2R::Native::IsAdvancedStashCountRejected(610u, 97u, 5) &&
        D2R::Native::IsAdvancedStashCountRejected(610u, 97u, 3) &&
        !D2R::Native::IsAdvancedStashCountRejected(610u, 97u, 2) &&
        !D2R::Native::IsAdvancedStashCountRejected(610u, 97u, 1),
        "multi-quantity rejection at 97 blocks overflow quantities (>= 3) while still allowing quantities 1..2");
    D2R::Native::ClearAdvancedStashRejections();
    Check(!D2R::Native::IsAdvancedStashCountRejected(610u, 97u, 5),
        "ClearAdvancedStashRejections resets cached rejections on session change or shutdown");
}
}

int main() {
    AvailabilityTests();
    FailureTests();
    StackCapacityTests();
    std::puts("inventory eligibility scope, bounded SDK reads, identity, stack capacity, failure, and session regressions passed");
}
