#include "plugin_coexistence.h"
#include "native_identify.h"
#include "native_identify_profile.h"
#include "identify_stat.h"
#include "identify_stat_profile.h"
#include "native_input_profile.h"
#include "core_compatibility.h"
#include "controller_input.h"
#include "shared_page_signatures.h"
#include "identify_bulk.h"
#include "identify_probe_profile.h"
#include "client_identify_policy.h"
#include "client_identify_profile.h"
#include "client_request_profile.h"
#include "client_transfer.h"
#include "native_d2r.h"
#include "belt_signatures.h"
#include "vendor_signatures.h"
#include "storage_focus.h"
#include <mutex>
#include <cstdio>
#include <atomic>
namespace QolNativeIdentify {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};
const D2RL::ItemService* items{};
const D2RL::ThreadService* threads{};
uintptr_t core{};
bool admitted{},remoteRoute{},sessionEvents{};
std::atomic<bool> busy{false};
thread_local bool forwarding{};
uint64_t generation{};
QolIdentify::BulkPlan bulk{};bool bulkActive{},bulkNative{};ULONGLONG bulkStarted{};
struct Work {QolIdentify::Info target{},tome{};D2RL::PlayerHandle player{};ULONGLONG start{};void* owner{};int before{};unsigned phase{},polls{};bool waited{},scroll{};} work;
template<class T>T At(uintptr_t rva) noexcept {return reinterpret_cast<T>(context->exeBase+rva);}
const char* admissionReason="not-initialized";
bool Bytes(uintptr_t base,uintptr_t rva,const unsigned char* expected,size_t size,const char* module) noexcept {
    auto actual=reinterpret_cast<const unsigned char*>(base+rva);
    for(size_t i=0;i<size;++i)if(actual[i]!=expected[i]) {
        char line[256];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] refused=byte-mismatch module=%s RVA=0x%llX offset=0x%llX expected=%02X actual=%02X.",module,static_cast<unsigned long long>(rva),static_cast<unsigned long long>(i),static_cast<unsigned>(expected[i]),static_cast<unsigned>(actual[i]));
        context->LogWarn(line);return false;
    }
    return true;
}
bool Slot(uintptr_t base,uintptr_t rva,uintptr_t expected,const char* module) noexcept {
    const auto actual=*reinterpret_cast<uintptr_t*>(base+rva);
    if(actual==expected)return true;
    char line[256];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] refused=slot-mismatch module=%s RVA=0x%llX expected=%p actual=%p.",module,static_cast<unsigned long long>(rva),reinterpret_cast<void*>(expected),reinterpret_cast<void*>(actual));
    context->LogWarn(line);return false;
}
bool Guard() noexcept {
    __try {
        if(!context)return false;
        if(!admitted) {char line[160];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] refused=admission reason=%s.",admissionReason);context->LogWarn(line);return false;}
        for(const auto& s:QolNativeIdentifyProfile::Game)
            if(!Bytes(context->exeBase,s.rva,s.bytes,s.size,"game"))return false;
        using namespace QolIdentifyStatProfile;
        return Bytes(context->exeBase,0x2f5020,Entry,sizeof(Entry),"game") &&
            QolCoexistence::StatSlot(context->exeBase,core) &&
            Bytes(core,0x831de0,Wrapper,sizeof(Wrapper),"core") &&
            Bytes(core,0x3d8bd0,Body,sizeof(Body),"core") &&
            Slot(core,0x70e9c8,context->exeBase+0x2c7540,"core") &&
            Slot(core,0x70e968,context->exeBase+0x2c49f0,"core");
    } __except(EXCEPTION_EXECUTE_HANDLER) {if(context)context->LogWarn("[QOL/IdentifyNative] refused=guard-read-fault.");return false;}
}
bool RemoteGuard() noexcept {
    __try {
        for(const auto& site:QolClientIdentify::Profile::Sites)
            if(!Bytes(context->exeBase,site.rva,site.bytes,site.size,"game"))return false;
        using namespace QolBelt::Signatures;
        return Bytes(context->exeBase,0x8b2d0,LocalContext,sizeof(LocalContext),"game") &&
            Bytes(context->exeBase,0x9a5d0,QolVendor::ClientUnitBytes,sizeof(QolVendor::ClientUnitBytes),"game") &&
            Bytes(context->exeBase,0x9a480,LocalPlayer,sizeof(LocalPlayer),"game") &&
            Bytes(context->exeBase,0x36cfe0,QolVendor::ItemPageBytes,sizeof(QolVendor::ItemPageBytes),"game") &&
            Bytes(context->exeBase,0x1c7360,QolIdentifyProbe::Blocked,sizeof(QolIdentifyProbe::Blocked),"game") &&
            QolClientRequest::Guard(context->exeBase,core);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
// UI-thread only; reacquire every callback. Never resolve hidden/shared/cube grids.
unsigned char* ResolveGrid(QolIdentify::Container container) noexcept {
    using C=QolIdentify::Container;
    auto find=At<void*(__fastcall*)(const char*)>(0x846170);
    auto child=At<void*(__fastcall*)(void*,const char*)>(0x856220);
    unsigned char* panel=nullptr;
    if(container==C::Inventory) {
        panel=static_cast<unsigned char*>(find("PlayerInventoryExpansionLayout"));
        if(!panel)panel=static_cast<unsigned char*>(find("PlayerInventoryOriginalLayout"));
    } else if(container==C::PersonalStash) {
        if(!Bytes(context->exeBase,0x23af50,SharedPageNative::SelectedTab,sizeof(SharedPageNative::SelectedTab),"game"))return nullptr;
        panel=static_cast<unsigned char*>(find("BankExpansionLayout"));
        if(!panel || !panel[0x50] || !panel[0x51] || *reinterpret_cast<uintptr_t*>(panel)!=context->exeBase+0x1ce4a20)return nullptr;
        if(At<uint8_t(__fastcall*)(void*)>(0x23af50)(panel)!=0)return nullptr;
        panel=static_cast<unsigned char*>(child(panel,"basicstash_container"));
    } else return nullptr;
    if(!panel || !panel[0x50] || !panel[0x51])return nullptr;
    auto grid=static_cast<unsigned char*>(child(panel,"grid"));
    if(!grid || !grid[0x50] || !grid[0x51] || *reinterpret_cast<uintptr_t*>(grid)!=context->exeBase+0x1cf3800)return nullptr;
    return grid;
}
void Finish(const char* text) noexcept {
    if(context)context->LogInfo(text);
    if(context && bulkActive){char line[160];std::snprintf(line,sizeof(line),"[QOL/IdentifyAll] stopped completed=%u planned=%u; no retry or replacement tome.",bulk.completed,bulk.count);context->LogInfo(line);}
    busy=false;forwarding=false;bulkActive=false;
}
void __cdecl SessionChanged(const D2RL::PluginContext*,const D2RL::Lifecycle::GameplayEvent*,void*) noexcept {
    std::lock_guard lock(mutex);
    ++generation;
    if(busy)Finish("[QOL/IdentifyNative] Session changed; pending work cancelled without retry.");
    work={};remoteRoute=false;
}
bool CurrentPlayer(D2RL::PlayerHandle expected) noexcept {
    const D2RL::InventoryService* inventory{};D2RL::PlayerHandle current{};
    return expected && context->QueryService(&inventory)==D2RL::ServiceQueryResult::Success && inventory &&
        inventory->getLocalPlayer(context,&current)==D2RL::Inventory::Result::Success && current==expected;
}
// Read-only client quantity through the same guarded grid and stat getter used by Tick.
bool ClientQuantity(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle handle,int32_t& quantity) noexcept {
    __try {
        QolIdentify::Info info{.structSize=D2RL::Items::ItemInfoSize};
        if(items->getItemInfo(context,handle,&info)!=D2RL::Items::Result::Success ||
           !QolIdentify::NativeContainer(info.container))return false;
        auto grid=ResolveGrid(info.container);if(!grid)return false;
        struct Cell {int32_t x,y;} cell{info.x,info.y};
        auto unit=static_cast<const unsigned char*>(At<void*(__fastcall*)(void*,const Cell*)>(0x2c49f0)(grid,&cell));
        if(!unit || *reinterpret_cast<const uint32_t*>(unit)!=4 ||
           *reinterpret_cast<const uint32_t*>(unit+8)!=info.runtimeId ||
           At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(unit)!=info.code)return false;
        quantity=At<int32_t(__fastcall*)(const void*,int32_t,uint16_t)>(0x2f5020)(unit,70,0);
        return quantity>=0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool SubmitRemote(void* tome,void* target,void* owner) noexcept {
    QolIdentify::Info liveTome{.structSize=D2RL::Items::ItemInfoSize},liveTarget{.structSize=D2RL::Items::ItemInfoSize};
    if(items->getItemInfo(context,work.tome.handle,&liveTome)!=D2RL::Items::Result::Success ||
       items->getItemInfo(context,work.target.handle,&liveTarget)!=D2RL::Items::Result::Success ||
       !QolClientTransfer::Source(work.tome,liveTome) || !QolClientTransfer::Source(work.target,liveTarget))return false;
    const auto sourceWords=static_cast<const uint32_t*>(tome),targetWords=static_cast<const uint32_t*>(target);
    if(!RemoteGuard() || owner!=D2R::Native::GetLocalPlayerUnit(context->exeBase) ||
       sourceWords[3]!=0 || targetWords[3]!=0 ||
       !QolIdentify::NativeContainer(work.tome.container) || !QolIdentify::NativeContainer(work.target.container) ||
       At<uint8_t(__fastcall*)(void*)>(0x36cfe0)(tome)!=QolClientTransfer::NativePage(work.tome.container) ||
       At<uint8_t(__fastcall*)(void*)>(0x36cfe0)(target)!=QolClientTransfer::NativePage(work.target.container) ||
       At<int(__fastcall*)(void*)>(0x1c7360)(tome) || At<int(__fastcall*)(void*)>(0x1c7360)(target))return false;
    const auto sourceCell=At<uint32_t(__fastcall*)(void*)>(0x34a110)(tome);
    const auto targetCell=At<uint32_t(__fastcall*)(void*)>(0x34a110)(target);
    if(!QolClientIdentify::Cell(work.tome.x,work.tome.y,sourceCell) ||
       !QolClientIdentify::Cell(work.target.x,work.target.y,targetCell) || !At<bool(__fastcall*)()>(0x1e3300)())return false;
    const auto useFlag=At<uint8_t(__fastcall*)(void*)>(0x308e80)(tome);
    work.phase=2;work.start=GetTickCount64();work.polls=0;
    // Preserve native pending-target bookkeeping, never edit charges or ID flags.
    At<void(__fastcall*)(void*)>(0x1c6ac0)(target);
    QolClientIdentify::Submit(At<QolClientIdentify::RequestFn>(0xed1e0),useFlag,
        work.tome.runtimeId,sourceCell,work.target.runtimeId,targetCell,
        QolClientTransfer::NativePage(work.tome.container),QolClientTransfer::NativePage(work.target.container));
    char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] direct-client-request target=%u tome=%u charges=%d; no cursor activation, awaiting client state.",work.target.runtimeId,work.tome.runtimeId,work.before);context->LogInfo(line);
    return true;
}
void __cdecl Tick(const D2RL::PluginContext*,void*) noexcept;
void __cdecl ContinueSdkBulk(const D2RL::PluginContext*,void*) noexcept;
// Game thread for offline work; UI thread for direct remote requests.
// Revalidate each captured item; never include items added later.
void AdvanceBulk() noexcept {
    if(GetTickCount64()-bulkStarted>60000){Finish("[QOL/IdentifyAll] Batch deadline reached.");return;}
    const D2RL::InventoryService* inv{};D2RL::PlayerHandle player{};
    if(context->QueryService(&inv)!=D2RL::ServiceQueryResult::Success || !inv ||
       inv->getLocalPlayer(context,&player)!=D2RL::Inventory::Result::Success || player!=work.player){Finish("[QOL/IdentifyAll] Player changed.");return;}
    QolIdentify::Info tome{};tome.structSize=D2RL::Items::ItemInfoSize;
    int32_t charges{};
    if(items->getItemInfo(context,bulk.tome.handle,&tome)!=D2RL::Items::Result::Success || !QolIdentify::Same(tome,bulk.tome) ||
       (remoteRoute && !QolClientTransfer::Source(bulk.tome,tome)) ||
       !(remoteRoute?ClientQuantity(context,items,tome.handle,charges):QolIdentifyStat::Read(context,items,tome.handle,charges))){Finish("[QOL/IdentifyAll] Selected tome moved or became unavailable.");return;}
    while(bulk.next<bulk.count) {
        const auto expected=bulk.targets[bulk.next++];
        QolIdentify::Info target{};target.structSize=D2RL::Items::ItemInfoSize;
        const bool available=items->getItemInfo(context,expected.handle,&target)==D2RL::Items::Result::Success;
        if(remoteRoute && (!available || !QolClientTransfer::Source(expected,target))) {
            Finish("[QOL/IdentifyAll] Remote target identity changed; remaining work cancelled.");return;
        }
        const auto step=QolIdentify::CheckBulkTarget(expected,target,available,charges);
        if(step==QolIdentify::BulkStep::Changed){Finish("[QOL/IdentifyAll] Inventory changed; remaining work cancelled.");return;}
        if(step==QolIdentify::BulkStep::Skip)continue;
        if(step==QolIdentify::BulkStep::Empty){Finish("[QOL/IdentifyAll] Selected tome is empty.");return;}
        if(!bulkNative) {
            const auto result=QolIdentify::ConsumeTome(context,items,player,target,tome,QolIdentifyStat::Read);
            if(result.status!=QolIdentify::Status::Success) {
                char line[256];std::snprintf(line,sizeof(line),"[QOL/IdentifyAll] SDK stopped result=%s operation=%u target=%u; no native retry.",QolIdentify::Name(result.status),static_cast<unsigned>(result.operation),target.runtimeId);Finish(line);return;
            }
            QolIdentify::Info confirmed{};confirmed.structSize=D2RL::Items::ItemInfoSize;
            if(items->getItemInfo(context,target.handle,&confirmed)!=D2RL::Items::Result::Success || !QolIdentify::Same(target,confirmed) || !(confirmed.stateFlags&D2RL::Items::ItemStateIdentified)) {
                Finish("[QOL/IdentifyAll] SDK target confirmation unavailable; no retry.");return;
            }
            ++bulk.completed;
            // Yield between edits. Revalidate player, source and target each game callback.
            if(threads->runOnGameThread(context,ContinueSdkBulk,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
                Finish("[QOL/IdentifyAll] SDK continuation unavailable.");
            return;
        }
        work={target,tome,player,GetTickCount64()};
        char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyAll] next=%u/%u target=%u code=0x%08X cell=(%d,%d) charges=%d.",bulk.next,bulk.count,target.runtimeId,target.code,target.x,target.y,charges);context->LogInfo(line);
        if(threads->runOnUiThread(context,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
            Finish("[QOL/IdentifyAll] UI scheduling unavailable.");
        return;
    }
    Finish("[QOL/IdentifyAll] Batch complete.");
}
void __cdecl ContinueSdkBulk(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(busy && bulkActive && !bulkNative && ctx==context && reinterpret_cast<uintptr_t>(token)==generation)AdvanceBulk();
}
D2RL::Inventory::IterationAction __cdecl CollectBulk(const D2RL::PluginContext*,const QolIdentify::Info* item,void*) noexcept {
    if(item)bulk.Observe(*item);
    return D2RL::Inventory::IterationAction::Continue;
}
void __cdecl BeginBulk(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(!busy || !bulkActive || ctx!=context || reinterpret_cast<uintptr_t>(token)!=generation)return;
    if(GetTickCount64()-bulkStarted>2000){Finish("[QOL/IdentifyAll] Initial request expired.");return;}
    if(remoteRoute && (!Guard() || !RemoteGuard() || !ControllerQoL::IsControllerUiActive() ||
       !QolStorageFocus::Matches(ctx,bulk.requested))) {
        Finish("[QOL/IdentifyAll] Remote tome selection changed or profile unavailable.");return;
    }
    const D2RL::InventoryService* inv{};D2RL::PlayerHandle player{};
    if(ctx->QueryService(&inv)!=D2RL::ServiceQueryResult::Success || !inv ||
       inv->getLocalPlayer(ctx,&player)!=D2RL::Inventory::Result::Success || player!=work.player){Finish("[QOL/IdentifyAll] Player unavailable.");return;}
    D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,
        bulk.TargetMask()|D2RL::Items::ContainerBit(bulk.requested.container),0};
    if(inv->forEachInventoryItem(ctx,player,&filter,CollectBulk,nullptr)!=D2RL::Inventory::Result::Success || !bulk.found || bulk.overflow) {
        Finish("[QOL/IdentifyAll] Snapshot incomplete or selected tome unavailable; no action.");return;
    }
    if(remoteRoute && !QolClientTransfer::Source(bulk.requested,bulk.tome)) {
        Finish("[QOL/IdentifyAll] Requested tome identity changed; no action.");return;
    }
    AdvanceBulk();
}
void __cdecl Verify(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(!busy || reinterpret_cast<uintptr_t>(token)!=generation || ctx!=context)return;
    D2RL::PlayerHandle current{};
    const D2RL::InventoryService* inv{};
    if(ctx->QueryService(&inv)!=D2RL::ServiceQueryResult::Success || !inv ||
       inv->getLocalPlayer(ctx,&current)!=D2RL::Inventory::Result::Success || current!=work.player) {
        Finish("[QOL/IdentifyNative] Session changed; no retry.");return;
    }
    QolIdentify::Info target{};target.structSize=D2RL::Items::ItemInfoSize;
    int32_t charges{};
    const bool good=items->getItemInfo(ctx,work.target.handle,&target)==D2RL::Items::Result::Success &&
        QolIdentify::Same(target,work.target) && (target.stateFlags&D2RL::Items::ItemStateIdentified) &&
        QolIdentifyStat::Read(ctx,items,work.tome.handle,charges) && charges==work.before-1;
    char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] authoritative=%s charges=%d->%d target=%u; no SDK quantity/identify edits.",good?"confirmed":"unconfirmed",work.before,charges,work.target.runtimeId);
    if(bulkActive && good){context->LogInfo(line);++bulk.completed;AdvanceBulk();return;}
    Finish(line);
}
void TickUnsafe() noexcept {
    __try {
        if(!Guard() || (remoteRoute && !RemoteGuard()) || !CurrentPlayer(work.player) || !ControllerQoL::IsControllerUiActive() || GetTickCount64()-work.start>(bulkActive && work.phase==0?5000u:2000u) || ++work.polls>(bulkActive && work.phase==0?1200u:240u)) {
            char line[224];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] Cancelled/expired phase=%u target=%u elapsedMs=%llu polls=%u; no retry. Any native targeting cursor is left for the player to cancel.",work.phase,work.target.runtimeId,static_cast<unsigned long long>(GetTickCount64()-work.start),work.polls);Finish(line);return;
        }
        if(remoteRoute) {
            QolIdentify::Info source{.structSize=D2RL::Items::ItemInfoSize},target{.structSize=D2RL::Items::ItemInfoSize};
            const bool sourceFound=items->getItemInfo(context,work.tome.handle,&source)==D2RL::Items::Result::Success;
            if(items->getItemInfo(context,work.target.handle,&target)!=D2RL::Items::Result::Success ||
               !QolClientTransfer::Source(work.target,target) ||
               (sourceFound && !QolClientTransfer::Source(work.tome,source)) ||
               (!sourceFound && !(work.scroll && work.phase==2))) {
                Finish("[QOL/IdentifyNative] Remote source/target identity changed; no retry.");return;
            }
            if(work.scroll && work.phase==2) {
                // A loose scroll disappears; do not demand a surviving tome or
                // dereference the now-empty source cell to confirm its use.
                if(!ResolveGrid(work.target.container) || !ResolveGrid(work.tome.container)) {
                    Finish("[QOL/IdentifyNative] Scroll source/target panel closed; no retry.");return;
                }
                auto remaining=At<void*(__fastcall*)(uint32_t,uint32_t)>(0x9a5d0)(work.tome.runtimeId,4);
                if(QolClientIdentify::ScrollConfirmed((target.stateFlags&D2RL::Items::ItemStateIdentified)!=0,sourceFound,remaining!=nullptr)) {
                    Finish("[QOL/IdentifyNative] client-confirmed target identified and exact loose scroll removed; no SDK edits.");return;
                }
                if(threads->runOnUiThread(context,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
                    Finish("[QOL/IdentifyNative] Scroll confirmation scheduling unavailable; no retry.");
                return;
            }
        }
        auto grid=ResolveGrid(work.target.container);
        auto sourceGrid=ResolveGrid(work.tome.container);
        if(!grid || !sourceGrid){Finish("[QOL/IdentifyNative] Required inventory/Personal Stash grid unavailable or tab changed; no retry.");return;}
        auto vt=*reinterpret_cast<uintptr_t*>(grid);
        if(!Slot(context->exeBase,vt-context->exeBase+0x118,context->exeBase+0x2aa7e0,"game") ||
           !Slot(context->exeBase,vt-context->exeBase+0xc8,context->exeBase+0x2c49f0,"game") ||
           !Slot(context->exeBase,vt-context->exeBase+0x100,context->exeBase+0x3e2b274,"game") ||
           !Slot(context->exeBase,0x3e2a7d8,core+0x8150a0,"game")) {
            Finish("[QOL/IdentifyNative] Inventory action route changed; no retry.");return;
        }
        auto owner=At<void*(__fastcall*)(void*)>(0x2a7810)(grid);
        if(!owner || At<void*(__fastcall*)(void*)>(0x2a7810)(sourceGrid)!=owner || (work.owner && owner!=work.owner)){Finish("[QOL/IdentifyNative] Inventory owner changed; no retry.");return;}
        struct Cell {int32_t x,y;};
        Cell sc{work.tome.x,work.tome.y},tc{work.target.x,work.target.y};
        auto lookup=At<void*(__fastcall*)(void*,const Cell*)>(0x2c49f0);
        auto tome=static_cast<unsigned char*>(lookup(sourceGrid,&sc));
        auto target=static_cast<unsigned char*>(lookup(grid,&tc));
        auto code=At<uint32_t(__fastcall*)(void*)>(0x36ef50);
        if(!tome || !target || *reinterpret_cast<unsigned*>(tome)!=4 || *reinterpret_cast<unsigned*>(target)!=4 ||
           *reinterpret_cast<unsigned*>(tome+8)!=work.tome.runtimeId || *reinterpret_cast<unsigned*>(target+8)!=work.target.runtimeId ||
           code(tome)!=work.tome.code || code(target)!=work.target.code) {Finish("[QOL/IdentifyNative] Exact source/target changed; no retry.");return;}
        const auto stat=At<int32_t(__fastcall*)(void*,int32_t,uint16_t)>(0x2f5020);
        const auto flag=At<int32_t(__fastcall*)(void*,uint32_t)>(0x36e2d0);
        void* cursor=At<void*(__fastcall*)()>(0x14f1e0)();
        const auto mode=At<int(__fastcall*)()>(0x14f210)();
        if(work.phase==0) {
            if(cursor || mode==5 || flag(target,0x10) || (!work.scroll && stat(tome,70,0)<=0)){Finish("[QOL/IdentifyNative] Busy cursor, identified target, or empty tome; no action.");return;}
            if(bulkActive) {
                if(!Bytes(context->exeBase,0x1c7360,QolIdentifyProbe::Blocked,sizeof(QolIdentifyProbe::Blocked),"game")) {
                    Finish("[QOL/IdentifyAll] Native readiness contract unavailable; no tome use.");return;
                }
                const auto ready=QolIdentify::CheckBulkReady(At<int(__fastcall*)(void*)>(0x1c7360)(tome)!=0,GetTickCount64()-work.start);
                if(ready==QolIdentify::BulkReady::Expired){Finish("[QOL/IdentifyAll] Native tome remained blocked; no use attempted.");return;}
                if(ready==QolIdentify::BulkReady::Wait) {
                    if(!work.waited){context->LogInfo("[QOL/IdentifyAll] Native tome blocked; waiting for game readiness.");work.waited=true;}
                    if(threads->runOnUiThread(context,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
                        Finish("[QOL/IdentifyAll] Readiness continuation unavailable.");
                    return;
                }
                if(work.waited)context->LogInfo("[QOL/IdentifyAll] Native tome ready; continuing batch.");
            }
            work.owner=owner;work.before=work.scroll?1:stat(tome,70,0);
            if(remoteRoute) {
                if(!SubmitRemote(tome,target,owner)){Finish("[QOL/IdentifyNative] Direct remote preflight refused; no cursor fallback.");return;}
            } else {
                work.phase=1;work.start=GetTickCount64();work.polls=0;
                At<void(__fastcall*)(void*,void*)>(0x2aa7e0)(sourceGrid,tome);
                if(bulkActive) {
                    char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyAll] post-hold target=%u cursor=%p mode=%d.",work.target.runtimeId,At<void*(__fastcall*)()>(0x14f1e0)(),At<int(__fastcall*)()>(0x14f210)());context->LogInfo(line);
                }
                context->LogInfo("[QOL/IdentifyNative] Native hold-A tome action sent; waiting for its targeting mode.");
            }
        } else if(work.phase==1) {
            if(flag(target,0x10)){Finish("[QOL/IdentifyNative] Target identified externally; no target activation.");return;}
            if(stat(tome,70,0)!=work.before){Finish("[QOL/IdentifyNative] Tome quantity changed before target activation; no action.");return;}
            if(cursor && (cursor!=tome || mode!=5)){Finish("[QOL/IdentifyNative] Foreign cursor; no target activation.");return;}
            if(cursor==tome && mode==5) {
                context->LogInfo("[QOL/IdentifyNative] Targeting mode observed; activating target.");
                work.phase=2;uint64_t cell{};std::memcpy(&cell,&tc,sizeof(cell));
                forwarding=true;
                reinterpret_cast<void(__fastcall*)(void*,uint64_t)>(*reinterpret_cast<uintptr_t*>(vt+0x100))(grid,cell);
                forwarding=false;
            }
        } else if(QolClientIdentify::Confirmed(flag(target,0x10)!=0,work.before,stat(tome,70,0))) {
            work.phase=3;
            if(remoteRoute) {
                char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] client-confirmed target=%u charges=%d->%d; native request only, no SDK edits.",work.target.runtimeId,work.before,work.before-1);
                if(bulkActive){context->LogInfo(line);++bulk.completed;AdvanceBulk();return;}
                Finish(line);return;
            }
            if(threads->runOnGameThread(context,Verify,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
                Finish("[QOL/IdentifyNative] Client observed identification and one charge consumed; authoritative confirmation unavailable, no retry.");
            return;
        }
        if(threads->runOnUiThread(context,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success)
            Finish("[QOL/IdentifyNative] UI continuation unavailable; no retry.");
    } __except(EXCEPTION_EXECUTE_HANDLER) {Finish("[QOL/IdentifyNative] Native contract fault; disabled, no retry.");admitted=false;}
}
void __cdecl Tick(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(busy && ctx==context && reinterpret_cast<uintptr_t>(token)==generation)TickUnsafe();
}
}
void Initialize(const D2RL::PluginContext* ctx,const D2RL::ItemService* service,const D2RL::ThreadService* ts) noexcept {
    std::lock_guard lock(mutex);context=ctx;items=service;threads=ts;
    ++generation;busy=false;remoteRoute=false;bulkActive=false;sessionEvents=false;
    const D2RL::LifecycleService* lifecycle{};
    if(ctx && ctx->QueryService(&lifecycle)==D2RL::ServiceQueryResult::Success &&
       D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) && lifecycle->registerGameplayEventListener) {
        sessionEvents=true;
        for(auto kind:{D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft}) {
            const D2RL::Lifecycle::GameplayEventListener listener{D2RL::Lifecycle::GameplayEventListenerSize,0,kind,0,SessionChanged,nullptr};
            D2RL::Lifecycle::ListenerHandle handle{};
            if(lifecycle->registerGameplayEventListener(ctx,&listener,&handle)!=D2RL::Lifecycle::Result::Success)sessionEvents=false;
        }
    }
    auto module=GetModuleHandleW(L"D2RCore.dll");core=reinterpret_cast<uintptr_t>(module);
    unsigned char actual[32]{};bool readable=false;
    const bool hashMatch=QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash,actual,&readable);
    char expectedText[65]{},actualText[65]{};
    for(unsigned i=0;i<32;++i){std::snprintf(expectedText+i*2,3,"%02X",static_cast<unsigned>(QolNativeProfile::CoreHash[i]));std::snprintf(actualText+i*2,3,"%02X",static_cast<unsigned>(actual[i]));}
    if(ctx){char line[320];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] admission coreSHA256=%s expected=%s itemService=%u uiScheduler=%u gameScheduler=%u.",readable?actualText:"unreadable",expectedText,service?1u:0u,(ts && ts->runOnUiThread)?1u:0u,(ts && ts->runOnGameThread)?1u:0u);ctx->LogInfo(line);}
    admissionReason=!ctx?"missing-context":!service?"missing-item-service":(!ts || !ts->runOnUiThread || !ts->runOnGameThread)?"missing-scheduler":!hashMatch?(readable?"core-hash-mismatch":"core-hash-unreadable"):"initial-contract-mismatch";
    admitted=ctx && service && ts && ts->runOnUiThread && ts->runOnGameThread && hashMatch;
    admitted=Guard();
    if(admitted && ctx)ctx->LogInfo("[QOL/IdentifyNative] admission=accepted.");
}
void Shutdown() noexcept {std::lock_guard lock(mutex);++generation;busy=false;bulkActive=false;admitted=false;context=nullptr;}
bool Forwarding() noexcept {return forwarding;}
bool Busy() noexcept {return busy.load();}
static bool PersonalStashOpenGuarded() noexcept {
    __try {return context && admitted && Guard() && ResolveGrid(QolIdentify::Container::PersonalStash)!=nullptr;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool PersonalStashOpen() noexcept { std::lock_guard lock(mutex);return PersonalStashOpenGuarded(); }
bool RequestAll(const D2RL::PluginContext* ctx,D2RL::PlayerHandle player,const QolIdentify::Info& tome,bool nativeMode,bool allowRemote) noexcept {
    std::lock_guard lock(mutex);
    if(ctx!=context || !items || !items->getItemInfo || !items->editItem || !threads || !threads->runOnGameThread || !QolIdentify::BulkTome(tome) || (nativeMode && !Guard()))return false;
    if(tome.container==QolIdentify::Container::PersonalStash && !PersonalStashOpen())return false;
    if(busy){ctx->LogWarn("[QOL/IdentifyAll] Existing identification pending; duplicate ignored.");return true;}
    remoteRoute=false;bulk={};bulk.includeCube=!nativeMode;bulk.requested=tome;bulkActive=true;bulkNative=nativeMode;bulkStarted=GetTickCount64();
    work={};work.player=player;work.start=bulkStarted;busy=true;++generation;
    const auto scheduled=threads->runOnGameThread(ctx,BeginBulk,reinterpret_cast<void*>(generation));
    if(scheduled==D2RL::Threads::Result::Success) {
        ctx->LogInfo(nativeMode?"[QOL/IdentifyAll] Requested native inventory batch using highlighted tome.":"[QOL/IdentifyAll] Requested SDK inventory and Cube batch using highlighted tome.");return true;
    }
    if(scheduled==D2RL::Threads::Result::Unavailable && allowRemote && sessionEvents &&
       QolIdentify::NativeContainer(tome.container) && Guard() && RemoteGuard()) {
        remoteRoute=true;bulkNative=true;bulk.includeCube=false;
        if(threads->runOnUiThread(ctx,BeginBulk,reinterpret_cast<void*>(generation))==D2RL::Threads::Result::Success) {
            ctx->LogInfo("[QOL/IdentifyAll] Remote inventory batch queued using highlighted tome; one confirmed native request at a time.");return true;
        }
    }
    Finish("[QOL/IdentifyAll] Game scheduling unavailable.");return false;
}
int Request(const D2RL::PluginContext* ctx,D2RL::PlayerHandle player,const QolIdentify::Info& target,const QolIdentify::Info& tome) noexcept {
    if(!QolIdentify::NativeContainer(target.container) || !QolIdentify::NativeContainer(tome.container))return 0;
    std::lock_guard lock(mutex);
    if(ctx!=context){if(ctx)ctx->LogWarn("[QOL/IdentifyNative] refused=context-mismatch.");return -1;}
    if(!Guard())return -1;
    if(busy) {
        ctx->LogWarn("[QOL/IdentifyNative] refused=request-already-pending.");return -1;
    }
    remoteRoute=false;work={target,tome,player,GetTickCount64()};busy=true;++generation;
    const auto scheduled=threads->runOnUiThread(ctx,Tick,reinterpret_cast<void*>(generation));
    char line[160];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] initial-ui-schedule=%u target=%u tome=%u.",static_cast<unsigned>(scheduled),target.runtimeId,tome.runtimeId);ctx->LogInfo(line);
    if(scheduled==D2RL::Threads::Result::Success)return 1;
    busy=false;return -1;
}
bool RequestRemote(const D2RL::PluginContext* ctx,D2RL::PlayerHandle player,const QolIdentify::Info& target) noexcept {
    std::lock_guard lock(mutex);
    if(ctx!=context || !sessionEvents || !items || !items->getItemInfo || !Guard() || !RemoteGuard() ||
       !CurrentPlayer(player) || !QolIdentify::NativeContainer(target.container))return false;
    if(busy || QolClientTransfer::Busy())return true; // Consume a duplicate; never also pick up the target.
    const D2RL::InventoryService* inventory{};
    if(ctx->QueryService(&inventory)!=D2RL::ServiceQueryResult::Success || !inventory || !inventory->forEachInventoryItem)return false;
    const bool personal=PersonalStashOpenGuarded();
    if(target.container==QolIdentify::Container::PersonalStash && !personal)return false;
    QolIdentify::Request request{player,target,true,true,personal};
    QolIdentify::Scan scan{};scan.request=&request;scan.items=items;scan.readQuantity=ClientQuantity;
    const D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,D2RL::Items::ContainerBit(QolIdentify::Container::Inventory)|
        (personal?D2RL::Items::ContainerBit(QolIdentify::Container::PersonalStash):0u),0};
    if(inventory->forEachInventoryItem(ctx,player,&filter,[](const D2RL::PluginContext* ctx,const QolIdentify::Info* info,void* user) noexcept {
        auto& scan=*static_cast<QolIdentify::Scan*>(user);
        const auto previousScroll=scan.scroll;
        const auto action=QolIdentify::Visit(ctx,info,user);
        scan.scroll=previousScroll; // Remote consumption admits loose scrolls only.
        if(info && QolIdentify::NativeContainer(info->container) && QolIdentify::Code(info->code,D2RL::Items::MakeItemCode("isc")) && QolClientIdentify::LooseScrollQuantity(info->quantity) &&
           (!scan.scroll.handle || (scan.scroll.container!=QolIdentify::Container::Inventory && info->container==QolIdentify::Container::Inventory)))scan.scroll=*info;
        return action;
    },&scan)!=D2RL::Inventory::Result::Success ||
       !scan.found || !QolClientTransfer::Source(target,scan.target) || (!scan.tome.handle && !scan.scroll.handle) || (scan.target.stateFlags&D2RL::Items::ItemStateIdentified)) {
        ctx->LogWarn("[QOL/IdentifyNative] Remote ID needs an unchanged Inventory/Personal target and an eligible charged tome or loose scroll; no action.");return false;
    }
    const auto supply=scan.tome.handle?scan.tome:scan.scroll;
    if(!scan.tome.handle && supply.quantity>1)return false; // Unqualified modded scroll stacks.
    work={scan.target,supply,player,GetTickCount64()};work.scroll=!scan.tome.handle;remoteRoute=true;bulkActive=false;busy=true;++generation;
    if(threads->runOnUiThread(ctx,Tick,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success) {
        Finish("[QOL/IdentifyNative] Remote UI scheduling unavailable; no action.");return false;
    }
    ctx->LogInfo("[QOL/IdentifyNative] Remote single ID queued through direct native request.");return true;
}
}
