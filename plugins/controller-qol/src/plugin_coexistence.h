#pragma once
#include "plugin_coexistence_profile.h"
#include "plugin_coexistence_policy.h"
#include "chat_forwarding_memory.h"
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
inline bool Send(uintptr_t core,const unsigned char* original,size_t size,const char** refusal=nullptr) noexcept {
    if(refusal)*refusal="sender-contract";
    __try {
        if(!core || !original || size<14)return false;
        const auto entry=core+0x822dd0;
        if(Match(entry,original,size)){if(refusal)*refusal=nullptr;return true;}
        const auto module=GetModuleHandleW(L"d2rl-global-chat.dll");
        const auto chat=reinterpret_cast<uintptr_t>(module);
        // Follow only the exact absolute-jump form already used by this Core.
        // The graph verifies every reachable non-chat helper, data owner and
        // original continuation; unrelated companion layout/content may move.
        uintptr_t hook{};
        std::memcpy(&hook,reinterpret_cast<const void*>(entry+6),sizeof(hook));
        QolChatForwarding::ImageMemory image{};
        if(chat && AbsoluteJump(reinterpret_cast<const unsigned char*>(entry),hook) && image.Open(module)) {
            const auto result=QolChatForwarding::Validate(image.Reader(),hook,
                reinterpret_cast<const unsigned char*>(entry),original,size,entry+14);
            if(result){if(refusal)*refusal=nullptr;return true;}
            if(refusal)*refusal=result.reason;
        } else if(refusal)*refusal="sender-detour-owner-or-image";
        // Both historical Chat artifacts satisfy this same complete graph.
        // An on-disk hash cannot excuse changed helpers or memory ownership in
        // the loaded image. A failed forwarding proof is always final.
        return false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {if(refusal)*refusal="sender-read-fault";return false;}
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
        if(!maps)return false;
        uintptr_t continuation{};
        if(QolCore::VerifyFileHash(module,MapsHash) && Match(maps+0x106d0,MapsHook,sizeof(MapsHook)))
            continuation=maps+0xbcd30;
        else if(QolCore::VerifyFileHash(module,MapsCurrentHash) && Match(maps+0x106d0,MapsCurrentHook,sizeof(MapsCurrentHook)))
            continuation=maps+0xbdd30;
        else return false;
        return Executable(actual) && StatRoute(actual,original,maps+0x106d0,
            *reinterpret_cast<const uintptr_t*>(continuation),true);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
