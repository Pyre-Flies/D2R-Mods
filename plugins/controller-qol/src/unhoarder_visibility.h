#pragma once
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/plugin_communication.h>
#include "unhoarder_visibility_service.h"
#include "ground_label_visibility_service.h"
#include "loot_performance.h"
namespace QolLootVisibility {
inline bool CannotHideLabel(const D2RL::PluginContext* context,
    const void* unit,uint32_t guid) noexcept {
    QolPerf::Scope timing(QolPerf::Counters[QolPerf::LabelVisibility]);
    if(!context || !unit || !guid)return false;
    const D2RL::PluginCommunicationService* communication{};
    if(context->QueryService(&communication)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasPluginCommunicationServiceField(communication,D2RL::PluginCommunicationServiceRequiredSize))return false;
    D2RL::PluginCommunication::ServiceLease<UnhoarderLabelVisibility::Service> lease;
    if(D2RL::PluginCommunication::Acquire(context,communication,"unhoarder",
        UnhoarderLabelVisibility::Name,UnhoarderLabelVisibility::Version,
        sizeof(UnhoarderLabelVisibility::Service),&lease)!=D2RL::PluginCommunication::Result::Success ||
       !lease || !UnhoarderLabelVisibility::Valid(lease.Get()))return false;
    return lease->query(unit,guid)==UnhoarderLabelVisibility::Decision::CannotHide;
}
// Each call holds a broker lease through the callback. Never retain native
// unit pointers or service tables after the call; late provider load is valid.
inline bool Hidden(const D2RL::PluginContext* context,const void* unit,
    uint32_t guid,uint32_t code) noexcept {
    QolPerf::Scope timing(QolPerf::Counters[QolPerf::Visibility]);
    if(!context || !unit || !guid)return false;
    const D2RL::PluginCommunicationService* communication{};
    if(context->QueryService(&communication)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasPluginCommunicationServiceField(communication,D2RL::PluginCommunicationServiceRequiredSize))return false;
    D2RL::PluginCommunication::ServiceLease<UnhoarderVisibility::Service> lease;
    if(D2RL::PluginCommunication::Acquire(context,communication,"unhoarder",
        UnhoarderVisibility::Name,UnhoarderVisibility::Version,
        sizeof(UnhoarderVisibility::Service),&lease)!=D2RL::PluginCommunication::Result::Success ||
       !lease || !UnhoarderVisibility::Valid(lease.Get()))return false;
    // Only a positive Hidden decision excludes a shortcut candidate. Unknown
    // identity/rules, missing providers and unavailable services fail open.
    return lease->query(unit,guid,code)==UnhoarderVisibility::Decision::Hidden;
}
}
