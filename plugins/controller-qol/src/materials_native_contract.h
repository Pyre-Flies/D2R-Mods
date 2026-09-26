#pragma once
#include <cstdint>
namespace QolMaterials {
using WithdrawWidgetFn = void(__fastcall*)(void*,const int32_t*,uint8_t);
using WithdrawOneFn = void(__fastcall*)(void*,void*,uint8_t);
inline bool SubmitInventory(WithdrawWidgetFn fn,void* widget) noexcept {
    if (!fn || !widget) return false;
    const int32_t cell[2]{};
    fn(widget,cell,0); // widget page 0 is mapped by native code to destination 1
    return true;
}
inline bool SubmitBelt(WithdrawOneFn fn,void* item,void* owner) noexcept {
    if (!fn || !item || !owner) return false;
    fn(item,owner,3); // native advanced-stash destination 3; no fake cursor item
    return true;
}
}
