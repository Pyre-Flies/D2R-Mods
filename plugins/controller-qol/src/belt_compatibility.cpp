#include <windows.h>
#include <mutex>
#include <cstring>
#include "belt_signatures.h"
#include "auto_belt_profile.h"
#include "core_compatibility.h"
#include "belt_compatibility.h"
namespace QolBeltCompat {
bool Match(uintptr_t address,const unsigned char* expected,size_t size) noexcept {
    __try {return address && !std::memcmp(reinterpret_cast<const void*>(address),expected,size);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReviewedHash(HMODULE module) noexcept {
    static std::once_flag once;static HMODULE reviewed{};static bool hashOk{};
    std::call_once(once,[module]{reviewed=module;hashOk=QolCore::VerifyFileHash(module,QolAutoBeltProfile::Hash);});
    return module==reviewed && hashOk;
}
bool Stored(uintptr_t entry) noexcept {
    using namespace QolBelt::Signatures;
    if(Match(entry,PlaceStored,sizeof(PlaceStored)))return true;
    __try {
        const auto* code=reinterpret_cast<const unsigned char*>(entry);
        if(code[0]!=0xe9 || !Match(entry+5,PlaceStored+5,sizeof(PlaceStored)-5))return false;
        int32_t displacement{};std::memcpy(&displacement,code+1,4);
        const auto relay=entry+5+displacement;
        constexpr unsigned char jump[]={0xff,0x25,0,0,0,0};
        if(!Match(relay,jump,sizeof(jump)))return false;
        const auto target=*reinterpret_cast<const uintptr_t*>(relay+6);
        HMODULE module{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(target),&module))return false;
        const auto base=reinterpret_cast<uintptr_t>(module);
        if(target!=base+0xb7f0)return false;
        // Cache file validation; all live code/link witnesses are checked each batch.
        if(!ReviewedHash(module) || !Match(target,QolAutoBeltProfile::Wrapper,sizeof(QolAutoBeltProfile::Wrapper)))return false;
        const auto trampoline=*reinterpret_cast<const uintptr_t*>(base+0x136438);
        if(!Match(trampoline,PlaceStored,5) || !Match(trampoline+5,jump,sizeof(jump)))return false;
        return *reinterpret_cast<const uintptr_t*>(trampoline+11)==entry+5;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
