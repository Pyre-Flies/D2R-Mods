#pragma once
#include "aim_policy.h"
#include <array>

namespace Aim {
enum class ObservationMode { None, MeteorPreviewCircle, MeteorCircle };
inline bool UseCoordinateTarget(int requestedSkill,int activeSkill) noexcept {
    return Supported(requestedSkill) && requestedSkill==activeSkill;
}
inline ObservationMode MeteorObservation(int activeSkill,int previewSkill) noexcept {
    if(SnapSkill(activeSkill)) return ObservationMode::MeteorCircle;
    if(activeSkill==-1 && Supported(previewSkill)) return ObservationMode::MeteorPreviewCircle;
    return ObservationMode::None;
}
// Only the UI's idle ground-skill preview may show the next spell's snap.
inline bool GroundSnapPreview(int activeSkill,int previewSkill,bool allowPreview) noexcept {
    return allowPreview && activeSkill==-1 && Supported(previewSkill) && !SnapSkill(previewSkill);
}
struct SnapCandidate {
    bool valid{};
    unsigned playerId{}, id{};
    Point position{};
    std::uint64_t tick{};
};
struct SnapDiagnostics {
    unsigned fresh{}, expired{}, outsideCircle{}, outsideRange{};
    unsigned rejectedId{};
    std::uint64_t rejectedTick{};
};
// Copied observations only: no retained native unit/controller pointers.
struct SnapBook {
    static constexpr std::uint64_t Lifetime=150;
    static constexpr float SwitchAdvantage=1.5f;
    std::array<SnapCandidate,128> candidates{};
    SnapCandidate retained{};
    SnapCandidate lastRejected{};
    void Observe(unsigned player,unsigned id,Point position,std::uint64_t tick,bool eligible) noexcept {
        auto* slot=&candidates[0];
        for(auto& candidate:candidates) {
            if(candidate.valid && candidate.playerId==player && candidate.id==id) { slot=&candidate; break; }
            if(!candidate.valid || candidate.tick<slot->tick) slot=&candidate;
        }
        // A rejected candidate invalidates only its own prior observation.
        if(!eligible || !ValidWorld(position)) {
            lastRejected={true,player,id,position,tick};
            if(slot->valid && slot->playerId==player && slot->id==id) slot->valid=false;
            if(retained.valid && retained.playerId==player && retained.id==id) retained={};
            return;
        }
        *slot={true,player,id,position,tick};
    }
    SnapCandidate Choose(unsigned player,Point center,Point playerPosition,std::uint64_t now,SnapDiagnostics* diagnostics=nullptr,float radius=SnapRadius,float switchAdvantage=SwitchAdvantage,bool retainOutsideCircle=false) noexcept {
        SnapDiagnostics reasons{};
        if(lastRejected.valid && lastRejected.playerId==player) {
            reasons.rejectedId=lastRejected.id; reasons.rejectedTick=lastRejected.tick;
        }
        SnapCandidate nearest{}, held{};
        float nearestDistance=radius+1, heldDistance=radius+1;
        for(const auto& candidate:candidates) {
            if(!candidate.valid || candidate.playerId!=player) continue;
            if(!Fresh(now,candidate.tick,Lifetime)) { ++reasons.expired; continue; }
            ++reasons.fresh;
            const bool locked=retainOutsideCircle && retained.valid && retained.playerId==player && candidate.id==retained.id;
            if(!locked && Score(center,candidate.position,radius)<0) { ++reasons.outsideCircle; continue; }
            if(DistanceSquared(playerPosition,candidate.position)>MaximumDistance*MaximumDistance) { ++reasons.outsideRange; continue; }
            const float distance=std::sqrt(DistanceSquared(center,candidate.position));
            if(distance<nearestDistance || (distance==nearestDistance && candidate.id<nearest.id)) {
                nearest=candidate; nearestDistance=distance;
            }
            if(retained.valid && retained.playerId==player && candidate.id==retained.id) {
                held=candidate; heldDistance=distance;
            }
        }
        if(held.valid && (retainOutsideCircle || !nearest.valid || nearest.id==held.id || nearestDistance+switchAdvantage>=heldDistance)) nearest=held;
        retained=nearest;
        if(diagnostics) *diagnostics=reasons;
        return nearest;
    }
};
}
