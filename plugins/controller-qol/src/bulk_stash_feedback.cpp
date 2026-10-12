#include "bulk_stash_feedback.h"
#include "stash_feedback.h"

namespace QolBulkStashFeedback {
void Initialize(const D2RL::PluginContext* context, QolBulkStash::Available available) noexcept {
    (void)InventoryDepositStashFeedback::Initialize(context);
    const QolBulkStash::Observer observer{
        nullptr,
        [](void*) noexcept { InventoryDepositStashFeedback::BeginBatch(); },
        [](void*, const D2RL::Items::ItemInfo& item) noexcept {
            InventoryDepositStashFeedback::ItemRemoved(item);
        },
        [](void*) noexcept { InventoryDepositStashFeedback::EndBatch(); },
    };
    // Observers only copy identifiers into the shared mailbox. They do not
    // request another deposit, schedule work, or call native UI operations.
    QolBulkStash::Initialize(context, available, &observer);
}

void Refresh() noexcept { InventoryDepositStashFeedback::Refresh(); }

void Shutdown() noexcept {
    QolBulkStash::Shutdown();
    InventoryDepositStashFeedback::Shutdown();
}
}
