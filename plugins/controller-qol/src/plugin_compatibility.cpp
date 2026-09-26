#include "plugin_compatibility.h"
#include "compatibility_signatures.h"
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <cstring>

namespace QolCompat {
namespace {
bool PluginIdentity(uintptr_t address, bool potionOnly) noexcept {
    __try {
        HMODULE module{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(address), &module)) return false;
        auto getInfo = reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(module,"D2RLoaderGetPluginInfo"));
        if (!getInfo) return false;
        const auto* info = getInfo();
        if (!info || info->infoSize < D2RL::PluginInfoRequiredSize || !info->id || !*info->id) return false;
        if (!potionOnly) return true;
        return info->version && std::strcmp(info->id,"ruffneckk-potion-auto-pickup")==0 &&
            std::strcmp(info->version,"1.3.3")==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// Recognize the standard rel32 -> absolute relay or direct RIP-indirect jump.
// No arbitrary thunk execution and no unbounded chain following.
uintptr_t JumpTarget(uintptr_t address, size_t& length) noexcept {
    const auto* p = reinterpret_cast<const unsigned char*>(address);
    if (p[0]==0xE9) {
        int32_t displacement{}; std::memcpy(&displacement,p+1,4);
        length=5; return address+5+displacement;
    }
    if (p[0]==0xFF && p[1]==0x25) {
        int32_t displacement{}; std::memcpy(&displacement,p+2,4);
        // Entry must be the 14-byte inline absolute form. A relay may use it too.
        if (displacement!=0) return 0;
        uintptr_t target{};std::memcpy(&target,p+6,8);length=14;return target;
    }
    return 0;
}
}
bool IsPluginCaller(uintptr_t address) noexcept { return PluginIdentity(address,false); }
bool ValidateBeltEntry(uintptr_t address) noexcept {
    __try {
        const auto* p=reinterpret_cast<const unsigned char*>(address);
        if (std::memcmp(p,Native::BeltEntry,sizeof(Native::BeltEntry))==0) return true;
        size_t replaced=0;
        uintptr_t target=JumpTarget(address,replaced);
        if (!target || std::memcmp(p+replaced,Native::BeltEntry+replaced,
            sizeof(Native::BeltEntry)-replaced)!=0) return false;
        if (PluginIdentity(target,true)) return true;
        size_t relaySize=0;
        target=JumpTarget(target,relaySize);
        return target && PluginIdentity(target,true);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
