#pragma once
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdint>
#include <cstring>

namespace DayNight {
using Block = std::array<unsigned char,16>;

inline bool Read(uintptr_t address,void* output,size_t size) noexcept {
    if(!address || !output || !size || address>UINTPTR_MAX-size) return false;
    SIZE_T count{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,size,&count) && count==size;
}
template<size_t N> bool Bytes(uintptr_t address,const std::array<unsigned char,N>& expected) noexcept {
    std::array<unsigned char,N> actual{};
    return Read(address,actual.data(),N) && actual==expected;
}

enum class PatchResult { Applied, Restored, Refused, Conflict, ProtectionFailed, ProtectionRestoreFailed, NotOwned };

// Core is outside the SDK mutation service's executable-relative address space.
// An aligned CMPXCHG16B publishes both changed instruction bytes together. All
// other bytes in the block remain identical; no plugin function is a target.
class OwnedPatch {
    uintptr_t address_{};
    Block original_{},replacement_{};
    DWORD protection_{};
    bool originalNeedsProtection_{};

    static bool Exchange(uintptr_t address,const Block& before,const Block& after) noexcept {
        alignas(16) long long comparand[2]{};
        long long desired[2]{};
        std::memcpy(comparand,before.data(),16);
        std::memcpy(desired,after.data(),16);
        return _InterlockedCompareExchange128(reinterpret_cast<volatile long long*>(address),
            desired[1],desired[0],comparand)!=0;
    }
public:
    bool Installed() const noexcept {return address_!=0;}
    bool Owned() const noexcept {return address_ && !originalNeedsProtection_ && Bytes(address_,replacement_);}

    PatchResult Install(uintptr_t address,const Block& before,const Block& after) noexcept {
        if(address_ || !address || (address&15) || before==after || !IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128))
            return PatchResult::Refused;
        MEMORY_BASIC_INFORMATION memory{};
        if(!VirtualQuery(reinterpret_cast<void*>(address),&memory,sizeof(memory)) || memory.State!=MEM_COMMIT ||
           (memory.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
           !(memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) ||
           !Bytes(address,before)) return PatchResult::Refused;
        DWORD old{};
        if(!VirtualProtect(reinterpret_cast<void*>(address),16,PAGE_EXECUTE_READWRITE,&old))
            return PatchResult::ProtectionFailed;
        const bool changed=Exchange(address,before,after);
        if(changed) {
            address_=address;original_=before;replacement_=after;protection_=old;originalNeedsProtection_=false;
            FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),16);
        }
        DWORD ignored{};
        const bool restored=VirtualProtect(reinterpret_cast<void*>(address),16,old,&ignored)!=FALSE;
        if(!restored) {
            if(!changed) {
                address_=address;original_=before;replacement_=after;protection_=old;originalNeedsProtection_=true;
            }
            return PatchResult::ProtectionRestoreFailed;
        }
        return changed?PatchResult::Applied:PatchResult::Conflict;
    }

    PatchResult Stop() noexcept {
        if(!address_) return PatchResult::NotOwned;
        const auto address=address_;
        if(originalNeedsProtection_) {
            if(!Bytes(address,original_)) {address_=0;originalNeedsProtection_=false;return PatchResult::NotOwned;}
            DWORD ignored{};
            if(!VirtualProtect(reinterpret_cast<void*>(address),16,protection_,&ignored))
                return PatchResult::ProtectionRestoreFailed;
            originalNeedsProtection_=false;address_=0;return PatchResult::Restored;
        }
        if(!Bytes(address,replacement_)) {address_=0;return PatchResult::NotOwned;}
        DWORD old{};
        if(!VirtualProtect(reinterpret_cast<void*>(address),16,PAGE_EXECUTE_READWRITE,&old))
            return PatchResult::ProtectionFailed;
        const bool changed=Exchange(address,replacement_,original_);
        FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),16);
        DWORD ignored{};
        const bool restored=VirtualProtect(reinterpret_cast<void*>(address),16,changed?protection_:old,&ignored)!=FALSE;
        // Retry protection only while our exact restored bytes remain present.
        if(restored || !changed) address_=0;
        else originalNeedsProtection_=true;
        if(!restored) return PatchResult::ProtectionRestoreFailed;
        return changed?PatchResult::Restored:PatchResult::NotOwned;
    }
};
}
