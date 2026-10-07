#include "client_loot.h"
#include "client_loot_profile.h"
#include "native_d2r.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <cstdio>
namespace QolClientLoot {
namespace {
std::recursive_mutex mutex;
std::atomic<bool> active{false};
const D2RL::PluginContext* context{};
RankFn rankItem{};
std::atomic<bool> ready{false};
bool wasHeld{};
using InGameFn=bool(__cdecl*)() noexcept;
InGameFn inGame{};
Slots slots{};
std::unordered_map<uint32_t,uint64_t> visible;
uintptr_t playerIdentity{}; // comparison only; never dereferenced between callbacks
uint32_t playerGuid{};
uint64_t epoch{1};
bool Validate() noexcept {
    if(!context || !inGame)return false;
    for(auto& s:Profile::Sites)
        if(!context->CheckExpectedBytes(s.rva,s.bytes,s.size))return false;
    return true;
}
void* Player() noexcept {
    if(!ready || !inGame || !inGame())return nullptr;
    return D2R::Native::GetLocalPlayerUnit(context->exeBase);
}
bool Read(void* player,uint32_t guid,uint32_t radius,Candidate& out) noexcept {
    __try {
        const auto base=context->exeBase;
        const auto unit=reinterpret_cast<void*(__fastcall*)(uint32_t,uint32_t)>(base+0x9a5d0)(guid,4);
        if(!unit || !player)return false;
        auto words=static_cast<const uint32_t*>(unit);
        if(words[0]!=4 || words[2]!=guid || words[3]!=3)return false;
        const auto distance=reinterpret_cast<int(__fastcall*)(void*,void*)>(base+0x325140)(player,unit);
        if(distance<0 || static_cast<uint32_t>(distance)>radius)return false;
        const auto collision=reinterpret_cast<int(__fastcall*)(void*,void*,uint32_t)>(base+0x350550)(player,unit,0x804);
        if(!Eligible(words[0],words[3],distance,radius,collision))return false;
        const auto code=reinterpret_cast<uint32_t(__fastcall*)(void*)>(base+0x36ef50)(unit);
        const auto data=*reinterpret_cast<const uint32_t* const*>(static_cast<const unsigned char*>(unit)+0x10);
        if(!data)return false;
        char text[5]{};std::memcpy(text,&code,4);
        out={guid,code,*data,distance,rankItem(text,*data)};
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool Session(void*& player) noexcept {
    __try {
        player=Player();
        if(!player) {
            if(playerIdentity || wasHeld)++epoch;
            slots={};visible.clear();playerIdentity=0;playerGuid=0;wasHeld=false;
            return false;
        }
        const auto guid=static_cast<const uint32_t*>(player)[2];
        if(playerIdentity && (playerIdentity!=reinterpret_cast<uintptr_t>(player) || playerGuid!=guid)) {
            ++epoch;
            slots={};visible.clear();wasHeld=false;
        }
        playerIdentity=reinterpret_cast<uintptr_t>(player);playerGuid=guid;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {++epoch;slots={};visible.clear();wasHeld=false;return false;}
}
bool Invoke(uintptr_t base,uint32_t guid) noexcept {
    __try {
        // Native interaction owns packet assembly, network dispatch and destination.
        reinterpret_cast<void(__fastcall*)(uint16_t,uint32_t)>(base+0xfa180)(4,guid);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
void Initialize(const D2RL::PluginContext* ctx,RankFn rank) noexcept {
    std::lock_guard lock(mutex);
    context=ctx;rankItem=rank;
    active=false;slots={};visible.clear();playerIdentity=0;playerGuid=0;wasHeld=false;++epoch;
    inGame=reinterpret_cast<InGameFn>(GetProcAddress(GetModuleHandleW(L"D2RCore.dll"),"IsInGame"));
    ready=rank && Validate();
    ctx->LogInfo(ready?"[QOL/ClientLoot] Native client pickup profile admitted.":
        "[QOL/ClientLoot] Profile unavailable; remote ground shortcuts disabled.");
}
void Shutdown() noexcept {
    std::lock_guard lock(mutex);
    active=false;ready=false;slots={};visible.clear();context=nullptr;inGame=nullptr;
}
bool Activate() noexcept {
    std::lock_guard lock(mutex);
    if(!ready)return false;
    if(!active.exchange(true)) {
        char message[128];
        std::snprintf(message,sizeof(message),"[QOL/ClientLoot] pid=%lu using client requests: game scheduler unavailable.",GetCurrentProcessId());
        context->LogInfo(message);
    }
    return true;
}
void Deactivate() noexcept {
    std::lock_guard lock(mutex);
    if(active.exchange(false)) {++epoch;slots={};wasHeld=false;playerIdentity=0;playerGuid=0;}
}
bool Active() noexcept {return active.load();}
bool Ready() noexcept {return ready.load();}
void Track(uint32_t guid,bool shown) noexcept {
    std::lock_guard lock(mutex);
    if(!ready || !guid)return;
    if(!shown)visible.erase(guid);
    else if(visible.size()<2048 || visible.contains(guid))visible[guid]=GetTickCount64();
}
int SlotFor(uint32_t guid) noexcept {
    std::lock_guard lock(mutex);
    if(!active || !wasHeld)return -1;
    for(unsigned i=0;i<slots.size();++i)if(slots[i].guid==guid)return static_cast<int>(i);
    return -1;
}
Candidate Slot(unsigned index) noexcept {
    std::lock_guard lock(mutex);
    return active && wasHeld && index<slots.size()?slots[index]:Candidate{};
}
Request Capture(unsigned index) noexcept {
    std::lock_guard lock(mutex);
    return {Slot(index),epoch,GetTickCount64()};
}
bool Refresh(uint32_t radius,bool held) noexcept {
    std::lock_guard lock(mutex);
    if(!active || !ready)return false;
    const auto old=slots;
    void* player{};
    const bool session=Session(player);
    if(!held || !session) {if(wasHeld)++epoch;slots={};wasHeld=false;}
    else {
        if(!wasHeld)slots={};
        wasHeld=true;
        std::vector<Candidate> candidates;
        const auto now=GetTickCount64();
        for(auto it=visible.begin();it!=visible.end();) {
            if(now-it->second>=1500) {it=visible.erase(it);continue;}
            Candidate c{};
            if(Read(player,it->first,radius,c))candidates.push_back(c);
            ++it;
        }
        slots=Assign(slots,std::move(candidates));
    }
    for(unsigned i=0;i<slots.size();++i)
        if(old[i].guid!=slots[i].guid) {
            char message[160];
            std::snprintf(message,sizeof(message),"[QOL/ClientLoot] pid=%lu slots A=%u X=%u Y=%u B=%u R1=%u R2=%u L2=%u",
                GetCurrentProcessId(),slots[0].guid,slots[1].guid,slots[2].guid,slots[3].guid,slots[4].guid,slots[5].guid,slots[6].guid);
            context->LogInfo(message);
            return true;
        }
    return false;
}
void Submit(const Request& request,uint32_t radius,bool held) noexcept {
    std::unique_lock lock(mutex);
    if(!active || !ready || !CurrentRequest(request,epoch,GetTickCount64(),held))return;
    const auto selected=request.item;
    void* player{};
    if(!Session(player) || !wasHeld || request.epoch!=epoch)return;
    // Recheck admission after other plugins have loaded, before any native action.
    if(!Validate()) {ready=false;slots={};context->LogWarn("[QOL/ClientLoot] Profile changed; request cancelled.");return;}
    auto it=visible.find(selected.guid);
    if(it==visible.end() || GetTickCount64()-it->second>=1500)return;
    bool assigned=false;
    for(auto s:slots)if(SameItem(s,selected))assigned=true;
    Candidate current{};
    if(!assigned || !Read(player,selected.guid,radius,current) || !SameItem(selected,current))return;
    const auto ctx=context;
    lock.unlock(); // Native interaction may rebuild tooltip text; do not invert label/loot locks.
    const bool submitted=Invoke(ctx->exeBase,selected.guid);
    char message[160];
    std::snprintf(message,sizeof(message),"[QOL/ClientLoot] pid=%lu GUID=%u distance=%d native request %s; awaiting server state.",
        GetCurrentProcessId(),selected.guid,current.distance,submitted?"submitted":"faulted");
    ctx->LogInfo(message);
    // Keep assignment/placard until the client observes removal. No optimistic
    // success and no automatic resend on rejection/full inventory/latency.
}
}
