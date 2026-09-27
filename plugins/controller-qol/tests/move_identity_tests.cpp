#include "move_identity.h"
#include <vector>
#include <cstdlib>
#include <cstdio>
using Info=D2RL::Items::ItemInfo;
using C=D2RL::Items::ItemContainer;
std::vector<Info> rows;
D2RL::Inventory::Result scanResult=D2RL::Inventory::Result::Success;
unsigned scans{};
void Check(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
D2RL::Items::Result __cdecl Get(const D2RL::PluginContext*,D2RL::ItemHandle h,Info* result) noexcept {
    for(const auto& row:rows)if(row.handle==h){*result=row;return D2RL::Items::Result::Success;}
    return D2RL::Items::Result::Unavailable;
}
D2RL::Inventory::Result __cdecl Scan(const D2RL::PluginContext* ctx,D2RL::PlayerHandle,const D2RL::Inventory::ItemFilter* filter,D2RL::Inventory::ItemCallback callback,void* user) noexcept {
    ++scans;Check(filter->containerMask==D2RL::Items::ContainerBit(C::PersonalStash),"only source container enumerated");
    // Deliberately include out-of-filter rows: resolver must still reject them.
    for(const auto& row:rows)if(callback(ctx,&row,user)==D2RL::Inventory::IterationAction::Stop)break;
    return scanResult;
}
int main(){
    D2RL::ItemService items{};items.getItemInfo=Get;
    D2RL::InventoryService inventory{};inventory.forEachInventoryItem=Scan;
    Info target{.structSize=D2RL::Items::ItemInfoSize};target.handle=6;target.runtimeId=50;target.code=123;target.container=C::PersonalStash;target.inventoryPage=4;target.x=5;
    auto charm=target;charm.handle=60;charm.container=C::CustomPage;charm.inventoryPage=6;
    auto cube=target;cube.handle=61;cube.container=C::Cube;
    auto wrong=target;wrong.handle=62;wrong.runtimeId=51;
    auto live=target;live.handle=63;
    rows={charm,cube,wrong,live};
    auto resolve=[&](){return QolMove::Resolve(nullptr,&items,&inventory,1,target);};
    Check(resolve()==63,"stale handle resolves exact stash item despite colliding cells/codes");
    rows={charm,cube,wrong};Check(resolve()==D2RL::InvalidItemHandle,"no cross-container or same-code fallback");
    rows={live};scanResult=D2RL::Inventory::Result::Unavailable;Check(resolve()==D2RL::InvalidItemHandle,"failed scan discards partial result");
    scanResult=D2RL::Inventory::Result::Success;rows={live};rows[0].handle=6;scans=0;Check(resolve()==6 && scans==0,"valid exact handle needs no scan");
    rows={charm,live};rows[0].handle=6;Check(resolve()==63,"reused handle cannot select charm");
    rows={live};rows[0].inventoryPage++;Check(resolve()==D2RL::InvalidItemHandle,"changed page rejected");
    Check(QolMove::CustomRoute(false,false,false),"custom page available on ordinary inventory");
    Check(!QolMove::CustomRoute(true,false,false) && !QolMove::CustomRoute(false,true,false) && !QolMove::CustomRoute(false,false,true),"stash cube and vendor take priority over custom page");
    std::puts("move identity regressions passed");
}
