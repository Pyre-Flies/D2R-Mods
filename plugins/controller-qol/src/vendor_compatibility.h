#pragma once
#include "vendor_signatures.h"
#include <windows.h>
#include <cstring>
#include <cstdint>
namespace QolVendor {
inline constexpr size_t RelocatedCalls[]={0x218,0x256,0x28C};
inline bool TransactionBodyMatches(const unsigned char* code) noexcept {
    for(size_t i=0;i<sizeof(TransactionBytes);++i) {
        bool displacement=false;
        for(auto call:RelocatedCalls) if(i>call && i<=call+4) displacement=true;
        if(!displacement && code[i]!=TransactionBytes[i]) return false;
    }
    return true;
}
inline bool ValidateTransaction(uintptr_t base) noexcept {
    __try {
        const auto* code=reinterpret_cast<const unsigned char*>(base+0x10D160);
        if(!std::memcmp(code,TransactionBytes,sizeof(TransactionBytes))) return true;
        if(!TransactionBodyMatches(code)) return false;
        const auto core=GetModuleHandleW(L"D2RCore.dll");
        if(!core) return false;
        for(auto call:RelocatedCalls) {
            int32_t displacement{};std::memcpy(&displacement,code+call+1,4);
            const auto* thunk=code+call+5+displacement;
            if(thunk[0]!=0xFF || thunk[1]!=0x25) return false;
            int32_t slot{};std::memcpy(&slot,thunk+2,4);
            uintptr_t target{};std::memcpy(&target,thunk+6+slot,sizeof(target));
            const auto expected=GetProcAddress(core,call==0x218?
                "ResolveNamespacedStringKey":"GetNamespacedStringById");
            if(!expected || target!=reinterpret_cast<uintptr_t>(expected)) return false;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
