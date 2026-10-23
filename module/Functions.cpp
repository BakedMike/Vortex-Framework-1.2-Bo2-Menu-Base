#include "stdafx.h"

bool bRedboxes = false;
bool bLaser = false;
bool bChams = false;
bool bNoRecoil = false;

// Custom FOV
bool bCustomFov = false;
float fCustomFov = 90.0f;

// TODO: No-Sway 0x826C6E6C
// TODO: No-Spread 0x826BB50C
// TODO: Class Item Count 0x826A5FBC

static void* pFovDvar = NULL;
static float fFovOriginal = 0.0f;
static float fFovLast = 0.0f;
static bool bFovActive = false;

void Redboxes(bool value)
{
    *(DWORD*)0x821F5B7C = value ? 0x38600001 : 0x38600000;
}

void Laser(bool value)
{
    *(DWORD*)0x82255E1C = value ? 0x2B0B0001 : 0x2B0B0000;
}

void Chams(bool value)
{
    *(DWORD*)0x821FC04C = value ? 0x38C0FFFF : 0x7FA6EB78;
}

void NoRecoil(bool value)
{
    *(DWORD*)0x82259BC8 = value ? 0x60000000 : 0x48461341;
}

// Applies the slider value, or puts the original back when off
void UpdateCustomFov()
{
    if (*(volatile int*)0x82C70F44 <= 0) // local client count
        return;

    if (!bCustomFov)
    {
        if (bFovActive)
        {
            if (MmIsAddressValid(pFovDvar))
                Dvar_SetFloat(pFovDvar, fFovOriginal);
            bFovActive = false;
        }
        return;
    }

    if (pFovDvar == NULL)
    {
        void* dvar = Dvar_FindVar("cg_fov");
        if (dvar == NULL || !MmIsAddressValid(dvar))
            return;
        pFovDvar = dvar;
    }

    float wanted = fCustomFov;
    if (wanted < 65.0f) wanted = 65.0f;
    if (wanted > 120.0f) wanted = 120.0f;

    if (bFovActive && wanted == fFovLast)
        return;
    if (!bFovActive)
        fFovOriginal = Dvar_GetFloat(pFovDvar);

    Dvar_SetFloat(pFovDvar, wanted);
    fFovLast = wanted;
    bFovActive = true;
}

// Toggle callback, bCustomFov is already flipped
void CustomFov(bool value)
{
    UpdateCustomFov();
}

void ResetCustomFov()
{
    fCustomFov = 90.0f;
}

// Called every frame
void UpdateFunctions()
{
    UpdateCustomFov();
}

// Called on detach
void ShutdownFunctions()
{
    Redboxes(false);
    Laser(false);
    Chams(false);
    NoRecoil(false);
    bCustomFov = false;
    UpdateCustomFov();
}

void PrintInfo(int clientIndex)
{
    GamePlayers players;
    if (!ReadGamePlayers(&players))
        return;

    for (int i = 0; i < players.count; i++)
    {
        const GamePlayer& player = players.list[i];
        if (player.slot != clientIndex)
            continue;

        DbgPrint("Player %d: name=%s team=%d host=%d\n", player.slot, player.name, player.team, player.slot == players.hostSlot);
        return;
    }
}

// Aimbot
int  TargetBone = 0;
int  ShieldTargetBone = 0;
bool bAutoBone = true;
bool bPreferVisibleBones = true;
bool bFallbackToChest = false;
bool bSwitchOnLowHealth = false;
int  BonePriority = 0;
bool bDrawTargetLine = false;
bool bHitMarkerFlash = false;
int  HighlightColor = 0;
bool bAutoShoot = false;
int  AutoCrouchMode = 0;
bool bAutoAds = false;
bool bJumpMacro = false;

// Anti-Aim
bool bAntiAimEnable = false;
int  AntiAimPitch = 0;
int  AntiAimYaw = 0;

// Visual
bool bEspEnable = false;
int  EspTargets = 0;
int  EspBoxStyle = 0;
int  EspSnaplineStyle = 0;
bool bEspName = false;
bool bEspRank = false;
bool bEspHealthBar = false;
bool bEspCompass = false;
bool bEspWeaponIcon = false;
bool bEspWeaponName = false;
bool bEspViewAngles = false;

// Misc
bool bShowFps = false;
bool bShowServerInfo = true;

// Settings tab
float MenuWidth = kDefaultMenuWidth;
float MenuScale = 1.0f;
float PositionX = 0.0f;
float PositionY = 0.0f;
float HeaderHeight = kDefaultHeaderHeight;
float TabsHeight = kDefaultTabsHeight;
float OptionSpacing = kDefaultOptionSpacing;
float ContentPadding = kDefaultContentPadding;
bool  bAnimations = true;
float AnimationSpeed = 1.0f;
float ScrollSpeed = 5.0f;
float FadeTime = 0.2f;
int   MenuColor = 0;
int   MenuOpacity = 0;

// Submenu groups
bool bAutoBoneConfigOpen = false;
bool bWeaponVisualOpen = false;
