#pragma once
#include "ground_supply_model.h"
#include "ground_supply_budget.h"
#include "ground_supply_profile.h"
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>
#include <atomic>
#include <cstring>

namespace TerrainProbe::GroundSupply {
inline std::atomic<bool> enabled{},inGame{};
inline std::atomic_flag busy=ATOMIC_FLAG_INIT;
inline std::atomic<uint64_t> expandedCalls{},forwardedCalls{},refusedCalls{};
inline std::atomic<uint64_t> noAdditionCalls{};
inline std::atomic<uint64_t> budgetRefusals{},clockRefusals{},readRefusals{},unstableRefusals{},invalidRefusals{};
inline std::atomic<unsigned> lastAdded{};
inline uintptr_t base{};
inline int64_t budgetTicks{};
inline void* originalAddress{};
using Worker=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
inline bool Read(uintptr_t address,void* output,size_t size) noexcept {
    if(!address || size>4096 || address>0x7fffffffffffULL-size) return false;
    SIZE_T copied{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,size,&copied)!=FALSE && copied==size;
}
inline uintptr_t __fastcall Hook(uintptr_t owner,uintptr_t view,uintptr_t arg3,uintptr_t arg4) noexcept {
    const auto original=reinterpret_cast<Worker>(originalAddress);
    if(!enabled.load() || !inGame.load() || reinterpret_cast<uintptr_t>(_ReturnAddress())!=base+Native::CallerReturn || busy.test_and_set())
        return original(owner,view,arg3,arg4);
    const auto clock=[](int64_t& value) noexcept {
        LARGE_INTEGER counter{};
        if(!QueryPerformanceCounter(&counter)) return false;
        value=counter.QuadPart;return true;
    };
    int64_t start{};
    const bool started=clock(start);
    const auto raw=[](uintptr_t address,void* output,size_t size) noexcept {return Read(address,output,size);};
    BudgetReader read{clock,raw,start,budgetTicks,start};
    if(!started) read.failure=ReadFailure::Clock;
    unsigned added{};Status status{};
    const auto result=Forward(true,base,owner,view,arg3,arg4,read,original,added,status);
    lastAdded.store(added);
    if(added) ++expandedCalls;else ++forwardedCalls;
    if(status==Status::Complete && !added) ++noAdditionCalls;
    if(status==Status::Refused || status==Status::Unstable) {
        ++refusedCalls;
        if(read.failure==ReadFailure::Budget) ++budgetRefusals;
        else if(read.failure==ReadFailure::Clock) ++clockRefusals;
        else if(read.failure==ReadFailure::Memory) ++readRefusals;
        else if(status==Status::Unstable) ++unstableRefusals;
        else ++invalidRefusals;
    }
    busy.clear();return result;
}
inline bool Guards(const D2RL::PluginContext* ctx) noexcept {
    if(!ctx->CheckExpectedBytes(Native::Entry,Native::Head,sizeof(Native::Head))) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return false;
    bool valid=true;
    for(const auto& body:Native::Bodies) {
        unsigned char bytes[4096]{},digest[32]{};
        if(body.size>sizeof(bytes) || !Read(ctx->exeBase+body.rva,bytes,body.size) ||
           BCryptHash(algorithm,nullptr,0,bytes,body.size,digest,sizeof(digest))<0 ||
           std::memcmp(digest,body.hash,sizeof(digest))) {valid=false;break;}
    }
    BCryptCloseAlgorithmProvider(algorithm,0);return valid;
}
inline void Install(const D2RL::PluginContext* ctx) noexcept {
    if(!Guards(ctx)) {ctx->LogWarn("GroundSupply b28: exact native guards refused; existing terrain behavior preserved.");return;}
    LARGE_INTEGER frequency{};
    if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0) {
        ctx->LogWarn("GroundSupply b28: performance clock unavailable; existing terrain behavior preserved.");return;
    }
    budgetTicks=frequency.QuadPart/125+(frequency.QuadPart%125!=0);
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&Hook),&pinned)) return;
    base=ctx->exeBase;
    if(!ctx->InstallInlineHook(Native::Entry,Native::Head,sizeof(Native::Head),reinterpret_cast<void*>(&Hook),&originalAddress) || !originalAddress) {
        ctx->LogWarn("GroundSupply b28: loader-owned hook refused; existing terrain behavior preserved.");return;
    }
    enabled.store(true);
    ctx->LogInfo("GroundSupply b28 admitted: native-level existing-room neighbor extension for fallback ground, clipped to 22-cell cache; 8ms performance-counter read budget; shared room selection, AI and targeting unchanged. Restart-only test.");
}
}
