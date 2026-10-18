#include "unhoarder_visibility.h"
#include "client_loot_policy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace C=D2RL::PluginCommunication;
using D=UnhoarderVisibility::Decision;
static D decision=D::Visible;
static bool available=true;
static int leases{},queries{};
static void Check(bool ok){if(!ok)std::abort();}
static D __cdecl Query(const void*,uint32_t,uint32_t) noexcept {Check(leases==1);++queries;return decision;}
static UnhoarderVisibility::Service visibility{sizeof(visibility),1,&Query};
static C::Result __cdecl Acquire(const D2RL::PluginContext*,const C::AcquireServiceRequest* r,C::AcquiredService* out) noexcept {
 Check(std::strcmp(r->providerPluginId,"unhoarder")==0 && std::strcmp(r->name,UnhoarderVisibility::Name)==0);
 if(!available)return C::Result::NotFound;
 ++leases;out->handle=1;out->table=&visibility;out->tableSize=sizeof(visibility);out->serviceVersion=1;return C::Result::Success;
}
static C::Result __cdecl Release(const D2RL::PluginContext*,C::ServiceHandle) noexcept {Check(leases==1);--leases;return C::Result::Success;}
int main(){
 D2RL::PluginCommunicationService broker{};broker.serviceSize=sizeof(broker);broker.serviceVersion=1;broker.acquireService=&Acquire;broker.releaseService=&Release;
 static const void* published{};published=&broker;
 D2RL::PluginApi api{};api.apiSize=sizeof(api);
 api.queryService=+[](const D2RL::PluginContext*,D2RL::ServiceId,uint32_t,const void** out) noexcept {*out=published;return D2RL::ServiceQueryResult::Success;};
 D2RL::PluginContext context{};context.contextSize=sizeof(context);context.api=&api;
 for(auto d:{D::Visible,D::Unknown,D::Hidden}){decision=d;Check(QolLootVisibility::Hidden(&context,&context,7,8)==(d==D::Hidden));Check(leases==0);}
 available=false;Check(!QolLootVisibility::Hidden(&context,&context,7,8));Check(leases==0);available=true;
 visibility.version=99;Check(!QolLootVisibility::Hidden(&context,&context,7,8));Check(leases==0);visibility.version=1;
 Check(!QolLootVisibility::Hidden(nullptr,&context,7,8));Check(queries==3);
 using namespace QolClientLoot;
 Candidate a{1,11,2,1,3},hidden{2,12,2,2,2},y{3,13,2,3,1},replacement{4,14,2,0,4};
 std::vector<Candidate> eligible{a,y};auto compact=Assign({},eligible);
 auto initial=HoldSnapshot({},compact,true);
 Check(initial[0].guid==1 && initial[1].guid==3 && !initial[2].guid);
 Slots held{};held[0]=a;held[1]=hidden;held[2]=y;eligible.push_back(replacement);
 auto retained=PruneHiddenSlots(held,{hidden.guid});
 Check(retained[0].guid==1 && !retained[1].guid && retained[2].guid==3);
 // A held candidate remains assigned even after moving below the top seven.
 for(uint32_t i=10;i<20;++i)eligible.push_back({i,i,2,0,99});
 retained=PruneHiddenSlots(HoldSnapshot(held,Assign({},eligible),false),{hidden.guid});
 Check(retained[0].guid==1 && !retained[1].guid && retained[2].guid==3);
 std::puts("Visibility: explicit hidden only, missing/unknown fallback, scoped leases, compact initial slots and stable held holes.");
}
