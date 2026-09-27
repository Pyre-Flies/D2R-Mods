#pragma once
#include <cstdint>
namespace QolRangePolicy {
inline unsigned Mask(bool active,uintptr_t caller,uintptr_t tooltipReturn,unsigned index,unsigned mask) noexcept {
    return active && tooltipReturn && caller==tooltipReturn && index<8 && mask==0x800 ? 0x200 : mask;
}
}
