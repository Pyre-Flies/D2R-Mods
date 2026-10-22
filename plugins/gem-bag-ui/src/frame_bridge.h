#pragma once
#include "relay_allocation.h"
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <new>

namespace GemBag::FrameBridge {
using Call=void(__fastcall*)(void*,void*) noexcept;
struct alignas(16) State {void* volatile target{};Call original{};volatile LONG active{};LONG reserved[3]{};};
static_assert(offsetof(State,original)==8&&offsetof(State,active)==16);
inline bool Read(uintptr_t address,void* data,size_t size) noexcept {
    SIZE_T got{};return address>=0x10000&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),data,size,&got)&&got==size;
}
struct Exchange {bool changed{},protectionRestored{};};
inline Exchange Swap(uintptr_t address,const std::array<unsigned char,16>& expected,const std::array<unsigned char,16>& desired) noexcept {
    if(address%16||!IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128))return {};
    std::array<unsigned char,16> current{};if(!Read(address,current.data(),current.size())||current!=expected)return {};
    DWORD previous{};if(!VirtualProtect(reinterpret_cast<void*>(address),16,PAGE_EXECUTE_READWRITE,&previous))return {};
    alignas(16) LONG64 comparand[2]{},replacement[2]{};
    std::memcpy(comparand,expected.data(),16);std::memcpy(replacement,desired.data(),16);
    const bool changed=InterlockedCompareExchange128(reinterpret_cast<volatile LONG64*>(address),replacement[1],replacement[0],comparand)!=0;
    if(changed)FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),16);
    DWORD unused{};const bool restored=VirtualProtect(reinterpret_cast<void*>(address),16,previous,&unused)!=0;
    return {changed,restored};
}
inline std::array<unsigned char,64> RelayBytes(const State* state) noexcept {
    // Two ordinary Win64 pointer arguments stay in RCX/RDX. Count entry before
    // loading the target, so clearing it then draining covers unload races.
    std::array<unsigned char,64> bytes{};bytes.fill(0xcc);
    constexpr unsigned char body[]={0x48,0x83,0xec,0x38,0x49,0xbb,0,0,0,0,0,0,0,0,
        0xf0,0x41,0xff,0x43,0x10,0x49,0x8b,0x03,0x48,0x85,0xc0,0x75,0x04,0x49,0x8b,0x43,0x08,
        0xff,0xd0,0x49,0xbb,0,0,0,0,0,0,0,0,0xf0,0x41,0xff,0x4b,0x10,0x48,0x83,0xc4,0x38,0xc3};
    std::memcpy(bytes.data(),body,sizeof(body));const auto address=reinterpret_cast<uintptr_t>(state);
    std::memcpy(bytes.data()+6,&address,8);std::memcpy(bytes.data()+35,&address,8);return bytes;
}
class Bridge {
public:
    Bridge()=default;Bridge(const Bridge&)=delete;Bridge& operator=(const Bridge&)=delete;
    ~Bridge(){Stop();}
    bool Install(uintptr_t site,Call original,Call target,const std::array<unsigned char,16>& expected) noexcept {
        if(state||!site||!original||!target)return false;
        block=site&~uintptr_t{15};const auto offset=site-block;
        if(offset>11||expected[offset]!=0xe8)return false;
        int32_t oldRelative{};std::memcpy(&oldRelative,expected.data()+offset+1,4);
        if(static_cast<uintptr_t>(static_cast<int64_t>(site+5)+oldRelative)!=reinterpret_cast<uintptr_t>(original))return false;
        auto memory=QolRelay::Allocate(site,4096);
        const auto stateMemory=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        state=stateMemory?new(stateMemory) State{}:nullptr;
        if(!memory||!state){if(memory)VirtualFree(memory,0,MEM_RELEASE);if(state)VirtualFree(state,0,MEM_RELEASE);state=nullptr;return false;}
        state->original=original;const auto bytes=RelayBytes(state);std::memcpy(memory,bytes.data(),bytes.size());
        DWORD previous{};
        const auto delta=static_cast<int64_t>(reinterpret_cast<uintptr_t>(memory))-static_cast<int64_t>(site+5);
        if(delta<INT32_MIN||delta>INT32_MAX||!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&previous)) {
            VirtualFree(memory,0,MEM_RELEASE);VirtualFree(state,0,MEM_RELEASE);state=nullptr;return false;
        }
        FlushInstructionCache(GetCurrentProcess(),memory,bytes.size());relay=reinterpret_cast<Call>(memory);
        before=after=expected;const auto relative=static_cast<int32_t>(delta);std::memcpy(after.data()+offset+1,&relative,4);
        InterlockedExchangePointer(&state->target,reinterpret_cast<void*>(target));
        const auto published=Swap(block,before,after);
        if(!published.changed){InterlockedExchangePointer(&state->target,nullptr);VirtualFree(memory,0,MEM_RELEASE);VirtualFree(state,0,MEM_RELEASE);state=nullptr;relay=nullptr;return false;}
        // Once reachable, the tiny RX relay and RW descriptor stay allocated
        // until process exit. A later hook may retain that relay after Stop.
        if(!published.protectionRestored){Stop();return false;}
        return true;
    }
    bool Owned() const noexcept {
        std::array<unsigned char,16> current{};return state&&Read(block,current.data(),16)&&current==after;
    }
    void Stop() noexcept {
        if(!state)return;
        auto prior=InterlockedExchangePointer(&state->target,nullptr);
        if(Owned())Swap(block,after,before); // Never replace a later owner's bytes.
        const auto start=GetTickCount64();
        while(InterlockedCompareExchange(&state->active,0,0)) {
            if(GetTickCount64()-start>2000&&prior) {
                HMODULE retained{};
                if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(prior),&retained))return;
            }
            Sleep(1);
        }
    }
    Call Relay() const noexcept{return relay;}
    State* Descriptor() const noexcept{return state;}
    const std::array<unsigned char,16>& Published() const noexcept{return after;}
private:
    State* state{};Call relay{};uintptr_t block{};
    std::array<unsigned char,16> before{},after{};
};
}
