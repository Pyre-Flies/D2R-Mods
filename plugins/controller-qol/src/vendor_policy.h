#pragma once
#include <D2RLPlugin/item.h>
namespace QolVendor {
using SellFn=void(__fastcall*)(void*,void*,void*,bool,bool,bool);
inline bool Context(bool shop,bool stash,bool cube,D2RL::Items::ItemContainer source) noexcept {
    return shop && !stash && !cube && source==D2RL::Items::ItemContainer::Inventory;
}
inline bool Submit(SellFn fn,void* panel,void* player,void* item) noexcept {
    if (!fn || !panel || !player || !item) return false;
    fn(panel,player,item,true,true,false); // native quick sell: sell, immediate, no forced shift
    return true;
}
}
