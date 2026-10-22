#pragma once
#include "perf_stats.h"
#include "current_page_query.h"
#include <D2RLPlugin/api.h>
namespace QolPerf {
enum Metric : std::size_t {Compatibility,Label,Refresh,Visibility,LabelVisibility,LabelFastPath,Count};
inline std::array<LootPerf::Counter,Count> Counters{};
inline LootPerf::ReportWindow<Count> Window;
using Scope=LootPerf::Scope;
inline constexpr std::array<LootPerf::Description,Count> Names{{
    {"item-code-compatibility","items"},{"completed-label","items"},
    {"held-refresh","candidateReads"},{"visibility-service","items"},
    {"label-visibility-preflight","items"},{"label-no-hide-query-skipped","items"}}};
inline void Report(const D2RL::PluginContext* context) noexcept {
    if(!context)return;
    const auto emit=[&](const char* line) noexcept {
        context->LogInfo(line);
        context->WriteConsoleMessage(line,D2RL::ConsoleMessageKind::Output);
    };
    Window.Print("QOL_PERF","1.3.1+rev.89",Names,Counters,emit);
    CurrentPageQuery::Report("qol",emit);
}
}
