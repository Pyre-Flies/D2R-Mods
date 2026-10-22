#include "item_code_compatibility.h"
#include "unhoarder_dispatch_compatibility.h"
#include "relay_allocation.h"
#include "client_loot.h"
#include "unhoarder_visibility.h"
#include "client_loot_profile.h"
#include "loot_approach_profile.h"
#include "loot_run_policy.h"
#include "loot_update_profile.h"
#include "glyph_calls.h"
#include "controller_input.h"
#include "native_d2r.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cstring>
#include <cstdio>
extern "C" {
    void QolLootRunBridge();
    void* QolLootNativeRunQuery=nullptr;
}
namespace QolClientLoot {
namespace {
std::recursive_mutex mutex;
std::atomic<bool> active{false};
const D2RL::PluginContext* context{};
RankFn rankItem{};
BlockPickupFn blockPickup{};
using InteractionFn=void(__fastcall*)(uint16_t,uint32_t);
InteractionFn originalInteraction{};
bool interactionHook{};
unsigned char installedInteraction[15]{};
std::atomic<bool> ready{false};
bool approachReady{};
std::atomic<bool> lifecycleAvailable{false},localPlayerReady{false};
LocalSession localSession;
std::array<NativePickupPermit,7> pickupPermits{};
RepeatGate repeatGate{};
bool runHook{};
unsigned char installedMovement[5]{};
uint32_t runGuid{};
using UpdateFn=void(__fastcall*)(void*);
UpdateFn originalUpdate{};
bool updateHook{};
unsigned char installedUpdate[5]{};
HeldFn heldNow{};
struct QueuedApproach {Request request{};uint32_t radius{};};
QueuedApproach queuedApproach{};
thread_local bool inClientUpdate{};
bool ValidateUpdate() noexcept {
    return updateHook && context &&
        context->CheckExpectedBytes(QolLootUpdate::RestoreSite,QolLootUpdate::Restore,5) &&
        context->CheckExpectedBytes(QolLootUpdate::WindowSite,QolLootUpdate::Window,14) &&
        context->CheckExpectedBytes(QolLootUpdate::CallSite,installedUpdate,5) &&
        context->CheckExpectedBytes(QolLootUpdate::CallSite+5,QolLootUpdate::Window+19,5);
}
bool wasHeld{};
using InGameFn=bool(__cdecl*)() noexcept;
InGameFn inGame{};
Slots slots{};
std::unordered_map<uint32_t,uint64_t> visible;
std::unordered_map<uint32_t,VisibilityProof> providerHidden;
uintptr_t playerIdentity{}; // comparison only; never dereferenced between callbacks
uint32_t playerGuid{};
uint64_t epoch{1};
void __fastcall HookClientUpdate(void* player) noexcept {
    // Preserve the game's pending action before starting one new explicit request.
    if(originalUpdate)originalUpdate(player);
    QueuedApproach queued{};HeldFn held{};
    {
        std::lock_guard lock(mutex);
        if(!active || !Ready() || !ValidateUpdate() ||
           reinterpret_cast<uintptr_t>(player)!=playerIdentity)return;
        queued=queuedApproach;queuedApproach={};held=heldNow;
    }
    if(!queued.request.item.guid || !held || !held())return;
    inClientUpdate=true;
    Submit(queued.request,queued.radius,true);
    inClientUpdate=false;
}
bool Validate() noexcept {
    if(!context || !inGame)return false;
    for(auto& s:Profile::Sites) {
        if(s.rva==0xfa180 && interactionHook) {
            if(!context->CheckExpectedBytes(s.rva,installedInteraction,sizeof(installedInteraction)) ||
               !context->CheckExpectedBytes(s.rva+15,s.bytes+15,s.size-15))return false;
        } else if(!QolItemCode::Check(context,s.rva,s.bytes,s.size))return false;
    }
    return true;
}
void* Player() noexcept {
    if(!Ready() || !inGame || !inGame())return nullptr;
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
        if(!QolLootVisibility::CannotHideLabel(context,unit,guid) &&
           QolLootVisibility::Hidden(context,unit,guid,code))return false;
        char text[5]{};std::memcpy(text,&code,4);
        out={guid,code,*data,distance,rankItem(text,*data)};
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool HiddenUnit(const void* unit,uint32_t guid,uint32_t code,bool preflightChecked=false) noexcept {
    __try {
        if(!unit || !guid || !context)return false;
        const auto* words=static_cast<const uint32_t*>(unit);
        if(words[0]!=4 || words[2]!=guid || words[3]!=3)return false;
        if(!preflightChecked && QolLootVisibility::CannotHideLabel(context,unit,guid)) {
            QolPerf::Counters[QolPerf::LabelFastPath].Record(0,1);
            return false;
        }
        return QolLootVisibility::Hidden(context,unit,guid,code);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool HiddenCandidate(const Candidate& item) noexcept {
    __try {
        if(!item.guid || !context)return false;
        const bool fromLabel=QolLootVisibility::CurrentLabelUnit &&
            QolLootVisibility::CurrentLabelGuid==item.guid;
        const auto unit=fromLabel?QolLootVisibility::CurrentLabelUnit:
            reinterpret_cast<const void*(__fastcall*)(uint32_t,uint32_t)>(context->exeBase+0x9a5d0)(item.guid,4);
        if(fromLabel) {
            QolLootVisibility::CurrentLabelGuid=0;
            QolLootVisibility::CurrentLabelUnit=nullptr;
        }
        return HiddenUnit(unit,item.guid,item.code,fromLabel);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool Session(void*& player) noexcept {
    __try {
        player=Player();
        if(!player) {
            if(playerIdentity || wasHeld)++epoch;
            slots={};visible.clear();providerHidden.clear();playerIdentity=0;playerGuid=0;wasHeld=false;pickupPermits={};repeatGate={};runGuid=0;
            return false;
        }
        const auto guid=static_cast<const uint32_t*>(player)[2];
        if(!localSession.Matches(static_cast<const uint32_t*>(player)[0],guid)) {
            ++epoch;slots={};visible.clear();providerHidden.clear();wasHeld=false;pickupPermits={};repeatGate={};runGuid=0;return false;
        }
        if(playerIdentity && (playerIdentity!=reinterpret_cast<uintptr_t>(player) || playerGuid!=guid)) {
            ++epoch;
            slots={};visible.clear();providerHidden.clear();wasHeld=false;pickupPermits={};repeatGate={};runGuid=0;
        }
        playerIdentity=reinterpret_cast<uintptr_t>(player);playerGuid=guid;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {++epoch;slots={};visible.clear();providerHidden.clear();wasHeld=false;pickupPermits={};repeatGate={};runGuid=0;return false;}
}
bool ValidateApproach() noexcept {
    for(const auto& s:ApproachProfile::Sites) {
        if(runHook && s.rva==0xfbef0) {
            constexpr auto offset=QolLootRun::CallSite-0xfbef0;
            if(!context->CheckExpectedBytes(s.rva,s.bytes,offset) ||
               !context->CheckExpectedBytes(QolLootRun::CallSite,installedMovement,5) ||
               !context->CheckExpectedBytes(QolLootRun::CallSite+5,s.bytes+offset+5,s.size-offset-5))return false;
        } else if(s.rva==0xfabe0) {
            if(!QolUnhoarderDispatch::Check(context,s.bytes,s.size))return false;
        } else if(!QolItemCode::Check(context,s.rva,s.bytes,s.size))return false;
    }
    return context && context->CheckExpectedBytes(QolLootRun::Target,QolLootRun::GetterWitness,sizeof(QolLootRun::GetterWitness));
}
uint32_t SelectMovement(uint32_t action,void* player,uint32_t type,uint32_t guid) noexcept {
    const D2RL::PluginContext* ctx{};
    const auto previous=action;
    {
        std::lock_guard lock(mutex);
        bool owned=false;
        if(runHook && active && Ready() && approachReady && context && ControllerQoL::IsControllerUiActive() &&
           reinterpret_cast<uintptr_t>(player)==playerIdentity && runGuid==guid && ValidateApproach()) {
            const auto now=GetTickCount64();
            for(const auto& permit:pickupPermits)
                if(permit.Current(localSession,playerGuid,guid,now)) {owned=true;break;}
        }
        action=QolLootRun::Action(action,type,owned);
        if(owned && type==4 && (previous==2 || previous==4))ctx=context;
    }
    if(ctx) {
        char message[160];
        std::snprintf(message,sizeof(message),"[QOL/ClientLoot] GUID=%u movement requested=run native=%s.",guid,previous==2?"walk":"run");
        ctx->LogInfo(message);
    }
    return action;
}
bool InstallCall(const D2RL::PluginContext* ctx,uintptr_t site,const unsigned char* expected,
                 const void* destination,unsigned char (&installedBytes)[5]) noexcept {
    if(!ctx->CheckExpectedBytes(site,expected,5))return false;
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&QolLootRunBridge),&pinned))return false;
    auto* relay=QolRelay::Allocate(ctx->exeBase+site,64);
    if(!relay)return false;
    relay[0]=0xff;relay[1]=0x25;std::memset(relay+2,0,4);
    std::memcpy(relay+6,&destination,sizeof(destination));
    unsigned char replacement[5]{};DWORD old{};
    if(!QolGlyphCalls::Encode(ctx->exeBase+site,reinterpret_cast<uintptr_t>(relay),replacement) ||
       !VirtualProtect(relay,64,PAGE_EXECUTE_READ,&old) || !FlushInstructionCache(GetCurrentProcess(),relay,64)) {
        VirtualFree(relay,0,MEM_RELEASE);return false;
    }
    // Loader owns restoration; retain the pinned relay after publication starts.
    if(!ctx->PatchBytes(site,expected,5,replacement,5))return false;
    std::memcpy(installedBytes,replacement,5);
    return true;
}
bool Invoke(uintptr_t base,void* player,uint32_t guid,bool approach) noexcept {
    __try {
        if(approach) {
            // Native target movement + deferred item action (0x16). The game
            // owns pathfinding, run/walk selection, cancellation and arrival.
            reinterpret_cast<void(__fastcall*)(void*,uint32_t,uint32_t,uint32_t)>(base+0xfbef0)(player,0x16,4,guid);
            return true;
        }
        // Native interaction owns packet assembly, network dispatch and destination.
        reinterpret_cast<void(__fastcall*)(uint16_t,uint32_t)>(base+0xfa180)(4,guid);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool ReadPickupCode(uint32_t guid,uint32_t& code) noexcept {
    __try {
        const auto* unit=(QolLootVisibility::CurrentLabelUnit && QolLootVisibility::CurrentLabelGuid==guid)?
            static_cast<const uint32_t*>(QolLootVisibility::CurrentLabelUnit):
            reinterpret_cast<const uint32_t*(__fastcall*)(uint32_t,uint32_t)>(context->exeBase+0x9a5d0)(guid,4);
        if(!unit || unit[0]!=4 || unit[2]!=guid || unit[3]!=3)return false;
        code=reinterpret_cast<uint32_t(__fastcall*)(const void*)>(context->exeBase+0x36ef50)(unit);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void __fastcall HookInteraction(uint16_t type,uint32_t guid) noexcept {
    BlockPickupFn filter{};
    uint32_t code{};
    {
        std::lock_guard lock(mutex);
        if(type==4 && guid==runGuid)runGuid=0;
        if(type==4 && Ready() && context && blockPickup && Validate() && ReadPickupCode(guid,code))filter=blockPickup;
    }
    // The visibility callback owns the placard lock; never call it under loot's
    // lock because the tooltip callback acquires those locks in the reverse order.
    if(filter && filter(guid,code))return;
    if(originalInteraction)originalInteraction(type,guid);
}
}
void Initialize(const D2RL::PluginContext* ctx,RankFn rank,BlockPickupFn filter,HeldFn held) noexcept {
    std::lock_guard lock(mutex);
    context=ctx;rankItem=rank;blockPickup=filter;heldNow=held;queuedApproach={};
    active=false;slots={};visible.clear();providerHidden.clear();playerIdentity=0;playerGuid=0;wasHeld=false;++epoch;
    lifecycleAvailable=false;localPlayerReady=false;localSession={};pickupPermits={};repeatGate={};runGuid=0;
    inGame=reinterpret_cast<InGameFn>(GetProcAddress(GetModuleHandleW(L"D2RCore.dll"),"IsInGame"));
    ready=rank && Validate();
    if(ready && filter && ctx->InstallInlineHook<InteractionFn>(0xfa180,Profile::Interaction,15,&HookInteraction,&originalInteraction)) {
        std::memcpy(installedInteraction,reinterpret_cast<const void*>(ctx->exeBase+0xfa180),sizeof(installedInteraction));
        interactionHook=true;
    }
    ctx->LogInfo(interactionHook?"[QOL/ClientLoot] Client filtered-pickup gate installed.":
        "[QOL/ClientLoot] Client filtered-pickup gate unavailable; native interaction preserved.");
    originalUpdate=reinterpret_cast<UpdateFn>(ctx->exeBase+0xf9bc0);
    updateHook=ready && held && ctx->CheckExpectedBytes(QolLootUpdate::RestoreSite,QolLootUpdate::Restore,5) &&
        ctx->CheckExpectedBytes(QolLootUpdate::WindowSite,QolLootUpdate::Window,sizeof(QolLootUpdate::Window)) &&
        InstallCall(ctx,QolLootUpdate::CallSite,QolLootUpdate::Window+14,
                    reinterpret_cast<const void*>(&HookClientUpdate),installedUpdate);
    approachReady=ready && updateHook && ValidateApproach();
    QolLootNativeRunQuery=reinterpret_cast<void*>(ctx->exeBase+QolLootRun::Target);
    runHook=approachReady && InstallCall(ctx,QolLootRun::CallSite,QolLootRun::Expected,
                                       reinterpret_cast<const void*>(&QolLootRunBridge),installedMovement);
    ctx->LogInfo(updateHook?"[QOL/ClientLoot] Approach requests queued at native client update.":
        "[QOL/ClientLoot] Native client update unavailable; approach disabled.");
    ctx->LogInfo(runHook?"[QOL/ClientLoot] Scoped native run requests enabled for shortcut loot.":
        "[QOL/ClientLoot] Run override unavailable; native walk/run choice preserved.");
    ctx->LogInfo(approachReady?"[QOL/ClientLoot] Native loot approach profile admitted.":
        "[QOL/ClientLoot] Approach unavailable; existing immediate pickup route retained.");
    ctx->LogInfo(ready?"[QOL/ClientLoot] Native client pickup profile admitted.":
        "[QOL/ClientLoot] Profile unavailable; remote ground shortcuts disabled.");
}
void SetLifecycleAvailable(bool available) noexcept {
    lifecycleAvailable=available;
}
void OnLifecycle(const D2RL::Lifecycle::GameplayEvent* event) noexcept {
    std::lock_guard lock(mutex);
    if(event && !D2RL::Lifecycle::HasGameplayEventField(event,D2RL::Lifecycle::GameplayEventRequiredSize))return;
    if(event && event->kind==D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady) {
        if(!localSession.Bind(event->sessionGeneration,event->playerId))return;
    } else if(event && event->kind==D2RL::Lifecycle::GameplayEventKind::GameJoined) {
        localSession.Join(event->sessionGeneration);
    } else {localSession.Leave();}
    queuedApproach={};
    localPlayerReady=localSession.Matches(0,localSession.playerId);
    ++epoch;active=false;slots={};visible.clear();providerHidden.clear();wasHeld=false;playerIdentity=0;playerGuid=0;pickupPermits={};repeatGate={};runGuid=0;
}
bool IsLocalActor(uint32_t type,uint32_t id) noexcept {
    std::lock_guard lock(mutex);
    return lifecycleAvailable && localSession.Matches(type,id);
}
bool TakeNativePickupPermit(uint32_t actorId,uint32_t guid) noexcept {
    std::lock_guard lock(mutex);
    const auto now=GetTickCount64();
    for(auto& permit:pickupPermits)if(permit.Current(localSession,actorId,guid,now)) {
        permit={};return true;
    }
    return false;
}
void Shutdown() noexcept {
    std::lock_guard lock(mutex);
    active=false;ready=false;lifecycleAvailable=false;localPlayerReady=false;localSession={};pickupPermits={};repeatGate={};runGuid=0;
    slots={};visible.clear();providerHidden.clear();queuedApproach={};heldNow=nullptr;blockPickup=nullptr;context=nullptr;inGame=nullptr;
}
bool Activate() noexcept {
    std::lock_guard lock(mutex);
    if(!Ready())return false;
    if(!active.exchange(true)) {
        char message[128];
        std::snprintf(message,sizeof(message),"[QOL/ClientLoot] pid=%lu using UI-owned local pickup requests (offline/host/client).",GetCurrentProcessId());
        context->LogInfo(message);
    }
    return true;
}
void Deactivate() noexcept {
    std::lock_guard lock(mutex);
    if(active.exchange(false)) {queuedApproach={};++epoch;slots={};wasHeld=false;playerIdentity=0;playerGuid=0;}
}
bool Active() noexcept {return active.load();}
bool HiddenForPickup(uint32_t guid) noexcept {
    std::lock_guard lock(mutex);
    uint32_t code{};
    if(!Ready() || !context || !guid || !Validate() || !ReadPickupCode(guid,code) ||
       !HiddenCandidate(Candidate{guid,code})) {
        QolLootVisibility::CurrentLabelGuid=0;QolLootVisibility::CurrentLabelUnit=nullptr;
        providerHidden.erase(guid);return false;
    }
    QolLootVisibility::CurrentLabelGuid=0;QolLootVisibility::CurrentLabelUnit=nullptr;
    if(providerHidden.size()<2048 || providerHidden.contains(guid))
        providerHidden[guid]={localSession.generation,GetTickCount64(),code};
    return true;
}
bool CachedHiddenForPickup(uint32_t guid) noexcept {
    std::lock_guard lock(mutex);
    const auto found=providerHidden.find(guid);
    return Ready() && found!=providerHidden.end() &&
        found->second.Current(localSession.generation,GetTickCount64());
}
bool Ready() noexcept {return ready.load() && lifecycleAvailable.load() && localPlayerReady.load();}
uint64_t Epoch() noexcept {std::lock_guard lock(mutex);return epoch;}
void Track(uint32_t guid,bool shown,const void* labelUnit) noexcept {
    std::lock_guard lock(mutex);
    if(!Ready() || !guid)return;
    if(shown) {
        // Only a proof that NO Hide action can match avoids the original query.
        // Potentially hidden/unknown items retain every native admission check.
        if(QolLootVisibility::CannotHideLabel(context,labelUnit,guid)) {
            providerHidden.erase(guid);
            QolPerf::Counters[QolPerf::LabelFastPath].Record(0,1);
        } else (void)HiddenForPickup(guid);
    } else providerHidden.erase(guid);
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
    QolPerf::Scope timing(QolPerf::Counters[QolPerf::Refresh]);
    uint64_t candidateReads{};
    timing.Items(0);
    std::lock_guard lock(mutex);
    if(!active || !Ready())return false;
    const auto old=slots;
    void* player{};
    const bool session=Session(player);
    if(!held || !session) {queuedApproach={};if(wasHeld)++epoch;slots={};wasHeld=false;}
    else {
        const bool firstRefresh=!wasHeld;
        wasHeld=true;
        std::vector<Candidate> candidates;
        const auto now=GetTickCount64();
        for(auto it=visible.begin();it!=visible.end();) {
            if(now-it->second>=1500) {it=visible.erase(it);continue;}
            // Only the first refresh assigns candidates. Keep observation TTL
            // cleanup below active throughout a hold, without repeating native
            // reads and sorting a replacement snapshot that cannot be used.
            if(firstRefresh) {
                timing.Items(++candidateReads);
                Candidate c{};
                if(Read(player,it->first,radius,c))candidates.push_back(c);
            }
            ++it;
        }
        if(firstRefresh)slots=Assign({},std::move(candidates));
        std::vector<uint32_t> hidden;
        for(const auto& slot:slots)if(HiddenCandidate(slot))hidden.push_back(slot.guid);
        slots=PruneHiddenSlots(slots,hidden);
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
    if(!active || !Ready() || !CurrentRequest(request,epoch,GetTickCount64(),held))return;
    const auto selected=request.item;
    void* player{};
    if(!Session(player) || !wasHeld || request.epoch!=epoch)return;
    // Recheck admission after other plugins have loaded, before any native action.
    if(!Validate()) {ready=false;slots={};context->LogWarn("[QOL/ClientLoot] Profile changed; request cancelled.");return;}
    auto it=visible.find(selected.guid);
    if(it==visible.end() || GetTickCount64()-it->second>=1500)return;
    bool assigned=false;
    for(auto s:slots)if(SameItem(s,selected))assigned=true;
    if(approachReady && (!ValidateApproach() || !ValidateUpdate())) {
        approachReady=false;
        context->LogWarn("[QOL/ClientLoot] Approach contract changed; immediate pickup route retained.");
    }
    Candidate current{};
    if(!assigned || !Read(player,selected.guid,SubmissionRadius(radius,approachReady),current) || !SameItem(selected,current))return;
    const bool approach=approachReady && NeedsApproach(current.distance);
    const auto now=GetTickCount64();
    if(repeatGate.Blocks(selected.guid,localSession.generation,now))return;
    if(approach && !inClientUpdate) {
        queuedApproach={request,radius};
        return; // Never mutate the native movement path from a UI callback.
    }
    NativePickupPermit* available=nullptr;
    for(auto& permit:pickupPermits) {
        // Renew the same item's completion permission on an explicit retry;
        // its lifetime must not also delay subsequent button presses.
        if(permit.Current(localSession,localSession.playerId,selected.guid,now)) {available=&permit;break;}
        if(!permit.guid || permit.generation!=localSession.generation || now<permit.issued || now-permit.issued>permit.lifetime)available=&permit;
    }
    if(!available)return;
    *available={selected.guid,localSession.generation,now,approach?5000u:1000u};
    runGuid=approach?selected.guid:0;
    repeatGate={selected.guid,localSession.generation,now};
    const auto generation=localSession.generation;
    const auto ctx=context;
    lock.unlock(); // Native interaction may rebuild tooltip text; do not invert label/loot locks.
    const bool submitted=Invoke(ctx->exeBase,player,selected.guid,approach);
    if(!submitted) {
        std::lock_guard failureLock(mutex);
        if(runGuid==selected.guid)runGuid=0;
        for(auto& permit:pickupPermits)if(permit.guid==selected.guid && permit.generation==generation && permit.issued==now)permit={};
    }
    char message[160];
    std::snprintf(message,sizeof(message),"[QOL/ClientLoot] pid=%lu GUID=%u distance=%d native %s request %s; awaiting server state.",
        GetCurrentProcessId(),selected.guid,current.distance,approach?"approach":"pickup",submitted?"submitted":"faulted");
    ctx->LogInfo(message);
    // Keep assignment/placard until the client observes removal. No optimistic
    // success and no automatic resend on rejection/full inventory/latency.
}
}

extern "C" uint64_t QolLootSelectRunFlag(uint64_t native,void* player,uint32_t type,uint32_t guid) noexcept {
    const auto action=((native>>8)&0xff)?4u:2u;
    return QolLootRun::Packed(native,QolClientLoot::SelectMovement(action,player,type,guid));
}
