#include "ground_action_hooks.h"
#include "plugin_compatibility.h"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <array>
static void Check(bool value,const char* message) { if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);} }
using QolCompat::ActionHooks;
static std::array<ActionHooks::Handler,18> installed{};
static int installs{}, nativeCalls{}, observations{}, projections{}, lastNative{}, lastObserved{}, failAt=-1;
static void* lastPacket{};
static uint8_t lastPacketByte{};
template<size_t I> int64_t __fastcall Native(void*,void*,void* packet,int32_t) { ++nativeCalls;lastNative=int(I+1);lastPacket=packet;lastPacketByte=packet?static_cast<uint8_t*>(packet)[1]:0;return 1000+I; }
template<size_t...I> auto Originals(std::index_sequence<I...>) { return std::array<ActionHooks::Handler,18>{&Native<I>...}; }
static const auto originals=Originals(std::make_index_sequence<18>{});
static bool __cdecl Match(const D2RL::PluginContext*,uint64_t rva,const void* bytes,uint32_t size) noexcept {
    for(const auto& s:QolCompat::Native::Actions) if(s.rva==rva) return size==32 && std::memcmp(bytes,s.bytes,32)==0;
    return false; // In particular, table-slot validation is forbidden here.
}
static bool __cdecl Install(const D2RL::PluginContext*,const D2RL::InlineHookRegistration* h) noexcept {
    if(installs==failAt) return false;
    int i=installs++;
    if(i>=18 || !Match(nullptr,h->rva,h->expected,h->expectedSize)) return false;
    installed[i]=reinterpret_cast<ActionHooks::Handler>(h->target);
    *h->original=reinterpret_cast<void*>(originals[i]);return true;
}
static void Observe(uint8_t opcode,void*,void*,void*,int32_t) noexcept {++observations;lastObserved=opcode;}
static bool Project(void*,const void* packet,int32_t size,std::array<uint8_t,5>& output) noexcept {
    ++projections;
    if(!packet || size!=5)return false;
    std::memcpy(output.data(),packet,5);output[1]=99;return true;
}
int main(int argc,char** argv) {
    Check(argc==2,"fixture argument");
    D2RL::PluginApi api{};api.apiSize=D2RL::PluginApiSize;api.checkExpectedBytes=&Match;api.installInlineHook=&Install;
    D2RL::PluginContext ctx{};ctx.contextSize=sizeof(ctx);ctx.api=&api;
    // Model another plugin's exclusive table ownership in BOTH load orders.
    for(bool potionFirst:{false,true}) {
        std::array<uintptr_t,18> table{}, expected{};
        for(size_t i=0;i<18;++i) table[i]=expected[i]=0x140000000+QolCompat::Native::Actions[i].rva;
        if(potionFirst) table.fill(0x12345678);
        const auto before=table;installs=0;failAt=-1;
        Check(ActionHooks::Install(&ctx,&Observe,&Project),"QOL hooks install in either table-owner order");
        Check(table==before,"QOL never mutates another plugin's table");
        if(!potionFirst) {Check(table==expected,"Potion native table fingerprint still passes after QOL");table.fill(0x12345678);}
        for(size_t i=0;i<18;++i) {
            nativeCalls=observations=projections=0;
            uint8_t packet[5]{5,1,2,3,4};
            Check(installed[i](nullptr,nullptr,packet,5)==1000+i,"native result forwarded");
            Check(nativeCalls==1 && observations==1 && lastNative==i+1 && lastObserved==i+1,"correct per-handler trampoline and exactly one observer");
            if(i==4) {
                Check(projections==1 && lastPacket!=packet && lastPacketByte==99,
                    "controller coordinate handler receives private projected packet");
                Check(packet[1]==1,"original controller packet remains unchanged");
            } else Check(projections==0 && lastPacket==packet,"unrelated handlers bypass projection");
        }
        ActionHooks::Shutdown();observations=nativeCalls=0;
        installed[0](nullptr,nullptr,nullptr,0);
        Check(nativeCalls==1 && observations==0 && table[0]==0x12345678,"shutdown forwards native and preserves other table owner");
    }
    installs=0;failAt=3;observations=nativeCalls=0;
    Check(!ActionHooks::Install(&ctx,&Observe),"partial hook installation reported");
    installed[0](nullptr,nullptr,nullptr,0);
    Check(nativeCalls==1 && observations==0,"partial installation remains passive");

    HMODULE fixture=LoadLibraryA(argv[1]);Check(fixture!=nullptr,"load fixture");
    auto target=reinterpret_cast<uintptr_t>(GetProcAddress(fixture,"FixtureTarget"));
    auto version=reinterpret_cast<void(*)(int)>(GetProcAddress(fixture,"FixtureVersion"));
    Check(QolCompat::IsPluginCaller(target),"direct plugin requests recognized");
    Check(!QolCompat::IsPluginCaller(reinterpret_cast<uintptr_t>(&main)),"ordinary game-like callers retain pickup restrictions");
    auto* entry=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(entry!=nullptr,"allocate fixture image");
    const auto reset=[&]{std::memcpy(entry,QolCompat::Native::BeltEntry,96);};
    reset();Check(QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"native belt signature accepted");
    entry[40]^=1;Check(!QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"native body mismatch rejected");
    reset();entry[0]=0xE9;int32_t relative=128-5;std::memcpy(entry+1,&relative,4);
    entry[128]=0xFF;entry[129]=0x25;std::memset(entry+130,0,4);std::memcpy(entry+134,&target,8);
    Check(QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"recognized rel32 relay belt hook accepted");
    version(0);Check(!QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"unreviewed plugin version rejected");version(1);
    entry[40]^=1;Check(!QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"recognized owner cannot hide changed native body");
    reset();entry[0]=0xFF;entry[1]=0x25;std::memset(entry+2,0,4);std::memcpy(entry+6,&target,8);
    Check(QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"recognized direct absolute hook accepted");
    uintptr_t unknown=reinterpret_cast<uintptr_t>(&main);std::memcpy(entry+6,&unknown,8);
    Check(!QolCompat::ValidateBeltEntry(reinterpret_cast<uintptr_t>(entry)),"unknown detour owner rejected");
    VirtualFree(entry,0,MEM_RELEASE);FreeLibrary(fixture);
    std::puts("Compatibility registration, forwarding, load-order model, pickup origin and cooperative belt admission passed.");
}
