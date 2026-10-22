#ifdef UNHOARDER_IMAGE_HEADERS
#include "current_page_query.hpp"
#include "maps_socket_memory.hpp"
namespace Reader=UnHoarder::MapsSocket;
#else
#include "current_page_query.h"
#include "chat_forwarding_memory.h"
namespace Reader=QolChatForwarding;
#endif
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks{};
static void Check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static bool Open(Reader::ImageMemory& image,HMODULE module){
#ifdef UNHOARDER_IMAGE_HEADERS
    return image.Open(module);
#else
    return image.Open(module,true);
#endif
}
static BOOL WINAPI UnavailableWs(HANDLE,PVOID,DWORD){return FALSE;}
static BOOL WINAPI UnavailableRegion(HANDLE,const VOID*,WIN32_MEMORY_INFORMATION_CLASS,PVOID,SIZE_T,PSIZE_T){return FALSE;}
static BOOL WINAPI PhysicalRegion(HANDLE,const VOID* at,WIN32_MEMORY_INFORMATION_CLASS,PVOID out,SIZE_T,PSIZE_T size){
    auto& r=*static_cast<WIN32_MEMORY_REGION_INFORMATION*>(out);r.AllocationBase=const_cast<void*>(at);r.RegionSize=0x1000;r.MappedPhysical=1;*size=sizeof(r);return TRUE;
}
static unsigned allocationCalls{};
static BOOL WINAPI ChangingRegion(HANDLE process,const VOID* at,WIN32_MEMORY_INFORMATION_CLASS type,PVOID out,SIZE_T bytes,PSIZE_T size){
    const auto ok=CurrentPageQuery::SystemApi().allocation(process,at,type,out,bytes,size);
    if(ok && ++allocationCalls==2)static_cast<WIN32_MEMORY_REGION_INFORMATION*>(out)->AllocationBase=nullptr;
    return ok;
}
int main(){
    const auto& api=CurrentPageQuery::SystemApi();
    Check(api.workingSet && api.allocation && api.pageSize,"native Windows API fixture availability");
    auto* source=static_cast<unsigned char*>(VirtualAlloc(nullptr,api.pageSize*3,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(source!=nullptr,"owned page fixture");std::memset(source,0x53,api.pageSize*3);
    DWORD protection{},old{};
    for(DWORD mode:{PAGE_READONLY,PAGE_READWRITE,PAGE_EXECUTE_READ,PAGE_EXECUTE_READWRITE,PAGE_EXECUTE,PAGE_NOACCESS,PAGE_READWRITE|PAGE_GUARD}){
        Check(VirtualProtect(source,api.pageSize,mode,&old)!=0,"current page protection mutation");
        MEMORY_BASIC_INFORMATION expected{};Check(VirtualQuery(source,&expected,sizeof(expected))==sizeof(expected),"original protection witness");
        const bool resident=CurrentPageQuery::SinglePage(reinterpret_cast<uintptr_t>(source),64,0,protection);
        if(resident)Check(protection==expected.Protect,"fresh resident protection equals original query");
        if(mode==PAGE_NOACCESS || (mode&PAGE_GUARD))Check(!resident,"inaccessible/guarded page uses original path");
        MEMORY_BASIC_INFORMATION after{};VirtualQuery(source,&after,sizeof(after));
        Check(after.Protect==expected.Protect,"query leaves guard protection intact");
    }
    Check(VirtualProtect(source,api.pageSize,PAGE_READWRITE,&old)!=0,"restore fixture");source[0]=0x53;
    const auto at=reinterpret_cast<uintptr_t>(source);
    Check(CurrentPageQuery::SinglePage(at,64,0,protection),"resident path active");
    Check(!CurrentPageQuery::SinglePage(at,64,at,protection),"private allocation cannot impersonate image ownership");
    Check(!CurrentPageQuery::SinglePage(at+api.pageSize-16,32,0,protection),"cross-page range keeps original region proof");
    Check(!CurrentPageQuery::SinglePage(0,64,0,protection) && !CurrentPageQuery::SinglePage(UINTPTR_MAX-3,64,0,protection) && !CurrentPageQuery::SinglePage(at,0,0,protection),"invalid bounds refused");
    auto unavailable=api;unavailable.workingSet=nullptr;
    Check(!CurrentPageQuery::Observe(unavailable,at,64,0,protection),"missing working-set API falls back");
    unavailable=api;unavailable.workingSet=UnavailableWs;
    Check(!CurrentPageQuery::Observe(unavailable,at,64,0,protection),"failed working-set API falls back");
    unavailable=api;unavailable.allocation=UnavailableRegion;
    Check(!CurrentPageQuery::Observe(unavailable,at,64,0,protection),"failed allocation API falls back");
    unavailable.allocation=PhysicalRegion;
    Check(!CurrentPageQuery::Observe(unavailable,at,64,0,protection),"physical allocation commitment is not inferred");
    unavailable=api;unavailable.allocation=ChangingRegion;allocationCalls=0;
    Check(!CurrentPageQuery::Observe(unavailable,at,64,0,protection) && allocationCalls==2,"allocation change across protection query invalidates witness");
    auto* cold=VirtualAlloc(nullptr,api.pageSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    Check(cold!=nullptr,"untouched committed page");
    Check(!CurrentPageQuery::SinglePage(reinterpret_cast<uintptr_t>(cold),64,0,protection),"nonresident committed page retains original fallback");
    unsigned char coldBytes[64]{};SIZE_T got{};
    Check(ReadProcessMemory(GetCurrentProcess(),cold,coldBytes,sizeof(coldBytes),&got) && got==sizeof(coldBytes),"original fallback can read a nonresident page");
    Check(CurrentPageQuery::SinglePage(reinterpret_cast<uintptr_t>(cold),64,0,protection),"new observation notices resident page without retained failure");
    VirtualFree(cold,0,MEM_RELEASE);
    Check(VirtualFree(source+api.pageSize*2,api.pageSize,MEM_DECOMMIT)!=0,"decommit after warmup");
    Check(!CurrentPageQuery::SinglePage(at+api.pageSize*2,64,0,protection),"decommitted page cannot reuse prior success");
    Reader::ImageMemory image{};Check(Open(image,GetModuleHandleW(nullptr)),"real fixture PE headers");
    const auto code=reinterpret_cast<uintptr_t>(&main);unsigned char output[16]{};
    const auto prior=CurrentPageQuery::residentQueries.load();
    Check(image.Read(code,output,sizeof(output),Reader::Kind::Code),"actual production image reader accepts its live code");
    Check(CurrentPageQuery::residentQueries.load()>prior,"production reader uses fresh resident-page proof");
    Check(!CurrentPageQuery::SinglePage(code,16,image.base+api.pageSize,protection),"wrong image allocation owner refused");
    // Copy-on-write data mapping: query must preserve the old readable/writable
    // classification even after a private copy is created.
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,static_cast<DWORD>(api.pageSize),nullptr);
    Check(mapping!=nullptr,"owned mapping");
    auto* view=static_cast<unsigned char*>(MapViewOfFile(mapping,FILE_MAP_COPY,0,0,api.pageSize));
    Check(view!=nullptr,"copy-on-write view");view[0]=7;
    MEMORY_BASIC_INFORMATION cow{};VirtualQuery(view,&cow,sizeof(cow));
    if(CurrentPageQuery::SinglePage(reinterpret_cast<uintptr_t>(view),16,0,protection))
        Check((protection&(PAGE_READWRITE|PAGE_WRITECOPY)) && (cow.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)),"copy-on-write retains writable classification");
    UnmapViewOfFile(view);CloseHandle(mapping);VirtualFree(source,0,MEM_RELEASE);
    std::printf("%u current-page proof checks passed.\n",checks);
}
