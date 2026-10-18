#include "loot_run_policy.h"
#include <cstdio>
#include <initializer_list>
#include <cstdlib>
#include <cstring>
extern "C" {
    void RunFixtureQuery();
    void RunFixtureClobber();
    void RunFixtureInvoke(void*,uint32_t,uint32_t);
    void* QolLootNativeRunQuery=reinterpret_cast<void*>(&RunFixtureQuery);
    alignas(16) unsigned char RunFixtureState[160]{};
    unsigned RunFixtureCalls=0;
}
static void* expectedPlayer;
static unsigned observations=0;
static uint32_t selection=2;
static void check(bool ok){if(!ok)std::abort();}
extern "C" uint64_t QolLootSelectRunFlag(uint64_t native,void* player,uint32_t type,uint32_t guid) noexcept {
    check(native==0x123456789abc0011ULL && player==expectedPlayer && type==4 && guid==42);
    ++observations;
    RunFixtureClobber();
    return QolLootRun::Packed(native,selection);
}
int main(){
    int player{};expectedPlayer=&player;
    for(const auto selected:{2u,4u}) {
        selection=selected;observations=RunFixtureCalls=0;
        RunFixtureInvoke(&player,4,42);
        check(RunFixtureCalls==1 && observations==1);
        uint64_t value{};std::memcpy(&value,RunFixtureState,8);
        check(value==QolLootRun::Packed(0x123456789abc0011ULL,selected));
        for(unsigned i=1;i<7;++i){std::memcpy(&value,RunFixtureState+8*i,8);check(value==0x11*(i+1));}
        std::memcpy(&value,RunFixtureState+56,8);check((value&0x40)!=0);
        for(unsigned i=64;i<160;++i)check(RunFixtureState[i]==0xff);
    }
    std::puts("Run-choice bridge: native call once, packed return, arguments, GPR/SIMD/flags preserved.");
}
