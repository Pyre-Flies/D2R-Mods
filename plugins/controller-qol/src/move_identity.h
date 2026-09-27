#pragma once
#include "identify_action.h"
namespace QolMove {
inline bool CustomRoute(bool stash,bool cube,bool vendor) noexcept {return !stash && !cube && !vendor;}
inline D2RL::ItemHandle Resolve(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,
    const D2RL::InventoryService* inventory,D2RL::PlayerHandle player,const D2RL::Items::ItemInfo& identity) noexcept {
    if(!items || !inventory || !player || identity.structSize!=D2RL::Items::ItemInfoSize)return D2RL::InvalidItemHandle;
    D2RL::Items::ItemInfo info{.structSize=D2RL::Items::ItemInfoSize};
    if(items->getItemInfo(ctx,identity.handle,&info)==D2RL::Items::Result::Success && QolIdentify::Same(identity,info))return info.handle;
    struct Find {const D2RL::Items::ItemInfo* identity;D2RL::ItemHandle found{};} find{&identity};
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(identity.container),0};
    const auto result=inventory->forEachInventoryItem(ctx,player,&filter,
        [](const D2RL::PluginContext*,const D2RL::Items::ItemInfo* item,void* user) noexcept {
            auto& f=*static_cast<Find*>(user);
            if(item && QolIdentify::Same(*f.identity,*item)){f.found=item->handle;return D2RL::Inventory::IterationAction::Stop;}
            return D2RL::Inventory::IterationAction::Continue;
        },&find);
    return result==D2RL::Inventory::Result::Success?find.found:D2RL::InvalidItemHandle;
}
}
