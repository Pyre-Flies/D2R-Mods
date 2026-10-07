#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <limits>
namespace QolTasks {
// Scheduler userdata is a non-reused token, never an owning raw pointer.
// Reset frees discarded work; a callback from before Reset cannot claim new work.
template<class T,size_t Capacity=32> class Payloads {
    struct Entry {uintptr_t token{};std::unique_ptr<T> value;};
    std::array<Entry,Capacity> entries{};
    std::mutex mutex;
    uintptr_t next{},cancelledThrough{};
public:
    uintptr_t Put(std::unique_ptr<T> value,uintptr_t predecessor=0) noexcept {
        std::lock_guard lock(mutex);
        if(!value || next==std::numeric_limits<uintptr_t>::max() || (predecessor && predecessor<=cancelledThrough))return 0;
        for(auto& entry:entries)if(!entry.value){entry.token=++next;entry.value=std::move(value);return entry.token;}
        return 0;
    }
    std::unique_ptr<T> Take(uintptr_t token) noexcept {
        std::lock_guard lock(mutex);
        for(auto& entry:entries)if(token && entry.token==token)return std::move(entry.value);
        return {};
    }
    void Reset() noexcept {std::lock_guard lock(mutex);cancelledThrough=next;for(auto& entry:entries)entry.value.reset();}
};
}
