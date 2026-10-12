#pragma once
#include "bulk_stash.h"

namespace QolBulkStashFeedback {
// Optional tab feedback owns no transfer policy. The queue retains the exact
// admission callback and remains available when visual admission fails.
void Initialize(const D2RL::PluginContext*, QolBulkStash::Available) noexcept;
// Called by the existing controller worker; native drawing is scheduled by the
// shared feedback implementation onto the SDK UI thread.
void Refresh() noexcept;
// Call after stopping the worker and controller input callbacks. Stop the queue
// before its observer receiver so cancellation can safely finish the batch.
void Shutdown() noexcept;
}
