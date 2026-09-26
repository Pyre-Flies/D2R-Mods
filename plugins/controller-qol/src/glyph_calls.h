#pragma once
#include <cstdint>
#include <cstring>
#include <limits>
namespace QolGlyphCalls {
inline bool Encode(uintptr_t site,uintptr_t target,unsigned char (&bytes)[5]) noexcept {
    const auto delta=static_cast<int64_t>(target)-static_cast<int64_t>(site+5);
    if(delta<std::numeric_limits<int32_t>::min() || delta>std::numeric_limits<int32_t>::max()) return false;
    bytes[0]=0xe8;const auto relative=static_cast<int32_t>(delta);std::memcpy(bytes+1,&relative,4);return true;
}
inline constexpr uint64_t Sites[]={0x86D6A9,0x86D6F3};
inline constexpr unsigned char Expected[][5]={{0xe8,0x72,0x57,0x09,0x00},{0xe8,0x28,0x57,0x09,0x00}};
}
