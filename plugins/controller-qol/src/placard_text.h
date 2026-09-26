#pragma once
#include <cstddef>
#include <cstring>
#include <algorithm>
namespace QolPlacardText {
// Restore only the exact text this instance wrote. Never strip a prefix merely
// because another plugin's text happens to look like a controller hint.
struct Lease {
    unsigned char original[512]{}, written[512]{};
    size_t bytes{};
    bool active{};
    bool Restore(void* buffer,size_t capacityBytes) noexcept {
        if(!active) return true;
        active=false;
        if(!buffer || bytes>capacityBytes || std::memcmp(buffer,written,bytes)) return false;
        std::memcpy(buffer,original,bytes);return true;
    }
    template<class C> bool Apply(C* buffer,size_t capacity,const C* prefix,size_t prefixLength) noexcept {
        if(!buffer || !prefix || !prefixLength) return false;
        capacity=std::min(capacity,sizeof(written)/sizeof(C));
        if(!Restore(buffer,capacity*sizeof(C))) return false;
        size_t length=0;
        while(length<capacity && buffer[length]) ++length;
        if(length==capacity || prefixLength>=capacity-length) return false;
        bytes=(length+prefixLength+1)*sizeof(C);
        // Original tail beyond its terminator is irrelevant; zero-fill it so
        // restoring the expanded span never copies uninitialized bytes.
        std::memset(original,0,bytes);
        std::memcpy(original,buffer,(length+1)*sizeof(C));
        std::memmove(buffer+prefixLength,buffer,(length+1)*sizeof(C));
        std::memcpy(buffer,prefix,prefixLength*sizeof(C));
        std::memcpy(written,buffer,bytes);active=true;return true;
    }
};
}
