#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <limits>

namespace QolXInput {
struct Record {
    uint8_t* targetProc{};
    void* relay{};
    void* trampoline{};
    bool installed{};
};
inline bool Executable(const void* p) noexcept {
    MEMORY_BASIC_INFORMATION m{};
    if (!p || !VirtualQuery(p,&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&PAGE_GUARD)) return false;
    return (m.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
inline bool Pin(const void* p) noexcept {
    HMODULE module{};
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(p),&module)!=FALSE;
}
inline void Jump(uint8_t* p, const void* destination) noexcept {
    const uint8_t opcode[]={0xff,0x25,0,0,0,0};
    std::memcpy(p,opcode,6);std::memcpy(p+6,&destination,8);
}
// Only complete, position-independent MOV [rsp+8],rbx or a rel32 JMP.
inline bool Plan(const uint8_t* bytes, uintptr_t address, uint8_t* trampoline, const void*& predecessor) noexcept {
    constexpr uint8_t mov[]={0x48,0x89,0x5c,0x24,0x08};
    if (!std::memcmp(bytes,mov,5)) {
        std::memcpy(trampoline,bytes,5);Jump(trampoline+5,reinterpret_cast<void*>(address+5));
        predecessor=reinterpret_cast<void*>(address);return true;
    }
    if(bytes[0]!=0xe9) return false;
    int32_t displacement{};std::memcpy(&displacement,bytes+1,4);
    predecessor=reinterpret_cast<void*>(address+5+displacement);
    return true;
}
inline bool Publish(uint8_t* target, uint64_t expected, uint64_t replacement) noexcept {
    if(reinterpret_cast<uintptr_t>(target)%8) return false;
    DWORD old{};
    if(!VirtualProtect(target,8,PAGE_EXECUTE_READWRITE,&old)) return false;
    const auto previous=InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(target),
        static_cast<LONG64>(replacement),static_cast<LONG64>(expected));
    DWORD ignored{};VirtualProtect(target,8,old,&ignored);
    FlushInstructionCache(GetCurrentProcess(),target,8);
    return static_cast<uint64_t>(previous)==expected;
}
inline bool Install(uint8_t* target,void* detour,void* relay,Record& record) noexcept {
    if(!target || !relay || !detour || record.installed || reinterpret_cast<uintptr_t>(target)%8 || !Executable(target)) return false;
    uint64_t expected{};std::memcpy(&expected,target,8);
    auto* bytes=reinterpret_cast<uint8_t*>(&expected);
    auto* code=static_cast<uint8_t*>(relay);
    const void* predecessor{};
    if(!Plan(bytes,reinterpret_cast<uintptr_t>(target),code+16,predecessor)) return false;
    if(bytes[0]==0xe9) {
        // A module-backed predecessor has a pin-able lifetime. Private relays
        // have no public owner/lifetime contract and are deliberately rejected.
        if(!Executable(predecessor) || !Pin(predecessor)) return false;
        Jump(code+16,predecessor);
    }
    if(!Pin(target) || !Pin(detour)) return false;
    const auto delta=reinterpret_cast<intptr_t>(relay)-(reinterpret_cast<intptr_t>(target)+5);
    if(delta<std::numeric_limits<int32_t>::min() || delta>std::numeric_limits<int32_t>::max()) return false;
    Jump(code,detour);
    DWORD old{};
    if(!VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old)) return false;
    FlushInstructionCache(GetCurrentProcess(),relay,64);
    uint64_t replacement=expected;auto* patch=reinterpret_cast<uint8_t*>(&replacement);
    patch[0]=0xe9;const auto relative=static_cast<int32_t>(delta);std::memcpy(patch+1,&relative,4);
    // Publish the predecessor before any caller can enter the detour.
    record.targetProc=target;record.relay=relay;record.trampoline=code+16;
    if(!Publish(target,expected,replacement)) {record={};return false;}
    record.installed=true;
    return true;
}
// No executable-byte restoration on shutdown: later owners may chain into us.
// Once published, records and RX relay pages live until process exit.
struct Gate {
    SRWLOCK lock=SRWLOCK_INIT;
    bool active=false;
    void Enable() noexcept {AcquireSRWLockExclusive(&lock);active=true;ReleaseSRWLockExclusive(&lock);}
    void Disable() noexcept {AcquireSRWLockExclusive(&lock);active=false;ReleaseSRWLockExclusive(&lock);}
    template<class F> void Run(F callback) {
        static thread_local bool processing=false;
        if(processing) return;
        struct Guard {
            SRWLOCK* lock; bool& processing;
            Guard(SRWLOCK* p,bool& b):lock(p),processing(b) {processing=true;AcquireSRWLockShared(lock);}
            ~Guard() {ReleaseSRWLockShared(lock);processing=false;}
        } guard(&lock,processing);
        if(active) callback();
    }
};
}
