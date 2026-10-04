#pragma once

#include <cstdint>

namespace D2RL { struct PluginContext; struct ThreadService; }
namespace ControllerQoL {
bool InstallNativeInputHook(const D2RL::PluginContext* context,const D2RL::ThreadService* threads) noexcept;
bool NativeInputInstalled() noexcept;
// Monotonic R3 press sequence from the existing admitted native input path.
bool NativeRightStickPresses(uint64_t& sequence) noexcept;
void PumpNativeInput() noexcept;

void InitControllerInput() noexcept;

void SetActiveModifier(const char* mode, uint8_t triggerThreshold) noexcept;
uint64_t GetHookCallCount() noexcept;
void GetCachedControllerState(uint8_t& outLT, uint8_t& outRT, uint16_t& outButtons, uint64_t& outAgeMs) noexcept;

// Returns the highest analog value of Left Trigger across all connected controllers (0 - 255)
uint8_t GetMaxLeftTriggerValue() noexcept;

// Checks if Left Trigger (LT / L2) is pulled past the threshold
bool IsLeftTriggerPressed(uint8_t triggerThreshold = 30) noexcept;

// Returns the highest analog value of Right Trigger across all connected controllers (0 - 255)
uint8_t GetMaxRightTriggerValue() noexcept;

// Checks if Right Trigger (RT / R2) is pulled past the threshold
bool IsRightTriggerPressed(uint8_t triggerThreshold = 30) noexcept;

// Checks if Left Bumper (LB / L1) is pressed
bool IsLeftBumperPressed() noexcept;

// Checks modifier according to mode: "trigger", "bumper", or "any"
bool IsControllerModifierActive(const char* mode, uint8_t triggerThreshold = 30) noexcept;

// Check specific face buttons (XInput button flags e.g. A, X, Y, B)
bool IsControllerButtonPressed(uint16_t buttonMask) noexcept;
// Current native UI mode, not controller connectivity or a cached button state.
// Unknown/unavailable native mode returns false (hide controller-only hints).
bool IsControllerUiActive() noexcept;

bool IsButtonAPressed() noexcept;
bool IsButtonBPressed() noexcept;
bool IsButtonXPressed() noexcept;
bool IsButtonYPressed() noexcept;
bool IsRightBumperPressed() noexcept;

// Check if the configured ground pickup button is pressed
bool IsGroundPickupActive(const char* button) noexcept;

bool IsAnyControllerConnected() noexcept;

using TriggerPassThroughPredicate = bool(*)() noexcept;
void SetTriggerPassThroughPredicate(TriggerPassThroughPredicate predicate) noexcept;

using QuickMoveCallback = void(*)();
void SetQuickMoveCallback(QuickMoveCallback callback) noexcept;

using QuickMoveCubeCallback = void(*)();
void SetQuickMoveCubeCallback(QuickMoveCubeCallback callback) noexcept;

using BulkStashCallback = bool(*)() noexcept;
void SetBulkStashCallback(BulkStashCallback callback) noexcept;

using AutoFillBeltCallback = void(*)();
void SetAutoFillBeltCallback(AutoFillBeltCallback callback) noexcept;


void TriggerSyntheticHold(uint16_t button, uint32_t maxDurationMs = 650) noexcept;
void CancelSyntheticHold() noexcept;
bool IsSyntheticHoldActive() noexcept;

using VendorContextPredicate = bool(*)() noexcept;
void SetVendorContextPredicate(VendorContextPredicate pred) noexcept;

bool InstallXInputHooks() noexcept;
const char* GetXInputHookReport() noexcept;
void UninstallXInputHooks() noexcept;

struct NativeBridgeDiag {
    bool d2rCoreFound = false;
    bool mgrFound = false;
    uint32_t controllerMode = 0;
    int altHoldRes = -1;
    uint32_t altHoldState = 0;
    int altPressRes = -1;
    uint32_t altPressState = 0;
    bool isAltActive = false;
    bool steamFound = false;
    int steamControllers = -1;
};

NativeBridgeDiag QueryNativeBridgeDiagnostics() noexcept;
bool IsNativeAltModifierActive() noexcept;

} // namespace ControllerQoL


