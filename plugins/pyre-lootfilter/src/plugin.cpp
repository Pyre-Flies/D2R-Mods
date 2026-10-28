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
#include "qol_interop.h"
#include "unit_cache.h"
#include "unit_reader.h"
#include "../../shared/chat_config.h"
#include <D2RLChat/native_commands.h>
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <bcrypt.h>
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

inline constexpr char PluginVersion[] = "1.0.0+rev.1";
inline constexpr char CommandHelp[] = "status/reload/perf/on/off/toggle/minimap/sound/alt";
static_assert(sizeof(CommandHelp) <= 64);
inline constexpr unsigned ConfigCapacity = 4096U;

struct Settings {
    bool enabled{true};
    bool autoReload{true};
    bool altPickupOverride{true};
    bool minimapIcons{true};
    bool dropSounds{true};
    bool xrayMode{false};
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
D2RLChat::NativeCommands g_chatCommands;

std::atomic<bool> g_ready{false};
std::atomic<bool> g_clipCorrectionAdmitted{false};
std::atomic<bool> g_enabled{true};
std::atomic<bool> g_autoReload{true};
std::atomic<bool> g_altPickupOverride{true};
std::atomic<bool> g_minimapIcons{true};
std::atomic<bool> g_dropSounds{true};
std::atomic<bool> g_xrayMode{false};

std::atomic<std::uint64_t> g_nextGeneration{1};
std::mutex g_rulesMutex;
std::shared_ptr<const Schema::FilterRuleTable> g_publishedRules;
std::array<std::shared_ptr<const Schema::FilterRuleTable>, 32> g_retiredRulesRing{};
std::size_t g_retiredRulesIndex{0};
std::atomic<const Schema::FilterRuleTable*> g_activeRulesPtr{nullptr};
std::filesystem::path g_filterJsonPath;
Schema::LiveReloadWatcher g_reloadWatcher{};
std::uint64_t g_lastWatcherPollMs{0};
bool g_lastReloadHotkeyHeld{false};

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

[[nodiscard]] const Schema::FilterRuleTable* CurrentRules() noexcept {
    return g_activeRulesPtr.load(std::memory_order_acquire);
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

[[nodiscard]] bool VerifyFileSha256(HMODULE module, const char* expectedHex) noexcept {
    if (!module || !expectedHex) return false;
    wchar_t path[4096]{};
    const DWORD len = GetModuleFileNameW(module, path, 4096);
    if (!len || len >= 4096) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool matched = false;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
        DWORD objSize = 0, got = 0;
        if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objSize),
                              sizeof(objSize), &got, 0) >= 0) {
            try {
                std::vector<unsigned char> obj(objSize), buf(65536);
                if (BCryptCreateHash(alg, &hash, obj.data(), objSize, nullptr, 0, 0) >= 0) {
                    DWORD readBytes = 0;
                    bool ok = true;
                    for (;;) {
                        if (!ReadFile(file, buf.data(), static_cast<DWORD>(buf.size()), &readBytes,
                                      nullptr)) {
                            ok = false;
                            break;
                        }
                        if (!readBytes) break;
                        if (BCryptHashData(hash, buf.data(), readBytes, 0) < 0) {
                            ok = false;
                            break;
                        }
                    }
                    unsigned char digest[32]{};
                    if (ok && BCryptFinishHash(hash, digest, 32, 0) >= 0) {
                        char hex[65]{};
                        for (unsigned i = 0; i < 32; ++i) {
                            std::snprintf(hex + i * 2, 3, "%02X", digest[i]);
                        }
                        matched = (std::strcmp(hex, expectedHex) == 0);
                    }
                    BCryptDestroyHash(hash);
                    hash = nullptr;
                }
            } catch (...) {
                if (hash) BCryptDestroyHash(hash);
            }
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    CloseHandle(file);
    return matched;
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
                    else if (k == "alt_pickup_override" || k == "ctrl_pickup_override") s.altPickupOverride = b;
                    else if (k == "minimap_icons") s.minimapIcons = b;
                    else if (k == "drop_sounds") s.dropSounds = b;
                    else if (k == "xray_mode") s.xrayMode = b;
                }
            }
        }
        cursor = (nl == text.npos) ? text.size() : nl + 1;
    }
    return s;
}

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
    g_enabled.store(s.enabled, std::memory_order_release);
    g_autoReload.store(s.autoReload, std::memory_order_release);
    g_altPickupOverride.store(s.altPickupOverride, std::memory_order_release);
    g_minimapIcons.store(s.minimapIcons, std::memory_order_release);
    g_dropSounds.store(s.dropSounds, std::memory_order_release);
    g_xrayMode.store(s.xrayMode, std::memory_order_release);
}

[[nodiscard]] std::filesystem::path ResolveFilterJsonPath(const D2RL::PluginContext* ctx) {
    if (!ctx) return {};
    if (ctx->pluginConfigPath && ctx->pluginConfigPath[0]) {
        return std::filesystem::path(ctx->pluginConfigPath).parent_path() / L"filter.json";
    }
    if (ctx->scopeRootDirectory && ctx->scopeRootDirectory[0]) {
        return std::filesystem::path(ctx->scopeRootDirectory) / L"config" / L"filter.json";
    }
    return {};
}

bool ReloadFilterJson(const char* trigger) noexcept {
    try {
        if (g_filterJsonPath.empty()) return false;
        LoadTomlSettings();
        const auto nextGen = g_nextGeneration.fetch_add(1, std::memory_order_acq_rel);
        const auto table = Schema::LoadFilterRuleTable(
            g_filterJsonPath, nextGen, [](const char* warnMsg) {
                if (g_ctx && warnMsg) g_ctx->LogWarn(warnMsg);
            });
        if (!table) {
            return false;
        }
        table->xrayMode = g_xrayMode.load(std::memory_order_acquire);
        PublishRules(table);
        g_cache.Clear();
        g_minimap.Clear();
        g_reloadWatcher.Resync(Schema::ReadFilterFileStamp(g_filterJsonPath));

        if (g_ctx) {
            char msg[320]{};
            std::snprintf(msg, sizeof(msg),
                          "[PyreLootFilter] Loaded filter.json (%s): schema v%u, %zu rules "
                          "(%zu hide, %zu bg, %zu text, %zu border, %zu sound, %zu minimap, %zu stat) [gen=%llu].",
                          trigger ? trigger : "init", table->schema, table->rules.size(),
                          table->hiddenRules, table->backgroundRules, table->textColorRules,
                          table->borderColorRules, table->soundRules, table->minimapIconRules,
                          table->statRules, static_cast<unsigned long long>(table->generation));
            g_ctx->LogInfo(msg);
        }
        return true;
    } catch (...) {
        if (g_ctx) g_ctx->LogWarn("[PyreLootFilter] Exception while loading filter.json; previous rules preserved.");
        return false;
    }
}

void PollLiveReloadAndHotkeys() noexcept {
    const auto nowMs = GetTickCount64();

    // Check Ctrl+Shift+F9 manual reload chord only when F9 is physically down
    if ((GetAsyncKeyState(VK_F9) & 0x8000) != 0) {
        DWORD fgPid = 0;
        (void)GetWindowThreadProcessId(GetForegroundWindow(), &fgPid);
        if (fgPid == GetCurrentProcessId()) {
            const bool chord = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 &&
                               (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            if (chord && !g_lastReloadHotkeyHeld) {
                if (ReloadFilterJson("Ctrl+Shift+F9")) {
                    if (g_ctx) g_ctx->WriteConsoleMessage("PyreLootFilter: reloaded filter.json (Ctrl+Shift+F9).");
                }
            }
            g_lastReloadHotkeyHeld = chord;
        } else {
            g_lastReloadHotkeyHeld = false;
        }
    } else {
        g_lastReloadHotkeyHeld = false;
    }

    if (!g_autoReload.load(std::memory_order_relaxed) || g_filterJsonPath.empty()) return;
    if (nowMs < g_lastWatcherPollMs || (nowMs - g_lastWatcherPollMs) < 350U) return;
    g_lastWatcherPollMs = nowMs;

    const auto stamp = Schema::ReadFilterFileStamp(g_filterJsonPath);
    if (g_reloadWatcher.Observe(stamp, nowMs, 600U)) {
        (void)ReloadFilterJson("auto-reload");
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

    if (!g_ready.load(std::memory_order_relaxed) ||
        !g_enabled.load(std::memory_order_relaxed) || !unit || !dest) {
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

            if (matched && decision.hasMinimapIcon &&
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
    if (!isGroundCall || !g_ready.load(std::memory_order_relaxed) ||
        !g_enabled.load(std::memory_order_relaxed)) {
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
// Hook 5: 0xD76E0 (AutomapRenderUnit) + Overlay Frame Callback
// ============================================================================
void __fastcall DetourAutomapRenderUnit(void* unit, void* automapContext) noexcept {
    const auto orig = g_originalAutomapRenderUnit;
    if (!orig) return;

    orig(unit, automapContext);

    if (!g_ready.load(std::memory_order_relaxed) ||
        !g_enabled.load(std::memory_order_relaxed) ||
        !g_minimapIcons.load(std::memory_order_relaxed) ||
        !automapContext || !unit || !g_projectClientToAutomap) {
        return;
    }

    const auto table = CurrentRules();
    if (!table || table->minimapIconRules == 0) return;

    PerfScope perf(g_perfAutomap);
    std::uint32_t unitType = 0xFFFFFFFFU;
    if (!Reader::CopyReadable(unit, &unitType, sizeof(unitType))) return;

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
    if (!g_ready.load(std::memory_order_relaxed) ||
        !g_enabled.load(std::memory_order_relaxed) ||
        !g_minimapIcons.load(std::memory_order_relaxed) || !g_overlay) {
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

    if (g_ready.load(std::memory_order_relaxed) &&
        g_enabled.load(std::memory_order_relaxed) && unit) {
        const auto* const table = CurrentRules();
        if (table && table->minimapIconRules > 0 &&
            g_minimapIcons.load(std::memory_order_relaxed)) {
            (void)g_minimap.ObserveGroundUnit(unit, table, g_readers, g_cache, g_ctx);
        }
        if (table) {
            const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
            const auto callerRva = caller >= g_readers.exeBase ? (caller - g_readers.exeBase) : 0;
            const auto action = AudioAndPickup::EvaluateDropSound(
                unit, callerRva, g_soundLoaderBase,
                g_dropSounds.load(std::memory_order_relaxed), table, g_readers, g_cache);
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
    if (!g_ready.load(std::memory_order_relaxed) ||
        !g_enabled.load(std::memory_order_relaxed)) {
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
// Controller QOL Visibility Callbacks
// ============================================================================
QolInterop::LabelVisibility::Decision __cdecl QueryLabelVisibilityCallback(
    const void* unit, std::uint32_t guid) noexcept {
    PerfScope perf(g_perfQolVis);
    const auto* const table = CurrentRules();
    const bool active =
        g_ready.load(std::memory_order_relaxed) && g_enabled.load(std::memory_order_relaxed);
    return QolInterop::EvaluateLabelPreflight(unit, guid, active, table, g_readers, g_cache);
}

QolInterop::GroundVisibility::Decision __cdecl QueryGroundVisibilityCallback(
    const void* unit, std::uint32_t guid, std::uint32_t code) noexcept {
    PerfScope perf(g_perfQolVis);
    const auto* const table = CurrentRules();
    const bool active =
        g_ready.load(std::memory_order_relaxed) && g_enabled.load(std::memory_order_relaxed);
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
    char msg[352]{};
    std::snprintf(
        msg, sizeof(msg),
        "PyreLootFilter %s: %s | rules=%zu (v%u, gen=%llu, hide=%zu, bg=%zu, text=%zu, border=%zu, "
        "sound=%zu, minimap=%zu) | minimap=%s | sound=%s | altOverride=%s | autoReload=%s",
        PluginVersion,
        !g_ready.load() ? "INACTIVE (unsupported build or conflict)"
                        : (g_enabled.load() ? "ON" : "OFF"),
        table ? table->rules.size() : 0U, table ? table->schema : 0U,
        table ? static_cast<unsigned long long>(table->generation) : 0ULL,
        table ? table->hiddenRules : 0U, table ? table->backgroundRules : 0U,
        table ? table->textColorRules : 0U, table ? table->borderColorRules : 0U,
        table ? table->soundRules : 0U, table ? table->minimapIconRules : 0U,
        g_minimapIcons.load() ? "on" : "off",
        g_dropSounds.load() ? "on" : "off",
        g_altPickupOverride.load() ? "on" : "off",
        g_autoReload.load() ? "on" : "off");
    g_ctx->WriteConsoleMessage(msg);
    g_ctx->LogInfo(msg);
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
        if (ReloadFilterJson("command")) {
            WriteStatusMessage();
            return D2RL::ConsoleCommandResult::Handled;
        }
        g_ctx->WriteConsoleMessage(
            "PyreLootFilter: reload failed (check filter.json syntax); previous rules preserved.",
            D2RL::ConsoleMessageKind::Warning);
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
    if (arg.empty() || arg == "toggle") {
        g_enabled.store(!g_enabled.load());
        saveNeeded = true;
    } else if (arg == "on") {
        g_enabled.store(true);
        saveNeeded = true;
    } else if (arg == "off") {
        g_enabled.store(false);
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

    if (saveNeeded) {
        (void)ChatConfig::Save(
            g_ctx, "pyre-lootfilter",
            {
                {"enabled", g_enabled.load()},
                {"auto_reload", g_autoReload.load()},
                {"alt_pickup_override", g_altPickupOverride.load()},
                {"minimap_icons", g_minimapIcons.load()},
                {"drop_sounds", g_dropSounds.load()},
            },
            ConfigCapacity);
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

    const HMODULE d2rCoreMod = GetModuleHandleW(L"D2RCore.dll");
    if (!VerifyFileSha256(reinterpret_cast<HMODULE>(g_ctx->exeBase),
                          "93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10") ||
        !VerifyFileSha256(d2rCoreMod,
                          "2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2")) {
        g_ctx->LogWarn("[PyreLootFilter] Exact-build guard refused; native loot display preserved.");
        return true;
    }

    // D2RLoader 1.3.1 on Steam 3.3.94995 leaves D2RCore's gameBuild string empty because Steam
    // lacks Battle.net's .build.info, causing D2RCore+0x75270 to reject compiler/3.3.94995/manifest.json
    // and D2RCore+0x1CE540 (Step 19/24) to skip compiling mod sounds.txt when building a snapshot.
    if (d2rCoreMod) {
        const auto coreBase = reinterpret_cast<std::uintptr_t>(d2rCoreMod);
        const auto* verChars = reinterpret_cast<const char*>(coreBase + 0x706110);
        const auto verLen = *reinterpret_cast<const std::uint64_t*>(coreBase + 0x706120);
        const auto buildLen = *reinterpret_cast<const std::uint64_t*>(coreBase + 0x706160);
        const auto buildCap = *reinterpret_cast<const std::uint64_t*>(coreBase + 0x706168);
        const auto wbuildLen = *reinterpret_cast<const std::uint64_t*>(coreBase + 0x706140);
        const auto wbuildCap = *reinterpret_cast<const std::uint64_t*>(coreBase + 0x706148);
        if (verLen == 9 && std::memcmp(verChars, "3.3.94995", 9) == 0 &&
            buildLen == 0 && buildCap == 15 && wbuildLen == 0 && wbuildCap == 7) {
            std::memcpy(reinterpret_cast<char*>(coreBase + 0x706150), "94995", 6);
            *reinterpret_cast<std::uint64_t*>(coreBase + 0x706160) = 5;
            std::memcpy(reinterpret_cast<wchar_t*>(coreBase + 0x706130), L"94995", 6 * sizeof(wchar_t));
            *reinterpret_cast<std::uint64_t*>(coreBase + 0x706140) = 5;
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
    g_filterJsonPath = ResolveFilterJsonPath(g_ctx);
    (void)ReloadFilterJson("startup");

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

    // Publish Controller QOL visibility services
    if (QolInterop::PublishVisibilityServices(g_ctx, g_groundVisService, g_labelVisService)) {
        g_ctx->LogInfo("[PyreLootFilter] Published Controller QOL visibility services.");
    }

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
    g_chatCommands.Stop();
    g_ready.store(false, std::memory_order_release);
    g_enabled.store(false, std::memory_order_release);
    if (g_overlay && g_overlay->unregisterFrameCallback &&
        g_overlayCallbackHandle != D2RL::Overlay::InvalidCallbackHandle && g_ctx) {
        (void)g_overlay->unregisterFrameCallback(g_ctx, g_overlayCallbackHandle);
    }
    g_overlayCallbackHandle = D2RL::Overlay::InvalidCallbackHandle;
    g_overlay = nullptr;
    g_cache.Clear();
    g_minimap.Clear();
    PublishRules(nullptr);
}
