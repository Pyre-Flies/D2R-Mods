#pragma once
#ifndef D2R_CURRENT_PAGE_QUERY_V1_INCLUDED
#define D2R_CURRENT_PAGE_QUERY_V1_INCLUDED
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <cstdint>
#include <cstdio>

// Source-identical in the independent UnHoarder fork. Only OS entry points and
// hardware page size are retained. Every protection/allocation observation is
// fresh. False means the owning reader must use its original VirtualQuery path.
namespace CurrentPageQuery {
struct Api {
    decltype(&QueryWorkingSetEx) workingSet{};
    decltype(&QueryVirtualMemoryInformation) allocation{};
    SIZE_T pageSize{};
};
inline const Api& SystemApi() noexcept {
    static const Api api=[]() noexcept {
        Api result{};
        const auto kernel=GetModuleHandleW(L"kernel32.dll");
        const auto base=GetModuleHandleW(L"kernelbase.dll");
        const auto native=GetModuleHandleW(L"ntdll.dll");
        // Proton/Wine retains the established reader; the resident-page
        // equivalence has been qualified on native Windows only.
        if(native && GetProcAddress(native,"wine_get_version"))return result;
        if(kernel)result.workingSet=reinterpret_cast<decltype(result.workingSet)>(GetProcAddress(kernel,"K32QueryWorkingSetEx"));
        if(base)result.allocation=reinterpret_cast<decltype(result.allocation)>(GetProcAddress(base,"QueryVirtualMemoryInformation"));
        SYSTEM_INFO info{};GetNativeSystemInfo(&info);result.pageSize=info.dwPageSize;
        return result;
    }();
    return api;
}
inline std::atomic<std::uint64_t> residentQueries{},fallbackQueries{};

// A valid ordinary resident page is committed. Its protection comes from the
// working-set query, NOT AllocationProtect. Single-page containment preserves
// the old contiguous-region range test. Image readers additionally require the
// current allocation to be MEM_IMAGE and owned by their exact module base.
inline bool Observe(const Api& api,std::uintptr_t address,SIZE_T bytes,
    std::uintptr_t imageOwner,DWORD& protection) noexcept {
    protection=0;
    if(!address || !bytes || address>UINTPTR_MAX-bytes || !api.pageSize ||
       !api.workingSet || !api.allocation ||
       address/api.pageSize!=(address+bytes-1)/api.pageSize)return false;
    WIN32_MEMORY_REGION_INFORMATION region{};SIZE_T returned{};
    if(!api.allocation(GetCurrentProcess(),reinterpret_cast<const void*>(address),
            MemoryRegionInfo,&region,sizeof(region),&returned) || returned!=sizeof(region))return false;
    // Physical/direct mappings have different commitment semantics. Unknown
    // allocations and unsupported platforms always keep the original proof.
    if(region.MappedPhysical || region.DirectMapped ||
       !(region.Private || region.MappedDataFile || region.MappedImage || region.MappedPageFile))return false;
    const auto base=reinterpret_cast<std::uintptr_t>(region.AllocationBase);
    if(!base || address<base || region.RegionSize>UINTPTR_MAX-base ||
       address-base>=region.RegionSize || bytes>region.RegionSize-(address-base) ||
       (imageOwner && (!region.MappedImage || base!=imageOwner)))return false;
    PSAPI_WORKING_SET_EX_INFORMATION page{};page.VirtualAddress=reinterpret_cast<void*>(address);
    if(!api.workingSet(GetCurrentProcess(),&page,sizeof(page)) ||
       !page.VirtualAttributes.Valid || page.VirtualAttributes.Bad || page.VirtualAttributes.LargePage)return false;
    // The APIs expose allocation and protection separately. Recheck the
    // allocation after the page query so a changed mapping cannot supply a
    // different page under an earlier ownership witness. Copying still has
    // the owning reader's SEH boundary, as with the original VirtualQuery path.
    WIN32_MEMORY_REGION_INFORMATION after{};returned=0;
    if(!api.allocation(GetCurrentProcess(),reinterpret_cast<const void*>(address),
            MemoryRegionInfo,&after,sizeof(after),&returned) || returned!=sizeof(after) ||
       after.AllocationBase!=region.AllocationBase || after.RegionSize!=region.RegionSize ||
       after.Private!=region.Private || after.MappedDataFile!=region.MappedDataFile ||
       after.MappedImage!=region.MappedImage || after.MappedPageFile!=region.MappedPageFile ||
       after.MappedPhysical!=region.MappedPhysical || after.DirectMapped!=region.DirectMapped)return false;
    protection=static_cast<DWORD>(page.VirtualAttributes.Win32Protection);
    return true;
}
inline bool SinglePage(std::uintptr_t address,SIZE_T bytes,
    std::uintptr_t imageOwner,DWORD& protection) noexcept {
    const bool observed=Observe(SystemApi(),address,bytes,imageOwner,protection);
    (observed?residentQueries:fallbackQueries).fetch_add(1,std::memory_order_relaxed);
    return observed;
}
template<class Emit> inline void Report(const char* owner,Emit emit) noexcept {
    char line[256]{};
    std::snprintf(line,sizeof(line),"PAGE_QUERY owner=%s residentSinceLoad=%llu fallbackSinceLoad=%llu fresh-per-read=1",
        owner,static_cast<unsigned long long>(residentQueries.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(fallbackQueries.load(std::memory_order_relaxed)));
    emit(line);
}
}
#endif
