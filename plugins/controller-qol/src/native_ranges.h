#pragma once
#include <intrin.h>
#include <atomic>
#include "range_input_policy.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
namespace QolNativeRanges {
using Query=bool(__fastcall*)(void*,unsigned,unsigned);
inline Query original=nullptr;
inline uintptr_t tooltipReturn=0;
inline bool installed=false;
inline std::atomic<bool> active{false};
inline constexpr unsigned char callBytes[]={0x48,0x89,0xf1,0x89,0xc2,0x41,0xb8,0x00,0x08,0x00,0x00,0xff,0x15,0x16,0x4b,0xee,0xff,0x90,0xc7,0x85,0xec,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0xe9,0xcf,0xfc,0xff};
__declspec(noinline) inline bool __fastcall Hook(void* input,unsigned index,unsigned mask) noexcept {
    // Do not lock navigation/input state here: the native input reader also
    // calls this function while holding its own mutex. No raw state is changed.
    mask=QolRangePolicy::Mask(active.load(std::memory_order_relaxed),
        reinterpret_cast<uintptr_t>(_ReturnAddress()),tooltipReturn,index,mask);
    return original(input,index,mask);
}
inline bool Match(uintptr_t game,uintptr_t core) noexcept {
    __try {
        return !std::memcmp(reinterpret_cast<const void*>(core+0x819949),callBytes,sizeof(callBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+QolNativeProfile::ButtonRva),
                QolNativeProfile::ButtonBytes,sizeof(QolNativeProfile::ButtonBytes));
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline void Enable(bool enabled) noexcept {active.store(installed && enabled,std::memory_order_release);}
inline void Install(const D2RL::PluginContext* ctx) noexcept {
    const auto module=GetModuleHandleW(L"D2RCore.dll");
    if(!ctx || !QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash) ||
        !Match(ctx->exeBase,reinterpret_cast<uintptr_t>(module))) {
        if(ctx)ctx->LogWarn("[QOL/Ranges] Native query contract unavailable; range binding unchanged.");
        return;
    }
    tooltipReturn=reinterpret_cast<uintptr_t>(module)+0x81995a;
    installed=ctx->InstallInlineHook<Query>(QolNativeProfile::ButtonRva,
        QolNativeProfile::ButtonBytes,15,&Hook,&original);
    ctx->LogInfo(installed?"[QOL/Ranges] Native tooltip RT query remapped to RB; provider call site and Item Roll Ranges slots untouched.":
        "[QOL/Ranges] Query hook unavailable; range binding unchanged.");
}
}
