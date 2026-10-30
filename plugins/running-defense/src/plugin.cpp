#include <D2RLPlugin/api.h>
#include "config.h"
#include "native_profile.h"
#include "sheet_layout.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string_view>

namespace {

const D2RL::PluginContext* ctx{};
const D2RL::OverlayService* overlay{};
D2RL::Overlay::CallbackHandle frameHandle{};

std::atomic<bool> admitted{};
std::atomic<bool> enabled{true};
std::atomic<int> defenseLossPercent{RunningDefense::DefaultDefenseLossPercent};
std::atomic<bool> overrideBlockPenalty{false};
std::atomic<int> blockLossPercent{RunningDefense::DefaultBlockLossPercent};
std::atomic<bool> showRunningIconOnSheet{true};
std::atomic<bool> highlightDefenseWhenRunning{true};

std::atomic<bool> autoHitPatched{};
std::atomic<bool> sheetBranchPatched{};
std::atomic<bool> blockDivPatched{};
std::atomic<bool> overlayClipReady{};

void* unitsGetDefenseOriginal{};
void* unitsGetBlockRateOriginal{};
void* updateDefenseStatOriginal{};

using UnitsGetDefenseFn = int(__fastcall*)(void* unit);
using UnitsGetAnimModeFn = int(__fastcall*)(void* unit);
using UnitsIsMovingFn = int(__fastcall*)(void* unit);
using UnitsGetBlockRateFn = int(__fastcall*)(void* unit, int expansion);
using UpdateDefenseStatFn = void(__fastcall*)(void* panel, void* player);
using WidgetFindChildFn = void*(__fastcall*)(void* widget, const char* childName);
using GetStatTextColorFn = float*(__fastcall*)(float* outColor, int colorIndex, int paletteIndex);

struct SheetSnapshot {
    RunningDefense::ScreenRect box{};
    std::uintptr_t widgetAddr{};
    std::uint64_t tick{};
    int displayedDefense{};
    bool isRunning{};
    bool valid{};
};

std::mutex sheetMutex;
SheetSnapshot sheetState{};

bool IsUnitRunningPlayer(void* unit) noexcept {
    if (!unit || !ctx) return false;
    __try {
        const auto type = *reinterpret_cast<const std::uint32_t*>(unit);
        if (type != 0U) return false;
        const auto getAnimMode = reinterpret_cast<UnitsGetAnimModeFn>(ctx->exeBase + RunningDefense::Native::UnitsGetAnimModeRva);
        return getAnimMode(unit) == 3;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsUnitRunningForBlock(void* unit) noexcept {
    if (!unit || !ctx) return false;
    __try {
        const auto type = *reinterpret_cast<const std::uint32_t*>(unit);
        if (type != 0U) return false;
        const auto isMoving = reinterpret_cast<UnitsIsMovingFn>(ctx->exeBase + RunningDefense::Native::UnitsIsMovingRva);
        if (!isMoving(unit)) return false;
        const auto getAnimMode = reinterpret_cast<UnitsGetAnimModeFn>(ctx->exeBase + RunningDefense::Native::UnitsGetAnimModeRva);
        return getAnimMode(unit) != 2;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadWidgetBounds(unsigned char* node, RunningDefense::ScreenRect& result) noexcept {
    if (!node) return false;
    std::array<RunningDefense::LayoutNode, 16> chain{};
    std::array<unsigned char*, 16> seen{};
    unsigned count = 0;
    __try {
        while (node && count < chain.size()) {
            if (!node[0x51]) return false; // Every ancestor in the widget hierarchy must be visible.
            for (unsigned i = 0; i < count; ++i) {
                if (seen[i] == node) return false;
            }
            seen[count] = node;
            const auto* r = reinterpret_cast<const int*>(node + 0x70);
            chain[count++] = {
                static_cast<float>(r[0]),
                static_cast<float>(r[1]),
                static_cast<float>(r[2]),
                static_cast<float>(r[3]),
                *reinterpret_cast<const float*>(node + 0x48),
                *reinterpret_cast<const float*>(node + 0x4C),
                *reinterpret_cast<const float*>(node + 0x80),
                node[0x52] != 0,
            };
            node = *reinterpret_cast<unsigned char**>(node + 0x30);
        }
        return !node && RunningDefense::LayoutBounds({chain.data(), count}, result);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void ApplyRunningTextColor(void* armorClassWidget) noexcept {
    if (!armorClassWidget || !ctx) return;
    __try {
        float colorBuf[4]{};
        const auto getColor = reinterpret_cast<GetStatTextColorFn>(ctx->exeBase + RunningDefense::Native::GetStatTextColorRva);
        const float* color = getColor(colorBuf, 1, 5);
        if (color) {
            std::memcpy(reinterpret_cast<unsigned char*>(armorClassWidget) + 0xBC, color, sizeof(float) * 4);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void* FindArmorClassWidget(void* panel) noexcept {
    if (!panel || !ctx) return nullptr;
    __try {
        const auto findChild = reinterpret_cast<WidgetFindChildFn>(ctx->exeBase + RunningDefense::Native::WidgetFindChildRva);
        return findChild(panel, "ArmorClass");
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int __fastcall UnitsGetDefenseHook(void* unit) noexcept {
    const auto original = reinterpret_cast<UnitsGetDefenseFn>(unitsGetDefenseOriginal);
    if (!original) return 0;
    const int baseDefense = original(unit);
    if (!enabled.load() || baseDefense <= 0) return baseDefense;
    if (IsUnitRunningPlayer(unit)) {
        return RunningDefense::ScaleDefenseForRun(baseDefense, defenseLossPercent.load());
    }
    return baseDefense;
}

int __fastcall UnitsGetBlockRateHook(void* unit, int expansion) noexcept {
    const auto original = reinterpret_cast<UnitsGetBlockRateFn>(unitsGetBlockRateOriginal);
    if (!original) return 0;
    const int baseBlock = original(unit, expansion);
    if (!enabled.load() || !overrideBlockPenalty.load() || baseBlock <= 0) return baseBlock;
    if (IsUnitRunningForBlock(unit)) {
        return RunningDefense::ScaleBlockForRun(baseBlock, blockLossPercent.load());
    }
    return baseBlock;
}

void __fastcall UpdateDefenseStatHook(void* panel, void* player) noexcept {
    const auto original = reinterpret_cast<UpdateDefenseStatFn>(updateDefenseStatOriginal);
    if (!original) return;
    original(panel, player);

    if (!enabled.load() || !panel || !player) return;
    void* armorClassWidget = FindArmorClassWidget(panel);
    if (!armorClassWidget) return;

    const bool running = IsUnitRunningPlayer(player);
    const int loss = defenseLossPercent.load();
    if (running && highlightDefenseWhenRunning.load() && loss > 0 && loss < 100) {
        ApplyRunningTextColor(armorClassWidget);
    }

    if (!showRunningIconOnSheet.load()) return;

    RunningDefense::ScreenRect rect{};
    if (!ReadWidgetBounds(reinterpret_cast<unsigned char*>(armorClassWidget), rect)) return;

    const int displayed = UnitsGetDefenseHook(player);
    const auto now = GetTickCount64();
    try {
        std::lock_guard lock(sheetMutex);
        sheetState = {
            .box = rect,
            .widgetAddr = reinterpret_cast<std::uintptr_t>(armorClassWidget),
            .tick = now,
            .displayedDefense = displayed,
            .isRunning = running,
            .valid = true,
        };
    } catch (...) {
    }
}

void* BeginClip(float width, float height) noexcept {
    if (!overlayClipReady.load() || !ctx) return nullptr;
    __try {
        const auto base = ctx->exeBase;
        void* list = reinterpret_cast<void*(__fastcall*)()>(base + RunningDefense::Native::DrawListRva)();
        if (!list) return nullptr;
        const RunningDefense::Point extent{width, height};
        std::uint64_t packed{};
        std::memcpy(&packed, &extent, sizeof(packed));
        reinterpret_cast<void(__fastcall*)(void*, std::uint64_t, std::uint64_t, bool)>(
            base + RunningDefense::Native::PushClipRva)(list, 0, packed, false);
        return list;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

void EndClip(void* list) noexcept {
    if (!list || !ctx) return;
    __try {
        reinterpret_cast<void(__fastcall*)(void*)>(ctx->exeBase + RunningDefense::Native::PopClipRva)(list);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

struct ScopedClip {
    void* list{};
    ~ScopedClip() { EndClip(list); }
};

void DrawLine(
    D2RL::Overlay::CanvasHandle canvas,
    RunningDefense::Point start,
    RunningDefense::Point end,
    D2RL::Overlay::Color color,
    float thickness) noexcept
{
    if (!overlay || !ctx || thickness <= 0.0f) return;
    const D2RL::Overlay::LineRequest req{
        .structSize = D2RL::Overlay::LineRequestSize,
        .flags = 0,
        .canvas = canvas,
        .start = {start.x, start.y},
        .end = {end.x, end.y},
        .color = color,
        .thickness = thickness,
        .reserved = 0,
    };
    overlay->drawLine(ctx, &req);
}

void DrawFilledDisc(
    D2RL::Overlay::CanvasHandle canvas,
    RunningDefense::Point center,
    float radius,
    D2RL::Overlay::Color color) noexcept
{
    if (!overlay || !ctx || radius <= 0.0f) return;
    const D2RL::Overlay::FilledRectangleRequest req{
        .structSize = D2RL::Overlay::FilledRectangleRequestSize,
        .flags = 0,
        .canvas = canvas,
        .rect = {center.x - radius, center.y - radius, center.x + radius, center.y + radius},
        .color = color,
        .rounding = radius,
        .reserved = 0,
    };
    overlay->drawFilledRectangle(ctx, &req);
}

void DrawDiscOutline(
    D2RL::Overlay::CanvasHandle canvas,
    RunningDefense::Point center,
    float radius,
    D2RL::Overlay::Color color,
    float thickness) noexcept
{
    if (!overlay || !ctx || radius <= 0.0f || thickness <= 0.0f) return;
    const D2RL::Overlay::RectangleRequest req{
        .structSize = D2RL::Overlay::RectangleRequestSize,
        .flags = 0,
        .canvas = canvas,
        .rect = {center.x - radius, center.y - radius, center.x + radius, center.y + radius},
        .color = color,
        .rounding = radius,
        .thickness = thickness,
    };
    overlay->drawRectangle(ctx, &req);
}

void DrawRunnerFigure(
    D2RL::Overlay::CanvasHandle canvas,
    RunningDefense::Point c,
    float s,
    D2RL::Overlay::Color color,
    float stroke,
    bool drawMotionTicks) noexcept
{
    // Head
    DrawFilledDisc(canvas, {c.x + s * 0.22f, c.y - s * 0.56f}, (std::max)(1.8f, s * 0.19f), color);

    // Leaning torso
    DrawLine(
        canvas,
        {c.x + s * 0.14f, c.y - s * 0.34f},
        {c.x - s * 0.10f, c.y + s * 0.12f},
        color,
        stroke);

    // Front arm (bent forward/up)
    DrawLine(
        canvas,
        {c.x + s * 0.08f, c.y - s * 0.25f},
        {c.x + s * 0.34f, c.y - s * 0.08f},
        color,
        stroke * 0.9f);
    DrawLine(
        canvas,
        {c.x + s * 0.34f, c.y - s * 0.08f},
        {c.x + s * 0.48f, c.y - s * 0.26f},
        color,
        stroke * 0.9f);

    // Back arm (bent back)
    DrawLine(
        canvas,
        {c.x + s * 0.08f, c.y - s * 0.25f},
        {c.x - s * 0.24f, c.y - s * 0.18f},
        color,
        stroke * 0.9f);
    DrawLine(
        canvas,
        {c.x - s * 0.24f, c.y - s * 0.18f},
        {c.x - s * 0.38f, c.y + s * 0.00f},
        color,
        stroke * 0.9f);

    // Front leg (striding forward)
    DrawLine(
        canvas,
        {c.x - s * 0.10f, c.y + s * 0.12f},
        {c.x + s * 0.24f, c.y + s * 0.28f},
        color,
        stroke);
    DrawLine(
        canvas,
        {c.x + s * 0.24f, c.y + s * 0.28f},
        {c.x + s * 0.16f, c.y + s * 0.66f},
        color,
        stroke);

    // Back leg (pushing off)
    DrawLine(
        canvas,
        {c.x - s * 0.10f, c.y + s * 0.12f},
        {c.x - s * 0.26f, c.y + s * 0.40f},
        color,
        stroke);
    DrawLine(
        canvas,
        {c.x - s * 0.26f, c.y + s * 0.40f},
        {c.x - s * 0.50f, c.y + s * 0.56f},
        color,
        stroke);

    if (drawMotionTicks) {
        const float tickStroke = (std::max)(1.0f, stroke * 0.72f);
        DrawLine(
            canvas,
            {c.x - s * 0.68f, c.y - s * 0.32f},
            {c.x - s * 0.40f, c.y - s * 0.32f},
            color,
            tickStroke);
        DrawLine(
            canvas,
            {c.x - s * 0.76f, c.y + s * 0.08f},
            {c.x - s * 0.48f, c.y + s * 0.08f},
            color,
            tickStroke);
    }
}

void DrawRunningIcon(
    D2RL::Overlay::CanvasHandle canvas,
    RunningDefense::Point center,
    float scale,
    bool isRunning) noexcept
{
    const float stroke = std::clamp(scale * 0.19f, 1.4f, 3.2f);
    const D2RL::Overlay::Color shadow{0.04f, 0.03f, 0.02f, 0.90f};
    // Warm D2R gold when standing; bright active amber-orange when actively running.
    const D2RL::Overlay::Color fg = isRunning
        ? D2RL::Overlay::Color{1.00f, 0.68f, 0.22f, 0.98f}
        : D2RL::Overlay::Color{0.82f, 0.73f, 0.50f, 0.88f};

    // Subtle dark backing disc so the icon stays legible against any stone texture or digit edge.
    DrawFilledDisc(canvas, center, scale * 0.92f, {0.05f, 0.04f, 0.03f, 0.55f});
    if (isRunning) {
        DrawDiscOutline(canvas, center, scale * 0.92f, {1.00f, 0.62f, 0.18f, 0.55f}, 1.0f);
    }

    DrawRunnerFigure(canvas, {center.x + 1.0f, center.y + 1.0f}, scale, shadow, stroke + 0.8f, isRunning);
    DrawRunnerFigure(canvas, center, scale, fg, stroke, isRunning);
}

void __cdecl Frame(const D2RL::PluginContext*, const D2RL::Overlay::Frame* frame, void*) noexcept {
    if (!enabled.load() || !showRunningIconOnSheet.load() || !frame || frame->structSize < sizeof(*frame)) return;
    try {
        SheetSnapshot snapshot{};
        {
            std::lock_guard lock(sheetMutex);
            snapshot = sheetState;
        }
        const auto now = GetTickCount64();
        if (!snapshot.valid || !snapshot.widgetAddr || now < snapshot.tick || (now - snapshot.tick) > 120) {
            return;
        }

        RunningDefense::ScreenRect liveRect{};
        if (!ReadWidgetBounds(reinterpret_cast<unsigned char*>(snapshot.widgetAddr), liveRect)) {
            return;
        }

        RunningDefense::Point iconCenter{};
        float iconScale{};
        if (!RunningDefense::ComputeRunningIconPlacement(liveRect, snapshot.displayedDefense, iconCenter, iconScale)) {
            return;
        }

        const ScopedClip clip{BeginClip(frame->screenWidth, frame->screenHeight)};
        DrawRunningIcon(frame->canvas, iconCenter, iconScale, snapshot.isRunning);
    } catch (...) {
    }
}

D2RL::ConsoleCommandResult __cdecl Command(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*) noexcept
{
    if (!command || command->contextSize < D2RL::ConsoleCommandContextSize || command->plugin != ctx ||
        command->argsLength > 256 || (!command->args && command->argsLength)) {
        return D2RL::ConsoleCommandResult::Failed;
    }

    const std::string_view args = RunningDefense::Detail::Trim({command->args ? command->args : "", command->argsLength});
    if (!args.empty() && args != "status") {
        ctx->WriteConsoleMessage(
            "Usage: /runningdefense [status] (settings are configured in running-defense.toml).",
            D2RL::ConsoleMessageKind::Warning);
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }

    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "Running Defense: %s | native=%s | defense_loss=%d%% (keep %d%%) | block_override=%s (%d%% loss) | sheet_icon=%s",
        enabled.load() ? "enabled" : "disabled",
        admitted.load() ? "admitted" : "inert",
        defenseLossPercent.load(),
        100 - defenseLossPercent.load(),
        overrideBlockPenalty.load() ? "on" : "off",
        blockLossPercent.load(),
        showRunningIconOnSheet.load() ? "on" : "off");
    ctx->WriteConsoleMessage(
        message,
        admitted.load() ? D2RL::ConsoleMessageKind::Output : D2RL::ConsoleMessageKind::Warning);
    return admitted.load() ? D2RL::ConsoleCommandResult::Handled : D2RL::ConsoleCommandResult::Failed;
}

bool AdmitBaseGuards() noexcept {
    using namespace RunningDefense::Native;
    if (!Hash(reinterpret_cast<HMODULE>(ctx->exeBase), ExeHash) ||
        !Hash(GetModuleHandleW(L"D2RCore.dll"), CoreHash)) {
        return false;
    }
    return ctx->CheckExpectedBytes(ResolveHitRunCheckRva, ResolveHitRunCheckBytes, sizeof(ResolveHitRunCheckBytes)) &&
           ctx->CheckExpectedBytes(UnitsGetDefenseRva, UnitsGetDefenseBytes, sizeof(UnitsGetDefenseBytes)) &&
           ctx->CheckExpectedBytes(UnitsGetAnimModeRva, UnitsGetAnimModeBytes, sizeof(UnitsGetAnimModeBytes)) &&
           ctx->CheckExpectedBytes(UpdateDefenseStatRva, UpdateDefenseStatBytes, sizeof(UpdateDefenseStatBytes)) &&
           ctx->CheckExpectedBytes(UpdateDefenseRunBranchRva, UpdateDefenseRunBranchBytes, sizeof(UpdateDefenseRunBranchBytes)) &&
           ctx->CheckExpectedBytes(WidgetFindChildRva, WidgetFindChildBytes, sizeof(WidgetFindChildBytes)) &&
           ctx->CheckExpectedBytes(GetStatTextColorRva, GetStatTextColorBytes, sizeof(GetStatTextColorBytes));
}

void RestorePatches() noexcept {
    if (!ctx) return;
    using namespace RunningDefense::Native;
    if (autoHitPatched.exchange(false)) {
        (void)ctx->PatchBytes(
            ResolveHitAutoHitAssignRva,
            Nop3Bytes,
            sizeof(Nop3Bytes),
            ResolveHitAutoHitAssignBytes,
            sizeof(ResolveHitAutoHitAssignBytes));
    }
    if (sheetBranchPatched.exchange(false)) {
        (void)ctx->PatchBytes(
            UpdateDefenseRunJneOpcodeRva,
            &JmpOpcodeByte,
            1,
            &JneOpcodeByte,
            1);
    }
    if (blockDivPatched.exchange(false)) {
        (void)ctx->PatchBytes(
            CheckBlockRunDiv3Rva,
            Nop14Bytes,
            sizeof(Nop14Bytes),
            CheckBlockRunDiv3Bytes,
            sizeof(CheckBlockRunDiv3Bytes));
    }
}

void Stop() noexcept {
    enabled.store(false);
    admitted.store(false);
    RestorePatches();
    if (ctx && overlay && frameHandle) {
        overlay->unregisterFrameCallback(ctx, frameHandle);
    }
    frameHandle = 0;
    {
        std::lock_guard lock(sheetMutex);
        sheetState = {};
    }
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    static constexpr D2RL::PluginInfo info{
        .infoSize = D2RL::PluginInfoSize,
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .id = "running-defense",
        .name = "Running Defense",
        .version = "1.3.1+rev.1",
        .author = "PyreFly",
        .description = "Adjustable Defense and Block penalty while running with Character Sheet running indicator.",
        .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
    };
    return &info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context || !context->GetApi()) return false;
    ctx = context;

    try {
        char configBuf[RunningDefense::ConfigCapacity]{};
        std::uint32_t required{};
        unsigned rejected{};
        if (!ctx->ReadConfig(configBuf, sizeof(configBuf) - 1, &required)) return false;

        const auto settings = RunningDefense::ParseConfig(configBuf, rejected);
        if (rejected) {
            ctx->LogWarn("Running Defense: invalid configuration values ignored; defaults retained.");
        }

        enabled.store(settings.enabled);
        defenseLossPercent.store(settings.defenseLossPercent);
        overrideBlockPenalty.store(settings.overrideBlockPenalty);
        blockLossPercent.store(settings.blockLossPercent);
        showRunningIconOnSheet.store(settings.showRunningIconOnSheet);
        highlightDefenseWhenRunning.store(settings.highlightDefenseWhenRunning);

        auto command = D2RL::MakeConsoleCommand(
            "runningdefense",
            Command,
            "Inspect Running Defense status and active running penalty settings.");
        command.usage = "/runningdefense [status]";
        command.scope = D2RL::ConsoleCommandScope::LocalOnly;
        if (!ctx->RegisterConsoleCommand(command)) return false;

        if (!AdmitBaseGuards()) {
            ctx->LogWarn("Running Defense: unsupported executable/Core or byte guards; plugin remains inert.");
            return true;
        }

        using namespace RunningDefense::Native;

        // Install UNITS_GetDefense inline hook.
        if (!ctx->InstallInlineHook(
                UnitsGetDefenseRva,
                UnitsGetDefenseBytes,
                sizeof(UnitsGetDefenseBytes),
                reinterpret_cast<void*>(UnitsGetDefenseHook),
                &unitsGetDefenseOriginal) ||
            !unitsGetDefenseOriginal) {
            ctx->LogWarn("Running Defense: UNITS_GetDefense hook refused; plugin remains inert.");
            return true;
        }

        // Install CharacterStatsPanel::UpdateDefenseStat inline hook.
        if (!ctx->InstallInlineHook(
                UpdateDefenseStatRva,
                UpdateDefenseStatBytes,
                sizeof(UpdateDefenseStatBytes),
                reinterpret_cast<void*>(UpdateDefenseStatHook),
                &updateDefenseStatOriginal) ||
            !updateDefenseStatOriginal) {
            ctx->LogWarn("Running Defense: UpdateDefenseStat hook refused; plugin remains inert.");
            return true;
        }

        // When enabled and defense_loss_percent < 100, bypass the melee running auto-hit and "---" Character Sheet branch.
        if (settings.enabled && settings.defenseLossPercent < 100) {
            if (!ctx->PatchNop(
                    ResolveHitAutoHitAssignRva,
                    ResolveHitAutoHitAssignBytes,
                    sizeof(ResolveHitAutoHitAssignBytes),
                    sizeof(ResolveHitAutoHitAssignBytes))) {
                ctx->LogWarn("Running Defense: melee running auto-hit patch refused.");
                Stop();
                return true;
            }
            autoHitPatched.store(true);

            if (!ctx->PatchBytes(
                    UpdateDefenseRunJneOpcodeRva,
                    &JneOpcodeByte,
                    1,
                    &JmpOpcodeByte,
                    1)) {
                ctx->LogWarn("Running Defense: Character Sheet running branch patch refused.");
                Stop();
                return true;
            }
            sheetBranchPatched.store(true);
        }

        // Optional running Shield Block penalty override.
        if (settings.enabled && settings.overrideBlockPenalty) {
            if (ctx->CheckExpectedBytes(UnitsGetBlockRateRva, UnitsGetBlockRateBytes, sizeof(UnitsGetBlockRateBytes)) &&
                ctx->CheckExpectedBytes(UnitsIsMovingRva, UnitsIsMovingBytes, sizeof(UnitsIsMovingBytes)) &&
                ctx->CheckExpectedBytes(CheckBlockRunDiv3Rva, CheckBlockRunDiv3Bytes, sizeof(CheckBlockRunDiv3Bytes)) &&
                ctx->InstallInlineHook(
                    UnitsGetBlockRateRva,
                    UnitsGetBlockRateBytes,
                    sizeof(UnitsGetBlockRateBytes),
                    reinterpret_cast<void*>(UnitsGetBlockRateHook),
                    &unitsGetBlockRateOriginal) &&
                unitsGetBlockRateOriginal &&
                ctx->PatchNop(
                    CheckBlockRunDiv3Rva,
                    CheckBlockRunDiv3Bytes,
                    sizeof(CheckBlockRunDiv3Bytes),
                    sizeof(CheckBlockRunDiv3Bytes))) {
                blockDivPatched.store(true);
                ctx->LogInfo("Running Defense: optional running Shield Block penalty override enabled.");
            } else {
                ctx->LogWarn("Running Defense: running Shield Block guards/patch refused; vanilla block penalty retained.");
            }
        }

        // Register Character Sheet running icon overlay callback.
        if (ctx->QueryService(&overlay) == D2RL::ServiceQueryResult::Success &&
            D2RL::HasOverlayServiceField(overlay, D2RL::OverlayServiceRequiredSize)) {
            overlayClipReady.store(
                ctx->CheckExpectedBytes(DrawListRva, DrawListBytes, sizeof(DrawListBytes)) &&
                ctx->CheckExpectedBytes(PushClipRva, PushClipBytes, sizeof(PushClipBytes)) &&
                ctx->CheckExpectedBytes(PopClipRva, PopClipBytes, sizeof(PopClipBytes)));
            const D2RL::Overlay::CallbackRegistration callback{
                sizeof(callback),
                0,
                D2RL::Overlay::Phase::AfterGameUi,
                0,
                Frame,
                nullptr,
            };
            if (overlay->registerFrameCallback(ctx, &callback, &frameHandle) != D2RL::Overlay::Result::Success) {
                ctx->LogWarn("Running Defense: overlay frame callback registration failed; Character Sheet running icon disabled.");
            }
        } else {
            ctx->LogWarn("Running Defense: OverlayService unavailable; Character Sheet running icon disabled.");
        }

        admitted.store(true);
        D2RL::LogInfoF(
            ctx,
            "Running Defense 1.3.1+rev.1: admitted (enabled=%s, defense_loss_percent=%d%%, keep=%d%%, sheet_icon=%s).",
            settings.enabled ? "true" : "false",
            settings.defenseLossPercent,
            100 - settings.defenseLossPercent,
            settings.showRunningIconOnSheet ? "true" : "false");
        return true;
    } catch (...) {
        Stop();
        return false;
    }
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Stop();
}
