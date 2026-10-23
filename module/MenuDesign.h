#pragma once

// Relation of a player row to the local player
enum PlayerRelation { RELATION_YOU, RELATION_FRIENDLY, RELATION_ENEMY, RELATION_NEUTRAL };

// Builds the items of a sub page; receives the argument given to OpenPage
typedef void (*PageBuilder)(int argument);

// Called after a toggle was flipped, receives the new value
typedef void (*ToggleAction)(bool bValue);

// Item building API (only valid inside BuildTab)
void AddSection(const char* title);
void AddSeparator(bool bAccent = false);
void AddInfoLabel(const char* text);
void AddToggleOption(const char* label, bool* pbValue);
void AddToggleOption(const char* label, bool* pbValue, ToggleAction toggleAction);   // flips the bool, then calls toggleAction once
void AddSliderBar(const char* label, float* value, float minValue, float maxValue, float step, int decimals);
void AddDropdown(const char* label, int* index, const char* const* options, int count);
void AddButton(const char* label, void (*callback)());
void AddButton(const char* label, void (*callback)(int), int argument);   // callback receives the argument
// Player list row (name is copied); RELATION_NEUTRAL is used for spectators
void AddPlayerRow(const char* name, bool bIsHost, PlayerRelation relation, int slot);
void AddSubmenu(const char* label, bool* expanded);   // items added until EndSubmenu are the group's children
void EndSubmenu();

// Sub pages replace the tab content until closed (the title is copied)
void OpenPage(PageBuilder builder, int argument, const char* title);
void ClosePage();

// Input, animation and drawing for one frame (in-game only)
void RunFrame();

// Section: Shared constants and types

// Screen and panel geometry (layout units)
const float kScreenWidth = 1280.0f;
const float kScreenHeight = 720.0f;
const float kPanelHeight = 500.0f;
const float kPanelPadding = 14.0f;
const float kBorderThickness = 2.0f;
const float kTitleHeight = kDefaultHeaderHeight;
const float kTabBarHeight = kDefaultTabsHeight;

// Row heights
const float kRowHeight = 36.0f;
const float kSectionHeight = 32.0f;
const float kSeparatorHeight = 12.0f;
const float kOverlayItemHeight = 26.0f;

// Text scales
const float kTextScale = 1.03f;
const float kSmallTextScale = 1.0f; //0.84f

// Colors (r, g, b, a)
const float kColorBackground[4]    = { 0.07f, 0.07f, 0.09f, 0.95f };
const float kColorSectionBar[4]    = { 0.12f, 0.12f, 0.15f, 0.95f };
const float kColorScrollTrack[4]   = { 0.14f, 0.14f, 0.17f, 1.00f };
const float kColorControlBox[4]    = { 0.04f, 0.04f, 0.05f, 1.00f };
const float kColorText[4]          = { 0.82f, 0.82f, 0.84f, 1.00f };
const float kColorMutedText[4]     = { 0.55f, 0.55f, 0.58f, 1.00f };
const float kColorWhite[4]         = { 1.00f, 1.00f, 1.00f, 1.00f };
const float kColorGreen[4]         = { 0.20f, 0.90f, 0.20f, 1.00f };
const float kColorYellow[4]        = { 0.95f, 0.85f, 0.20f, 1.00f };
const float kColorRed[4]           = { 0.90f, 0.20f, 0.20f, 1.00f };
const float kColorKnobOff[4]       = { 0.95f, 0.25f, 0.25f, 1.00f };
const float kColorControlBorder[4] = { 0.28f, 0.28f, 0.32f, 1.00f };
const float kColorLime[4]          = { 0.70f, 0.90f, 0.20f, 1.00f };

const int kMaxItems = 64;
const int kMaxSubmenus = 8;   // submenu ids are 1..7
const int kPlayerNameSize = 40;

enum ItemType
{
    ITEM_SECTION, ITEM_SEPARATOR, ITEM_TOGGLE, ITEM_DROPDOWN, ITEM_SUBMENU,
    ITEM_INFO, ITEM_SLIDER, ITEM_BUTTON, ITEM_PLAYER
};

struct MenuItem
{
    ItemType type;
    const char* label;
    int parentSubmenu;          // 0 = top level, else id of the parent submenu
    int submenuId;              // id of this submenu (ITEM_SUBMENU)
    bool bAccentLine;            // ITEM_SEPARATOR: accent colored line
    bool* boolValue;
    ToggleAction toggleAction;  // ITEM_TOGGLE: called once with the new value after the flip, NULL = none
    int* intValue;
    float* floatValue;
    float minValue, maxValue, step;
    int decimals;
    const char* const* options;
    int optionCount;
    void (*callback)();
    void (*actionCallback)(int);   // ITEM_BUTTON: used instead of callback when set
    int argument;               // ITEM_BUTTON: passed to actionCallback
    int slot;                   // ITEM_PLAYER: client number
    PlayerRelation relation;    // ITEM_PLAYER
    bool bIsHost;                // ITEM_PLAYER: shows the host badge
    char playerName[kPlayerNameSize];   // ITEM_PLAYER: own copy of the player name
};

inline float ClampFloat(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

inline float Lerp(float from, float to, float t)
{
    return from + (to - from) * t;
}

// Section: Shared state

// Items
extern MenuItem gItems[kMaxItems];
extern int gItemCount;
extern bool* gSubmenuExpanded[kMaxSubmenus];

// Menu state
extern bool gbMenuOpen;
extern int gCurrentTab;
extern int gSelectedItem;
extern int gVisibleItems[kMaxItems];
extern int gVisibleCount;
extern bool gbDropdownOpen;
extern int gDropdownHover;

// Runtime layout and theme, updated every frame from the settings
extern float gPanelWidth;
extern float gPanelHeight;
extern float gTitleHeight;
extern float gTabBarHeight;
extern float gRowExtraHeight;
extern float gContentPadding;
extern float gAccentColor[4];
extern float gOpacity;

// Animation state
extern float gOpenProgress;
extern float gToggleAnim[kMaxItems];
extern float gSelectAnim[kMaxItems];
extern float gSubmenuAnim[kMaxSubmenus];
extern float gSliderDisplay[kMaxItems];
extern float gDropdownAnim;
extern float gDropdownHoverY;
extern int gDropdownItem;                      // item that owns the (possibly collapsing) overlay
extern float gInfoProgress;

// List view state; rows are in content space of the visible list
extern float gRowTop[kMaxItems];
extern float gRowHeights[kMaxItems];
extern float gContentHeight;
extern float gHighlightY;
extern float gHighlightHeight;
extern float gHighlightPulse;
extern float gTabIndicatorX;
extern float gTabIndicatorWidth;
extern float gTabTransition;
extern int gTabDirection;                      // +1 = moved right, -1 = moved left

// Raw pad snapshot written by the XamInputGetState hook (Main.cpp)
extern volatile LONG  gRawPadValid;
extern volatile DWORD gRawPadTick;
extern volatile DWORD gRawPadPacket;
extern volatile WORD  gRawPadButtons;
extern volatile BYTE  gRawPadLeftTrigger, gRawPadRightTrigger;
extern volatile SHORT gRawPadThumbLeftX, gRawPadThumbLeftY, gRawPadThumbRightX, gRawPadThumbRightY;

// 1 while the game must not see button input
extern volatile LONG  gMenuBlocksInput;

// Section: Internal functions

// Items, visible list and selection
bool IsSubmenuExpanded(int submenuId);
void BuildVisibleList(bool bForDraw);
int FindVisiblePosition(int itemIndex);
void SelectFirstItem();
void SetTab(int tab);
bool IsPageOpen();
void MoveSelection(int direction);

// Animation
float EaseOutCubic(float t);
float Smoothstep(float t);
float ScaledDuration(float baseSeconds);
float Approach(float current, float target, float duration);
float SmoothTo(float current, float target, float rate, float epsilon);
void BeginAnimationFrame();
void SnapItemAnimations();
void SnapViewIfNeeded();
void UpdateItemAnimations();
void UpdateOpenAnimation();
void UpdateInfoAnimation();

// List layout, scrolling and highlight
float ItemHeight(const MenuItem& item);
void ComputeRowMetrics();
float MaxScrollOffset(float listHeight);
float UpdateScroll(float listHeight, int selectedPosition);
void UpdateHighlight(int selectedPosition);
void UpdateTabIndicator(float targetX, float targetWidth);
void StartTabTransition(int direction);
void UpdateTabTransition();
void SnapListView();

// Input
void HandleInput();

// Rows, controls and the dropdown overlay
void DrawRow(int itemIndex, float left, float right, float top);
void ResetDropdownOverlay();
void DrawDropdownOverlay();

// Footer button hints
void DrawFooter(float centerX, float top, float height);

// Server info box and FPS counter
void DrawServerInfo();
void DrawFpsCounter();

// Panel
void DrawPanel();