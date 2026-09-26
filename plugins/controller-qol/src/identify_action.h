#pragma once
#include <D2RLPlugin/api.h>
namespace QolIdentify {
using Info=D2RL::Items::ItemInfo;
using Container=D2RL::Items::ItemContainer;
using ItemResult=D2RL::Items::Result;
using ScanResult=D2RL::Inventory::Result;
enum class Status { Success, ServicesUnavailable, PlayerUnavailable, ScanFailed, TargetMissing, AlreadyIdentified, NoConsumable, EmptyTome, MutationFailed, RollbackFailed };
struct Request { D2RL::PlayerHandle player{}; Info target{}; bool requireConsumable{true},consume{true}; };
struct Result {
    Status status{Status::ServicesUnavailable};
    ScanResult scan{ScanResult::Unavailable};
    ItemResult operation{ItemResult::Unavailable};
    unsigned scanned{},tomes{},scrolls{};
    int32_t maxTomeQuantity{};
    bool consumed{};
};
inline bool Code(uint32_t code,uint32_t expected) noexcept {
    // Recognize only the documented space padding or equivalent NUL padding.
    return code==expected || (code==(expected&0x00FFFFFF));
}
inline bool Same(const Info& a,const Info& b) noexcept {
    return a.runtimeId==b.runtimeId && a.code==b.code && a.container==b.container &&
        a.inventoryPage==b.inventoryPage && a.x==b.x && a.y==b.y &&
        (a.container!=Container::SharedStash || a.sharedStashPage==b.sharedStashPage);
}
struct Scan {
    const Request* request{};
    Result result{};
    Info target{},tome{},scroll{};
    bool found{};
};
inline D2RL::Inventory::IterationAction __cdecl Visit(const D2RL::PluginContext*,const Info* item,void* user) noexcept {
    auto& s=*static_cast<Scan*>(user);
    if(!item) return D2RL::Inventory::IterationAction::Continue;
    ++s.result.scanned;
    if(Same(*item,s.request->target)) {s.target=*item;s.found=true;}
    if(item->container==Container::Inventory || item->container==Container::Cube) {
        if(Code(item->code,D2RL::Items::MakeItemCode("ibk"))) {
            ++s.result.tomes;
            if(item->quantity>s.result.maxTomeQuantity)s.result.maxTomeQuantity=item->quantity;
            if(item->quantity>0 && !s.tome.handle)s.tome=*item;
        } else if(Code(item->code,D2RL::Items::MakeItemCode("isc"))) {
            ++s.result.scrolls;
            if(!s.scroll.handle)s.scroll=*item;
        }
    }
    // Once both exact identity and the preferred charged tome are known, stop.
    return s.found && s.tome.handle ? D2RL::Inventory::IterationAction::Stop : D2RL::Inventory::IterationAction::Continue;
}
inline ItemResult SetIdentified(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,D2RL::PlayerHandle player,D2RL::ItemHandle item,bool value) noexcept {
    D2RL::Items::ItemEdit edit{};
    edit.structSize=D2RL::Items::ItemEditSize;
    edit.fields=D2RL::Items::EditFieldBit(D2RL::Items::EditField::Identified);
    edit.stateFlags=value?D2RL::Items::ItemStateIdentified:0;
    return items->editItem(ctx,player,item,&edit);
}
inline ItemResult SetQuantity(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,D2RL::PlayerHandle player,D2RL::ItemHandle item,uint32_t quantity) noexcept {
    D2RL::Items::ItemEdit edit{};
    edit.structSize=D2RL::Items::ItemEditSize;
    edit.fields=D2RL::Items::EditFieldBit(D2RL::Items::EditField::Quantity);
    edit.quantity=quantity;
    return items->editItem(ctx,player,item,&edit);
}
// One authoritative game-thread call. No per-item logs, raw native mutation,
// cross-thread consumable preflight or same-code/coordinate-only fallback.
inline Result Execute(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,const D2RL::InventoryService* inventory,const Request& req) noexcept {
    Scan s{};s.request=&req;
    if(!items || !inventory || !items->editItem || !items->executeExistingItemTransaction || !items->destroyItem || !inventory->getLocalPlayer || !inventory->forEachInventoryItem)return s.result;
    D2RL::PlayerHandle player{};
    if(inventory->getLocalPlayer(ctx,&player)!=ScanResult::Success || !player || player!=req.player) {s.result.status=Status::PlayerUnavailable;return s.result;}
    D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,
        D2RL::Items::ContainerBit(req.target.container)|D2RL::Items::ContainerBit(Container::Inventory)|D2RL::Items::ContainerBit(Container::Cube),0};
    s.result.scan=inventory->forEachInventoryItem(ctx,player,&filter,Visit,&s);
    if(s.result.scan!=ScanResult::Success) {s.result.status=Status::ScanFailed;return s.result;}
    if(!s.found) {s.result.status=Status::TargetMissing;return s.result;}
    if(s.target.stateFlags&D2RL::Items::ItemStateIdentified) {s.result.status=Status::AlreadyIdentified;return s.result;}
    const Info& supply=s.tome.handle?s.tome:s.scroll;
    if(req.requireConsumable && !supply.handle) {s.result.status=s.result.tomes?Status::EmptyTome:Status::NoConsumable;return s.result;}
    s.result.status=Status::MutationFailed;
    if(!req.consume || !supply.handle) {
        s.result.operation=SetIdentified(ctx,items,player,s.target.handle,true);
    } else if(supply.quantity>1) {
        // Debit requires positive remainder: combine it with the identify edit.
        D2RL::Items::ExistingItemOperation ops[2]{};
        ops[0].structSize=ops[1].structSize=D2RL::Items::ExistingItemOperationSize;
        ops[0].kind=D2RL::Items::ExistingItemOperationKind::Edit;ops[0].item=s.target.handle;
        ops[0].edit.fields=D2RL::Items::EditFieldBit(D2RL::Items::EditField::Identified);
        ops[0].edit.stateFlags=D2RL::Items::ItemStateIdentified;
        ops[1].kind=D2RL::Items::ExistingItemOperationKind::Debit;ops[1].item=supply.handle;ops[1].debit.quantity=1;
        D2RL::Items::ExistingItemTransaction txn{};txn.structSize=D2RL::Items::ExistingItemTransactionSize;txn.player=player;txn.operationCount=2;txn.operations=ops;
        D2RL::Items::ExistingItemTransactionResult out{.structSize=D2RL::Items::ExistingItemTransactionResultSize};
        s.result.operation=items->executeExistingItemTransaction(ctx,&txn,&out);
        s.result.consumed=s.result.operation==ItemResult::Success;
    } else if(s.tome.handle) {
        // Keep the empty book. Debit-to-zero is explicitly unsupported by SDK.
        s.result.operation=SetQuantity(ctx,items,player,supply.handle,0);
        if(s.result.operation==ItemResult::Success) {
            s.result.operation=SetIdentified(ctx,items,player,s.target.handle,true);
            if(s.result.operation!=ItemResult::Success && SetQuantity(ctx,items,player,supply.handle,1)!=ItemResult::Success) s.result.status=Status::RollbackFailed;
            s.result.consumed=s.result.operation==ItemResult::Success;
        }
    } else {
        s.result.operation=SetIdentified(ctx,items,player,s.target.handle,true);
        if(s.result.operation==ItemResult::Success) {
            s.result.operation=items->destroyItem(ctx,player,supply.handle,D2RL::Items::SocketedItemPolicy::RejectIfNotEmpty);
            if(s.result.operation!=ItemResult::Success && SetIdentified(ctx,items,player,s.target.handle,false)!=ItemResult::Success)s.result.status=Status::RollbackFailed;
            s.result.consumed=s.result.operation==ItemResult::Success;
        }
    }
    if(s.result.operation==ItemResult::Success)s.result.status=Status::Success;
    return s.result;
}
inline const char* Name(Status s) noexcept {
    switch(s) {
    case Status::Success:return "success";
    case Status::ServicesUnavailable:return "SDK-services-unavailable";
    case Status::PlayerUnavailable:return "authoritative-player-unavailable-or-changed";
    case Status::ScanFailed:return "inventory-enumeration-failed";
    case Status::TargetMissing:return "exact-target-not-found";
    case Status::AlreadyIdentified:return "already-identified-no-charge-used";
    case Status::NoConsumable:return "no-identify-tome-or-scroll-in-inventory-or-cube";
    case Status::EmptyTome:return "tome-seen-but-no-positive-authoritative-quantity";
    case Status::MutationFailed:return "SDK-operation-failed";
    case Status::RollbackFailed:return "SDK-rollback-failed-inspect-item-and-consumable";
    }
    return "unknown";
}
}
