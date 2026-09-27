#include "identify_stat.h"
#include "identify_stat_profile.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
namespace QolIdentifyStat {
namespace {
uintptr_t core{};
bool admitted{};
bool Guard(uintptr_t game) noexcept {
    using namespace QolIdentifyStatProfile;
    __try {
        return game && core && !std::memcmp(reinterpret_cast<void*>(game+0x2f5020),Entry,sizeof(Entry)) &&
            *reinterpret_cast<const uintptr_t*>(game+0x3e2a218)==core+0x831de0 &&
            !std::memcmp(reinterpret_cast<void*>(core+0x831de0),Wrapper,sizeof(Wrapper)) &&
            !std::memcmp(reinterpret_cast<void*>(core+0x3d8bd0),Body,sizeof(Body));
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
struct ReadState {uintptr_t game;int32_t value{};bool ok{};};
void __cdecl ReadNative(const D2RL::PluginContext*,void* item,void* user) noexcept {
    auto& state=*static_cast<ReadState*>(user);
    __try {
        if(!item || !Guard(state.game))return;
        state.value=reinterpret_cast<int32_t(__fastcall*)(void*,int32_t,uint16_t)>(state.game+0x2f5020)(item,70,0);
        state.ok=state.value>=0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {state.ok=false;}
}
}
void Initialize(const D2RL::PluginContext* ctx) noexcept {
    const auto module=GetModuleHandleW(L"D2RCore.dll");core=reinterpret_cast<uintptr_t>(module);
    admitted=ctx && QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash) && Guard(ctx->exeBase);
    if(ctx)ctx->LogInfo(admitted?"[QOL/Identify] Stat-70 tome reader admitted; SDK quantity edits verified by native readback.":
        "[QOL/Identify] Stat-70 contract unavailable; tome identification refuses unknown charges.");
}
bool Read(const D2RL::PluginContext* ctx,const D2RL::ItemService* items,D2RL::ItemHandle handle,int32_t& value) noexcept {
    if(!admitted || !ctx || !items || !items->editNativeItem || !handle)return false;
    ReadState state{ctx->exeBase};
    if(items->editNativeItem(ctx,handle,ReadNative,&state)!=D2RL::Items::Result::Success || !state.ok)return false;
    value=state.value;return true;
}
}
