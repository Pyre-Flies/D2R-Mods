#include "bulk_stash_feedback.h"
#include "stash_feedback.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// Compile the real production bridge and record calls only at its two module
// boundaries. The shared feedback tests exercise mailbox/timer/native behavior.
struct Fixture {
    D2RL::PluginContext context{}, nextContext{};
    const D2RL::PluginContext* feedbackContext{}, *bulkContext{};
    QolBulkStash::Available available{};
    QolBulkStash::Observer observer{};
    std::vector<std::string> events;
    std::vector<D2RL::Items::ItemInfo> removed;
    bool admission{true}, feedbackInitialized{}, bulkInitialized{}, batch{}, allowed{true};
    unsigned feedbackStarts{}, bulkStarts{}, feedbackStops{}, bulkStops{}, requests{}, admissionReads{}, refreshes{};
};
Fixture* fixture{};

bool Available() noexcept { ++fixture->admissionReads; return fixture->allowed; }
bool NextAvailable() noexcept { ++fixture->admissionReads; return !fixture->allowed; }

void WiringTests() {
    Fixture f; fixture = &f;
    QolBulkStashFeedback::Initialize(&f.context, Available);
    Check(f.events == std::vector<std::string>{"feedback.initialize", "bulk.initialize"},
        "feedback receiver initializes before the queue can emit observations");
    Check(f.feedbackContext == &f.context && f.bulkContext == &f.context && f.available == Available,
        "both modules receive the owning context and the unchanged bulk admission callback");
    Check(f.requests == 0 && f.admissionReads == 0,
        "initialization neither starts a deposit nor evaluates transfer admission");
    Check(f.observer.user == nullptr && f.observer.begin && f.observer.removed && f.observer.end,
        "queue receives all callbacks without retaining a stack-owned user pointer");

    f.observer.begin(f.observer.user);
    D2RL::Items::ItemInfo item{};
    item.structSize = D2RL::Items::ItemInfoSize;
    item.runtimeId = 73; item.classId = 1205;
    item.container = D2RL::Items::ItemContainer::Inventory;
    f.observer.removed(f.observer.user, item);
    item.runtimeId = 91; item.classId = 1211;
    f.observer.removed(f.observer.user, item);
    item.runtimeId = 0; item.classId = 0;
    f.observer.end(f.observer.user);
    Check(f.removed.size() == 2 && f.removed[0].runtimeId == 73 && f.removed[0].classId == 1205 &&
        f.removed[1].runtimeId == 91 && f.removed[1].classId == 1211 &&
        f.removed[0].container == D2RL::Items::ItemContainer::Inventory,
        "actual observer forwards original item identities to the copying feedback boundary");
    Check(f.events == std::vector<std::string>{"feedback.initialize", "bulk.initialize", "feedback.begin",
        "feedback.removed", "feedback.removed", "feedback.end"},
        "observer callbacks only forward batch observations without refreshing or requesting");

    for (unsigned i = 0; i < 3; ++i) QolBulkStashFeedback::Refresh();
    Check(f.refreshes == 3 && f.requests == 0 && f.admissionReads == 0,
        "worker refresh only forwards visual work and never requests a deposit");
    f.allowed = false;
    Check(!f.available(), "queue keeps the caller's live transfer restriction");
    f.allowed = true;
    Check(f.available(), "restored eligibility is still evaluated by the original callback");

    f.observer.begin(f.observer.user);
    f.events.clear();
    QolBulkStashFeedback::Shutdown();
    Check(f.events == std::vector<std::string>{"bulk.shutdown", "feedback.end", "feedback.shutdown"},
        "queue shutdown can finish an active batch before its observer receiver stops");
    Check(!f.bulkInitialized && !f.feedbackInitialized && f.requests == 0,
        "shutdown releases both modules without initiating a transfer");
    fixture = nullptr;
}

void OptionalFailureTests() {
    Fixture f; fixture = &f; f.admission = false;
    QolBulkStashFeedback::Initialize(&f.context, Available);
    Check(f.feedbackStarts == 1 && f.bulkStarts == 1 && f.bulkInitialized && f.available == Available,
        "failed optional feedback admission does not disable or replace bulk stash");
    f.observer.begin(f.observer.user);
    D2RL::Items::ItemInfo item{}; item.runtimeId = 4; item.classId = 30;
    f.observer.removed(f.observer.user, item);
    f.observer.end(f.observer.user);
    QolBulkStashFeedback::Refresh();
    Check(f.refreshes == 1 && f.requests == 0 && f.admissionReads == 0,
        "unavailable feedback does not introduce transfers or admission side effects");
    QolBulkStashFeedback::Shutdown();
    Check(f.bulkStops == 1 && f.feedbackStops == 1,
        "optional admission failure still receives orderly teardown");
    fixture = nullptr;
}

void ReloadTests() {
    Fixture f; fixture = &f;
    QolBulkStashFeedback::Initialize(&f.context, Available);
    QolBulkStashFeedback::Shutdown();
    f.events.clear();
    QolBulkStashFeedback::Initialize(&f.nextContext, NextAvailable);
    Check(f.feedbackStarts == 2 && f.bulkStarts == 2 && f.feedbackContext == &f.nextContext &&
        f.bulkContext == &f.nextContext && f.available == NextAvailable,
        "reload binds the new owner and predicate without retaining the prior context");
    Check(f.events == std::vector<std::string>{"feedback.initialize", "bulk.initialize"},
        "reload preserves receiver-before-producer initialization order");
    f.observer.begin(f.observer.user);
    D2RL::Items::ItemInfo item{}; item.runtimeId = 73; item.classId = 40;
    f.observer.removed(f.observer.user, item);
    QolBulkStashFeedback::Refresh();
    QolBulkStashFeedback::Shutdown();
    Check(f.removed.size() == 1 && f.removed.front().runtimeId == 73 && f.removed.front().classId == 40 &&
        f.bulkStops == 2 && f.feedbackStops == 2 && f.requests == 0,
        "reloaded observer forwards fresh observations and tears down without automatic deposits");
    fixture = nullptr;
}
}

namespace InventoryDepositStashFeedback {
bool Initialize(const D2RL::PluginContext* context) noexcept {
    Check(!fixture->feedbackInitialized && !fixture->bulkInitialized, "previous owner stopped before initialization");
    fixture->events.emplace_back("feedback.initialize");
    fixture->feedbackContext = context; fixture->feedbackInitialized = true; ++fixture->feedbackStarts;
    return fixture->admission;
}
void BeginBatch() noexcept {
    Check(fixture->feedbackInitialized, "batch receiver exists before begin");
    fixture->events.emplace_back("feedback.begin"); fixture->batch = true;
}
void ItemRemoved(const D2RL::Items::ItemInfo& item) noexcept {
    Check(fixture->feedbackInitialized && fixture->batch, "removal belongs to a begun batch");
    fixture->events.emplace_back("feedback.removed"); fixture->removed.push_back(item);
}
void EndBatch() noexcept {
    Check(fixture->feedbackInitialized, "batch receiver survives queue completion");
    fixture->events.emplace_back("feedback.end"); fixture->batch = false;
}
void Refresh() noexcept {
    Check(fixture->feedbackInitialized, "worker refresh uses an initialized feedback owner");
    fixture->events.emplace_back("feedback.refresh"); ++fixture->refreshes;
}
void Shutdown() noexcept {
    Check(!fixture->bulkInitialized, "queue stops before feedback teardown");
    fixture->events.emplace_back("feedback.shutdown");
    fixture->feedbackInitialized = false; fixture->feedbackContext = nullptr; ++fixture->feedbackStops;
}
}

namespace QolBulkStash {
void Initialize(const D2RL::PluginContext* context, Available available, const Observer* observer) noexcept {
    Check(fixture->feedbackInitialized && observer, "queue initialization has a ready observer receiver");
    fixture->events.emplace_back("bulk.initialize");
    fixture->bulkContext = context; fixture->available = available; fixture->observer = *observer;
    fixture->bulkInitialized = true; ++fixture->bulkStarts;
}
bool Request() noexcept { ++fixture->requests; return true; }
bool Busy() noexcept { return fixture->batch; }
void Shutdown() noexcept {
    fixture->events.emplace_back("bulk.shutdown");
    // Model the queue's documented cancellation callback, not any transfer or
    // animation behavior. Its receiver must still be alive at this boundary.
    if (fixture->batch && fixture->observer.end) fixture->observer.end(fixture->observer.user);
    fixture->bulkInitialized = false; fixture->bulkContext = nullptr;
    fixture->available = nullptr; fixture->observer = {}; ++fixture->bulkStops;
}
}

int main() {
    WiringTests(); OptionalFailureTests(); ReloadTests();
    std::puts("Controller bulk stash feedback wiring, optional failure, refresh isolation and lifecycle passed.");
}
