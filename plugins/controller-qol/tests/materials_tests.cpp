#include "materials_policy.h"
#include "shared_item_policy.h"
#include "vendor_policy.h"
#include "materials_native_contract.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace QolMaterials;
int checks{},calls{},buyCalls{};
void Check(bool value,const char* why) { ++checks; if (!value) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
void __fastcall Widget(void* widget,const int32_t* cell,uint8_t page) {
    ++calls; Check(widget && cell,"valid widget and synchronous coordinate storage");
    Check(cell[0]==0 && cell[1]==0 && page==0,"native inventory page must be zero, not destination enum one");
}
void __fastcall One(void* item,void* owner,uint8_t destination) {
    ++calls; Check(item && owner && item!=owner,"proxy and stash owner are distinct");
    Check(destination==3,"advanced-stash request must select belt directly");
}
void __fastcall Sell(void* panel,void* player,void* item,bool sell,bool immediate,bool shift) {
    Check(panel && player && item && item!=player,"native sell receives exact item and local player");
    Check(sell && immediate && !shift,"native quick-sell flags match reviewed caller");
}
void __fastcall Buy(void* panel,void* player,void* item,bool sell,bool immediate,bool shift) {
    ++buyCalls;
    Check(panel && player && item && item!=player,"native purchase keeps stock distinct from player");
    Check(!sell && immediate && shift,"bulk purchase must force Shift without using sell flags");
}
int main() {
    using C=D2RL::Items::ItemContainer;
    int panel{},player{},saleItem{};
    const auto beforeBuy=buyCalls;
    Check(QolVendor::SubmitBuy(Buy,&panel,&player,&saleItem),"native bulk-buy invocation");
    Check(buyCalls==beforeBuy+1,"exactly one bulk request, no purchase loop");
    Check(!QolVendor::SubmitBuy(Buy,nullptr,&player,&saleItem) &&
          !QolVendor::SubmitBuy(Buy,&panel,nullptr,&saleItem) &&
          !QolVendor::SubmitBuy(Buy,&panel,&player,nullptr) &&
          !QolVendor::SubmitBuy(nullptr,&panel,&player,&saleItem),"missing native purchase dependencies refused");
    Check(buyCalls==beforeBuy+1,"refused requests never buy");
    Check(QolVendor::Submit(Sell,&panel,&player,&saleItem),"native sell submitted once");
    Check(!QolVendor::Submit(Sell,nullptr,&player,&saleItem),"missing vendor cannot submit");
    Check(QolVendor::Context(true,false,false,C::Inventory),"inventory sale in shop");
    Check(!QolVendor::Context(false,false,false,C::Inventory) && !QolVendor::Context(true,true,false,C::Inventory) &&
          !QolVendor::Context(true,false,true,C::Inventory) && !QolVendor::Context(true,false,false,C::SharedStash),"sale excludes dialogue-only, stash, Cube and non-inventory source");
    D2RL::Items::ItemInfo focus{}; focus.container=C::SharedStash; focus.runtimeId=105; focus.code=123; focus.classId=7; focus.sharedStashPage=104;
    auto candidate=focus; candidate.sharedStashPage=0;
    Check(!QolShared::SameItem(focus,candidate),"same cell/code on page 1 cannot replace page 105");
    candidate=focus; candidate.runtimeId=106;
    Check(!QolShared::SameItem(focus,candidate),"duplicate code on selected page cannot replace exact item");
    candidate=focus; candidate.handle=900;
    Check(QolShared::SameItem(focus,candidate),"new game-thread handle for same page and runtime identity accepted");
    candidate.container=C::PersonalStash;
    Check(!QolShared::SameItem(focus,candidate),"Personal item never substitutes for Shared");
    candidate=focus; candidate.sharedStashPage=UINT32_MAX;
    Check(!QolShared::SameItem(focus,candidate),"known Shared page cannot match unknown candidate page");
    focus.sharedStashPage=UINT32_MAX;
    Check(QolShared::SameItem(focus,candidate),"unknown UI page still requires exact runtime identity");
    Check(!AdvancedProxyCandidate(C::Cube) && !AdvancedProxyCandidate(C::Inventory) && !AdvancedProxyCandidate(C::PersonalStash),"ordinary grids bypass advanced handler");
    Check(AdvancedProxyCandidate(C::Cursor) && AdvancedProxyCandidate(C::SharedStash),"observed advanced proxy containers retained");
    Check(UseMaterialsRoute(2,C::Inventory) && UseMaterialsRoute(3,C::Inventory),"Gems/Materials inventory action uses shared Cube/storage route without history");
    Check(!UseMaterialsRoute(4,C::Inventory) && !UseMaterialsRoute(3,C::Cube) && !UseMaterialsRoute(2,C::Cube) && !UseMaterialsRoute(1,C::Inventory) && !UseMaterialsRoute(0xFFFFFFFF,C::Inventory),"other tabs and Cube withdrawal do not enter storage deposit");
    Check(!UseStashDeposit(C::Inventory,C::Cube,true),"open stash cannot hijack explicit Cube destination");
    Check(UseStashDeposit(C::Inventory,C::PersonalStash,true) && !UseStashDeposit(C::Cube,C::Inventory,true),"stash deposit limited to requested stash route");
    int item{},owner{};
    Check(SubmitInventory(Widget,&item),"inventory submission");
    Check(SubmitBelt(One,&item,&owner),"belt submission");
    Check(calls==2,"one native action per request");
    Check(!SubmitInventory(nullptr,&item) && !SubmitInventory(Widget,nullptr),"invalid inventory calls refused");
    Check(!SubmitBelt(One,&item,nullptr) && !SubmitBelt(nullptr,&item,&owner),"invalid belt calls refused");
    Check(calls==2,"refusal sends no request");
    Check(Category(0)==nullptr && Category(1)==nullptr && Category(5)==nullptr,"ordinary/unknown tabs excluded");
    Check(std::strcmp(Category(2),"advancedstash_gems")==0,"gems layout mapping");
    Check(std::strcmp(Category(3),"advancedstash_materials")==0,"materials layout mapping");
    Check(std::strcmp(Category(4),"advancedstash_runes")==0,"runes layout mapping");
    Check(RejuvenationCode(0)==D2RL::Items::MakeItemCode("rvl"),"full rejuvenation first");
    Check(RejuvenationCode(1)==D2RL::Items::MakeItemCode("rvs"),"small rejuvenation fallback");
    Check(!Confirmed(4,4) && !Confirmed(4,3),"unchanged/decreased destination never confirms");
    Check(Confirmed(4,5),"destination increase permits next request");
    Check(CanContinue(15) && !CanContinue(16),"batch bounded by maximum belt slots");
    Check(RefillCode(D2RL::Items::MakeItemCode("rvs"),0)==D2RL::Items::MakeItemCode("rvs"),"small highlighted potion wins over full default");
    Check(RefillCode(D2RL::Items::MakeItemCode("rvl"),1)==D2RL::Items::MakeItemCode("rvl"),"focused refill remains exact type");
    Check(!RefillFallback(D2RL::Items::MakeItemCode("rvs"),0),"focused stock exhaustion cannot spend other potion");
    Check(RefillFallback(0,0) && !RefillFallback(0,1),"unfocused legacy refill retains bounded fallback");
    char name[5]{};
    Check(WidgetName(D2RL::Items::MakeItemCode("r22"),name) && std::strcmp(name,"r22")==0,"rune code becomes actual widget name");
    Check(WidgetName(D2RL::Items::MakeItemCode("rvl"),name) && std::strcmp(name,"rvl")==0,"padded potion code parsed");
    Check(!WidgetName(D2RL::Items::MakeItemCode("../"),name),"invalid widget code rejected");
    Check(!WidgetName(D2RL::Items::MakeItemCode(" "),name),"empty widget code rejected");
    std::printf("Materials native/policy: %d checks passed.\n",checks);
}
