#pragma once
#include <cstdint>
#include <cstddef>

namespace TerrainProbe::GroundSupply {
enum class ReadFailure {None,Budget,Clock,Memory};

// Clock returns a monotonic performance-counter value. The caller supplies
// ceil(frequency / 125), i.e. an eight-millisecond budget in counter ticks.
// Check both sides of each OS read; the read itself cannot be interrupted.
template<class Clock,class Read> struct BudgetReader {
    Clock& clock;Read& raw;int64_t start{},ticks{},previous{};
    ReadFailure failure{ReadFailure::None};
    bool Check() noexcept {
        if(failure!=ReadFailure::None) return false;
        int64_t now{};
        if(start<0 || ticks<=0 || !clock(now) || now<start || now<previous) {
            failure=ReadFailure::Clock;return false;
        }
        previous=now;
        if(now-start>ticks) {failure=ReadFailure::Budget;return false;}
        return true;
    }
    bool operator()(uintptr_t address,void* output,size_t size) noexcept {
        if(!Check()) return false;
        if(!raw(address,output,size)) {failure=ReadFailure::Memory;return false;}
        return Check();
    }
};
}
