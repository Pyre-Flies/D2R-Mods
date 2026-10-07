#pragma once
#include "identify_action.h"
#include "client_transfer_policy.h"
#include <array>
namespace QolBulkStash {
using Info=D2RL::Items::ItemInfo;
inline bool Source(const Info& i) noexcept {return i.container==D2RL::Items::ItemContainer::Inventory && i.inventoryPage==0;}
struct Plan {
    std::array<Info,256> items{};unsigned count{},next{},submitted{},removed{},skipped{};bool overflow{};
    std::array<bool,256> sent{},present{};
    void Submitted(unsigned index) noexcept {if(index<count && !sent[index]){sent[index]=true;++submitted;}}
    void BeginVerification() noexcept {present.fill(false);}
    void ObserveRemaining(const Info& i) noexcept {
        for(unsigned n=0;n<count;++n)if(sent[n] && items[n].runtimeId==i.runtimeId)present[n]=true;
    }
    unsigned Remaining() noexcept {
        unsigned remaining=0;for(unsigned n=0;n<count;++n)if(sent[n] && present[n])++remaining;
        removed=submitted-remaining;return remaining;
    }
    void Observe(const Info& i) noexcept {
        if(!Source(i))return;
        for(unsigned n=0;n<count;++n)if(items[n].runtimeId==i.runtimeId)return;
        if(count==items.size()){overflow=true;return;}
        items[count++]=i;
    }
};
enum class Check { Missing, Changed, Ready };
inline Check Validate(const Info& expected,const Info* actual) noexcept {
    return !actual?Check::Missing:!Source(*actual)||!QolIdentify::Same(expected,*actual)?Check::Changed:Check::Ready;
}
enum class RemoteStep { Ready, Wait, Removed, Stop };
inline RemoteStep CheckRemote(const Info& expected,const Info* actual,bool submitted,bool expired) noexcept {
    if(!actual)return submitted?RemoteStep::Removed:RemoteStep::Stop;
    if(!Source(*actual) || !QolClientTransfer::Source(expected,*actual))return RemoteStep::Stop;
    if(!submitted)return RemoteStep::Ready;
    return expired?RemoteStep::Stop:RemoteStep::Wait;
}
}
