#include "bulk_stash_policy.h"
#include "bulk_stash_input.h"
#include "native_d2r.h"
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
    Require(CheckRemote(i,&i,false,false)==RemoteStep::Ready,"unchanged remote snapshot may submit once");
    Require(CheckRemote(i,&i,true,false)==RemoteStep::Wait,"pending remote submission cannot submit again");
    Require(CheckRemote(i,&i,true,true)==RemoteStep::Rejected,"unobserved submission rejects item class instead of retrying or aborting batch");
    Require(CheckRemote(i,nullptr,false,false)==RemoteStep::Stop,"pre-submission removal cancels stale snapshot");
    Require(CheckRemote(i,nullptr,true,false)==RemoteStep::Removed,"post-submission absence permits next snapshot item");
    x=i;++x.x;Require(CheckRemote(i,&x,true,false)==RemoteStep::Stop,"manual movement during confirmation stops batch");
    Require(CheckRemote(i,&x,true,true)==RemoteStep::Stop,"manual movement at timeout still stops batch");
    x=i;++x.itemSeed;Require(CheckRemote(i,&x,false,false)==RemoteStep::Stop,"remote source substitution rejected before sending");
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
    Plan feedback{};for(unsigned n=1;n<=4;++n){x=i;x.runtimeId=n;x.itemSeed=100+n;feedback.Observe(x);}
    feedback.Submitted(0);feedback.Submitted(1);feedback.Submitted(2);
    std::array<Info,4> notifications{};unsigned notificationCount=0;
    auto notify=[&](const Info& item) noexcept {Require(notificationCount<notifications.size(),"notification bounds");notifications[notificationCount++]=item;};
    feedback.BeginVerification();feedback.ObserveRemaining(feedback.items[1]);
    feedback.ConfirmAbsent(false,notify);
    Require(notificationCount==0,"partial failed enumeration never reports absent identities");
    feedback.BeginVerification();feedback.ObserveRemaining(feedback.items[1]);feedback.ObserveRemaining(feedback.items[2]);
    feedback.ConfirmAbsent(true,notify);
    Require(notificationCount==1 && notifications[0].runtimeId==1 && notifications[0].itemSeed==101,"first confirmed removal reports copied submitted snapshot");
    feedback.ConfirmAbsent(true,notify);
    Require(notificationCount==1,"same successful observation does not duplicate feedback");
    feedback.BeginVerification();feedback.ObserveRemaining(feedback.items[2]);feedback.ConfirmAbsent(true,notify);
    Require(notificationCount==2 && notifications[1].runtimeId==2,"later successful partial observation adds only new removal");
    feedback.BeginVerification();feedback.ObserveRemaining(feedback.items[0]);feedback.ConfirmAbsent(true,notify);
    Require(notificationCount==3 && notifications[2].runtimeId==3,"reappearing prior identity cannot reset a confirmed notification");
    feedback.BeginVerification();feedback.ConfirmAbsent(true,notify);
    Require(notificationCount==3 && !feedback.notified[3],"unsubmitted absence and repeated polls never emit feedback");
    feedback.ConfirmRemoval(feedback.count,notify);feedback.ConfirmRemoval(256,notify);feedback.ConfirmRemoval(3,notify);
    Require(notificationCount==3,"remote-style confirmation refuses out of bounds and unsubmitted identities");
    Plan remoteFeedback{};remoteFeedback.Observe(i);remoteFeedback.ConfirmRemoval(0,notify);
    Require(notificationCount==3,"remote source absence before successful submission cannot report removal");
    remoteFeedback.Submitted(0);remoteFeedback.ConfirmRemoval(0,notify);remoteFeedback.ConfirmRemoval(0,notify);
    Require(notificationCount==4 && notifications[3].runtimeId==1,"remote-style confirmed removal notifies once after submission");
    // Stack capacity and in-flight classId projection (prevents full-stack batch abort and over-submission).
    Plan stackPlan{};
    for(unsigned n=1;n<=4;++n) {
        auto item=i;item.runtimeId=n;item.classId=n<=3?610u:611u;item.quantity=1;
        stackPlan.Observe(item);
    }
    Require(!Plan::HasStackRoom(99,99) && !Plan::HasStackRoom(100,99) && !Plan::HasStackRoom(0,99,true),
        "already-full advanced-stash counter has no stack room");
    Require(Plan::HasStackRoom(98,99) && Plan::HasStackRoom(99,500),
        "counters below default or custom D2RCore stack limit admit deposits");
    Require(stackPlan.EffectiveCount(610u,97u)==97u && Plan::HasStackRoom(stackPlan.EffectiveCount(610u,97u),99u),
        "first copy of classId 610 sees baseline 97 and has room");
    stackPlan.Submitted(0);
    Require(stackPlan.SubmittedForClass(610u)==1u && stackPlan.EffectiveCount(610u,97u)==98u &&
        Plan::HasStackRoom(stackPlan.EffectiveCount(610u,97u),99u),
        "second copy of classId 610 projects 98 in single-tick batch and still has room");
    stackPlan.Submitted(1);
    Require(stackPlan.SubmittedForClass(610u)==2u && stackPlan.EffectiveCount(610u,97u)==99u &&
        !Plan::HasStackRoom(stackPlan.EffectiveCount(610u,97u),99u),
        "third copy of classId 610 projects full stack 99 and is skipped before over-submitting");
    Require(stackPlan.EffectiveCount(610u,99u)==99u,
        "remote mode live counter update to 99 is not double-counted with baseline projection");
    Require(stackPlan.EffectiveCount(611u,0u)==0u && Plan::HasStackRoom(stackPlan.EffectiveCount(611u,0u),99u),
        "other material classId 611 remains eligible after classId 610 reaches max stack");
    Require(!Plan::HasStackRoom(98u,99u,false,5) && Plan::HasStackRoom(98u,99u,false,1) &&
        Plan::HasStackRoom(97u,99u,false,2) && !Plan::HasStackRoom(97u,99u,false,3) &&
        Plan::HasStackRoom(98u,99u,false,0) && Plan::HasStackRoom(98u,99u,false,-1),
        "incoming stack quantity is checked against remaining custom tab capacity");
    Plan multiStackPlan{};
    {
        auto s1=i;s1.runtimeId=10u;s1.classId=610u;s1.quantity=2;multiStackPlan.Observe(s1);
        auto s2=i;s2.runtimeId=11u;s2.classId=610u;s2.quantity=5;multiStackPlan.Observe(s2);
        auto s3=i;s3.runtimeId=12u;s3.classId=610u;s3.quantity=1;multiStackPlan.Observe(s3);
    }
    Require(Plan::HasStackRoom(multiStackPlan.EffectiveCount(610u,96u),99u,false,multiStackPlan.items[0].quantity),
        "quantity-2 stack against 96/99 is allowed");
    multiStackPlan.Submitted(0);
    Require(multiStackPlan.EffectiveCount(610u,96u)==98u &&
        !Plan::HasStackRoom(multiStackPlan.EffectiveCount(610u,96u),99u,false,multiStackPlan.items[1].quantity) &&
        Plan::HasStackRoom(multiStackPlan.EffectiveCount(610u,96u),99u,false,multiStackPlan.items[2].quantity),
        "after quantity-2 stack brings effective count to 98/99, quantity-5 stack is refused while quantity-1 stack is allowed");
    Plan rejectedPlan{};
    {
        auto r1=i;r1.runtimeId=20u;r1.classId=610u;r1.quantity=1;rejectedPlan.Observe(r1);
        auto r2=i;r2.runtimeId=21u;r2.classId=610u;r2.quantity=1;rejectedPlan.Observe(r2);
        auto r3=i;r3.runtimeId=22u;r3.classId=611u;r3.quantity=1;rejectedPlan.Observe(r3);
    }
    rejectedPlan.Submitted(0);
    rejectedPlan.RejectSubmitted(0);
    Require(rejectedPlan.submitted==0u && rejectedPlan.skipped==1u &&
        rejectedPlan.SubmittedForClass(610u)==0u &&
        rejectedPlan.IsClassRejected(610u,1) && !rejectedPlan.IsClassRejected(611u,1),
        "server rejection of a capped item class skips remaining copies of that class while allowing other item classes");
    Require(!Plan::IsServerCapCandidate(10u,500u,1) && !Plan::IsServerCapCandidate(98u,500u,1) &&
        Plan::IsServerCapCandidate(99u,500u,1) && Plan::IsServerCapCandidate(97u,500u,3) &&
        !Plan::IsServerCapCandidate(99u,99u,1) && !Plan::IsServerCapCandidate(500u,500u,1),
        "remote timeout only qualifies as a server-cap rejection when local limit exceeds default 99 and deposit crosses 99");
    Require(Plan::ServerCapRejectedQuantity(99u)==1u && Plan::ServerCapRejectedQuantity(120u)==1u &&
        Plan::ServerCapRejectedQuantity(97u)==3u,
        "server-cap rejection threshold records the exact overflow quantity relative to default 99");
    Plan multiStackRejected{};
    {
        auto m1=i;m1.runtimeId=30u;m1.classId=610u;m1.quantity=5;multiStackRejected.Observe(m1);
        auto m2=i;m2.runtimeId=31u;m2.classId=610u;m2.quantity=1;multiStackRejected.Observe(m2);
    }
    multiStackRejected.Submitted(0);
    multiStackRejected.RejectSubmitted(0,static_cast<int32_t>(Plan::ServerCapRejectedQuantity(99u)));
    Require(multiStackRejected.IsClassRejected(610u,1),
        "rejecting a quantity-5 stack at 99/99 also rejects subsequent quantity-1 stacks of that class");
    {
        auto* fakeImage=static_cast<unsigned char*>(
            VirtualAlloc(nullptr,D2R::Native::FindAdvancedStashCounterRva+4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        Require(fakeImage!=nullptr,"synthetic native image allocated");
        const auto fakeBase=reinterpret_cast<uintptr_t>(fakeImage);
        uint32_t fakeItem[4]{4u,610u,1u,0u};
        uint32_t fakePlayer[4]{0u,0u,1u,0u};
        Require(!D2R::Native::HasVerifiedAdvancedStashCounterContract(fakeBase) &&
            !D2R::Native::DepositToAdvancedStash(fakeBase,fakeItem,fakePlayer,0,1),
            "unwitnessed exeBase refuses DepositToAdvancedStash before calling native functions");
        std::memcpy(fakeImage+D2R::Native::FindAdvancedStashCounterRva,
            D2R::Native::FindAdvancedStashCounterWitness,
            sizeof(D2R::Native::FindAdvancedStashCounterWitness));
        std::memcpy(fakeImage+D2R::Native::GetAdvancedStashCounterByteRva,
            D2R::Native::GetAdvancedStashCounterByteWitness,
            sizeof(D2R::Native::GetAdvancedStashCounterByteWitness));
        Require(D2R::Native::HasVerifiedAdvancedStashCounterContract(fakeBase),
            "exact FindAdvancedStashCounter + GetAdvancedStashCounterByte witnesses admit counter contract");
        Require(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase)==0,
            "unwitnessed D2RCore base falls back safely to 0");
        std::memcpy(fakeImage+D2R::Native::CoreGetMaterialsLimitRva,
            D2R::Native::CoreMaterialsLimitWitness,
            sizeof(D2R::Native::CoreMaterialsLimitWitness));
        std::memcpy(fakeImage+D2R::Native::CoreGetCounterFullStatusRva,
            D2R::Native::CoreCounterFullWitness,
            sizeof(D2R::Native::CoreCounterFullWitness));
        Require(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase)==fakeBase,
            "exact D2RCore witnesses admit custom stack-limit queries");
        fakeImage[D2R::Native::CoreGetCounterFullStatusRva]^=0xFFu;
        Require(D2R::Native::VerifiedAdvancedStashCoreBase(fakeBase)==0,
            "mutated D2RCore byte immediately rejects custom stack-limit queries");
        VirtualFree(fakeImage,0,MEM_RELEASE);
    }
    Gesture g;unsigned requests=0;bool open=false;auto request=[&]() noexcept {++requests;return open;};
    Require(!g.Update(false,true,request) && requests==0,"normal L3 preserved");
    Require(!g.Update(true,true,request) && requests==1,"outside stash chord not consumed");
    g.Update(false,false,request);open=true;
    Require(g.Update(true,true,request) && requests==2,"stash chord captured once");
    Require(g.Update(true,true,request) && requests==2,"held chord does not repeat");
    open=false;Require(g.Update(false,true,request),"modifier release does not emit spurious Cube press");
    Require(!g.Update(false,false,request),"stick release clears suppression");
    std::puts("Bulk stash scope, snapshot bounds, identity, stack capacity, removal observation and chord lifecycle passed.");
}
