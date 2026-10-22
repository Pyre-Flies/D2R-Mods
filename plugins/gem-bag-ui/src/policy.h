#pragma once
#include <D2RLPlugin/api.h>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace GemBag {
using Info=D2RL::Items::ItemInfo;
using Container=D2RL::Items::ItemContainer;
// English names are diagnostic labels only; visible names use the game's key.
struct Gem { const char* name; uint32_t code; const char* nameKey; };
inline constexpr std::array<Gem,8> Gems{{
    {"Amethyst",D2RL::Items::MakeItemCode("gmm"),"@d2r:gmm"},
    {"Topaz",D2RL::Items::MakeItemCode("gmt"),"@d2r:gmt"},
    {"Sapphire",D2RL::Items::MakeItemCode("gms"),"@d2r:gms"},
    {"Emerald",D2RL::Items::MakeItemCode("gme"),"@d2r:gme"},
    {"Diamond",D2RL::Items::MakeItemCode("gmd"),"@d2r:gmd"},
    {"Ruby",D2RL::Items::MakeItemCode("gmr"),"@d2r:gmr"},
    {"Skull",D2RL::Items::MakeItemCode("gmk"),"@d2r:gmk"},
    {"Chaos Onyx",D2RL::Items::MakeItemCode("gmo"),"@d2r:gmo"}
}};
inline constexpr uint32_t BagCode=D2RL::Items::MakeItemCode("bag");
inline constexpr uint32_t CreditStat=386;
inline constexpr uint32_t StorageMask=D2RL::Items::ContainerBit(Container::Inventory)|
    D2RL::Items::ContainerBit(Container::Cube)|D2RL::Items::ContainerBit(Container::PersonalStash)|
    D2RL::Items::ContainerBit(Container::SharedStash);
inline bool Supported(const Info& i) noexcept {
    return i.code && i.classId<65536 && i.handle && i.runtimeId && (D2RL::Items::ContainerBit(i.container)&StorageMask) &&
        (i.container!=Container::SharedStash || i.sharedStashPage!=UINT32_MAX);
}
inline bool Same(const Info& a,const Info& b) noexcept {
    return a.runtimeId==b.runtimeId && a.classId==b.classId && a.code==b.code &&
        a.generationSeed==b.generationSeed && a.itemSeed==b.itemSeed &&
        a.container==b.container && a.inventoryPage==b.inventoryPage && a.sharedStashPage==b.sharedStashPage && a.x==b.x && a.y==b.y;
}
inline bool SameStoredTarget(const Info& ui,const Info& game) noexcept {
    return ui.container==game.container&&ui.inventoryPage==game.inventoryPage&&ui.x==game.x&&ui.y==game.y&&
        ui.code==game.code&&ui.classId==game.classId&&ui.quality==game.quality&&
        (ui.container!=Container::SharedStash||ui.sharedStashPage==game.sharedStashPage);
}
inline bool FreshHover(uint64_t observed,uint64_t now,long dx,long dy) noexcept {
    return observed&&now>=observed&&now-observed<=400&&dx>=-4&&dx<=4&&dy>=-4&&dy<=4;
}
inline const char* Destination(const Info& i) noexcept {
    switch(i.container) {
    case Container::Inventory:return "Inventory";
    case Container::Cube:return "Horadric Cube";
    case Container::PersonalStash:return "Personal Stash";
    case Container::SharedStash:return "Shared Stash";
    default:return "Unavailable";
    }
}
struct Cell {uint32_t x{},y{};};
inline std::optional<Cell> FreeCell(unsigned w,unsigned h,const std::array<uintptr_t,256>& cells) noexcept {
    if(!w || !h || w>16 || h>16)return {};
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)if(!cells[y*w+x])return Cell{x,y};
    return {};
}
struct Gesture {
    unsigned down{},owned{};
    bool Edge(unsigned key,bool pressed) noexcept {const bool edge=pressed && !(down&key);if(pressed)down|=key;else down&=~key;return edge;}
    bool ReleaseOwned(unsigned key,bool pressed) noexcept {
        if(!(owned&key))return false;
        if(!pressed)owned&=~key;
        return true;
    }
};
// All mutations occur synchronously in one authoritative game callback. A
// failed step must compensate before returning; uncertain compensation latches
// the capability off. This is not an extension of the SDK transaction ABI.
template<class Backend> bool Withdraw(Backend& b) {
    if(!b.Preflight())return false;
    if(!b.Debit())return false;
    if(!b.Create()) {if(!b.Restore())b.Halt();return false;}
    if(b.Verify() && b.Publish())return true;
    const bool removed=b.Remove();
    // Never refund while an unremoved gem could remain: report the uncertain
    // state and stop. This prevents a compensation failure minting a free gem.
    if(!removed){b.Halt();return false;}
    if(!b.Restore())b.Halt();
    return false;
}
}
