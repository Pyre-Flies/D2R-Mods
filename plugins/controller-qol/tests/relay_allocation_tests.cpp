#include "relay_allocation.h"
#include "glyph_calls.h"
#include <cstdio>
#include <vector>
#include <cstring>
namespace {
unsigned checks=0,failures=0;
void Check(bool value,const char* name) {++checks;if(!value){++failures;std::printf("FAIL: %s\n",name);}}
}
int main() {
    using namespace QolRelay;
    constexpr uintptr_t site=0x1401922cd,step=0x10000;
    const auto ranges=Plan(site,step,0x7ffffffeffff,64);
    Check(ranges[0].first==site+5+FarMin,"prefer outside the loader hook radius");
    Check(ranges[1].last==site+5-FarMin,"outer lower range before near fallback");
    for(const auto range:ranges) {
        unsigned char encoded[5]{};
        Check(range.first && QolGlyphCalls::Encode(site,range.first,encoded),"lower rel32 boundary reachable");
        Check(range.last && QolGlyphCalls::Encode(site,range.last,encoded),"upper rel32 boundary reachable");
    }
    Check(!Plan(0,step,0x7ffffffeffff,64)[0].first,"reject zero site");
    Check(!Plan(UINTPTR_MAX-4,step,UINTPTR_MAX,64)[0].first,"reject next-instruction overflow");
    Check(!Plan(site,step,step+31,64)[0].first,"reject size beyond user range");
    const auto low=Plan(0x10000,step,0x7ffffffeffff,64);
    Check(!low[1].first && low[3].first==step,"low address does not wrap lower search");
    const auto high=Plan(UINTPTR_MAX-5,step,UINTPTR_MAX,64);
    Check(!high[0].first && high[1].first,"high address does not wrap upper search");
    uintptr_t aligned{};
    Check(!Align(UINTPTR_MAX,step,aligned),"reject alignment overflow");
    Check(!Align(site,0,aligned) && !Align(site,3,aligned),"reject invalid granularity");
    std::vector<uintptr_t> attempted;
    const auto query=[](uintptr_t address,Region& r) noexcept {
        if(address<0x30000)r={0x10000,0x20000,false};
        else if(address<0x40000)r={0x30000,0x1000,false};
        else r={0x40000,0x30000,true};
        return true;
    };
    const auto result=Search({0x10001,0x60000},64,step,query,[&](uintptr_t address,uintptr_t) {
        attempted.push_back(address);return address==0x50000;
    });
    Check(result==0x50000 && attempted==std::vector<uintptr_t>{0x40000,0x50000},"skip occupied regions and recover from reservation race");
    Check(!Search({0x10000,0x20000},64,step,[](uintptr_t,Region& r){r={0x10000,0x10000-32,true};return true;},
        [](uintptr_t,uintptr_t){return false;}),"bounded failure on exhausted free region");
    Check(!Search({0x10000,0x20000},64,step,[](uintptr_t,Region& r){r={0x20000,0x10000,true};return true;},
        [](uintptr_t,uintptr_t){return true;}),"refuse malformed region ownership");
    Check(!Search({0x10000,0x20000},64,step,[](uintptr_t,Region&){return false;},
        [](uintptr_t,uintptr_t){return true;}),"query failure leaves no reservation");
    // Exercise the real Windows allocator and the unchanged executable relay.
    auto* relay=Allocate(reinterpret_cast<uintptr_t>(&main),64);
    Check(relay!=nullptr,"real relay allocation");
    if(relay) {
        MEMORY_BASIC_INFORMATION memory{};VirtualQuery(relay,&memory,sizeof(memory));
        Check(memory.AllocationBase==relay && memory.Protect==PAGE_READWRITE,"new reservation is owned and non-executable");
        const unsigned char code[]={0xb8,0x2a,0,0,0,0xc3};std::memcpy(relay,code,sizeof(code));
        DWORD old{};const bool sealed=VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old) && FlushInstructionCache(GetCurrentProcess(),relay,64);
        Check(sealed,"relay seals executable without write permission");
        if(sealed)Check(reinterpret_cast<int(*)()>(relay)()==42,"far allocation executes unchanged code");
        unsigned char call[5]{};Check(QolGlyphCalls::Encode(reinterpret_cast<uintptr_t>(&main),reinterpret_cast<uintptr_t>(relay),call),"real allocation remains rel32 reachable");
        Check(VirtualFree(relay,0,MEM_RELEASE)!=0,"unpublished test allocation releases");
    }
    std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
