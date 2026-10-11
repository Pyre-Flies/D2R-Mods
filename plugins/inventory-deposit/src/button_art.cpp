#include "button_art.h"
#include "button_art_resource.h"
#include <windows.h>

namespace InventoryDepositButtonArt {
bool Load(unsigned resourceId,Blob& output) noexcept {
    if(resourceId!=INVENTORY_DEPOSIT_BUTTON_SPRITE_RESOURCE_ID &&
        resourceId!=INVENTORY_DEPOSIT_BUTTON_LOWEND_RESOURCE_ID)return false;

    HMODULE module{};
    // A null module would inspect D2R.exe. Resolve the module containing this
    // implementation so art always comes from the plugin DLL being loaded.
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Load),&module))return false;
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(resourceId),MAKEINTRESOURCEW(10));
    if(!resource)return false;
    const auto size=SizeofResource(module,resource);
    if(!size || size>MaximumByteCount)return false;
    const auto loaded=LoadResource(module,resource);
    const auto bytes=loaded?LockResource(loaded):nullptr;
    if(!bytes)return false;
    output={bytes,size};
    return true;
}
}
