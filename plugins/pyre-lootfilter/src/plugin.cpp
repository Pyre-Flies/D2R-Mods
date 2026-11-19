// pyre-lootfilter: Clean-room native loot filter plugin for D2RLoader.
// Supports filter.json v1/v2/v3, custom names, stack quantities, background/text
// colors, custom drop audio, OverlayService minimap icons, keyboard Alt pickup
// override, live auto-reload, and Controller QOL visibility integration.

#include "audio_and_pickup.h"
#include "filter_schema.h"
#include "filter_tables.h"
#include "json_value.h"
#include "label_pipeline.h"
#include "minimap_pipeline.h"
#include "profile_dropdown.h"
#include "qol_interop.h"
#include "build_admission.h"
#include "unit_cache.h"
#include "unit_reader.h"
#include "../../shared/chat_config.h"
#include <D2RLChat/native_commands.h>
#include <D2RLPlugin/api.h>
#include <D2RLSettings/settings.h>
#include <windows.h>
#include <intrin.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace PyreLootFilter {
namespace {

inline constexpr char PluginVersion[] = "1.3.1+rev.7";
inline constexpr char CommandHelp[] = "status/reload/perf/on/off/toggle/minimap/sound/alt";
static_assert(sizeof(CommandHelp) <= 64);
inline constexpr unsigned ConfigCapacity = 4096U;

struct Settings {
    bool enabled{true};
    bool autoReload{true};
    bool autoShowItemsOnJoin{true};
    bool altPickupOverride{true};
    bool minimapIcons{true};
    bool dropSounds{true};
    bool dropSound3dAttenuation{false};
    bool xrayMode{false};
    std::string selectedFilter{};
};

struct PerfCounter {
    std::atomic<std::uint64_t> calls{0};
    std::atomic<std::uint64_t> totalTicks{0};

    void Record(std::uint64_t ticks) noexcept {
        calls.fetch_add(1, std::memory_order_relaxed);
        totalTicks.fetch_add(ticks, std::memory_order_relaxed);
    }

    void Reset() noexcept {
        calls.store(0, std::memory_order_relaxed);
        totalTicks.store(0, std::memory_order_relaxed);
    }
};

std::atomic<bool> g_perfTimingEnabled{false};

struct PerfScope {
    PerfCounter& counter;
    LARGE_INTEGER start{};
    bool timing{false};

    explicit PerfScope(PerfCounter& c) noexcept
        : counter(c), timing(g_perfTimingEnabled.load(std::memory_order_relaxed)) {
        if (timing) {
            QueryPerformanceCounter(&start);
        } else {
            counter.calls.fetch_add(1, std::memory_order_relaxed);
        }
    }
    ~PerfScope() noexcept {
        if (!timing) return;
        LARGE_INTEGER end{};
        QueryPerformanceCounter(&end);
        if (end.QuadPart >= start.QuadPart) {
            counter.Record(static_cast<std::uint64_t>(end.QuadPart - start.QuadPart));
        }
    }
};

const D2RL::PluginContext* g_ctx = nullptr;
const D2RL::OverlayService* g_overlay = nullptr;
D2RL::Overlay::CallbackHandle g_overlayCallbackHandle = D2RL::Overlay::InvalidCallbackHandle;
const D2RL::SharedEventService* g_sharedEvents = nullptr;
D2RL::SharedEvents::ListenerHandle g_uiMessageHandle = 0;
const D2RL::ThreadService* g_uiThreads = nullptr;
using IsInGameFn = bool(__cdecl*)() noexcept;
IsInGameFn g_isInGame = nullptr;
D2RLChat::NativeCommands g_chatCommands;

std::atomic<bool> g_ready{false};
std::atomic<bool> g_clipCorrectionAdmitted{false};
std::atomic<bool> g_showItemsAdmitted{false};
std::atomic<bool> g_inGameFilterCoexistenceAdmitted{false};
Labels::ShowItemsJoinSwapTracker g_showItemsTracker{};
std::atomic<bool> g_showItemsUiTaskQueued{false};
std::atomic<bool> g_warnedMissingSoundRows{false};
std::atomic<bool> g_enabled{true};
std::atomic<bool> g_autoReload{true};
std::atomic<bool> g_autoShowItemsOnJoin{true};
std::atomic<bool> g_altPickupOverride{true};
std::atomic<bool> g_minimapIcons{true};
std::atomic<bool> g_dropSounds{true};
std::atomic<bool> g_dropSound3dAttenuation{false};
std::atomic<bool> g_xrayMode{false};

std::atomic<std::uint64_t> g_nextGeneration{1};
std::mutex g_rulesMutex;
std::shared_ptr<const Schema::FilterRuleTable> g_publishedRules;
std::array<std::shared_ptr<const Schema::FilterRuleTable>, 32> g_retiredRulesRing{};
std::size_t g_retiredRulesIndex{0};
std::atomic<const Schema::FilterRuleTable*> g_activeRulesPtr{nullptr};
struct CandidateReloadWatcher {
    std::filesystem::path path;
    Schema::LiveReloadWatcher watcher{};
};

std::filesystem::path g_filterJsonPath;
std::filesystem::path g_activeModRoot;
Schema::FilterPathInputs g_pathInputs{};
std::vector<CandidateReloadWatcher> g_candidateWatchers;
std::uint64_t g_lastWatcherPollMs{0};
bool g_lastReloadHotkeyHeld{false};

std::mutex g_profileMutex;
std::vector<Schema::FilterProfileDescriptor> g_discoveredProfiles;
std::string g_selectedFilter;
std::string g_activeShortPath;
std::string g_lastLoadError;
std::string g_lastFailedProfileLabel;
std::filesystem::path g_lastSelectedProfilePath;
std::atomic<bool> g_pendingInGameChatNotice{false};
std::string g_startupChatMessage;
bool g_startupChatIsError{false};

Reader::NativeReaders g_readers{};
Cache::UnitEvaluationCache g_cache{};
Minimap::MinimapState g_minimap{};
std::uintptr_t g_soundLoaderBase{0};

PerfCounter g_perfFormatter{};
PerfCounter g_perfInnerName{};
PerfCounter g_perfPaint{};
PerfCounter g_perfGlyph{};
PerfCounter g_perfAutomap{};
PerfCounter g_perfPickup{};
PerfCounter g_perfQolVis{};

// Original trampolines stored in non-executable writable .data so structural
// validators (including Controller QOL's QolUnhoarderDispatch::ValidateStructural)
// can verify the RIP-relative load (48 8B 05 disp32) in the detour prologue.
Labels::LabelFormatterFn g_originalLabelFormatter = nullptr;
Labels::InnerNameWriterFn g_originalInnerNameWriter = nullptr;
Labels::SharedLabelPaintFn g_originalSharedLabelPaint = nullptr;
Labels::CorrectedGlyphBFn g_originalCorrectedGlyphB = nullptr;
Labels::IsHdModeFn g_isHdMode = nullptr;
Labels::LegacyCoordToScreenFn g_legacyCoordToScreen = nullptr;
Labels::DrawUiFilledRectFn g_drawUiFilledRect = nullptr;
Minimap::AutomapRenderUnitFn g_originalAutomapRenderUnit = nullptr;
Minimap::ProjectClientToAutomapFn g_projectClientToAutomap = nullptr;
AudioAndPickup::NativeDropPlayerFn g_originalDropPlayer = nullptr;
AudioAndPickup::NativeActionDispatchFn g_originalActionDispatch = nullptr;

D2RLSettings::Dropdown g_profileDropdown;
D2RLSettings::Status   g_statusSetting;
D2RLSettings::Toggle   g_minimapSetting;
D2RLSettings::Toggle   g_dropSoundsSetting;
D2RLSettings::Toggle   g_dropSound3dSetting;
D2RLSettings::Toggle   g_autoShowItemsSetting;
D2RLSettings::Toggle   g_altOverrideSetting;
D2RLSettings::Toggle   g_autoReloadSetting;

void SyncSettingsStatusAndDropdown() noexcept;
void SyncSettingsTogglesFromMemory() noexcept;

[[nodiscard]] const Schema::FilterRuleTable* CurrentRules() noexcept {
    return g_activeRulesPtr.load(std::memory_order_acquire);
}

[[nodiscard]] bool IsInGameLootFilterActive() noexcept {
    return g_inGameFilterCoexistenceAdmitted.load(std::memory_order_relaxed) &&
           ProfileDropdown::IsInGameFilterActive(g_readers.exeBase);
}

[[nodiscard]] bool IsPyreFilterActive() noexcept {
    return g_ready.load(std::memory_order_relaxed) &&
           g_enabled.load(std::memory_order_relaxed) &&
           !IsInGameLootFilterActive();
}

void PublishRules(std::shared_ptr<const Schema::FilterRuleTable> next) noexcept {
    std::lock_guard lock(g_rulesMutex);
    const Schema::FilterRuleTable* const raw = next.get();
    g_activeRulesPtr.store(raw, std::memory_order_release);
    if (g_publishedRules) {
        g_retiredRulesRing[g_retiredRulesIndex] = std::move(g_publishedRules);
        g_retiredRulesIndex = (g_retiredRulesIndex + 1U) % g_retiredRulesRing.size();
    }
    g_publishedRules = std::move(next);
    if (!raw) {
        g_retiredRulesRing.fill({});
        g_retiredRulesIndex = 0;
    }
}

[[nodiscard]] Settings ParseTomlConfig(std::string_view text) noexcept {
    Settings s{};
    bool inSection = false;
    for (std::size_t cursor = 0; cursor < text.size();) {
        const auto nl = text.find('\n', cursor);
        const auto end = (nl == text.npos) ? text.size() : nl;
        auto line = text.substr(cursor, end - cursor);
        if (cursor == 0 && line.starts_with("\xEF\xBB\xBF")) line.remove_prefix(3);
        const auto comment = line.find('#');
        const auto logical = ChatConfig::Trim(line.substr(0, comment));
        if (!logical.empty() && logical.front() == '[') {
            inSection = (logical == "[pyre-lootfilter]");
        } else if (inSection) {
            const auto eq = logical.find('=');
            if (eq != logical.npos) {
                const auto k = ChatConfig::Trim(logical.substr(0, eq));
                const auto v = ChatConfig::Trim(logical.substr(eq + 1));
                if (v == "true" || v == "false") {
                    const bool b = (v == "true");
                    if (k == "enabled") s.enabled = b;
                    else if (k == "auto_reload") s.autoReload = b;
                    else if (k == "auto_show_items_on_join") s.autoShowItemsOnJoin = b;
                    else if (k == "alt_pickup_override" || k == "ctrl_pickup_override") s.altPickupOverride = b;
                    else if (k == "minimap_icons") s.minimapIcons = b;
                    else if (k == "drop_sounds") s.dropSounds = b;
                    else if (k == "drop_sound_3d_attenuation" || k == "drop_sound_attenuation" ||
                             k == "positional_drop_sounds" || k == "sound_3d_attenuation") {
                        s.dropSound3dAttenuation = b;
                    }
                    else if (k == "xray_mode") s.xrayMode = b;
                } else if (k == "selected_filter" && v.size() >= 2 &&
                           ((v.front() == '"' && v.back() == '"') ||
                            (v.front() == '\'' && v.back() == '\''))) {
                    s.selectedFilter = std::string(v.substr(1, v.size() - 2));
                }
            }
        }
        cursor = (nl == text.npos) ? text.size() : nl + 1;
    }
    return s;
}

[[nodiscard]] bool IsNoneSelection(std::string_view value) noexcept {
    const auto trimmed = ChatConfig::Trim(value);
    const auto eqAscii = [](std::string_view a, std::string_view b) noexcept {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? static_cast<char>(a[i] - 'A' + 'a') : a[i];
            const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? static_cast<char>(b[i] - 'A' + 'a') : b[i];
            if (ca != cb) return false;
        }
        return true;
    };
    return eqAscii(trimmed, "none") || eqAscii(trimmed, "off") || eqAscii(trimmed, "none (off)");
}

void SyncSettingsTogglesFromMemory() noexcept;

void LoadTomlSettings() noexcept {
    char buf[ConfigCapacity]{};
    unsigned required = 0;
    Settings s{};
    if (g_ctx && g_ctx->ReadConfig(buf, sizeof(buf), &required) && required <= sizeof(buf)) {
        const auto len = strnlen_s(buf, sizeof(buf));
        if (len < sizeof(buf)) {
            s = ParseTomlConfig(std::string_view(buf, len));
        }
    }
    if (!s.selectedFilter.empty() && IsNoneSelection(s.selectedFilter)) {
        s.enabled = false;
        s.selectedFilter = "none";
    }
    g_enabled.store(s.enabled, std::memory_order_release);
    g_autoReload.store(s.autoReload, std::memory_order_release);
    g_autoShowItemsOnJoin.store(s.autoShowItemsOnJoin, std::memory_order_release);
    g_altPickupOverride.store(s.altPickupOverride, std::memory_order_release);
    g_minimapIcons.store(s.minimapIcons, std::memory_order_release);
    g_dropSounds.store(s.dropSounds, std::memory_order_release);
    g_dropSound3dAttenuation.store(s.dropSound3dAttenuation, std::memory_order_release);
    g_xrayMode.store(s.xrayMode, std::memory_order_release);
    {
        std::lock_guard lock(g_profileMutex);
        g_selectedFilter = s.selectedFilter;
    }
    SyncSettingsTogglesFromMemory();
}

void SavePluginConfig(bool persistSelectedFilter = false) noexcept {
    if (!g_ctx) return;
    (void)ChatConfig::Save(
        g_ctx, "pyre-lootfilter",
        {
            {"enabled", g_enabled.load()},
            {"auto_reload", g_autoReload.load()},
            {"auto_show_items_on_join", g_autoShowItemsOnJoin.load()},
            {"alt_pickup_override", g_altPickupOverride.load()},
            {"minimap_icons", g_minimapIcons.load()},
            {"drop_sounds", g_dropSounds.load()},
            {"drop_sound_3d_attenuation", g_dropSound3dAttenuation.load()},
        },
        ConfigCapacity);

    if (persistSelectedFilter) {
        std::string sel;
        {
            std::lock_guard lock(g_profileMutex);
            sel = g_selectedFilter;
        }
        char buf[ConfigCapacity]{};
        unsigned required = 0;
        if (g_ctx->ReadConfig(buf, sizeof(buf), &required) && required <= sizeof(buf)) {
            const auto len = strnlen_s(buf, sizeof(buf));
            if (len < sizeof(buf)) {
                const std::string updated = ProfileDropdown::UpdateTomlString(
                    std::string_view(buf, len), "pyre-lootfilter", "selected_filter", sel);
                if (updated.size() < ConfigCapacity) {
                    (void)g_ctx->WriteConfig(updated.c_str());
                }
            }
        }
    }
}

[[nodiscard]] Schema::FilterPathInputs BuildFilterPathInputs(const D2RL::PluginContext* ctx) {
    Schema::FilterPathInputs in{};
    if (!ctx) return in;
    if (ctx->modDirectory && ctx->modDirectory[0]) {
        in.modDirectory = ctx->modDirectory;
    }
    if (ctx->modSupportDirectory && ctx->modSupportDirectory[0]) {
        in.modSupportDirectory = ctx->modSupportDirectory;
    }
    if (ctx->pluginConfigPath && ctx->pluginConfigPath[0]) {
        in.pluginConfigPath = ctx->pluginConfigPath;
    }
    if (ctx->scopeRootDirectory && ctx->scopeRootDirectory[0]) {
        in.scopeRootDirectory = ctx->scopeRootDirectory;
    }
    if (ctx->activeMod && ctx->activeMod[0]) {
        in.activeMod = ctx->activeMod;
    }
    if (ctx->exeBase) {
        wchar_t exeBuf[4096]{};
        const DWORD len = GetModuleFileNameW(reinterpret_cast<HMODULE>(ctx->exeBase), exeBuf, 4096);
        if (len > 0 && len < 4096) {
            in.gameDirectory = std::filesystem::path(exeBuf, exeBuf + len).parent_path();
        }
    }
    return in;
}

void RefreshDiscoveredProfiles() noexcept {
    try {
        auto discovered = Schema::DiscoverFilterProfiles(g_pathInputs);
        const auto candidates = Schema::BuildFilterJsonCandidates(g_pathInputs, &discovered);

        std::lock_guard lock(g_profileMutex);
        std::vector<CandidateReloadWatcher> nextWatchers;
        nextWatchers.reserve(candidates.size());
        for (const auto& c : candidates) {
            CandidateReloadWatcher cw{.path = c};
            bool matchedExisting = false;
            for (const auto& oldCw : g_candidateWatchers) {
                if (Schema::PathEquivalent(oldCw.path, c)) {
                    cw.watcher = oldCw.watcher;
                    matchedExisting = true;
                    break;
                }
            }
            if (!matchedExisting) {
                cw.watcher.Resync(Schema::ReadFilterFileStamp(c));
            }
            nextWatchers.push_back(std::move(cw));
        }
        g_candidateWatchers = std::move(nextWatchers);
        g_discoveredProfiles = std::move(discovered);
    } catch (...) {
    }
}

[[nodiscard]] std::string FindProfileShortPath(const std::filesystem::path& p) noexcept {
    std::lock_guard lock(g_profileMutex);
    for (const auto& prof : g_discoveredProfiles) {
        if (Schema::PathEquivalent(prof.path, p)) {
            return prof.shortPath;
        }
    }
    return p.filename().generic_string();
}

[[nodiscard]] std::string FindProfileDisplayLabel(const std::filesystem::path& p) noexcept {
    std::lock_guard lock(g_profileMutex);
    for (const auto& prof : g_discoveredProfiles) {
        if (Schema::PathEquivalent(prof.path, p)) {
            return prof.displayName;
        }
    }
    return p.filename().generic_string();
}

void ResyncCandidateWatchers() noexcept {
    std::lock_guard lock(g_profileMutex);
    for (auto& cw : g_candidateWatchers) {
        cw.watcher.Resync(Schema::ReadFilterFileStamp(cw.path));
    }
}

bool ReloadFilterJson(const char* trigger,
                      const std::filesystem::path& preferredPath = {},
                      bool strictTarget = false,
                      bool reloadToml = true) noexcept {
    try {
        const std::filesystem::path requestedPath = preferredPath;
        RefreshDiscoveredProfiles();
        if (reloadToml) {
            LoadTomlSettings();
        }

        bool noneSelected = false;
        {
            std::lock_guard lock(g_profileMutex);
            noneSelected = requestedPath.empty() && IsNoneSelection(g_selectedFilter);
            if (noneSelected) {
                g_filterJsonPath.clear();
                g_activeShortPath.clear();
                g_lastLoadError.clear();
                g_lastFailedProfileLabel.clear();
                g_lastSelectedProfilePath.clear();
                g_pendingInGameChatNotice.store(false, std::memory_order_release);
            }
        }
        if (noneSelected) {
            PublishRules(nullptr);
            g_cache.Clear();
            g_minimap.Clear();
            ResyncCandidateWatchers();
            SyncSettingsStatusAndDropdown();
            return true;
        }

        std::vector<std::filesystem::path> ordered;
        {
            std::lock_guard lock(g_profileMutex);
            if (strictTarget && !requestedPath.empty()) {
                ordered.push_back(requestedPath.lexically_normal());
            } else {
                ordered = Schema::SelectReloadTargetCandidates(
                    g_pathInputs, g_discoveredProfiles, g_selectedFilter,
                    g_lastSelectedProfilePath, g_filterJsonPath, requestedPath);
            }
        }
        if (ordered.empty()) return false;

        bool anyExists = false;
        for (const auto& c : ordered) {
            if (Schema::ReadFilterFileStamp(c).exists) {
                anyExists = true;
                break;
            }
        }

        const auto nextGen = g_nextGeneration.fetch_add(1, std::memory_order_acq_rel);
        std::shared_ptr<Schema::FilterRuleTable> table;
        std::filesystem::path loadedPath;
        std::string firstHumanError;
        std::filesystem::path firstFailedPath;

        for (std::size_t i = 0; i < ordered.size(); ++i) {
            const auto& candidate = ordered[i];
            if (anyExists && !Schema::ReadFilterFileStamp(candidate).exists) {
                continue;
            }
            std::string humanErr;
            table = Schema::LoadFilterRuleTable(
                candidate, nextGen,
                [](const char* warnMsg) {
                    if (g_ctx && warnMsg) g_ctx->LogWarn(warnMsg);
                },
                g_activeModRoot, &humanErr);
            if (table) {
                loadedPath = candidate;
                break;
            }
            if (firstHumanError.empty() && !humanErr.empty()) {
                firstHumanError = std::move(humanErr);
                firstFailedPath = candidate;
            }
            if (!anyExists || strictTarget || ordered.size() == 1U) break;
        }

        if (!table) {
            ResyncCandidateWatchers();
            if (anyExists || strictTarget || ordered.size() == 1U) {
                const auto failedPath =
                    !firstFailedPath.empty() ? firstFailedPath : ordered.front();
                const std::string failedLabel = FindProfileDisplayLabel(failedPath);
                const std::string errReason =
                    !firstHumanError.empty()
                        ? firstHumanError
                        : "Filter file could not be read or parsed.";
                const bool preservedPrevious = (CurrentRules() != nullptr);
                char consoleErr[512]{};
                std::snprintf(consoleErr, sizeof(consoleErr),
                              "PyreLootFilter ERROR (%s): %s%s",
                              failedPath.filename().generic_string().c_str(),
                              errReason.c_str(),
                              preservedPrevious ? " [last-valid-rules-preserved]" : "");
                const bool inGameNow = (g_isInGame != nullptr && g_isInGame());
                {
                    std::lock_guard lock(g_profileMutex);
                    g_lastLoadError = errReason;
                    g_lastFailedProfileLabel = failedLabel;
                    if (!failedPath.empty()) {
                        g_lastSelectedProfilePath = failedPath;
                    }
                    if ((!inGameNow || (trigger && std::strcmp(trigger, "startup") == 0)) &&
                        g_enabled.load(std::memory_order_relaxed)) {
                        g_startupChatMessage = consoleErr;
                        g_startupChatIsError = true;
                        g_pendingInGameChatNotice.store(true, std::memory_order_release);
                    } else {
                        g_pendingInGameChatNotice.store(false, std::memory_order_release);
                    }
                }
                if (g_ctx) {
                    if (inGameNow) {
                        g_ctx->WriteConsoleMessage(consoleErr, D2RL::ConsoleMessageKind::Warning);
                    }
                    g_ctx->LogWarn(consoleErr);
                }
            }
            SyncSettingsStatusAndDropdown();
            return false;
        }

        const std::string shortPath = FindProfileShortPath(loadedPath);
        const std::string displayName =
            !table->filterName.empty() ? table->filterName
                                       : loadedPath.filename().generic_string();
        char chatMsg[320]{};
        std::snprintf(chatMsg, sizeof(chatMsg),
                      "PyreLootFilter: loaded '%s' (%zu rules, schema v%u) [%s].",
                      displayName.c_str(), table->rules.size(), table->schema,
                      shortPath.c_str());
        const bool inGameNow = (g_isInGame != nullptr && g_isInGame());
        {
            std::lock_guard lock(g_profileMutex);
            g_filterJsonPath = loadedPath;
            g_activeShortPath = shortPath;
            g_lastLoadError.clear();
            g_lastFailedProfileLabel.clear();
            g_lastSelectedProfilePath = loadedPath;
            if ((!inGameNow || (trigger && std::strcmp(trigger, "startup") == 0)) &&
                g_enabled.load(std::memory_order_relaxed)) {
                g_startupChatMessage = chatMsg;
                g_startupChatIsError = false;
                g_pendingInGameChatNotice.store(true, std::memory_order_release);
            } else {
                g_pendingInGameChatNotice.store(false, std::memory_order_release);
            }
        }
        table->xrayMode = g_xrayMode.load(std::memory_order_acquire);
        PublishRules(table);
        g_cache.Clear();
        g_minimap.Clear();
        ResyncCandidateWatchers();
        SyncSettingsStatusAndDropdown();

        if (g_ctx) {
            char msg[384]{};
            std::snprintf(msg, sizeof(msg),
                          "[PyreLootFilter] Loaded %s (%s): schema v%u, %zu rules "
                          "(%zu hide, %zu bg, %zu text, %zu border, %zu sound, %zu minimap, %zu stat) [gen=%llu].",
                          shortPath.c_str(), trigger ? trigger : "init", table->schema,
                          table->rules.size(), table->hiddenRules, table->backgroundRules,
                          table->textColorRules, table->borderColorRules, table->soundRules,
                          table->minimapIconRules, table->statRules,
                          static_cast<unsigned long long>(table->generation));
            g_ctx->LogInfo(msg);
            if (inGameNow && (!trigger || std::strcmp(trigger, "startup") != 0)) {
                g_ctx->WriteConsoleMessage(chatMsg);
            }
        }
        return true;
    } catch (...) {
        if (g_ctx) g_ctx->LogWarn("[PyreLootFilter] Exception while loading filter.json; previous rules preserved.");
        return false;
    }
}

void PollLiveReloadAndHotkeys() noexcept {
    if (!g_enabled.load(std::memory_order_relaxed)) return;
    const auto nowMs = GetTickCount64();

    // Check Ctrl+Shift+F9 manual reload chord only when F9 is physically down
    if ((GetAsyncKeyState(VK_F9) & 0x8000) != 0) {
        DWORD fgPid = 0;
        (void)GetWindowThreadProcessId(GetForegroundWindow(), &fgPid);
        if (fgPid == GetCurrentProcessId()) {
            const bool chord = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 &&
                               (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            if (chord && !g_lastReloadHotkeyHeld) {
                std::filesystem::path targetPath;
                {
                    std::lock_guard lock(g_profileMutex);
                    targetPath = !g_lastSelectedProfilePath.empty() ? g_lastSelectedProfilePath
                                                                    : g_filterJsonPath;
                }
                (void)ReloadFilterJson("Ctrl+Shift+F9", targetPath);
            }
            g_lastReloadHotkeyHeld = chord;
        } else {
            g_lastReloadHotkeyHeld = false;
        }
    } else {
        g_lastReloadHotkeyHeld = false;
    }

    if (!g_autoReload.load(std::memory_order_relaxed)) return;
    if (nowMs < g_lastWatcherPollMs || (nowMs - g_lastWatcherPollMs) < 350U) return;
    g_lastWatcherPollMs = nowMs;

    std::filesystem::path changedPath;
    {
        std::lock_guard lock(g_profileMutex);
        if (g_candidateWatchers.empty()) return;
        std::filesystem::path activeTarget =
            !g_lastSelectedProfilePath.empty() ? g_lastSelectedProfilePath : g_filterJsonPath;
        if (activeTarget.empty()) {
            activeTarget = Schema::ResolveConfiguredProfilePath(
                g_selectedFilter, g_discoveredProfiles);
        }
        for (auto& cw : g_candidateWatchers) {
            if (!activeTarget.empty() && !Schema::PathEquivalent(cw.path, activeTarget)) {
                continue;
            }
            const auto stamp = Schema::ReadFilterFileStamp(cw.path);
            if (cw.watcher.Observe(stamp, nowMs, 600U)) {
                changedPath = cw.path;
                break;
            }
        }
    }
    if (!changedPath.empty()) {
        (void)ReloadFilterJson("auto-reload", changedPath);
    }
}

// ============================================================================
// Hook 1: 0x1FA9F0 (LabelFormatter)
// ============================================================================
std::uint8_t __fastcall DetourLabelFormatter(
    void* unit, void* dest, void* record, std::uint32_t flags,
    std::uint64_t stack5, std::uint64_t stack6) noexcept {
    const auto orig = g_originalLabelFormatter;
    if (!orig) return 0;

    if (!IsPyreFilterActive() || !unit || !dest) {
        return orig(unit, dest, record, flags, stack5, stack6);
    }

    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const auto base = g_readers.exeBase;
    const bool firstCall = (caller == base + Labels::Rva::LabelFormatterCall1 + Labels::Rva::DirectCallBytes);
    const bool secondCall = (caller == base + Labels::Rva::LabelFormatterCall2 + Labels::Rva::DirectCallBytes);
    const bool controllerCall = (caller == base + Labels::Rva::ControllerFormatterReturn);

    if (!firstCall && !secondCall && !controllerCall) {
        return orig(unit, dest, record, flags, stack5, stack6);
    }

    bool paired = false;
    void* const effectiveRecord = Labels::ResolveEffectiveFormatterRecord(
        firstCall, secondCall, controllerCall, dest, record, paired);
    if (!paired) {
        return orig(unit, dest, record, flags, stack5, stack6);
    }

    PerfScope perf(g_perfFormatter);
    const auto table = CurrentRules();
    const auto prevScope = Labels::g_activeFormatter;
    Labels::g_activeFormatter = {};

    if (table) {
        Reader::UnitHeader hdr{};
        if (Reader::ReadUnitHeader(unit, hdr) &&
            Reader::AllowsMode(hdr.mode, Reader::Purpose::VerifiedLabel)) {
            Schema::ItemFacts facts{};
            Schema::EvaluatedDecision decision{};
            const bool matched = g_readers.EvaluateUnit(
                unit, 0, table, g_cache, Reader::Purpose::VerifiedLabel, facts, decision);

            if (matched && decision.show && decision.hasMinimapIcon &&
                g_minimapIcons.load(std::memory_order_relaxed)) {
                (void)g_minimap.ObserveEvaluatedGroundUnit(unit, hdr, facts, decision,
                                                           table->generation, g_ctx);
            }

            // Reject hidden items directly in LabelFormatter (0x1FA9F0 -> return 0) so they
            // never consume D2R's 32-label array cap (0x1517149 / 0x15176D9 / 0x151A23D) or
            // occupy space in the 16-step screen-space collision solver (0x1516A90 / 0x15190A0).
            if (matched && !decision.show) {
                const bool ctrlOverride =
                    (firstCall || secondCall) &&
                    g_altPickupOverride.load(std::memory_order_relaxed) &&
                    AudioAndPickup::IsKeyboardCtrlHeldCached(GetTickCount64());
                if (!ctrlOverride) {
                    Labels::ResetRejectedFormatterRecord(
                        effectiveRecord, dest, firstCall || secondCall);
                    Labels::g_activeFormatter = prevScope;
                    return 0;
                }
            }

            const std::uint32_t qty =
                (!Tables::IsGoldCode(facts.code) && facts.quantityKnown) ? facts.quantity : 1U;

            Labels::g_activeFormatter = Labels::ActiveFormatterScope{
                .unit = unit,
                .dest = dest,
                .unitId = hdr.unitId,
                .quantity = qty,
                .hasDecision = matched,
                .decision = decision,
            };
        }
    }

    const auto res = orig(unit, dest, record, flags, stack5, stack6);
    Labels::g_activeFormatter = prevScope;
    return res;
}

// ============================================================================
// Hook 2: 0xCBEB0 (InnerNameWriter)
// ============================================================================
std::uint64_t __fastcall DetourInnerNameWriter(
    void* unit, void* output, std::uint32_t bufferSize, void* outMetadata) noexcept {
    const auto orig = g_originalInnerNameWriter;
    if (!orig) return 0;

    const auto result = orig(unit, output, bufferSize, outMetadata);
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (caller != g_readers.exeBase + Labels::Rva::InnerNameWriterCaller + Labels::Rva::DirectCallBytes ||
        !unit || !output || bufferSize != Labels::Rva::InnerNameBufferBytes) {
        return result;
    }

    const auto& scope = Labels::g_activeFormatter;
    if (scope.unit != unit || scope.dest != output || !scope.unitId) {
        return result;
    }

    const bool hasCustomName = scope.hasDecision && scope.decision.hasName && scope.decision.bytes > 1;
    const bool hasStackQty = scope.quantity > 1U;
    if (!hasCustomName && !hasStackQty) {
        return result;
    }

    PerfScope perf(g_perfInnerName);
    const auto outputAddr = reinterpret_cast<std::uintptr_t>(output);
    if (outputAddr < Labels::Rva::GroundTextOffset ||
        outputAddr > UINTPTR_MAX - Labels::Rva::InnerNameBufferBytes) {
        return result;
    }

    constexpr std::size_t kCapacity =
        Labels::Rva::InnerNameBufferBytes - Labels::Rva::InnerNamePrefixBytes;
    const auto textAddr = outputAddr + Labels::Rva::InnerNamePrefixBytes;
    std::array<char, kCapacity> currentBuf{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(textAddr), currentBuf.data(),
                              currentBuf.size())) {
        return result;
    }
    const auto* nul = static_cast<const char*>(
        std::memchr(currentBuf.data(), '\0', currentBuf.size()));
    if (!nul || nul == currentBuf.data()) {
        return result;
    }

    const std::string_view nativeName(currentBuf.data(),
                                      static_cast<std::size_t>(nul - currentBuf.data()));
    std::array<char, Labels::Rva::InnerNameBufferBytes> expandedBuf{};
    std::size_t expandedLen = 0;
    std::string_view baseName = nativeName;
    if (hasCustomName) {
        const std::string_view configuredName(scope.decision.name.data(),
                                              scope.decision.bytes - 1U);
        if (Cache::QuantityFormat::ExpandNameTemplate(configuredName, nativeName,
                                                      expandedBuf.data(), kCapacity, expandedLen)) {
            baseName = std::string_view(expandedBuf.data(), expandedLen);
        } else {
            baseName = configuredName;
        }
    }

    std::array<char, Labels::Rva::InnerNameBufferBytes> formatted{};
    const char* replacement = baseName.data();
    std::size_t replacementBytes = baseName.size() + 1U;

    if (hasStackQty) {
        std::size_t formattedLen = 0;
        if (Cache::QuantityFormat::Append(baseName, scope.quantity, formatted.data(), kCapacity,
                                          formattedLen)) {
            replacement = formatted.data();
            replacementBytes = formattedLen + 1U;
        } else if (!hasCustomName) {
            return result;
        }
    }

    if (replacementBytes <= kCapacity) {
        (void)Reader::WriteWritable(reinterpret_cast<void*>(textAddr), replacement,
                                    replacementBytes);
    }
    return result;
}

// ============================================================================
// Hook 3: 0x1FA8E0 (SharedLabelPaint)
// ============================================================================
void __fastcall DetourSharedLabelPaint(void* rect, void* textArg, void* colorArg) noexcept {
    const auto orig = g_originalSharedLabelPaint;
    if (!orig) return;

    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const auto base = g_readers.exeBase;
    const bool isKeyboardGroundCall = (caller == base + Labels::Rva::KeyboardPaintReturn);
    const bool isGroundCall = isKeyboardGroundCall ||
                              (caller == base + Labels::Rva::ControllerPaintReturn);
    if (!isGroundCall || !IsPyreFilterActive()) {
        orig(rect, textArg, colorArg);
        return;
    }

    const auto table = CurrentRules();
    if (!table || (table->hiddenRules == 0 && table->backgroundRules == 0 &&
                   table->textColorRules == 0 && table->borderColorRules == 0)) {
        orig(rect, textArg, colorArg);
        return;
    }

    PerfScope perf(g_perfPaint);
    const auto recAddr = reinterpret_cast<std::uintptr_t>(rect);
    const auto textAddr = reinterpret_cast<std::uintptr_t>(textArg);
    const auto colAddr = reinterpret_cast<std::uintptr_t>(colorArg);
    const bool paired = rect && textArg && colorArg &&
                        recAddr <= UINTPTR_MAX - 0x80U &&
                        textAddr == recAddr + Labels::Rva::GroundTextOffset &&
                        colAddr == recAddr + Labels::Rva::GroundColorOffset;

    if (!paired) {
        orig(rect, textArg, colorArg);
        return;
    }

    std::uint32_t recordId = 0;
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(recAddr + Labels::Rva::GroundRecordIdOffset),
                              &recordId, sizeof(recordId)) ||
        recordId == 0) {
        orig(rect, textArg, colorArg);
        return;
    }

    const auto nowMs = GetTickCount64();
    Schema::ItemFacts facts{};
    Schema::EvaluatedDecision decision{};
    if (!g_cache.LookupByUnitId(recordId, table->generation, nowMs,
                                table->usesSellPrice, facts, decision) ||
        !decision.matched) {
        orig(rect, textArg, colorArg);
        return;
    }

    // Conceal ground label when rule has show:false (unless keyboard Ctrl override is held)
    if (!decision.show) {
        if (!(isKeyboardGroundCall &&
              g_altPickupOverride.load(std::memory_order_relaxed) &&
              AudioAndPickup::IsKeyboardCtrlHeldCached(nowMs))) {
            return;
        }
    }

    void* forwardedColor = colorArg;
    std::array<float, 4> customBg{};
    if (decision.hasBackground) {
        std::array<float, 4> nativeRgba{};
        if (Reader::CopyReadable(colorArg, nativeRgba.data(), sizeof(nativeRgba)) &&
            Labels::IsKnownNativeBlackBackground(nativeRgba)) {
            customBg = decision.background;
            forwardedColor = customBg.data();
        }
    }

    const float* const prevGlyphColor = Labels::g_activeGlyphColor;
    std::array<float, 4> customText{};
    if (decision.hasTextColor) {
        customText = decision.textColor;
        Labels::g_activeGlyphColor = customText.data();
    } else {
        Labels::g_activeGlyphColor = nullptr;
    }

    orig(rect, textArg, forwardedColor);
    Labels::g_activeGlyphColor = prevGlyphColor;

    if (decision.hasBorderColor) {
        Labels::DrawLabelBorder(rect, decision.borderColor, g_isHdMode,
                                g_legacyCoordToScreen, g_drawUiFilledRect);
    }
}

// ============================================================================
// Hook 4: 0x658510 (CorrectedGlyphB)
// ============================================================================
std::uint64_t __fastcall DetourCorrectedGlyphB(
    void* glyphContext, float x, float y, const float* nativeRgba) noexcept {
    const auto orig = g_originalCorrectedGlyphB;
    if (!orig) return 0;

    const float* const overrideColor = Labels::g_activeGlyphColor;
    if (!overrideColor) {
        return orig(glyphContext, x, y, nativeRgba);
    }

    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (caller != g_readers.exeBase + Labels::Rva::CorrectedGlyphBCaller + Labels::Rva::DirectCallBytes ||
        !nativeRgba) {
        return orig(glyphContext, x, y, nativeRgba);
    }

    PerfScope perf(g_perfGlyph);
    std::array<float, 4> before{};
    if (!Reader::CopyReadable(nativeRgba, before.data(), sizeof(before))) {
        return orig(glyphContext, x, y, nativeRgba);
    }
    for (float c : before) {
        if (!std::isfinite(c) || c < 0.0f || c > 1.001f) {
            return orig(glyphContext, x, y, nativeRgba);
        }
    }
    return orig(glyphContext, x, y, overrideColor);
}

// ============================================================================
// Hook 5: 0xD76E0 (AutomapRenderUnit) + Overlay Frame & Game-Join Callbacks
// ============================================================================
void FlushPendingInGameChatNotice() noexcept {
    if (!g_ctx || !g_pendingInGameChatNotice.exchange(false, std::memory_order_acq_rel)) {
        return;
    }
    std::string msg;
    bool isErr = false;
    {
        std::lock_guard lock(g_profileMutex);
        msg = g_startupChatMessage;
        isErr = g_startupChatIsError;
    }
    if (!msg.empty()) {
        g_ctx->WriteConsoleMessage(
            msg.c_str(),
            isErr ? D2RL::ConsoleMessageKind::Warning : D2RL::ConsoleMessageKind::Output);
    }
}

thread_local bool g_updatingShowItems = false;

void ResetGameJoinShowItemsState() noexcept {
    g_showItemsTracker.ResetForGameJoin();
}

void CheckControllerToKeyboardSwap() noexcept {
    g_showItemsTracker.ObserveControllerMode(
        g_readers.exeBase, g_showItemsAdmitted.load(std::memory_order_relaxed));
}

void TryApplyGameJoinActionsOnUiThread() noexcept {
    if (g_updatingShowItems || !g_ready.load(std::memory_order_relaxed) || !g_readers.exeBase) {
        return;
    }
    if (g_isInGame && !g_isInGame()) {
        ResetGameJoinShowItemsState();
        return;
    }
    const bool admitted = g_showItemsAdmitted.load(std::memory_order_relaxed);
    bool shouldAutoShow = false;
    if (g_showItemsTracker.pendingJoin.load(std::memory_order_acquire)) {
        shouldAutoShow = admitted &&
                         g_enabled.load(std::memory_order_relaxed) &&
                         !IsInGameLootFilterActive() &&
                         g_autoShowItemsOnJoin.load(std::memory_order_relaxed) &&
                         CurrentRules() != nullptr;
        if (shouldAutoShow) {
            std::lock_guard lock(g_profileMutex);
            if (IsNoneSelection(g_selectedFilter)) {
                shouldAutoShow = false;
            }
        }
    }
    const bool altKeyHeld = AudioAndPickup::IsKeyboardAltHeld(
        GetAsyncKeyState(VK_LMENU), GetAsyncKeyState(VK_RMENU));
    g_updatingShowItems = true;
    const std::uint8_t events = g_showItemsTracker.StepOnUiThread(
        g_readers.exeBase, admitted, g_readers.IsLocalPlayerReady(),
        shouldAutoShow, GetTickCount64(), altKeyHeld);
    g_updatingShowItems = false;

    if ((events & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::AttemptedJoinEnable)) != 0U ||
        g_pendingInGameChatNotice.load(std::memory_order_relaxed)) {
        FlushPendingInGameChatNotice();
    }
    if (g_ctx) {
        if ((events & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::EnabledOnJoin)) != 0U) {
            g_ctx->LogInfo("[PyreLootFilter] Enabled ground item labels on game join.");
        }
        if ((events & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::JoinTimedOut)) != 0U) {
            g_ctx->LogWarn(
                "[PyreLootFilter] Timed out waiting to enable ground item labels on game join.");
        }
        if ((events & static_cast<std::uint8_t>(Labels::ShowItemsStepEvents::SwapTimedOut)) != 0U) {
            g_ctx->LogWarn(
                "[PyreLootFilter] Timed out waiting to reopen ground item labels after input mode switch.");
        }
    }
}

void QueueGameJoinUiTask() noexcept {
    if (!g_uiThreads || !g_uiThreads->runOnUiThread || !g_ctx) {
        return;
    }
    if (!g_showItemsUiTaskQueued.exchange(true, std::memory_order_acq_rel)) {
        if (g_uiThreads->runOnUiThread(
                g_ctx,
                [](const D2RL::PluginContext*, void*) noexcept {
                    g_showItemsUiTaskQueued.store(false, std::memory_order_release);
                    TryApplyGameJoinActionsOnUiThread();
                },
                nullptr) != D2RL::Threads::Result::Success) {
            g_showItemsUiTaskQueued.store(false, std::memory_order_release);
        }
    }
}

auto __cdecl OnUiMessageEvent(const D2RL::PluginContext*,
                              const D2RL::SharedEvents::UiMessageEvent* event,
                              void*) noexcept -> D2RL::SharedEvents::UiMessageAction {
    if (!event || event->structSize < D2RL::SharedEvents::UiMessageEventRequiredSize) {
        return D2RL::SharedEvents::UiMessageAction::Continue;
    }
    if (g_isInGame && !g_isInGame()) {
        ResetGameJoinShowItemsState();
        return D2RL::SharedEvents::UiMessageAction::Continue;
    }
    CheckControllerToKeyboardSwap();
    if (g_showItemsTracker.HasPendingWork() ||
        g_pendingInGameChatNotice.load(std::memory_order_relaxed)) {
        QueueGameJoinUiTask();
    }
    return D2RL::SharedEvents::UiMessageAction::Continue;
}

void __fastcall DetourAutomapRenderUnit(void* unit, void* automapContext) noexcept {
    const auto orig = g_originalAutomapRenderUnit;
    if (!orig) return;

    orig(unit, automapContext);

    if (!g_ready.load(std::memory_order_relaxed) || !unit) {
        return;
    }

    std::uint32_t unitType = 0xFFFFFFFFU;
    if (!Reader::CopyReadable(unit, &unitType, sizeof(unitType))) return;

    if (unitType == 0U && g_pendingInGameChatNotice.load(std::memory_order_relaxed)) {
        FlushPendingInGameChatNotice();
    }

    if (!g_enabled.load(std::memory_order_relaxed) ||
        IsInGameLootFilterActive() ||
        !g_minimapIcons.load(std::memory_order_relaxed) ||
        !automapContext || !g_projectClientToAutomap) {
        return;
    }

    const auto table = CurrentRules();
    if (!table || table->minimapIconRules == 0) return;

    PerfScope perf(g_perfAutomap);
    if (unitType == Reader::UnitTypeItem) {
        (void)g_minimap.ObserveGroundUnit(unit, table, g_readers, g_cache, g_ctx);
        g_minimap.ProjectAndPublish(automapContext, g_projectClientToAutomap, g_readers.exeBase,
                                    table->generation, false, g_ctx);
    } else if (unitType == 0U) {
        // Player unit pass: discover any ground items directly from the client item table
        // and project all active ground item markers into the current automap clip.
        g_minimap.ScanClientGroundItems(g_readers.exeBase, table, g_readers, g_cache, g_ctx);
        g_minimap.ProjectAndPublish(automapContext, g_projectClientToAutomap, g_readers.exeBase,
                                    table->generation, true, g_ctx);
    }
}

void __cdecl OnOverlayFrame(const D2RL::PluginContext* ctx, const D2RL::Overlay::Frame* frame,
                            void*) noexcept {
    PollLiveReloadAndHotkeys();
    if (g_isInGame && !g_isInGame()) {
        ResetGameJoinShowItemsState();
    } else {
        CheckControllerToKeyboardSwap();
        if (g_showItemsTracker.HasPendingWork() ||
            g_pendingInGameChatNotice.load(std::memory_order_relaxed)) {
            QueueGameJoinUiTask();
        }
    }
    if (!IsPyreFilterActive() ||
        !g_minimapIcons.load(std::memory_order_relaxed) || !g_overlay) {
        if (IsInGameLootFilterActive()) {
            g_minimap.Clear();
        }
        return;
    }
    Minimap::OverlayRender::RenderMarkerFrame(
        ctx, g_overlay, frame, g_readers.exeBase,
        g_clipCorrectionAdmitted.load(std::memory_order_relaxed), g_minimap);
}

// ============================================================================
// Hook 6: 0x1C8130 (NativeDropPlayer)
// ============================================================================
void __fastcall DetourNativeDropPlayer(void* unit) noexcept {
    const auto orig = g_originalDropPlayer;
    if (!orig) return;

    if (IsPyreFilterActive() && unit) {
        const auto* const table = CurrentRules();
        if (table && table->minimapIconRules > 0 &&
            g_minimapIcons.load(std::memory_order_relaxed)) {
            (void)g_minimap.ObserveGroundUnit(unit, table, g_readers, g_cache, g_ctx);
        }
        if (table) {
            const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
            const auto callerRva = caller >= g_readers.exeBase ? (caller - g_readers.exeBase) : 0;
            bool missingCustomSound = false;
            const auto action = AudioAndPickup::EvaluateDropSound(
                unit, callerRva, g_soundLoaderBase,
                g_dropSounds.load(std::memory_order_relaxed), table, g_readers, g_cache,
                g_dropSound3dAttenuation.load(std::memory_order_relaxed), &missingCustomSound);
            if (missingCustomSound &&
                !g_warnedMissingSoundRows.exchange(true, std::memory_order_acq_rel) && g_ctx) {
                g_ctx->LogWarn(
                    "[PyreLootFilter] Custom Filter01..16 sound rows not found in loaded Sounds "
                    "table (< 12008 rows); falling back to vanilla drop sounds.");
            }
            if (action == AudioAndPickup::DropSoundAction::PlayedCustom ||
                action == AudioAndPickup::DropSoundAction::SuppressHidden) {
                return;
            }
        }
    }
    orig(unit);
}

// ============================================================================
// Hook 7: 0xFABE0 (NativeActionDispatch - Mouse Pickup Guard)
// ============================================================================
__declspec(noinline) bool ShouldBlockActionDispatchHelper(
    std::uint32_t action, void* player, std::uint32_t type, std::uint32_t unitId) noexcept {
    if (!IsPyreFilterActive()) {
        return false;
    }
    if (action == AudioAndPickup::Rva::PickupActionId && type == Reader::UnitTypeItem && unitId) {
        PerfScope perf(g_perfPickup);
        const auto* const table = CurrentRules();
        if (AudioAndPickup::ShouldBlockMouseGroundPickup(
                action, player, type, unitId,
                g_altPickupOverride.load(std::memory_order_relaxed),
                g_readers.exeBase, table, g_readers, g_cache)) {
            return true;
        }
        Schema::ItemFacts cachedFacts{};
        Schema::EvaluatedDecision cachedDecision{};
        if (table &&
            g_cache.LookupByUnitId(unitId, table->generation, GetTickCount64(),
                                   table->usesSellPrice, cachedFacts, cachedDecision) &&
            Tables::IsGoldCode(cachedFacts.code)) {
            return false;
        }
        // Item pickup is proceeding: invalidate cached state for this unitId so if the player
        // modifies and re-drops the item later, it is re-evaluated fresh.
        g_cache.ForgetUnit(unitId);
        g_minimap.ForgetUnit(unitId);
    }
    return false;
}

// Keep DetourNativeActionDispatch compact so the RIP-relative load of
// g_originalActionDispatch sits within the first 160 bytes of the function
// for Controller QOL's QolUnhoarderDispatch::ValidateDispatchTrampolineOnly.
void __fastcall DetourNativeActionDispatch(
    std::uint32_t action, void* player, std::uint32_t type, std::uint32_t unitId) noexcept {
    if (!ShouldBlockActionDispatchHelper(action, player, type, unitId)) {
        if (const auto orig = g_originalActionDispatch) {
            orig(action, player, type, unitId);
        }
    }
}

// ============================================================================
// Options > Gameplay Settings & Status Integration (D2RL::Settings)
// & Native D2R 3.x In-Game Loot Filter Coexistence
// ============================================================================
[[nodiscard]] std::string BuildCurrentStatusText() {
    const auto* const table = CurrentRules();
    bool enabled = g_enabled.load(std::memory_order_relaxed);
    std::string activeShort;
    std::string lastErr;
    std::string failedLabel;
    {
        std::lock_guard lock(g_profileMutex);
        if (IsNoneSelection(g_selectedFilter)) {
            enabled = false;
        }
        activeShort = g_activeShortPath;
        lastErr = g_lastLoadError;
        failedLabel = g_lastFailedProfileLabel;
    }
    std::string activeInGame;
    if (g_inGameFilterCoexistenceAdmitted.load(std::memory_order_relaxed)) {
        activeInGame = ProfileDropdown::ReadActiveInGameFilterName(g_readers.exeBase);
    }
    return ProfileDropdown::BuildPanelStatusText(
        enabled, table, activeShort, lastErr, failedLabel, activeInGame);
}

void BuildProfileDropdownChoices(std::vector<std::string>& outLabels,
                                 std::vector<std::string>& outConfigVals,
                                 std::int32_t& outSelectedIndex) {
    outLabels.clear();
    outConfigVals.clear();
    outSelectedIndex = 0;

    const bool enabled = g_enabled.load(std::memory_order_relaxed);
    std::lock_guard lock(g_profileMutex);
    outLabels.reserve(g_discoveredProfiles.size() + 1U);
    outConfigVals.reserve(g_discoveredProfiles.size() + 1U);
    outLabels.emplace_back("None (Off)");
    outConfigVals.emplace_back("none");

    const bool noneSelected = IsNoneSelection(g_selectedFilter);
    const auto targetPath =
        !g_lastSelectedProfilePath.empty() ? g_lastSelectedProfilePath : g_filterJsonPath;
    for (std::size_t i = 0; i < g_discoveredProfiles.size(); ++i) {
        outLabels.push_back(g_discoveredProfiles[i].displayName);
        outConfigVals.push_back(g_discoveredProfiles[i].shortPath);
        if (enabled && !noneSelected && !targetPath.empty() &&
            Schema::PathEquivalent(g_discoveredProfiles[i].path, targetPath)) {
            outSelectedIndex = static_cast<std::int32_t>(i + 1U);
        }
    }
}

void SyncSettingsStatusAndDropdown() noexcept {
    try {
        g_statusSetting.SetText(BuildCurrentStatusText());

        std::vector<std::string> labels;
        std::vector<std::string> configVals;
        std::int32_t selectedIndex = 0;
        BuildProfileDropdownChoices(labels, configVals, selectedIndex);

        std::vector<const char*> labelPtrs;
        std::vector<const char*> cfgPtrs;
        labelPtrs.reserve(labels.size());
        cfgPtrs.reserve(configVals.size());
        for (const auto& s : labels) labelPtrs.push_back(s.c_str());
        for (const auto& s : configVals) cfgPtrs.push_back(s.c_str());
        (void)g_profileDropdown.SetOptions(
            labelPtrs.data(), cfgPtrs.data(),
            static_cast<std::uint32_t>(labelPtrs.size()), selectedIndex, false);
    } catch (...) {
    }
}

void SyncSettingsTogglesFromMemory() noexcept {
    (void)g_minimapSetting.ReloadFromConfig(false);
    g_minimapSetting.SetValue(g_minimapIcons.load(std::memory_order_relaxed), false, false);
    (void)g_dropSoundsSetting.ReloadFromConfig(false);
    g_dropSoundsSetting.SetValue(g_dropSounds.load(std::memory_order_relaxed), false, false);
    (void)g_dropSound3dSetting.ReloadFromConfig(false);
    g_dropSound3dSetting.SetValue(g_dropSound3dAttenuation.load(std::memory_order_relaxed), false, false);
    (void)g_autoShowItemsSetting.ReloadFromConfig(false);
    g_autoShowItemsSetting.SetValue(g_autoShowItemsOnJoin.load(std::memory_order_relaxed), false, false);
    (void)g_altOverrideSetting.ReloadFromConfig(false);
    g_altOverrideSetting.SetValue(g_altPickupOverride.load(std::memory_order_relaxed), false, false);
    (void)g_autoReloadSetting.ReloadFromConfig(false);
    g_autoReloadSetting.SetValue(g_autoReload.load(std::memory_order_relaxed), false, false);
}

void OnProfileDropdownRefresh() noexcept {
    RefreshDiscoveredProfiles();
    SyncSettingsStatusAndDropdown();
}

void OnStatusRowRefresh() noexcept {
    try {
        g_statusSetting.SetText(BuildCurrentStatusText());
    } catch (...) {
    }
}

void OnProfileDropdownApplied(std::int32_t selectedIndex) noexcept {
    try {
        if (selectedIndex <= 0) {
            g_enabled.store(false, std::memory_order_release);
            const char* const disabledMsg =
                "PyreLootFilter: disabled via Options > Gameplay (None selected).";
            const bool inGameNow = (g_isInGame != nullptr && g_isInGame());
            {
                std::lock_guard lock(g_profileMutex);
                g_selectedFilter = "none";
                g_filterJsonPath.clear();
                g_activeShortPath.clear();
                g_lastLoadError.clear();
                g_lastFailedProfileLabel.clear();
                g_lastSelectedProfilePath.clear();
                if (!inGameNow) {
                    g_startupChatMessage = disabledMsg;
                    g_startupChatIsError = false;
                    g_pendingInGameChatNotice.store(true, std::memory_order_release);
                } else {
                    g_pendingInGameChatNotice.store(false, std::memory_order_release);
                }
            }
            PublishRules(nullptr);
            ResetGameJoinShowItemsState();
            g_cache.Clear();
            g_minimap.Clear();
            SavePluginConfig(true);
            if (g_ctx) {
                if (inGameNow) {
                    g_ctx->WriteConsoleMessage(disabledMsg);
                }
                g_ctx->LogInfo(disabledMsg);
            }
            SyncSettingsStatusAndDropdown();
            return;
        }

        Schema::FilterProfileDescriptor chosen{};
        bool found = false;
        {
            std::lock_guard lock(g_profileMutex);
            const auto idx = static_cast<std::size_t>(selectedIndex - 1);
            if (idx < g_discoveredProfiles.size()) {
                chosen = g_discoveredProfiles[idx];
                found = true;
                g_lastSelectedProfilePath = chosen.path;
                g_selectedFilter = chosen.shortPath;
            }
        }
        if (!found) return;

        g_enabled.store(true, std::memory_order_release);
        SavePluginConfig(true);
        (void)ReloadFilterJson("options", chosen.path, /*strictTarget=*/true,
                               /*reloadToml=*/false);
    } catch (...) {
    }
}

void OnMinimapSettingChanged(bool value) noexcept {
    g_minimapIcons.store(value, std::memory_order_release);
    if (!value) g_minimap.Clear();
}

void OnDropSoundsSettingChanged(bool value) noexcept {
    g_dropSounds.store(value, std::memory_order_release);
}

void OnDropSound3dSettingChanged(bool value) noexcept {
    g_dropSound3dAttenuation.store(value, std::memory_order_release);
}

void OnAutoShowItemsSettingChanged(bool value) noexcept {
    g_autoShowItemsOnJoin.store(value, std::memory_order_release);
}

void OnAltOverrideSettingChanged(bool value) noexcept {
    g_altPickupOverride.store(value, std::memory_order_release);
}

void OnAutoReloadSettingChanged(bool value) noexcept {
    g_autoReload.store(value, std::memory_order_release);
}

void RegisterPluginSettings(const D2RL::PluginContext* context) noexcept {
    try {
        std::vector<std::string> labels;
        std::vector<std::string> configVals;
        std::int32_t selectedIndex = 0;
        BuildProfileDropdownChoices(labels, configVals, selectedIndex);

        std::vector<const char*> labelPtrs;
        std::vector<const char*> cfgPtrs;
        labelPtrs.reserve(labels.size());
        cfgPtrs.reserve(configVals.size());
        for (const auto& s : labels) labelPtrs.push_back(s.c_str());
        for (const auto& s : configVals) cfgPtrs.push_back(s.c_str());

        (void)g_profileDropdown.Register(context, {
            .key          = "selected_filter",
            .label        = "Loot Filter Profile",
            .tooltip      = "Select an active loot filter profile from d2rloader/config/filter/, or None (Off) to disable filtering.",
            .section      = D2RLSettings::Section::LootFilter,
            .options      = labelPtrs.data(),
            .optionCount  = static_cast<std::uint32_t>(labelPtrs.size()),
            .configValues = cfgPtrs.data(),
            .defaultValue = selectedIndex,
            .onChanged    = &OnProfileDropdownApplied,
            .onRefresh    = &OnProfileDropdownRefresh,
            .tomlSection  = "pyre-lootfilter",
            .saveToConfig = false,
        });
        g_profileDropdown.SetValue(selectedIndex, false, false);

        const std::string initialStatus = BuildCurrentStatusText();
        (void)g_statusSetting.Register(context, {
            .key         = "status",
            .label       = "Loot Filter Status",
            .tooltip     = "Shows the currently loaded loot filter profile, active rule count, or any parse/validation error.",
            .section     = D2RLSettings::Section::LootFilter,
            .initialText = initialStatus.c_str(),
            .onRefresh   = &OnStatusRowRefresh,
        });

        (void)g_minimapSetting.Register(context, {
            .key          = "minimap_icons",
            .label        = "Loot Filter Minimap Icons",
            .tooltip      = "Draw custom minimap icons for ground items that match a filter rule with minimapIcon.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = true,
            .onChanged    = &OnMinimapSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        (void)g_dropSoundsSetting.Register(context, {
            .key          = "drop_sounds",
            .label        = "Loot Filter Drop Sounds",
            .tooltip      = "Play custom drop alert sounds for ground items that match a filter rule with dropSound.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = true,
            .onChanged    = &OnDropSoundsSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        (void)g_dropSound3dSetting.Register(context, {
            .key          = "drop_sound_3d_attenuation",
            .label        = "Loot Filter 3D Drop Audio",
            .tooltip      = "Use 3D positional distance attenuation for custom filter drop sounds instead of full-volume 2D alerts.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = false,
            .onChanged    = &OnDropSound3dSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        (void)g_autoShowItemsSetting.Register(context, {
            .key          = "auto_show_items_on_join",
            .label        = "Loot Filter Auto-Show Items",
            .tooltip      = "Automatically enable ground item labels when joining a game in Toggle Show Items mode.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = true,
            .onChanged    = &OnAutoShowItemsSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        (void)g_altOverrideSetting.Register(context, {
            .key          = "alt_pickup_override",
            .label        = "Loot Filter Ctrl Override",
            .tooltip      = "Hold Keyboard Ctrl while Show Items is active to temporarily reveal and pick up hidden items.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = true,
            .onChanged    = &OnAltOverrideSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        (void)g_autoReloadSetting.Register(context, {
            .key          = "auto_reload",
            .label        = "Loot Filter Auto-Reload",
            .tooltip      = "Automatically reload the active filter JSON file when it is saved on disk.",
            .section      = D2RLSettings::Section::LootFilter,
            .defaultValue = true,
            .onChanged    = &OnAutoReloadSettingChanged,
            .tomlSection  = "pyre-lootfilter",
        });

        SyncSettingsTogglesFromMemory();
    } catch (...) {
    }
}

void ShutdownPluginSettings() noexcept {
    g_autoReloadSetting.Shutdown();
    g_altOverrideSetting.Shutdown();
    g_autoShowItemsSetting.Shutdown();
    g_dropSound3dSetting.Shutdown();
    g_dropSoundsSetting.Shutdown();
    g_minimapSetting.Shutdown();
    g_statusSetting.Shutdown();
    g_profileDropdown.Shutdown();
}

// ============================================================================
// Controller QOL Visibility Callbacks
// ============================================================================
QolInterop::LabelVisibility::Decision __cdecl QueryLabelVisibilityCallback(
    const void* unit, std::uint32_t guid) noexcept {
    PerfScope perf(g_perfQolVis);
    const auto* const table = CurrentRules();
    const bool active = IsPyreFilterActive();
    return QolInterop::EvaluateLabelPreflight(unit, guid, active, table, g_readers, g_cache);
}

QolInterop::GroundVisibility::Decision __cdecl QueryGroundVisibilityCallback(
    const void* unit, std::uint32_t guid, std::uint32_t code) noexcept {
    PerfScope perf(g_perfQolVis);
    const auto* const table = CurrentRules();
    const bool active = IsPyreFilterActive();
    return QolInterop::EvaluateGroundVisibility(unit, guid, code, active, table, g_readers,
                                                g_cache);
}

const QolInterop::GroundVisibility::Service g_groundVisService{
    .size = sizeof(QolInterop::GroundVisibility::Service),
    .version = QolInterop::GroundVisibility::Version,
    .query = &QueryGroundVisibilityCallback,
};

const QolInterop::LabelVisibility::Service g_labelVisService{
    .size = sizeof(QolInterop::LabelVisibility::Service),
    .version = QolInterop::LabelVisibility::Version,
    .query = &QueryLabelVisibilityCallback,
};

// ============================================================================
// Console & Chat Command (/filter)
// ============================================================================
void WriteStatusMessage() noexcept {
    if (!g_ctx) return;
    const auto table = CurrentRules();
    std::string activeShort;
    std::string lastErr;
    {
        std::lock_guard lock(g_profileMutex);
        activeShort = g_activeShortPath;
        lastErr = g_lastLoadError;
    }
    char msg[448]{};
    std::snprintf(
        msg, sizeof(msg),
        "PyreLootFilter %s: %s | file=%s | rules=%zu (v%u, gen=%llu, hide=%zu, bg=%zu, text=%zu, "
        "border=%zu, sound=%zu, minimap=%zu) | minimap=%s | sound=%s (3d=%s) | altOverride=%s | autoReload=%s",
        PluginVersion,
        !g_ready.load() ? "INACTIVE (unsupported build or conflict)"
                        : (IsInGameLootFilterActive() ? "OFF (in-game .fltr active)"
                                                      : (g_enabled.load() ? "ON" : "OFF")),
        !activeShort.empty() ? activeShort.c_str() : "none",
        table ? table->rules.size() : 0U, table ? table->schema : 0U,
        table ? static_cast<unsigned long long>(table->generation) : 0ULL,
        table ? table->hiddenRules : 0U, table ? table->backgroundRules : 0U,
        table ? table->textColorRules : 0U, table ? table->borderColorRules : 0U,
        table ? table->soundRules : 0U, table ? table->minimapIconRules : 0U,
        g_minimapIcons.load() ? "on" : "off",
        g_dropSounds.load() ? "on" : "off",
        g_dropSound3dAttenuation.load() ? "on" : "off",
        g_altPickupOverride.load() ? "on" : "off",
        g_autoReload.load() ? "on" : "off");
    g_ctx->WriteConsoleMessage(msg);
    g_ctx->LogInfo(msg);
    if (!lastErr.empty()) {
        std::string errLine = "PyreLootFilter last load error: " + lastErr;
        g_ctx->WriteConsoleMessage(errLine.c_str(), D2RL::ConsoleMessageKind::Warning);
    }
}

void WritePerfMessage(bool resetAfter) noexcept {
    if (!g_ctx) return;
    LARGE_INTEGER freq{};
    QueryPerformanceFrequency(&freq);
    const double toUs = freq.QuadPart > 0 ? (1000000.0 / static_cast<double>(freq.QuadPart)) : 0.0;

    const auto fmtCalls = g_perfFormatter.calls.load();
    const auto nameCalls = g_perfInnerName.calls.load();
    const auto paintCalls = g_perfPaint.calls.load();
    const auto glyphCalls = g_perfGlyph.calls.load();
    const auto mapCalls = g_perfAutomap.calls.load();
    const auto pickCalls = g_perfPickup.calls.load();
    const auto qolCalls = g_perfQolVis.calls.load();

    char msg[384]{};
    std::snprintf(
        msg, sizeof(msg),
        "PyreLootFilter perf: fmt=%llu (%.1fus) | name=%llu (%.1fus) | paint=%llu (%.1fus) | "
        "glyph=%llu (%.1fus) | map=%llu (%.1fus) | pickup=%llu (%.1fus) | qol=%llu (%.1fus)%s",
        static_cast<unsigned long long>(fmtCalls),
        static_cast<double>(g_perfFormatter.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(nameCalls),
        static_cast<double>(g_perfInnerName.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(paintCalls),
        static_cast<double>(g_perfPaint.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(glyphCalls),
        static_cast<double>(g_perfGlyph.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(mapCalls),
        static_cast<double>(g_perfAutomap.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(pickCalls),
        static_cast<double>(g_perfPickup.totalTicks.load()) * toUs,
        static_cast<unsigned long long>(qolCalls),
        static_cast<double>(g_perfQolVis.totalTicks.load()) * toUs,
        resetAfter ? " [reset]" : "");
    g_ctx->WriteConsoleMessage(msg);
    g_ctx->LogInfo(msg);

    if (resetAfter) {
        g_perfFormatter.Reset();
        g_perfInnerName.Reset();
        g_perfPaint.Reset();
        g_perfGlyph.Reset();
        g_perfAutomap.Reset();
        g_perfPickup.Reset();
        g_perfQolVis.Reset();
    }
}

D2RL::ConsoleCommandResult __cdecl FilterConsoleCommand(
    D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept {
    if (!command || command->contextSize < D2RL::ConsoleCommandContextSize ||
        command->plugin != g_ctx || command->argsLength > 64 ||
        (!command->args && command->argsLength)) {
        return D2RL::ConsoleCommandResult::Failed;
    }

    const auto arg = ChatConfig::Trim(
        std::string_view(command->args ? command->args : "", command->argsLength));

    if (!g_ready.load()) {
        g_ctx->WriteConsoleMessage(
            "PyreLootFilter: inactive (unsupported game build or legacy unhoarder.dll conflict).",
            D2RL::ConsoleMessageKind::Warning);
        return D2RL::ConsoleCommandResult::Failed;
    }

    if (arg == "status") {
        WriteStatusMessage();
        return D2RL::ConsoleCommandResult::Handled;
    }
    if (arg == "reload") {
        std::filesystem::path targetPath;
        {
            std::lock_guard lock(g_profileMutex);
            targetPath = !g_lastSelectedProfilePath.empty() ? g_lastSelectedProfilePath
                                                            : g_filterJsonPath;
        }
        if (ReloadFilterJson("command", targetPath)) {
            WriteStatusMessage();
            return D2RL::ConsoleCommandResult::Handled;
        }
        return D2RL::ConsoleCommandResult::Failed;
    }
    if (arg == "perf" || arg == "perf reset" || arg == "perf on" || arg == "perf off") {
        if (arg == "perf off") {
            g_perfTimingEnabled.store(false, std::memory_order_relaxed);
        } else {
            g_perfTimingEnabled.store(true, std::memory_order_relaxed);
        }
        WritePerfMessage(arg == "perf reset");
        return D2RL::ConsoleCommandResult::Handled;
    }

    bool saveNeeded = false;
    bool persistSelected = false;
    bool enabledTurnedOn = false;
    bool enabledTurnedOff = false;
    if (arg.empty() || arg == "toggle") {
        const bool next = !g_enabled.load();
        g_enabled.store(next);
        enabledTurnedOn = next;
        enabledTurnedOff = !next;
        saveNeeded = true;
    } else if (arg == "on") {
        g_enabled.store(true);
        enabledTurnedOn = true;
        saveNeeded = true;
    } else if (arg == "off") {
        g_enabled.store(false);
        enabledTurnedOff = true;
        saveNeeded = true;
    } else if (arg == "minimap" || arg == "minimap on" || arg == "minimap off") {
        const bool next = (arg == "minimap") ? !g_minimapIcons.load() : (arg == "minimap on");
        g_minimapIcons.store(next);
        if (!next) g_minimap.Clear();
        saveNeeded = true;
    } else if (arg == "sound" || arg == "sound on" || arg == "sound off") {
        const bool next = (arg == "sound") ? !g_dropSounds.load() : (arg == "sound on");
        g_dropSounds.store(next);
        saveNeeded = true;
    } else if (arg == "sound3d" || arg == "sound3d on" || arg == "sound3d off" ||
               arg == "attenuation" || arg == "attenuation on" || arg == "attenuation off") {
        const bool next = (arg == "sound3d" || arg == "attenuation")
                              ? !g_dropSound3dAttenuation.load()
                              : (arg == "sound3d on" || arg == "attenuation on");
        g_dropSound3dAttenuation.store(next);
        saveNeeded = true;
    } else if (arg == "alt" || arg == "alt on" || arg == "alt off" ||
               arg == "ctrl" || arg == "ctrl on" || arg == "ctrl off") {
        const bool next = (arg == "alt" || arg == "ctrl")
                              ? !g_altPickupOverride.load()
                              : (arg == "alt on" || arg == "ctrl on");
        g_altPickupOverride.store(next);
        saveNeeded = true;
    } else if (arg == "autoreload" || arg == "autoreload on" || arg == "autoreload off") {
        const bool next = (arg == "autoreload") ? !g_autoReload.load() : (arg == "autoreload on");
        g_autoReload.store(next);
        saveNeeded = true;
    } else {
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }

    if (enabledTurnedOff) {
        g_pendingInGameChatNotice.store(false, std::memory_order_release);
        ResetGameJoinShowItemsState();
        g_cache.Clear();
        g_minimap.Clear();
    } else if (enabledTurnedOn) {
        if (CurrentRules() == nullptr) {
            {
                std::lock_guard lock(g_profileMutex);
                if (IsNoneSelection(g_selectedFilter)) {
                    g_selectedFilter.clear();
                    persistSelected = true;
                }
            }
            Schema::EnsureFilterDirectoriesExist(g_pathInputs);
            (void)ReloadFilterJson("command", {}, /*strictTarget=*/false, /*reloadToml=*/false);
            {
                std::lock_guard lock(g_profileMutex);
                if (!g_activeShortPath.empty()) {
                    g_selectedFilter = g_activeShortPath;
                    persistSelected = true;
                }
            }
        }
    }

    if (saveNeeded) {
        SavePluginConfig(persistSelected);
        SyncSettingsTogglesFromMemory();
        SyncSettingsStatusAndDropdown();
    }
    WriteStatusMessage();
    return D2RL::ConsoleCommandResult::Handled;
}

void NativeChatFilterCommand(const char* args, void*) noexcept {
    if (!g_ctx) return;
    const auto len = args ? strnlen_s(args, 65) : 0;
    if (len > 64) return;
    const D2RL::ConsoleCommandContext cmd{
        .contextSize = D2RL::ConsoleCommandContextSize,
        .argsLength = static_cast<std::uint32_t>(len),
        .args = args,
        .plugin = g_ctx,
    };
    (void)FilterConsoleCommand(nullptr, &cmd, nullptr);
}

} // namespace
} // namespace PyreLootFilter

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo kInfo{
        .infoSize = D2RL::PluginInfoSize,
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .id = "pyre-lootfilter",
        .name = "Pyre Loot Filter",
        .version = PyreLootFilter::PluginVersion,
        .author = "PyreFly",
        .description = "Clean-room ground loot filter, custom drop sounds, and minimap icons.",
        .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
    };
    return &kInfo;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    using namespace PyreLootFilter;
    if (!context || !context->GetApi()) return false;
    g_ctx = context;

    LoadTomlSettings();
    if (!g_ctx->RegisterConsoleCommand("filter", &FilterConsoleCommand, CommandHelp)) {
        return false;
    }

    if (GetModuleHandleW(L"unhoarder.dll") != nullptr) {
        g_ctx->LogWarn(
            "[PyreLootFilter] Legacy unhoarder.dll is loaded in this process; refusing to arm "
            "duplicate loot filter hooks. Remove unhoarder.dll to use pyre-lootfilter.");
        return true;
    }

    if (!IsSupportedBuildVersion(D2RL::GetBuildVersion(g_ctx))) {
        g_ctx->LogWarn(
            "[PyreLootFilter] Unsupported or missing game buildVersion; native loot display preserved.");
        return true;
    }
    if (!g_ctx->exeBase) {
        g_ctx->LogWarn("[PyreLootFilter] Missing exeBase; native loot display preserved.");
        return true;
    }

    const HMODULE d2rCoreMod = GetModuleHandleW(L"D2RCore.dll");
    // Steam installs lack Battle.net's .build.info, leaving D2RCore's gameBuild string empty so
    // D2RCore+0x75270 rejects compiler/<ver>/manifest.json and D2RCore+0x1CE540 (Step 19/24)
    // skips compiling mod sounds.txt. Guarded by exact D2RCore 1.3.1-beta PE + code witnesses.
    if (d2rCoreMod) {
        SteamHotfixResult hotfix{};
        if (TryApplySteamGameBuildHotfix(reinterpret_cast<std::uintptr_t>(d2rCoreMod), &hotfix)) {
            char msg[160]{};
            std::snprintf(
                msg, sizeof(msg),
                "[PyreLootFilter] Populated Steam D2RCore gameBuild '%s' from version '%s'.",
                hotfix.gameBuild.data(), hotfix.gameVersion.data());
            g_ctx->LogInfo(msg);
        }
    }

    // Verify core label pipeline witnesses before installing any hooks
    if (!g_ctx->CheckExpectedBytes(Labels::Rva::LabelFormatter,
                                   Labels::Witnesses::LabelFormatterEntry.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::LabelFormatterEntry.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::LabelFormatterCall1,
                                   Labels::Witnesses::LabelFormatterCall1Bytes.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::LabelFormatterCall1Bytes.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::LabelFormatterCall2,
                                   Labels::Witnesses::LabelFormatterCall2Bytes.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::LabelFormatterCall2Bytes.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::InnerNameWriter,
                                   Labels::Witnesses::InnerNameWriterEntry.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::InnerNameWriterEntry.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::InnerNameWriterCaller,
                                   Labels::Witnesses::InnerNameCallBytes.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::InnerNameCallBytes.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::SharedLabelPaint,
                                   Labels::Witnesses::SharedLabelPaintEntry.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::SharedLabelPaintEntry.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::CorrectedGlyphB,
                                   Labels::Witnesses::CorrectedGlyphBEntry.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::CorrectedGlyphBEntry.size())) ||
        !g_ctx->CheckExpectedBytes(Labels::Rva::CorrectedGlyphBCaller,
                                   Labels::Witnesses::CorrectedGlyphBCallBytes.data(),
                                   static_cast<std::uint32_t>(Labels::Witnesses::CorrectedGlyphBCallBytes.size()))) {
        g_ctx->LogWarn("[PyreLootFilter] Label pipeline byte witness mismatch; refusing to arm.");
        return true;
    }

    g_readers.Initialize(g_ctx->exeBase);
    g_isInGame = d2rCoreMod ? reinterpret_cast<IsInGameFn>(GetProcAddress(d2rCoreMod, "IsInGame")) : nullptr;
    const bool showItemsGuardsOk =
        g_ctx->CheckExpectedBytes(
            Labels::Rva::SetShowItemsState,
            Labels::Witnesses::SetShowItemsStateEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::SetShowItemsStateEntry.size())) &&
        g_ctx->CheckExpectedBytes(
            Labels::Rva::RefreshShowItemsUi,
            Labels::Witnesses::RefreshShowItemsUiEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::RefreshShowItemsUiEntry.size())) &&
        g_ctx->CheckExpectedBytes(
            Labels::Rva::IsControllerMode,
            Labels::Witnesses::IsControllerModeEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::IsControllerModeEntry.size())) &&
        g_ctx->CheckExpectedBytes(
            Labels::Rva::IsLoadingScreenActive,
            Labels::Witnesses::IsLoadingScreenActiveEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::IsLoadingScreenActiveEntry.size())) &&
        g_ctx->CheckExpectedBytes(
            Labels::Rva::FindUiStatePanel,
            Labels::Witnesses::FindUiStatePanelEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::FindUiStatePanelEntry.size()));
    g_showItemsAdmitted.store(showItemsGuardsOk, std::memory_order_relaxed);
    if (!showItemsGuardsOk) {
        g_ctx->LogWarn("[PyreLootFilter] Show-items placard byte witnesses failed; default-on placards unavailable.");
    }
    if (g_ctx->CheckExpectedBytes(Labels::Rva::SharedPaintIsHdCall,
                                  Labels::Witnesses::SharedPaintIsHdCallBytes.data(),
                                  static_cast<std::uint32_t>(Labels::Witnesses::SharedPaintIsHdCallBytes.size())) &&
        g_ctx->CheckExpectedBytes(Labels::Rva::SharedPaintLegacyCoordCall,
                                  Labels::Witnesses::SharedPaintLegacyCoordCallBytes.data(),
                                  static_cast<std::uint32_t>(Labels::Witnesses::SharedPaintLegacyCoordCallBytes.size())) &&
        g_ctx->CheckExpectedBytes(Labels::Rva::SharedPaintRectCall,
                                  Labels::Witnesses::SharedPaintRectCallBytes.data(),
                                  static_cast<std::uint32_t>(Labels::Witnesses::SharedPaintRectCallBytes.size())) &&
        g_ctx->CheckExpectedBytes(Labels::Rva::DrawUiFilledRect,
                                  Labels::Witnesses::DrawUiFilledRectEntry.data(),
                                  static_cast<std::uint32_t>(Labels::Witnesses::DrawUiFilledRectEntry.size()))) {
        g_isHdMode = reinterpret_cast<Labels::IsHdModeFn>(
            g_ctx->exeBase + Labels::Rva::IsHdMode);
        g_legacyCoordToScreen = reinterpret_cast<Labels::LegacyCoordToScreenFn>(
            g_ctx->exeBase + Labels::Rva::LegacyCoordToScreen);
        g_drawUiFilledRect = reinterpret_cast<Labels::DrawUiFilledRectFn>(
            g_ctx->exeBase + Labels::Rva::DrawUiFilledRect);
    } else {
        g_ctx->LogWarn("[PyreLootFilter] Label border UI rect witnesses failed; tooltip borders unavailable.");
    }

    g_pathInputs = BuildFilterPathInputs(g_ctx);
    g_activeModRoot = Schema::ResolveActiveModRoot(g_pathInputs);
    Schema::EnsureFilterDirectoriesExist(g_pathInputs);

    // Verify native D2R 3.x LootFilterConfig witness for active .fltr check
    const bool inGameFilterGuardsOk =
        g_ctx->CheckExpectedBytes(
            ProfileDropdown::Rva::GetLootFilterConfig,
            ProfileDropdown::Witnesses::GetLootFilterConfigEntry.data(),
            static_cast<std::uint32_t>(ProfileDropdown::Witnesses::GetLootFilterConfigEntry.size()));
    g_inGameFilterCoexistenceAdmitted.store(inGameFilterGuardsOk, std::memory_order_relaxed);
    if (!inGameFilterGuardsOk) {
        g_ctx->LogWarn(
            "[PyreLootFilter] In-game LootFilterConfig byte witness failed; .fltr check unavailable.");
    }

    if (g_enabled.load(std::memory_order_relaxed)) {
        (void)ReloadFilterJson("startup");
    } else {
        RefreshDiscoveredProfiles();
        g_ctx->LogInfo("[PyreLootFilter] Disabled via config (enabled = false).");
    }

    // Install core label pipeline hooks
    if (!g_ctx->InstallInlineHook(
            Labels::Rva::LabelFormatter, Labels::Witnesses::LabelFormatterEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::LabelFormatterEntry.size()),
            &DetourLabelFormatter, &g_originalLabelFormatter) ||
        !g_originalLabelFormatter ||
        !g_ctx->InstallInlineHook(
            Labels::Rva::InnerNameWriter, Labels::Witnesses::InnerNameWriterEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::InnerNameWriterEntry.size()),
            &DetourInnerNameWriter, &g_originalInnerNameWriter) ||
        !g_originalInnerNameWriter ||
        !g_ctx->InstallInlineHook(
            Labels::Rva::SharedLabelPaint, Labels::Witnesses::SharedLabelPaintEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::SharedLabelPaintEntry.size()),
            &DetourSharedLabelPaint, &g_originalSharedLabelPaint) ||
        !g_originalSharedLabelPaint ||
        !g_ctx->InstallInlineHook(
            Labels::Rva::CorrectedGlyphB, Labels::Witnesses::CorrectedGlyphBEntry.data(),
            static_cast<std::uint32_t>(Labels::Witnesses::CorrectedGlyphBEntry.size()),
            &DetourCorrectedGlyphB, &g_originalCorrectedGlyphB) ||
        !g_originalCorrectedGlyphB) {
        g_ctx->LogWarn("[PyreLootFilter] Failed to install core label pipeline hooks.");
        return true;
    }

    // Install Automap observer + OverlayService callback for minimap icons and live reload
    const bool clipGuardsOk =
        g_ctx->CheckExpectedBytes(Minimap::Rva::DrawListRva,
                                  Minimap::Witnesses::DrawListBytes.data(),
                                  static_cast<std::uint32_t>(Minimap::Witnesses::DrawListBytes.size())) &&
        g_ctx->CheckExpectedBytes(Minimap::Rva::PushClipRva,
                                  Minimap::Witnesses::PushClipBytes.data(),
                                  static_cast<std::uint32_t>(Minimap::Witnesses::PushClipBytes.size())) &&
        g_ctx->CheckExpectedBytes(Minimap::Rva::PopClipRva,
                                  Minimap::Witnesses::PopClipBytes.data(),
                                  static_cast<std::uint32_t>(Minimap::Witnesses::PopClipBytes.size()));
    g_clipCorrectionAdmitted.store(clipGuardsOk, std::memory_order_relaxed);
    if (!clipGuardsOk) {
        g_ctx->LogWarn("[PyreLootFilter] ImDrawList clip byte witnesses failed; minimap icons unavailable.");
    }

    if (g_ctx->CheckExpectedBytes(Minimap::Rva::AutomapRenderUnit,
                                  Minimap::Witnesses::AutomapRenderUnitEntry.data(),
                                  static_cast<std::uint32_t>(Minimap::Witnesses::AutomapRenderUnitEntry.size())) &&
        g_ctx->CheckExpectedBytes(Minimap::Rva::ProjectClientToAutomap,
                                  Minimap::Witnesses::ProjectClientToAutomapEntry.data(),
                                  static_cast<std::uint32_t>(Minimap::Witnesses::ProjectClientToAutomapEntry.size()))) {
        g_projectClientToAutomap = reinterpret_cast<Minimap::ProjectClientToAutomapFn>(
            g_ctx->exeBase + Minimap::Rva::ProjectClientToAutomap);
        if (!g_ctx->InstallInlineHook(
                Minimap::Rva::AutomapRenderUnit,
                Minimap::Witnesses::AutomapRenderUnitEntry.data(),
                static_cast<std::uint32_t>(Minimap::Witnesses::AutomapRenderUnitEntry.size()),
                &DetourAutomapRenderUnit, &g_originalAutomapRenderUnit)) {
            g_ctx->LogWarn("[PyreLootFilter] Automap hook refused; minimap icons unavailable.");
        }
    }

    const D2RL::OverlayService* overlaySvc = nullptr;
    if (g_ctx->QueryService(&overlaySvc) == D2RL::ServiceQueryResult::Success &&
        D2RL::HasOverlayServiceField(overlaySvc, D2RL::OverlayServiceRequiredSize) &&
        overlaySvc->registerFrameCallback != nullptr) {
        g_overlay = overlaySvc;
        const D2RL::Overlay::CallbackRegistration reg{
            .structSize = D2RL::Overlay::CallbackRegistrationSize,
            .flags = 0,
            .phase = D2RL::Overlay::Phase::AfterGameUi,
            .priority = 0,
            .callback = &OnOverlayFrame,
            .userData = nullptr,
        };
        const auto regRes = g_overlay->registerFrameCallback(g_ctx, &reg, &g_overlayCallbackHandle);
        if (regRes != D2RL::Overlay::Result::Success) {
            g_ctx->LogWarn("[PyreLootFilter] OverlayService::registerFrameCallback failed.");
        }
    } else {
        g_ctx->LogWarn("[PyreLootFilter] OverlayService unavailable; minimap icons will not draw.");
    }

    // Install custom drop sound hook
    g_soundLoaderBase = AudioAndPickup::QualifyNamedSoundLoaderBase(g_ctx->exeBase);
    if (g_soundLoaderBase != 0 &&
        g_ctx->CheckExpectedBytes(AudioAndPickup::Rva::NativeDropPlayer,
                                  AudioAndPickup::Witnesses::NativeDropPlayerEntry.data(),
                                  static_cast<std::uint32_t>(AudioAndPickup::Witnesses::NativeDropPlayerEntry.size()))) {
        (void)g_ctx->InstallInlineHook(
            AudioAndPickup::Rva::NativeDropPlayer,
            AudioAndPickup::Witnesses::NativeDropPlayerEntry.data(),
            static_cast<std::uint32_t>(AudioAndPickup::Witnesses::NativeDropPlayerEntry.size()),
            &DetourNativeDropPlayer, &g_originalDropPlayer);
    }

    // Install mouse ground pickup guard hook (with Alt override)
    if (g_ctx->CheckExpectedBytes(AudioAndPickup::Rva::NativeActionDispatch,
                                  AudioAndPickup::Witnesses::NativeActionDispatchEntry.data(),
                                  static_cast<std::uint32_t>(AudioAndPickup::Witnesses::NativeActionDispatchEntry.size()))) {
        (void)g_ctx->InstallInlineHook(
            AudioAndPickup::Rva::NativeActionDispatch,
            AudioAndPickup::Witnesses::NativeActionDispatchEntry.data(),
            static_cast<std::uint32_t>(AudioAndPickup::Witnesses::NativeActionDispatchEntry.size()),
            &DetourNativeActionDispatch, &g_originalActionDispatch);
    }

    // Register Options > Gameplay rows via D2RL::Settings
    RegisterPluginSettings(g_ctx);

    // Publish Controller QOL visibility services
    if (QolInterop::PublishVisibilityServices(g_ctx, g_groundVisService, g_labelVisService)) {
        g_ctx->LogInfo("[PyreLootFilter] Published Controller QOL visibility services.");
    }

    if (g_ctx->QueryService(&g_uiThreads) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasThreadServiceField(g_uiThreads, D2RL::ThreadServiceRequiredSize) ||
        !g_uiThreads->runOnUiThread) {
        g_uiThreads = nullptr;
    }

    const D2RL::SharedEventService* sharedEvtSvc = nullptr;
    if (g_ctx->QueryService(&sharedEvtSvc) == D2RL::ServiceQueryResult::Success &&
        D2RL::HasSharedEventServiceField(sharedEvtSvc, D2RL::SharedEventServiceRequiredSize) &&
        sharedEvtSvc->registerUiMessageListener != nullptr) {
        g_sharedEvents = sharedEvtSvc;
        const D2RL::SharedEvents::UiMessageListener uiListener{
            .structSize = D2RL::SharedEvents::UiMessageListenerSize,
            .priority = 0,
            .callback = &OnUiMessageEvent,
        };
        if (g_sharedEvents->registerUiMessageListener(g_ctx, &uiListener, &g_uiMessageHandle) !=
            D2RL::SharedEvents::Result::Success) {
            g_uiMessageHandle = 0;
        }
    }

    ResetGameJoinShowItemsState();
    g_ready.store(true, std::memory_order_release);

    try {
        if (!g_chatCommands.Start(g_ctx) ||
            !g_chatCommands.Register("filter", &NativeChatFilterCommand, nullptr, CommandHelp)) {
            g_ctx->LogWarn("[PyreLootFilter] Native chat command (/filter) unavailable; console command remains active.");
        }
    } catch (...) {
        g_ctx->LogWarn("[PyreLootFilter] Native chat command registration failed; console command remains active.");
    }

    g_ctx->LogInfo("[PyreLootFilter] Ready.");
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace PyreLootFilter;
    ShutdownPluginSettings();
    g_chatCommands.Stop();
    g_ready.store(false, std::memory_order_release);
    g_enabled.store(false, std::memory_order_release);
    g_pendingInGameChatNotice.store(false, std::memory_order_release);
    g_showItemsTracker.ClearAll();
    g_showItemsUiTaskQueued.store(false, std::memory_order_release);
    g_warnedMissingSoundRows.store(false, std::memory_order_release);
    if (g_sharedEvents && g_sharedEvents->unregisterUiMessageListener &&
        g_uiMessageHandle != 0 && g_ctx) {
        (void)g_sharedEvents->unregisterUiMessageListener(g_ctx, g_uiMessageHandle);
    }
    g_uiMessageHandle = 0;
    g_sharedEvents = nullptr;
    g_uiThreads = nullptr;
    g_isInGame = nullptr;
    if (g_overlay && g_overlay->unregisterFrameCallback &&
        g_overlayCallbackHandle != D2RL::Overlay::InvalidCallbackHandle && g_ctx) {
        (void)g_overlay->unregisterFrameCallback(g_ctx, g_overlayCallbackHandle);
    }
    g_overlayCallbackHandle = D2RL::Overlay::InvalidCallbackHandle;
    g_overlay = nullptr;
    g_cache.Clear();
    g_minimap.Clear();
    g_activeModRoot.clear();
    {
        std::lock_guard lock(g_profileMutex);
        g_candidateWatchers.clear();
        g_filterJsonPath.clear();
        g_discoveredProfiles.clear();
        g_selectedFilter.clear();
        g_activeShortPath.clear();
        g_lastLoadError.clear();
        g_lastFailedProfileLabel.clear();
        g_lastSelectedProfilePath.clear();
        g_startupChatMessage.clear();
        g_startupChatIsError = false;
    }
    PublishRules(nullptr);
}
