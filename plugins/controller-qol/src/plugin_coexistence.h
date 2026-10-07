#pragma once
#include "plugin_coexistence_profile.h"
#include "plugin_coexistence_policy.h"
#include "core_compatibility.h"
#include <windows.h>
namespace QolCoexistence {
inline bool Executable(uintptr_t address) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    return address && VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info)) &&
        info.State==MEM_COMMIT && !(info.Protect&PAGE_GUARD) &&
        (info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
inline bool Match(uintptr_t address,const unsigned char* bytes,size_t size) noexcept {
    return !std::memcmp(reinterpret_cast<const void*>(address),bytes,size);
}
// These checks admit known forwarding hooks only. Callers still invoke the
// engine entry/slot, retaining all installed plugin behavior and SDK ownership.
inline bool Send(uintptr_t core,const unsigned char* original,size_t size) noexcept {
    __try {
        if(!core)return false;
        const auto entry=core+0x822dd0;
        if(Match(entry,original,size))return true;
        const auto module=GetModuleHandleW(L"d2rl-global-chat.dll");
        const auto chat=reinterpret_cast<uintptr_t>(module);
        using namespace QolCoexistenceProfile;
        if(!chat || !QolCore::VerifyFileHash(module,ChatHash) ||
            !Match(chat+0x538b0,ChatHook,sizeof(ChatHook)) ||
            !Match(chat+0x53730,ChatDispatch,sizeof(ChatDispatch)) ||
            !Match(chat+0x17cc0,ChatOriginalGetter,sizeof(ChatOriginalGetter)))return false;
        const auto trampoline=*reinterpret_cast<const uintptr_t*>(chat+0xa75c8);
        return Executable(chat+0x538b0) && Executable(trampoline) &&
            SendChain(reinterpret_cast<const unsigned char*>(entry),original,size,chat+0x538b0,
                reinterpret_cast<const unsigned char*>(trampoline),entry+14,true);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
inline bool StatSlot(uintptr_t game,uintptr_t core) noexcept {
    __try {
        if(!game || !core)return false;
        const auto actual=*reinterpret_cast<const uintptr_t*>(game+0x3e2a218);
        const auto original=core+0x831de0;
        if(actual==original)return true;
        const auto module=GetModuleHandleW(L"d2rl-maps.dll");
        const auto maps=reinterpret_cast<uintptr_t>(module);
        using namespace QolCoexistenceProfile;
        if(!maps || !QolCore::VerifyFileHash(module,MapsHash) ||
            !Match(maps+0x106d0,MapsHook,sizeof(MapsHook)))return false;
        return Executable(actual) && StatRoute(actual,original,maps+0x106d0,
            *reinterpret_cast<const uintptr_t*>(maps+0xbcd30),true);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
