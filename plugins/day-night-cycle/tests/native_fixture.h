#pragma once
#include "native_profile.h"
#include "owned_patch.h"
#include "test.h"
#include <vector>

namespace Fixture {
struct Time {float blend;int32_t period;};
static_assert(sizeof(Time)==8);
using Setter = void(__fastcall*)(void*,float,int32_t);
inline unsigned calls{};
inline void* lastEnvironment{};
inline Time lastTime{};
inline Setter nativeSetter{};
inline void __fastcall CountedSetter(void* environment,float blend,int32_t period) {
    ++calls;lastEnvironment=environment;lastTime={blend,period};
    nativeSetter(environment,blend,period);
}

// Copy the witnessed caller -> thunk -> Core wrapper -> native setter path.
// Relocate only its address operands into owned memory. No game is loaded.
class NativePath {
    unsigned char* code_{};
    std::vector<unsigned char> renderer_;
    void Displacement(size_t position,size_t end,size_t destination) {
        const auto value=static_cast<int32_t>(destination)-static_cast<int32_t>(end);
        std::memcpy(code_+position,&value,4);
    }
    void Pointer(size_t position,uintptr_t value) {std::memcpy(code_+position,&value,sizeof(value));}
public:
    DayNight::Block before{},after{};
    DayNight::OwnedPatch patch;
    NativePath():renderer_(0x11920,0xa5) {
        using namespace DayNight::Profile;
        code_=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        Check(code_!=nullptr,"executable fixture allocation");
        std::memcpy(code_,CoreWrapper.data(),CoreWrapper.size());
        std::memcpy(code_+0x100,GameCaller.data(),GameCaller.size());
        std::memcpy(code_+0x200,NativeSetter.data(),NativeSetter.size());
        std::memcpy(code_+0x300,GameThunk.data(),GameThunk.size());
        // renderer getter: MOV RAX, immediate64; RET.
        code_[0x400]=0x48;code_[0x401]=0xb8;code_[0x40a]=0xc3;
        Pointer(0x402,reinterpret_cast<uintptr_t>(renderer_.data()));
        Displacement(21,25,0x1500); // Core mode/safety flag
        Displacement(32,36,0x1510); // native NoDebug pointer
        Displacement(43,47,0x1520); // native setter pointer
        Pointer(0x1510,reinterpret_cast<uintptr_t>(code_+0x1501));
        Pointer(0x1520,reinterpret_cast<uintptr_t>(CountedSetter));
        Pointer(0x1530,reinterpret_cast<uintptr_t>(code_));
        Displacement(0x10a,0x10e,0x400); // renderer getter call
        Displacement(0x12c,0x130,0x300); // native caller's tail jump
        Displacement(0x302,0x306,0x1530); // thunk pointer
        std::memcpy(before.data(),code_+32,16);after=before;
        after[CoreBranchRva-CoreBlockRva]=0x90;
        after[CoreBranchRva-CoreBlockRva+1]=0x90;
        DWORD old{};
        Check(VirtualProtect(code_,4096,PAGE_EXECUTE_READ,&old)!=FALSE,"fixture executable protection");
        FlushInstructionCache(GetCurrentProcess(),code_,4096);
        nativeSetter=reinterpret_cast<Setter>(code_+0x200);
    }
    ~NativePath() {patch.Stop();VirtualFree(code_,0,MEM_RELEASE);}
    uintptr_t PatchAddress() const {return reinterpret_cast<uintptr_t>(code_+32);}
    void Flags(unsigned char noDebug,unsigned char safety,unsigned char overrideTime) {
        code_[0x1501]=noDebug;code_[0x1500]=safety;
        renderer_[DayNight::Profile::RendererOverrideOffset]=overrideTime;
    }
    unsigned char NoDebug() const {return code_[0x1501];}
    unsigned char Safety() const {return code_[0x1500];}
    bool Run(Time value) {
        const auto snapshot=renderer_;
        calls=0;lastEnvironment=nullptr;
        reinterpret_cast<void(__fastcall*)(void*,const Time*)>(code_+0x100)(nullptr,&value);
        if(!calls) {Check(snapshot==renderer_,"suppressed path preserves renderer");return false;}
        Check(calls==1,"setter called exactly once");
        Check(lastEnvironment==renderer_.data()+DayNight::Profile::RendererEnvironmentOffset,"RCX environment preserved");
        Check(std::memcmp(&lastTime,&value,sizeof(value))==0,"XMM1 blend and R8D period preserved");
        auto expected=snapshot;
        const auto environment=DayNight::Profile::RendererEnvironmentOffset;
        std::memcpy(expected.data()+environment+DayNight::Profile::EnvironmentPeriodOffset,&value.period,4);
        std::memcpy(expected.data()+environment+DayNight::Profile::EnvironmentBlendOffset,&value.blend,4);
        Check(expected==renderer_,"native setter changes only the two witnessed fields");
        return true;
    }
};
}
