#pragma once
#include "chat_forwarding_profile.h"
#include "plugin_coexistence_policy.h"
#include <array>
#include <cstring>
#include <limits>

namespace QolChatForwarding {
static_assert(std::size(Profile::Blocks)==static_cast<size_t>(Node::HookObject));
static_assert(std::size(ReleaseProfile::Blocks)==static_cast<size_t>(ReleaseNode::Continuation));
enum class Kind { Code, Data, Trampoline };
struct Memory {
    void* user{};
    bool (*read)(void*,uintptr_t,void*,size_t,Kind) noexcept{};
};
struct Result {
    const char* reason{};
    explicit operator bool() const noexcept { return reason==nullptr; }
};
inline bool Relative(uintptr_t instructionEnd,int32_t displacement,uintptr_t& target) noexcept {
    if(displacement>=0) {
        const auto distance=static_cast<uintptr_t>(displacement);
        if(instructionEnd>UINTPTR_MAX-distance)return false;
        target=instructionEnd+distance;
    } else {
        const auto distance=static_cast<uintptr_t>(-static_cast<int64_t>(displacement));
        if(instructionEnd<distance)return false;
        target=instructionEnd-distance;
    }
    return target!=0;
}
inline Result FinishValidation(const Memory& memory,uintptr_t hook,const unsigned char* entry,
    const unsigned char* original,size_t originalSize,uintptr_t resume,uintptr_t object,uintptr_t cookieAddress) noexcept {
    std::array<unsigned char,0x44> objectBytes{};
    uintptr_t cookie{},trampoline{};
    if((object&7) || (cookieAddress&7) ||
       !memory.read(memory.user,object,objectBytes.data(),objectBytes.size(),Kind::Data) ||
       !memory.read(memory.user,cookieAddress,&cookie,sizeof(cookie),Kind::Data))return {"data-owner"};
    // The exact /GS epilogue must compare the same initialized cookie loaded
    // by the gate. Its normal return requires a zero upper 16 bits. The failure
    // branch is unreachable for the unmodified frame/cookie on the checked
    // path. Stack-corruption recovery and chat-only code are outside this proof.
    if(!cookie || cookie>>48)return {"security-cookie"};
    std::memcpy(&trampoline,objectBytes.data()+8,sizeof(trampoline));
    std::array<unsigned char,28> relocated{};
    if(!memory.read(memory.user,trampoline,relocated.data(),relocated.size(),Kind::Trampoline))return {"trampoline-memory"};
    if(!QolCoexistence::SendChain(entry,original,originalSize,hook,relocated.data(),resume,true))return {"sender-continuation"};
    return {};
}
inline Result ValidateDebug(const Memory& memory,uintptr_t hook,const unsigned char* entry,
    const unsigned char* original,size_t originalSize,uintptr_t resume) noexcept {
    if(!memory.read || !hook || !entry || !original || originalSize<14)return {"sender-arguments"};
    constexpr auto count=static_cast<size_t>(Node::Count);
    std::array<uintptr_t,count> addresses{};
    std::array<Node,count> queue{};
    size_t queued=1,processed=0;
    queue[0]=Node::Hook;addresses[0]=hook;
    while(processed<queued) {
        const auto node=queue[processed++];
        const auto index=static_cast<size_t>(node),address=addresses[index];
        if(index>=std::size(Profile::Blocks))continue;
        const auto& block=Profile::Blocks[index];
        std::array<unsigned char,192> bytes{};
        if(block.size>bytes.size() || !memory.read(memory.user,address,bytes.data(),block.size,Kind::Code))return {block.name};
        for(size_t i=0;i<block.size;++i) {
            bool operand=false;
            for(size_t r=0;r<block.count;++r) {
                const auto offset=block.refs[r].displacement;
                if(offset>block.size || block.size-offset<4)return {"invalid-profile"};
                operand|=i>=offset && i-offset<4;
            }
            if(!operand && bytes[i]!=block.bytes[i])return {block.name};
        }
        for(size_t r=0;r<block.count;++r) {
            const auto& ref=block.refs[r];
            int32_t displacement{};std::memcpy(&displacement,bytes.data()+ref.displacement,4);
            uintptr_t target{};
            if(address>UINTPTR_MAX-ref.displacement-4 ||
               !Relative(address+ref.displacement+4,displacement,target))return {"reference-range"};
            const auto targetIndex=static_cast<size_t>(ref.target);
            if(targetIndex>=count)return {"invalid-profile"};
            if(addresses[targetIndex]) {
                if(addresses[targetIndex]!=target)return {"inconsistent-reference"};
            } else {
                if(queued==queue.size())return {"graph-overflow"};
                addresses[targetIndex]=target;queue[queued++]=ref.target;
            }
        }
    }
    if(queued!=count)return {"incomplete-graph"};
    // Distinct roles cannot point into each other's code/data windows. The
    // dispatch gate and epilogue are intentionally separate, disjoint blocks.
    for(size_t i=0;i<count;++i) {
        const size_t size=i<std::size(Profile::Blocks)?Profile::Blocks[i].size:
            i==static_cast<size_t>(Node::HookObject)?0x44:8;
        if(!addresses[i] || addresses[i]>UINTPTR_MAX-size)return {"node-range"};
        for(size_t j=0;j<i;++j) {
            const size_t previous=j<std::size(Profile::Blocks)?Profile::Blocks[j].size:
                j==static_cast<size_t>(Node::HookObject)?0x44:8;
            if(addresses[i]<addresses[j]+previous && addresses[j]<addresses[i]+size)return {"overlapping-roles"};
        }
    }
    const auto object=addresses[static_cast<size_t>(Node::HookObject)];
    const auto cookieAddress=addresses[static_cast<size_t>(Node::Cookie)];
    return FinishValidation(memory,hook,entry,original,originalSize,resume,object,cookieAddress);
}
inline Result ValidateRelease(const Memory& memory,uintptr_t hook,const unsigned char* entry,
    const unsigned char* original,size_t originalSize,uintptr_t resume) noexcept {
    if(!memory.read || !hook || !entry || !original || originalSize<14)return {"sender-arguments"};
    constexpr auto count=static_cast<size_t>(ReleaseNode::Count);
    std::array<uintptr_t,count> addresses{};
    std::array<ReleaseNode,count> queue{};
    size_t queued=1,processed=0;
    queue[0]=ReleaseNode::Hook;addresses[0]=hook;
    while(processed<queued) {
        const auto node=queue[processed++];
        const auto index=static_cast<size_t>(node),address=addresses[index];
        if(index>=std::size(ReleaseProfile::Blocks))continue;
        const auto& block=ReleaseProfile::Blocks[index];
        std::array<unsigned char,192> bytes{};
        if(block.size>bytes.size() || !memory.read(memory.user,address,bytes.data(),block.size,Kind::Code))return {block.name};
        for(size_t i=0;i<block.size;++i) {
            bool operand=false;
            for(size_t r=0;r<block.count;++r) {
                const auto offset=block.refs[r].displacement;
                if(offset>block.size || block.size-offset<4)return {"invalid-profile"};
                operand|=i>=offset && i-offset<4;
            }
            if(!operand && bytes[i]!=block.bytes[i])return {block.name};
        }
        for(size_t r=0;r<block.count;++r) {
            const auto& ref=block.refs[r];
            int32_t displacement{};std::memcpy(&displacement,bytes.data()+ref.displacement,4);
            uintptr_t target{};
            if(address>UINTPTR_MAX-ref.displacement-4 ||
               !Relative(address+ref.displacement+4,displacement,target))return {"reference-range"};
            const auto targetIndex=static_cast<size_t>(ref.target);
            if(targetIndex>=count)return {"invalid-profile"};
            if(addresses[targetIndex]) {
                if(addresses[targetIndex]!=target)return {"inconsistent-reference"};
            } else {
                if(queued==queue.size())return {"graph-overflow"};
                addresses[targetIndex]=target;queue[queued++]=ref.target;
            }
        }
    }
    if(queued!=count)return {"incomplete-graph"};
    const auto continuation=addresses[static_cast<size_t>(ReleaseNode::Continuation)];
    const auto counter=addresses[static_cast<size_t>(ReleaseNode::Counter)];
    const auto cookieAddress=addresses[static_cast<size_t>(ReleaseNode::Cookie)];
    if(continuation<8 || continuation>UINTPTR_MAX-0x3c || counter!=continuation+0x38)return {"inconsistent-reference"};
    const auto object=continuation-8;
    struct Region { uintptr_t start; size_t size; };
    const std::array<Region,6> regions={{
        {addresses[static_cast<size_t>(ReleaseNode::Hook)],ReleaseProfile::Blocks[0].size},
        {addresses[static_cast<size_t>(ReleaseNode::Dispatch)],ReleaseProfile::Blocks[1].size},
        {addresses[static_cast<size_t>(ReleaseNode::DispatchTail)],ReleaseProfile::Blocks[2].size},
        {addresses[static_cast<size_t>(ReleaseNode::CookieCheck)],ReleaseProfile::Blocks[3].size},
        {object,0x44},
        {cookieAddress,8},
    }};
    for(size_t i=0;i<regions.size();++i) {
        if(!regions[i].start || regions[i].start>UINTPTR_MAX-regions[i].size)return {"node-range"};
        for(size_t j=0;j<i;++j) {
            if(regions[i].start<regions[j].start+regions[j].size &&
               regions[j].start<regions[i].start+regions[i].size)return {"overlapping-roles"};
        }
    }
    return FinishValidation(memory,hook,entry,original,originalSize,resume,object,cookieAddress);
}
// This is a proof for the non-chat item requests generated by QOL's separately
// guarded native builders. It is not a general Global Chat API or hook chain.
// Every CALL used on that path resolves to another fully checked graph node.
inline Result Validate(const Memory& memory,uintptr_t hook,const unsigned char* entry,
    const unsigned char* original,size_t originalSize,uintptr_t resume) noexcept {
    const auto debug=ValidateDebug(memory,hook,entry,original,originalSize,resume);
    if(debug || !debug.reason || std::strcmp(debug.reason,"Hook")!=0)return debug;
    return ValidateRelease(memory,hook,entry,original,originalSize,resume);
}
} // namespace QolChatForwarding
