#pragma once
#include "policy.h"
#include <cstring>
#include <type_traits>

namespace GemBag::Protocol {
inline constexpr uint64_t Compatibility=0x47454d4241470003ull;
inline constexpr uint32_t Magic=0x31424752;
inline constexpr uint16_t RequestMessage=1,ReplyMessage=2;
enum class Action:uint16_t { Hello=1,Inspect=2,Withdraw=3,Deposit=4 };
enum class Status:uint32_t { Ok,Refused,Stale,Busy,Unavailable,Halted };
inline constexpr uint32_t HasSnapshot=1,Success=2,Halted=4,HasCell=8,AllFlags=15;
struct Target {
    uint32_t code{},classId{},quality{},container{},page{},x{},y{},sharedPage{};
    bool operator==(const Target&) const=default;
};
inline Target Identify(const Info& info) {
    return {info.code,info.classId,static_cast<uint32_t>(info.quality),static_cast<uint32_t>(info.container),
        static_cast<uint32_t>(info.inventoryPage),static_cast<uint32_t>(info.x),static_cast<uint32_t>(info.y),info.sharedStashPage};
}
inline bool Valid(const Target& target) {
    const auto container=static_cast<Container>(target.container);
    const bool page=(container==Container::Inventory&&target.page==0)||(container==Container::Cube&&target.page==3)||
        ((container==Container::PersonalStash||container==Container::SharedStash)&&target.page==4);
    return target.code&&target.classId<65536&&target.quality>=1&&target.quality<=8&&page&&target.x<16&&target.y<16&&
        (container==Container::SharedStash?target.sharedPage!=UINT32_MAX:target.sharedPage==UINT32_MAX);
}
struct Request {
    uint64_t clientSession{},serverSession{},sequence{},lease{};
    Target target{};
    uint32_t magic=Magic;
    Action action{};
    uint16_t gem{};
    bool operator==(const Request&) const=default;
};
struct Reply {
    Request request{};
    uint64_t serverSession{},lease{};
    uint32_t flags{},credits{},width{},height{},cellX{},cellY{};
    Status status=Status::Refused;
    uint32_t reserved{};
    uint64_t definition{};
    uint32_t bank{},gemMask{};
    char message[88]{};
};
inline bool Valid(const Request& request) {
    if(request.magic!=Magic||!request.clientSession||!request.sequence)return false;
    if(request.action==Action::Hello)return !request.serverSession&&!request.lease&&!request.gem&&request.target==Target{};
    if(!request.serverSession||!Valid(request.target))return false;
    if(request.action==Action::Inspect)return !request.lease&&!request.gem;
    if(request.action==Action::Deposit)return request.lease&&!request.gem;
    return request.action==Action::Withdraw&&request.lease&&request.gem<Gems.size();
}
inline bool Valid(const Reply& reply) {
    if(!Valid(reply.request)||reply.reserved||(reply.flags&~AllFlags)||reply.status>Status::Halted||
        !std::memchr(reply.message,0,sizeof(reply.message)))return false;
    if(reply.flags&HasSnapshot) {
        if(reply.request.action==Action::Hello||!reply.lease||!reply.serverSession||!reply.definition||reply.bank<1||reply.bank>3||!reply.gemMask||(reply.gemMask&~255u)||reply.credits>65535||!reply.width||!reply.height||reply.width>16||reply.height>16)return false;
        if((reply.flags&HasCell)&&(reply.cellX>=reply.width||reply.cellY>=reply.height))return false;
    } else if(reply.flags&HasCell)return false;
    return !(reply.flags&Success)||reply.status==Status::Ok;
}
template<class T> bool Copy(const void* data,uint32_t size,T& result) {
    if(!data||size!=sizeof(T))return false;
    std::memcpy(&result,data,sizeof(result));return Valid(result);
}
static_assert(std::is_trivially_copyable_v<Request>&&std::is_trivially_copyable_v<Reply>);
static_assert(sizeof(Target)==32&&sizeof(Request)==72&&sizeof(Reply)==224);
static_assert(sizeof(Reply)<=D2RL::Network::MaxPayloadSize);
}
