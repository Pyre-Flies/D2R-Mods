#pragma once
#include "catalog.h"
namespace GemBag {
inline bool DepositPage(const Info& bag,const Info& item) {
    return Supported(item)&&item.runtimeId!=bag.runtimeId&&item.container==bag.container&&item.inventoryPage==bag.inventoryPage&&
        item.sharedStashPage==bag.sharedStashPage&&item.quality==D2RL::Items::Quality::Normal&&item.quantity>=1&&item.quantity<=4096&&
        !item.socketCount&&!item.socketedItemCount&&!(item.stateFlags&D2RL::Items::ItemStateEthereal)&&
        std::all_of(std::begin(item.prefixIds),std::end(item.prefixIds),[](auto n){return n==0;})&&
        std::all_of(std::begin(item.suffixIds),std::end(item.suffixIds),[](auto n){return n==0;});
}
// A credit write is reversible only while the input is confirmed unchanged.
// Once consumption succeeds, publication failure cannot justify a refund.
template<class Backend> bool DepositOne(Backend& b) {
    if(!b.Credit())return false;
    if(!b.Consume()) {
        if(!b.Unchanged()||!b.Restore())b.Halt();
        return false;
    }
    if(!b.Verify()||!b.Publish()){b.Halt();return false;}
    return true;
}
}
