#include "unhoarder_visibility.h"
#include "client_loot_policy.h"
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
namespace C=D2RL::PluginCommunication;
using D=UnhoarderLabelVisibility::Decision;
static unsigned leases{},queries{},fallbackReads{},checks{};
static bool available=true,legacyHidden=false,ready=true;
static uint64_t fixtureTick=1000;
static D decision=D::CannotHide;
static uint32_t returnedCode=123;
static int unit;
static void Check(bool ok,const char* what){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s (queries=%u fallback=%u)\n",what,queries,fallbackReads);std::exit(1);}}
static D __cdecl Query(const void* observed,uint32_t guid) noexcept {
    Check(leases==1 && observed==&unit && guid!=0,"borrowed item observed under broker lease");
    ++queries;return decision;
}
static UnhoarderLabelVisibility::Service service{sizeof(service),1,&Query};
static C::Result __cdecl Acquire(const D2RL::PluginContext*,const C::AcquireServiceRequest* request,C::AcquiredService* result) noexcept {
    Check(std::strcmp(request->providerPluginId,"unhoarder")==0 && std::strcmp(request->name,UnhoarderLabelVisibility::Name)==0,"exact provider and service");
    if(!available)return C::Result::NotFound;
    ++leases;result->handle=1;result->table=&service;result->tableSize=sizeof(service);result->serviceVersion=1;return C::Result::Success;
}
static C::Result __cdecl Release(const D2RL::PluginContext*,C::ServiceHandle) noexcept {Check(leases==1,"lease balanced");--leases;return C::Result::Success;}
namespace QolClientLoot {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};
LocalSession localSession{7,1,true};
std::unordered_map<uint32_t,uint64_t> visible;
std::unordered_map<uint32_t,VisibilityProof> providerHidden;
bool Ready() noexcept {return ready;}
bool HiddenForPickup(uint32_t guid) noexcept {
    ++fallbackReads;
    if(legacyHidden)providerHidden[guid]={localSession.generation,fixtureTick,returnedCode};
    else providerHidden.erase(guid);
    return legacyHidden;
}
static ULONGLONG Clock() noexcept{return fixtureTick;}
#define GetTickCount64 Clock
#include "label_track_runtime.inc"
#undef GetTickCount64
}
int main() {
    D2RL::PluginCommunicationService broker{};broker.serviceSize=sizeof(broker);broker.serviceVersion=1;broker.acquireService=&Acquire;broker.releaseService=&Release;
    static const void* published{};published=&broker;
    D2RL::PluginApi api{};api.apiSize=sizeof(api);
    api.queryService=+[](const D2RL::PluginContext*,D2RL::ServiceId,uint32_t,const void** out) noexcept {*out=published;return D2RL::ServiceQueryResult::Success;};
    D2RL::PluginContext owned{};owned.contextSize=sizeof(owned);owned.api=&api;QolClientLoot::context=&owned;
    using namespace QolClientLoot;
    // The reported reproducer: fifteen labels repeatedly observed while idle.
    for(unsigned frame=0;frame<120;++frame)for(uint32_t id=1;id<=15;++id)Track(id,true,&unit);
    Check(fallbackReads==0 && queries==1800,"combined label observations do not repeat QOL native lookup/compatibility path");
    Check(visible.size()==15 && providerHidden.empty() && leases==0,"visible tracking retained without hidden proofs");
    decision=D::RequireQuery;legacyHidden=true;Track(1,true,&unit);
    Check(fallbackReads==1,"potentially hidden items retain full native validation and visibility query");
    Check(providerHidden.at(1).code==123 && providerHidden.at(1).Current(7,1250),"same-session hidden pickup proof retained");
    Check(!providerHidden.at(1).Current(7,1251) && !providerHidden.at(1).Current(8,1000),"proof expiry and session fences unchanged");
    legacyHidden=false;Track(1,true,&unit);Check(!providerHidden.contains(1) && fallbackReads==2,"legacy guard refusal/unknown still clears proof");
    legacyHidden=true;decision=static_cast<D>(99);Track(1,true,&unit);Check(fallbackReads==3 && providerHidden.contains(1),"unrecognized response cannot skip original guards");
    decision=D::CannotHide;Track(1,true,&unit);Check(!providerHidden.contains(1) && fallbackReads==3,"fresh no-Hide proof clears old hidden state");
    decision=D::RequireQuery;Track(1,true,&unit);Track(1,false,nullptr);Check(!providerHidden.contains(1) && !visible.contains(1),"removed label clears observation and proof");
    available=false;Track(2,true,&unit);Check(fallbackReads==5 && providerHidden.contains(2),"older provider retains complete legacy query");available=true;
    service.version=99;Track(3,true,&unit);Check(fallbackReads==6 && leases==0,"unsupported service uses guarded legacy query");service.version=1;
    Track(4,true,nullptr);Check(fallbackReads==7,"callers without borrowed label retain legacy checks");
    const auto before=queries;ready=false;Track(5,true,&unit);ready=true;Track(0,true,&unit);Check(queries==before,"inactive session and invalid id do no provider work");
    std::printf("%u production label-observation and broker checks passed; 1800 repeated native queries eliminated.\n",checks);
}
