#include "identify_action.h"
#include "identify_bulk.h"
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
int nativeDispatchResult=1;unsigned nativeDispatches=0;
int NativeDispatch(const D2RL::PluginContext*,D2RL::PlayerHandle,const Info& target,const Info& tome) noexcept {++nativeDispatches;return target.runtimeId==42 && tome.quantity==97?nativeDispatchResult:-1;}
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
    Check(t->operations[0].item==100 && t->operations[1].item==rows.back().handle,"fresh game handles used");
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
    req=Reset(0);nativeModel=true;nativeQuantity=97;nativeDispatches=0;
    r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
    Check(r.status==Status::NativePending && nativeDispatches==1 && edits==0 && transactions==0 && !r.consumed,"native dispatch uses stat70 and performs no SDK writes or premature success");
    nativeDispatchResult=-1;
    r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
    Check(r.status==Status::NativeRefused && std::strcmp(r.route,"native-tome")==0 && edits==0 && transactions==0,"native refusal does not fall through to SDK consumption");
    nativeDispatchResult=1;
    req=Reset(0);nativeModel=true;nativeQuantity=97;nativeDispatches=0;
    Info cubeTome=rows.back();cubeTome.container=Container::Cube;cubeTome.handle=202;cubeTome.runtimeId=99;cubeTome.quantity=100;
    rows.insert(rows.begin(),cubeTome);
    r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
    Check(r.status==Status::NativePending && nativeDispatches==1 && r.sourceContainer==Container::Inventory && r.tomes==1 && r.nativeReadFailures==0,"Cube tome enumerated first cannot displace inventory tome or invoke charge reader");
    req=Reset(97);rows.back().container=Container::Cube;nativeDispatches=0;
    r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
    Check(r.status==Status::NoConsumable && r.tomes==0 && nativeDispatches==0 && edits==0 && transactions==0,"Cube-only tome is unavailable without mutation");
    req=Reset(0);cubeTome=rows.back();cubeTome.container=Container::Cube;cubeTome.quantity=100;rows.insert(rows.begin(),cubeTome);
    Check(run(req).status==Status::EmptyTome && edits==0 && transactions==0,"charged Cube tome does not rescue empty inventory tome");
    for(const auto container:{Container::PersonalStash,Container::SharedStash}) {
        req=Reset(0);nativeModel=true;nativeQuantity=97;nativeDispatches=0;
        req.target.container=rows[0].container=container;
        r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
        Check(r.sourceContainer==Container::Inventory && (container==Container::PersonalStash ? r.status==Status::NativePending && nativeQuantity==97 && nativeDispatches==1 : r.status==Status::Success && nativeQuantity==96 && nativeDispatches==0),"Personal target uses native; Shared target retains existing SDK path");
    }
    req=Reset(0);nativeModel=true;nativeQuantity=97;nativeDispatches=0;
    req.personalStashOpen=true;rows.back().container=Container::PersonalStash;
    r=Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch);
    Check(r.status==Status::NativePending && r.sourceContainer==Container::PersonalStash && edits==0,"open Personal Stash tome uses native route");
    req.personalStashOpen=false;
    Check(Execute(nullptr,&items,&inv,req,ReadNativeQuantity,NativeDispatch).status==Status::NoConsumable,"closed Personal Stash excluded");
    req=Reset(97);req.personalStashOpen=true;rows.back().container=Container::SharedStash;
    Check(run(req).status==Status::NoConsumable && edits==0 && transactions==0,"Shared tome excluded");
    req=Reset(97);req.personalStashOpen=true;
    Info personalTome=rows.back();personalTome.container=Container::PersonalStash;personalTome.handle=202;personalTome.runtimeId=99;
    rows.insert(rows.begin(),personalTome);
    r=run(req);
    Check(r.status==Status::Success && r.sourceContainer==Container::Inventory,"inventory tome wins over earlier Personal tome");
    req=Reset();BulkPlan plan{};plan.requested=rows.back();
    Check(BulkTome(rows.back()),"inventory tome offers bulk action");
    for(const auto c:{Container::Cube,Container::SharedStash}) {auto other=rows.back();other.container=c;Check(!BulkTome(other),"excluded tome has no bulk shortcut");}
    plan.Observe(rows.back());plan.Observe(rows[0]);plan.Observe(rows[0]);
    auto other=rows[0];other.runtimeId=55;other.container=Container::PersonalStash;plan.Observe(other);
    other.container=Container::Cube;plan.Observe(other);
    other.container=Container::SharedStash;plan.Observe(other);
    other.container=Container::Inventory;other.stateFlags=D2RL::Items::ItemStateIdentified;plan.Observe(other);
    Check(plan.found && plan.count==1 && plan.tome.runtimeId==43,"snapshot contains only unique unidentified inventory targets and binds clicked tome");
    BulkPlan cubePlan{};cubePlan.includeCube=true;cubePlan.requested=rows.back();
    cubePlan.Observe(rows.back());cubePlan.Observe(rows.front());
    other=rows.front();other.runtimeId=56;other.handle=156;other.container=Container::Cube;
    cubePlan.Observe(other);cubePlan.Observe(other);
    Check(cubePlan.count==2 && cubePlan.found && cubePlan.targets[1].container==Container::Cube,"SDK snapshot includes unique Cube targets with inventory tome");
    auto movedCube=other;movedCube.container=Container::Inventory;
    Check(CheckBulkTarget(other,movedCube,true,5)==BulkStep::Changed,"Cube item moved after snapshot cancels rather than following it");
    other.runtimeId=57;other.stateFlags=D2RL::Items::ItemStateIdentified;cubePlan.Observe(other);
    other.stateFlags=0;other.container=Container::PersonalStash;cubePlan.Observe(other);
    other.container=Container::SharedStash;cubePlan.Observe(other);
    other=rows.back();other.container=Container::Cube;cubePlan.Observe(other);
    Check(cubePlan.count==2 && cubePlan.tome.container==Container::Inventory,"identified Cube items, stash targets and Cube tomes excluded");
    Check((cubePlan.TargetMask()&D2RL::Items::ContainerBit(Container::Cube))!=0 && (plan.TargetMask()&D2RL::Items::ContainerBit(Container::Cube))==0,"SDK enumeration includes Cube; native compatibility stays inventory-only");
    Check(CheckBulkTarget(rows[0],rows[0],true,1)==BulkStep::Identify,"last charge can identify one item");
    Check(CheckBulkTarget(rows[0],rows[0],true,0)==BulkStep::Empty,"empty tome stops batch");
    other=rows[0];++other.x;Check(CheckBulkTarget(rows[0],other,true,9)==BulkStep::Changed,"moved target cancels remaining batch");
    other=rows[0];++other.runtimeId;Check(CheckBulkTarget(rows[0],other,true,9)==BulkStep::Changed,"replacement target never identified");
    other=rows[0];other.stateFlags=D2RL::Items::ItemStateIdentified;Check(CheckBulkTarget(rows[0],other,true,0)==BulkStep::Skip,"externally identified target skips without charge");
    Check(CheckBulkTarget(rows[0],rows[0],false,9)==BulkStep::Changed,"missing target cancels batch");
    for(unsigned i=0;i<257;++i){other=rows[0];other.runtimeId=1000+i;plan.Observe(other);}
    Check(plan.overflow && plan.count==256,"oversized snapshot refuses rather than silently truncating");
    Check(CheckBulkReady(false,0)==BulkReady::Ready,"unblocked tome can start immediately");
    Check(CheckBulkReady(true,250)==BulkReady::Wait && CheckBulkReady(true,4999)==BulkReady::Wait,"blocked tome waits without another use attempt");
    Check(CheckBulkReady(false,4999)==BulkReady::Ready,"game readiness permits continuation before deadline");
    Check(CheckBulkReady(true,5000)==BulkReady::Expired && CheckBulkReady(false,5000)==BulkReady::Expired,"expired readiness window never dispatches");
    // SDK bulk uses the same charge-first operation, without rescanning or native dispatch.
    req=Reset(0);nativeModel=true;nativeQuantity=6;nativeDispatches=0;
    const auto selectedTome=rows.back();const auto firstTarget=rows.front();
    for(unsigned i=1;i<6;++i){auto extra=firstTarget;extra.handle=200+i;extra.runtimeId=200+i;if(i%2)extra.container=Container::Cube;rows.push_back(extra);}
    unsigned completed=0;
    for(const auto target:rows) {
        if(Code(target.code,D2RL::Items::MakeItemCode("ibk")))continue;
        const auto outcome=ConsumeTome(nullptr,&items,1,target,selectedTome,ReadNativeQuantity);
        if(outcome.status!=Status::Success)break;
        ++completed;
    }
    Check(completed==6 && nativeQuantity==0 && edits==12 && scans==0 && nativeDispatches==0 && destroys==0,"SDK batch consumes six charges for mixed inventory/Cube targets without native cursor or repeated scans");
    req=Reset(0);nativeModel=true;nativeQuantity=2;
    for(unsigned i=0;i<3;++i) {
        rows[0].stateFlags=0;
        const auto outcome=ConsumeTome(nullptr,&items,1,rows[0],rows[1],ReadNativeQuantity);
        Check(i<2 ? outcome.status==Status::Success : outcome.status==Status::ChargeReadFailed,"SDK batch stops at exhaustion");
    }
    Check(nativeQuantity==0 && edits==4 && !rows[0].stateFlags,"no third free identification or negative charge");
    req=Reset(0);nativeModel=true;nativeQuantity=3;mutationResult=ItemResult::PolicyRejected;
    r=ConsumeTome(nullptr,&items,1,rows[0],rows[1],ReadNativeQuantity);
    Check(r.status==Status::MutationFailed && edits==1 && nativeQuantity==3 && !rows[0].stateFlags,"rejected SDK debit never identifies or dispatches native fallback");
    req=Reset(0);nativeModel=true;nativeQuantity=3;failIdentify=true;
    r=ConsumeTome(nullptr,&items,1,rows[0],rows[1],ReadNativeQuantity);
    Check(r.status==Status::MutationFailed && nativeQuantity==3 && !rows[0].stateFlags,"bulk shares verified SDK compensation on rejected identify");
    std::puts("Identify authoritative scan, identity, consumables and failure/rollback checks passed.");
}
