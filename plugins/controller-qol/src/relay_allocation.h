#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <windows.h>

namespace QolRelay {
struct Range { uintptr_t first{}, last{}; }; // Inclusive candidate starts.
struct Region { uintptr_t first{}, size{}; bool free{}; };
inline constexpr uintptr_t FarMin=0x50000000, FarMax=0x70000000;
inline bool Align(uintptr_t value,uintptr_t step,uintptr_t& result) noexcept {
    if(!step || (step&(step-1)) || value>UINTPTR_MAX-(step-1))return false;
    result=(value+step-1)&~(step-1);return true;
}
inline std::array<Range,4> Plan(uintptr_t site,uintptr_t minimum,uintptr_t maximum,uintptr_t size) noexcept {
    std::array<Range,4> ranges{};
    if(!site || site>UINTPTR_MAX-5 || !size || maximum<minimum || size-1>maximum-minimum)return ranges;
    const auto next=site+5;
    const auto low=next>0x80000000?next-0x80000000:0;
    const auto high=next>UINTPTR_MAX-0x7fffffff?UINTPTR_MAX:next+0x7fffffff;
    const auto first=std::max(minimum,low),last=std::min(maximum-(size-1),high);
    if(!first || first>last)return ranges;
    const auto window=[first,last](uintptr_t a,uintptr_t b) noexcept -> Range {
        a=std::max(first,a);b=std::min(last,b);return a<=b?Range{a,b}:Range{};
    };
    // QOL's CALL relays can reach farther than this Core's +/-1 GiB hook pool.
    // Prefer the outer range so small QOL reservations do not compete with it.
    if(next<=UINTPTR_MAX-FarMin)
        ranges[0]=window(next+FarMin,next>UINTPTR_MAX-FarMax?UINTPTR_MAX:next+FarMax);
    if(next>=FarMin)ranges[1]=window(next>FarMax?next-FarMax:0,next-FarMin);
    ranges[2]=window(next,last);
    if(next)ranges[3]=window(first,next-1);
    return ranges;
}
template<class Query,class Allocate>
uintptr_t Search(Range range,uintptr_t size,uintptr_t step,Query&& query,Allocate&& allocate) noexcept {
    uintptr_t cursor{};
    if(!range.first || range.first>range.last || !size || !Align(range.first,step,cursor))return 0;
    while(cursor<=range.last) {
        Region region{};
        if(!query(cursor,region) || !region.size || region.first>cursor ||
           region.size>UINTPTR_MAX-region.first)return 0;
        const auto end=region.first+region.size;
        if(end<=cursor)return 0;
        if(region.free && size<=end-cursor) {
            if(allocate(cursor,size))return cursor;
            // Another allocator may have won the reservation. Advance rather
            // than retrying that address forever or accepting a different one.
            if(cursor>UINTPTR_MAX-step)return 0;
            cursor+=step;
        } else if(!Align(end,step,cursor))return 0;
    }
    return 0;
}
inline unsigned char* Allocate(uintptr_t site,uintptr_t size) noexcept {
    SYSTEM_INFO info{};GetSystemInfo(&info);
    const auto ranges=Plan(site,reinterpret_cast<uintptr_t>(info.lpMinimumApplicationAddress),
        reinterpret_cast<uintptr_t>(info.lpMaximumApplicationAddress),size);
    const auto query=[](uintptr_t address,Region& region) noexcept {
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info)))return false;
        region={reinterpret_cast<uintptr_t>(info.BaseAddress),info.RegionSize,info.State==MEM_FREE};return true;
    };
    const auto allocate=[](uintptr_t address,uintptr_t bytes) noexcept {
        auto* result=VirtualAlloc(reinterpret_cast<void*>(address),bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(result==reinterpret_cast<void*>(address))return true;
        if(result)VirtualFree(result,0,MEM_RELEASE);
        return false;
    };
    for(const auto range:ranges)
        if(const auto result=Search(range,size,info.dwAllocationGranularity,query,allocate))
            return reinterpret_cast<unsigned char*>(result);
    return nullptr;
}
}
