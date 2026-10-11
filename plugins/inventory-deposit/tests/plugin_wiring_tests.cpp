#include "bulk_stash.h"
#include "deposit_button.h"
#include "button_state.h"
#include "deposit_native.h"
#include "gold_anchor.h"
#include "stash_feedback.h"
#include <D2RLPlugin/api.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>

extern "C" bool D2RLoaderLoadPlugin(const D2RL::PluginContext*) noexcept;
extern "C" void D2RLoaderUnloadPlugin() noexcept;

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// Exercise the actual callbacks selected by plugin_main.cpp. Native state is
// supplied at the adapter boundary; no game addresses or item movement run.
struct Fixture {
    D2RL::PluginApi api{};
    D2RL::PluginContext context{};
    InventoryDepositButton::Predicate visible{}, enabled{}, request{};
    QolBulkStash::Available bulkAvailable{};
    bool contextOpen{true}, actionReady{true}, eligibleItems{true}, busy{};
    unsigned requests{}, buttonStops{}, nativeStops{}, bulkStops{}, anchorStops{}, feedbackStops{}, eligibilityReads{}, stateStarts{}, stateStops{};
    std::atomic<bool> allowRefresh{true};
    std::atomic<unsigned> refreshes{};

    Fixture() {
        api.apiSize = D2RL::PluginApiSize;
        context.contextSize = D2RL::PluginContextSize;
        context.abiVersion = D2RL_PLUGIN_ABI_VERSION;
        context.api = &api;
    }
};
Fixture* fixture{};

void WiringTests() {
    Fixture f;
    fixture = &f;
    Check(D2RLoaderLoadPlugin(&f.context), "real plugin entrypoint loads against adapter fixtures");
    Check(f.visible && f.enabled && f.request && f.bulkAvailable,
        "plugin wires both button policies and bulk admission");
    Check(f.stateStarts == 1, "native visual adapter is initialized before the button");
    Check(f.visible() && f.enabled() && f.bulkAvailable(), "open usable stash exposes an enabled button");

    f.eligibleItems = false;
    Check(f.visible() && !f.enabled() && f.bulkAvailable(),
        "no eligible inventory items leaves a visible disabled button and valid bulk context");
    Check(!f.request() && f.requests == 0, "an empty or wholly ineligible inventory cannot submit");
    f.eligibleItems = true;
    Check(f.visible() && f.enabled() && f.requests == 0,
        "adding an eligible item reenables without automatically requesting a deposit");

    f.actionReady = false;
    Check(f.visible(), "holding an item preserves button visibility");
    Check(!f.enabled(), "holding an item selects the disabled button state");
    Check(!f.bulkAvailable(), "holding an item still blocks the actual bulk admission callback");
    Check(!f.request() && f.requests == 0, "held-item action never reaches the bulk request");

    f.actionReady = true;
    Check(f.visible() && f.enabled() && f.bulkAvailable() && f.requests == 0,
        "releasing the item restores enablement without requesting a deposit");
    Check(f.request() && f.requests == 1, "a new explicit click can deposit after cursor release");

    f.busy = true;
    f.eligibleItems = false;
    const auto readsBeforeBusy = f.eligibilityReads;
    Check(f.visible() && !f.enabled(), "an active batch keeps the button visible and disabled");
    Check(f.bulkAvailable(), "a busy batch with its last eligible item removed can finish verification");
    Check(!f.request() && f.requests == 1, "an active batch blocks additional button requests");
    Check(f.eligibilityReads == readsBeforeBusy, "busy batches skip unnecessary eligibility scans");
    f.busy = false;
    Check(f.visible() && !f.enabled() && f.bulkAvailable(),
        "completed batch with no eligible items leaves the button grey rather than hiding it");
    f.eligibleItems = true;

    f.contextOpen = false;
    Check(!f.visible() && !f.enabled() && !f.bulkAvailable(),
        "a closed or unsupported stash hides the button and blocks the queue");
    Check(!f.request() && f.requests == 1, "an unavailable context cannot reach bulk request");
    f.contextOpen = true;
    Check(f.visible() && f.enabled() && f.requests == 1,
        "restored context does not replay an earlier blocked request");

    D2RLoaderUnloadPlugin();
    Check(f.buttonStops == 1 && f.nativeStops == 1 && f.bulkStops == 1 &&
        f.anchorStops == 1 && f.feedbackStops == 1 && f.stateStops == 1,
        "unload stops the worker and shuts down each initialized adapter once");
    fixture = nullptr;
}
}

namespace InventoryDepositNative {
bool Initialize(const D2RL::PluginContext*) noexcept { return true; }
void Shutdown() noexcept { ++fixture->nativeStops; }
bool Supported() noexcept { return true; }
bool Visible() noexcept { return fixture->contextOpen; }
bool Available() noexcept { return fixture->contextOpen && fixture->actionReady; }
bool HasEligibleItems() noexcept {
    ++fixture->eligibilityReads;
    return Available() && fixture->eligibleItems;
}
}

namespace InventoryDepositButton {
bool Initialize(const D2RL::PluginContext*, Predicate visible, Predicate enabled,
    Predicate request, int, int, Positioner, StateApplier applyState) noexcept {
    Check(applyState == InventoryDepositButtonState::Apply && fixture->stateStarts == 1,
        "button receives the admitted native visual callback");
    fixture->visible = visible;
    fixture->enabled = enabled;
    fixture->request = request;
    return true;
}
void Refresh() noexcept {
    Check(fixture && fixture->allowRefresh.load(), "polling worker cannot outlive adapter shutdown");
    ++fixture->refreshes;
}
void Shutdown() noexcept {
    fixture->allowRefresh.store(false);
    ++fixture->buttonStops;
}
}

namespace QolBulkStash {
void Initialize(const D2RL::PluginContext*, Available available, const Observer*) noexcept {
    fixture->bulkAvailable = available;
}
bool Busy() noexcept { return fixture->busy; }
// Deliberately does not repeat admission: the regression must catch a wrongly
// wired plugin request, rather than have the stub conceal it with another gate.
bool Request() noexcept { ++fixture->requests; return true; }
void Shutdown() noexcept { ++fixture->bulkStops; }
}

namespace InventoryDepositGoldAnchor {
bool Initialize(const D2RL::PluginContext*, int, int) noexcept { return true; }
bool Position(const char*, D2RL::Widgets::WidgetHandle, D2RL::Widgets::WidgetHandle) noexcept { return true; }
void Shutdown() noexcept { ++fixture->anchorStops; }
}

namespace InventoryDepositButtonState {
bool Initialize(const D2RL::PluginContext*) noexcept { ++fixture->stateStarts; return true; }
bool Apply(const char*, D2RL::Widgets::WidgetHandle, D2RL::Widgets::WidgetHandle, bool) noexcept { return true; }
void Shutdown() noexcept { ++fixture->stateStops; }
}

namespace InventoryDepositStashFeedback {
bool Initialize(const D2RL::PluginContext*) noexcept { return true; }
void BeginBatch() noexcept {}
void ItemRemoved(const D2RL::Items::ItemInfo&) noexcept {}
void EndBatch() noexcept {}
void Refresh() noexcept {
    Check(fixture && fixture->allowRefresh.load(), "feedback polling cannot outlive button shutdown");
}
void Shutdown() noexcept { ++fixture->feedbackStops; }
}

int main() {
    WiringTests();
    WiringTests();
    std::puts("plugin entrypoint visibility, action admission, bulk admission, and reload wiring regressions passed");
}
