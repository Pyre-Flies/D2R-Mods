#pragma once
#include "identify_action.h"
namespace QolClientTransfer {
using Info=D2RL::Items::ItemInfo;
using C=D2RL::Items::ItemContainer;
enum class StashRoute { Refuse, Ordinary, Advanced, Cube };
inline bool AdvancedTab(uint32_t tab) noexcept {return tab>=2 && tab<=4;}
inline bool EmbeddedCube(bool stashOpen,uint32_t tab,C source) noexcept {
    return stashOpen && AdvancedTab(tab) && source==C::Cube;
}
inline bool CubeView(bool cubeOpen,bool stashOpen,uint32_t tab) noexcept {
    return cubeOpen || (stashOpen && AdvancedTab(tab));
}
inline StashRoute ChooseStashRoute(uint32_t tab,C source,bool eligible) noexcept {
    if(tab>4)return StashRoute::Refuse;
    if(source==C::Cube && AdvancedTab(tab))return eligible?StashRoute::Advanced:StashRoute::Refuse;
    if(source==C::Inventory) {
        if(eligible)return StashRoute::Advanced;
        return tab<2?StashRoute::Ordinary:StashRoute::Cube;
    }
    return tab<2?StashRoute::Ordinary:StashRoute::Refuse;
}
// One queued request, one possible submission. Cancellation invalidates callbacks
// even when the next session reuses the same player and item identifiers.
struct RequestFence {
    uint64_t generation{},started{};unsigned polls{};bool pending{},submitted{};
    void Begin(uint64_t now) noexcept {++generation;started=now;polls=0;pending=true;submitted=false;}
    void Cancel() noexcept {++generation;pending=false;}
    bool Current(uint64_t token) const noexcept {return pending && token==generation;}
    bool Poll(uint64_t now) noexcept {return pending && now-started<=(submitted?2500u:750u) && ++polls<=480;}
    bool SubmitOnce(uint64_t now) noexcept {
        if(!pending || submitted)return false;
        submitted=true;started=now;polls=0;return true;
    }
};
inline bool Storage(C c) noexcept {return c==C::Inventory || c==C::PersonalStash || c==C::SharedStash || c==C::Cube;}
inline uint8_t NativePage(C c) noexcept {return c==C::Inventory?0:c==C::Cube?3:(c==C::PersonalStash || c==C::SharedStash)?4:0xff;}
inline bool Identity(const Info& a,const Info& b) noexcept {
    return a.runtimeId==b.runtimeId && a.code==b.code && a.classId==b.classId &&
        a.generationSeed==b.generationSeed && a.itemSeed==b.itemSeed;
}
inline bool Source(const Info& a,const Info& b) noexcept {return Identity(a,b) && QolIdentify::Same(a,b);}
inline bool Confirmed(const Info& before,const Info& after,C destination,uint32_t sharedPage) noexcept {
    return Identity(before,after) && after.container==destination &&
        (destination!=C::SharedStash || (sharedPage!=UINT32_MAX && after.sharedStashPage==sharedPage));
}
inline bool NeedsStash(C source,C destination) noexcept {
    return source==C::PersonalStash || source==C::SharedStash || destination==C::PersonalStash || destination==C::SharedStash;
}
inline bool NeedsShared(C source,C destination) noexcept {return source==C::SharedStash || destination==C::SharedStash;}
inline bool Legal(C source,C destination,uint32_t code) noexcept {
    return Storage(source) && Storage(destination) && source!=destination &&
        !(destination==C::Cube && QolIdentify::Code(code,D2RL::Items::MakeItemCode("box"))) &&
        (source==C::Inventory || destination==C::Inventory || destination==C::Cube);
}
}
