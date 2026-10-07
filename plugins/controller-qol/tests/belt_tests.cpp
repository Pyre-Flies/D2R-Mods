#include "belt_policy.h"
#include "belt_native_contract.h"
#include <cstdio>
#include <cstdlib>
using namespace QolBelt;
int checks{}, calls{};
uint8_t expectedPage{};
void Check(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
void __fastcall Place(void* item, void* player, uint8_t page, uint8_t flag, void* state) noexcept {
    ++calls;
    Check(item != player, "item and player are distinct arguments");
    Check(page == expectedPage && flag == 1, "source page preserved on stored route");
    auto* placement = static_cast<D2R::Native::BeltPlacementState*>(state);
    Check(placement && placement->targetSlot == 7 && placement->engaged == 1, "validated slot and optional engagement passed");
    placement->engaged = 0; // Native function clears it; this is NOT a failure/success signal.
}
bool __fastcall SharedTransfer(void* item,void* owner,uint8_t dest,uint8_t source,bool flag,void* state) noexcept {
    Check(item && owner && item!=owner,"Shared transfer keeps item and destination owner distinct");
    Check(dest==4 && source==0 && flag,"Shared deposit uses native stash page 4, not category index 1");
    Check(state && static_cast<unsigned char*>(state)[8]==0,"automatic placement starts disengaged");
    return true;
}
bool __fastcall SharedWithdrawal(void* item,void* player,uint8_t dest,uint8_t source,bool flag,void* state) noexcept {
    Check(item && player && item!=player,"withdrawal passes distinct item and local player");
    Check(dest==0 && source==4 && flag,"Shared withdrawal reverses deposit native pages 4 -> 0");
    Check(state && static_cast<unsigned char*>(state)[8]==0,"withdrawal requests automatic placement");
    return true;
}
int main() {
    int sharedItem{},sharedOwner{};
    Check(D2R::Native::SubmitSharedWithdrawal(SharedWithdrawal,&sharedItem,&sharedOwner),"Shared withdrawal contract");
    Check(!D2R::Native::SubmitSharedWithdrawal(SharedWithdrawal,&sharedItem,nullptr),"missing withdrawal player refused");
    Check(D2R::Native::SubmitSharedTransfer(SharedTransfer,&sharedItem,&sharedOwner),"Shared native transfer contract");
    Check(!D2R::Native::SubmitSharedTransfer(SharedTransfer,&sharedItem,nullptr),"missing Shared owner refused");
    int item{}, player{};
    Check(SubmitStoredPlacement(Place, &item, &player, 7), "valid stored-item request submits");
    Check(calls == 1, "only one native placement invocation");
    for (int slot : {-1, 16, 100}) Check(!SubmitStoredPlacement(Place, &item, &player, slot), "invalid slots never submit");
    Check(!SubmitStoredPlacement(nullptr, &item, &player, 7), "missing function never submits");
    Check(!SubmitStoredPlacement(Place, nullptr, &player, 7), "missing item never submits");
    Check(calls == 1, "refusals leave native game untouched");
    for(uint8_t page:{3,4}) {
        expectedPage=page;
        Check(SubmitStoredPlacement(Place,&item,&player,7,page),"Cube and ordinary stash use native stored placement");
    }
    for(uint8_t page:{1,2,255})Check(!SubmitStoredPlacement(Place,&item,&player,7,page),"unsupported native page refused");
    Check(calls==3,"page refusals do not send requests");
    D2RL::Items::ItemInfo a{};
    a.runtimeId=42; a.code=D2RL::Items::MakeItemCode("hp5"); a.itemSeed=123;
    a.container=Container::Inventory; a.inventoryPage=0; a.x=3; a.y=2;
    auto b=a; b.handle=999;
    Check(SameSource(a,b), "identity survives UI/game handle change");
    b.runtimeId=43;
    Check(!SameItem(a,b), "same code/cell cannot substitute different potion");
    b=a; ++b.classId;
    Check(!SameItem(a,b), "different item class rejected");
    b=a; ++b.itemSeed;
    Check(SameItem(a,b), "PRNG state is not a stable identity contract");
    b=a; b.container=Container::PersonalStash;
    Check(!SameSource(a,b), "user movement cancels stale source");
    b=a; ++b.sharedStashPage;
    Check(!SameSource(a,b), "different stash page rejected");
    Check(Observe(Phase::AwaitBelt,Container::Inventory,true,false)==Observation::Wait, "returning from native action does not confirm");
    Check(Observe(Phase::AwaitBelt,Container::Belt,true,false)==Observation::Confirmed, "SDK belt observation confirms");
    Check(Observe(Phase::AwaitBelt,Container::Inventory,true,true)==Observation::TimedOut, "rejected packet stops after deadline");
    Check(Observe(Phase::AwaitInventory,Container::PersonalStash,true,false)==Observation::Wait, "stash dispatch cannot immediately place");
    Check(Observe(Phase::AwaitInventory,Container::Inventory,true,false)==Observation::Confirmed, "observed withdrawal permits placement");
    Check(Observe(Phase::AwaitBelt,Container::Unknown,false,false)==Observation::Wait, "temporary handle absence is not success");
    Check(Observe(Phase::AwaitBelt,Container::Unknown,false,true)==Observation::TimedOut, "missing item cannot retry forever");
    Check(Observe(Phase::AwaitBelt,Container::Cursor,true,false)==Observation::Invalid, "manual cursor movement cancels");
    Check(!SupportedSource(Container::CustomPage) && SupportedSource(Container::Cube), "ordinary Cube supported without admitting custom pages");
    Check(ClientBeltSource(Container::Inventory,0),"remote stored inventory may request belt placement");
    Check(ClientBeltSource(Container::PersonalStash,0) && ClientBeltSource(Container::SharedStash,1) && ClientBeltSource(Container::Cube,3),"remote stored sources admitted for guarded native placement");
    Check(!ClientBeltSource(Container::Inventory,3) && !ClientBeltSource(Container::Cursor,0),"wrong page and cursor rejected remotely");
    for (auto code : {D2RL::Items::MakeItemCode("hp5"),D2RL::Items::MakeItemCode("mp6"),D2RL::Items::MakeItemCode("rvl"),D2RL::Items::MakeItemCode("yps")})
        Check(BeltCandidate(code), "standard, modded tier, rejuv and utility potion candidates");
    for(auto code:{D2RL::Items::MakeItemCode("isc"),D2RL::Items::MakeItemCode("tsc"),D2RL::Items::MakeItemCode("ibk"),D2RL::Items::MakeItemCode("tbk")})
        Check(!BeltCandidate(code),"scrolls and tomes must never enter potion refill");
    Check(!BeltCandidate(D2RL::Items::MakeItemCode("r01")), "runes not potion candidates");
    const auto potion=D2RL::Items::MakeItemCode("rvl");
    Check(SharedInputCandidate(Container::SharedStash,1,potion),"normal Shared potion may use early A handler");
    Check(!SharedInputCandidate(Container::SharedStash,3,potion) && !SharedInputCandidate(Container::SharedStash,UINT32_MAX,potion),"advanced and unknown tabs cannot use ordinary Shared belt handler");
    Check(!SharedInputCandidate(Container::Inventory,1,potion) && !SharedInputCandidate(Container::Cube,1,potion),"other stored item input paths retain their handlers");
    Check(!SharedInputCandidate(Container::SharedStash,1,D2RL::Items::MakeItemCode("isc")),"Shared scroll cannot enter potion handler");
    a.container=Container::SharedStash;a.sharedStashPage=UINT32_MAX;
    b=a;b.sharedStashPage=2;b.inventoryPage=1;
    Check(!SameSource(a,b) && BindShared(a,b,2,true),"initial binding canonicalizes tooltip metadata only with exact selected identity");
    Check(!BindShared(a,b,2,false) && !BindShared(a,b,3,true),"empty focus or wrong selected page cannot bind");
    ++b.runtimeId;Check(!BindShared(a,b,2,true),"same-code replacement cannot bind");
    b=a;b.container=Container::Belt;Check(!BindShared(a,b,0,true),"already-moved item cannot rebind as Shared source");
    std::printf("Belt contract/policy: %d checks passed.\n", checks);
}
