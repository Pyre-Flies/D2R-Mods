#pragma once
#include <D2RLPlugin/item.h>
namespace QolVendor {
constexpr uint32_t TomeForScroll(uint32_t code) noexcept {
    using D2RL::Items::MakeItemCode;
    return code==MakeItemCode("isc")?MakeItemCode("ibk"):code==MakeItemCode("tsc")?MakeItemCode("tbk"):0;
}
constexpr bool TomeRoom(int32_t quantity,int32_t maximum) noexcept {
    return quantity>=0 && maximum>0 && maximum<=511 && quantity<maximum;
}
using SellFn=void(__fastcall*)(void*,void*,void*,bool,bool,bool);
inline bool Context(bool shop,bool stash,bool cube,D2RL::Items::ItemContainer source) noexcept {
    return shop && !stash && !cube && source==D2RL::Items::ItemContainer::Inventory;
}
inline bool SubmitBuy(SellFn fn,void* panel,void* player,void* item) noexcept {
    if (!fn || !panel || !player || !item) return false;
    fn(panel,player,item,false,true,true); // native buy, immediate, forced Shift
    return true;
}
inline bool Submit(SellFn fn,void* panel,void* player,void* item) noexcept {
    if (!fn || !panel || !player || !item) return false;
    fn(panel,player,item,true,true,false); // native quick sell: sell, immediate, no forced shift
    return true;
}
}
