#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
namespace QolClientLoot {
struct VisibilityProof {
    uint64_t generation{},observed{};
    uint32_t code{};
    bool Current(uint64_t session,uint64_t now) const noexcept {
        return generation && generation==session && now>=observed && now-observed<=250;
    }
};
inline bool IsGoldCode(uint32_t code) noexcept {return code==0x20646c67u || code==0x00646c67u;}
// The SDK's UI lifecycle supplies identity, never a borrowed native player.
struct LocalSession {
    uint64_t generation{};
    uint32_t playerId{UINT32_MAX};
    bool joined{};
    void Join(uint64_t value) noexcept {generation=value;playerId=UINT32_MAX;joined=value!=0;}
    void Leave() noexcept {joined=false;playerId=UINT32_MAX;}
    bool Bind(uint64_t value,uint32_t id) noexcept {
        if(!joined || !value || value!=generation || id==UINT32_MAX)return false;
        playerId=id;return true;
    }
    bool Matches(uint32_t type,uint32_t id) const noexcept {
        return joined && generation && playerId!=UINT32_MAX && type==0 && id==playerId;
    }
};
struct NativePickupPermit {
    uint32_t guid{};
    uint64_t generation{},issued{};
    uint32_t lifetime{1000};
    bool Current(const LocalSession& session,uint32_t actorId,uint32_t itemId,uint64_t now) const noexcept {
        return guid && guid==itemId && generation==session.generation && session.Matches(0,actorId) &&
            now>=issued && now-issued<=lifetime;
    }
};
// Native item-click branch at 0x101A0F uses the same inclusive reach of four.
inline bool NeedsApproach(int distance) noexcept {return distance>4;}
// Nearby radius selects the hold snapshot; admitted native travel has no QOL
// distance ceiling when submitting an already assigned, still targetable item.
inline uint32_t SubmissionRadius(uint32_t nearbyRadius,bool approachReady) noexcept {
    return approachReady?UINT32_MAX:nearbyRadius;
}
struct RepeatGate {
    uint32_t guid{};
    uint64_t generation{},issued{};
    bool Blocks(uint32_t item,uint64_t session,uint64_t now) const noexcept {
        return guid && guid==item && generation==session && now>=issued && now-issued<250;
    }
};
struct Candidate {
    uint32_t guid{}, code{}, quality{};
    int distance{}, rank{};
};
inline bool SameItem(const Candidate& a, const Candidate& b) {
    return a.guid && a.guid==b.guid && a.code==b.code && a.quality==b.quality;
}
using Slots = std::array<Candidate,7>;
// A hold owns its original identities, including temporarily hidden labels and
// removed items. Submission separately revalidates visibility/native identity.
inline Slots HoldSnapshot(const Slots& original,const Slots& observed,bool firstRefresh) {
    return firstRefresh?observed:original;
}
inline Slots PruneHiddenSlots(const Slots& original,const std::vector<uint32_t>& hidden) {
    auto result=original;
    for(auto& slot:result) {
        if(std::find(hidden.begin(),hidden.end(),slot.guid)!=hidden.end())slot={};
    }
    return result; // Leave holes while held; never redirect an existing button.
}
struct Request { Candidate item{}; uint64_t epoch{}, queuedAt{}; };
inline bool CurrentRequest(const Request& request,uint64_t epoch,uint64_t now,bool held) {
    return held && request.item.guid && request.epoch==epoch && now>=request.queuedAt && now-request.queuedAt<=500;
}
// Existing assignments survive priority/distance changes. A removed item leaves
// a hole; never redirect an already queued request to the replacement in that hole.
inline Slots Assign(Slots slots, std::vector<Candidate> candidates) {
    std::sort(candidates.begin(),candidates.end(),[](auto a,auto b) {
        if(a.rank!=b.rank)return a.rank>b.rank;
        if(a.distance!=b.distance)return a.distance<b.distance;
        return a.guid<b.guid;
    });
    for(auto& slot:slots) {
        auto it=std::find_if(candidates.begin(),candidates.end(),[&](auto c){return SameItem(slot,c);});
        if(it==candidates.end())slot={};
        else {slot=*it;candidates.erase(it);}
    }
    for(auto c:candidates) {
        auto it=std::find_if(slots.begin(),slots.end(),[](auto s){return !s.guid;});
        if(it==slots.end())break;
        *it=c;
    }
    return slots;
}
inline bool Eligible(uint32_t type,uint32_t mode,int distance,uint32_t radius,int collision) {
    return type==4 && mode==3 && distance>=0 && static_cast<uint32_t>(distance)<=radius && collision==0;
}
}
