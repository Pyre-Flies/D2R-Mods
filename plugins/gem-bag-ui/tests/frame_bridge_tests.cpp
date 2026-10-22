#include "frame_bridge.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {
using namespace GemBag::FrameBridge;
void Check(bool value,const char* text){if(!value){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
std::atomic<unsigned> hooks{},originals{};
std::atomic<bool> hold{},entered{},released{};
void __fastcall Original(void* first,void* second) noexcept {
    Check(first==reinterpret_cast<void*>(0x1234)&&second==reinterpret_cast<void*>(0x5678),"original gets both unchanged Win64 arguments");++originals;
}
void __fastcall Target(void* first,void* second) noexcept {
    ++hooks;entered=true;
    while(hold&&!released)Sleep(1);
    Original(first,second);
}
void Invoke(Call callback){callback(reinterpret_cast<void*>(0x1234),reinterpret_cast<void*>(0x5678));}
}
int main() {
    auto fixture=QolRelay::Allocate(reinterpret_cast<uintptr_t>(Original),4096);Check(fixture!=nullptr,"owned near-code fixture allocated");
    std::memset(fixture,0x90,64);const unsigned char start[]={0x48,0x83,0xec,0x28};std::memcpy(fixture,start,sizeof(start));
    const auto site=reinterpret_cast<uintptr_t>(fixture+19);
    const auto relative=static_cast<int32_t>(static_cast<int64_t>(reinterpret_cast<uintptr_t>(Original))-static_cast<int64_t>(site+5));
    fixture[19]=0xe8;std::memcpy(fixture+20,&relative,4);
    const unsigned char finish[]={0x48,0x83,0xc4,0x28,0xc3};std::memcpy(fixture+24,finish,sizeof(finish));
    std::array<unsigned char,16> before{};std::memcpy(before.data(),fixture+16,16);
    std::array<unsigned char,16> prefix{},suffix{};std::memcpy(prefix.data(),fixture,16);std::memcpy(suffix.data(),fixture+32,16);
    DWORD previous{};Check(VirtualProtect(fixture,4096,PAGE_EXECUTE_READ,&previous)!=0,"owned fixture sealed RX");FlushInstructionCache(GetCurrentProcess(),fixture,64);
    const auto call=reinterpret_cast<Call>(fixture);
    Bridge bridge;Check(bridge.Install(site,Original,Target,before)&&bridge.Owned(),"qualified call block installed atomically");
    for(size_t i=0;i<16;++i)if(i<4||i>7)Check(bridge.Published()[i]==before[i],"call patch changes only the rel32 operand");
    Invoke(call);Check(hooks==1&&originals==1&&bridge.Descriptor()->active==0,"actual relay enters target once and balances its in-flight count");
    Check(!std::memcmp(prefix.data(),fixture,16)&&!std::memcmp(suffix.data(),fixture+32,16),"bytes outside the aligned publication block are preserved");
    const auto retained=bridge.Relay();bridge.Stop();
    Check(!bridge.Owned()&&!std::memcmp(before.data(),fixture+16,16),"owned call restored on stop");
    Invoke(call);Invoke(retained);Check(hooks==1&&originals==3,"retained relay forwards original after target removal");
    Bridge foreign;Check(foreign.Install(site,Original,Target,before),"second owned fixture installation");
    auto changed=foreign.Published();changed[0]=0xcc;
    Check(Swap(site&~uintptr_t{15},foreign.Published(),changed).changed,"simulate a later call-block owner without executing it");
    const auto laterRelay=foreign.Relay();foreign.Stop();
    Check(!std::memcmp(changed.data(),fixture+16,16),"stop preserves a later owner's bytes");Invoke(laterRelay);
    Check(Swap(site&~uintptr_t{15},changed,before).changed,"restore only the owned test mutation");
    Bridge concurrent;Check(concurrent.Install(site,Original,Target,before),"concurrent teardown fixture installed");
    hold=true;entered=false;released=false;std::atomic<bool> stopped{};
    std::thread worker([&]{Invoke(call);});
    while(!entered)Sleep(1);
    std::thread stop([&]{concurrent.Stop();stopped=true;});
    while(InterlockedCompareExchangePointer(&concurrent.Descriptor()->target,nullptr,nullptr))Sleep(1);
    Check(!stopped,"teardown waits for an already-entered target");released=true;worker.join();stop.join();
    Check(stopped&&concurrent.Descriptor()->active==0,"teardown completes after in-flight target returns");
    const auto finalHooks=hooks.load();Invoke(concurrent.Relay());Check(hooks==finalHooks,"old relay cannot call a detached plugin target");
    std::puts("Executed Win64 call relay: unchanged arguments, original once, aligned atomic ownership, retained-relay forwarding and in-flight unload passed; owned code only.");
}
