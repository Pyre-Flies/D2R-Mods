#include "input_hook.h"
#include "identify_handler.h"
#include "transfer_handler.h"
#include <iostream>

namespace D2R {

bool InputHook::s_installed = false;
bool InputHook::s_leftTriggerHeld = false;
WORD InputHook::s_prevButtons = 0;
BYTE InputHook::s_prevLeftTrigger = 0;
decltype(&XInputGetState) InputHook::s_originalXInputGetState = nullptr;

constexpr BYTE TRIGGER_THRESHOLD = 30;

bool InputHook::Initialize() {
    if (s_installed) return true;

    // Dynamically locate XInput module
    HMODULE hXInput = GetModuleHandleA("xinput1_4.dll");
    if (!hXInput) hXInput = GetModuleHandleA("xinput1_3.dll");
    if (!hXInput) hXInput = GetModuleHandleA("xinput9_1_0.dll");

    if (hXInput) {
        s_originalXInputGetState = reinterpret_cast<decltype(&XInputGetState)>(
            GetProcAddress(hXInput, "XInputGetState")
        );
    }

    if (!s_originalXInputGetState) {
        // Fallback: direct link
        s_originalXInputGetState = &XInputGetState;
    }

    s_installed = true;
    OutputDebugStringA("[controller-qol] InputHook initialized successfully.\n");
    return true;
}

void InputHook::Shutdown() {
    s_installed = false;
}

bool InputHook::IsLeftTriggerHeld() {
    return s_leftTriggerHeld;
}

DWORD WINAPI InputHook::HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    DWORD result = ERROR_SUCCESS;
    if (s_originalXInputGetState) {
        result = s_originalXInputGetState(dwUserIndex, pState);
    }

    if (result != ERROR_SUCCESS || !pState) {
        return result;
    }

    XINPUT_GAMEPAD& pad = pState->Gamepad;
    s_leftTriggerHeld = (pad.bLeftTrigger > TRIGGER_THRESHOLD);

    // Compute newly pressed buttons on this frame
    WORD pressedThisFrame = pad.wButtons & ~s_prevButtons;

    // --- PD2-Style Shortcut Chords (While Left Trigger is held) ---
    if (s_leftTriggerHeld) {
        // 1. LT + A: Instant Quick Identify
        if (pressedThisFrame & XINPUT_GAMEPAD_A) {
            if (IdentifyHandler::HandleIdentifyAction()) {
                // Suppress A button so game does not execute default pickup/move
                pad.wButtons &= ~XINPUT_GAMEPAD_A;
            }
        }

        // 2. LT + X: Quick Transfer (Move to Stash / Cube)
        if (pressedThisFrame & XINPUT_GAMEPAD_X) {
            if (TransferHandler::HandleTransferAction()) {
                pad.wButtons &= ~XINPUT_GAMEPAD_X;
            }
        }

        // 3. LT + Y: Quick Drop
        if (pressedThisFrame & XINPUT_GAMEPAD_Y) {
            if (TransferHandler::HandleDropAction()) {
                pad.wButtons &= ~XINPUT_GAMEPAD_Y;
            }
        }

        // 4. LT + D-Pad: Feed Mercenary from Belt Slots 1-4
        if (pressedThisFrame & XINPUT_GAMEPAD_DPAD_LEFT) {
            if (TransferHandler::HandleMercenaryPotion(0)) pad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
        } else if (pressedThisFrame & XINPUT_GAMEPAD_DPAD_UP) {
            if (TransferHandler::HandleMercenaryPotion(1)) pad.wButtons &= ~XINPUT_GAMEPAD_DPAD_UP;
        } else if (pressedThisFrame & XINPUT_GAMEPAD_DPAD_DOWN) {
            if (TransferHandler::HandleMercenaryPotion(2)) pad.wButtons &= ~XINPUT_GAMEPAD_DPAD_DOWN;
        } else if (pressedThisFrame & XINPUT_GAMEPAD_DPAD_RIGHT) {
            if (TransferHandler::HandleMercenaryPotion(3)) pad.wButtons &= ~XINPUT_GAMEPAD_DPAD_RIGHT;
        }
    }

    // 5. Right Stick Click (R3): Auto-Restock Belt from Inventory
    if (pressedThisFrame & XINPUT_GAMEPAD_RIGHT_THUMB) {
        if (TransferHandler::HandleBeltRestock()) {
            pad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
        }
    }

    s_prevButtons = pState->Gamepad.wButtons;
    s_prevLeftTrigger = pad.bLeftTrigger;

    return result;
}

} // namespace D2R
