#include "stash_feedback.h"
#include "stash_feedback_policy.h"
#include <array>
#include <mutex>
#include <windows.h>
#ifndef INVENTORY_DEPOSIT_FEEDBACK_TESTS
#include "stash_feedback_profile.h"
#include "gold_anchor_profile.h"
#include "native_identify_profile.h"
#include "native_input_profile.h"
#include "materials_signatures.h"
#include "aim/tree_layout_profile.h"
#include "core_compatibility.h"
#include <cmath>
#include <cstring>

namespace InventoryDepositStashFeedback::Detail {
namespace {
template<class T> T Read(const void* object,std::size_t offset) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(value));return value;
}
template<class T> T At(const D2RL::PluginContext* ctx,std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(ctx->exeBase+rva);
}
bool Match(const D2RL::PluginContext* ctx,std::uintptr_t rva,const unsigned char* bytes,std::size_t size) noexcept {
    return size<=UINT32_MAX && ctx->CheckExpectedBytes(rva,bytes,static_cast<std::uint32_t>(size));
}
bool Same(const D2RL::Widgets::Rect& a,const D2RL::Widgets::Rect& b) noexcept {
    return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height;
}
bool FrameUnsafe(const D2RL::PluginContext* ctx,const D2RL::WidgetService* widgets,Frame& frame) noexcept {
    using R=D2RL::Widgets::Result;
    D2RL::Widgets::WidgetHandle bankHandle{},tabsHandle{};
    D2RL::Widgets::Rect sdkBank{},sdkTabs{},nativeBank{},nativeTabs{};
    if(widgets->findPanel(ctx,"BankExpansionLayout",&bankHandle)!=R::Success || !bankHandle ||
       widgets->findWidget(ctx,bankHandle,"BankTabs",&tabsHandle)!=R::Success || !tabsHandle ||
       widgets->getWidgetRect(ctx,bankHandle,&sdkBank)!=R::Success ||
       widgets->getWidgetRect(ctx,tabsHandle,&sdkTabs)!=R::Success)return false;
    auto bank=At<void*(__fastcall*)(const char*)>(ctx,0x846170)("BankExpansionLayout");
    if(!bank || Read<std::uintptr_t>(bank,0)!=ctx->exeBase+InventoryDepositStashProfile::BankVtableRva)return false;
    const auto count=Read<std::uint64_t>(bank,0x60);
    const auto children=Read<const void*>(bank,0x58);
    if(!children || !count || count>256)return false;
    for(std::uint64_t i=0;i<count;++i)if(!Read<const void*>(children,static_cast<std::size_t>(i)*sizeof(void*)))return false;
    auto tabs=At<void*(__fastcall*)(void*,const char*)>(ctx,InventoryDepositGoldProfile::FindChildRva)(bank,"BankTabs");
    if(!tabs || Read<const void*>(tabs,0x30)!=bank ||
       Read<std::uintptr_t>(tabs,0)!=ctx->exeBase+InventoryDepositStashProfile::TabVtableRva)return false;
    auto rect=At<D2RL::Widgets::Rect*(__fastcall*)(void*,D2RL::Widgets::Rect*)>(ctx,0x8562a0);
    rect(bank,&nativeBank);rect(tabs,&nativeTabs);
    if(!Same(sdkBank,nativeBank) || !Same(sdkTabs,nativeTabs))return false;
    const auto seconds=Read<float>(bank,0x240);
    const auto cycles=Read<std::int32_t>(bank,0x244);
    const auto nativeTab=Read<unsigned char>(bank,0x260);
    // Keep malformed replacement layouts from creating an unbounded effect.
    if(!std::isfinite(seconds) || seconds<0.1f || seconds>2.0f || cycles<1 || cycles>10 || nativeTab>5)return false;
    frame.bank=bank;frame.tabs=tabs;frame.nativeTab=nativeTab;
    frame.fadeMs=static_cast<std::uint32_t>(std::lround(seconds*1000.0f));
    frame.cycles=static_cast<std::uint32_t>(cycles);
    frame.open=At<bool(__fastcall*)(std::uint32_t)>(ctx,0xce500)(0x18);
    return true;
}
}
bool Admit(const D2RL::PluginContext* ctx) noexcept {
    if(!ctx || !ctx->exeBase || !GetModuleHandleW(L"D2RCore.dll"))return false;
    for(const auto& site:InventoryDepositStashProfile::Sites)
        if(!Match(ctx,site.rva,site.bytes,site.size))return false;
    constexpr unsigned char one[]={0x00,0x00,0x80,0x3f};
    return Match(ctx,0x1cba904,one,sizeof(one)) &&
        Match(ctx,0x846170,QolNativeIdentifyProfile::FindPanel,sizeof(QolNativeIdentifyProfile::FindPanel)) &&
        Match(ctx,InventoryDepositGoldProfile::FindChildRva,InventoryDepositGoldProfile::FindChild,sizeof(InventoryDepositGoldProfile::FindChild)) &&
        Match(ctx,0x8562a0,TreeLayoutNative::Rect,sizeof(TreeLayoutNative::Rect)) &&
        Match(ctx,0xce500,QolMaterials::Signatures::UiMode,sizeof(QolMaterials::Signatures::UiMode));
}
bool ReadFrame(const D2RL::PluginContext* ctx,const D2RL::WidgetService* widgets,Frame& frame) noexcept {
    __try{return FrameUnsafe(ctx,widgets,frame);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
std::uint8_t Category(const D2RL::PluginContext* ctx,std::uint32_t classId) noexcept {
    __try{return At<std::uint8_t(__fastcall*)(std::uint32_t)>(ctx,InventoryDepositStashProfile::CategoryRva)(classId);}
    __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
void Enable(const D2RL::PluginContext* ctx,void* tabs,unsigned tab,bool visible) noexcept {
    At<void(__fastcall*)(void*,int,bool)>(ctx,InventoryDepositStashProfile::EnableRva)(tabs,static_cast<int>(tab),visible);
}
void Alpha(const D2RL::PluginContext* ctx,void* tabs,unsigned tab,float alpha) noexcept {
    At<void(__fastcall*)(void*,int,float)>(ctx,InventoryDepositStashProfile::AlphaRva)(tabs,static_cast<int>(tab),alpha);
}
std::uint64_t Clock() noexcept {return GetTickCount64();}
}
#endif

namespace InventoryDepositStashFeedback {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};
const D2RL::WidgetService* widgets{};
const D2RL::ThreadService* threads{};
const D2RL::LifecycleService* lifecycle{};
std::array<D2RL::Lifecycle::ListenerHandle,2> listeners{};
std::array<bool,5> owned{};
Policy::Mailbox mailbox;
Policy::Timers timers;
std::uintptr_t generation{},ownerGeneration{};
DWORD uiThread{};
bool ready{},queued{},cleanup{},warned{};

bool AnyOwned() noexcept {return owned[2] || owned[3] || owned[4];}
void ClearOwned(const Detail::Frame& frame) noexcept {
    for(unsigned tab=2;tab<=4;++tab) {
        // Do not cancel the game's latest ordinary deposit notification.
        if(owned[tab] && tab!=frame.nativeTab)Detail::Enable(context,frame.tabs,tab,false);
        owned[tab]=false;
    }
}
void UpdateUnsafe(bool forceClear) noexcept {
    if(!Detail::Admit(context)) {
        ready=false;mailbox.Clear();timers.Clear();owned={};
        context->LogWarn("[InventoryDeposit] Stash feedback profile changed; feedback disabled, deposit unaffected.");
        return;
    }
    Detail::Frame frame;
    if(!Detail::ReadFrame(context,widgets,frame)) {
        // A destroyed/replaced panel cannot be touched or retained. Normal
        // close keeps the live panel resolvable and uses ClearOwned below.
        mailbox.Clear();timers.Clear();owned={};cleanup=false;return;
    }
    if(forceClear || !frame.open) {
        ClearOwned(frame);mailbox.Clear();timers.Clear();cleanup=false;return;
    }
    if(cleanup) {
        // Session/queue-failure cancellation already discarded the old
        // mailbox. A newer batch may have arrived before this UI callback;
        // release old visuals without discarding its fresh confirmations.
        ClearOwned(frame);timers.Clear();cleanup=false;
    }
    const auto now=Detail::Clock();
    while(mailbox.next<mailbox.count) {
        const auto index=mailbox.next++;
        const auto category=Detail::Category(context,mailbox.classes[index]);
        if(category>=1 && category<=3)
            timers.Mark(static_cast<unsigned>(category)+1,mailbox.batches[index],now,frame.fadeMs,frame.cycles);
    }
    for(unsigned tab=2;tab<=4;++tab) {
        if(!timers.Marked(tab))continue;
        if(!timers.Active(tab,now)) {
            if(owned[tab] && tab!=frame.nativeTab)Detail::Enable(context,frame.tabs,tab,false);
            owned[tab]=false;timers.Clear(tab);continue;
        }
        // Vanilla already animates its one current tab. Leave that animation
        // alone; replay only the additional confirmed destination tabs.
        if(tab==frame.nativeTab)continue;
        Detail::Enable(context,frame.tabs,tab,true);
        Detail::Alpha(context,frame.tabs,tab,timers.Alpha(tab,now));
        owned[tab]=true;
    }
}
void Update(bool forceClear) noexcept {
#ifdef INVENTORY_DEPOSIT_FEEDBACK_TESTS
    UpdateUnsafe(forceClear);
#else
    __try {UpdateUnsafe(forceClear);}
    __except(EXCEPTION_EXECUTE_HANDLER) {
        ready=false;mailbox.Clear();timers.Clear();owned={};
        if(context)context->LogWarn("[InventoryDeposit] Stash feedback native access fault; feedback disabled, deposit unaffected.");
    }
#endif
}
void __cdecl Apply(const D2RL::PluginContext* ctx,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(!ready || ctx!=context || reinterpret_cast<std::uintptr_t>(token)!=generation)return;
    queued=false;uiThread=GetCurrentThreadId();Update(false);
}
void __cdecl Session(const D2RL::PluginContext* ctx,const D2RL::Lifecycle::GameplayEvent*,void* token) noexcept {
    std::lock_guard lock(mutex);
    if(!ready || ctx!=context || reinterpret_cast<std::uintptr_t>(token)!=ownerGeneration)return;
    ++generation;queued=false;mailbox.Clear();timers.Clear();cleanup=AnyOwned();
    // This listener only invalidates copied state; the next scheduled UI task
    // performs cleanup. No stale game callback can become a new flash.
}
}
static bool Register(const D2RL::PluginContext* ctx) noexcept {
    if(!ctx || !Detail::Admit(ctx))return false;
#ifndef INVENTORY_DEPOSIT_FEEDBACK_TESTS
    if(!QolCore::VerifyFileHash(GetModuleHandleW(L"D2RCore.dll"),QolNativeProfile::CoreHash))return false;
#endif
    std::lock_guard lock(mutex);context=ctx;
    if(ctx->QueryService(&widgets)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasWidgetServiceField(widgets,D2RL::WidgetServiceRequiredSize) ||
       !widgets->findPanel || !widgets->findWidget || !widgets->getWidgetRect ||
       ctx->QueryService(&threads)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasThreadServiceField(threads,D2RL::ThreadServiceRequiredSize) || !threads->runOnUiThread ||
       ctx->QueryService(&lifecycle)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasLifecycleServiceField(lifecycle,D2RL::LifecycleServiceRequiredSize) ||
       !lifecycle->registerGameplayEventListener || !lifecycle->unregisterGameplayEventListener)return false;
    constexpr D2RL::Lifecycle::GameplayEventKind kinds[]={D2RL::Lifecycle::GameplayEventKind::GameJoined,D2RL::Lifecycle::GameplayEventKind::GameLeft};
    for(std::size_t i=0;i<listeners.size();++i) {
        const D2RL::Lifecycle::GameplayEventListener listener{
            .structSize=D2RL::Lifecycle::GameplayEventListenerSize,.kind=kinds[i],.callback=Session,
            .userData=reinterpret_cast<void*>(ownerGeneration)};
        if(lifecycle->registerGameplayEventListener(ctx,&listener,&listeners[i])!=D2RL::Lifecycle::Result::Success)return false;
    }
    ready=true;ctx->LogInfo("[InventoryDeposit] Confirmed-deposit stash tab feedback admitted.");return true;
}
bool Initialize(const D2RL::PluginContext* ctx) noexcept {
    Shutdown();
    if(Register(ctx))return true;
    Shutdown();
    if(ctx)ctx->LogWarn("[InventoryDeposit] Stash feedback unavailable; deposit remains enabled.");
    return false;
}
void BeginBatch() noexcept {std::lock_guard lock(mutex);if(ready)mailbox.Begin();}
void ItemRemoved(const D2RL::Items::ItemInfo& item) noexcept {
    std::lock_guard lock(mutex);if(ready)(void)mailbox.Observe(item.runtimeId,item.classId);
}
void EndBatch() noexcept {std::lock_guard lock(mutex);if(ready)mailbox.End();}
void Refresh() noexcept {
    std::lock_guard lock(mutex);
    if(!ready || queued || (mailbox.next==mailbox.count && !timers.Any() && !cleanup && !AnyOwned()))return;
    queued=true;
    if(threads->runOnUiThread(context,Apply,reinterpret_cast<void*>(generation))!=D2RL::Threads::Result::Success) {
        queued=false;mailbox.Clear();timers.Clear();cleanup=AnyOwned();
        if(!warned)context->LogWarn("[InventoryDeposit] Stash feedback UI scheduling unavailable; deposit unaffected.");
        warned=true;
    } else warned=false;
}
void Shutdown() noexcept {
    const D2RL::PluginContext* oldContext{};
    const D2RL::LifecycleService* oldLifecycle{};
    std::array<D2RL::Lifecycle::ListenerHandle,2> oldListeners{};
    {
        std::lock_guard lock(mutex);
        // Never write native UI state from an arbitrary unload thread. The
        // SDK drops pending UI work once unloading begins (see docs).
        if(ready && AnyOwned() && uiThread==GetCurrentThreadId())Update(true);
        ready=false;++generation;++ownerGeneration;
        oldContext=context;oldLifecycle=lifecycle;oldListeners=listeners;
        context=nullptr;widgets=nullptr;threads=nullptr;lifecycle=nullptr;listeners={};
        owned={};mailbox.Clear();timers.Clear();queued=cleanup=warned=false;uiThread=0;
    }
    if(oldContext && oldLifecycle && oldLifecycle->unregisterGameplayEventListener)
        for(auto handle:oldListeners)if(handle)(void)oldLifecycle->unregisterGameplayEventListener(oldContext,handle);
}
}
