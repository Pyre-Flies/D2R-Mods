#pragma once
#include "identify_action.h"
#include "client_transfer_policy.h"
#include <array>
namespace QolBulkStash {
using Info=D2RL::Items::ItemInfo;
inline bool Source(const Info& i) noexcept {return i.container==D2RL::Items::ItemContainer::Inventory && i.inventoryPage==0;}
struct Plan {
    std::array<Info,256> items{};unsigned count{},next{},submitted{},removed{},skipped{};bool overflow{};
    std::array<bool,256> sent{},present{},notified{};
    std::array<uint32_t,256> baselineClassIds{},baselineCounts{};unsigned baselineKnown{};
    std::array<uint32_t,256> rejectedClassIds{},rejectedQuantities{};unsigned rejectedKnown{};
    static constexpr uint32_t ItemQuantity(int32_t quantity) noexcept {
        return quantity>0?static_cast<uint32_t>(quantity):1u;
    }
    void Submitted(unsigned index) noexcept {if(index<count && !sent[index]){sent[index]=true;++submitted;}}
    void RejectClass(uint32_t classId,int32_t itemQuantity=1) noexcept {
        const uint32_t qty=ItemQuantity(itemQuantity);
        for(unsigned n=0;n<rejectedKnown;++n) {
            if(rejectedClassIds[n]==classId) {
                if(qty<rejectedQuantities[n])rejectedQuantities[n]=qty;
                return;
            }
        }
        if(rejectedKnown<rejectedClassIds.size()) {
            rejectedClassIds[rejectedKnown]=classId;
            rejectedQuantities[rejectedKnown]=qty;
            ++rejectedKnown;
        }
    }
    bool IsClassRejected(uint32_t classId,int32_t itemQuantity=1) const noexcept {
        const uint32_t qty=ItemQuantity(itemQuantity);
        for(unsigned n=0;n<rejectedKnown;++n) {
            if(rejectedClassIds[n]==classId)return qty>=rejectedQuantities[n];
        }
        return false;
    }
    static constexpr uint32_t ServerCapRejectedQuantity(uint32_t liveCount,uint32_t defaultLimit=99u) noexcept {
        return liveCount>=defaultLimit?1u:(defaultLimit-liveCount+1u);
    }
    void RejectSubmitted(unsigned index,int32_t rejectedQuantity=0) noexcept {
        if(index>=count)return;
        if(sent[index]) {
            sent[index]=false;
            if(submitted>0)--submitted;
        }
        ++skipped;
        RejectClass(items[index].classId,rejectedQuantity>0?rejectedQuantity:items[index].quantity);
    }
    uint32_t SubmittedForClass(uint32_t classId) const noexcept {
        uint32_t total=0;
        for(unsigned n=0;n<count;++n) {
            if(sent[n] && items[n].classId==classId) {
                total+=ItemQuantity(items[n].quantity);
            }
        }
        return total;
    }
    uint32_t EffectiveCount(uint32_t classId,uint32_t liveCount) noexcept {
        uint32_t baseline=liveCount;
        bool found=false;
        for(unsigned n=0;n<baselineKnown;++n) {
            if(baselineClassIds[n]==classId){baseline=baselineCounts[n];found=true;break;}
        }
        if(!found && baselineKnown<baselineClassIds.size()) {
            baselineClassIds[baselineKnown]=classId;
            baselineCounts[baselineKnown]=liveCount;
            ++baselineKnown;
        }
        const uint32_t projected=baseline+SubmittedForClass(classId);
        return liveCount>projected?liveCount:projected;
    }
    static bool HasStackRoom(uint32_t effectiveCount,uint32_t maxLimit,bool nativeFull=false,int32_t itemQuantity=1) noexcept {
        const uint32_t incoming=itemQuantity>0?static_cast<uint32_t>(itemQuantity):1u;
        return !nativeFull && maxLimit>0 && effectiveCount<maxLimit && (maxLimit-effectiveCount)>=incoming;
    }
    static constexpr bool IsServerCapCandidate(uint32_t liveCount,uint32_t localLimit,int32_t itemQuantity=1,uint32_t defaultLimit=99u) noexcept {
        const uint32_t incoming=ItemQuantity(itemQuantity);
        return localLimit>defaultLimit && liveCount<localLimit && (liveCount>=defaultLimit || (defaultLimit-liveCount)<incoming);
    }
    void BeginVerification() noexcept {present.fill(false);}
    void ObserveRemaining(const Info& i) noexcept {
        for(unsigned n=0;n<count;++n)if(sent[n] && items[n].runtimeId==i.runtimeId)present[n]=true;
    }
    unsigned Remaining() noexcept {
        unsigned remaining=0;for(unsigned n=0;n<count;++n)if(sent[n] && present[n])++remaining;
        removed=submitted-remaining;return remaining;
    }
    template<class Notify> void ConfirmRemoval(unsigned index,Notify notify) noexcept {
        if(index>=count || !sent[index] || notified[index])return;
        notified[index]=true;notify(items[index]);
    }
    template<class Notify> void ConfirmAbsent(bool enumerationSucceeded,Notify notify) noexcept {
        // A failed/partial enumeration must never turn an unobserved identity
        // into removal feedback. Repeated successful polls notify only once.
        if(!enumerationSucceeded)return;
        for(unsigned n=0;n<count;++n)if(!present[n])ConfirmRemoval(n,notify);
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
enum class RemoteStep { Ready, Wait, Removed, Rejected, Stop };
inline RemoteStep CheckRemote(const Info& expected,const Info* actual,bool submitted,bool expired) noexcept {
    if(!actual)return submitted?RemoteStep::Removed:RemoteStep::Stop;
    if(!Source(*actual) || !QolClientTransfer::Source(expected,*actual))return RemoteStep::Stop;
    if(!submitted)return RemoteStep::Ready;
    return expired?RemoteStep::Rejected:RemoteStep::Wait;
}
}
