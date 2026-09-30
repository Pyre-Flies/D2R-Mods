#pragma once
#include <cstddef>
#include <span>
namespace Aim {
// Only the two documented QOL contact CALL displacements may differ. Their
// destination admission is separate; even CALL opcodes must match exactly.
inline bool SameEnumeration(std::span<const unsigned char> actual,
    std::span<const unsigned char> expected) noexcept {
    if(actual.size()!=expected.size() || actual.size()<0x98d) return false;
    for(std::size_t i=0;i<actual.size();++i) {
        if((i>=0x8de && i<0x8e2) || (i>=0x989 && i<0x98d)) continue;
        if(actual[i]!=expected[i]) return false;
    }
    return true;
}
}
