#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
namespace QolCoexistence {
inline bool AbsoluteJump(const unsigned char* bytes,uintptr_t target) noexcept {
    constexpr unsigned char prefix[]={0xff,0x25,0,0,0,0};
    uintptr_t actual{};
    if(!bytes || !target || std::memcmp(bytes,prefix,sizeof(prefix)))return false;
    std::memcpy(&actual,bytes+6,sizeof(actual));
    return actual==target;
}
// Buffers are readable for size, size and 28 bytes respectively. The original
// sender's first 14 bytes contain no RIP-relative instructions.
inline bool SendChain(const unsigned char* entry,const unsigned char* original,size_t size,
    uintptr_t hook,const unsigned char* trampoline,uintptr_t resume,bool verifiedOwner) noexcept {
    return verifiedOwner && entry && original && trampoline && size>=14 &&
        AbsoluteJump(entry,hook) && !std::memcmp(entry+14,original+14,size-14) &&
        !std::memcmp(trampoline,original,14) && AbsoluteJump(trampoline+14,resume);
}
inline bool StatRoute(uintptr_t actual,uintptr_t original,uintptr_t hook,uintptr_t continuation,bool verifiedOwner) noexcept {
    return original && (actual==original || (verifiedOwner && hook && actual==hook && continuation==original));
}
}
