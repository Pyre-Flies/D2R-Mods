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
#include "qol_navigation.h"
#include "qol_glyphs.h"
#include "physical_input.h"
#include "core_compatibility.h"
#include "native_signatures.h"
#include "label_signatures.h"
#include "menu_signatures.h"
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
        isInGame && isInGame(),channel);
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
    ctx->LogInfo(labelHooks?"[QOL/Labels] Label press/release hooks installed via SDK. L1 mode locks FILTERED labels ON.":
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
void InstallMenuHook() noexcept {
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
void __fastcall HookTabMessage(void* widget,void* message) noexcept {
    // Preserve the original function's void ABI. Do not mutate input, widget
    // selection, message payload, or raw state used by controller-qol.
    bool block=false;
    MenuCopy subCopy;
    int subDecision=0;
    {
        std::lock_guard lock(stateMutex);
        ++nativeCalls;
        if(ctx) {
            auto n=DecodeNative(widget,message);
            if(n.fault) ++faults;
            if(n.controllerBegin) {
                ++nativeControllerCalls;
                if(SubMenusEnabled() && (subPanels&1) && n.switchEnabled && n.left==7 && n.right==8) {
                    subDecision=PrepareMenuMessage(message,subCopy,true);
                    if(subDecision<0) ++questReleased;
                    if(subDecision>0) ++questTranslated;
                    if(subDecision && trace) ctx->LogInfo(subDecision>0?
                        "[QOL/Submenu] Quest: bumper translated to act tab navigation.":
                        "[QOL/Submenu] Quest: trigger released to main menu without marking input handled.");
                }
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
    if(!block && subDecision>=0 && originalTabMessage) originalTabMessage(widget,subDecision>0?subCopy.message:message);
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
    sharedPageInputActive.store(false);
    QolGlyphs::Shutdown();
    if(ctx && events) {
        if(uiHandle && events->unregisterUiMessageListener) events->unregisterUiMessageListener(ctx,uiHandle);
        if(tooltipHandle && events->unregisterItemTooltipListener) events->unregisterItemTooltipListener(ctx,tooltipHandle);
    }
    uiHandle=tooltipHandle=0;
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
