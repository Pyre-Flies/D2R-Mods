#include "controller_input.h"
#include "bulk_stash_input.h"
#include "physical_input.h"
#include "xinput_hook.h"
#include "native_input_policy.h"
#include "native_input_profile.h"
#include <D2RLPlugin/api.h>
#include "core_compatibility.h"
#include <mutex>
#include "shared_page_policy.h"
#include <windows.h>
#include <xinput.h>
#include <intrin.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <string>
#include <cstdlib>

namespace ControllerQoL {

using XInputGetStateFn = DWORD(WINAPI*)(DWORD dwUserIndex, XINPUT_STATE* pState);

static HMODULE s_hXInput = nullptr;
static XInputGetStateFn s_pfnXInputGetState = nullptr;
static bool s_initialized = false;
static HMODULE s_PhysicalModules[3]{};
static XInputGetStateFn s_PhysicalReaders[3]{};
static XInputGetStateFn s_PrimaryExport = nullptr;
static char s_ActiveModifierMode[32] = "any";
static uint8_t s_ActiveTriggerThreshold = 30;
static std::atomic<uint64_t> s_HookCallCount{0};

using HookRecord = QolXInput::Record;
static QolXInput::Gate s_InputGate;
static bool s_XInputRetired=false;
static char s_XInputReport[512]{};
const char* GetXInputHookReport() noexcept { return s_XInputReport; }

struct ModuleHooks {
    HookRecord getState;
    HookRecord getStateEx;
};

static ModuleHooks s_ModuleHooks[3];

static std::atomic<uint8_t> s_CachedLeftTrigger{0};
static std::atomic<uint8_t> s_CachedRightTrigger{0};
static std::atomic<uint16_t> s_CachedButtons{0};
static std::atomic<DWORD> s_CachedUserIndex{0};
static std::atomic<ULONGLONG> s_LastStateTick{0};

void SetActiveModifier(const char* mode, uint8_t triggerThreshold) noexcept {
    if (mode && mode[0] != '\0') {
        std::strncpy(s_ActiveModifierMode, mode, sizeof(s_ActiveModifierMode) - 1);
        s_ActiveModifierMode[sizeof(s_ActiveModifierMode) - 1] = '\0';
    } else {
        std::strcpy(s_ActiveModifierMode, "any");
    }
    s_ActiveTriggerThreshold = triggerThreshold;
}

uint64_t GetHookCallCount() noexcept {
    return s_HookCallCount.load();
}

void GetCachedControllerState(uint8_t& outLT, uint8_t& outRT, uint16_t& outButtons, uint64_t& outAgeMs) noexcept {
    outLT = s_CachedLeftTrigger.load(std::memory_order_relaxed);
    outRT = s_CachedRightTrigger.load(std::memory_order_relaxed);
    outButtons = s_CachedButtons.load(std::memory_order_relaxed);
    const ULONGLONG last = s_LastStateTick.load(std::memory_order_relaxed);
    const ULONGLONG now = GetTickCount64();
    outAgeMs = (last > 0 && now >= last) ? (now - last) : 999999;
}

static bool IsActiveModifierHeldForPad(DWORD dwUserIndex, const XINPUT_STATE* pState) noexcept {
    (void)dwUserIndex;
    if (!pState) return false;

    const bool isLtHeld = (pState->Gamepad.bLeftTrigger > s_ActiveTriggerThreshold);
    const bool isLbHeld = (pState->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    const bool isBracketHeld = ((GetAsyncKeyState(VK_OEM_4) & 0x8000) != 0);

    if (_stricmp(s_ActiveModifierMode, "trigger") == 0 || _stricmp(s_ActiveModifierMode, "lt") == 0 || _stricmp(s_ActiveModifierMode, "l2") == 0) {
        return isLtHeld;
    }
    if (_stricmp(s_ActiveModifierMode, "bumper") == 0 || _stricmp(s_ActiveModifierMode, "lb") == 0 || _stricmp(s_ActiveModifierMode, "l1") == 0) {
        return isLbHeld;
    }
    if (_stricmp(s_ActiveModifierMode, "l3") == 0) {
        return (pState->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    }
    if (_stricmp(s_ActiveModifierMode, "r3") == 0) {
        return (pState->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    }
    // "any" mode: accepts Left Trigger, Left Bumper, or [ bracket
    if (_stricmp(s_ActiveModifierMode, "any") == 0) {
        return isLtHeld || isLbHeld || isBracketHeld;
    }

    return isLtHeld || isLbHeld || isBracketHeld;
}

void InitControllerInput() noexcept {
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    const wchar_t* names[] = {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"};
    for (unsigned i = 0; i < 3; ++i) {
        s_PhysicalModules[i] = LoadLibraryExW(names[i], nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!s_PhysicalModules[i]) continue;
        auto fn = reinterpret_cast<XInputGetStateFn>(GetProcAddress(s_PhysicalModules[i], "XInputGetState"));
        s_PhysicalReaders[i] = fn;
        if (fn && !s_hXInput) {
            s_hXInput = s_PhysicalModules[i];
            s_pfnXInputGetState = s_PrimaryExport = fn;
        }
    }
}

static std::atomic<bool> s_NativeInstalled{false};
static std::atomic<uint64_t> s_NativeSnapshot{0};
bool NativeInputInstalled() noexcept {return s_NativeInstalled.load();}

PhysicalInput ReadControllerInput() noexcept {
    if (NativeInputInstalled()) {
        const auto packed=s_NativeSnapshot.load(std::memory_order_acquire);
        return QolNativeInput::Fresh(GetTickCount64(),packed>>16)
            ? QolNativeInput::Decode(static_cast<unsigned>(packed&0xffff)) : PhysicalInput{};
    }
    InitControllerInput();
    return CollectPhysicalInput([](unsigned provider, unsigned user, PhysicalInput& sample) noexcept {
        auto fn = s_PhysicalReaders[provider];
        // Read before our own X/Y suppression and synthetic-input engine.
        // If a hook is installed on this provider, invoke its passthrough trampoline
        if (provider < 3 && s_ModuleHooks[provider].getState.installed && s_ModuleHooks[provider].getState.trampoline) {
            fn = reinterpret_cast<XInputGetStateFn>(s_ModuleHooks[provider].getState.trampoline);
        } else if (fn && fn == s_PrimaryExport) {
            fn = s_pfnXInputGetState;
        }
        XINPUT_STATE state{};
        if (!fn || fn(user, &state) != ERROR_SUCCESS) return false;
        sample.buttons = state.Gamepad.wButtons;
        sample.leftTrigger = state.Gamepad.bLeftTrigger;
        sample.rightTrigger = state.Gamepad.bRightTrigger;
        return true;
    });
}

uint8_t GetMaxLeftTriggerValue() noexcept {
    return ReadControllerInput().leftTrigger;
}

bool IsLeftTriggerPressed(uint8_t triggerThreshold) noexcept {
    if (IsNativeAltModifierActive()) {
        return true;
    }
    return GetMaxLeftTriggerValue() > triggerThreshold;
}

uint8_t GetMaxRightTriggerValue() noexcept {
    return ReadControllerInput().rightTrigger;
}

bool IsRightTriggerPressed(uint8_t triggerThreshold) noexcept {
    return GetMaxRightTriggerValue() > triggerThreshold;
}

bool IsLeftBumperPressed() noexcept {
    return (ReadControllerInput().buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
}

bool IsControllerModifierActive(const char* mode, uint8_t triggerThreshold) noexcept {
    const bool nativeAlt = IsNativeAltModifierActive();

    if (!mode || mode[0] == '\0') {
        return nativeAlt || IsLeftTriggerPressed(triggerThreshold);
    }
    if (_stricmp(mode, "any") == 0) {
        return nativeAlt || IsLeftTriggerPressed(triggerThreshold) || IsLeftBumperPressed() || ((GetAsyncKeyState(VK_OEM_4) & 0x8000) != 0);
    }
    if (_stricmp(mode, "bumper") == 0 || _stricmp(mode, "lb") == 0 || _stricmp(mode, "l1") == 0) {
        return IsLeftBumperPressed();
    }
    if (_stricmp(mode, "trigger") == 0 || _stricmp(mode, "lt") == 0 || _stricmp(mode, "l2") == 0) {
        return nativeAlt || IsLeftTriggerPressed(triggerThreshold);
    }
    return IsGroundPickupActive(mode);
}

bool IsControllerButtonPressed(uint16_t buttonMask) noexcept {
    return (ReadControllerInput().buttons & buttonMask) != 0;
}

bool IsButtonAPressed() noexcept {
    return IsControllerButtonPressed(XINPUT_GAMEPAD_A);
}

bool IsButtonBPressed() noexcept {
    return IsControllerButtonPressed(XINPUT_GAMEPAD_B);
}

bool IsButtonXPressed() noexcept {
    return IsControllerButtonPressed(XINPUT_GAMEPAD_X);
}

bool IsButtonYPressed() noexcept {
    return IsControllerButtonPressed(XINPUT_GAMEPAD_Y);
}

bool IsRightBumperPressed() noexcept {
    return IsControllerButtonPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool IsGroundPickupActive(const char* button) noexcept {
    if (!button || button[0] == '\0') return IsLeftBumperPressed();
    if (_stricmp(button, "lb") == 0 || _stricmp(button, "l1") == 0 || _stricmp(button, "bumper") == 0) {
        return IsLeftBumperPressed();
    }
    if (_stricmp(button, "rb") == 0 || _stricmp(button, "r1") == 0) {
        return IsControllerButtonPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
    }
    if (_stricmp(button, "l3") == 0) {
        return IsControllerButtonPressed(XINPUT_GAMEPAD_LEFT_THUMB);
    }
    if (_stricmp(button, "r3") == 0) {
        return IsControllerButtonPressed(XINPUT_GAMEPAD_RIGHT_THUMB);
    }
    if (_stricmp(button, "lt") == 0 || _stricmp(button, "l2") == 0 || _stricmp(button, "trigger") == 0) {
        return IsLeftTriggerPressed();
    }
    if (_stricmp(button, "rt") == 0 || _stricmp(button, "r2") == 0) {
        return IsRightTriggerPressed();
    }
    // Support [ bracket key / L4 paddle
    if (std::strcmp(button, "[") == 0 || _stricmp(button, "l4") == 0 || _stricmp(button, "bracket") == 0 || _stricmp(button, "leftbracket") == 0) {
        return (GetAsyncKeyState(VK_OEM_4) & 0x8000) != 0;
    }
    // Support ] bracket key / R4 paddle
    if (std::strcmp(button, "]") == 0 || _stricmp(button, "r4") == 0 || _stricmp(button, "rightbracket") == 0) {
        return (GetAsyncKeyState(VK_OEM_6) & 0x8000) != 0;
    }
    if (button[1] == '\0') {
        char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(button[0])));
        if (ch >= 'A' && ch <= 'Z') {
            return (GetAsyncKeyState(ch) & 0x8000) != 0;
        }
        if (ch >= '0' && ch <= '9') {
            return (GetAsyncKeyState(ch) & 0x8000) != 0;
        }
    }
    return IsLeftBumperPressed();
}

bool IsAnyControllerConnected() noexcept {
    const NativeBridgeDiag diag = QueryNativeBridgeDiagnostics();
    if (diag.mgrFound && diag.controllerMode != 0) {
        return true;
    }
    if (diag.steamFound && diag.steamControllers > 0) {
        return true;
    }

    return ReadControllerInput().valid;
}

// ---------------------------------------------------------------------------
// Controller shortcut input filtering
// ---------------------------------------------------------------------------
static QuickMoveCallback s_QuickMoveCallback = nullptr;
static TriggerPassThroughPredicate s_TriggerPassThroughPredicate=nullptr;
void SetTriggerPassThroughPredicate(TriggerPassThroughPredicate predicate) noexcept {
    s_TriggerPassThroughPredicate=predicate;
}
static QuickMoveCubeCallback s_QuickMoveCubeCallback = nullptr;

void SetQuickMoveCallback(QuickMoveCallback callback) noexcept {
    s_QuickMoveCallback = callback;
}

void SetQuickMoveCubeCallback(QuickMoveCubeCallback callback) noexcept {
    s_QuickMoveCubeCallback = callback;
}

static BulkStashCallback s_BulkStashCallback=nullptr;
static QolBulkStash::Gesture s_BulkStashGesture[8]{};
void SetBulkStashCallback(BulkStashCallback cb) noexcept {s_BulkStashCallback=cb;}
static AutoFillBeltCallback s_AutoFillBeltCallback = nullptr;

void SetAutoFillBeltCallback(AutoFillBeltCallback callback) noexcept {
    s_AutoFillBeltCallback = callback;
}

static uint16_t s_PrevDpadButtons[8] = {};
static uint16_t s_PrevRawButtons[8] = {};
static bool s_QuickMoveTriggeredX[8] = {};
static bool s_QuickMoveTriggeredY[8] = {};
static bool s_AutoFillBeltTriggered[8] = {};

struct SyntheticHoldEngine {
    bool active = false;
    uint16_t button = 0;
    ULONGLONG startTick = 0;
    DWORD maxDurationMs = 650;
    int releaseFrames = 0;
    DWORD activeUserIndex = 0;
};

static SyntheticHoldEngine s_HoldEngine;
static VendorContextPredicate s_VendorContextPredicate = nullptr;

void TriggerSyntheticHold(uint16_t button, uint32_t maxDurationMs) noexcept {
    s_HoldEngine.button = button;
    s_HoldEngine.maxDurationMs = maxDurationMs;
    s_HoldEngine.startTick = GetTickCount64();
    s_HoldEngine.releaseFrames = 0;
    s_HoldEngine.active = true;
}

void CancelSyntheticHold() noexcept {
    if (s_HoldEngine.active) {
        s_HoldEngine.active = false;
        s_HoldEngine.releaseFrames = 3;
    }
}

bool IsSyntheticHoldActive() noexcept {
    return s_HoldEngine.active;
}

void SetVendorContextPredicate(VendorContextPredicate pred) noexcept {
    s_VendorContextPredicate = pred;
}

static void ProcessGamepadShortcuts(DWORD dwUserIndex, XINPUT_STATE* pState) noexcept {
    if (!pState || dwUserIndex >= 8) return;

    s_HookCallCount.fetch_add(1);

    const uint16_t rawButtons = pState->Gamepad.wButtons;
    uint16_t buttons = rawButtons;

    const uint16_t newlyPressedAny = rawButtons & ~s_PrevRawButtons[dwUserIndex];
    s_PrevRawButtons[dwUserIndex] = rawButtons;

    const bool modHeld = IsActiveModifierHeldForPad(dwUserIndex, pState);

    // Keep L3 suppressed until release, even if the stash closes during the batch.
    if(s_BulkStashGesture[dwUserIndex].Update(modHeld,(rawButtons&XINPUT_GAMEPAD_LEFT_THUMB)!=0,
        []() noexcept {return s_BulkStashCallback && s_BulkStashCallback();}))buttons&=~XINPUT_GAMEPAD_LEFT_THUMB;

    // Vendor selling tap-protection and auto-sustain:
    // When vendor window is open and cursor is on an item in Inventory, tapping X
    // would normally drop the item on the ground unless held. We automatically sustain
    // the hold so a normal tap executes the native sell safely without dropping!
    if (s_VendorContextPredicate && s_VendorContextPredicate()) {
        if ((newlyPressedAny & XINPUT_GAMEPAD_X) != 0 && !modHeld) {
            s_HoldEngine.activeUserIndex = dwUserIndex;
            TriggerSyntheticHold(XINPUT_GAMEPAD_X, 650);
        }
    }

    // Quick Move: Modifier + X (Transfer to Stash/Inventory), Modifier + Y (Transfer to Horadric Cube), and Modifier + R3 (Fill Belt)
    const bool xPressed = (rawButtons & XINPUT_GAMEPAD_X) != 0;
    const bool yPressed = (rawButtons & XINPUT_GAMEPAD_Y) != 0;
    const bool r3Pressed = (rawButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    if (modHeld) {
        if (xPressed) {
            buttons &= ~XINPUT_GAMEPAD_X;
            if (!s_QuickMoveTriggeredX[dwUserIndex]) {
                s_QuickMoveTriggeredX[dwUserIndex] = true;
                if (s_QuickMoveCallback) {
                    s_QuickMoveCallback();
                }
            }
        } else {
            s_QuickMoveTriggeredX[dwUserIndex] = false;
        }

        if (yPressed) {
            buttons &= ~XINPUT_GAMEPAD_Y;
            if (!s_QuickMoveTriggeredY[dwUserIndex]) {
                s_QuickMoveTriggeredY[dwUserIndex] = true;
                if (s_QuickMoveCubeCallback) {
                    s_QuickMoveCubeCallback();
                }
            }
        } else {
            s_QuickMoveTriggeredY[dwUserIndex] = false;
        }

        if (r3Pressed) {
            buttons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
            if (!s_AutoFillBeltTriggered[dwUserIndex]) {
                s_AutoFillBeltTriggered[dwUserIndex] = true;
                if (s_AutoFillBeltCallback) {
                    s_AutoFillBeltCallback();
                }
            }
        } else {
            s_AutoFillBeltTriggered[dwUserIndex] = false;
        }
    } else {
        s_QuickMoveTriggeredX[dwUserIndex] = false;
        s_QuickMoveTriggeredY[dwUserIndex] = false;
        s_AutoFillBeltTriggered[dwUserIndex] = false;
    }

    // When modifier is held:
    // DO NOT mask out Button A! D2R must see A so D2RLoader's OnItemInteraction fires,
    // where quick-identify identifies the item and returns Decision::Consume to prevent pickup.
    // Suppress chorded face buttons and shoulders (B, X, Y, RB, RT, LT, R3) so D2R does NOT
    // cast spells, swap weapons, or drop items while looting ground items or managing inventory!
    PhysicalInput filterInput;
    filterInput.buttons=buttons;
    filterInput.leftTrigger=pState->Gamepad.bLeftTrigger;
    filterInput.rightTrigger=pState->Gamepad.bRightTrigger;
    const bool sharedTriggers=modHeld && s_TriggerPassThroughPredicate && s_TriggerPassThroughPredicate();
    const auto filtered=QolNativeInput::FilterModified(filterInput,modHeld,sharedTriggers);
    buttons=filtered.buttons;
    pState->Gamepad.bLeftTrigger=filtered.leftTrigger;
    pState->Gamepad.bRightTrigger=filtered.rightTrigger;

    const uint16_t dpadMask = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN |
                              XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT;
    const uint16_t newlyPressed = (rawButtons & dpadMask) & ~s_PrevDpadButtons[dwUserIndex];
    s_PrevDpadButtons[dwUserIndex] = (rawButtons & dpadMask);

    if (newlyPressed != 0) {
        // Any navigation cancels in-flight synthetic hold for safety
        if (s_HoldEngine.active) {
            CancelSyntheticHold();
        }

    }

    // Preserve physical D-pad navigation, including while the modifier is held.
    const ULONGLONG now = GetTickCount64();

    // Process synthetic button hold (e.g. native hold-to-sell)
    if (s_HoldEngine.active && dwUserIndex == s_HoldEngine.activeUserIndex) {
        if (now - s_HoldEngine.startTick >= s_HoldEngine.maxDurationMs) {
            s_HoldEngine.active = false;
            s_HoldEngine.releaseFrames = 3;
        } else {
            buttons |= s_HoldEngine.button;
        }
    } else if (s_HoldEngine.releaseFrames > 0 && dwUserIndex == s_HoldEngine.activeUserIndex) {
        buttons &= ~s_HoldEngine.button;
        s_HoldEngine.releaseFrames--;
    }

    pState->Gamepad.wButtons = buttons;
}


// Observe normalized key transitions before they mutate the game held set or
// dispatch gameplay/UI actions. The query helper alone is not a frame hook.
using NativeButtonFn=bool(__fastcall*)(void*,unsigned,unsigned);
using NativeEventFn=void(__fastcall*)(unsigned,bool);
using NativeResetFn=void(__fastcall*)(void*);
static NativeEventFn s_NativeEventOriginal=nullptr;
static NativeResetFn s_NativeResetOriginal=nullptr;
static uintptr_t s_NativeGame=0;
static const D2RL::PluginContext* s_NativeContext=nullptr;
static const D2RL::ThreadService* s_NativeThreads=nullptr;
static std::recursive_mutex s_NativeMutex;
static unsigned s_NativeRaw=0,s_NativeDelivered=0,s_NativeIndex=~0u;
static std::atomic<bool> s_NativeTaskPending{false};
static thread_local bool s_InsideNative=false;
static unsigned NativeActiveIndex() noexcept {
    __try {
        if(!s_NativeGame) return ~0u;
        const auto manager=*reinterpret_cast<const unsigned char* const*>(s_NativeGame+QolNativeProfile::ManagerSlotRva);
        const auto index=*reinterpret_cast<const unsigned*>(s_NativeGame+QolNativeProfile::ActiveIndexRva);
        return manager && *reinterpret_cast<const unsigned*>(manager+0xdc)==1 && index<8 ? index : ~0u;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return ~0u;}
}
bool IsControllerUiActive() noexcept {
    return NativeActiveIndex()!=~0u;
}
static void ResetNativeTracking() noexcept {
    s_NativeRaw=s_NativeDelivered=0;s_NativeIndex=~0u;
    s_NativeSnapshot.store(0);
    for(unsigned n=0;n<8;++n) {
        s_PrevDpadButtons[n]=s_PrevRawButtons[n]=0;
        s_QuickMoveTriggeredX[n]=s_QuickMoveTriggeredY[n]=s_AutoFillBeltTriggered[n]=false;
        s_BulkStashGesture[n]={};
    }
    s_HoldEngine={};
}
static void SeedNativeTracking(unsigned index) noexcept {
    if(index==s_NativeIndex) return;
    ResetNativeTracking();s_NativeIndex=index;
    // Game-thread only, before this provider has filtered any event for this pad.
    auto read=reinterpret_cast<NativeButtonFn>(s_NativeGame+QolNativeProfile::ButtonRva);
    auto input=reinterpret_cast<void*>(s_NativeGame+QolNativeProfile::InputObjectRva);
    for(unsigned key=1;key<=0x8000;key<<=1) if(read(input,index,key)) s_NativeRaw|=key;
    s_NativeDelivered=s_NativeRaw;
}
static void ProcessNativeState(unsigned primaryKey=0,bool primaryPressed=false) noexcept {
    const auto now=GetTickCount64();
    s_NativeSnapshot.store((now<<16)|s_NativeRaw,std::memory_order_release);
    const auto raw=QolNativeInput::Decode(s_NativeRaw);
    s_CachedButtons.store(raw.buttons);s_CachedLeftTrigger.store(raw.leftTrigger);
    s_CachedRightTrigger.store(raw.rightTrigger);s_CachedUserIndex.store(s_NativeIndex);s_LastStateTick.store(now);
    XINPUT_STATE state{};
    state.Gamepad.wButtons=raw.buttons;
    state.Gamepad.bLeftTrigger=raw.leftTrigger;state.Gamepad.bRightTrigger=raw.rightTrigger;
    ProcessGamepadShortcuts(s_NativeIndex,&state);
    auto filtered=raw;filtered.buttons=state.Gamepad.wButtons;
    filtered.leftTrigger=state.Gamepad.bLeftTrigger;filtered.rightTrigger=state.Gamepad.bRightTrigger;
    const unsigned desired=QolNativeInput::Encode(filtered);
    QolNativeInput::Deliver(s_NativeDelivered,desired,primaryKey,primaryPressed,
        [](unsigned key,bool pressed) noexcept {s_NativeEventOriginal(key,pressed);});
}
static void __fastcall HookNativeEvent(unsigned key,bool pressed) noexcept {
    if(s_InsideNative || !NativeInputInstalled() || !QolNativeInput::KnownKey(key)) {
        s_NativeEventOriginal(key,pressed);return;
    }
    const unsigned index=NativeActiveIndex();
    if(index>=8) {s_NativeEventOriginal(key,pressed);return;}
    struct Recursion {Recursion(){s_InsideNative=true;} ~Recursion(){s_InsideNative=false;}} recursion;
    bool processed=false;
    s_InputGate.Run([&] {
        std::lock_guard lock(s_NativeMutex);
        SeedNativeTracking(index);
        if(pressed) s_NativeRaw|=key;else s_NativeRaw&=~key;
        ProcessNativeState(key,pressed);processed=true;
    });
    if(!processed) s_NativeEventOriginal(key,pressed);
}
static void __fastcall HookNativeReset(void* controller) noexcept {
    // Device disconnect and the game's own input reset must clear captured keys,
    // including keys QOL consumed and therefore absent from the game held set.
    if(NativeInputInstalled()) {
        std::lock_guard lock(s_NativeMutex);
        if(s_NativeIndex<8 && controller==reinterpret_cast<void*>(s_NativeGame+
            QolNativeProfile::InputObjectRva+s_NativeIndex*0x1c8)) ResetNativeTracking();
    }
    s_NativeResetOriginal(controller);
}
void PumpNativeInput() noexcept {
    if(!NativeInputInstalled() || s_NativeTaskPending.exchange(true)) return;
    const auto result=s_NativeThreads->runOnGameThread(s_NativeContext,
        [](const D2RL::PluginContext*,void*) noexcept {
            s_NativeTaskPending.store(false);
            if(!NativeInputInstalled() || s_InsideNative) return;
            struct Recursion {Recursion(){s_InsideNative=true;} ~Recursion(){s_InsideNative=false;}} recursion;
            s_InputGate.Run([&] {
                std::lock_guard lock(s_NativeMutex);
                const unsigned index=NativeActiveIndex();
                if(index>=8) {ResetNativeTracking();return;}
                SeedNativeTracking(index);
                ProcessNativeState();
            });
        },nullptr);
    if(result!=D2RL::Threads::Result::Success) s_NativeTaskPending.store(false);
}
static bool CheckNativeProfile(uintptr_t game,uintptr_t core) noexcept {
    __try {
        using namespace QolNativeProfile;
        return !std::memcmp(reinterpret_cast<const void*>(game+ButtonRva),ButtonBytes,sizeof(ButtonBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+EventRva),EventBytes,sizeof(EventBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+ResetRva),ResetBytes,sizeof(ResetBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+EventCallerRva),EventCallerBytes,sizeof(EventCallerBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+InputRva),InputBytes,sizeof(InputBytes)) &&
            !std::memcmp(reinterpret_cast<const void*>(game+IndexRva),IndexBytes,sizeof(IndexBytes)) &&
            *reinterpret_cast<const uintptr_t*>(core+0x701bb0)==game+ManagerSlotRva &&
            *reinterpret_cast<const uintptr_t*>(core+0x6fe440)==game+InputRva &&
            *reinterpret_cast<const uintptr_t*>(core+0x7004a8)==game+IndexRva;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool InstallNativeInputHook(const D2RL::PluginContext* context,const D2RL::ThreadService* threads) noexcept {
    if(NativeInputInstalled()) return true;
    if(!context || !threads || !D2RL::HasThreadServiceField(threads,D2RL::ThreadServiceRequiredSize) || !threads->runOnGameThread) return false;
    const auto core=GetModuleHandleW(L"D2RCore.dll");
    if(!QolCore::VerifyFileHash(core,QolNativeProfile::CoreHash) ||
        !CheckNativeProfile(context->exeBase,reinterpret_cast<uintptr_t>(core))) {
        context->LogWarn("[QOL/NativeInput] Profile rejected (core hash, game event/reset/query code or input/index/manager slot). Native input untouched; XInput fallback has limited device coverage.");
        return false;
    }
    s_NativeGame=context->exeBase;s_NativeContext=context;s_NativeThreads=threads;
    // If only Reset installs, it remains an unconditional passthrough while
    // NativeInstalled is false. SDK owns its lifetime; no ad hoc restoration.
    if(!context->InstallInlineHook<NativeResetFn>(QolNativeProfile::ResetRva,
            QolNativeProfile::ResetBytes,14,&HookNativeReset,&s_NativeResetOriginal) ||
        !context->InstallInlineHook<NativeEventFn>(QolNativeProfile::EventRva,
            QolNativeProfile::EventBytes,21,&HookNativeEvent,&s_NativeEventOriginal)) {
        context->LogWarn("[QOL/NativeInput] SDK input hooks incomplete; callbacks pass through and existing owners are untouched.");
        return false;
    }
    s_InputGate.Enable();s_NativeInstalled.store(true);
    context->LogInfo("[QOL/NativeInput] Native normalized controller input active (SDK event hook game+0x13EDD0; reset+0x13DEF0). DualShock/Xbox/Steam use the game's input; no XInput hooks or shared core-slot replacements installed.");
    return true;
}

using XInputGetStateExFn = DWORD(WINAPI*)(DWORD dwUserIndex, void* pState);

static void* s_pTrampolineGetState = nullptr;
static void* s_pTrampolineGetStateEx = nullptr;
static bool s_XInputHooksInstalled = false;

template <size_t Index>
static DWORD WINAPI HookedXInputGetState_N(DWORD dwUserIndex, XINPUT_STATE* pState) {
    auto origFn = reinterpret_cast<XInputGetStateFn>(s_ModuleHooks[Index].getState.trampoline);
    if (!origFn) return ERROR_DEVICE_NOT_CONNECTED;
    DWORD result = origFn(dwUserIndex, pState);
    if (result == ERROR_SUCCESS && pState) {
        s_CachedLeftTrigger.store(pState->Gamepad.bLeftTrigger, std::memory_order_relaxed);
        s_CachedRightTrigger.store(pState->Gamepad.bRightTrigger, std::memory_order_relaxed);
        s_CachedButtons.store(pState->Gamepad.wButtons, std::memory_order_relaxed);
        s_CachedUserIndex.store(dwUserIndex, std::memory_order_relaxed);
        s_LastStateTick.store(GetTickCount64(), std::memory_order_relaxed);

        s_InputGate.Run([&] { ProcessGamepadShortcuts(dwUserIndex, pState); });
    }
    return result;
}

template <size_t Index>
static DWORD WINAPI HookedXInputGetStateEx_N(DWORD dwUserIndex, void* pState) {
    auto origFn = reinterpret_cast<XInputGetStateExFn>(s_ModuleHooks[Index].getStateEx.trampoline);
    if (!origFn) return ERROR_DEVICE_NOT_CONNECTED;
    DWORD result = origFn(dwUserIndex, pState);
    if (result == ERROR_SUCCESS && pState) {
        auto* xiState = reinterpret_cast<XINPUT_STATE*>(pState);
        s_CachedLeftTrigger.store(xiState->Gamepad.bLeftTrigger, std::memory_order_relaxed);
        s_CachedRightTrigger.store(xiState->Gamepad.bRightTrigger, std::memory_order_relaxed);
        s_CachedButtons.store(xiState->Gamepad.wButtons, std::memory_order_relaxed);
        s_CachedUserIndex.store(dwUserIndex, std::memory_order_relaxed);
        s_LastStateTick.store(GetTickCount64(), std::memory_order_relaxed);

        s_InputGate.Run([&] { ProcessGamepadShortcuts(dwUserIndex, xiState); });
    }
    return result;
}

static void* AllocateNear(uint8_t* target, size_t size) noexcept {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const uintptr_t step = si.dwAllocationGranularity ? si.dwAllocationGranularity : 65536;
    const uintptr_t targetAddr = reinterpret_cast<uintptr_t>(target);

    const uintptr_t minAddr = (targetAddr > 0x70000000ULL)
                            ? (targetAddr - 0x70000000ULL)
                            : reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress);
    const uintptr_t maxAddr = (targetAddr + 0x70000000ULL < reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress))
                            ? (targetAddr + 0x70000000ULL)
                            : reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);

    for (uintptr_t addr = (targetAddr & ~(step - 1)) - step; addr >= minAddr; addr -= step) {
        void* p = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (p) return p;
    }
    for (uintptr_t addr = (targetAddr & ~(step - 1)) + step; addr <= maxAddr; addr += step) {
        void* p = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (p) return p;
    }
    return nullptr;
}

static bool InstallSingleHook(uint8_t* targetProc, void* detourProc, HookRecord& hookRec) {
    void* relay=AllocateNear(targetProc,64);
    if(!relay) return false;
    if(QolXInput::Install(targetProc,detourProc,relay,hookRec)) return true;
    VirtualFree(relay,0,MEM_RELEASE); // Never published: no caller can reach this allocation.
    return false;
}

bool InstallXInputHooks() noexcept {
    if (s_XInputHooksInstalled) return true;
    if (s_XInputRetired) return false;

    InitControllerInput();

    auto installForModule = [](size_t index, HMODULE hMod, void* detourGetState, void* detourGetStateEx) {
        if (!hMod) return false;
        auto targetGetState = reinterpret_cast<uint8_t*>(GetProcAddress(hMod, "XInputGetState"));
        auto targetGetStateEx = reinterpret_cast<uint8_t*>(GetProcAddress(hMod, MAKEINTRESOURCEA(100)));

        bool hookedAny = false;
        if (targetGetState) {
            if (InstallSingleHook(targetGetState, detourGetState, s_ModuleHooks[index].getState)) {
                hookedAny = true;
            }
        }
        if (targetGetStateEx) {
            if (InstallSingleHook(targetGetStateEx, detourGetStateEx, s_ModuleHooks[index].getStateEx)) {
                hookedAny = true;
            }
        }
        return hookedAny;
    };

    bool anyInstalled = false;
    if (installForModule(0, s_PhysicalModules[0], reinterpret_cast<void*>(&HookedXInputGetState_N<0>), reinterpret_cast<void*>(&HookedXInputGetStateEx_N<0>))) {
        anyInstalled = true;
    }
    if (installForModule(1, s_PhysicalModules[1], reinterpret_cast<void*>(&HookedXInputGetState_N<1>), reinterpret_cast<void*>(&HookedXInputGetStateEx_N<1>))) {
        anyInstalled = true;
    }
    if (installForModule(2, s_PhysicalModules[2], reinterpret_cast<void*>(&HookedXInputGetState_N<2>), reinterpret_cast<void*>(&HookedXInputGetStateEx_N<2>))) {
        anyInstalled = true;
    }

    if (s_ModuleHooks[0].getState.installed && s_ModuleHooks[0].getState.trampoline) {
        s_pTrampolineGetState = s_ModuleHooks[0].getState.trampoline;
        s_pfnXInputGetState = reinterpret_cast<XInputGetStateFn>(s_pTrampolineGetState);
    }

    std::snprintf(s_XInputReport,sizeof(s_XInputReport),
        "[QOL/XInput] 1_4 state=%s ex=%s; 1_3 state=%s ex=%s; 9_1_0 state=%s. Rejected entries: alignment/prologue, unpinnable predecessor, protection/allocation or concurrent ownership change. Published hooks use process-lifetime pass-through shutdown.",
        s_ModuleHooks[0].getState.installed?"active":"unavailable/rejected",s_ModuleHooks[0].getStateEx.installed?"active":"unavailable/rejected",
        s_ModuleHooks[1].getState.installed?"active":"unavailable/rejected",s_ModuleHooks[1].getStateEx.installed?"active":"unavailable/rejected",
        s_ModuleHooks[2].getState.installed?"active":"unavailable/rejected");
    if(anyInstalled) s_InputGate.Enable();
    s_XInputHooksInstalled = anyInstalled;
    return s_XInputHooksInstalled;
}

void UninstallXInputHooks() noexcept {
    s_InputGate.Disable();
    s_NativeInstalled.store(false);
    s_NativeSnapshot.store(0);
    if (!s_XInputHooksInstalled) return;

    // Drain QOL processing; retained detours subsequently call only their predecessor.
    s_InputGate.Disable();
    s_XInputRetired=true;

    s_pTrampolineGetState = nullptr;
    s_pTrampolineGetStateEx = nullptr;
    if (s_hXInput) {
        s_pfnXInputGetState = reinterpret_cast<XInputGetStateFn>(
            GetProcAddress(s_hXInput, "XInputGetState")
        );
    }
    s_XInputHooksInstalled = false;
}

// ---------------------------------------------------------------------------
// Native D2RCore Controller Subsystem Bridge
// ---------------------------------------------------------------------------
using GetActionStateByNameFn = int(__fastcall*)(const char* actionName, uint32_t* outState);
using IsActionActiveFn = bool(__fastcall*)(const char* actionName, uint8_t mode);

static HMODULE s_hD2RCore = nullptr;
static void** s_ppControllerManager = nullptr;
static GetActionStateByNameFn s_pfnGetActionStateByName = nullptr;
static IsActionActiveFn s_pfnIsActionActive = nullptr;
static std::once_flag s_NativeBridgeOnce;

using SteamAPI_SteamInput_v006_Fn = void*(*)();
using SteamAPI_ISteamInput_GetConnectedControllers_Fn = int(*)(void* self, uint64_t* handlesOut);

static HMODULE s_hSteamApi = nullptr;
static SteamAPI_SteamInput_v006_Fn s_pfnSteamInput_v006 = nullptr;
static SteamAPI_ISteamInput_GetConnectedControllers_Fn s_pfnSteamInput_GetConnectedControllers = nullptr;
static void* s_pSteamInput = nullptr;
static std::atomic<bool> s_SteamBridgeAttempted{false};

static void EnsureNativeBridgeInitialized() noexcept {
    // Called from polling and XInput callbacks. Publish all pointers together,
    // only after exact-build admission. Unsupported builds use physical XInput.
    std::call_once(s_NativeBridgeOnce, [] {
        auto module = GetModuleHandleW(L"D2RCore.dll");
        if (!QolCore::VerifyCore(module)) return;
        const uintptr_t base = reinterpret_cast<uintptr_t>(module);
        s_ppControllerManager = reinterpret_cast<void**>(base + 0x67F4D0);
        s_pfnGetActionStateByName = reinterpret_cast<GetActionStateByNameFn>(base + 0x456440);
        s_pfnIsActionActive = reinterpret_cast<IsActionActiveFn>(base + 0x4542F0);
        s_hD2RCore = module;
    });
}

static void EnsureSteamBridgeInitialized() noexcept {
    if (s_SteamBridgeAttempted.load(std::memory_order_relaxed)) {
        return;
    }

    s_hSteamApi = GetModuleHandleA("steam_api64.dll");
    if (!s_hSteamApi) {
        s_hSteamApi = LoadLibraryA("steam_api64.dll");
    }
    if (s_hSteamApi) {
        s_pfnSteamInput_v006 = reinterpret_cast<SteamAPI_SteamInput_v006_Fn>(
            GetProcAddress(s_hSteamApi, "SteamAPI_SteamInput_v006")
        );
        s_pfnSteamInput_GetConnectedControllers = reinterpret_cast<SteamAPI_ISteamInput_GetConnectedControllers_Fn>(
            GetProcAddress(s_hSteamApi, "SteamAPI_ISteamInput_GetConnectedControllers")
        );
        if (s_pfnSteamInput_v006) {
            __try {
                s_pSteamInput = s_pfnSteamInput_v006();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                s_pSteamInput = nullptr;
            }
        }
        s_SteamBridgeAttempted.store(true, std::memory_order_relaxed);
    }
}

NativeBridgeDiag QueryNativeBridgeDiagnostics() noexcept {
    NativeBridgeDiag diag{};
    if(NativeInputInstalled()) {
        const auto input=ReadControllerInput();
        diag.d2rCoreFound=true;diag.mgrFound=input.valid;diag.controllerMode=input.valid?1:0;
        diag.isAltActive=input.leftTrigger!=0;
        return diag;
    }
    EnsureNativeBridgeInitialized();

    if (s_hD2RCore) {
        diag.d2rCoreFound = true;
        __try {
            if (s_ppControllerManager && *s_ppControllerManager) {
                diag.mgrFound = true;
                const uint8_t* pMgr = reinterpret_cast<const uint8_t*>(*s_ppControllerManager);
                diag.controllerMode = *reinterpret_cast<const uint32_t*>(pMgr + 0xDC);

                if (s_pfnGetActionStateByName) {
                    uint32_t holdState = 0;
                    diag.altHoldRes = s_pfnGetActionStateByName("ControllerAltHold", &holdState);
                    diag.altHoldState = holdState;

                    uint32_t pressState = 0;
                    diag.altPressRes = s_pfnGetActionStateByName("ControllerAltPress", &pressState);
                    diag.altPressState = pressState;
                }

                if (s_pfnIsActionActive) {
                    diag.isAltActive = s_pfnIsActionActive("ControllerAltHold", 1);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    EnsureSteamBridgeInitialized();
    if (s_pSteamInput && s_pfnSteamInput_GetConnectedControllers) {
        diag.steamFound = true;
        __try {
            uint64_t handles[16]{};
            diag.steamControllers = s_pfnSteamInput_GetConnectedControllers(s_pSteamInput, handles);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            diag.steamControllers = -1;
        }
    }

    return diag;
}

bool IsNativeAltModifierActive() noexcept {
    if(NativeInputInstalled()) return ReadControllerInput().leftTrigger!=0;
    EnsureNativeBridgeInitialized();
    if (!s_hD2RCore || !s_ppControllerManager) {
        return false;
    }

    __try {
        if (!*s_ppControllerManager) {
            return false;
        }

        // Check 1: IsActionActive with mode 1
        if (s_pfnIsActionActive) {
            if (s_pfnIsActionActive("ControllerAltHold", 1)) {
                return true;
            }
        }

        // Check 2: GetActionStateByName
        // Returns 0 on success, outState == 2 means ACTIVE / HELD!
        if (s_pfnGetActionStateByName) {
            uint32_t state = 0;
            if (s_pfnGetActionStateByName("ControllerAltHold", &state) == 0) {
                if (state == 2) {
                    return true;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return false;
}

} // namespace ControllerQoL

