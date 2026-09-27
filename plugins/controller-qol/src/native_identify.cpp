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
#include <mutex>
#include <cstdio>
namespace QolNativeIdentify {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};
const D2RL::ItemService* items{};
const D2RL::ThreadService* threads{};
uintptr_t core{};
bool admitted{},busy{};
thread_local bool forwarding{};
uint64_t generation{};
QolIdentify::BulkPlan bulk{};bool bulkActive{},bulkNative{};ULONGLONG bulkStarted{};
struct Work {QolIdentify::Info target{},tome{};D2RL::PlayerHandle player{};ULONGLONG start{};void* owner{};int before{};unsigned phase{},polls{};bool waited{};} work;
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
            Slot(context->exeBase,0x3e2a218,core+0x831de0,"game") &&
            Bytes(core,0x831de0,Wrapper,sizeof(Wrapper),"core") &&
            Bytes(core,0x3d8bd0,Body,sizeof(Body),"core") &&
            Slot(core,0x70e9c8,context->exeBase+0x2c7540,"core") &&
            Slot(core,0x70e968,context->exeBase+0x2c49f0,"core");
    } __except(EXCEPTION_EXECUTE_HANDLER) {if(context)context->LogWarn("[QOL/IdentifyNative] refused=guard-read-fault.");return false;}
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
void __cdecl Tick(const D2RL::PluginContext*,void*) noexcept;
void __cdecl ContinueSdkBulk(const D2RL::PluginContext*,void*) noexcept;
// Game-thread only. Revalidate each captured item; never include items added later.
void AdvanceBulk() noexcept {
    if(GetTickCount64()-bulkStarted>60000){Finish("[QOL/IdentifyAll] Batch deadline reached.");return;}
    const D2RL::InventoryService* inv{};D2RL::PlayerHandle player{};
    if(context->QueryService(&inv)!=D2RL::ServiceQueryResult::Success || !inv ||
       inv->getLocalPlayer(context,&player)!=D2RL::Inventory::Result::Success || player!=work.player){Finish("[QOL/IdentifyAll] Player changed.");return;}
    QolIdentify::Info tome{};tome.structSize=D2RL::Items::ItemInfoSize;
    int32_t charges{};
    if(items->getItemInfo(context,bulk.tome.handle,&tome)!=D2RL::Items::Result::Success || !QolIdentify::Same(tome,bulk.tome) ||
       !QolIdentifyStat::Read(context,items,tome.handle,charges)){Finish("[QOL/IdentifyAll] Selected tome moved or became unavailable.");return;}
    while(bulk.next<bulk.count) {
        const auto expected=bulk.targets[bulk.next++];
        QolIdentify::Info target{};target.structSize=D2RL::Items::ItemInfoSize;
        const bool available=items->getItemInfo(context,expected.handle,&target)==D2RL::Items::Result::Success;
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
    const D2RL::InventoryService* inv{};D2RL::PlayerHandle player{};
    if(ctx->QueryService(&inv)!=D2RL::ServiceQueryResult::Success || !inv ||
       inv->getLocalPlayer(ctx,&player)!=D2RL::Inventory::Result::Success || player!=work.player){Finish("[QOL/IdentifyAll] Player unavailable.");return;}
    D2RL::Inventory::ItemFilter filter{D2RL::Inventory::ItemFilterSize,0,
        bulk.TargetMask()|D2RL::Items::ContainerBit(bulk.requested.container),0};
    if(inv->forEachInventoryItem(ctx,player,&filter,CollectBulk,nullptr)!=D2RL::Inventory::Result::Success || !bulk.found || bulk.overflow) {
        Finish("[QOL/IdentifyAll] Snapshot incomplete or selected tome unavailable; no action.");return;
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
        if(!Guard() || !ControllerQoL::IsControllerUiActive() || GetTickCount64()-work.start>(bulkActive && work.phase==0?5000u:2000u) || ++work.polls>(bulkActive && work.phase==0?1200u:240u)) {
            char line[224];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] Cancelled/expired phase=%u target=%u elapsedMs=%llu polls=%u; no retry. Any native targeting cursor is left for the player to cancel.",work.phase,work.target.runtimeId,static_cast<unsigned long long>(GetTickCount64()-work.start),work.polls);Finish(line);return;
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
            if(cursor || mode==5 || flag(target,0x10) || stat(tome,70,0)<=0){Finish("[QOL/IdentifyNative] Busy cursor, identified target, or empty tome; no action.");return;}
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
            work.owner=owner;work.before=stat(tome,70,0);work.phase=1;work.start=GetTickCount64();work.polls=0;
            At<void(__fastcall*)(void*,void*)>(0x2aa7e0)(sourceGrid,tome);
            if(bulkActive) {
                char line[192];std::snprintf(line,sizeof(line),"[QOL/IdentifyAll] post-hold target=%u cursor=%p mode=%d.",work.target.runtimeId,At<void*(__fastcall*)()>(0x14f1e0)(),At<int(__fastcall*)()>(0x14f210)());context->LogInfo(line);
            }
            context->LogInfo("[QOL/IdentifyNative] Native hold-A tome action sent; waiting for its targeting mode.");
        } else if(work.phase==1) {
            if(flag(target,0x10)){Finish("[QOL/IdentifyNative] Target identified externally; no target activation.");return;}
            if(cursor && (cursor!=tome || mode!=5)){Finish("[QOL/IdentifyNative] Foreign cursor; no target activation.");return;}
            if(cursor==tome && mode==5) {
                context->LogInfo("[QOL/IdentifyNative] Targeting mode observed; activating target.");
                work.phase=2;uint64_t cell{};std::memcpy(&cell,&tc,sizeof(cell));
                forwarding=true;
                reinterpret_cast<void(__fastcall*)(void*,uint64_t)>(*reinterpret_cast<uintptr_t*>(vt+0x100))(grid,cell);
                forwarding=false;
            }
        } else if(flag(target,0x10) && stat(tome,70,0)==work.before-1) {
            work.phase=3;
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
static bool PersonalStashOpenGuarded() noexcept {
    __try {return context && admitted && Guard() && ResolveGrid(QolIdentify::Container::PersonalStash)!=nullptr;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool PersonalStashOpen() noexcept { std::lock_guard lock(mutex);return PersonalStashOpenGuarded(); }
bool RequestAll(const D2RL::PluginContext* ctx,D2RL::PlayerHandle player,const QolIdentify::Info& tome,bool nativeMode) noexcept {
    std::lock_guard lock(mutex);
    if(ctx!=context || !items || !items->getItemInfo || !items->editItem || !threads || !threads->runOnGameThread || !QolIdentify::BulkTome(tome) || (nativeMode && !Guard()))return false;
    if(tome.container==QolIdentify::Container::PersonalStash && !PersonalStashOpen())return false;
    if(busy){ctx->LogWarn("[QOL/IdentifyAll] Existing identification pending; duplicate ignored.");return true;}
    bulk={};bulk.includeCube=!nativeMode;bulk.requested=tome;bulkActive=true;bulkNative=nativeMode;bulkStarted=GetTickCount64();
    work={};work.player=player;work.start=bulkStarted;busy=true;++generation;
    if(threads->runOnGameThread(ctx,BeginBulk,reinterpret_cast<void*>(generation))==D2RL::Threads::Result::Success) {
        ctx->LogInfo(nativeMode?"[QOL/IdentifyAll] Requested native inventory batch using highlighted tome.":"[QOL/IdentifyAll] Requested SDK inventory and Cube batch using highlighted tome.");return true;
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
    work={target,tome,player,GetTickCount64()};busy=true;++generation;
    const auto scheduled=threads->runOnUiThread(ctx,Tick,reinterpret_cast<void*>(generation));
    char line[160];std::snprintf(line,sizeof(line),"[QOL/IdentifyNative] initial-ui-schedule=%u target=%u tome=%u.",static_cast<unsigned>(scheduled),target.runtimeId,tome.runtimeId);ctx->LogInfo(line);
    if(scheduled==D2RL::Threads::Result::Success)return 1;
    busy=false;return -1;
}
}
