#pragma once
#include "startup_profile.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace TerrainProbe::Startup {
struct Summary {
    std::array<uint32_t,5> nonzeroBytes{},nonzeroRecords{};
    std::array<uint64_t,5> fingerprints{};
    uint32_t cpuMaterialSlots{},submittedSlots{};
    uintptr_t maskPointer{},defaultMaterial{};
    std::array<int32_t,3> dimensions{};
    std::array<int32_t,2> cpuOrigin{};
    std::array<int32_t,6> gpuState{};
    // Opaque owner bytes: nonzero is not interpreted as a valid resource.
    std::array<unsigned char,32> ownerPrefix{};
    bool operator==(const Summary&) const = default;
};

// Reader receives a module-relative address. Never follows a captured pointer.
template<class Reader> bool Capture(Reader&& reader,Summary& output) noexcept {
    Summary value{};
    std::array<unsigned char,400> record{};
    for(size_t arrayIndex=0;arrayIndex<5;++arrayIndex) {
        const auto& array=StartupProfile::Arrays[arrayIndex];
        if(array.stride>record.size() || array.count!=324) return false;
        uint64_t digest=14695981039346656037ULL;
        for(size_t cell=0;cell<array.count;++cell) {
            if(!reader(array.rva+cell*array.stride,record.data(),array.stride)) return false;
            bool occupied=false;
            for(size_t byte=0;byte<array.stride;++byte) {
                occupied=occupied || record[byte]!=0;
                value.nonzeroBytes[arrayIndex]+=record[byte]!=0?1U:0U;
                digest=(digest^record[byte])*1099511628211ULL;
            }
            value.nonzeroRecords[arrayIndex]+=occupied?1U:0U;
            if(arrayIndex==0 || arrayIndex==4) {
                uintptr_t pointer{};
                std::memcpy(&pointer,record.data()+(arrayIndex==0?16:0),sizeof(pointer));
                if(arrayIndex==0) value.cpuMaterialSlots+=pointer?1U:0U;
                else value.submittedSlots+=pointer?1U:0U;
            }
        }
        value.fingerprints[arrayIndex]=digest;
    }
    if(!reader(0x353b8b8,&value.maskPointer,sizeof(value.maskPointer)) ||
       !reader(0x34fb400,&value.defaultMaterial,sizeof(value.defaultMaterial)) ||
       !reader(0x34ff5f8,value.dimensions.data(),sizeof(value.dimensions)) ||
       !reader(0x34ff608,value.cpuOrigin.data(),sizeof(value.cpuOrigin)) ||
       !reader(0x353ade0,value.gpuState.data(),sizeof(value.gpuState)) ||
       !reader(0x34ff390,value.ownerPrefix.data(),value.ownerPrefix.size())) return false;
    output=value;return true;
}
}
