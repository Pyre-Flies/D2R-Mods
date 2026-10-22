#include "server_frame.h"
#include "frame_bridge.h"
#include "frame_profile.h"
#include "native_input_profile.h"
#include "core_compatibility.h"
#include <atomic>

namespace GemBag::ServerFrame {
namespace {
FrameBridge::Bridge bridge;
const D2RL::PluginContext* context{};
uintptr_t core{};
std::atomic<Pump> pump{};
FrameBridge::Call original{};
template<class T> bool Read(uintptr_t address,T& value) noexcept{return FrameBridge::Read(address,&value,sizeof(value));}
template<size_t N> bool Bytes(uintptr_t address,const unsigned char(&expected)[N]) noexcept {
    std::array<unsigned char,N> actual{};return FrameBridge::Read(address,actual.data(),N)&&!std::memcmp(actual.data(),expected,N);
}
bool Owned() noexcept {
    using namespace FrameProfile;
    if(!core||!bridge.Owned()||!Bytes(core+DrainRva,Drain)||!Bytes(core+PeerRva,Peer))return false;
    std::array<unsigned char,sizeof(Dispatch)> actual{};
    if(!FrameBridge::Read(core+DispatchRva,actual.data(),actual.size()))return false;
    std::memcpy(actual.data()+BlockRva-DispatchRva,Dispatch+BlockRva-DispatchRva,16);
    return !std::memcmp(actual.data(),Dispatch,sizeof(Dispatch));
}
void __fastcall Adapter(void* queue,void* localGame) noexcept {
    try {
        if(reinterpret_cast<uintptr_t>(queue)==core+FrameProfile::QueueRva) {
            const auto callback=pump.load();const auto game=CurrentGame(context);
            if(callback&&game)callback(reinterpret_cast<void*>(game));
        }
    }catch(...){ }
    original(queue,localGame); // Preserve the SDK drain and every owned callback.
}
}
uintptr_t CurrentGame(const D2RL::PluginContext* owner) noexcept {
    if(!owner||owner!=context||!pump.load()||!Owned())return 0;
    DWORD thread{},index{},mode{};uintptr_t network{},tls{};
    if(!Read(core+FrameProfile::ThreadRva,thread)||thread!=GetCurrentThreadId()||
       !Read(core+FrameProfile::NetworkRva,network)||!network||!Read(network+0x14,mode)||(mode!=1&&mode!=3)||
       !Read(core+FrameProfile::TlsIndexRva,index)||index>=1088)return 0;
    const auto vector=static_cast<uintptr_t>(__readgsqword(0x58));
    uintptr_t game{};return vector&&Read(vector+8*index,tls)&&tls&&Read(tls+FrameProfile::TlsGameOffset,game)?game:0;
}
bool Install(const D2RL::PluginContext* owner,Pump callback) noexcept {
    using namespace FrameProfile;
    if(!owner||!callback||context)return false;
    const auto module=GetModuleHandleW(L"D2RCore.dll");core=reinterpret_cast<uintptr_t>(module);
    if(!QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash)||!Bytes(core+DispatchRva,Dispatch)||
       !Bytes(core+DrainRva,Drain)||!Bytes(core+PeerRva,Peer)){core=0;return false;}
    context=owner;original=reinterpret_cast<FrameBridge::Call>(core+DrainRva);pump=callback;
    std::array<unsigned char,16> block{};std::memcpy(block.data(),Dispatch+BlockRva-DispatchRva,16);
    if(!bridge.Install(core+CallRva,original,Adapter,block)){pump=nullptr;context=nullptr;return false;}
    owner->LogInfo("[GemBag/Host] Qualified per-game frame bridge installed; SDK callbacks and native game scope preserved.");return true;
}
void Stop() noexcept {pump=nullptr;bridge.Stop();}
}
