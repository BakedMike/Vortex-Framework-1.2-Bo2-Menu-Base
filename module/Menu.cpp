#include "stdafx.h"


const char* const kTabNames[TAB_COUNT] = { "Aimbot", "Anti-Aim", "Visual", "Misc", "Players", "Settings" };

const char* const kBoneOptions[]       = { "Head", "Neck", "Chest", "Stomach", "Pelvis", "Arms", "Legs", "Feet" };
const char* const kPriorityOptions[]   = { "Head First", "Closest", "Random" };
const char* const kHighlightOptions[]  = { "Red", "Green", "Blue", "White" };
const char* const kCrouchOptions[]     = { "Off", "Stand", "Crouch", "Prone" };
const char* const kPitchOptions[]      = { "Off", "Up", "Down", "Jitter" };
const char* const kYawOptions[]        = { "Off", "Spin", "Jitter", "Backwards" };
const char* const kTargetOptions[]     = { "Off", "Enemies Only", "Allies Only", "Both Teams" };
const char* const kBoxStyleOptions[]   = { "2D", "Corners", "3D" };
const char* const kSnaplineOptions[]   = { "Bottom", "Center", "Top" };
const char* const kColorOptions[]      = { "Red", "Blue", "Green", "Purple", "Orange" };
const char* const kOpacityOptions[]    = { "95%", "80%", "65%", "45%" };

static char gPlayersTitle[48];
static char gPlayerInfoLines[5][48];

static PlayerRelation GetPlayerRelation(const GamePlayers& players, const GamePlayer& player)
{
    if (player.slot == players.localSlot)
        return RELATION_YOU;
    if (player.team == 9) // 9 = TEAM_SPECTATOR
        return RELATION_NEUTRAL;
    if (!players.bTeamBased)
        return RELATION_ENEMY;
    if (players.localTeam != 1 && players.localTeam != 2) // 1 = TEAM_ALLIES, 2 = TEAM_AXIS
        return RELATION_NEUTRAL;
    return (player.team == players.localTeam) ? RELATION_FRIENDLY : RELATION_ENEMY;
}

static const GamePlayer* FindPlayer(const GamePlayers& players, int slot)
{
    for (int iter = 0; iter < players.count; iter++)
        if (players.list[iter].slot == slot)
            return &players.list[iter];
    return NULL;
}

static const char* TeamName(int team)
{
    switch (team)
    {
        case 0: return "Free"; // TEAM_FREE
        case 1: return "Allies"; // TEAM_ALLIES
        case 2: return "Axis"; // TEAM_AXIS
        case 9: return "Spectator"; // TEAM_SPECTATOR (3..8 are TEAM_THREE..TEAM_EIGHT)
    }
    return "Unknown";
}

static const char* RelationName(PlayerRelation relation)
{
    switch (relation)
    {
        case RELATION_YOU:      return "You";
        case RELATION_FRIENDLY: return "Friendly";
        case RELATION_ENEMY:    return "Enemy";
    }
    return "Neutral";
}

static void AddInfoLine(int line, const char* name, const char* value)
{
    _snprintf(gPlayerInfoLines[line], sizeof(gPlayerInfoLines[line]), "%s: %s", name, value);
    gPlayerInfoLines[line][sizeof(gPlayerInfoLines[line]) - 1] = 0;
    AddInfoLabel(gPlayerInfoLines[line]);
}

const char* TabName(int tab)
{
    return kTabNames[tab];
}

void BuildTab(int tab)
{
    switch (tab)
    {
        case TAB_AIMBOT:
        {
            AddSection("Bone Targeting");
            AddDropdown("Target Bone", &TargetBone, kBoneOptions, (int)(sizeof(kBoneOptions) / sizeof(kBoneOptions[0])));
            AddDropdown("Shield Target Bone", &ShieldTargetBone, kBoneOptions, (int)(sizeof(kBoneOptions) / sizeof(kBoneOptions[0])));
            AddSeparator();
            AddToggleOption("Enable AutoBone", &bAutoBone);
            AddSubmenu("AutoBone Configuration", &bAutoBoneConfigOpen);
            AddToggleOption("Prefer Visible Bones", &bPreferVisibleBones);
            AddToggleOption("Fallback To Chest", &bFallbackToChest);
            AddToggleOption("Switch On Low Health", &bSwitchOnLowHealth);
            AddDropdown("Bone Priority", &BonePriority, kPriorityOptions, (int)(sizeof(kPriorityOptions) / sizeof(kPriorityOptions[0])));
            EndSubmenu();
            AddSection("Weapon & Visual");
            AddSubmenu("Weapon|Visual Effects", &bWeaponVisualOpen);
            AddToggleOption("Draw Target Line", &bDrawTargetLine);
            AddToggleOption("Hit Marker Flash", &bHitMarkerFlash);
            AddDropdown("Highlight Color", &HighlightColor, kHighlightOptions, (int)(sizeof(kHighlightOptions) / sizeof(kHighlightOptions[0])));
            EndSubmenu();
            AddSection("Automation Settings");
            AddToggleOption("Auto Shoot", &bAutoShoot);
            AddDropdown("Auto Crouch Mode", &AutoCrouchMode, kCrouchOptions, (int)(sizeof(kCrouchOptions) / sizeof(kCrouchOptions[0])));
            AddToggleOption("Auto ADS", &bAutoAds);
            AddToggleOption("Jump Macro", &bJumpMacro);
            AddSection("Example Feature");
            AddToggleOption("Custom FOV", &bCustomFov, CustomFov);
            AddSliderBar("FOV Value", &fCustomFov, 65.0f, 120.0f, 1.0f, 0);
            AddButton("Reset FOV", &ResetCustomFov);
            break;
        }

        case TAB_ANTIAIM:
        {
            AddSection("Anti-Aim");
            AddToggleOption("Enable", &bAntiAimEnable);
            AddDropdown("Pitch", &AntiAimPitch, kPitchOptions, (int)(sizeof(kPitchOptions) / sizeof(kPitchOptions[0])));
            AddDropdown("Yaw", &AntiAimYaw, kYawOptions, (int)(sizeof(kYawOptions) / sizeof(kYawOptions[0])));
            break;
        }

        case TAB_VISUAL:
        {
            AddSection("ESP Settings");
            AddToggleOption("Enable ESP", &bEspEnable);
            AddDropdown("Show Targets", &EspTargets, kTargetOptions, (int)(sizeof(kTargetOptions) / sizeof(kTargetOptions[0])));
            AddDropdown("Box Style", &EspBoxStyle, kBoxStyleOptions, (int)(sizeof(kBoxStyleOptions) / sizeof(kBoxStyleOptions[0])));
            AddDropdown("Snapline Style", &EspSnaplineStyle, kSnaplineOptions, (int)(sizeof(kSnaplineOptions) / sizeof(kSnaplineOptions[0])));
            AddSection("Components");
            AddToggleOption("Name", &bEspName);
            AddToggleOption("Rank", &bEspRank);
            AddToggleOption("Health Bar", &bEspHealthBar);
            AddToggleOption("Compass", &bEspCompass);
            AddToggleOption("Weapon Icon", &bEspWeaponIcon);
            AddToggleOption("Weapon Name", &bEspWeaponName);
            AddToggleOption("View Angles", &bEspViewAngles);
            break;
        }

        case TAB_MISC:
        {
            AddSection("Display");
            AddToggleOption("Show FPS", &bShowFps);
            AddToggleOption("Show Server Info", &bShowServerInfo);
            AddSection("Patches");
            AddToggleOption("Redboxes", &bRedboxes, Redboxes);
            AddToggleOption("Laser", &bLaser, Laser);
            AddToggleOption("Chams", &bChams, Chams);
            AddToggleOption("No Recoil", &bNoRecoil, NoRecoil);
            break;
        }

        case TAB_PLAYERS:
        {
            GamePlayers players;
            bool bReadable = ReadGamePlayers(&players);
            int maxClients = GetGameMaxClients();

            if (bReadable && maxClients > 0)
                _snprintf(gPlayersTitle, sizeof(gPlayersTitle), "Players (%d/%d)", players.count, maxClients);
            else if (bReadable)
                _snprintf(gPlayersTitle, sizeof(gPlayersTitle), "Players (%d)", players.count);
            else
                _snprintf(gPlayersTitle, sizeof(gPlayersTitle), "Players");
            gPlayersTitle[sizeof(gPlayersTitle) - 1] = 0;
            AddSection(gPlayersTitle);

            if (!bReadable || players.count == 0)
            {
                AddInfoLabel("No players");
                return;
            }
            for (int iter = 0; iter < players.count; iter++)
            {
                const GamePlayer& player = players.list[iter];
                AddPlayerRow(player.name, player.slot == players.hostSlot, GetPlayerRelation(players, player), player.slot);
            }
            break;
        }

        case TAB_SETTINGS:
        {
            AddSection("Menu Appearance");
            AddSliderBar("Menu Width", &MenuWidth, 300.0f, 700.0f, 10.0f, 2);
            AddSliderBar("Menu Scale", &MenuScale, 0.70f, 1.50f, 0.05f, 2);
            AddSliderBar("Position X Offset", &PositionX, -300.0f, 300.0f, 5.0f, 2);
            AddSliderBar("Position Y Offset", &PositionY, -300.0f, 300.0f, 5.0f, 2);
            AddSeparator(true);
            AddSection("Layout Settings");
            AddSliderBar("Header Height", &HeaderHeight, 40.0f, 120.0f, 2.5f, 2);
            AddSliderBar("Tabs Height", &TabsHeight, 24.0f, 50.0f, 0.5f, 2);
            AddSliderBar("Option Spacing", &OptionSpacing, 0.0f, 12.0f, 0.5f, 2);
            AddSliderBar("Content Padding", &ContentPadding, 4.0f, 40.0f, 1.0f, 2);
            AddSeparator(true);
            AddSection("Animation Settings");
            AddToggleOption("Animations", &bAnimations);
            AddSliderBar("Animation Speed", &AnimationSpeed, kAnimationSpeedMin, kAnimationSpeedMax, 0.1f, 2);
            AddSliderBar("Scroll Speed", &ScrollSpeed, 1.0f, 10.0f, 1.0f, 2);
            AddSliderBar("Fade Time", &FadeTime, 0.0f, 1.0f, 0.05f, 2);
            AddSeparator(true);
            AddSection("Colors");
            AddDropdown("Menu Color", &MenuColor, kColorOptions, (int)(sizeof(kColorOptions) / sizeof(kColorOptions[0])));
            AddDropdown("Menu Opacity", &MenuOpacity, kOpacityOptions, (int)(sizeof(kOpacityOptions) / sizeof(kOpacityOptions[0])));
            break;
        }
    }
}

void BuildPlayerOptionsPage(int clientIndex)
{
    GamePlayers players;
    const GamePlayer* player = ReadGamePlayers(&players) ? FindPlayer(players, clientIndex) : NULL;
    if (player == NULL)
    {
        ClosePage();
        return;
    }

    char indexText[8];
    _snprintf(indexText, sizeof(indexText), "%d", player->slot);
    indexText[sizeof(indexText) - 1] = 0;

    AddInfoLine(0, "Name", player->name);
    AddInfoLine(1, "Client Index", indexText);
    AddInfoLine(2, "Team", TeamName(player->team));
    AddInfoLine(3, "Host", player->slot == players.hostSlot ? "Yes" : "No");
    AddInfoLine(4, "Relation", RelationName(GetPlayerRelation(players, *player)));

    AddSection("Actions");
    AddButton("Print Player Info", &PrintInfo, clientIndex);
}

void RunMenuFrame()
{
    // 0x82C70F44 = local client count, 0x82C70F4C = client-active pointer
    if (*(volatile int*)0x82C70F44 <= 0 || *(volatile DWORD*)0x82C70F4C == 0)
    {
        UpdateFunctions();
        return;
    }

    UpdateFunctions();
    RunFrame();
}