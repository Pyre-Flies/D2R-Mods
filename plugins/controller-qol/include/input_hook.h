#pragma once
#include <windows.h>
#include <xinput.h>

namespace D2R {

class InputHook {
public:
    static bool Initialize();
    static void Shutdown();

    static bool IsLeftTriggerHeld();
    static DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

private:
    static bool s_installed;
    static bool s_leftTriggerHeld;
    static WORD s_prevButtons;
    static BYTE s_prevLeftTrigger;
    static decltype(&XInputGetState) s_originalXInputGetState;
};

} // namespace D2R
