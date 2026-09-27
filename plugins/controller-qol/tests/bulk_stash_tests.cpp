#include "bulk_stash_policy.h"
#include "bulk_stash_input.h"
#include <cstdio>
#include <cstdlib>
using namespace QolBulkStash;
void Require(bool b,const char* why){if(!b){std::printf("FAIL: %s\n",why);std::exit(1);}}
int main(){
    Info i{};i.runtimeId=1;i.code=123;i.container=D2RL::Items::ItemContainer::Inventory;i.inventoryPage=0;
    Plan p{};p.Observe(i);p.Observe(i);Require(p.count==1,"snapshot deduplicates item identity");
    for(auto c:{D2RL::Items::ItemContainer::Cube,D2RL::Items::ItemContainer::PersonalStash,D2RL::Items::ItemContainer::SharedStash,D2RL::Items::ItemContainer::Belt}){auto x=i;x.container=c;x.runtimeId++;p.Observe(x);}
    auto x=i;x.inventoryPage=1;x.runtimeId++;p.Observe(x);Require(p.count==1,"only ordinary inventory sources scanned");
    Require(Validate(i,&i)==Check::Ready,"unchanged source ready");
    x=i;++x.x;Require(Validate(i,&x)==Check::Changed,"moved item refused");
    x=i;++x.runtimeId;Require(Validate(i,&x)==Check::Changed,"replacement refused");
    Require(Validate(i,nullptr)==Check::Missing,"absence distinct from changed source");
    for(unsigned n=2;n<=257;++n){x=i;x.runtimeId=n;p.Observe(x);}Require(p.overflow && p.count==256,"overflow refuses partial batch");
    Plan batch{};for(unsigned n=1;n<=8;++n){x=i;x.runtimeId=n;batch.Observe(x);}
    for(unsigned n=0;n<batch.count;++n)batch.Submitted(n);
    batch.Submitted(0);Require(batch.submitted==8,"whole snapshot submitted once before verification");
    batch.BeginVerification();batch.ObserveRemaining(batch.items[3]);batch.ObserveRemaining(batch.items[3]);
    x=i;x.runtimeId=99;batch.ObserveRemaining(x);
    Require(batch.Remaining()==1 && batch.removed==7,"partial batch counts only submitted identities, no duplicates or unrelated items");
    batch.BeginVerification();Require(batch.Remaining()==0 && batch.removed==8,"later empty scan confirms all removals without resubmission");
    Plan refused{};refused.Observe(i);refused.Observe(x);refused.Submitted(0);refused.BeginVerification();
    refused.ObserveRemaining(x);Require(refused.Remaining()==0 && refused.removed==1,"unsubmitted items do not block partial-batch verification");
    Gesture g;unsigned requests=0;bool open=false;auto request=[&]() noexcept {++requests;return open;};
    Require(!g.Update(false,true,request) && requests==0,"normal L3 preserved");
    Require(!g.Update(true,true,request) && requests==1,"outside stash chord not consumed");
    g.Update(false,false,request);open=true;
    Require(g.Update(true,true,request) && requests==2,"stash chord captured once");
    Require(g.Update(true,true,request) && requests==2,"held chord does not repeat");
    open=false;Require(g.Update(false,true,request),"modifier release does not emit spurious Cube press");
    Require(!g.Update(false,false,request),"stick release clears suppression");
    std::puts("Bulk stash scope, snapshot bounds, identity and chord lifecycle passed.");
}
