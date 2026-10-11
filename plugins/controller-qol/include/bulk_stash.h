#pragma once
#include <D2RLPlugin/api.h>
namespace QolBulkStash {
using Available=bool(*)() noexcept;
// Optional feedback only; callbacks run synchronously under the queue lock on
// its current game/UI thread. They may copy POD to a mailbox, but must not call
// native UI functions or reenter the queue. Initialize copies this structure;
// the user pointer must remain valid until Shutdown returns. Source removal is
// observed only after a successful inventory read, not proof of destination
// counters or save persistence. The item is the original copied snapshot.
struct Observer {
    void* user{};
    void (*begin)(void*) noexcept{};
    void (*removed)(void*,const D2RL::Items::ItemInfo&) noexcept{};
    void (*end)(void*) noexcept{};
};
void Initialize(const D2RL::PluginContext*,Available,const Observer* observer=nullptr) noexcept;
bool Request() noexcept;
bool Busy() noexcept;
void Shutdown() noexcept;
}
