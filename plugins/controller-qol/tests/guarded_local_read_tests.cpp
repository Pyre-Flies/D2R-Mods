#include "guarded_local_read.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void Check(bool ok,const char* message) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main() {
    SYSTEM_INFO info{};GetSystemInfo(&info);const auto page=info.dwPageSize;
    auto* source=static_cast<unsigned char*>(VirtualAlloc(nullptr,page*3,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(source!=nullptr,"owned source allocation");
    for(SIZE_T i=0;i<page*3;++i)source[i]=static_cast<unsigned char>(i*17U);
    const DWORD protections[]={PAGE_READONLY,PAGE_READWRITE,PAGE_EXECUTE_READ,PAGE_EXECUTE_READWRITE,PAGE_NOACCESS,PAGE_EXECUTE};
    for(const auto protection:protections) {
        DWORD old{};Check(VirtualProtect(source,page,protection,&old)!=0,"change owned source protection");
        std::array<unsigned char,80> original{},local{};SIZE_T copied{};
        const bool expected=ReadProcessMemory(GetCurrentProcess(),source,original.data(),original.size(),&copied)!=FALSE && copied==original.size();
        copied=123;
        const bool actual=GuardedLocalRead::Read(GetCurrentProcess(),source,local.data(),local.size(),&copied)!=FALSE;
        if(actual!=expected)std::fprintf(stderr,"protection=0x%lx original=%u local=%u\n",protection,expected,actual);
        Check(actual==expected,"local copies retain current-process readable protection admission");
        Check(actual?copied==local.size() && local==original:copied==0,"exact success bytes or explicit failure");
    }
    DWORD old{};Check(VirtualProtect(source,page*3,PAGE_READWRITE,&old)!=0,"restore owned pages");
    Check(VirtualProtect(source+page,page,PAGE_READONLY,&old)!=0,"second readable region");
    std::array<unsigned char,80> output{};
    Check(GuardedLocalRead::Copy(source+page-40,output.data(),output.size()) &&
        std::memcmp(output.data(),source+page-40,output.size())==0,"copy across readable regions");
    Check(VirtualProtect(source+page,page,PAGE_NOACCESS,&old)!=0,"inaccessible second region");
    output.fill(0x5a);
    Check(!GuardedLocalRead::Copy(source+page-40,output.data(),output.size()),
        "inaccessible trailing region refuses; failed snapshots are discarded");
    Check(!GuardedLocalRead::Detail::CopyReadable(source+page,output.data(),output.size()),
        "fault boundary refuses source protection changed after qualification");
    Check(VirtualProtect(source,page,PAGE_READWRITE|PAGE_GUARD,&old)!=0,"guard source page");
    SIZE_T guardCopied{};
    Check(!ReadProcessMemory(GetCurrentProcess(),source,output.data(),output.size(),&guardCopied),"original API refuses guard source");
    MEMORY_BASIC_INFORMATION originalGuard{};VirtualQuery(source,&originalGuard,sizeof(originalGuard));
    Check(VirtualProtect(source,page,PAGE_READWRITE|PAGE_GUARD,&old)!=0,"reset owned guard source");
    Check(!GuardedLocalRead::Copy(source,output.data(),output.size()),"local copy refuses guard source");
    MEMORY_BASIC_INFORMATION guarded{};VirtualQuery(source,&guarded,sizeof(guarded));
    Check(guarded.Protect==originalGuard.Protect,"guard fault behavior matches the replaced API");
    Check(VirtualProtect(source,page,PAGE_READWRITE,&old)!=0,"restore first page");
    Check(GuardedLocalRead::Copy(source,output.data(),output.size()),"warm readable source");
    Check(VirtualProtect(source,page,PAGE_NOACCESS,&old)!=0,"invalidate after warmup");
    Check(!GuardedLocalRead::Copy(source,output.data(),output.size()),"no cached readability after warmup");
    Check(VirtualFree(source+page*2,page,MEM_DECOMMIT)!=0,"decommit owned last page");
    Check(!GuardedLocalRead::Copy(source+page*2,output.data(),output.size()),"decommitted source refused");
    Check(!GuardedLocalRead::Copy(nullptr,output.data(),output.size()) &&
        !GuardedLocalRead::Copy(reinterpret_cast<void*>(UINTPTR_MAX-10),output.data(),80) &&
        !GuardedLocalRead::Copy(source,output.data(),0) && !GuardedLocalRead::Copy(source,nullptr,80),"invalid bounds refused");
    SIZE_T copied=123;
    Check(!GuardedLocalRead::Read(nullptr,source,output.data(),80,&copied) && copied==0,"foreign process API use refused");
    VirtualFree(source,0,MEM_RELEASE);
    std::puts("Local copies preserve readable regions, full ranges, guard pages, faults and live protection changes.");
}
