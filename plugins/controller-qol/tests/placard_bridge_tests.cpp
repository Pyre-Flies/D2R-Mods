#include "placard_text.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
    void PlacardFixtureQuery();
    void PlacardFixtureClobber();
    void PlacardFixtureInvoke(void*,void*);
    void* QolPlacardNativeQuery=reinterpret_cast<void*>(&PlacardFixtureQuery);
    alignas(16) unsigned char PlacardFixtureState[160]{};
    unsigned PlacardFixtureCalls=0;
}
static unsigned observations=0;
static void* expectedUnit;
static void* expectedOutput;
static QolPlacardText::Lease lease;
static void check(bool ok){if(!ok)std::abort();}
extern "C" void QolPlacardObserveCompletedName(void* unit,void* output) noexcept {
    check(unit==expectedUnit && output==expectedOutput);
    ++observations;
    check(lease.Apply(static_cast<char*>(output)+4,124,"[X] ",4));
    PlacardFixtureClobber();
}
int main(){
    unsigned unit[4]={4,0,42,0};
    char output[128]{};
    std::memcpy(output,"HEAD",4);
    std::strcpy(output+4,"\xff" "c4Unhoarder name [Missing]");
    expectedUnit=unit;expectedOutput=output;
    PlacardFixtureInvoke(unit,output);
    check(PlacardFixtureCalls==1 && observations==1);
    for(unsigned i=0;i<7;++i){std::uint64_t value{};std::memcpy(&value,PlacardFixtureState+8*i,8);check(value==0x11*(i+1));}
    std::uint64_t flags{};std::memcpy(&flags,PlacardFixtureState+56,8);check((flags&0x40)!=0);
    for(unsigned i=64;i<160;++i)check(PlacardFixtureState[i]==0xff);
    check(std::memcmp(output,"HEAD",4)==0);
    check(std::strcmp(output+4,"[X] \xff" "c4Unhoarder name [Missing]")==0);
    check(lease.Restore(output+4,124));
    check(std::strcmp(output+4,"\xff" "c4Unhoarder name [Missing]")==0);
    std::puts("Post-name bridge: native call once, arguments, GPR/SIMD/flags, metadata and foreign text preserved.");
}
