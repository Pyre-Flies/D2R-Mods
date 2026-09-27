#include "shared_sdk_transfer.h"
#include "shared_sdk_selection.h"
#include <cstdio>
#include <cstdlib>
using namespace D2RL;
using namespace D2RL::Items;
using C=ItemContainer;
unsigned calls{};
uint32_t page{};
bool deposit{};
Result response=Result::Success;
uint32_t committed=1;
void Check(bool v,const char* why){if(!v){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
Result __cdecl Execute(const PluginContext*,const ExistingItemTransaction* txn,ExistingItemTransactionResult* out) noexcept {
    ++calls;
    Check(txn->player==5 && txn->operationCount==1,"one operation for captured player");
    const auto& op=*txn->operations;
    Check(op.structSize==ExistingItemOperationSize && op.structSize>=ExistingItemOperationSharedStashPageFieldEnd,"full operation includes destination page");
    Check(op.kind==ExistingItemOperationKind::Move && op.item==9,"move existing identity, no delete/recreate");
    Check(op.move.destination.structSize==ItemDestinationSize && op.move.destination.placement==Placement::Automatic,"loader owns placement");
    Check(op.move.destination.container==(deposit?C::SharedStash:C::Inventory),"correct destination");
    Check(op.move.destination.sharedStashPage==(deposit?page:UINT32_MAX),"explicit page including zero");
    out->committedOperationCount=committed;out->failureIndex=response==Result::Success?NoFailedOperation:0;return response;
}
int main(){
    PluginContext ctx{};
    ItemService items{};items.serviceVersion=1;items.serviceSize=ItemServiceSize;items.executeExistingItemTransaction=Execute;
    ItemInfo item{.structSize=ItemInfoSize};item.handle=9;item.container=C::Inventory;
    Check(!QolSharedSdk::Supported(nullptr),"missing service");
    Check(!QolSharedSdk::Move(&ctx,&items,5,item,0,true).attempted && calls==0,"absent capability retains legacy choice before mutation");
    items.capabilities=ItemServiceCapabilityBit(ItemServiceCapability::SharedStashWrite);
    items.serviceSize=ItemServiceRequiredSize;
    Check(!QolSharedSdk::Supported(&items),"old service size must not expose capability tail");
    items.serviceSize=ItemServiceSize;
    for(auto p:{0U,3U,104U}){
        page=p;deposit=true;item.container=C::Inventory;
        Check(QolSharedSdk::Move(&ctx,&items,5,item,page,true).committed,"normal shared deposit");
        deposit=false;item.container=C::SharedStash;item.sharedStashPage=page;
        Check(QolSharedSdk::Move(&ctx,&items,5,item,page,false).committed,"normal shared withdrawal");
    }
    const auto before=calls;
    item.sharedStashPage=0;
    Check(!QolSharedSdk::Move(&ctx,&items,5,item,104,false).committed && calls==before,"wrong source page never transferred");
    Check(!QolSharedSdk::Move(&ctx,&items,5,item,UINT32_MAX,false).committed && calls==before,"unknown page never becomes page zero");
    item.container=C::PersonalStash;
    Check(!QolSharedSdk::Move(&ctx,&items,5,item,0,false).committed && calls==before,"personal source cannot become shared withdrawal");
    item.container=C::SharedStash;page=0;deposit=false;
    response=Result::PolicyRejected;committed=0;
    const auto refused=QolSharedSdk::Move(&ctx,&items,5,item,0,false);
    Check(refused.attempted && !refused.committed && calls==before+1,"rejection handled once, never retry");
    response=Result::Success;
    Check(!QolSharedSdk::Move(&ctx,&items,5,item,0,false).committed,"success without committed operation is not reported moved");
    using S=QolSharedSdk::Selection;
    Check(QolSharedSdk::SameSelection(S{true,false,3},S{true,false,3}),"same selected page");
    Check(!QolSharedSdk::SameSelection(S{true,false,3},S{true,false,4}),"page changed while queued");
    Check(!QolSharedSdk::SameSelection(S{true,false,0},S{true,true,0}),"season changed");
    Check(!QolSharedSdk::SameSelection(S{},S{}),"unavailable selection never matches");
    std::puts("Shared SDK capability, identity/page, transaction and rejection checks passed.");
}
