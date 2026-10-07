#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
namespace QolClientLoot {
struct Candidate {
    uint32_t guid{}, code{}, quality{};
    int distance{}, rank{};
};
inline bool SameItem(const Candidate& a, const Candidate& b) {
    return a.guid && a.guid==b.guid && a.code==b.code && a.quality==b.quality;
}
using Slots = std::array<Candidate,7>;
struct Request { Candidate item{}; uint64_t epoch{}, queuedAt{}; };
inline bool CurrentRequest(const Request& request,uint64_t epoch,uint64_t now,bool held) {
    return held && request.item.guid && request.epoch==epoch && now-request.queuedAt<=500;
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
