#pragma once
// Controller QOL visibility service provider ("unhoarder-ground-visibility" &
// "unhoarder-label-visibility" v1) via D2RL::PluginCommunicationService.

#include "unit_reader.h"
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/plugin_communication.h>
#include <cstdint>
#include <type_traits>

namespace PyreLootFilter::QolInterop {

namespace GroundVisibility {
inline constexpr char Name[] = "unhoarder-ground-visibility";
inline constexpr std::uint32_t Version = 1;
enum class Decision : std::uint32_t { Unknown = 0, Visible = 1, Hidden = 2 };

struct Service {
    std::uint32_t size;
    std::uint32_t version;
    Decision(__cdecl* query)(const void* unit, std::uint32_t guid, std::uint32_t code) noexcept;
};
static_assert(sizeof(Service) == 16 && std::is_standard_layout_v<Service>);
} // namespace GroundVisibility

namespace LabelVisibility {
inline constexpr char Name[] = "unhoarder-label-visibility";
inline constexpr std::uint32_t Version = 1;
enum class Decision : std::uint32_t { RequireQuery = 0, CannotHide = 1 };

struct Service {
    std::uint32_t size;
    std::uint32_t version;
    Decision(__cdecl* query)(const void* unit, std::uint32_t guid) noexcept;
};
static_assert(sizeof(Service) == 16 && std::is_standard_layout_v<Service>);
} // namespace LabelVisibility

[[nodiscard]] inline LabelVisibility::Decision EvaluateLabelPreflight(
    const void* unit, std::uint32_t guid, bool filterEnabled,
    const Schema::FilterRuleTable* table,
    const Reader::NativeReaders& readers,
    Cache::UnitEvaluationCache& cache) noexcept {
    if (!filterEnabled || !table || table->hiddenRules == 0) {
        return LabelVisibility::Decision::CannotHide;
    }
    if (!unit || !guid) {
        return LabelVisibility::Decision::RequireQuery;
    }

    Reader::UnitHeader before{};
    if (!Reader::ReadUnitHeader(unit, before) || before.unitId != guid ||
        !Reader::AllowsMode(before.mode, Reader::Purpose::VerifiedLabel)) {
        return LabelVisibility::Decision::RequireQuery;
    }

    const auto nowMs = GetTickCount64();

    // Fast path 1: check UnitEvaluationCache first
    Schema::ItemFacts cachedFacts{};
    Schema::EvaluatedDecision cachedDecision{};
    if (cache.LookupByUnitId(guid, table->generation, nowMs, table->usesSellPrice, cachedFacts,
                             cachedDecision)) {
        Reader::UnitHeader after{};
        if (Reader::ReadUnitHeader(unit, after) && after == before && cachedDecision.show) {
            return LabelVisibility::Decision::CannotHide;
        }
        return LabelVisibility::Decision::RequireQuery;
    }

    // Fast path 2: check code-level CanEverHideCode
    const std::uint32_t packed = readers.ReadItemCode(const_cast<void*>(unit));
    if (packed != 0 && !Schema::CanEverHideCode(table, packed)) {
        Reader::UnitHeader after{};
        if (Reader::ReadUnitHeader(unit, after) && after == before) {
            return LabelVisibility::Decision::CannotHide;
        }
    }
    return LabelVisibility::Decision::RequireQuery;
}

[[nodiscard]] inline GroundVisibility::Decision EvaluateGroundVisibility(
    const void* unit, std::uint32_t guid, std::uint32_t code, bool filterEnabled,
    const Schema::FilterRuleTable* table,
    const Reader::NativeReaders& readers,
    Cache::UnitEvaluationCache& cache) noexcept {
    if (!filterEnabled || !table || table->hiddenRules == 0) {
        return GroundVisibility::Decision::Visible;
    }
    if (!unit || !guid) {
        return GroundVisibility::Decision::Unknown;
    }

    Reader::UnitHeader before{};
    if (!Reader::ReadUnitHeader(unit, before) || before.unitId != guid ||
        !Reader::AllowsMode(before.mode, Reader::Purpose::StrictGround)) {
        return GroundVisibility::Decision::Unknown;
    }

    const std::uint32_t packedHint = Tables::CanonicalItemCode(code);
    Schema::ItemFacts facts{};
    Schema::EvaluatedDecision decision{};
    (void)readers.EvaluateUnit(const_cast<void*>(unit), packedHint, table, cache,
                               Reader::Purpose::StrictGround, facts, decision);
    if (!Tables::PrintableItemCode(facts.code)) {
        return GroundVisibility::Decision::Unknown;
    }

    Reader::UnitHeader after{};
    if (!Reader::ReadUnitHeader(unit, after) || after != before) {
        return GroundVisibility::Decision::Unknown;
    }
    return decision.show ? GroundVisibility::Decision::Visible
                         : GroundVisibility::Decision::Hidden;
}

inline bool PublishVisibilityServices(
    const D2RL::PluginContext* context,
    const GroundVisibility::Service& groundService,
    const LabelVisibility::Service& labelService) noexcept {
    if (!context) return false;
    const D2RL::PluginCommunicationService* comm = nullptr;
    if (context->QueryService(&comm) != D2RL::ServiceQueryResult::Success ||
        !D2RL::HasPluginCommunicationServiceField(
            comm, D2RL::PluginCommunicationServiceRequiredSize) ||
        comm->publishService == nullptr) {
        return false;
    }

    const D2RL::PluginCommunication::PublishServiceRequest reqGround{
        .structSize = D2RL::PluginCommunication::PublishServiceRequestSize,
        .flags = 0,
        .name = GroundVisibility::Name,
        .serviceVersion = GroundVisibility::Version,
        .tableSize = sizeof(GroundVisibility::Service),
        .table = &groundService,
    };
    const D2RL::PluginCommunication::PublishServiceRequest reqLabel{
        .structSize = D2RL::PluginCommunication::PublishServiceRequestSize,
        .flags = 0,
        .name = LabelVisibility::Name,
        .serviceVersion = LabelVisibility::Version,
        .tableSize = sizeof(LabelVisibility::Service),
        .table = &labelService,
    };

    const auto r1 = comm->publishService(context, &reqGround);
    const auto r2 = comm->publishService(context, &reqLabel);
    return (r1 == D2RL::PluginCommunication::Result::Success ||
            r1 == D2RL::PluginCommunication::Result::AlreadyPublished) &&
           (r2 == D2RL::PluginCommunication::Result::Success ||
            r2 == D2RL::PluginCommunication::Result::AlreadyPublished);
}

} // namespace PyreLootFilter::QolInterop
