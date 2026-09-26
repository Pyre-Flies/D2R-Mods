#include "belt_policy.h"
#include "belt_native_contract.h"
#include <cstdio>
#include <cstdlib>
using namespace QolBelt;
int checks{}, calls{};
void Check(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
void __fastcall Place(void* item, void* player, uint8_t page, uint8_t flag, void* state) noexcept {
    ++calls;
    Check(item != player, "item and player are distinct arguments");
    Check(page == 0 && flag == 1, "stored inventory route, not cursor route");
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
    Check(!SupportedSource(Container::CustomPage) && !SupportedSource(Container::Cube), "unsupported source routes excluded");
    for (auto code : {D2RL::Items::MakeItemCode("hp5"),D2RL::Items::MakeItemCode("mp6"),D2RL::Items::MakeItemCode("rvl"),D2RL::Items::MakeItemCode("isc")})
        Check(BeltCandidate(code), "standard, modded tier, rejuv and scroll candidates");
    Check(!BeltCandidate(D2RL::Items::MakeItemCode("r01")), "runes not potion candidates");
    std::printf("Belt contract/policy: %d checks passed.\n", checks);
}
