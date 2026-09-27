#include "identify_action.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace QolIdentify;
std::vector<Info> rows;
ScanResult scanResult=ScanResult::Success;
ItemResult mutationResult=ItemResult::Success,destroyResult=ItemResult::Success;
unsigned scans{},edits{},transactions{},destroys{};
bool failIdentify{};
int32_t nativeQuantity{};
bool nativeModel{},nativeReadFails{},ignoreQuantityEdit{},failRestore{};
bool ReadNativeQuantity(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle handle,int32_t& value) noexcept {
    if(nativeReadFails || handle!=101)return false;
    value=nativeQuantity;return true;
}
void Check(bool b,const char* why) {if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
ScanResult __cdecl Player(const D2RL::PluginContext*,D2RL::PlayerHandle* p) noexcept {*p=1;return ScanResult::Success;}
ScanResult __cdecl Enumerate(const D2RL::PluginContext* ctx,D2RL::PlayerHandle,const D2RL::Inventory::ItemFilter* filter,D2RL::Inventory::ItemCallback cb,void* data) noexcept {
    ++scans;
    for(const auto& row:rows)if(filter->containerMask&D2RL::Items::ContainerBit(row.container))
        if(cb(ctx,&row,data)==D2RL::Inventory::IterationAction::Stop)break;
    return scanResult;
}
ItemResult __cdecl Edit(const D2RL::PluginContext*,D2RL::PlayerHandle,D2RL::ItemHandle h,const D2RL::Items::ItemEdit* e) noexcept {
    ++edits;
    if(mutationResult!=ItemResult::Success)return mutationResult;
    for(auto& row:rows)if(row.handle==h) {
        if(e->fields&D2RL::Items::EditFieldBit(D2RL::Items::EditField::Quantity)) {
            if(nativeModel) {
                if(failRestore && e->quantity>static_cast<uint32_t>(nativeQuantity))return ItemResult::PolicyRejected;
                if(!ignoreQuantityEdit)nativeQuantity=static_cast<int32_t>(e->quantity);
            } else row.quantity=static_cast<int32_t>(e->quantity);
        }
        if(e->fields&D2RL::Items::EditFieldBit(D2RL::Items::EditField::Identified)) {
            if(failIdentify && e->stateFlags)return ItemResult::PolicyRejected;
            row.stateFlags=e->stateFlags;
        }
    }
    return ItemResult::Success;
}
ItemResult __cdecl Destroy(const D2RL::PluginContext*,D2RL::PlayerHandle,D2RL::ItemHandle,D2RL::Items::SocketedItemPolicy) noexcept {++destroys;return destroyResult;}
ItemResult __cdecl Transaction(const D2RL::PluginContext*,const D2RL::Items::ExistingItemTransaction* t,D2RL::Items::ExistingItemTransactionResult*) noexcept {
    ++transactions;
    Check(t->operationCount==2,"identify and debit are one transaction");
    Check(t->operations[0].kind==D2RL::Items::ExistingItemOperationKind::Edit && t->operations[1].kind==D2RL::Items::ExistingItemOperationKind::Debit,"operation types");
    Check(t->operations[0].item==rows[0].handle && t->operations[1].item==rows.back().handle,"fresh game handles used");
    return mutationResult;
}
Request Reset(int quantity=3) {
    nativeModel=nativeReadFails=ignoreQuantityEdit=failRestore=false;nativeQuantity=0;
    scans=edits=transactions=destroys=0;scanResult=ScanResult::Success;mutationResult=destroyResult=ItemResult::Success;failIdentify=false;
    Info target{.structSize=D2RL::Items::ItemInfoSize};target.handle=100;target.runtimeId=42;target.code=D2RL::Items::MakeItemCode("axe");target.container=Container::Inventory;target.x=2;target.y=3;
    Info tome=target;tome.handle=101;tome.runtimeId=43;tome.code=D2RL::Items::MakeItemCode("ibk");tome.quantity=quantity;tome.x=5;
    rows={target,tome};target.handle=10; // UI handle differs; runtime identity remains exact.
    return {1,target,true,true};
}
int main() {
    D2RL::InventoryService inv{};inv.getLocalPlayer=Player;inv.forEachInventoryItem=Enumerate;
    D2RL::ItemService items{};items.editItem=Edit;items.destroyItem=Destroy;items.executeExistingItemTransaction=Transaction;
    auto run=[&](const Request& r){return Execute(nullptr,&items,&inv,r);};
    auto req=Reset();Check(run(req).status==Status::Success && scans==1 && transactions==1 && edits==0,"stale UI handle resolved in single authoritative scan");
    req=Reset();rows[0].runtimeId=99;Check(run(req).status==Status::TargetMissing && transactions==0,"same code and cell cannot select a different item");
    req=Reset();scanResult=ScanResult::Unavailable;Check(run(req).status==Status::ScanFailed && transactions==0,"partial enumeration failure must not mutate");
    req=Reset(0);Check(run(req).status==Status::EmptyTome && transactions==0,"empty authoritative tome has distinct refusal");
    req=Reset();rows.pop_back();Check(run(req).status==Status::NoConsumable,"missing supply distinguished from empty tome");
    req=Reset();rows.back().code&=0xFFFFFF;Check(run(req).status==Status::Success,"NUL-padded tome recognized");
    req=Reset();rows.back().code^=0x01000000;Check(run(req).status==Status::NoConsumable,"unknown padding not normalized blindly");
    req=Reset();rows[0].stateFlags=D2RL::Items::ItemStateIdentified;Check(run(req).status==Status::AlreadyIdentified && transactions==0,"other plugin already identified target: no debit");
    req=Reset();mutationResult=ItemResult::NotAuthoritative;Check(run(req).status==Status::MutationFailed && edits==0,"failed atomic transaction has no raw native bypass");
    req=Reset(1);Check(run(req).status==Status::Success && rows.back().quantity==0 && destroys==0,"last tome charge preserves empty book");
    req=Reset(1);failIdentify=true;Check(run(req).status==Status::MutationFailed && rows.back().quantity==1,"restore final tome charge if identify rejected");
    req=Reset(1);rows.back().code=D2RL::Items::MakeItemCode("isc");destroyResult=ItemResult::PolicyRejected;Check(run(req).status==Status::MutationFailed && !rows[0].stateFlags,"restore unidentified state if scroll consumption rejected");
    req=Reset();req.consume=false;Check(run(req).status==Status::Success && edits==1 && transactions==0,"consume setting honored");
    req=Reset();req.requireConsumable=false;rows.pop_back();Check(run(req).status==Status::Success,"explicit free-identify option retained");
    req=Reset();req.player=2;Check(run(req).status==Status::PlayerUnavailable && scans==0,"changed player rejected");
    auto nativeRun=[&](const Request& r){return Execute(nullptr,&items,&inv,r,ReadNativeQuantity);};
    req=Reset(0);nativeModel=true;nativeQuantity=97;
    auto r=nativeRun(req);Check(r.status==Status::Success && nativeQuantity==96 && r.remainingQuantity==96 && r.quantityMismatches==1 && transactions==0,"stat70 charged book works despite zero SDK quantity");
    req=Reset(-1);nativeModel=true;nativeQuantity=1;
    Check(nativeRun(req).status==Status::Success && nativeQuantity==0 && destroys==0,"final native charge preserves book");
    req=Reset(97);nativeModel=true;nativeQuantity=0;
    Check(nativeRun(req).status==Status::EmptyTome && edits==0,"native empty overrides positive snapshot");
    req=Reset(0);nativeModel=true;nativeQuantity=97;ignoreQuantityEdit=true;
    Check(nativeRun(req).status==Status::ChargeVerificationFailed && !rows[0].stateFlags,"no identification if SDK edit did not debit stat70");
    req=Reset(0);nativeModel=true;nativeQuantity=97;failIdentify=true;
    Check(nativeRun(req).status==Status::MutationFailed && nativeQuantity==97,"failed identify restores native charges via SDK");
    req=Reset(0);nativeModel=true;nativeQuantity=97;failIdentify=true;failRestore=true;
    Check(nativeRun(req).status==Status::RollbackFailed,"failed compensation reported explicitly");
    req=Reset(97);nativeReadFails=true;
    Check(nativeRun(req).status==Status::ChargeReadFailed && edits==0,"unavailable native read is not a free charge");
    std::puts("Identify authoritative scan, identity, consumables and failure/rollback checks passed.");
}
