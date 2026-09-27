#include <D2RLPlugin/api.h>
#include <windows.h>
#include <bcrypt.h>
#include <xinput.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <mutex>
#include <atomic>
#include "policy.h"
#include "filter_focus_signatures.h"
#include "qol_navigation.h"
#include "qol_glyphs.h"
#include "physical_input.h"
#include "controller_input.h"
#include "core_compatibility.h"
#include "native_signatures.h"
#include "label_signatures.h"
#include "label_recovery_signatures.h"
#include "menu_signatures.h"
#include "menu_route_profile.h"
#include "native_input_profile.h"
#include "native_ranges.h"
#include "skills_signatures.h"
#include "shared_page_signatures.h"
#include "shared_page_policy.h"

namespace {
const D2RL::PluginContext* ctx=nullptr;
std::recursive_mutex stateMutex;
const D2RL::SharedEventService* events=nullptr;
D2RL::SharedEvents::ListenerHandle uiHandle=0, tooltipHandle=0;
bool enabled=true, trace=false;
bool useL1=true;
bool screenMode=false;
bool nativeMode=true, nativeHookInstalled=false;
uint64_t nativeCalls=0,nativeControllerCalls=0,nativeBlocked=0,nativeTraceLines=0;
using TabMessageFn=void(__fastcall*)(void*,void*);
TabMessageFn originalTabMessage=nullptr;
uintptr_t core=0;
bool nativeVerified=false;
using LabelActionFn=void(__fastcall*)(unsigned char);
using LabelSetFn=void(__fastcall*)(unsigned char,bool);
using RefreshLabelsFn=void(__fastcall*)();
using InGameFn=bool(__cdecl*)() noexcept;
LabelActionFn originalLabelPress=nullptr,originalLabelRelease=nullptr;
InGameFn isInGame=nullptr;
bool labelHooks=false;
bool labelRecoveryReady=false,labelRecoveryQueued=false;
ULONGLONG lastLabelCheck=0;
bool labelRefreshNeeded=true;
uint64_t labelPressBlocked=0,labelReleaseBlocked=0,labelEnforced=0;
thread_local bool updatingLabels=false;
int ReadFilteredLabelState() noexcept {
    __try {
        auto base=ctx->exeBase;
        auto count=*reinterpret_cast<const uintptr_t*>(base+0x235d368);
        auto states=*reinterpret_cast<const unsigned char* const*>(base+0x235d360);
        if(count!=2 || !states || states[0]>1 || states[1]>1) return -1;
        return states[0];
    } __except(EXCEPTION_EXECUTE_HANDLER) {return -1;}
}
bool LabelLockActive(unsigned channel=0) noexcept {
    return ctx && Probe::LockFilteredLabels(enabled,useL1,labelHooks,
        isInGame && isInGame(),ControllerQoL::IsControllerUiActive(),channel);
}
bool ControllerLabelsStale() noexcept {
    if(!ctx || !labelRecoveryReady)return false;
    __try {
        const auto base=ctx->exeBase;
        // Recheck witnesses before calling: another plugin may patch after load.
        for(const auto& sig:LabelRecoverySignatures)
            if(std::memcmp(reinterpret_cast<const void*>(base+sig.rva),sig.bytes,sig.size)) {
                labelRecoveryReady=false;ctx->LogWarn("[QOL/Labels] Recovery contract changed; recovery disabled.");return false;
            }
        if(!reinterpret_cast<bool(__fastcall*)()>(base+0x77e10)())return false;
        auto input=reinterpret_cast<void*(__fastcall*)()>(base+0x13ce90)();
        if(!input || !reinterpret_cast<bool(__fastcall*)(void*)>(base+0x13de60)(input))return false;
        auto render=reinterpret_cast<const unsigned char*(__fastcall*)()>(base+0x144640)();
        return render && Probe::RecoverControllerLabels(true,true,true,ReadFilteredLabelState(),render[0x1f60]);
    } __except(EXCEPTION_EXECUTE_HANDLER) {labelRecoveryReady=false;return false;}
}
bool EnsureLabels() noexcept {
    if(updatingLabels) return false;
    if(!LabelLockActive()) {labelRefreshNeeded=true;return false;}
    int value=ReadFilteredLabelState();
    if(value<0) return false;
    if(value==0 || labelRefreshNeeded) {
        updatingLabels=true;
        labelRefreshNeeded=false;
        reinterpret_cast<LabelSetFn>(ctx->exeBase+0x1fae90)(0,true);
        reinterpret_cast<RefreshLabelsFn>(ctx->exeBase+0xce450)();
        updatingLabels=false;
        ++labelEnforced;
        if(ctx && trace) {
            ctx->LogInfo("[QOL/Labels] Filtered-label state set ON through native setter/refresh. Unfiltered state untouched.");
        }
    }
    return true;
}
void HandleLabelAction(unsigned char channel,bool press) noexcept {
    bool block=false;
    {
        std::lock_guard lock(stateMutex);
        block=LabelLockActive(channel) && EnsureLabels();
        if(block) {
            if(press) ++labelPressBlocked; else ++labelReleaseBlocked;
            if(ctx && trace) {
                ctx->LogInfo(press?"[QOL/Labels] CONSUME filtered-label PRESS; labels remain ON.":
                    "[QOL/Labels] CONSUME filtered-label RELEASE; labels remain ON.");
            }
        }
    }
    auto original=press?originalLabelPress:originalLabelRelease;
    if(!block && original) original(channel);
}
void __fastcall HookLabelPress(unsigned char channel) noexcept {HandleLabelAction(channel,true);}
void __fastcall HookLabelRelease(unsigned char channel) noexcept {HandleLabelAction(channel,false);}
bool CheckLabelSignatures(uintptr_t base) noexcept {
    __try {
        for(const auto& sig:LabelSignatures)
            if(std::memcmp(reinterpret_cast<const void*>(base+sig.rva),sig.bytes,sig.size)) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void InstallLabelHooks() noexcept {
    isInGame=reinterpret_cast<InGameFn>(GetProcAddress(reinterpret_cast<HMODULE>(core),"IsInGame"));
    if(!isInGame || !CheckLabelSignatures(ctx->exeBase)) {
        ctx->LogWarn("[QOL/Labels] Label hooks disabled: binary guards or IsInGame export unavailable; fail open.");return;
    }
    bool press=ctx->InstallInlineHook<LabelActionFn>(0xc66a0,LabelBytes0,19,&HookLabelPress,&originalLabelPress);
    bool release=ctx->InstallInlineHook<LabelActionFn>(0xc6e90,LabelBytes1,20,&HookLabelRelease,&originalLabelRelease);
    labelHooks=press && release;
    labelRecoveryReady=labelHooks;
    for(const auto& sig:LabelRecoverySignatures)
        if(!ctx->CheckExpectedBytes(sig.rva,sig.bytes,sig.size))labelRecoveryReady=false;
    ctx->LogInfo(labelRecoveryReady?"[QOL/Labels] Guarded controller display recovery enabled.":
        "[QOL/Labels] Controller display recovery unavailable; existing label hooks retained.");
    ctx->LogInfo(labelHooks?"[QOL/Labels] Label press/release hooks installed via SDK. Controller mode locks FILTERED labels ON; keyboard label actions pass through.":
        "[QOL/Labels] Label hook installation incomplete; both callbacks pass through.");
}

using MenuMessageFn=void(__fastcall*)(void*,void*);
MenuMessageFn originalMenuMessage=nullptr;
bool menuHook=false,menuRemap=true;
unsigned dedicatedPanels=0,subPanels=0;
bool skillsHook=false;
MenuMessageFn originalSkillsMessage=nullptr;
uint64_t questTranslated=0,questReleased=0,skillsTranslated=0,skillsReleased=0;
uint64_t menuTranslated=0,menuBumpersBlocked=0;
struct MenuCopy {
    alignas(16) unsigned char message[0x120]{};
    alignas(16) unsigned char payload[0x20]{};
};
MenuMessageFn originalBankMessage=nullptr;
bool sharedPageHook=false;
std::atomic<bool> sharedPageInputActive{false};
unsigned ReadBankTab(void* bank) noexcept {
    __try { return bank && ctx ? reinterpret_cast<unsigned char(__fastcall*)(void*)>(ctx->exeBase+0x23AF50)(bank) : 0xFFFFFFFF; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return 0xFFFFFFFF; }
}
Probe::SharedPageGesture sharedPageGesture;
struct BankAction { bool valid{},begin{},repeat{}; unsigned action{},tab{}; };
BankAction DecodeBankAction(void* bank,void* message,MenuCopy& copy) noexcept {
    BankAction result;
    __try {
        if (!bank || !message) return result;
        auto m=static_cast<const unsigned char*>(message);
        if (*reinterpret_cast<const uint64_t*>(m)!=0x4ae067c6248b6042ULL) return result;
        auto command=*reinterpret_cast<const uint64_t*>(m+0x88);
        if (command!=0x1e439768f7dd4594ULL && command!=0x4d833cc71f08b3c0ULL) return result;
        auto payload=*reinterpret_cast<const unsigned char* const*>(m+0x110);
        if (!payload) return result;
        result.begin=command==0x1e439768f7dd4594ULL;
        result.action=*reinterpret_cast<const unsigned*>(payload+0x10);
        result.repeat=payload[0x18]!=0;
        result.tab=reinterpret_cast<unsigned char(__fastcall*)(void*)>(ctx->exeBase+0x23AF50)(bank);
        std::memcpy(copy.message,m,sizeof(copy.message));
        std::memcpy(copy.payload,payload,sizeof(copy.payload));
        *reinterpret_cast<void**>(copy.message+0x110)=copy.payload;
        result.valid=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { result={}; }
    return result;
}
void __fastcall HookBankMessage(void* bank,void* message) noexcept {
    MenuCopy copy;
    int decision=0;
    {
        std::lock_guard lock(stateMutex);
        if (ctx && sharedPageHook) {
            sharedPageInputActive.store(enabled && useL1 && ReadBankTab(bank)==1);
            auto action=DecodeBankAction(bank,message,copy);
            if (action.valid) {
                const auto physical=ControllerQoL::ReadControllerInput();
                decision=sharedPageGesture.Handle(enabled && useL1 && physical.valid,
                    action.tab==1,(physical.buttons&XINPUT_GAMEPAD_LEFT_SHOULDER)!=0,
                    action.action,action.begin,action.repeat);
                static unsigned sharedTraceLines=0;
                if (trace && (action.action==7 || action.action==8) && sharedTraceLines++<64) {
                    char line[192];
                    std::snprintf(line,sizeof(line),"[QOL/Shared] Input action=%u tab=%u begin=%d repeat=%d LB=%d physical=%d decision=%d.",
                        action.action,action.tab,action.begin,action.repeat,(physical.buttons&XINPUT_GAMEPAD_LEFT_SHOULDER)!=0,physical.valid,decision);
                    ctx->LogInfo(line);
                }
                if (decision>0) {
                    *reinterpret_cast<unsigned*>(copy.payload+0x10)=static_cast<unsigned>(decision);
                    copy.payload[0x18]=0;
                    ctx->LogInfo(decision==19?"[QOL/Shared] LB+LT -> native previous Shared page.":
                        "[QOL/Shared] LB+RT -> native next Shared page.");
                }
            }
        }
    }
    // The mapped copy goes through BankPanel and its children synchronously.
    // Children see cycleLeft/right, never the original main-tab trigger.
    if (decision>=0 && originalBankMessage) originalBankMessage(bank,decision>0?copy.message:message);
    {
        std::lock_guard lock(stateMutex);
        sharedPageInputActive.store(ctx && sharedPageHook && enabled && useL1 && ReadBankTab(bank)==1);
    }
}
void InstallSharedPageHook() noexcept {
    bool match=ctx->CheckExpectedBytes(0x23CAA0,SharedPageNative::Handler,sizeof(SharedPageNative::Handler)) &&
        ctx->CheckExpectedBytes(0x23AF50,SharedPageNative::SelectedTab,sizeof(SharedPageNative::SelectedTab));
    if (match) sharedPageHook=ctx->InstallInlineHook<MenuMessageFn>(0x23CAA0,SharedPageNative::Handler,16,&HookBankMessage,&originalBankMessage);
    ctx->LogInfo(sharedPageHook?"[QOL/Shared] Guarded BankPanel remap installed: LB+LT/RT pages; bare bumpers suppressed only on Shared.":
        "[QOL/Shared] Profile/hook unavailable; native Shared input and prompts retained.");
}

// Copies are synchronous borrowed views, never retained or destroyed by this hook.
// Native handler reads action +0x10 and repeat flag +0x18 from the payload.
int PrepareMenuMessage(void* message,MenuCopy& copy,bool subMenu=false) noexcept {
    if(!message) return 0;
    __try {
        auto m=static_cast<const unsigned char*>(message);
        if(*reinterpret_cast<const uint64_t*>(m)!=0x4ae067c6248b6042ULL ||
           *reinterpret_cast<const uint64_t*>(m+0x88)!=0x1e439768f7dd4594ULL) return 0;
        auto p=*reinterpret_cast<const unsigned char* const*>(m+0x110);
        if(!p) return 0;
        unsigned action=*reinterpret_cast<const unsigned*>(p+0x10);
        int mapped=subMenu?Probe::SubMenuAction(true,action):Probe::MenuAction(true,action);
        if(mapped==-1) return -1;
        if(mapped==static_cast<int>(action)) return 0;
        std::memcpy(copy.message,m,sizeof(copy.message));
        std::memcpy(copy.payload,p,sizeof(copy.payload));
        *reinterpret_cast<unsigned*>(copy.payload+0x10)=static_cast<unsigned>(mapped);
        *reinterpret_cast<void**>(copy.message+0x110)=copy.payload;
        return 1;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return 0;}
}
void __fastcall HookMenuMessage(void* panel,void* message) noexcept {
    MenuCopy copy;
    int decision=0;
    {
        std::lock_guard lock(stateMutex);
        if(ctx && enabled && useL1 && menuRemap && dedicatedPanels==0) {
            decision=PrepareMenuMessage(message,copy);
            if(decision<0) ++menuBumpersBlocked;
            if(decision>0) ++menuTranslated;
            if(decision && trace) ctx->LogInfo(decision>0?
                "[QOL/Menu] Trigger translated to menu navigation; shared input untouched.":
                "[QOL/Menu] Bumper menu navigation consumed.");
        }
    }
    if(decision>=0 && originalMenuMessage)
        originalMenuMessage(panel,decision>0?copy.message:message);
}
bool SubMenusEnabled() noexcept {
    return ctx && enabled && useL1 && menuRemap && menuHook && nativeHookInstalled && skillsHook && dedicatedPanels==0;
}
void __fastcall HookSkillsMessage(void* panel,void* message) noexcept {
    MenuCopy copy;
    int decision=0;
    {
        std::lock_guard lock(stateMutex);
        if(SubMenusEnabled() && (subPanels&2)) {
            decision=PrepareMenuMessage(message,copy,true);
            if(decision<0) ++skillsReleased;
            if(decision>0) ++skillsTranslated;
            if(decision && trace) ctx->LogInfo(decision>0?
                "[QOL/Submenu] Skills: bumper translated to skill tab navigation.":
                "[QOL/Submenu] Skills: trigger released to main menu without marking input handled.");
        }
    }
    if(decision>=0 && originalSkillsMessage) originalSkillsMessage(panel,decision>0?copy.message:message);
}
void InstallSkillsHook() noexcept {
    bool match=true;
    for(const auto& sig:SkillsSignatures) match=match && ctx->CheckExpectedBytes(sig.rva,sig.bytes,sig.size);
    if(match) skillsHook=ctx->InstallInlineHook<MenuMessageFn>(0x14c6810,SkillsBytes0,sizeof(SkillsBytes0),&HookSkillsMessage,&originalSkillsMessage);
    ctx->LogInfo(skillsHook?"[QOL/Submenu] Skill Tree hook installed; Quest tab hook already available.":
        "[QOL/Submenu] Skill Tree signature/API mismatch; submenu inversion disabled.");
}
bool CheckMenuSignatures(uintptr_t base) noexcept {
    __try {
        for(const auto& sig:MenuSignatures)
            if(std::memcmp(reinterpret_cast<const void*>(base+sig.rva),sig.bytes,sig.size)) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool CheckMenuRoute(uintptr_t game,uintptr_t provider) noexcept {
    __try {
        using namespace QolMenuRoute;
        return ValidLinks(game,provider,*reinterpret_cast<const uintptr_t*>(game+Slot),
            reinterpret_cast<const unsigned char*>(game+Thunk),
            *reinterpret_cast<const uintptr_t*>(game+Import),
            *reinterpret_cast<const uintptr_t*>(provider+0x70ea68)) &&
            std::memcmp(reinterpret_cast<const void*>(provider+Dispatcher),DispatcherBytes,sizeof(DispatcherBytes))==0 &&
            std::memcmp(reinterpret_cast<const void*>(provider+0x463cc0),RouteBytes,sizeof(RouteBytes))==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void InstallMenuHook() noexcept {
    // The loader's registered-route handler precedes the native menu handler.
    // Intercept this one virtual dispatch, then forward to the unchanged loader
    // thunk so translated triggers cycle through registered plugin tabs too.
    auto provider=GetModuleHandleW(L"D2RCore.dll");
    if(QolCore::VerifyFileHash(provider,QolNativeProfile::CoreHash) &&
        CheckMenuSignatures(ctx->exeBase) && CheckMenuRoute(ctx->exeBase,reinterpret_cast<uintptr_t>(provider))) {
        HMODULE pinned{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&HookMenuMessage),&pinned)) return;
        const uintptr_t expected=ctx->exeBase+QolMenuRoute::Thunk;
        const auto replacement=&HookMenuMessage;
        originalMenuMessage=reinterpret_cast<MenuMessageFn>(expected);
        menuHook=ctx->PatchBytes(QolMenuRoute::Slot,&expected,sizeof(expected),&replacement,sizeof(replacement));
        ctx->LogInfo(menuHook?
            "[QOL/Menu] Route-aware UISwitcher dispatch installed at game+0x1CEF450; loader controller routes preserved.":
            "[QOL/Menu] Route-aware dispatch declined; original navigation retained.");
        // Do not fall back after a publication attempt. A retained/pinned wrapper
        // is safe on SDK cleanup; Cleanup clears ctx and makes it passthrough.
        return;
    }
    if(CheckMenuSignatures(ctx->exeBase))
        menuHook=ctx->InstallInlineHook<MenuMessageFn>(0x27df80,MenuBytes0,sizeof(MenuBytes0),&HookMenuMessage,&originalMenuMessage);
    ctx->LogInfo(menuHook?"[QOL/Menu] UISwitcher native remap installed: triggers navigate, bumpers pass to other systems.":
        "[QOL/Menu] UISwitcher remap unavailable; signature/API mismatch, original behavior retained.");
}

std::string selectedTarget="BankPanelMessage", selectedCommand="SelectTab";
uint64_t messages=0, matched=0, consumed=0, samples=0, faults=0, traceLines=0;
uint64_t controllerMessages=0, relevantMessages=0, l2Samples=0;
using Lookup=unsigned char*(__fastcall*)(void*,const char*);


struct Raw { bool valid=false, l2=false, l1=false, held=false; int previous=-1,current=-1; unsigned lt=0; bool fault=false; };

// No patching or writes to game memory. Offsets are enabled only for the exact
// inspected D2RCore file and matching live query instructions.

Raw ReadNative() noexcept {
    Raw result;
    if(!nativeVerified) return result;
    __try {
        // The inspected instructions perform TWO dereferences to reach manager.
        void** managerSlot=*reinterpret_cast<void***>(core+0x67F4D0);
        auto lookup=*reinterpret_cast<Lookup*>(core+0x680D28);
        if(!managerSlot || !*managerSlot || !lookup) return result;
        MEMORY_BASIC_INFORMATION memory{};
        if(!VirtualQuery(reinterpret_cast<void*>(lookup),&memory,sizeof(memory)) ||
           memory.State!=MEM_COMMIT || (memory.Protect & (PAGE_GUARD|PAGE_NOACCESS)) ||
           !(memory.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return result;
        auto action=lookup(*managerSlot,"ControllerAltHold");
        if(!action) return result;
        result.previous=action[0x50]; result.current=action[0x51];
        result.valid=result.previous<=1 && result.current<=1;
        result.l2=result.valid && Probe::OwnGesture(result.previous,result.current);
    } __except(EXCEPTION_EXECUTE_HANDLER) { result={}; result.fault=true; }
    return result;
}

Raw ReadRaw() noexcept {
    Raw r=useL1 ? Raw{} : ReadNative();
    if(r.fault) ++faults;
    if(!r.valid) {
        auto physical=ControllerQoL::ReadControllerInput();
        r.valid=physical.valid;
        r.l1=(physical.buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)!=0;
        r.lt=physical.leftTrigger;
    }
    r.l2=r.l2 || r.lt>30;
    r.held=Probe::ModifierHeld(useL1,r.l1,r.l2);
    ++samples;
    if(r.held) ++l2Samples;
    return r;
}

void LogRaw(const Raw& r) noexcept {
    if (!trace) return;
    static int last=-1;
    int key=(r.valid?4:0)+(r.l2?2:0)+(r.fault?1:0)+(r.l1?8:0)+(useL1?16:0);
    if(key==last) return;
    last=key;
    char line[256];
    std::snprintf(line,sizeof(line),"[QOL/Input] RAW modifier=%s held=%d L1=%d valid=%d L2-gesture=%d native(prev=%d,current=%d) xinputLT=%u faults=%llu",
        useL1?"L1":"L2",r.held,r.l1,r.valid,r.l2,r.previous,r.current,r.lt,static_cast<unsigned long long>(faults));
    ctx->LogInfo(line);
}

struct NativeMessage {
    bool controllerBegin=false, switchEnabled=false, fault=false;
    unsigned action=0,left=0,right=0;
};
NativeMessage DecodeNative(void* widget,void* message) noexcept {
    NativeMessage n;
    if(!widget || !message) return n;
    __try {
        auto m=static_cast<const unsigned char*>(message);
        auto w=static_cast<const unsigned char*>(widget);
        if(*reinterpret_cast<const uint64_t*>(m)!=0x4ae067c6248b6042ULL ||
           *reinterpret_cast<const uint64_t*>(m+0x88)!=0x1e439768f7dd4594ULL) return n;
        auto payload=*reinterpret_cast<const unsigned char* const*>(m+0x110);
        if(!payload) return n;
        n.action=*reinterpret_cast<const unsigned*>(payload+0x10);
        n.left=*reinterpret_cast<const unsigned*>(w+0x16a4);
        n.right=*reinterpret_cast<const unsigned*>(w+0x16a8);
        n.switchEnabled=w[0x16a0]!=0;
        n.controllerBegin=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {n={};n.fault=true;}
    return n;
}
// Same bounded widget-name/parent layout used by the existing scoped glyph hook.
bool IsScopedSubTab(void* widget,unsigned kind=0) noexcept {
    __try {
        const auto* w=static_cast<const unsigned char*>(widget);
        if(!w)return false;
        const auto* parent=*reinterpret_cast<const unsigned char* const*>(w+0x30);
        if(!parent)return false;
        const auto* name=*reinterpret_cast<const char* const*>(w+8);
        const auto* owner=*reinterpret_cast<const char* const*>(parent+8);
        if(!name || !owner)return false;
        size_t n=0,p=0;
        while(n<64 && name[n])++n;
        while(p<64 && owner[p])++p;
        const bool visible=w[0x50] && w[0x51] && parent[0x50] && parent[0x51];
        return n<64 && p<64 && (kind==2 ? Probe::LootFilterTab({name,n},{owner,p},visible) : kind==1 ? Probe::ChronicleTab({name,n},{owner,p},visible) :
            Probe::OptionsTab({name,n},{owner,p},visible));
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
// Borrowed synchronous message routed through the existing provider wrapper.
// No focus-pointer writes, new inline hooks, or global direction remaps.
bool filterFocusAdmitted=false;
unsigned FilterSelection(void* widget) noexcept {
    __try {
        auto w=static_cast<unsigned char*>(widget);
        if(!w)return ~0u;
        auto parent=*reinterpret_cast<unsigned char**>(w+0x30);
        if(!parent || *reinterpret_cast<uintptr_t*>(parent)!=core+0x7b13d0)return ~0u;
        return *reinterpret_cast<unsigned*>(parent+0x3fc);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return ~0u;}
}
bool EnterFilterItems(void* widget,void* message,unsigned action,bool recover=false) noexcept {
    if(!filterFocusAdmitted || !Probe::EnterFilterItems(true,true,action))return false;
    __try {
        const auto base=ctx->exeBase;
        using namespace QolFilterFocus;
        if(std::memcmp(reinterpret_cast<void*>(base+0x846020),FocusGetter,sizeof(FocusGetter)) ||
           std::memcmp(reinterpret_cast<void*>(base+0x14af750),RuleInput,sizeof(RuleInput)) ||
           std::memcmp(reinterpret_cast<void*>(base+0x14af520),Resolver,sizeof(Resolver)) ||
           std::memcmp(reinterpret_cast<void*>(core+0x2c1840),Wrapper,sizeof(Wrapper))) {
            filterFocusAdmitted=false;
            ctx->LogWarn("[QOL/Filter] Focus contract changed; native input preserved.");return false;
        }
        auto w=static_cast<unsigned char*>(widget);
        auto parent=*reinterpret_cast<unsigned char**>(w+0x30);
        auto manager=*reinterpret_cast<unsigned char**>(base+0x3440170);
        if(!manager || !parent)return false;
        auto focus=*reinterpret_cast<unsigned char**>(manager+0xd0);
        if(!focus)return false;
        if(recover) {
            if(FilterSelection(widget)>1 || *reinterpret_cast<void**>(focus+0x190))return false;
        } else if(*reinterpret_cast<void**>(focus+0x188)!=parent ||
                  *reinterpret_cast<void**>(focus+0x190)!=widget)return false;
        if(*reinterpret_cast<uintptr_t*>(parent)!=core+0x7b13d0 ||
           *reinterpret_cast<uintptr_t*>(core+0x7b13f0)!=core+0x2c1840 ||
           *reinterpret_cast<uintptr_t*>(core+0x7b1350)!=base+0x14aff80)return false;
        bool moved=false;
        if(!recover) {
        auto m=static_cast<unsigned char*>(message);
        auto payload=*reinterpret_cast<unsigned char**>(m+0x110);
        if(!payload)return false;
        MenuCopy copy;
        std::memcpy(copy.message,m,sizeof(copy.message));
        std::memcpy(copy.payload,payload,sizeof(copy.payload));
        *reinterpret_cast<unsigned*>(copy.payload+0x10)=22;
        *reinterpret_cast<void**>(copy.message+0x110)=copy.payload;
        reinterpret_cast<TabMessageFn>(core+0x2c1840)(parent,copy.message);
        moved=*reinterpret_cast<void**>(focus+0x188)!=parent ||
            *reinterpret_cast<void**>(focus+0x190)!=widget;
        }
        // The loader merges Equipment/Misc into one panel. The native section
        // resolver still names the two absent panels; use its registered pair.
        if(!moved &&
           !std::memcmp(reinterpret_cast<void*>(base+0x14abc60),EnterPanel,sizeof(EnterPanel)) &&
           !std::memcmp(reinterpret_cast<void*>(base+0x14aa6d0),EnterList,sizeof(EnterList)) &&
           !std::memcmp(reinterpret_cast<void*>(core+0x2c0f00),ItemWrapper,sizeof(ItemWrapper))) {
            for(unsigned i=0;i<4;++i) {
                auto slot=core+0x7b0ed0+i*24;
                auto panel=*reinterpret_cast<unsigned char**>(slot);
                auto helper=*reinterpret_cast<unsigned char**>(slot+8);
                if(!panel || !helper || !panel[0x50] || !panel[0x51])continue;
                if(recover && *reinterpret_cast<void**>(focus+0x188)!=panel)continue;
                if(*reinterpret_cast<uintptr_t*>(panel)!=core+0x7b10b0 ||
                   *reinterpret_cast<uintptr_t*>(helper)!=base+0x1fd6098 ||
                   *reinterpret_cast<void**>(panel+0x168)!=parent ||
                   *reinterpret_cast<void**>(helper+0x168)!=parent)continue;
                const auto model=*reinterpret_cast<void**>(parent+0x168);
                if(!model || *reinterpret_cast<void**>(panel+0x170)!=model ||
                   *reinterpret_cast<void**>(helper+0x170)!=model)continue;
                auto list=*reinterpret_cast<unsigned char**>(panel+0x178);
                if(!list || *reinterpret_cast<void**>(helper+0x178)!=list ||
                   *reinterpret_cast<uintptr_t*>(list)!=core+0x7b1240 ||
                   *reinterpret_cast<void**>(list+0x30)!=panel ||
                   !list[0x50] || !list[0x51] || !*reinterpret_cast<void**>(list+0x320))continue;
                if(*reinterpret_cast<uintptr_t*>(core+0x7b10d0)!=core+0x2c0f00)continue;
                reinterpret_cast<void(__fastcall*)(void*)>(base+0x14abc60)(panel);
                moved=recover?(*reinterpret_cast<void**>(focus+0x188)==panel && *reinterpret_cast<void**>(focus+0x190)!=nullptr):
                    (*reinterpret_cast<void**>(focus+0x188)!=parent || *reinterpret_cast<void**>(focus+0x190)!=widget);
                static unsigned entrySamples=0;
                if(entrySamples++<8)ctx->LogInfo(moved?
                    "[QOL/Filter] Merged item-panel native entry changed focus.":
                    "[QOL/Filter] Merged item-panel native entry left focus unchanged.");
                break;
            }
        }
        static unsigned samples=0;
        if(samples++<8)ctx->LogInfo(moved?
            "[QOL/Filter] Down native handoff changed focus.":
            "[QOL/Filter] Native section/merged-panel handoff did not change focus.");
        return moved;
    } __except(EXCEPTION_EXECUTE_HANDLER) {filterFocusAdmitted=false;return false;}
}
// Tab updates can invalidate the former focus after the tab handler returns.
// Reacquire through the current registry on later SDK UI updates; never call a
// cached widget pointer. Short-lived repair applies only to the same rule/panel.
const D2RL::ThreadService* filterUiThreads=nullptr;
uint64_t filterRepairGeneration=0;
struct FilterRepair {uintptr_t parent{},panel{},model{};unsigned before{},remaining{};ULONGLONG started{};} filterRepair;
bool CaptureFilterRepair(void* widget,unsigned before) noexcept {
    if(!filterFocusAdmitted || before>1 || !filterUiThreads)return false;
    __try {
        auto parent=*reinterpret_cast<unsigned char**>(static_cast<unsigned char*>(widget)+0x30);
        auto manager=*reinterpret_cast<unsigned char**>(ctx->exeBase+0x3440170);
        if(!parent || !manager)return false;
        auto focus=*reinterpret_cast<unsigned char**>(manager+0xd0);
        if(!focus)return false;
        for(unsigned i=0;i<4;++i) {
            auto panel=*reinterpret_cast<unsigned char**>(core+0x7b0ed0+i*24);
            if(!panel || *reinterpret_cast<uintptr_t*>(panel)!=core+0x7b10b0 ||
               *reinterpret_cast<void**>(panel+0x168)!=parent || *reinterpret_cast<void**>(focus+0x188)!=panel)continue;
            filterRepair={reinterpret_cast<uintptr_t>(parent),reinterpret_cast<uintptr_t>(panel),
                *reinterpret_cast<uintptr_t*>(parent+0x168),before,8,GetTickCount64()};
            ++filterRepairGeneration;return true;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
// -1 cancel, 0 await destination, 1 destination active. Output is freshly resolved.
int ResolveFilterRepair(void*& widget) noexcept {
    __try {
        for(unsigned i=0;i<4;++i) {
            auto panel=*reinterpret_cast<unsigned char**>(core+0x7b0ed0+i*24);
            if(reinterpret_cast<uintptr_t>(panel)!=filterRepair.panel)continue;
            if(!panel || *reinterpret_cast<uintptr_t*>(panel)!=core+0x7b10b0 || !panel[0x50] || !panel[0x51])return -1;
            auto parent=*reinterpret_cast<unsigned char**>(panel+0x168);
            if(reinterpret_cast<uintptr_t>(parent)!=filterRepair.parent || !parent ||
               *reinterpret_cast<uintptr_t*>(parent)!=core+0x7b13d0 ||
               *reinterpret_cast<uintptr_t*>(parent+0x168)!=filterRepair.model)return -1;
            auto manager=*reinterpret_cast<unsigned char**>(ctx->exeBase+0x3440170);
            if(!manager)return -1;
            auto focus=*reinterpret_cast<unsigned char**>(manager+0xd0);
            if(!focus || *reinterpret_cast<void**>(focus+0x188)!=panel)return -1;
            widget=*reinterpret_cast<void**>(parent+0x160);
            if(!IsScopedSubTab(widget,2))return -1;
            return Probe::RecoverFilterTabFocus(filterRepair.before,FilterSelection(widget),true)?1:0;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return -1;
}
void __cdecl RepairFilterFocus(const D2RL::PluginContext* owner,void* token) noexcept {
    std::lock_guard lock(stateMutex);
    if(!ctx || owner!=ctx || reinterpret_cast<uintptr_t>(token)!=filterRepairGeneration || !filterRepair.remaining)return;
    if(!SubMenusEnabled() || !ControllerQoL::IsControllerUiActive() || GetTickCount64()-filterRepair.started>250) {filterRepair.remaining=0;return;}
    --filterRepair.remaining;
    void* widget=nullptr;
    const int state=ResolveFilterRepair(widget);
    if(state<0){filterRepair.remaining=0;return;}
    if(state>0 && EnterFilterItems(widget,nullptr,13,true))ctx->LogInfo("[QOL/Filter] Deferred tab-change focus recovered through native list entry.");
    if(filterRepair.remaining && filterUiThreads->runOnUiThread(ctx,RepairFilterFocus,token)!=D2RL::Threads::Result::Success)filterRepair.remaining=0;
}
void __fastcall HookTabMessage(void* widget,void* message) noexcept {
    // Preserve the original function's void ABI. Do not mutate input, widget
    // selection, message payload, or raw state used by controller-qol.
    bool block=false;
    MenuCopy subCopy;
    int subDecision=0;
    bool repairFilter=false;
    uint64_t repairGeneration=0;
    {
        std::lock_guard lock(stateMutex);
        ++nativeCalls;
        if(ctx) {
            auto n=DecodeNative(widget,message);
            if(n.fault) ++faults;
            if(n.controllerBegin) {
                ++nativeControllerCalls;
                const bool options=(subPanels&4) && IsScopedSubTab(widget);
                const bool chronicle=(subPanels&8) && IsScopedSubTab(widget,1);
                const bool lootFilter=(subPanels&16) && IsScopedSubTab(widget,2);
                if(lootFilter) {
                    static unsigned filterInputSamples=0;
                    if(filterInputSamples++<32) {
                        char line[128];std::snprintf(line,sizeof(line),"[QOL/Filter] Controller tab input action=%u bindings=%u/%u switch=%d",n.action,n.left,n.right,n.switchEnabled);
                        ctx->LogInfo(line);
                    }
                }
                if(SubMenusEnabled() && ((subPanels&1) || options || chronicle || lootFilter) && n.switchEnabled && n.left==7 && n.right==8) {
                    subDecision=PrepareMenuMessage(message,subCopy,true);
                    if(lootFilter && subDecision>0) {
                        repairFilter=CaptureFilterRepair(widget,FilterSelection(widget));
                        repairGeneration=filterRepairGeneration;
                    }
                    if(subDecision<0) ++questReleased;
                    if(subDecision>0) ++questTranslated;
                    if(subDecision && trace && options) ctx->LogInfo(subDecision>0?
                        "[QOL/Submenu] Options: bumper translated to settings tab navigation.":
                        "[QOL/Submenu] Options: trigger released to main menu.");
                    if(subDecision && trace && chronicle) ctx->LogInfo(subDecision>0?
                        "[QOL/Submenu] Chronicle: bumper translated to inner tab navigation.":
                        "[QOL/Submenu] Chronicle: trigger released to main menu.");
                    if(subDecision && trace && lootFilter) ctx->LogInfo("[QOL/Submenu] Loot filter Equipment/Items tabs: scoped bumper remap.");
                    if(subDecision && trace && !options && !chronicle && !lootFilter) ctx->LogInfo(subDecision>0?
                        "[QOL/Submenu] Quest: bumper translated to act tab navigation.":
                        "[QOL/Submenu] Quest: trigger released to main menu without marking input handled.");
                }
                if(lootFilter && SubMenusEnabled() && ControllerQoL::IsControllerUiActive() &&
                   EnterFilterItems(widget,message,n.action))return;
                auto r=ReadRaw();
                block=Probe::ConsumeTabLeft(enabled && nativeMode && !useL1,r.valid && r.held,
                    n.controllerBegin,n.switchEnabled,n.action,n.left,n.right);
                if(block) ++nativeBlocked;
                if(block || (trace && nativeTraceLines<128)) {
                    ++nativeTraceLines;
                    char line[320];
                    std::snprintf(line,sizeof(line),"[QOL/Input] NATIVE tab=%p action=%u left=%u right=%u tabSwitch=%d modifier=%s held=%d L1=%d L2=%d decision=%s",
                        widget,n.action,n.left,n.right,n.switchEnabled,useL1?"L1":"L2",r.held,r.l1,r.l2,block?"CONSUME":"CONTINUE");
                    ctx->LogInfo(line);
                }
            }
        }
    }
    if(!block && subDecision>=0 && originalTabMessage) {
        originalTabMessage(widget,subDecision>0?subCopy.message:message);
        std::lock_guard lock(stateMutex);
        if(ctx && repairFilter && repairGeneration==filterRepairGeneration && filterUiThreads &&
           filterUiThreads->runOnUiThread(ctx,RepairFilterFocus,reinterpret_cast<void*>(repairGeneration))!=D2RL::Threads::Result::Success)
            filterRepair.remaining=0;
    }
}
bool CheckNativeSignatures(uintptr_t base) noexcept {
    if(!base) return false;
    __try {
        for(const auto& sig:NativeSignatures)
            if(std::memcmp(reinterpret_cast<const void*>(base+sig.rva),sig.bytes,sig.size)!=0) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void InstallNativeHook() noexcept {
    if(!CheckNativeSignatures(ctx->exeBase)) {
        ctx->LogWarn("[QOL/Input] Native hook disabled: D2R TabBar signatures mismatch. Native mode fails open; no late SelectTab fallback.");
        return;
    }
    nativeHookInstalled=ctx->InstallInlineHook<TabMessageFn>(0x878d30,NativeSignatures[0].bytes,
        NativeSignatures[0].size,&HookTabMessage,&originalTabMessage);
    ctx->LogInfo(nativeHookInstalled ?
        "[QOL/Input] Native TabBar hook installed through D2RLoader SDK at RVA 0x878d30. Left navigation is filtered BEFORE selection mutation.":
        "[QOL/Input] D2RLoader declined native TabBar hook. Native mode passes through; inspect loader diagnostics.");
}

auto OnMessage(const D2RL::PluginContext*,const D2RL::SharedEvents::UiMessageEvent* event,void*) noexcept
 -> D2RL::SharedEvents::UiMessageAction {
    std::lock_guard lock(stateMutex);
    using A=D2RL::SharedEvents::UiMessageAction;
    if(!ctx || !event || event->structSize<D2RL::SharedEvents::UiMessageEventRequiredSize) return A::Continue;
    ++messages;
    EnsureLabels(); // Shared UI callback runs on the game thread; never enforce from load/console threads.
    Raw raw=ReadRaw(); LogRaw(raw);
    std::string_view target=event->target?event->target:"", command=event->command?event->command:"";
    if(target=="PanelManager" && event->text) {
        const std::string_view name=event->text;
        const unsigned bit=Probe::DedicatedPanelBit(name);
        const unsigned subBit=Probe::SubPanelBit(name);
        if(command=="OpenPanel" || command=="OpenExclusivePanel") subPanels|=subBit;
        if(command=="ClosePanel" || command=="UnloadPanel") subPanels&=~subBit;
        if(command=="OpenPanel" || command=="OpenExclusivePanel") dedicatedPanels|=bit;
        if(command=="ClosePanel" || command=="UnloadPanel") dedicatedPanels&=~bit;
        if(name=="HUDPanel" && (command=="ClosePanel" || command=="UnloadPanel")) {dedicatedPanels=0;subPanels=0;sharedPageGesture={};sharedPageInputActive.store(false);}
        if(bit==1 && (command=="ClosePanel" || command=="UnloadPanel")) {sharedPageGesture={};sharedPageInputActive.store(false);}
    }
    bool relevant=Probe::ShouldTrace(target,command);
    if(relevant) ++relevantMessages;
    if(Probe::IsControllerAction(target,command)) ++controllerMessages;
    bool match=screenMode ? Probe::IsScreenTransition(target,command) :
        Probe::ShouldConsume(true,true,target,command,selectedTarget,selectedCommand);
    if(match) ++matched;
    bool block=!useL1 && !nativeMode && enabled && raw.valid && raw.held && match;
    if(block) ++consumed;
    // Per-frame HUD traffic used the entire v0.1 trace budget before testing.
    // Only navigation/controller routes consume this budget. Consumption itself
    // is always logged, even after the diagnostic trace budget is exhausted.
    if(block || (trace && (relevant || match) && traceLines<2000)) {
        ++traceLines;
        char payload[161]{};
        // Log payload only for navigation/controller events, never keyboard,
        // chat, console input or unrelated UI messages. Escape control chars.
        if(relevant && event->text) for(size_t i=0;i<160 && event->text[i];++i) {
            const unsigned char c=static_cast<unsigned char>(event->text[i]);
            payload[i]=c>=32 && c!=127?static_cast<char>(c):'?';
        }
        char line[896];
        std::snprintf(line,sizeof(line),"[QOL/Input] UI target=\"%.160s\" command=\"%.160s\" text=\"%s\" L1=%d held=%d L2=%d rawValid=%d decision=%s hashes=%llx:%llx",
            event->target?event->target:"",event->command?event->command:"",payload,raw.l1,raw.held,raw.l2,raw.valid,block?"CONSUME":"CONTINUE",
            static_cast<unsigned long long>(event->targetHash),static_cast<unsigned long long>(event->commandHash));
        ctx->LogInfo(line);
        if(traceLines==2000) ctx->LogWarn("[QOL/Input] Trace capped at 2000 lines. qol trace resets this budget.");
    }
    return block?A::Consume:A::Continue;
}
void OnTooltip(const D2RL::PluginContext*,D2RL::SharedEvents::ItemTooltipEvent*,void*) noexcept {
    std::lock_guard lock(stateMutex);
    if(ctx) LogRaw(ReadRaw()); // Read-only; contribute no tooltip and consume no item action.
}

auto Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext* cmd,void*) noexcept -> D2RL::ConsoleCommandResult {
    std::lock_guard lock(stateMutex);
    if(!ctx || !cmd) return D2RL::ConsoleCommandResult::Failed;
    try {
        std::string arg(cmd->args?cmd->args:"",cmd->args?cmd->argsLength:0);
        auto first=arg.find_first_not_of(" \t\r\n"); arg=first==std::string::npos?"":arg.substr(first);
        auto last=arg.find_last_not_of(" \t\r\n"); if(last!=std::string::npos) arg.resize(last+1);
        if(arg=="input on") {enabled=true;labelRefreshNeeded=true;}
        else if(arg=="input off") {enabled=false;labelRefreshNeeded=true;}
        else if(arg=="menu on") menuRemap=true;
        else if(arg=="menu off") menuRemap=false;
        else if(arg=="trace") {trace=true;traceLines=0;nativeTraceLines=0;}
        else if(arg=="quiet") trace=false;
        else if(!arg.empty() && arg!="status") {
            ctx->WriteConsoleMessage("qol status|trace|quiet|menu on|menu off|input on|input off");
            return D2RL::ConsoleCommandResult::InvalidArguments;
        }
        QolNativeRanges::Enable(enabled && useL1 && menuRemap && menuHook);
        auto r=ReadRaw();LogRaw(r);
        char line[896];
        std::snprintf(line,sizeof(line),"[QOL/Input] v1.3.1+rev.4 %s mode=%s route=%s:%s coreVerified=%d nativeValid=%d rawValid=%d modifier=%s held=%d L1=%d L2=%d UI=%llu matched=%llu consumed=%llu samples=%llu faults=%llu controllerEvents=%llu relevant=%llu modifierSamples=%llu nativeHook=%d nativeCalls=%llu nativeController=%llu nativeBlocked=%llu",
            enabled?"ON":"OFF",nativeMode?"native":(screenMode?"screens":"route"),selectedTarget.c_str(),selectedCommand.c_str(),nativeVerified,r.previous>=0 && r.previous<=1 && r.current>=0 && r.current<=1,r.valid,useL1?"L1":"L2",r.held,r.l1,r.l2,
            static_cast<unsigned long long>(messages),static_cast<unsigned long long>(matched),static_cast<unsigned long long>(consumed),
            static_cast<unsigned long long>(samples),static_cast<unsigned long long>(faults),
            static_cast<unsigned long long>(controllerMessages),static_cast<unsigned long long>(relevantMessages),static_cast<unsigned long long>(l2Samples),
            nativeHookInstalled,static_cast<unsigned long long>(nativeCalls),static_cast<unsigned long long>(nativeControllerCalls),static_cast<unsigned long long>(nativeBlocked));
        ctx->WriteConsoleMessage(line);ctx->LogInfo(line);
        std::snprintf(line,sizeof(line),"[QOL/Labels] labelHooks=%d pressBlocked=%llu releaseBlocked=%llu enforced=%llu labels=%d",
            labelHooks,static_cast<unsigned long long>(labelPressBlocked),static_cast<unsigned long long>(labelReleaseBlocked),
            static_cast<unsigned long long>(labelEnforced),ctx && labelHooks?ReadFilteredLabelState():-1);
        ctx->WriteConsoleMessage(line);ctx->LogInfo(line);
        std::snprintf(line,sizeof(line),"[QOL/Menu] menuHook=%d remap=%d dedicatedPanels=%u translated=%llu bumpersBlocked=%llu",
            menuHook,enabled && useL1 && menuRemap,dedicatedPanels,static_cast<unsigned long long>(menuTranslated),static_cast<unsigned long long>(menuBumpersBlocked));
        ctx->WriteConsoleMessage(line);ctx->LogInfo(line);
        std::snprintf(line,sizeof(line),"[QOL/Submenu] skillsHook=%d subPanels=%u questTranslated=%llu questReleased=%llu skillsTranslated=%llu skillsReleased=%llu",
            skillsHook,subPanels,static_cast<unsigned long long>(questTranslated),static_cast<unsigned long long>(questReleased),
            static_cast<unsigned long long>(skillsTranslated),static_cast<unsigned long long>(skillsReleased));
        ctx->WriteConsoleMessage(line);ctx->LogInfo(line);
        QolGlyphs::Status();
        return D2RL::ConsoleCommandResult::Handled;
    } catch(...) {return D2RL::ConsoleCommandResult::Failed;}
}
void Cleanup() noexcept {
    ++filterRepairGeneration;filterRepair.remaining=0;filterUiThreads=nullptr;
    QolNativeRanges::Enable(false);
    sharedPageInputActive.store(false);
    QolGlyphs::Shutdown();
    if(ctx && events) {
        if(uiHandle && events->unregisterUiMessageListener) events->unregisterUiMessageListener(ctx,uiHandle);
        if(tooltipHandle && events->unregisterItemTooltipListener) events->unregisterItemTooltipListener(ctx,tooltipHandle);
    }
    uiHandle=tooltipHandle=0;
    labelRecoveryReady=false;
    ctx=nullptr;events=nullptr;
}
}
bool QolNavigation::Initialize(const D2RL::PluginContext* context,bool featureEnabled,bool traceEnabled) noexcept {
    if(!context) return false;
    ctx=context;
    enabled=featureEnabled;
    trace=traceEnabled;
    if(ctx->QueryService(&events)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasSharedEventServiceField(events,D2RL::SharedEventServiceRequiredSize) ||
       !events->registerUiMessageListener || !events->registerItemTooltipListener) {
        ctx=nullptr;events=nullptr;
        return false;
    }
    auto module=GetModuleHandleW(L"D2RCore.dll");core=reinterpret_cast<uintptr_t>(module);
    nativeVerified=module && QolCore::VerifyCore(module);
    filterFocusAdmitted=module && QolCore::VerifyFileHash(module,QolNativeProfile::CoreHash);
    if(ctx->QueryService(&filterUiThreads)!=D2RL::ServiceQueryResult::Success ||
       !D2RL::HasThreadServiceField(filterUiThreads,D2RL::ThreadServiceRequiredSize) || !filterUiThreads->runOnUiThread)filterUiThreads=nullptr;
    D2RL::SharedEvents::UiMessageListener listener{
        .structSize=D2RL::SharedEvents::UiMessageListenerSize,.priority=1000,.callback=OnMessage};
    if(events->registerUiMessageListener(ctx,&listener,&uiHandle)!=D2RL::SharedEvents::Result::Success) {Cleanup();return false;}
    D2RL::SharedEvents::ItemTooltipListener tooltip{
        .structSize=D2RL::SharedEvents::ItemTooltipListenerSize,.callback=OnTooltip};
    if(events->registerItemTooltipListener(ctx,&tooltip,&tooltipHandle)!=D2RL::SharedEvents::Result::Success)
        ctx->LogWarn("[QOL/Input] Tooltip sampling unavailable; UI/console sampling remains enabled.");
    if(!ctx->RegisterConsoleCommand("qol",Command,"QOL: status, trace, quiet, menu on/off, input on/off")) {Cleanup();return false;}
    InstallNativeHook();
    InstallLabelHooks();
    InstallMenuHook();
    InstallSkillsHook();
    InstallSharedPageHook();
    QolNativeRanges::Install(ctx);
    QolNativeRanges::Enable(enabled && useL1 && menuRemap && menuHook);
    // These hooks touch guarded D2R.exe code, not private D2RCore addresses.
    QolGlyphs::Initialize(ctx,menuHook && nativeHookInstalled && skillsHook);
    ctx->LogInfo("[QOL/Input] v1.3.1+rev.4 loaded. Native hook uses D2RLoader SDK only; no MinHook. Item actions are integrated in this QOL DLL.");
    ctx->LogInfo(useL1?"[QOL/Labels] L1 mode: filtered labels always ON, label press/release suppressed, main pages use triggers; Quest/Skill sub-tabs use bumpers.":nativeMode?"[QOL/Input] NATIVE mode: only the tab widget's configured left action is consumed while the selected modifier is active. Late SelectTab filtering is OFF.":"[QOL/Input] Legacy UI filtering selected. Production mode uses L1.");
    ctx->LogInfo(nativeVerified?"[QOL/Input] Exact D2RCore hash matched; native raw-state reader enabled.":"[QOL/Input] Historical action-name bridge disabled for this core; navigation uses the selected controller input provider.");
    return true;
}
void QolNavigation::SetTrace(bool traceEnabled) noexcept {
    std::lock_guard lock(stateMutex);
    trace = traceEnabled;
}
void QolNavigation::Shutdown() noexcept {
    std::lock_guard lock(stateMutex);
    Cleanup();
}

// Called only by the item module's game-thread tasks. Retain the existing
// binary guards and filtered-label policy; never toggle the unfiltered channel.
void QolNavigation::RefreshGroundLabels() noexcept {
    std::lock_guard lock(stateMutex);
    if (!LabelLockActive() || updatingLabels) return;
    labelRefreshNeeded=true;
    EnsureLabels();
}
void QolNavigation::GetGlyphModes(bool& primary,bool& secondary) noexcept {
    std::lock_guard lock(stateMutex);
    primary=ctx && enabled && useL1 && menuRemap && menuHook && dedicatedPanels==0;
    secondary=SubMenusEnabled();
}

bool QolNavigation::SharedPageRemapEnabled() noexcept {
    std::lock_guard lock(stateMutex);
    return ctx && enabled && useL1 && sharedPageHook;
}

bool QolNavigation::SharedPageInputActive() noexcept { return sharedPageInputActive.load(); }

bool QolNavigation::OptionsRemapEnabled() noexcept {
    std::lock_guard lock(stateMutex);
    return SubMenusEnabled() && (subPanels&4);
}

bool QolNavigation::GroundShortcutsAllowed() noexcept {
    std::lock_guard lock(stateMutex);
    return ctx && enabled && Probe::GroundShortcutsAllowed(dedicatedPanels,subPanels);
}

bool QolNavigation::ChronicleRemapEnabled() noexcept {
    std::lock_guard lock(stateMutex);
    return SubMenusEnabled() && (subPanels&8);
}

void QolNavigation::PumpLabels(const D2RL::PluginContext* context,const D2RL::ThreadService* threads) noexcept {
    std::lock_guard lock(stateMutex);
    if(!ctx || ctx!=context || !threads || !threads->runOnGameThread || !labelRecoveryReady || !enabled || !useL1 || labelRecoveryQueued)return;
    const auto now=GetTickCount64();
    if(now-lastLabelCheck<250)return;
    lastLabelCheck=now;labelRecoveryQueued=true;
    if(threads->runOnGameThread(context,[](const D2RL::PluginContext* owner,void*) noexcept {
        std::lock_guard taskLock(stateMutex);
        labelRecoveryQueued=false;
        if(!ctx || ctx!=owner || updatingLabels)return;
        // Re-entering controller mode must restore its labels even if Alt last
        // left them OFF. In keyboard mode EnsureLabels only marks refresh dirty.
        if(!EnsureLabels())return;
        if(ControllerLabelsStale()) {
            labelRefreshNeeded=true;
            EnsureLabels();
            if(trace)ctx->LogInfo("[QOL/Labels] Recovered stale controller display flag through native refresh.");
        }
    },nullptr)!=D2RL::Threads::Result::Success)labelRecoveryQueued=false;
}

bool QolNavigation::RangesRemapEnabled() noexcept {return QolNativeRanges::active.load(std::memory_order_acquire);}

bool QolNavigation::LootFilterRemapEnabled() noexcept {
    std::lock_guard lock(stateMutex);return SubMenusEnabled() && (subPanels&16);
}
