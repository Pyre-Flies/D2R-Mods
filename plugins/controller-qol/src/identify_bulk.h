#pragma once
#include "identify_action.h"
#include <array>
namespace QolIdentify {
inline bool BulkTome(const Info& item) noexcept {
    return NativeContainer(item.container) && Code(item.code,D2RL::Items::MakeItemCode("ibk"));
}
enum class BulkReady { Ready, Wait, Expired };
inline BulkReady CheckBulkReady(bool blocked,uint64_t elapsed) noexcept {return elapsed>=5000?BulkReady::Expired:blocked?BulkReady::Wait:BulkReady::Ready;}
enum class BulkStep { Changed, Skip, Empty, Identify };
inline BulkStep CheckBulkTarget(const Info& expected,const Info& actual,bool available,int32_t charges) noexcept {
    if(!available || !Same(expected,actual))return BulkStep::Changed;
    if(actual.stateFlags&D2RL::Items::ItemStateIdentified)return BulkStep::Skip;
    return charges>0?BulkStep::Identify:BulkStep::Empty;
}
struct BulkPlan {
    Info requested{},tome{};
    std::array<Info,256> targets{};
    unsigned count{},next{},completed{};
    bool found{},overflow{},includeCube{};
    uint32_t TargetMask() const noexcept {
        return D2RL::Items::ContainerBit(Container::Inventory) |
            (includeCube?D2RL::Items::ContainerBit(Container::Cube):0u);
    }
    void Observe(const Info& item) noexcept {
        if(Same(item,requested)){tome=item;found=true;}
        if(!(TargetMask()&D2RL::Items::ContainerBit(item.container)) || (item.stateFlags&D2RL::Items::ItemStateIdentified) ||
           Code(item.code,D2RL::Items::MakeItemCode("ibk")))return;
        for(unsigned i=0;i<count;++i)if(targets[i].runtimeId==item.runtimeId)return;
        if(count==targets.size()){overflow=true;return;}
        targets[count++]=item;
    }
};
}
