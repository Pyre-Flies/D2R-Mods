#include "bulk_stash.h"
#include "deposit_button.h"
#include "button_state.h"
#include "deposit_native.h"
#include "gold_anchor.h"
#include "stash_feedback.h"
#include <D2RLSettings/service.h>
#define INVENTORY_DEPOSIT_TEST_SETTINGS_LIB 1
#include <D2RLPlugin/api.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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
    QolBulkStash::Observer bulkObserver{};
    D2RL::ConsoleCommandCallback consoleCommand{};
    void* consoleCommandUser{};
    std::string configText{"[inventory_deposit]\nenabled = true\ndeposit_rejuvenation_potions = true\n"};
    std::string lastMessage{};
    D2RL::ConsoleMessageKind lastMessageKind{D2RL::ConsoleMessageKind::Output};
    bool allowConfigWrite{true};
    bool contextOpen{true}, actionReady{true}, eligibleItems{true}, busy{};
    unsigned requests{}, buttonStops{}, nativeStops{}, bulkStops{}, anchorStops{}, feedbackStops{}, eligibilityReads{}, stateStarts{}, stateStops{}, configWrites{};
    std::atomic<bool> allowRefresh{true};
    std::atomic<unsigned> refreshes{};
#if INVENTORY_DEPOSIT_TEST_SETTINGS_LIB
    D2RL::PluginCommunicationService commService{};
    D2RLSettings::ServiceV1 settingsService{};
    D2RLSettings::ToggleSpec registeredToggle{};
    bool toggleRegistered{}, toggleRemoved{};
    bool currentToggleValue{true};
#endif

    Fixture();

    D2RL::ConsoleCommandResult RunCommand(std::string_view args) {
        Check(consoleCommand != nullptr, "autodeposit console command registered");
        const D2RL::ConsoleCommandContext cmd{
            .contextSize = D2RL::ConsoleCommandContextSize,
            .argsLength = static_cast<uint32_t>(args.size()),
            .args = args.data(),
            .plugin = &context,
        };
        return consoleCommand(nullptr, &cmd, consoleCommandUser);
    }
};
Fixture* fixture{};

bool __cdecl MockReadConfig(const D2RL::PluginContext*, char* buffer, uint32_t bufferSize, uint32_t* requiredSize) noexcept {
    if (!fixture) return false;
    const auto needed = static_cast<uint32_t>(fixture->configText.size() + 1);
    if (requiredSize) *requiredSize = needed;
    if (!buffer || bufferSize < needed) return false;
    std::memcpy(buffer, fixture->configText.c_str(), needed);
    return true;
}

bool __cdecl MockWriteConfig(const D2RL::PluginContext*, const char* content) noexcept {
    if (!fixture || !fixture->allowConfigWrite || !content) return false;
    fixture->configText = content;
    ++fixture->configWrites;
    return true;
}

bool __cdecl MockRegisterConsoleCommand(const D2RL::PluginContext*, const D2RL::ConsoleCommandRegistration* reg) noexcept {
    if (!fixture || !reg || !reg->name || std::strcmp(reg->name, "autodeposit") != 0 || !reg->callback) return false;
    fixture->consoleCommand = reg->callback;
    fixture->consoleCommandUser = reg->userData;
    return true;
}

void __cdecl MockWriteConsoleMessage(const D2RL::PluginContext*, const char* message, D2RL::ConsoleMessageKind kind) noexcept {
    if (!fixture || !message) return;
    fixture->lastMessage = message;
    fixture->lastMessageKind = kind;
}

#if INVENTORY_DEPOSIT_TEST_SETTINGS_LIB
D2RLSettings::Result __cdecl MockAddToggle(const D2RLSettings::ToggleSpec* spec, D2RLSettings::RowHandle* handle) noexcept {
    if (!fixture || !spec || !handle || spec->structSize < sizeof(D2RLSettings::ToggleSpec)) {
        return D2RLSettings::Result::InvalidArgument;
    }
    fixture->registeredToggle = *spec;
    fixture->toggleRegistered = true;
    fixture->currentToggleValue = spec->value;
    *handle = 1;
    return D2RLSettings::Result::Success;
}

D2RLSettings::Result __cdecl MockSetToggle(D2RLSettings::RowHandle row, bool value) noexcept {
    if (!fixture || row != 1) return D2RLSettings::Result::InvalidArgument;
    fixture->currentToggleValue = value;
    return D2RLSettings::Result::Success;
}

void __cdecl MockRemoveRow(D2RLSettings::RowHandle row) noexcept {
    if (!fixture || row != 1) return;
    fixture->toggleRemoved = true;
}

D2RL::PluginCommunication::Result __cdecl MockAcquireService(
    const D2RL::PluginContext*, const D2RL::PluginCommunication::AcquireServiceRequest* req,
    D2RL::PluginCommunication::AcquiredService* out) noexcept {
    if (!fixture || !req || !out || !req->providerPluginId || !req->name) {
        return D2RL::PluginCommunication::Result::InvalidArgument;
    }
    if (std::strcmp(req->providerPluginId, D2RLSettings::ProviderPluginId) == 0 &&
        std::strcmp(req->name, D2RLSettings::ServiceName) == 0) {
        out->handle = 1;
        out->table = &fixture->settingsService;
        out->serviceVersion = D2RLSettings::ServiceVersion;
        out->tableSize = D2RLSettings::ServiceV1Size;
        return D2RL::PluginCommunication::Result::Success;
    }
    return D2RL::PluginCommunication::Result::NotFound;
}

D2RL::PluginCommunication::Result __cdecl MockReleaseService(
    const D2RL::PluginContext*, D2RL::PluginCommunication::ServiceHandle) noexcept {
    return D2RL::PluginCommunication::Result::Success;
}

D2RL::PluginCommunication::Result __cdecl MockSubscribeEvent(
    const D2RL::PluginContext*, const D2RL::PluginCommunication::EventSubscription*,
    D2RL::PluginCommunication::SubscriptionHandle* outHandle) noexcept {
    if (outHandle) *outHandle = 1;
    return D2RL::PluginCommunication::Result::Success;
}

D2RL::PluginCommunication::Result __cdecl MockUnsubscribeEvent(
    const D2RL::PluginContext*, D2RL::PluginCommunication::SubscriptionHandle) noexcept {
    return D2RL::PluginCommunication::Result::Success;
}

D2RL::ServiceQueryResult __cdecl MockQueryService(
    const D2RL::PluginContext*, D2RL::ServiceId serviceId, uint32_t, const void** service) noexcept {
    if (!fixture || !service) return D2RL::ServiceQueryResult::InvalidArgument;
    if (serviceId == D2RL::ServiceId::PluginCommunication) {
        *service = &fixture->commService;
        return D2RL::ServiceQueryResult::Success;
    }
    return D2RL::ServiceQueryResult::UnknownService;
}
#endif

Fixture::Fixture() {
    api.apiSize = D2RL::PluginApiSize;
    api.readConfig = MockReadConfig;
    api.writeConfig = MockWriteConfig;
    api.registerConsoleCommand = MockRegisterConsoleCommand;
    api.writeConsoleMessage = MockWriteConsoleMessage;
#if INVENTORY_DEPOSIT_TEST_SETTINGS_LIB
    api.queryService = MockQueryService;
    commService.serviceSize = D2RL::PluginCommunicationServiceSize;
    commService.serviceVersion = D2RL::PluginCommunicationService::AbiVersion;
    commService.acquireService = MockAcquireService;
    commService.releaseService = MockReleaseService;
    commService.subscribeEvent = MockSubscribeEvent;
    commService.unsubscribeEvent = MockUnsubscribeEvent;
    settingsService.serviceSize = D2RLSettings::ServiceV1Size;
    settingsService.serviceVersion = D2RLSettings::ServiceVersion;
    settingsService.addToggle = MockAddToggle;
    settingsService.setToggle = MockSetToggle;
    settingsService.removeRow = MockRemoveRow;
#endif
    context.contextSize = D2RL::PluginContextSize;
    context.abiVersion = D2RL_PLUGIN_ABI_VERSION;
    context.api = &api;
}

void WiringTests() {
    Fixture f;
    fixture = &f;
    Check(D2RLoaderLoadPlugin(&f.context), "real plugin entrypoint loads against adapter fixtures");
    Check(f.visible && f.enabled && f.request && f.bulkAvailable && f.bulkObserver.allow && f.consoleCommand,
        "plugin wires button policies, bulk admission, item filter, and /autodeposit command");
    Check(InventoryDepositNative::RejuvenationDepositEnabled(), "rejuvenation deposit starts enabled by default");
    Check(f.stateStarts == 1, "native visual adapter is initialized before the button");
    Check(f.visible() && f.enabled() && f.bulkAvailable(), "open usable stash exposes an enabled button");

    D2RL::Items::ItemInfo rejuv{.structSize = D2RL::Items::ItemInfoSize, .code = D2RL::Items::MakeItemCode("rvs")};
    D2RL::Items::ItemInfo fullRejuv{.structSize = D2RL::Items::ItemInfoSize, .code = D2RL::Items::MakeItemCode("rvl")};
    D2RL::Items::ItemInfo gem{.structSize = D2RL::Items::ItemInfoSize, .code = D2RL::Items::MakeItemCode("gcv")};
    Check(f.bulkObserver.allow(f.bulkObserver.user, rejuv) &&
        f.bulkObserver.allow(f.bulkObserver.user, fullRejuv) &&
        f.bulkObserver.allow(f.bulkObserver.user, gem),
        "rejuvenation potions and gems are allowed when rejuvenation deposit is enabled");

    Check(f.RunCommand("rejuv off") == D2RL::ConsoleCommandResult::Handled, "/autodeposit rejuv off succeeds");
    Check(!InventoryDepositNative::RejuvenationDepositEnabled() && !f.currentToggleValue && f.configWrites == 1 &&
        f.configText.find("deposit_rejuvenation_potions = false") != std::string::npos,
        "/autodeposit rejuv off disables rejuvenation deposit, syncs Options toggle, and persists to config");
    Check(!f.bulkObserver.allow(f.bulkObserver.user, rejuv) &&
        !f.bulkObserver.allow(f.bulkObserver.user, fullRejuv) &&
        f.bulkObserver.allow(f.bulkObserver.user, gem),
        "bulk stash observer skips rejuvenation potions while still allowing gems");

    Check(f.RunCommand("rejuv") == D2RL::ConsoleCommandResult::Handled, "bare /autodeposit rejuv toggles back on");
    Check(InventoryDepositNative::RejuvenationDepositEnabled() && f.configWrites == 2 &&
        f.configText.find("deposit_rejuvenation_potions = true") != std::string::npos,
        "bare /autodeposit rejuv re-enables rejuvenation deposit and persists to config");
#if INVENTORY_DEPOSIT_TEST_SETTINGS_LIB
    Check(f.toggleRegistered &&
        std::strcmp(f.registeredToggle.key, "inventory_deposit_deposit_rejuvenation_potions") == 0 &&
        std::strcmp(f.registeredToggle.label, "Deposit Rejuvenation Potions") == 0 &&
        f.registeredToggle.section == D2RLSettings::Section::Ui &&
        f.currentToggleValue,
        "Options > Gameplay > UI registers Deposit Rejuvenation Potions toggle and syncs with /autodeposit");
    Check(f.registeredToggle.onApplied != nullptr, "Options toggle wires an Apply callback");
    f.registeredToggle.onApplied(false, f.registeredToggle.userData);
    Check(!InventoryDepositNative::RejuvenationDepositEnabled() && f.configWrites == 3 &&
        f.configText.find("deposit_rejuvenation_potions = false") != std::string::npos,
        "Options > Gameplay Apply callback disables rejuvenation deposit and saves to config");
    f.registeredToggle.onApplied(true, f.registeredToggle.userData);
    Check(InventoryDepositNative::RejuvenationDepositEnabled() && f.configWrites == 4 &&
        f.configText.find("deposit_rejuvenation_potions = true") != std::string::npos,
        "Options > Gameplay Apply callback re-enables rejuvenation deposit and saves to config");
#endif

    const auto baselineWrites = f.configWrites;
    Check(f.RunCommand("status") == D2RL::ConsoleCommandResult::Handled && f.configWrites == baselineWrites,
        "/autodeposit status reports state without writing config");
    Check(f.RunCommand("help") == D2RL::ConsoleCommandResult::Handled && f.configWrites == baselineWrites,
        "/autodeposit help reports usage without writing config");
    Check(f.RunCommand("invalid") == D2RL::ConsoleCommandResult::InvalidArguments &&
        f.configWrites == baselineWrites && InventoryDepositNative::RejuvenationDepositEnabled(),
        "invalid /autodeposit syntax fails without writing config or mutating state");

    f.allowConfigWrite = false;
    Check(f.RunCommand("rejuv off") == D2RL::ConsoleCommandResult::Failed &&
        InventoryDepositNative::RejuvenationDepositEnabled() &&
        f.lastMessageKind == D2RL::ConsoleMessageKind::Warning,
        "failed config save rolls back and retains previous rejuvenation deposit setting");
    f.allowConfigWrite = true;

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
#if INVENTORY_DEPOSIT_TEST_SETTINGS_LIB
    Check(f.toggleRemoved, "unload removes the Options > Gameplay toggle row");
#endif
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
void Initialize(const D2RL::PluginContext*, Available available, const Observer* observer) noexcept {
    fixture->bulkAvailable = available;
    fixture->bulkObserver = observer ? *observer : Observer{};
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
    std::puts("plugin entrypoint visibility, action admission, bulk admission, /autodeposit command, and reload wiring regressions passed");
}
