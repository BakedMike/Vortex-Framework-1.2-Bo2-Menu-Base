#include "stdafx.h"

static Detour<void> R_EndFrame_Detour;
static void R_EndFrame_Hook()
{
    RunMenuFrame();
    R_EndFrame_Detour.CallOriginal();
}

static Detour<DWORD> XamInputGetState_Detour;
static volatile LONG gInputLatched = 0;

static DWORD XamInputGetState_Hook(DWORD userIndex, DWORD flags, XINPUT_STATE* state)
{
    DWORD result = XamInputGetState_Detour.CallOriginal(userIndex, flags, state);
    if (userIndex != 0 || result != ERROR_SUCCESS || state == NULL)
        return result;

    WORD buttons = state->Gamepad.wButtons;

    gRawPadPacket = state->dwPacketNumber;
    gRawPadLeftTrigger = state->Gamepad.bLeftTrigger;
    gRawPadRightTrigger = state->Gamepad.bRightTrigger;
    gRawPadThumbLeftX = state->Gamepad.sThumbLX;
    gRawPadThumbLeftY = state->Gamepad.sThumbLY;
    gRawPadThumbRightX = state->Gamepad.sThumbRX;
    gRawPadThumbRightY = state->Gamepad.sThumbRY;
    gRawPadButtons = buttons;
    gRawPadTick = GetTickCount();
    gRawPadValid = 1;

    const WORD openChord = XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_DPAD_DOWN;
    bool bChordHeld = (buttons & openChord) == openChord;

    if (gMenuBlocksInput || bChordHeld)
        gInputLatched = 1;

    if (gInputLatched)
    {
        if (gMenuBlocksInput || bChordHeld || buttons != 0)
        {
            state->Gamepad.wButtons = 0;
            state->Gamepad.bLeftTrigger = 0;
            state->Gamepad.bRightTrigger = 0;
        }
        else
            gInputLatched = 0;
    }
    return result;
}

static DWORD WINAPI InitThread(LPVOID param)
{
    // Wait until the default_mp addresses are valid
    for (;;)
    {
        if (MmIsAddressValid((PVOID)0x828B9F58) && *(volatile DWORD*)0x828B9F58 == 0x7D8802A6)
            break;
        Sleep(500);
    }
    Sleep(500);

    if (!R_EndFrame_Detour.SetupDetour(0x828B9F58, (PVOID)R_EndFrame_Hook))
        DbgPrint("Menu Base: failed to hook R_EndFrame");

    if (!XamInputGetState_Detour.SetupDetour(0x8293D884, (PVOID)XamInputGetState_Hook))
        DbgPrint("Menu Base: failed to hook XamInputGetState");

    return 0;
}

BOOL APIENTRY DllMain(HANDLE handle, DWORD reason, PVOID reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH: 
        {
            HANDLE threadHandle = 0;
            DWORD threadId = 0;

            ExCreateThread(&threadHandle, 0, &threadId, (PVOID)XapiThreadStartup, (LPTHREAD_START_ROUTINE)InitThread, NULL, 0x2 | CREATE_SUSPENDED);

            XSetThreadProcessor(threadHandle, 4);
            SetThreadPriority(threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);

            ResumeThread(threadHandle);
            CloseHandle(threadHandle);
            break;
        }


        case DLL_PROCESS_DETACH:
        {
            ShutdownFunctions();

            if (R_EndFrame_Detour.IsDetoured())
                R_EndFrame_Detour.TakeDownDetour();

            if (XamInputGetState_Detour.IsDetoured())
                XamInputGetState_Detour.TakeDownDetour();
            break;
        }
    }
    return TRUE;
}
