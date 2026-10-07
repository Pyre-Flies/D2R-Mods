#include "client_transfer_policy.h"
#include <cstdio>
#include <cstdlib>
using namespace QolClientTransfer;
void Check(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main() {
    for(uint32_t tab=0;tab<=4;++tab) {
        Check(ChooseStashRoute(tab,C::Inventory,true)==StashRoute::Advanced,"eligible inventory rune/gem/material uses advanced owner on every tab");
        Check(ChooseStashRoute(tab,C::Inventory,false)==(tab<2?StashRoute::Ordinary:StashRoute::Cube),"ineligible item preserves ordinary stash or materials-tab Cube route");
    }
    Check(ChooseStashRoute(UINT32_MAX,C::Inventory,true)==StashRoute::Refuse &&
        ChooseStashRoute(5,C::Inventory,true)==StashRoute::Refuse,"unknown tab cannot become a smart deposit");
    Check(ChooseStashRoute(3,C::SharedStash,true)==StashRoute::Refuse,"advanced proxy cannot enter inventory deposit path");
    Check(ChooseStashRoute(1,C::SharedStash,true)==StashRoute::Ordinary,"ordinary shared withdrawal never becomes a smart deposit");
    for(uint32_t tab=2;tab<=4;++tab) {
        Check(EmbeddedCube(true,tab,C::Cube) && CubeView(false,true,tab),"embedded Cube does not require standalone Cube mode");
        Check(ChooseStashRoute(tab,C::Cube,true)==StashRoute::Advanced,"eligible embedded Cube item deposits to advanced owner");
        Check(ChooseStashRoute(tab,C::Cube,false)==StashRoute::Refuse,"ineligible embedded Cube item cannot spill to Inventory");
    }
    Check(CubeView(true,false,UINT32_MAX),"standalone Cube remains usable without stash");
    Check(!CubeView(false,true,1) && !CubeView(false,true,UINT32_MAX) && !CubeView(false,false,2),"closed Cube and ordinary or unknown stash tab cannot authorize Cube source");
    Check(!EmbeddedCube(true,2,C::Inventory) && !EmbeddedCube(false,2,C::Cube),"embedded context requires actual Cube source and open stash");
    Info source{.structSize=D2RL::Items::ItemInfoSize};
    source.runtimeId=25;source.code=D2RL::Items::MakeItemCode("cap");source.classId=306;
    source.container=C::Inventory;source.x=8;source.generationSeed=123;source.itemSeed=456;
    auto live=source;live.handle=100;
    Check(Source(source,live),"fresh SDK handle retains exact identity");
    live.runtimeId++;Check(!Source(source,live),"same-code cap at same cell is not the target");
    live=source;live.itemSeed++;Check(!Source(source,live),"reused runtime ID cannot bind another item");
    live=source;live.x--;Check(!Source(source,live),"moved source cancels queued action");
    live=source;live.container=C::PersonalStash;live.x=0;live.y=2;
    Check(Confirmed(source,live,C::PersonalStash,UINT32_MAX),"personal stash confirmation allows automatic placement");
    Check(!Confirmed(source,source,C::PersonalStash,UINT32_MAX),"unchanged source is not success");
    Check(!Confirmed(source,live,C::Cube,UINT32_MAX),"stash placement cannot confirm Cube request");
    live.container=C::SharedStash;live.sharedStashPage=2;
    Check(Confirmed(source,live,C::SharedStash,2),"exact selected shared page confirms");
    Check(!Confirmed(source,live,C::SharedStash,1) && !Confirmed(source,live,C::SharedStash,UINT32_MAX),"wrong or unknown shared page cannot confirm");
    source=live;live.sharedStashPage=3;Check(!Source(source,live),"shared source changing page cancels");
    Check(NativePage(C::Inventory)==0 && NativePage(C::Cube)==3 && NativePage(C::PersonalStash)==4 && NativePage(C::SharedStash)==4,"native page ABI is distinct from SDK containers and shared page numbers");
    Check(!Legal(C::Inventory,C::Cube,D2RL::Items::MakeItemCode("box")),"Cube cannot go into itself");
    Check(!Legal(C::CustomPage,C::Inventory,source.code) && !Legal(C::Ground,C::Inventory,source.code),"advanced and ground routes excluded");
    Check(Legal(C::SharedStash,C::Inventory,source.code) && Legal(C::PersonalStash,C::Cube,source.code),"ordinary withdrawal and explicit Cube destinations supported");
    RequestFence f;f.Begin(100);const auto old=f.generation;
    Check(f.Current(old) && f.Poll(850),"initial scheduling bound inclusive");
    Check(!f.Poll(851),"delayed initial task expires before submission");
    f.Cancel();f.Begin(900);Check(!f.Current(old),"stale callback cannot run new request after session change");
    Check(f.SubmitOnce(950),"first native submission admitted");
    Check(!f.SubmitOnce(960),"native false/unconfirmed result must never resubmit");
    Check(f.Poll(3450) && !f.Poll(3451),"confirmation waits are bounded separately from queue time");
    f.Cancel();Check(!f.Current(f.generation) && !f.SubmitOnce(3500),"cancelled request cannot submit");
    f.Begin(4000);for(unsigned i=0;i<480;++i)Check(f.Poll(4000),"bounded repeated UI callback admitted");
    Check(!f.Poll(4000),"scheduler cannot spin indefinitely without advancing time");
    std::puts("remote transfer identity, destination and lifecycle regressions passed");
}
