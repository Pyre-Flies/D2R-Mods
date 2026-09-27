#pragma once
#include <D2RLPlugin/api.h>
namespace QolSharedSdk {
using C=D2RL::Items::ItemContainer;
inline bool Supported(const D2RL::ItemService* items) noexcept {
    return D2RL::HasItemServiceCapability(items,D2RL::Items::ItemServiceCapability::SharedStashWrite) && items->executeExistingItemTransaction;
}
struct Outcome {
    bool attempted{};
    bool committed{};
    D2RL::Items::Result result{D2RL::Items::Result::Unsupported};
    uint32_t failureIndex{D2RL::Items::NoFailedOperation};
};
inline Outcome Move(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,
    D2RL::PlayerHandle player,const D2RL::Items::ItemInfo& item,uint32_t selectedPage,bool deposit) noexcept {
    if (!Supported(items)) return {};
    // Refuse unknown pages and wrong sources. A supported-but-rejected request
    // must never fall through to a second native transfer implementation.
    Outcome out{true,false,D2RL::Items::Result::InvalidArgument};
    if (!ctx || !player || !item.handle || selectedPage==UINT32_MAX ||
        (deposit ? item.container!=C::Inventory : item.container!=C::SharedStash || item.sharedStashPage!=selectedPage)) return out;
    D2RL::Items::ExistingItemOperation op{};
    op.structSize=D2RL::Items::ExistingItemOperationSize;
    op.kind=D2RL::Items::ExistingItemOperationKind::Move;
    op.item=item.handle;
    op.move.destination.structSize=D2RL::Items::ItemDestinationSize;
    op.move.destination.container=deposit?C::SharedStash:C::Inventory;
    op.move.destination.placement=D2RL::Items::Placement::Automatic;
    op.move.destination.sharedStashPage=deposit?selectedPage:UINT32_MAX;
    const D2RL::Items::ExistingItemTransaction txn{
        .structSize=D2RL::Items::ExistingItemTransactionSize,.player=player,.operationCount=1,.operations=&op};
    D2RL::Items::ExistingItemTransactionResult result{.structSize=D2RL::Items::ExistingItemTransactionResultSize};
    out.result=items->executeExistingItemTransaction(ctx,&txn,&result);
    out.failureIndex=result.failureIndex;
    out.committed=out.result==D2RL::Items::Result::Success && result.committedOperationCount==1;
    return out;
}
}
