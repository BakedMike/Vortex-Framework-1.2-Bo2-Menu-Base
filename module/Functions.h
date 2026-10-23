#pragma once

// Patches
extern bool bRedboxes;
extern bool bLaser;
extern bool bChams;
extern bool bNoRecoil;

// Custom FOV
extern bool bCustomFov;
extern float fCustomFov;

void UpdateFunctions();
void ShutdownFunctions();

void Redboxes(bool value);
void Laser(bool value);
void Chams(bool value);
void NoRecoil(bool value);

void CustomFov(bool value);
void UpdateCustomFov();
void ResetCustomFov();

// Per-player actions for the player options page. How to add your own is explained in Functions.cpp.

// Logs name, slot, team and host flag of the player to the debug output
void PrintInfo(int clientIndex);

// Menu settings: plain global option state. Menu items point at these variables.

// Slider defaults that the menu layout math is based on
const float kDefaultMenuWidth = 620.0f;
const float kDefaultHeaderHeight = 68.0f;
const float kDefaultTabsHeight = 36.0f;
const float kDefaultOptionSpacing = 4.0f;
const float kDefaultContentPadding = 20.0f;
const float kAnimationSpeedMin = 0.1f;
const float kAnimationSpeedMax = 3.0f;

// Aimbot
extern int  TargetBone;
extern int  ShieldTargetBone;
extern bool bAutoBone;
extern bool bPreferVisibleBones;
extern bool bFallbackToChest;
extern bool bSwitchOnLowHealth;
extern int  BonePriority;
extern bool bDrawTargetLine;
extern bool bHitMarkerFlash;
extern int  HighlightColor;
extern bool bAutoShoot;
extern int  AutoCrouchMode;
extern bool bAutoAds;
extern bool bJumpMacro;

// Anti-Aim
extern bool bAntiAimEnable;
extern int  AntiAimPitch;
extern int  AntiAimYaw;

// Visual
extern bool bEspEnable;
extern int  EspTargets;
extern int  EspBoxStyle;
extern int  EspSnaplineStyle;
extern bool bEspName;
extern bool bEspRank;
extern bool bEspHealthBar;
extern bool bEspCompass;
extern bool bEspWeaponIcon;
extern bool bEspWeaponName;
extern bool bEspViewAngles;

// Misc
extern bool bShowFps;
extern bool bShowServerInfo;

// Settings tab
extern float MenuWidth;
extern float MenuScale;
extern float PositionX;
extern float PositionY;
extern float HeaderHeight;
extern float TabsHeight;
extern float OptionSpacing;
extern float ContentPadding;
extern bool  bAnimations;
extern float AnimationSpeed;
extern float ScrollSpeed;
extern float FadeTime;
extern int   MenuColor;
extern int   MenuOpacity;

// Collapsed/expanded state of the submenu groups
extern bool bAutoBoneConfigOpen;
extern bool bWeaponVisualOpen;
