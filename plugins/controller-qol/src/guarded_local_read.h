#pragma once
#ifndef D2R_GUARDED_LOCAL_READ_V1_INCLUDED
#define D2R_GUARDED_LOCAL_READ_V1_INCLUDED
#include <windows.h>
#include <cstdint>
#include <cstring>

// Source-identical in the independent UnHoarder fork. Current-process copies
// only: no retained page information, item snapshot or successful admission.
namespace GuardedLocalRead {
namespace Detail {
inline bool CopyReadable(const void* source,void* output,SIZE_T bytes) noexcept {
    // The owning reader retains its page/ownership checks. Catch inaccessible
    // memory and protection/decommit races; discard output after any failure.
    __try {std::memcpy(output,source,bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
inline bool Copy(const void* source,void* output,SIZE_T bytes) noexcept {
    const auto address=reinterpret_cast<std::uintptr_t>(source);
    if(!address || !output || !bytes || address>UINTPTR_MAX-bytes)return false;
    const auto end=address+bytes;
    for(auto cursor=address;cursor<end;) {
        MEMORY_BASIC_INFORMATION page{};
        if(VirtualQuery(reinterpret_cast<const void*>(cursor),&page,sizeof(page))!=sizeof(page) ||
           page.State!=MEM_COMMIT || (page.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
        switch(page.Protect&0xffU) {
        // ReadProcessMemory admits execute-only pages on the supported Windows
        // x64 host too. Kind-specific image readers retain their stricter gates.
        case PAGE_READONLY:case PAGE_READWRITE:case PAGE_WRITECOPY:case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:case PAGE_EXECUTE_READWRITE:case PAGE_EXECUTE_WRITECOPY:break;
        default:return false;
        }
        const auto base=reinterpret_cast<std::uintptr_t>(page.BaseAddress);
        if(base>cursor || page.RegionSize>UINTPTR_MAX-base || base+page.RegionSize<=cursor)return false;
        cursor=base+page.RegionSize;
    }
    return Detail::CopyReadable(source,output,bytes);
}
inline BOOL Read(HANDLE process,const void* source,void* output,SIZE_T bytes,SIZE_T* copied) noexcept {
    if(copied)*copied=0;
    if(process!=GetCurrentProcess() || !Copy(source,output,bytes))return FALSE;
    if(copied)*copied=bytes;
    return TRUE;
}
}
#endif
