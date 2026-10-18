#include "cube_controls.h"
#include "cube_controls_policy.h"
#include "item_code_compatibility.h"
#include "native_identify_profile.h"
#include "storage_focus_profile.h"
#include "filter_focus_signatures.h"
#include "materials_signatures.h"
#include "belt_signatures.h"
#include "identify_probe_profile.h"
#include "native_input_profile.h"
#include "client_transfer_policy.h"
#include "materials_actions.h"
#include "header_policy.h"
#include "vendor_signatures.h"
#include "cube_native_calls.h"
#include "cube_native_profile.h"
#include "relay_allocation.h"
#include "controller_input.h"
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <mutex>
#include <atomic>
#include <cstdio>
#include <limits>

namespace QolCubeControls {
namespace {
std::recursive_mutex mutex;
const D2RL::PluginContext* context{};const D2RL::WidgetService* widgets{};
uintptr_t core{};uint64_t session{};uint32_t player{};
bool admitted{},ready{},swapEnabled{},transmuteEnabled{};
uint16_t modifierButtons=0x100;
Gesture gesture{};char transmuteText[96]{};
NativeHold nativeHold{};HoverRequest hoverRequest{};
const D2RL::SharedEventService* sharedEvents{};
D2RL::SharedEvents::ListenerHandle hoverListener{};
std::atomic<uint64_t> hintSnapshot{};
bool nativeCalls{},operationBusy{},modifierDown{};
uint64_t inputTick{};uint16_t observedButtons{};
uint64_t nativeEntries[4]{};unsigned nativeTraces[4]{};
unsigned char callPatches[4][8]{};
using Formatter=void*(__fastcall*)(void*,const char*,const char*);
Formatter originalFormatter{};
using StringGetter=const char*(__fastcall*)(uint32_t);
StringGetter originalString{};
constexpr unsigned char Gateway[]={0xff,0x25,0x5e,0xf5,0xff,0xff};
constexpr unsigned char UiBlocked[]={0x48,0x8b,0x05,0x99,0xa0,0xbf,0x02,0x48,0x85,0xc0,0x74,0x08,0x0f,0xb6,0x80,0xb8,0x00,0x00,0x00,0xc3,0xc3};
constexpr unsigned char CoreTap[]={0x55,0x48,0x83,0xec,0x40,0x48,0x8d,0x6c,0x24,0x40,0x48,0xc7,0x45,0xf8,0xfe,0xff,0xff,0xff,0x48,0x89,0x55,0xf0,0x48,0x8d,0x55,0xf0,0x48,0x89,0x4d,0xe8,0xff,0x15,0xa4,0x98,0xef,0xff};
template<class T>T At(uintptr_t rva) noexcept {return reinterpret_cast<T>(context->exeBase+rva);}
bool Bytes(uintptr_t base,uintptr_t rva,const unsigned char* expected,size_t size) noexcept {
    unsigned char actual[1024]{};SIZE_T got{};
    return size<=sizeof(actual) && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),actual,size,&got) && got==size && !std::memcmp(actual,expected,size);
}
bool CommonGuard() noexcept {
    using namespace QolNativeIdentifyProfile;
    return context && admitted &&
        Bytes(context->exeBase,0xce500,QolMaterials::Signatures::UiMode,sizeof(QolMaterials::Signatures::UiMode)) &&
        Bytes(context->exeBase,0x846170,FindPanel,sizeof(FindPanel)) && Bytes(context->exeBase,0x856220,FindChild,sizeof(FindChild)) &&
        Bytes(context->exeBase,0x14f1e0,Cursor,sizeof(Cursor)) && Bytes(context->exeBase,0x14f210,Mode,sizeof(Mode)) &&
        Bytes(context->exeBase,0x8460d0,UiBlocked,sizeof(UiBlocked));
}
bool CursorFree() noexcept {return CursorAvailable(At<void*(__fastcall*)()>(0x14f1e0)()!=nullptr,At<int(__fastcall*)()>(0x14f210)());}
bool Visible(void* widget) noexcept {const auto p=static_cast<const unsigned char*>(widget);return p && p[0x50] && p[0x51];}
void* Panel(const char* name) noexcept {return At<void*(__fastcall*)(const char*)>(0x846170)(name);}
void* Child(void* panel,const char* name) noexcept {return panel?At<void*(__fastcall*)(void*,const char*)>(0x856220)(panel,name):nullptr;}
bool Mode(int value) noexcept {return At<bool(__fastcall*)(int)>(0xce500)(value);}
const char* CubeTarget() noexcept {
    if(At<bool(__fastcall*)()>(0x8460d0)() || Mode(0x0b) || Mode(0x17) || !CursorFree())return nullptr;
    if(Mode(0x19)) {
        auto panel=Panel("HoradricCubeLayout");
        if(Visible(panel) && Visible(Child(panel,"convert")))return "HoradricCubePanelMessage";
    }
    if(Mode(0x18) && QolClientTransfer::AdvancedTab(QolMaterials::SelectedStashTab(context))) {
        auto panel=Panel("BankExpansionLayout");if(!Visible(panel))panel=Panel("BankOriginalLayout");
        if(Visible(panel) && Visible(Child(panel,"convert")))return "BankPanelMessage";
    }
    return nullptr;
}
struct Focus {Identity identity{};void* grid{};void* item{};};
bool Resolve(Focus& out,CursorUse use=CursorUse::Idle) noexcept {
    using namespace QolNativeIdentifyProfile;
    const auto game=context->exeBase;
    const bool holdArmed=(nativeHold.armed || (use==CursorUse::CompleteHold && nativeHold.completed)) &&
        observedButtons==A && !modifierDown && !operationBusy;
    if(At<bool(__fastcall*)()>(0x8460d0)() || !Mode(1) || Mode(0xb) || Mode(0x17) ||
       !ResolveCursorAvailable(At<void*(__fastcall*)()>(0x14f1e0)()!=nullptr,At<int(__fastcall*)()>(0x14f210)(),use,holdArmed) ||
       !Bytes(game,0x846020,QolFilterFocus::FocusGetter,sizeof(QolFilterFocus::FocusGetter)) ||
       !Bytes(game,0x876160,QolStorageProfile::ControllerFocus,sizeof(QolStorageProfile::ControllerFocus)) ||
       !Bytes(game,0x2c49f0,Lookup,sizeof(Lookup)) || !Bytes(game,0x2a7810,Owner,sizeof(Owner)) ||
       !Bytes(game,0x36cfe0,QolVendor::ItemPageBytes,sizeof(QolVendor::ItemPageBytes)) ||
       !QolItemCode::Match(game,0x36ef50,Code,sizeof(Code)))return false;
    const auto manager=*reinterpret_cast<const unsigned char* const*>(game+0x3440170);
    const auto focus=manager?*reinterpret_cast<const unsigned char* const*>(manager+0xd0):nullptr;
    auto grid=focus?*reinterpret_cast<unsigned char* const*>(focus+0x190):nullptr;
    if(!Visible(grid))return false;
    const auto table=*reinterpret_cast<const uintptr_t* const*>(grid);
    if(!table || table[0xc8/8]!=game+0x2c49f0 || table[0x118/8]!=game+0x2aa7e0 || table[0x100/8]!=game+0x3e2b274)return false;
    struct Cell {int32_t x,y;};const auto cell=*reinterpret_cast<const Cell*>(grid+0x544);
    if(cell.x<0 || cell.y<0 || cell.x>255 || cell.y>255)return false;
    auto item=static_cast<const uint32_t*>(At<void*(__fastcall*)(void*,const Cell*)>(0x2c49f0)(grid,&cell));
    if(!item || item[0]!=4 || !item[2] ||
       At<uint32_t(__fastcall*)(const void*)>(0x36ef50)(item)!=D2RL::Items::MakeItemCode("box") ||
       At<uint8_t(__fastcall*)(const void*)>(0x36cfe0)(item)!=0)return false;
    const auto actor=static_cast<const uint32_t*>(At<void*(__fastcall*)(void*)>(0x2a7810)(grid));
    if(!actor || actor[0]!=0 || actor[2]!=player)return false;
    // The original game hold and Core tap gateways remain owned by their
    // original providers. Refuse changed bodies/slots rather than bypass them.
    if(!Bytes(game,0x2aa7e0,Hold,sizeof(Hold)) || !Bytes(game,0x2c7540,Activate,sizeof(Activate)) ||
       !Bytes(game,QolCubeNativeProfile::PickupCursorBranchRva,QolCubeNativeProfile::PickupCursorBranch,sizeof(QolCubeNativeProfile::PickupCursorBranch)) ||
       !Bytes(game,0x3e2b274,Gateway,sizeof(Gateway)) || !Bytes(core,0x8150a0,CoreTap,sizeof(CoreTap)) ||
       *reinterpret_cast<const uintptr_t*>(game+0x3e2a7d8)!=core+0x8150a0 ||
       *reinterpret_cast<const uintptr_t*>(core+0x70e9c8)!=game+0x2c7540 ||
       *reinterpret_cast<const uintptr_t*>(core+0x70e968)!=game+0x2c49f0 ||
       !Bytes(game,0x1c7360,QolIdentifyProbe::Blocked,sizeof(QolIdentifyProbe::Blocked)) ||
       At<int(__fastcall*)(const void*)>(0x1c7360)(item)!=0)return false;
    out={{session,player,item[2],cell.x,cell.y},grid,const_cast<uint32_t*>(item)};return true;
}
bool HoverConvertGuard() noexcept {
    using namespace QolCubeNativeProfile;
    return Bytes(context->exeBase,ConvertHandlerRva,ConvertHandler,sizeof(ConvertHandler)) &&
        Bytes(context->exeBase,ConvertStartRva,ConvertStart,sizeof(ConvertStart)) &&
        Bytes(context->exeBase,ConvertSubmitRva,ConvertSubmit,sizeof(ConvertSubmit)) &&
        Bytes(context->exeBase,ConvertPreludeRva,ConvertPrelude,sizeof(ConvertPrelude)) &&
        Bytes(context->exeBase,ConvertBodyRva,ConvertBody,sizeof(ConvertBody));
}
bool ItemDescriptionGuard() noexcept {
    using namespace QolCubeNativeProfile;
    return Bytes(context->exeBase,ItemDescriptionBeforeRva,ItemDescriptionBefore,sizeof(ItemDescriptionBefore)) &&
        Bytes(context->exeBase,ItemDescriptionAfterRva,ItemDescriptionAfter,sizeof(ItemDescriptionAfter)) &&
        Bytes(context->exeBase,StringGatewayRva,StringGateway,sizeof(StringGateway)) &&
        Bytes(core,CoreStringRva,CoreString,sizeof(CoreString)) &&
        *reinterpret_cast<const uintptr_t*>(context->exeBase+StringSlotRva)==core+CoreStringRva;
}
bool NativeCallsOwned() noexcept {
    if(!nativeCalls || !context)return false;
    for(size_t i=0;i<std::size(QolCubeNativeCalls::Sites);++i) {
        const auto& site=QolCubeNativeCalls::Sites[i];
        if(!Bytes(context->exeBase,site.rva,callPatches[i],site.size))return false;
    }
    return Bytes(context->exeBase,QolCubeNativeProfile::FormatterRva,QolCubeNativeProfile::Formatter,sizeof(QolCubeNativeProfile::Formatter)) && ItemDescriptionGuard();
}
// Bounded diagnostics: no input injection, state edits, or extra native actions.
void TraceNative(unsigned kind,void* grid,uintptr_t original,bool handled) noexcept {
    ++nativeEntries[kind];
    __try {
        if(!context || !ready || nativeTraces[kind]>=12 || !ControllerQoL::IsControllerUiActive())return;
        Focus focus{};if(!CommonGuard() || !Resolve(focus,kind==1?CursorUse::CompleteHold:CursorUse::Idle))return;
        ++nativeTraces[kind];
        const auto now=GetTickCount64();
        char line[360]{};
        std::snprintf(line,sizeof(line),"[QOL/CubeTrace] path=%s described=%u raw=0x%04x owned=%u ageMs=%llu gridMatch=%u callee=0x%llx cursorMode=%d busy=%u modifier=%u entry=%llu.",
            kind==0?"tap":kind==1?"hold":"prompt",static_cast<unsigned>(handled),observedButtons,
            static_cast<unsigned>(NativeCallsOwned()),static_cast<unsigned long long>(now>=inputTick?now-inputTick:~0ull),
            static_cast<unsigned>(focus.grid==grid),static_cast<unsigned long long>(original-context->exeBase),
            At<int(__fastcall*)()>(0x14f210)(),static_cast<unsigned>(operationBusy),static_cast<unsigned>(modifierDown),
            static_cast<unsigned long long>(nativeEntries[kind]));
        context->LogInfo(line);
    } __except(EXCEPTION_EXECUTE_HANDLER) { /* Diagnostics never alter routing. */ }
}
uint64_t PackedCell(const Identity& identity) noexcept {
    return static_cast<uint64_t>(static_cast<uint32_t>(identity.x))|(static_cast<uint64_t>(static_cast<uint32_t>(identity.y))<<32);
}
bool RouteNativeUnsafe(void* grid,uint64_t cell,void* item,uintptr_t original,bool held,bool& submitted) noexcept {
    if(!NativeCallsOwned() || !ready || !swapEnabled || !CommonGuard() ||
       (observedButtons&~A) || !QolHeaderPolicy::Fresh(GetTickCount64(),inputTick) || !ControllerQoL::IsControllerUiActive())return false;
    Focus current{};if(!Resolve(current,held?CursorUse::CompleteHold:CursorUse::Idle))return false;
    const bool matches=current.grid==grid && (held?current.item==item:PackedCell(current.identity)==cell) &&
        original==context->exeBase+(held?0x2aa7e0:0x3e2b274);
    const auto action=NativeAction(held,true,matches,modifierDown,operationBusy);
    if(action==Action::None)return false;
    if(held) {
        if(observedButtons!=A || !nativeHold.Matches(current.identity))return false;
        if(nativeHold.completed)return true;
        nativeHold.Complete();
    }
    submitted=true; // Never retry the original action after an uncertain native submission.
    if(action==Action::Open) {
        At<void(__fastcall*)(void*,void*)>(0x2aa7e0)(grid,current.item);
        context->LogInfo("[QOL/Cube] Tap A: native open request submitted; native hold feedback retained.");
    } else {
        At<void(__fastcall*)(void*,uint64_t)>(0x3e2b274)(grid,PackedCell(current.identity));
        context->LogInfo("[QOL/Cube] Hold A: native pickup request submitted; native hold feedback retained.");
    }
    return true;
}
bool RouteNative(void* grid,uint64_t cell,void* item,uintptr_t original,bool held) noexcept {
    bool submitted=false;
    __try {return RouteNativeUnsafe(grid,cell,item,original,held,submitted);}
    __except(EXCEPTION_EXECUTE_HANDLER) {
        nativeCalls=false;
        if(context)context->LogWarn("[QOL/Cube] Native call adapter fault; disabled without retrying an uncertain action.");
        return submitted;
    }
}
void __fastcall TapCall(void* grid,uint64_t cell,uintptr_t original) noexcept {
    std::lock_guard lock(mutex);
    TraceNative(0,grid,original,false);
    const bool handled=RouteNative(grid,cell,nullptr,original,false);
    if(!handled && original)
        reinterpret_cast<void(__fastcall*)(void*,uint64_t)>(original)(grid,cell);
}
void __fastcall HoldCall(void* grid,void* item,uintptr_t original) noexcept {
    std::lock_guard lock(mutex);
    TraceNative(1,grid,original,false);
    const bool handled=RouteNative(grid,0,item,original,true);
    if(!handled && original)
        reinterpret_cast<void(__fastcall*)(void*,void*)>(original)(grid,item);
}
bool DescribeCube(void* grid) noexcept {
    __try {
        Focus focus{};
        return NativeCallsOwned() && ready && swapEnabled && CommonGuard() &&
            ControllerQoL::IsControllerUiActive() && Resolve(focus,CursorUse::Description) && focus.grid==grid;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void* __fastcall PromptCall(void* output,const char* format,const char* action,void* grid) noexcept {
    std::lock_guard lock(mutex);
    const bool described=DescribeCube(grid);TraceNative(2,grid,reinterpret_cast<uintptr_t>(originalFormatter),described);
    if(described)format="\xEE\x80\x8F Open (Hold: Pick Up)";
    return originalFormatter(output,format,action);
}
bool DescribeItem(uint32_t id,const void* item) noexcept {
    __try {
        Focus focus{};
        return id==23009 && NativeCallsOwned() && ready && swapEnabled && CommonGuard() &&
            ControllerQoL::IsControllerUiActive() && Resolve(focus,CursorUse::Description) && focus.item==item;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
const char* __fastcall ItemPromptCall(uint32_t id,const void* item) noexcept {
    std::lock_guard lock(mutex);
    // Preserve the original getter, bounded native copy, newline and color.
    const auto text=originalString(id);
    if(!DescribeItem(id,item))return text;
    if(!nativeTraces[3]) {++nativeTraces[3];context->LogInfo("[QOL/Cube] Carried Cube item instruction replaced; native tooltip formatting retained.");}
    return "\xEE\x81\xBE" "4\xEE\x80\x8F Open (Hold: Pick Up)";
}
bool SubmitHoverUnsafe(const D2RL::SharedEvents::UiMessageEvent* event,bool& submitted) noexcept {
    if(!event || event->structSize<D2RL::SharedEvents::UiMessageEventRequiredSize || event->flags ||
       event->targetHash!=0x85b4ec198d19aa27ull || event->commandHash!=0xfa0c605f107fc998ull || (event->text && *event->text) ||
       !ready || !transmuteEnabled || !CommonGuard() || operationBusy ||
       GetCurrentThreadId()!=*reinterpret_cast<const DWORD*>(core+0x7df080) ||
       !QolHeaderPolicy::Fresh(GetTickCount64(),inputTick) || !ControllerQoL::IsControllerUiActive())return false;
    // Open panels always keep their existing SDK/native handling. This tail
    // exists only for an owned shortcut whose hidden panel cannot receive it.
    if(Mode(0x19) || Mode(0x18)) {hoverRequest.Cancel();return false;}
    Focus current{};
    if(!Resolve(current) || !hoverRequest.Matches(GetTickCount64(),current.identity) || !HoverConvertGuard())return false;
    if(!hoverRequest.Begin(GetTickCount64(),current.identity))return false;
    submitted=true;
    At<void(__fastcall*)()>(QolCubeNativeProfile::ConvertPreludeRva)();
    At<void(__fastcall*)()>(QolCubeNativeProfile::ConvertStartRva)();
    hoverRequest.End();
    context->LogInfo("[QOL/Cube] Highlighted Cube: native Convert submitted once after shared listeners; native recipe/server checks retained.");
    return true;
}
bool SubmitHover(const D2RL::SharedEvents::UiMessageEvent* event) noexcept {
    bool submitted=false;
    __try {return SubmitHoverUnsafe(event,submitted);}
    __except(EXCEPTION_EXECUTE_HANDLER) {
        hoverRequest.Cancel();transmuteEnabled=false;
        if(context)context->LogWarn("[QOL/Cube] Hover Convert fault; shortcut disabled without retrying an uncertain request.");
        return submitted;
    }
}
D2RL::SharedEvents::UiMessageAction __cdecl HoverMessage(const D2RL::PluginContext* owner,const D2RL::SharedEvents::UiMessageEvent* event,void*) noexcept {
    std::lock_guard lock(mutex);
    return owner==context && SubmitHover(event)?D2RL::SharedEvents::UiMessageAction::Consume:D2RL::SharedEvents::UiMessageAction::Continue;
}
bool InstallNativeCalls() noexcept {
    using namespace QolCubeNativeProfile;
    if(!swapEnabled || !CommonGuard() ||
       !Bytes(context->exeBase,ControllerCallsRva,ControllerCalls,sizeof(ControllerCalls)) ||
       !Bytes(context->exeBase,PromptCallRva,QolCubeNativeProfile::PromptCall,sizeof(QolCubeNativeProfile::PromptCall)) ||
       !Bytes(context->exeBase,FormatterRva,QolCubeNativeProfile::Formatter,sizeof(QolCubeNativeProfile::Formatter)) || !ItemDescriptionGuard())return false;
    for(const auto& site:QolCubeNativeCalls::Sites)
        if(!context->CheckExpectedBytes(site.rva,site.expected,static_cast<uint32_t>(site.size)))return false;
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&TapCall),&pinned))return false;
    auto* relays=QolRelay::Allocate(context->exeBase+QolCubeNativeCalls::Sites[0].rva,128);
    if(!relays)return false;
    const uintptr_t callbacks[]={reinterpret_cast<uintptr_t>(&TapCall),reinterpret_cast<uintptr_t>(&HoldCall),reinterpret_cast<uintptr_t>(&PromptCall),reinterpret_cast<uintptr_t>(&ItemPromptCall)};
    for(size_t i=0;i<std::size(callbacks);++i) {
        unsigned char code[32]{};const auto& site=QolCubeNativeCalls::Sites[i];
        const auto size=QolCubeNativeCalls::Relay(site.kind,callbacks[i],code);
        if(!size || !QolCubeNativeCalls::Patch(context->exeBase+site.rva,reinterpret_cast<uintptr_t>(relays+i*32),site.size,callPatches[i])) {
            VirtualFree(relays,0,MEM_RELEASE);return false;
        }
        std::memcpy(relays+i*32,code,size);
    }
    DWORD protection{};
    if(!VirtualProtect(relays,128,PAGE_EXECUTE_READ,&protection)) {VirtualFree(relays,0,MEM_RELEASE);return false;}
    FlushInstructionCache(GetCurrentProcess(),relays,128);
    // Retain published relays even on partial setup; inactive adapters forward
    // the original per-call callee exactly once. SDK owns patch cleanup.
    for(size_t i=0;i<std::size(callbacks);++i) {
        const auto& site=QolCubeNativeCalls::Sites[i];
        if(!context->PatchBytes(site.rva,site.expected,static_cast<uint32_t>(site.size),callPatches[i],static_cast<uint32_t>(site.size)))return false;
    }
    return true;
}
uint16_t InputUnsafe(uint16_t buttons,bool modifier,uint64_t now,bool busy) noexcept {
    operationBusy=busy;modifierDown=modifier;inputTick=now;observedButtons=buttons;
    Focus focus{};const bool valid=ready && CommonGuard();
    const bool canTrack=valid && swapEnabled && NativeCallsOwned() && !modifier && !busy && buttons==A && ControllerQoL::IsControllerUiActive();
    Focus tracked{};
    const bool trackingFocus=canTrack && Resolve(tracked,CursorUse::TrackHold);
    nativeHold.Update(buttons,canTrack,trackingFocus,tracked.identity);
    const bool carried=valid && (swapEnabled || transmuteEnabled) && Resolve(focus);
    const bool focused=carried && swapEnabled;
    const auto openTarget=valid && transmuteEnabled?CubeTarget():nullptr;
    const bool hover=valid && transmuteEnabled && hoverListener && !Mode(0x19) && !Mode(0x18) && TransmuteAvailable(false,carried,carried && HoverConvertGuard());
    const auto target=openTarget?openTarget:hover?"HoradricCubePanelMessage":nullptr;
    // Render hints describe the controls through the native press/hold states.
    // Keep this observation separate from the idle-only action target above.
    Focus captionFocus{};
    const bool described=carried || (valid && (swapEnabled || transmuteEnabled) && Resolve(captionFocus,CursorUse::Description));
    const bool captionHover=hover || (!carried && described && transmuteEnabled && hoverListener && !Mode(0x19) && !Mode(0x18) && HoverConvertGuard());
    hintSnapshot.store((now<<2)|CaptionFlags(described,swapEnabled,transmuteEnabled,openTarget!=nullptr,captionHover));
    const Frame frame{now,buttons,modifier,swapEnabled && !NativeCallsOwned(),transmuteEnabled,valid,focused,target!=nullptr,busy,focus.identity,modifierButtons};
    const auto decision=gesture.Update(frame);
    if(decision.action==Action::Open || decision.action==Action::PickUp) {
        Focus current{};
        if(!Resolve(current) || !(current.identity==gesture.captured))return decision.consumed;
        if(decision.action==Action::Open) {
            At<void(__fastcall*)(void*,void*)>(0x2aa7e0)(current.grid,current.item);
            context->LogInfo("[QOL/Cube] Tap A: native open request submitted.");
        } else {
            const auto cell=PackedCell(current.identity);
            At<void(__fastcall*)(void*,uint64_t)>(0x3e2b274)(current.grid,cell);
            context->LogInfo("[QOL/Cube] Hold A: native pickup request submitted.");
        }
    } else if(decision.action==Action::Transmute && target) {
        hoverRequest.Cancel();
        if(!openTarget && hover)hoverRequest.Arm(now,focus.identity);
        const D2RL::Widgets::UiAction action{D2RL::Widgets::UiActionSize,0,target,"Convert",nullptr};
        const auto result=widgets->dispatchUiAction(context,&action);
        if(result!=D2RL::Widgets::Result::Success)hoverRequest.Cancel();
        char line[160];std::snprintf(line,sizeof(line),"[QOL/Cube] Transmute shortcut submitted once via %s (dispatch=%u); native recipe/server checks retained.",target,static_cast<unsigned>(result));
        context->LogInfo(line);
    }
    return decision.consumed;
}
uint16_t SafeInput(uint16_t buttons,bool modifier,uint64_t now,bool busy) noexcept {
    __try {return InputUnsafe(buttons,modifier,now,busy);}
    __except(EXCEPTION_EXECUTE_HANDLER) {admitted=false;gesture.Cancel();nativeHold.Cancel();hoverRequest.Cancel();hintSnapshot=0;if(context)context->LogWarn("[QOL/Cube] Native UI fault; Cube shortcuts disabled, no retry.");return gesture.ownedA?A:0;}
}
bool Recent(unsigned bit) noexcept {
    const auto snapshot=hintSnapshot.load();const auto tick=snapshot>>2;
    return (snapshot&bit) && QolHeaderPolicy::Fresh(GetTickCount64(),tick);
}
}
void Initialize(const D2RL::PluginContext* ctx,const D2RL::WidgetService* service,bool swap,bool transmute,const char* modifier) noexcept {
    std::lock_guard lock(mutex);context=ctx;widgets=service;gesture={};nativeHold={};hoverRequest={};hintSnapshot=0;
    sharedEvents=nullptr;hoverListener=0;
    swapEnabled=swap;transmuteEnabled=transmute;ready=false;
    nativeCalls=false;inputTick=0;observedButtons=0;operationBusy=modifierDown=false;
    std::memset(nativeEntries,0,sizeof(nativeEntries));std::memset(nativeTraces,0,sizeof(nativeTraces));
    core=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"D2RCore.dll"));
    admitted=ctx && QolCore::VerifyFileHash(reinterpret_cast<HMODULE>(core),QolNativeProfile::CoreHash) &&
        D2RL::HasWidgetServiceField(service,D2RL::WidgetServiceRequiredSize) && service->dispatchUiAction;
    originalFormatter=ctx?At<Formatter>(QolCubeNativeProfile::FormatterRva):nullptr;
    originalString=ctx?At<StringGetter>(QolCubeNativeProfile::StringGatewayRva):nullptr;
    if(admitted && transmute && ctx->QueryService(&sharedEvents)==D2RL::ServiceQueryResult::Success &&
       D2RL::HasSharedEventServiceField(sharedEvents,D2RL::SharedEventServiceRequiredSize) &&
       sharedEvents->registerUiMessageListener && sharedEvents->unregisterUiMessageListener) {
        const D2RL::SharedEvents::UiMessageListener listener{D2RL::SharedEvents::UiMessageListenerSize,0,std::numeric_limits<int32_t>::min(),0,HoverMessage,nullptr};
        if(sharedEvents->registerUiMessageListener(ctx,&listener,&hoverListener)!=D2RL::SharedEvents::Result::Success)hoverListener=0;
    }
    nativeCalls=InstallNativeCalls();
    if(ctx && swap)ctx->LogInfo(nativeCalls?"[QOL/Cube] Scoped native tap/hold and prompt calls installed; native hold animation retained.":"[QOL/Cube] Native call-site profile unavailable; guarded timed tap/hold fallback retained.");
    const auto mode=modifier?modifier:"bumper";
    const auto label=(!_stricmp(mode,"bumper") || !_stricmp(mode,"lb") || !_stricmp(mode,"l1"))?"LB":
        !_stricmp(mode,"l3")?"L3":!_stricmp(mode,"r3")?"R3":!_stricmp(mode,"any")?"LB":
        (!std::strcmp(mode,"[") || !_stricmp(mode,"bracket") || !_stricmp(mode,"l4") || !_stricmp(mode,"leftbracket"))?"[":"LT";
    modifierButtons=!std::strcmp(label,"LB")?0x100:!std::strcmp(label,"L3")?0x40:!std::strcmp(label,"R3")?0x80:!std::strcmp(label,"[")?0:0x400;
    if(!_stricmp(mode,"any"))modifierButtons=0x500;
    const auto glyph=!std::strcmp(label,"R3")?QolHeaderPolicy::Buttons[3]:QolHeaderPolicy::Modifier(label);
    std::snprintf(transmuteText,sizeof(transmuteText),"%s+\xEE\x80\x90 Transmute",glyph);
    if(ctx)ctx->LogInfo(admitted?"[QOL/Cube] Controller tap-open/hold-pickup and modifier+B transmute available; native UI routes retained.":"[QOL/Cube] Required native/UI services unavailable; original Cube controls retained.");
}
void OnLifecycle(const D2RL::Lifecycle::GameplayEvent* event) noexcept {
    std::lock_guard lock(mutex);gesture.Cancel();nativeHold.Cancel();hoverRequest.Cancel();hintSnapshot=0;inputTick=0;
    if(event && D2RL::Lifecycle::HasGameplayEventField(event,D2RL::Lifecycle::GameplayEventRequiredSize) &&
       event->kind==D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady && event->sessionGeneration && event->playerId!=UINT32_MAX) {session=event->sessionGeneration;player=event->playerId;ready=true;}
    else {++session;player=0;ready=false;}
}
void Cancel() noexcept {std::lock_guard lock(mutex);gesture.Cancel();nativeHold.Cancel();hoverRequest.Cancel();hintSnapshot=0;inputTick=0;}
void Shutdown() noexcept {
    std::lock_guard lock(mutex);nativeCalls=ready=admitted=false;gesture.Cancel();nativeHold.Cancel();hoverRequest.Cancel();hintSnapshot=0;inputTick=0;
    if(hoverListener && sharedEvents)sharedEvents->unregisterUiMessageListener(context,hoverListener);
    hoverListener=0;
}
uint16_t Input(uint16_t buttons,bool modifier,uint64_t now,bool busy) noexcept {std::lock_guard lock(mutex);return SafeInput(buttons,modifier,now,busy);}
const char* PrimaryHint() noexcept {return Recent(1)?"\xEE\x80\x8F Open (Hold: Pick Up)":nullptr;}
const char* TransmuteHint() noexcept {return Recent(2)?transmuteText:nullptr;}
}
