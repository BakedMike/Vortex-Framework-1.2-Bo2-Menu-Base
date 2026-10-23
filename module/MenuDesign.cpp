#include "stdafx.h"

// Section: Menu state, theme tables and page handling

MenuItem gItems[kMaxItems];
int gItemCount = 0;
bool* gSubmenuExpanded[kMaxSubmenus];

bool gbMenuOpen = false;
int gCurrentTab = TAB_AIMBOT;
int gSelectedItem = -1;
int gVisibleItems[kMaxItems];
int gVisibleCount = 0;
bool gbDropdownOpen = false;
int gDropdownHover = 0;

float gPanelWidth = kDefaultMenuWidth;
float gPanelHeight = kPanelHeight;
float gTitleHeight = kTitleHeight;
float gTabBarHeight = kTabBarHeight;
float gRowExtraHeight = 0.0f;
float gContentPadding = kPanelPadding;
float gAccentColor[4] = { 1.0f, 0.27f, 0.30f, 1.0f };
float gOpacity = 1.0f;

// Menu Color dropdown choices
const float kAccentColors[5][3] =
{
    { 1.00f, 0.27f, 0.30f },
    { 0.30f, 0.55f, 1.00f },
    { 0.25f, 0.85f, 0.40f },
    { 0.70f, 0.40f, 1.00f },
    { 1.00f, 0.60f, 0.15f }
};
// Menu Opacity dropdown choices
const float kOpacityLevels[4] = { 1.00f, 0.85f, 0.70f, 0.50f };

static MenuItem gOverflowItem;
static PageBuilder gPageBuilder = NULL;
static int gPageArgument = 0;
static char gPageTitle[kPlayerNameSize];
static bool gbBuildingItems = false;
static bool gbPageClosed = false;   // a builder closed its own page, the tab is rebuilt afterwards
static int gCurrentSubmenu = 0;
static int gSubmenuCount = 0;

static bool IsSelectable(const MenuItem& item)
{
    return item.type == ITEM_TOGGLE || item.type == ITEM_DROPDOWN || item.type == ITEM_SUBMENU
        || item.type == ITEM_SLIDER || item.type == ITEM_BUTTON || item.type == ITEM_PLAYER;
}

static MenuItem& NewItem(ItemType type, const char* label)
{
    MenuItem& item = (gItemCount < kMaxItems) ? gItems[gItemCount++] : gOverflowItem;
    memset(&item, 0, sizeof(item));
    item.type = type;
    item.label = label;
    item.parentSubmenu = gCurrentSubmenu;
    return item;
}

static void FillItems()
{
    gItemCount = 0;
    gCurrentSubmenu = 0;
    gSubmenuCount = 0;
    memset(gSubmenuExpanded, 0, sizeof(gSubmenuExpanded));
    if (gPageBuilder == NULL)
    {
        BuildTab(gCurrentTab);
        return;
    }
    gbBuildingItems = true;
    AddSection(gPageTitle);
    gPageBuilder(gPageArgument);
    gbBuildingItems = false;
}

static void RebuildItems()
{
    FillItems();
    if (!gbPageClosed)
        return;
    // the page closed itself while building (for example its player left): show the tab again
    gbPageClosed = false;
    FillItems();
    SelectFirstItem();
    StartTabTransition(-1);
}

bool IsSubmenuExpanded(int submenuId)
{
    return submenuId > 0 && submenuId < kMaxSubmenus && gSubmenuExpanded[submenuId] != NULL && *gSubmenuExpanded[submenuId];
}

// bForDraw also keeps the children of collapsing submenus
void BuildVisibleList(bool bForDraw)
{
    gVisibleCount = 0;
    for (int i = 0; i < gItemCount && gVisibleCount < kMaxItems; i++)
    {
        const MenuItem& item = gItems[i];
        if (item.parentSubmenu != 0 && !IsSubmenuExpanded(item.parentSubmenu)
            && !(bForDraw && gSubmenuAnim[item.parentSubmenu] > 0.001f))
            continue;
        gVisibleItems[gVisibleCount++] = i;
    }
}

int FindVisiblePosition(int itemIndex)
{
    for (int i = 0; i < gVisibleCount; i++)
        if (gVisibleItems[i] == itemIndex)
            return i;
    return -1;
}

void SelectFirstItem()
{
    BuildVisibleList(false);
    gSelectedItem = -1;
    for (int i = 0; i < gVisibleCount; i++)
    {
        if (IsSelectable(gItems[gVisibleItems[i]]))
        {
            gSelectedItem = gVisibleItems[i];
            break;
        }
    }
}

void SetTab(int tab)
{
    int direction = (tab > gCurrentTab) ? 1 : -1;
    if (tab < 0) { tab = TAB_COUNT - 1; direction = -1; }
    if (tab >= TAB_COUNT) { tab = 0; direction = 1; }
    gCurrentTab = tab;
    gPageBuilder = NULL;
    gbPageClosed = false;
    gbDropdownOpen = false;
    RebuildItems();
    SelectFirstItem();
    StartTabTransition(direction);
}

bool IsPageOpen()
{
    return gPageBuilder != NULL;
}

void OpenPage(PageBuilder builder, int argument, const char* title)
{
    strncpy(gPageTitle, title ? title : "", sizeof(gPageTitle) - 1);   // copy first, title may point into gItems
    gPageTitle[sizeof(gPageTitle) - 1] = 0;
    gPageBuilder = builder;
    gPageArgument = argument;
    gbPageClosed = false;
    gbDropdownOpen = false;
    RebuildItems();
    if (gPageBuilder == NULL)
        return;
    SelectFirstItem();
    StartTabTransition(1);
}

void ClosePage()
{
    if (gPageBuilder == NULL)
        return;
    gPageBuilder = NULL;
    gbPageClosed = true;
    gbDropdownOpen = false;
    if (!gbBuildingItems)
        RebuildItems();
}

void MoveSelection(int direction)
{
    BuildVisibleList(false);
    int position = FindVisiblePosition(gSelectedItem);
    if (position < 0)
    {
        SelectFirstItem();
        return;
    }
    for (int step = 0; step < gVisibleCount; step++)
    {
        position += direction;
        if (position < 0) position = gVisibleCount - 1;
        if (position >= gVisibleCount) position = 0;
        if (IsSelectable(gItems[gVisibleItems[position]]))
        {
            gSelectedItem = gVisibleItems[position];
            return;
        }
    }
}

// Section: Item building API

void AddSection(const char* title)
{
    NewItem(ITEM_SECTION, title);
}

void AddSeparator(bool bAccent)
{
    NewItem(ITEM_SEPARATOR, "").bAccentLine = bAccent;
}

void AddInfoLabel(const char* text)
{
    NewItem(ITEM_INFO, text);
}

void AddToggleOption(const char* label, bool* pbValue)
{
    AddToggleOption(label, pbValue, NULL);
}

void AddToggleOption(const char* label, bool* pbValue, ToggleAction toggleAction)
{
    MenuItem& item = NewItem(ITEM_TOGGLE, label);   // NewItem clears the item, so toggleAction never leaks from an old item
    item.boolValue = pbValue;
    item.toggleAction = toggleAction;
}

void AddSliderBar(const char* label, float* value, float minValue, float maxValue, float step, int decimals)
{
    MenuItem& item = NewItem(ITEM_SLIDER, label);
    item.floatValue = value;
    item.minValue = minValue;
    item.maxValue = maxValue;
    item.step = step;
    item.decimals = decimals;
}

void AddDropdown(const char* label, int* index, const char* const* options, int count)
{
    MenuItem& item = NewItem(ITEM_DROPDOWN, label);
    item.intValue = index;
    item.options = options;
    item.optionCount = count;
}

void AddButton(const char* label, void (*callback)())
{
    NewItem(ITEM_BUTTON, label).callback = callback;
}

void AddButton(const char* label, void (*callback)(int), int argument)
{
    MenuItem& item = NewItem(ITEM_BUTTON, label);
    item.actionCallback = callback;
    item.argument = argument;
}

void AddPlayerRow(const char* name, bool bIsHost, PlayerRelation relation, int slot)
{
    MenuItem& item = NewItem(ITEM_PLAYER, "");
    strncpy(item.playerName, name ? name : "", sizeof(item.playerName) - 1);
    item.playerName[sizeof(item.playerName) - 1] = 0;
    item.bIsHost = bIsHost;
    item.relation = relation;
    item.slot = slot;
}

void AddSubmenu(const char* label, bool* expanded)
{
    MenuItem& item = NewItem(ITEM_SUBMENU, label);
    int id = (gSubmenuCount + 1 < kMaxSubmenus) ? ++gSubmenuCount : 0;
    item.submenuId = id;
    if (id > 0)
        gSubmenuExpanded[id] = expanded;
    gCurrentSubmenu = id;
}

void EndSubmenu()
{
    gCurrentSubmenu = 0;
}

// Section: Animation

float gOpenProgress = 0.0f;
float gToggleAnim[kMaxItems];
float gSelectAnim[kMaxItems];
float gSubmenuAnim[kMaxSubmenus];
float gSliderDisplay[kMaxItems];
float gDropdownAnim = 0.0f;
float gDropdownHoverY = 0.0f;
int gDropdownItem = -1;
float gInfoProgress = 0.0f;

// Durations are base seconds, divided by the Animation Speed setting; rates are per second
const float kDefaultFrameSeconds = 0.016f;
const float kMaxFrameSeconds = 0.05f;
const float kCloseDurationFactor = 0.8f;
const float kToggleDuration = 0.12f;
const float kDropdownDuration = 0.14f;
const float kSubmenuDuration = 0.16f;
const float kSelectNudgeDuration = 0.12f;
const float kInfoDuration = 0.20f;
const float kSliderRate = 22.0f;
const float kDropdownHoverRate = 28.0f;
const float kSliderSnapEpsilon = 0.002f;
const float kDropdownHoverSnapEpsilon = 0.05f;

static bool gbAnimationsEnabled = true;
static float gAnimationSpeed = 1.0f;
static float gFrameSeconds = kDefaultFrameSeconds;
static DWORD gLastFrameTick = 0;

static bool gbNeedSnap = true;            // snap all view animations on the next panel draw
static bool gbDropdownWasOpen = false;
static bool gbInfoInitialized = false;

static bool IsToggleOn(const MenuItem& item)
{
    return item.boolValue != NULL && *item.boolValue;
}

static float SliderFraction(const MenuItem& item)
{
    float fraction = (item.maxValue > item.minValue) ? (*item.floatValue - item.minValue) / (item.maxValue - item.minValue) : 0.0f;
    return ClampFloat(fraction, 0.0f, 1.0f);
}

float EaseOutCubic(float t)
{
    t = ClampFloat(t, 0.0f, 1.0f);
    float inverse = 1.0f - t;
    return 1.0f - inverse * inverse * inverse;
}

float Smoothstep(float t)
{
    t = ClampFloat(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float ScaledDuration(float baseSeconds)
{
    return baseSeconds / gAnimationSpeed;
}

// Linear-time progress toward target; animations off or duration <= 0 snaps
float Approach(float current, float target, float duration)
{
    if (!gbAnimationsEnabled || duration <= 0.0f)
        return target;
    float step = gFrameSeconds / duration;
    if (current < target) { current += step; if (current > target) current = target; }
    else if (current > target) { current -= step; if (current < target) current = target; }
    return current;
}

// Exponential ease-out toward target (rate per second, scaled by Animation Speed)
float SmoothTo(float current, float target, float rate, float epsilon)
{
    if (!gbAnimationsEnabled)
        return target;
    float blend = 1.0f - (float)exp((double)(-rate * gAnimationSpeed * gFrameSeconds));
    current += (target - current) * blend;
    if (fabs(target - current) < epsilon)
        current = target;
    return current;
}

// Frame-rate independent: dt comes from GetTickCount
void BeginAnimationFrame()
{
    DWORD now = GetTickCount();
    float seconds = (gLastFrameTick == 0) ? kDefaultFrameSeconds : (float)(DWORD)(now - gLastFrameTick) * 0.001f;
    gLastFrameTick = now;
    gFrameSeconds = ClampFloat(seconds, 0.0f, kMaxFrameSeconds);
    gAnimationSpeed = ClampFloat(AnimationSpeed, kAnimationSpeedMin, kAnimationSpeedMax);
    gbAnimationsEnabled = bAnimations && (AnimationSpeed < kAnimationSpeedMax - 0.001f);
}

// Jump every per-item animation value to its target
void SnapItemAnimations()
{
    for (int i = 0; i < gItemCount; i++)
    {
        const MenuItem& item = gItems[i];
        gToggleAnim[i] = (item.type == ITEM_TOGGLE && IsToggleOn(item)) ? 1.0f : 0.0f;
        gSelectAnim[i] = (i == gSelectedItem && gbMenuOpen) ? 1.0f : 0.0f;
        gSliderDisplay[i] = (item.type == ITEM_SLIDER) ? SliderFraction(item) : 0.0f;
    }
    for (int submenu = 0; submenu < kMaxSubmenus; submenu++)
        gSubmenuAnim[submenu] = IsSubmenuExpanded(submenu) ? 1.0f : 0.0f;
    gDropdownAnim = gbDropdownOpen ? 1.0f : 0.0f;
    gDropdownHoverY = (float)gDropdownHover * kOverlayItemHeight;
    gbDropdownWasOpen = gbDropdownOpen;
}

void SnapViewIfNeeded()
{
    if (!gbNeedSnap)
        return;
    SnapItemAnimations();
    SnapListView();
    gbNeedSnap = false;
}

// Toggles, label nudge, submenus, slider display values and the dropdown
void UpdateItemAnimations()
{
    for (int i = 0; i < gItemCount; i++)
    {
        const MenuItem& item = gItems[i];
        if (item.type == ITEM_TOGGLE)
            gToggleAnim[i] = Approach(gToggleAnim[i], IsToggleOn(item) ? 1.0f : 0.0f, ScaledDuration(kToggleDuration));
        if (item.type == ITEM_SLIDER)
            gSliderDisplay[i] = SmoothTo(gSliderDisplay[i], SliderFraction(item), kSliderRate, kSliderSnapEpsilon);
        gSelectAnim[i] = Approach(gSelectAnim[i], (i == gSelectedItem) ? 1.0f : 0.0f, ScaledDuration(kSelectNudgeDuration));
    }
    for (int submenu = 0; submenu < kMaxSubmenus; submenu++)
        gSubmenuAnim[submenu] = Approach(gSubmenuAnim[submenu], IsSubmenuExpanded(submenu) ? 1.0f : 0.0f, ScaledDuration(kSubmenuDuration));

    if (gbDropdownOpen && !gbDropdownWasOpen)
        gDropdownHoverY = (float)gDropdownHover * kOverlayItemHeight;   // start the hover bar on the current value
    gbDropdownWasOpen = gbDropdownOpen;
    gDropdownAnim = Approach(gDropdownAnim, gbDropdownOpen ? 1.0f : 0.0f, ScaledDuration(kDropdownDuration));
    gDropdownHoverY = SmoothTo(gDropdownHoverY, (float)gDropdownHover * kOverlayItemHeight, kDropdownHoverRate, kDropdownHoverSnapEpsilon);
}

// Open / close progress; the panel keeps drawing while closing
void UpdateOpenAnimation()
{
    float duration = ScaledDuration(FadeTime) * (gbMenuOpen ? 1.0f : kCloseDurationFactor);
    gOpenProgress = Approach(gOpenProgress, gbMenuOpen ? 1.0f : 0.0f, duration);
    if (!gbMenuOpen && gOpenProgress <= 0.0f)
        gbNeedSnap = true;   // next open starts from a clean view animation state
}

void UpdateInfoAnimation()
{
    float target = bShowServerInfo ? 1.0f : 0.0f;
    if (!gbInfoInitialized)
    {
        gInfoProgress = target;
        gbInfoInitialized = true;
    }
    else
        gInfoProgress = Approach(gInfoProgress, target, ScaledDuration(kInfoDuration));
}

// Section: List layout, scrolling and highlight

float gRowTop[kMaxItems];
float gRowHeights[kMaxItems];
float gContentHeight = 0.0f;
float gHighlightY = 0.0f;
float gHighlightHeight = 0.0f;
float gHighlightPulse = 0.0f;
float gTabIndicatorX = 0.0f;
float gTabIndicatorWidth = 0.0f;
float gTabTransition = 1.0f;
int gTabDirection = 1;

const float kTabTransitionDuration = 0.15f;
const float kHighlightPulseDuration = 0.20f;
const float kScrollRate = 14.0f;
const float kHighlightRate = 24.0f;
const float kTabIndicatorRate = 20.0f;
const float kSnapEpsilon = 0.05f;
const float kReferenceScrollSpeed = 5.0f;   // Scroll Speed setting at which kScrollRate applies unchanged

static float gScrollCurrent = 0.0f;
static float gScrollTarget = 0.0f;
static bool gbSnapScroll = true;

static int gPreviousSelected = -1;
static bool gbSnapHighlight = true;
static bool gbSnapTabIndicator = true;

// Row height including the submenu expand / collapse animation
static float AnimatedItemHeight(int itemIndex)
{
    const MenuItem& item = gItems[itemIndex];
    float height = ItemHeight(item);
    if (item.parentSubmenu != 0)
        height *= EaseOutCubic(gSubmenuAnim[item.parentSubmenu]);
    return height;
}

float ItemHeight(const MenuItem& item)
{
    if (item.type == ITEM_SECTION) return kSectionHeight;
    if (item.type == ITEM_SEPARATOR) return kSeparatorHeight;
    return kRowHeight + gRowExtraHeight;
}

// Rows including rows of collapsing / expanding submenus (animated heights)
void ComputeRowMetrics()
{
    BuildVisibleList(true);
    gContentHeight = 0.0f;
    for (int i = 0; i < gVisibleCount; i++)
    {
        gRowTop[i] = gContentHeight;
        gRowHeights[i] = AnimatedItemHeight(gVisibleItems[i]);
        gContentHeight += gRowHeights[i];
    }
}

float MaxScrollOffset(float listHeight)
{
    return (gContentHeight > listHeight) ? (gContentHeight - listHeight) : 0.0f;
}

// Smooth scrolling: the target keeps the selected row visible, the current offset eases toward it.
// Returns the whole-pixel scroll offset in layout units.
float UpdateScroll(float listHeight, int selectedPosition)
{
    float maxOffset = MaxScrollOffset(listHeight);
    if (maxOffset <= 0.0f)
        gScrollTarget = 0.0f;
    else if (selectedPosition >= 0)
    {
        float selectedTop = gRowTop[selectedPosition];
        float selectedBottom = selectedTop + gRowHeights[selectedPosition];
        if (selectedPosition > 0 && gItems[gVisibleItems[selectedPosition - 1]].type == ITEM_SECTION)
            selectedTop = gRowTop[selectedPosition - 1];   // also reveal a header directly above
        if (selectedTop < gScrollTarget)
            gScrollTarget = selectedTop;
        if (selectedBottom > gScrollTarget + listHeight)
            gScrollTarget = selectedBottom - listHeight;
    }
    gScrollTarget = ClampFloat(gScrollTarget, 0.0f, maxOffset);
    if (gbSnapScroll)
    {
        gScrollCurrent = gScrollTarget;
        gbSnapScroll = false;
    }
    else
        gScrollCurrent = SmoothTo(gScrollCurrent, gScrollTarget, kScrollRate * (ScrollSpeed / kReferenceScrollSpeed), kSnapEpsilon);
    gScrollCurrent = ClampFloat(gScrollCurrent, 0.0f, maxOffset);
    float scale = GetDrawScale();
    return floorf(gScrollCurrent * scale + 0.5f) / scale;
}

// The selection highlight glides between rows
void UpdateHighlight(int selectedPosition)
{
    if (selectedPosition >= 0)
    {
        if (gbSnapHighlight)
        {
            gHighlightY = gRowTop[selectedPosition];
            gHighlightHeight = gRowHeights[selectedPosition];
            gHighlightPulse = 0.0f;
            gbSnapHighlight = false;
        }
        else
        {
            if (gPreviousSelected != gSelectedItem)
                gHighlightPulse = 1.0f;
            gHighlightY = SmoothTo(gHighlightY, gRowTop[selectedPosition], kHighlightRate, kSnapEpsilon);
            gHighlightHeight = SmoothTo(gHighlightHeight, gRowHeights[selectedPosition], kHighlightRate, kSnapEpsilon);
        }
        gPreviousSelected = gSelectedItem;
    }
    gHighlightPulse = Approach(gHighlightPulse, 0.0f, ScaledDuration(kHighlightPulseDuration));
}

// The active-tab rectangle slides to the new tab
void UpdateTabIndicator(float targetX, float targetWidth)
{
    if (gbSnapTabIndicator)
    {
        gTabIndicatorX = targetX;
        gTabIndicatorWidth = targetWidth;
        gbSnapTabIndicator = false;
    }
    else
    {
        gTabIndicatorX = SmoothTo(gTabIndicatorX, targetX, kTabIndicatorRate, kSnapEpsilon);
        gTabIndicatorWidth = SmoothTo(gTabIndicatorWidth, targetWidth, kTabIndicatorRate, kSnapEpsilon);
    }
}

void StartTabTransition(int direction)
{
    gTabDirection = direction;
    gTabTransition = 0.0f;
    gbSnapScroll = true;
    gbSnapHighlight = true;
    gScrollTarget = 0.0f;
    gPreviousSelected = -1;
    SnapItemAnimations();
}

void UpdateTabTransition()
{
    gTabTransition = Approach(gTabTransition, 1.0f, ScaledDuration(kTabTransitionDuration));
}

// Jump scroll, highlight, tab indicator and tab transition to their targets
void SnapListView()
{
    gbSnapScroll = gbSnapHighlight = gbSnapTabIndicator = true;
    gTabTransition = 1.0f;
}

// Section: Input

volatile LONG  gRawPadValid = 0;
volatile DWORD gRawPadTick = 0;
volatile DWORD gRawPadPacket = 0;
volatile WORD  gRawPadButtons = 0;
volatile BYTE  gRawPadLeftTrigger = 0;
volatile BYTE  gRawPadRightTrigger = 0;
volatile SHORT gRawPadThumbLeftX = 0, gRawPadThumbLeftY = 0, gRawPadThumbRightX = 0, gRawPadThumbRightY = 0;
volatile LONG  gMenuBlocksInput = 0;

const DWORD kRepeatDelayMs = 400;
const DWORD kRepeatRateMs = 150;
const DWORD kFreshSnapshotMs = 100;
const int kSliderAccelerateAfterMs = 1000;
const int kSliderAcceleratedMultiplier = 4;

static WORD gPreviousButtons = 0;
static WORD gButtons = 0;
static WORD gPressedButtons = 0;
static DWORD gNow = 0;
static DWORD gNextRepeatUp = 0;
static DWORD gNextRepeatDown = 0;
static DWORD gNextRepeatLeft = 0;
static DWORD gNextRepeatRight = 0;
static DWORD gHoldStart = 0;

// direction = -1 / +1, multiplier = step multiplier (hold acceleration)
static void AdjustSlider(const MenuItem& item, int direction, int multiplier)
{
    if (item.type != ITEM_SLIDER || !item.floatValue)
        return;
    float value = *item.floatValue + (float)direction * item.step * (float)multiplier;
    value = item.minValue + floorf((value - item.minValue) / item.step + 0.5f) * item.step;   // snap to the step grid
    *item.floatValue = ClampFloat(value, item.minValue, item.maxValue);
}

static void CycleDropdown(const MenuItem& item, int direction)
{
    if (item.optionCount <= 0 || !item.intValue)
        return;
    int value = *item.intValue + direction;
    if (value < 0) value = item.optionCount - 1;
    if (value >= item.optionCount) value = 0;
    *item.intValue = value;
}

static void OpenDropdown(const MenuItem& item)
{
    int value = item.intValue ? *item.intValue : 0;
    if (value < 0 || value >= item.optionCount) value = 0;
    gDropdownHover = value;
    gbDropdownOpen = true;
}

// direction 0 = A pressed, -1 / +1 = d-pad left / right
static void ActivateSelected(int direction)
{
    if (gSelectedItem < 0)
        return;
    const MenuItem& item = gItems[gSelectedItem];
    if (item.type == ITEM_TOGGLE)
    {
        if (item.boolValue)
        {
            *item.boolValue = !*item.boolValue;   // flip first, then tell the feature about the new value
            if (item.toggleAction)
                item.toggleAction(*item.boolValue);
        }
    }
    else if (item.type == ITEM_DROPDOWN)
    {
        if (direction == 0)
            OpenDropdown(item);
        else
            CycleDropdown(item, direction);
    }
    else if (item.type == ITEM_SUBMENU)
    {
        if (item.submenuId > 0 && gSubmenuExpanded[item.submenuId])
            *gSubmenuExpanded[item.submenuId] = !*gSubmenuExpanded[item.submenuId];
    }
    else if (item.type == ITEM_SLIDER)
    {
        if (direction != 0)
            AdjustSlider(item, direction, 1);
    }
    else if (item.type == ITEM_BUTTON)
    {
        if (direction != 0)
            return;
        if (item.actionCallback)
            item.actionCallback(item.argument);
        else if (item.callback)
            item.callback();
    }
    else if (item.type == ITEM_PLAYER)
    {
        if (direction == 0)
            OpenPage(BuildPlayerOptionsPage, item.slot, item.playerName);
    }
}

static bool ReadPadSnapshot(XINPUT_STATE* state)
{
    if (!gRawPadValid)
        return false;
    state->dwPacketNumber = gRawPadPacket;
    state->Gamepad.wButtons = gRawPadButtons;
    state->Gamepad.bLeftTrigger = gRawPadLeftTrigger;
    state->Gamepad.bRightTrigger = gRawPadRightTrigger;
    state->Gamepad.sThumbLX = gRawPadThumbLeftX;
    state->Gamepad.sThumbLY = gRawPadThumbLeftY;
    state->Gamepad.sThumbRX = gRawPadThumbRightX;
    state->Gamepad.sThumbRY = gRawPadThumbRightY;
    return true;
}

// Raw pad state: the hook's snapshot when fresh, else a direct poll
static bool GetMenuPad(XINPUT_STATE* state)
{
    DWORD now = GetTickCount();
    if (gRawPadValid && (DWORD)(now - gRawPadTick) < kFreshSnapshotMs)
        return ReadPadSnapshot(state);

    DWORD tickBefore = gRawPadTick;
    if (XInputGetState(0, state) != ERROR_SUCCESS)
        return false;
    // the poll went through our hook, so state is the masked copy: use the raw snapshot it just stored
    if (gRawPadTick != tickBefore)
        ReadPadSnapshot(state);
    return true;
}

// True on the first press and then on the auto-repeat schedule
static bool RepeatTrigger(WORD mask, DWORD* nextRepeat)
{
    if (!(gButtons & mask))
        return false;
    if (gPressedButtons & mask)
    {
        *nextRepeat = gNow + kRepeatDelayMs;
        return true;
    }
    if ((int)(gNow - *nextRepeat) >= 0)
    {
        *nextRepeat = gNow + kRepeatRateMs;
        return true;
    }
    return false;
}

// LB + DPad Down opens / closes the menu
static bool HandleOpenChord()
{
    bool bChord = (gButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
        && (gPressedButtons & (XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_LEFT_SHOULDER))
        && (gButtons & XINPUT_GAMEPAD_DPAD_DOWN);
    if (!bChord)
        return false;
    gbMenuOpen = !gbMenuOpen;
    gbDropdownOpen = false;
    if (gbMenuOpen && gSelectedItem < 0)
        SelectFirstItem();
    gNextRepeatUp = gNextRepeatDown = gNow + kRepeatDelayMs;
    return true;
}

static void HandleDropdownInput()
{
    const MenuItem& dropdown = gItems[gSelectedItem >= 0 ? gSelectedItem : 0];
    int optionCount = dropdown.optionCount > 0 ? dropdown.optionCount : 1;
    int move = 0;
    if (gButtons & XINPUT_GAMEPAD_DPAD_UP)
    {
        if (RepeatTrigger(XINPUT_GAMEPAD_DPAD_UP, &gNextRepeatUp)) move = -1;
    }
    else if (gButtons & XINPUT_GAMEPAD_DPAD_DOWN)
    {
        if (RepeatTrigger(XINPUT_GAMEPAD_DPAD_DOWN, &gNextRepeatDown)) move = 1;
    }
    if (move != 0)
    {
        gDropdownHover += move;
        if (gDropdownHover < 0) gDropdownHover = optionCount - 1;
        if (gDropdownHover >= optionCount) gDropdownHover = 0;
    }
    if (gPressedButtons & XINPUT_GAMEPAD_A)
    {
        if (gSelectedItem >= 0 && gItems[gSelectedItem].type == ITEM_DROPDOWN && gItems[gSelectedItem].intValue)
            *gItems[gSelectedItem].intValue = gDropdownHover;
        gbDropdownOpen = false;
    }
    else if (gPressedButtons & XINPUT_GAMEPAD_B)
        gbDropdownOpen = false;
}

// Slider auto-repeat while Left/Right is held (accelerates after 1s)
static void HandleSliderRepeat()
{
    if (gPressedButtons & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT))
    {
        gNextRepeatLeft = gNextRepeatRight = gNow + kRepeatDelayMs;
        gHoldStart = gNow;
        return;
    }
    if (gSelectedItem < 0 || gItems[gSelectedItem].type != ITEM_SLIDER)
        return;
    const MenuItem& slider = gItems[gSelectedItem];
    int multiplier = ((int)(gNow - gHoldStart) > kSliderAccelerateAfterMs) ? kSliderAcceleratedMultiplier : 1;
    if ((gButtons & XINPUT_GAMEPAD_DPAD_LEFT) && (int)(gNow - gNextRepeatLeft) >= 0)
    {
        AdjustSlider(slider, -1, multiplier);
        gNextRepeatLeft = gNow + kRepeatRateMs;
    }
    else if ((gButtons & XINPUT_GAMEPAD_DPAD_RIGHT) && (int)(gNow - gNextRepeatRight) >= 0)
    {
        AdjustSlider(slider, 1, multiplier);
        gNextRepeatRight = gNow + kRepeatRateMs;
    }
}

static void HandleMenuInput()
{
    bool bLeftBumperHeld = (gButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;

    if (RepeatTrigger(XINPUT_GAMEPAD_DPAD_UP, &gNextRepeatUp))
        MoveSelection(-1);
    if (!bLeftBumperHeld && RepeatTrigger(XINPUT_GAMEPAD_DPAD_DOWN, &gNextRepeatDown))
        MoveSelection(1);

    if (gPressedButtons & XINPUT_GAMEPAD_A)
        ActivateSelected(0);
    if (gPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT)
        ActivateSelected(-1);
    if (gPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
        ActivateSelected(1);

    HandleSliderRepeat();

    if (!(gButtons & XINPUT_GAMEPAD_DPAD_DOWN))
    {
        if (gPressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
            SetTab(gCurrentTab - 1);
        if (gPressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
            SetTab(gCurrentTab + 1);
    }
    if (gPressedButtons & XINPUT_GAMEPAD_B)
    {
        if (IsPageOpen())
            ClosePage();
        else
            gbMenuOpen = false;
    }
}

void HandleInput()
{
    XINPUT_STATE state;
    memset(&state, 0, sizeof(state));
    if (!GetMenuPad(&state))
        return;

    gButtons = state.Gamepad.wButtons;
    gPressedButtons = (WORD)(gButtons & ~gPreviousButtons);
    gNow = GetTickCount();

    if (HandleOpenChord())
    {
        gPreviousButtons = gButtons;
        return;
    }

    if (gbMenuOpen && gbDropdownOpen)
        HandleDropdownInput();
    else if (gbMenuOpen)
        HandleMenuInput();

    gPreviousButtons = gButtons;
}

// Section: Rows and controls

const float kToggleWidth = 56.0f;
const float kToggleHeight = 22.0f;
const float kToggleKnobInset = 3.0f;
const float kDropdownWidth = 156.0f;
const float kDropdownHeight = 26.0f;
const float kDropdownTextInset = 8.0f;
const float kDropdownArrowInset = 11.0f;
const float kDropdownTextScale = 0.90f;
const float kSliderWidth = 120.0f;
const float kSliderHeight = 14.0f;
const float kSliderKnobWidth = 6.0f;
const float kSliderKnobHeight = 18.0f;
const float kSliderValueGap = 12.0f;
const float kControlBorderThickness = 1.0f;

const float kSubmenuIndent = 18.0f;
const float kSelectNudgePixels = 4.0f;
const float kRowTextInset = 10.0f;
const float kSectionTextInset = 14.0f;
const float kSectionAccentWidth = 3.0f;
const float kInfoTextInset = 14.0f;
const float kSubmenuArrowInset = 16.0f;
const float kSubmenuTextInset = 28.0f;

const float kSeparatorAccentAlpha = 0.45f;
const float kSeparatorMutedAlpha = 0.35f;
const float kOverlayHoverAlpha = 0.35f;
const float kOverlayCurrentLighten = 0.25f;
const float kOverlayEdgeMargin = 4.0f;
const float kOverlayTopMargin = 2.0f;

const int kTriangleRows = 4;
const float kTriangleBaseWidth = 7.0f;

const float kPlayerTagGap = 6.0f;
const float kPlayerSlotWidth = 28.0f;
const float kPlayerNameGap = 6.0f;

const float kColorTagHost[4]    = { 1.00f, 0.78f, 0.15f, 1.00f };
const float kColorTagFriend[4]  = { 0.20f, 0.90f, 0.20f, 1.00f };
const float kColorTagEnemy[4]   = { 0.95f, 0.25f, 0.25f, 1.00f };
const float kColorTagBracket[4] = { 0.45f, 0.45f, 0.48f, 1.00f };

struct RowBounds { float left, right, top, height; };

// Dropdown overlay anchor recorded while drawing rows (layout units, relative to panel origin)
static bool gbOverlayPending = false;
static float gOverlayX = 0.0f, gOverlayY = 0.0f, gOverlayWidth = 0.0f, gOverlayBoxHeight = 0.0f;
static int gOverlayItem = -1;

// rightEdge = right edge of the control column, rowTop = row top, progress = animated 0..1
static void DrawToggle(float rightEdge, float rowTop, float progress)
{
    float x = rightEdge - kToggleWidth;
    float y = rowTop + (kRowHeight - kToggleHeight) * 0.5f;
    DrawShader(x - 1.0f, y - 1.0f, kToggleWidth + 2.0f, kToggleHeight + 2.0f, kColorControlBox);
    DrawBorder(x - 1.0f, y - 1.0f, kToggleWidth + 2.0f, kToggleHeight + 2.0f, kControlBorderThickness, kColorControlBorder);
    float knobWidth = (kToggleWidth - 2.0f * kToggleKnobInset) * 0.5f;
    float knobHeight = kToggleHeight - 2.0f * kToggleKnobInset;
    float eased = Smoothstep(progress);
    float knobX = Lerp(x + kToggleKnobInset, x + kToggleWidth - kToggleKnobInset - knobWidth, eased);
    float knobColor[4];

    for (int channel = 0; channel < 3; channel++)
        knobColor[channel] = Lerp(kColorKnobOff[channel], kColorGreen[channel], eased);

    knobColor[3] = 1.0f;
    DrawShader(knobX, y + kToggleKnobInset, knobWidth, knobHeight, knobColor);
}

// Small triangles built from rects (the font may lack arrow glyphs)
static void DrawTriangleDown(float centerX, float centerY, const float* color)
{
    for (int i = 0; i < kTriangleRows; i++)
    {
        float width = kTriangleBaseWidth - 2.0f * (float)i;
        DrawShader(centerX - width * 0.5f, centerY - 2.0f + (float)i, width, 1.0f, color);
    }
}

// flip = 0 points down, flip = 1 points up (rows slide through each other)
static void DrawTriangleFlip(float centerX, float centerY, float flip, const float* color)
{
    for (int i = 0; i < kTriangleRows; i++)
    {
        float width = kTriangleBaseWidth - 2.0f * (float)i;
        float rowY = Lerp(centerY - 2.0f + (float)i, centerY + 1.0f - (float)i, flip);
        DrawShader(centerX - width * 0.5f, floorf(rowY + 0.5f), width, 1.0f, color);
    }
}

static void DrawTriangleRight(float centerX, float centerY, const float* color)
{
    for (int i = 0; i < kTriangleRows; i++)
    {
        float height = kTriangleBaseWidth - 2.0f * (float)i;
        DrawShader(centerX - 2.0f + (float)i, centerY - height * 0.5f, 1.0f, height, color);
    }
}

static void DrawSlider(float rightEdge, float rowTop, int itemIndex)
{
    const MenuItem& item = gItems[itemIndex];
    float x = rightEdge - kSliderWidth;
    float y = rowTop + (kRowHeight - kSliderHeight) * 0.5f;
    DrawShader(x, y, kSliderWidth, kSliderHeight, kColorControlBox);
    DrawBorder(x, y, kSliderWidth, kSliderHeight, kControlBorderThickness, kColorControlBorder);

    float fraction = ClampFloat(gSliderDisplay[itemIndex], 0.0f, 1.0f);   // glides to the value
    float innerX = x + 1.0f, innerY = y + 1.0f, innerWidth = kSliderWidth - 2.0f, innerHeight = kSliderHeight - 2.0f;
    float knobX = floorf(innerX + fraction * (innerWidth - kSliderKnobWidth) + 0.5f);
    float fillWidth = knobX + kSliderKnobWidth * 0.5f - innerX;
    DrawShader(innerX, innerY, fillWidth, innerHeight, gAccentColor);
    DrawShader(knobX, y + (kSliderHeight - kSliderKnobHeight) * 0.5f, kSliderKnobWidth, kSliderKnobHeight, kColorWhite);

    char valueText[24];
    _snprintf(valueText, sizeof(valueText), "%.*f", item.decimals, *item.floatValue);
    valueText[sizeof(valueText) - 1] = 0;
    DrawText(valueText, x - kSliderValueGap, rowTop, kRowHeight, TEXT_ALIGN_RIGHT, kSmallTextScale, kColorWhite);
}

static void DrawDropdown(float rightEdge, float rowTop, const char* value, int itemIndex)
{
    // "open" includes the collapse phase so the overlay keeps drawing until it has shrunk away
    bool bOpen = (itemIndex == gDropdownItem && gDropdownAnim > 0.001f);
    float x = rightEdge - kDropdownWidth;
    float y = rowTop + (kRowHeight - kDropdownHeight) * 0.5f;
    DrawShader(x - 1.0f, y - 1.0f, kDropdownWidth + 2.0f, kDropdownHeight + 2.0f, kColorControlBox);
    DrawBorder(x - 1.0f, y - 1.0f, kDropdownWidth + 2.0f, kDropdownHeight + 2.0f, kControlBorderThickness, kColorControlBorder);
    DrawText(value, x + kDropdownTextInset, y, kDropdownHeight, TEXT_ALIGN_LEFT, kDropdownTextScale, kColorWhite);
    DrawTriangleFlip(x + kDropdownWidth - kDropdownArrowInset, y + kDropdownHeight * 0.5f,
        bOpen ? Smoothstep(gDropdownAnim) : 0.0f, kColorWhite);
    if (bOpen)
    {
        gbOverlayPending = true;
        gOverlayItem = itemIndex;
        gOverlayX = x;
        gOverlayY = y + kDropdownHeight;
        gOverlayWidth = kDropdownWidth;
        gOverlayBoxHeight = kDropdownHeight;
    }
}

static void DrawSectionRow(const MenuItem& item, const RowBounds& row)
{
    DrawShader(row.left, row.top + 1.0f, row.right - row.left, row.height - 2.0f, kColorSectionBar);
    DrawShader(row.left, row.top + 4.0f, kSectionAccentWidth, row.height - 8.0f, gAccentColor);
    DrawText(item.label, row.left + kSectionTextInset, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, gAccentColor);
}

static void DrawSeparatorRow(const MenuItem& item, const RowBounds& row)
{
    float color[4];
    if (item.bAccentLine)
        MakeColorWithAlpha(color, gAccentColor, kSeparatorAccentAlpha);
    else
        MakeColorWithAlpha(color, kColorMutedText, kSeparatorMutedAlpha);
    DrawShader(row.left + 4.0f, row.top + row.height * 0.5f, row.right - row.left - 8.0f, 1.0f, color);
}

static void DrawSubmenuRow(const MenuItem& item, bool bSelected, float nudge, const RowBounds& row)
{
    const float* color = bSelected ? kColorWhite : kColorText;
    if (IsSubmenuExpanded(item.submenuId))
        DrawTriangleDown(row.left + kSubmenuArrowInset, row.top + row.height * 0.5f, color);
    else
        DrawTriangleRight(row.left + kSubmenuArrowInset, row.top + row.height * 0.5f, color);
    if (bSelected)
        DrawTextBold(item.label, row.left + kSubmenuTextInset + nudge, row.top, row.height, kTextScale, kColorWhite);
    else
        DrawText(item.label, row.left + kSubmenuTextInset + nudge, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorText);
}

static void DrawControlRow(const MenuItem& item, int itemIndex, bool bSelected, float nudge, const RowBounds& row)
{
    float indent = (item.parentSubmenu != 0) ? kSubmenuIndent : 0.0f;
    float labelX = row.left + kRowTextInset + indent + nudge;
    if (bSelected)
        DrawTextBold(item.label, labelX, row.top, row.height, kTextScale, kColorWhite);
    else
        DrawText(item.label, labelX, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorText);

    float rightEdge = row.right - kRowTextInset;
    float controlTop = row.top + (row.height - kRowHeight) * 0.5f;   // controls stay centered when row spacing changes
    if (item.type == ITEM_TOGGLE)
        DrawToggle(rightEdge, controlTop, gToggleAnim[itemIndex]);
    else if (item.type == ITEM_SLIDER)
        DrawSlider(rightEdge, controlTop, itemIndex);
    else if (item.type == ITEM_DROPDOWN)
    {
        int value = item.intValue ? *item.intValue : 0;
        if (value < 0 || value >= item.optionCount) value = 0;
        DrawDropdown(rightEdge, controlTop, item.options[value], itemIndex);
    }
}

static void FitTextToWidth(char* text, float scale, float maxWidth)
{
    int length = (int)strlen(text);
    if (TextWidth(text, scale) <= maxWidth)
        return;
    while (length > 1)
    {
        length--;
        text[length] = 0;
        char shortened[48];
        _snprintf(shortened, sizeof(shortened), "%s..", text);
        shortened[sizeof(shortened) - 1] = 0;
        if (TextWidth(shortened, scale) <= maxWidth)
        {
            strcpy(text, shortened);
            return;
        }
    }
}

// Bracketed tag like [H] with a colored bold letter; the letter is centered in a fixed-width slot
static void DrawPlayerTag(const char* letter, const float* color, float x, const RowBounds& row)
{
    float bracketWidth = TextWidth("[", kTextScale);
    float letterSlot = TextWidth("W", kTextScale);
    float letterX = x + bracketWidth + (letterSlot - TextWidth(letter, kTextScale)) * 0.5f;
    DrawText("[", x, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorTagBracket);
    DrawTextBold(letter, letterX, row.top, row.height, kTextScale, color);
    DrawText("]", x + bracketWidth + letterSlot, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorTagBracket);
}

// Host tag then F/E tag, right aligned in fixed columns; returns the left edge of the tag area
static float DrawPlayerBadges(const MenuItem& item, const RowBounds& row)
{
    float tagWidth = TextWidth("[", kTextScale) + TextWidth("W", kTextScale) + TextWidth("]", kTextScale);
    float relationX = row.right - kRowTextInset - tagWidth;
    float hostX = relationX - kPlayerTagGap - tagWidth;
    bool bHasRelation = (item.relation == RELATION_FRIENDLY || item.relation == RELATION_ENEMY);
    if (item.bIsHost)
        DrawPlayerTag("H", kColorTagHost, hostX, row);
    if (item.relation == RELATION_FRIENDLY)
        DrawPlayerTag("F", kColorTagFriend, relationX, row);
    else if (item.relation == RELATION_ENEMY)
        DrawPlayerTag("E", kColorTagEnemy, relationX, row);
    if (item.bIsHost || bHasRelation)
        return hostX;
    return row.right - kRowTextInset;
}

// Slot number on the left, name next to it, role tags (H, F/E) right aligned
static void DrawPlayerRow(const MenuItem& item, bool bSelected, float nudge, const RowBounds& row)
{
    float badgesLeft = DrawPlayerBadges(item, row);

    char slotText[8];
    _snprintf(slotText, sizeof(slotText), "%d", item.slot);
    slotText[sizeof(slotText) - 1] = 0;
    DrawText(slotText, row.left + 12.0f + nudge, row.top, row.height, TEXT_ALIGN_LEFT, kSmallTextScale, kColorMutedText);

    float nameX = row.left + 12.0f + kPlayerSlotWidth + nudge;
    char name[kPlayerNameSize];
    strncpy(name, item.playerName, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    FitTextToWidth(name, kTextScale, badgesLeft - nameX - kPlayerNameGap);
    if (bSelected)
        DrawTextBold(name, nameX, row.top, row.height, kTextScale, kColorWhite);
    else
        DrawText(name, nameX, row.top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorText);
}

// top = row top (layout units); the selection highlight is drawn separately
void DrawRow(int itemIndex, float left, float right, float top)
{
    const MenuItem& item = gItems[itemIndex];
    RowBounds row;
    row.left = left;
    row.right = right;
    row.top = top;
    row.height = ItemHeight(item);

    if (item.type == ITEM_SECTION)
    {
        DrawSectionRow(item, row);
        return;
    }
    if (item.type == ITEM_SEPARATOR)
    {
        DrawSeparatorRow(item, row);
        return;
    }
    if (item.type == ITEM_INFO)
    {
        DrawText(item.label, left + kInfoTextInset, top, row.height, TEXT_ALIGN_LEFT, kTextScale, kColorMutedText);
        return;
    }

    bool bSelected = (itemIndex == gSelectedItem);
    float nudge = kSelectNudgePixels * EaseOutCubic(gSelectAnim[itemIndex]);   // selected label slides right a little
    if (item.type == ITEM_PLAYER)
        DrawPlayerRow(item, bSelected, nudge, row);
    else if (item.type == ITEM_SUBMENU)
        DrawSubmenuRow(item, bSelected, nudge, row);
    else
        DrawControlRow(item, itemIndex, bSelected, nudge, row);
}

void ResetDropdownOverlay()
{
    gbOverlayPending = false;
}

// Drawn last so it covers the rows below the box
void DrawDropdownOverlay()
{
    if (!gbOverlayPending || gOverlayItem < 0)
        return;
    gbOverlayPending = false;
    const MenuItem& item = gItems[gOverlayItem];
    if (item.optionCount <= 0 || !item.intValue)
        return;
    float fullHeight = (float)item.optionCount * kOverlayItemHeight + 2.0f;
    float x = gOverlayX;
    float y = gOverlayY;
    bool bFlipped = false;
    if (y + fullHeight > gPanelHeight - kOverlayEdgeMargin)
    {
        y = gOverlayY - gOverlayBoxHeight - fullHeight;   // flip above the box if it would leave the panel
        bFlipped = true;
    }
    if (y < kOverlayTopMargin) y = kOverlayTopMargin;
    y = floorf(y + 0.5f);
    x = floorf(x + 0.5f);

    // visible height grows from 0 with ease-out, the rest is clipped away
    float eased = EaseOutCubic(gDropdownAnim);
    float visibleHeight = floorf(fullHeight * eased + 0.5f);
    if (visibleHeight < 1.0f)
        return;
    float top = bFlipped ? (y + fullHeight - visibleHeight) : y;   // flipped lists grow upward from the box

    DrawShader(x, top, gOverlayWidth, visibleHeight, kColorControlBox);
    DrawBorder(x, top, gOverlayWidth, visibleHeight, kControlBorderThickness, gAccentColor);
    BeginClip(top, top + visibleHeight, false);

    float savedFade = GetDrawFade();
    SetDrawFade(savedFade * eased);   // items fade in with the expansion
    int currentValue = *item.intValue;

    // hovered-item bar glides between items
    float hoverColor[4];
    MakeColorWithAlpha(hoverColor, gAccentColor, kOverlayHoverAlpha);
    DrawShader(x + 1.0f, y + 1.0f + gDropdownHoverY, gOverlayWidth - 2.0f, kOverlayItemHeight, hoverColor);

    // current-value text: accent lightened toward white
    float currentColor[4] =
    {
        Lerp(gAccentColor[0], 1.0f, kOverlayCurrentLighten),
        Lerp(gAccentColor[1], 1.0f, kOverlayCurrentLighten),
        Lerp(gAccentColor[2], 1.0f, kOverlayCurrentLighten),
        1.0f
    };
    for (int i = 0; i < item.optionCount; i++)
    {
        float optionY = y + 1.0f + (float)i * kOverlayItemHeight;
        const float* textColor = (i == currentValue) ? currentColor : ((i == gDropdownHover) ? kColorWhite : kColorText);
        DrawText(item.options[i], x + kDropdownTextInset, optionY, kOverlayItemHeight, TEXT_ALIGN_LEFT, kDropdownTextScale, textColor);
    }
    SetDrawFade(savedFade);
    EndClip();
}

// Section: Footer button hints

const float kFooterTextScale = 0.66f;

const float kGlyphBadgeSize = 13.0f;
const float kGlyphPillWidth = 23.0f;
const float kGlyphPillHeight = 11.0f;
const float kGlyphPillGap = 2.0f;
const float kGlyphPillTextScale = 0.46f;
const float kGlyphDpadSize = 13.0f;
const float kGlyphLabelGap = 4.0f;
const float kGlyphEntryGap = 12.0f;
const float kGlyphIconGap = 2.0f;

const float kColorGlyphA[4]          = { 0.45f, 0.75f, 0.20f, 1.00f };
const float kColorGlyphB[4]          = { 0.85f, 0.20f, 0.20f, 1.00f };
const float kColorGlyphX[4]          = { 0.20f, 0.45f, 0.85f, 1.00f };
const float kColorGlyphY[4]          = { 0.90f, 0.75f, 0.15f, 1.00f };
const float kColorGlyphPill[4]       = { 0.35f, 0.35f, 0.38f, 1.00f };
const float kColorGlyphPillBorder[4] = { 0.62f, 0.62f, 0.66f, 1.00f };
const float kColorGlyphDpad[4]       = { 0.75f, 0.75f, 0.78f, 1.00f };

enum FooterGlyph { FOOTER_GLYPH_A, FOOTER_GLYPH_B, FOOTER_GLYPH_X, FOOTER_GLYPH_Y, FOOTER_GLYPH_BUMPERS, FOOTER_GLYPH_DPAD };

struct FooterEntry { FooterGlyph glyph; const char* label; };

const FooterEntry kFooterNormal[] =
{
    { FOOTER_GLYPH_BUMPERS, "Tabs" }, { FOOTER_GLYPH_DPAD, "Navigate" }, { FOOTER_GLYPH_A, "Select" }, { FOOTER_GLYPH_B, "Back" }
};
const FooterEntry kFooterOverlay[] =
{
    { FOOTER_GLYPH_A, "Confirm" }, { FOOTER_GLYPH_B, "Cancel" }
};
const int kFooterNormalCount = (int)(sizeof(kFooterNormal) / sizeof(kFooterNormal[0]));
const int kFooterOverlayCount = (int)(sizeof(kFooterOverlay) / sizeof(kFooterOverlay[0]));

// Icons making up a glyph; returns the count
static int GetGlyphIcons(FooterGlyph glyph, IconId* icons)
{
    switch (glyph)
    {
    case FOOTER_GLYPH_A: icons[0] = ICON_A; return 1;
    case FOOTER_GLYPH_B: icons[0] = ICON_B; return 1;
    case FOOTER_GLYPH_X: icons[0] = ICON_X; return 1;
    case FOOTER_GLYPH_Y: icons[0] = ICON_Y; return 1;
    case FOOTER_GLYPH_BUMPERS: icons[0] = ICON_LB; icons[1] = ICON_RB; return 2;
    case FOOTER_GLYPH_DPAD: icons[0] = ICON_DPAD_UP; icons[1] = ICON_DPAD_DOWN; return 2;
    }
    return 0;
}

static bool AreGlyphIconsReady(FooterGlyph glyph)
{
    IconId icons[2];
    int count = GetGlyphIcons(glyph, icons);
    if (count == 0)
        return false;
    for (int i = 0; i < count; i++)
        if (!IsIconReady(icons[i]))
            return false;
    return true;
}

static float GlyphIconsWidth(FooterGlyph glyph)
{
    IconId icons[2];
    int count = GetGlyphIcons(glyph, icons);
    float width = 0.0f;
    for (int i = 0; i < count; i++)
    {
        width += IconWidth(icons[i]);
        if (i + 1 < count) width += kGlyphIconGap;
    }
    return width;
}

static void DrawGlyphIcons(FooterGlyph glyph, float x, float centerY)
{
    IconId icons[2];
    int count = GetGlyphIcons(glyph, icons);
    for (int i = 0; i < count; i++)
    {
        DrawIcon(icons[i], x, centerY);
        x += IconWidth(icons[i]) + kGlyphIconGap;
    }
}

static float FooterLabelWidth(const char* label)
{
    return TextWidth(label, kFooterTextScale);
}

static float FooterGlyphWidth(FooterGlyph glyph)
{
    if (AreGlyphIconsReady(glyph)) return GlyphIconsWidth(glyph);
    if (glyph == FOOTER_GLYPH_BUMPERS) return kGlyphPillWidth * 2.0f + kGlyphPillGap;
    if (glyph == FOOTER_GLYPH_DPAD) return kGlyphDpadSize;
    return kGlyphBadgeSize;
}

static void DrawBumper(float x, float y, const char* label)
{
    DrawShader(x, y, kGlyphPillWidth, kGlyphPillHeight, kColorGlyphPill);
    DrawBorder(x, y, kGlyphPillWidth, kGlyphPillHeight, 1.0f, kColorGlyphPillBorder);
    DrawText(label, x + kGlyphPillWidth * 0.5f, y, kGlyphPillHeight, TEXT_ALIGN_CENTER, kGlyphPillTextScale, kColorWhite);
}

static void DrawDpadGlyph(float x, float y)
{
    const float size = kGlyphDpadSize;
    const float arm = floorf(size / 3.0f + 0.5f);   // arm thickness
    const float offset = floorf((size - arm) * 0.5f + 0.5f);
    DrawShader(x + offset, y, arm, size, kColorGlyphDpad);
    DrawShader(x, y + offset, size, arm, kColorGlyphDpad);
}

static void DrawGlyph(FooterGlyph glyph, float x, float centerY)
{
    if (AreGlyphIconsReady(glyph))
    {
        DrawGlyphIcons(glyph, x, centerY);
        return;
    }
    const float badgeY = centerY - kGlyphBadgeSize * 0.5f;
    const float pillY = centerY - kGlyphPillHeight * 0.5f;
    switch (glyph)
    {
    case FOOTER_GLYPH_A: DrawBadge(x, badgeY, kGlyphBadgeSize, "A", kColorGlyphA); break;
    case FOOTER_GLYPH_B: DrawBadge(x, badgeY, kGlyphBadgeSize, "B", kColorGlyphB); break;
    case FOOTER_GLYPH_X: DrawBadge(x, badgeY, kGlyphBadgeSize, "X", kColorGlyphX); break;
    case FOOTER_GLYPH_Y: DrawBadge(x, badgeY, kGlyphBadgeSize, "Y", kColorGlyphY); break;
    case FOOTER_GLYPH_BUMPERS:
        DrawBumper(x, pillY, "LB");
        DrawBumper(x + kGlyphPillWidth + kGlyphPillGap, pillY, "RB");
        break;
    case FOOTER_GLYPH_DPAD: DrawDpadGlyph(x, centerY - kGlyphDpadSize * 0.5f); break;
    }
}

void DrawFooter(float centerX, float top, float height)
{
    ResolveIcons();
    const FooterEntry* entries = gbDropdownOpen ? kFooterOverlay : kFooterNormal;
    int count = gbDropdownOpen ? kFooterOverlayCount : kFooterNormalCount;

    float totalWidth = 0.0f;
    for (int i = 0; i < count; i++)
    {
        totalWidth += FooterGlyphWidth(entries[i].glyph) + kGlyphLabelGap + FooterLabelWidth(entries[i].label);
        if (i + 1 < count) totalWidth += kGlyphEntryGap;
    }

    // whole-pixel start x so every entry stays on the pixel grid
    float x = floorf(centerX - totalWidth * 0.5f + 0.5f);
    float centerY = top + height * 0.5f;
    for (int i = 0; i < count; i++)
    {
        DrawGlyph(entries[i].glyph, x, centerY);
        x += FooterGlyphWidth(entries[i].glyph) + kGlyphLabelGap;
        float labelWidth = FooterLabelWidth(entries[i].label);
        DrawText(entries[i].label, x, top, height, TEXT_ALIGN_LEFT, kFooterTextScale, kColorMutedText);
        x += labelWidth + kGlyphEntryGap;
    }
}

const float kInfoWidth = 290.0f;
const float kInfoRowHeight = 23.0f;
const float kInfoMargin = 14.0f;
const float kInfoSlidePixels = 60.0f;
const float kInfoRowsTop = 40.0f;
const float kInfoBottomPadding = 8.0f;
const float kInfoTitleY = 3.0f;
const float kInfoTitleHeight = 32.0f;
const float kInfoTitleLineY = 35.0f;
const float kServerInfoTextInset = 10.0f;
const int kInfoRowCount = 6;
const int kHostNameMaxChars = 22;

const float kFpsCounterMargin = 14.0f;
const float kFpsCounterHeight = 26.0f;

const int kGoodFps = 60;
const int kAcceptableFps = 30;
const int kGoodPing = 80;
const int kAcceptablePing = 150;
const int kMaxPlausiblePing = 999;

const DWORD kPingMinuendOffset = 88;
const DWORD kPingSubtrahendOffset = 10908;

struct NameEntry { const char* code; const char* name; };

const NameEntry kMapNames[] =
{
    { "mp_nuketown_2020", "Nuketown 2025" },
    { "mp_hijacked", "Hijacked" },
    { "mp_raid", "Raid" },
    { "mp_slums", "Slums" },
    { "mp_village", "Standoff" },
    { "mp_turbine", "Turbine" },
    { "mp_socotra", "Yemen" },
    { "mp_nuked", "Nuketown" },
    { "mp_overflow", "Overflow" },
    { "mp_metropolis", "Metropolis" },
    { "mp_dockside", "Cargo" },
    { "mp_carrier", "Carrier" },
    { "mp_drone", "Drone" },
    { "mp_express", "Express" },
    { "mp_studio", "Studio" },
    { "mp_vertigo", "Vertigo" },
    { "mp_la", "Aftermath" },
    { "mp_uplink", "Uplink" },
    { "mp_castaway", "Cove" },
    { "mp_downhill", "Downhill" },
    { "mp_magma", "Magma" },
    { "mp_frostbite", "Frost" },
    { "mp_takeoff", "Takeoff" },
    { "mp_pod", "Pod" },
    { "mp_hydro", "Hydro" },
    { "mp_bridge", "Detour" },
    { "mp_dig", "Dig" },
    { "mp_skate", "Grind" },
    { "mp_prison", "Detention" },
    { "mp_concert", "Encore" }
};
const int kMapNameCount = (int)(sizeof(kMapNames) / sizeof(kMapNames[0]));

const NameEntry kModeNames[] =
{
    { "tdm", "Team Deathmatch" }, 
    { "dm", "Free-for-All" }, 
    { "dom", "Domination" },
    { "sd", "Search & Destroy" }, 
    { "hq", "Headquarters" }, 
    { "koth", "Hardpoint" },
    { "ctf", "Capture the Flag" }, 
    { "conf", "Kill Confirmed" }, 
    { "dem", "Demolition" },
    { "sas", "Sticks & Stones" }, 
    { "gun", "Gun Game" }, 
    { "oic", "One in the Chamber" },
    { "shrp", "Sharpshooter" }, 
    { "sr", "Search & Rescue" }, 
    { "clean", "Clean" },
    { "twar", "War" }, 
    { "hack", "Hacker" }
};

const int kModeNameCount = (int)(sizeof(kModeNames) / sizeof(kModeNames[0]));

static const char* LookupName(const NameEntry* table, int count, const char* code)
{
    for (int i = 0; i < count; i++)
        if (_stricmp(table[i].code, code) == 0)
            return table[i].name;
    return code;
}

static void* FindDvar(const char* name)
{
    void* dvar = Dvar_FindVar(name);
    if (!dvar || !MmIsAddressValid(dvar))
        return NULL;
    return dvar;
}

static const char* GetDvarString(const char* name)
{
    void* dvar = FindDvar(name);
    if (!dvar)
        return "";
    const char* value = Dvar_GetString(dvar);
    if (!value || !MmIsAddressValid((PVOID)value))
        return "";
    return value;
}

static int GetFps()
{
    // 0x82BC2AE4 = last frame time in ms
    if (!MmIsAddressValid((PVOID)0x82BC2AE4))
        return 0;
    float frameTimeMs = *(volatile float*)0x82BC2AE4;
    if (frameTimeMs < 0.1f)
        return 0;
    return (int)floorf(1000.0f / frameTimeMs + 0.5f);
}

static int GetPing()
{
    DWORD clientActive = *(volatile DWORD*)0x82C70F4C; // client-active pointer
    if (!clientActive || !MmIsAddressValid((PVOID)(clientActive + kPingSubtrahendOffset)))
        return -1;
    int ping = *(int*)(clientActive + kPingMinuendOffset) - *(int*)(clientActive + kPingSubtrahendOffset);
    if (ping < 0 || ping > kMaxPlausiblePing)
        return -1;
    return ping;
}

static void DrawInfoRow(int rowIndex, const char* label, const char* value, const float* valueColor)
{
    float y = kInfoRowsTop + (float)rowIndex * kInfoRowHeight;
    char labelText[48];
    _snprintf(labelText, sizeof(labelText), "%s:", label);
    labelText[sizeof(labelText) - 1] = 0;
    DrawText(labelText, kServerInfoTextInset, y, kInfoRowHeight, TEXT_ALIGN_LEFT, kSmallTextScale, kColorMutedText);
    DrawText(value, kInfoWidth - kServerInfoTextInset, y, kInfoRowHeight, TEXT_ALIGN_RIGHT, kSmallTextScale, valueColor);
}

static const float* GetFpsColor(int fps)
{
    return fps >= kGoodFps ? kColorGreen : (fps >= kAcceptableFps ? kColorLime : kColorRed);
}

static const float* GetPingColor(int ping)
{
    if (ping < 0) return kColorMutedText;
    return ping < kGoodPing ? kColorGreen : (ping < kAcceptablePing ? kColorYellow : kColorRed);
}

static void FormatPlayerCount(char* buffer, int bufferSize)
{
    GamePlayers players;
    int maxClients = GetGameMaxClients();
    bool bHaveCount = ReadGamePlayers(&players) && players.count > 0;
    if (bHaveCount && maxClients > 0)
        _snprintf(buffer, bufferSize, "%d/%d", players.count, maxClients);
    else if (bHaveCount)
        _snprintf(buffer, bufferSize, "%d", players.count);
    else if (maxClients > 0)
        _snprintf(buffer, bufferSize, "?/%d", maxClients);
    else
        _snprintf(buffer, bufferSize, "?");
    buffer[bufferSize - 1] = 0;
}

void DrawServerInfo()
{
    float boxHeight = kInfoRowsTop + (float)kInfoRowCount * kInfoRowHeight + kInfoBottomPadding;
    float scale = GetDrawScale();
    float slideProgress = EaseOutCubic(gInfoProgress);   // fade + slide in from the right edge
    SetDrawFade(slideProgress);
    SetDrawOrigin(floorf(kScreenWidth - kInfoWidth * scale - kInfoMargin + (1.0f - slideProgress) * kInfoSlidePixels * scale + 0.5f),
        floorf(kInfoMargin + 0.5f));

    float background[4];
    MakeColorWithAlpha(background, kColorBackground, kColorBackground[3] * gOpacity);
    DrawShader(0.0f, 0.0f, kInfoWidth, boxHeight, background);
    DrawBorder(0.0f, 0.0f, kInfoWidth, boxHeight, kBorderThickness, gAccentColor);
    DrawText("SERVER INFO", kInfoWidth * 0.5f, kInfoTitleY, kInfoTitleHeight, TEXT_ALIGN_CENTER, kTextScale, gAccentColor);
    DrawShader(kBorderThickness, kInfoTitleLineY, kInfoWidth - kBorderThickness * 2.0f, 1.0f, gAccentColor);

    char text[96];

    int fps = GetFps();
    _snprintf(text, sizeof(text), "%d", fps);
    text[sizeof(text) - 1] = 0;
    DrawInfoRow(0, "FPS", text, GetFpsColor(fps));

    const char* hostName = GetDvarString("sv_hostname");
    if (!hostName[0]) hostName = GetDvarString("party_hostname");
    if (!hostName[0]) hostName = "Unknown";
    _snprintf(text, sizeof(text), "%.*s", kHostNameMaxChars, hostName);
    text[sizeof(text) - 1] = 0;
    DrawInfoRow(1, "Host", text, kColorWhite);

    const char* mapCode = GetDvarString("mapname");
    DrawInfoRow(2, "Map", mapCode[0] ? LookupName(kMapNames, kMapNameCount, mapCode) : "Unknown", kColorWhite);

    const char* modeCode = GetDvarString("g_gametype");
    DrawInfoRow(3, "Mode", modeCode[0] ? LookupName(kModeNames, kModeNameCount, modeCode) : "Unknown", kColorWhite);

    FormatPlayerCount(text, sizeof(text));
    DrawInfoRow(4, "Players", text, kColorWhite);

    int ping = GetPing();
    if (ping >= 0)
        _snprintf(text, sizeof(text), "%dms", ping);
    else
        _snprintf(text, sizeof(text), "N/A");
    text[sizeof(text) - 1] = 0;
    DrawInfoRow(5, "Ping", text, GetPingColor(ping));
}

void DrawFpsCounter()
{
    SetDrawOrigin(kFpsCounterMargin, kFpsCounterMargin);
    char text[32];
    _snprintf(text, sizeof(text), "FPS: %d", GetFps());
    text[sizeof(text) - 1] = 0;
    DrawText(text, 0.0f, 0.0f, kFpsCounterHeight, TEXT_ALIGN_LEFT, kTextScale, kColorGreen, 3); // style 3 = shadowed
}

const float kScrollbarWidth = 5.0f;
const float kScrollThumbMinHeight = 20.0f;
const float kListGap = 8.0f;
const float kListRightGap = 4.0f;
const float kFooterHeight = 28.0f;
const float kFooterTopGap = 4.0f;
const float kFooterBottomGap = 6.0f;
const float kTabBarTopGap = 7.0f;
const float kTabInset = 2.0f;

const float kTitleScale = 1.40f;
const float kTitleY = 6.0f;
const float kTitleTextHeight = 36.0f;
const float kSubtitleY = 39.0f;
const float kSubtitleTextHeight = 22.0f;

const float kOpenScaleFrom = 0.92f;
const float kOpenSlidePixels = 12.0f;
const float kMinVisiblePanelPixels = 60.0f;
const float kTabSlidePixels = 20.0f;
const float kHighlightPulseDip = 0.45f;
const float kHighlightAlpha = 0.22f;
const float kHighlightEdgeWidth = 2.0f;

struct ListArea { float left, right, top, height; };

static void ApplyPanelTransform(float baseScale)
{
    float scaledWidth = gPanelWidth * baseScale, scaledHeight = gPanelHeight * baseScale;
    float originX = (kScreenWidth - scaledWidth) * 0.5f + PositionX;
    float originY = (kScreenHeight - scaledHeight) * 0.5f + PositionY;
    originX = ClampFloat(originX, kMinVisiblePanelPixels - scaledWidth, kScreenWidth - kMinVisiblePanelPixels);
    originY = ClampFloat(originY, kMinVisiblePanelPixels - scaledHeight, kScreenHeight - kMinVisiblePanelPixels);
    float centerX = originX + scaledWidth * 0.5f;
    float centerY = originY + scaledHeight * 0.5f;

    float eased = EaseOutCubic(gOpenProgress);
    float scale = baseScale * Lerp(kOpenScaleFrom, 1.0f, eased);
    SetDrawFade(eased);
    SetDrawScale(scale);
    SetDrawOrigin(floorf(centerX - gPanelWidth * scale * 0.5f + 0.5f),
        floorf(centerY - gPanelHeight * scale * 0.5f + (1.0f - eased) * kOpenSlidePixels * baseScale + 0.5f));
}

static void PrepareFrameState()
{
    ResetDropdownOverlay();
    EndClip();
    if (gbDropdownOpen && (gSelectedItem < 0 || gItems[gSelectedItem].type != ITEM_DROPDOWN))
        gbDropdownOpen = false;

    BuildVisibleList(false);
    if (gSelectedItem < 0 || FindVisiblePosition(gSelectedItem) < 0)
        SelectFirstItem();

    SnapViewIfNeeded();
    if (gbDropdownOpen)
        gDropdownItem = gSelectedItem;
    UpdateItemAnimations();
    UpdateTabTransition();
}

static void DrawPanelFrame()
{
    float background[4];
    MakeColorWithAlpha(background, kColorBackground, kColorBackground[3] * gOpacity);
    DrawShader(0.0f, 0.0f, gPanelWidth, gPanelHeight, background);
    DrawBorder(0.0f, 0.0f, gPanelWidth, gPanelHeight, kBorderThickness, gAccentColor);

    float titleOffset = (gTitleHeight - kTitleHeight) * 0.5f;
    // style 3 = shadowed
    DrawText("Vortex Framework 1.2", gPanelWidth * 0.5f, kTitleY + titleOffset, kTitleTextHeight, TEXT_ALIGN_CENTER, kTitleScale, gAccentColor, 3, FONT_TITLE);
    DrawText("Menu Base by Baked_Mike", gPanelWidth * 0.5f, kSubtitleY + titleOffset, kSubtitleTextHeight, TEXT_ALIGN_CENTER, kSmallTextScale, kColorMutedText);

    // full width, same color and thickness as the border so it joins left and right edges
    DrawShader(0.0f, gTitleHeight, gPanelWidth, kBorderThickness, gAccentColor);
}

static void DrawTabBar(float tabY)
{
    float tabWidth = (gPanelWidth - gContentPadding * 2.0f) / (float)TAB_COUNT;
    UpdateTabIndicator(gContentPadding + tabWidth * (float)gCurrentTab + kTabInset, tabWidth - 2.0f * kTabInset);
    DrawShader(gTabIndicatorX, tabY, gTabIndicatorWidth, gTabBarHeight, gAccentColor);
    for (int tab = 0; tab < TAB_COUNT; tab++)
    {
        float tabX = gContentPadding + tabWidth * (float)tab;
        DrawText(TabName(tab), tabX + tabWidth * 0.5f, tabY, gTabBarHeight, TEXT_ALIGN_CENTER, kTextScale,
            (tab == gCurrentTab) ? kColorWhite : kColorMutedText);
    }
}

static void DrawHighlight(const ListArea& list, float scrollOffset, float slideX)
{
    float highlightY = list.top + gHighlightY - scrollOffset;
    float color[4];
    MakeColorWithAlpha(color, gAccentColor, kHighlightAlpha * (1.0f - kHighlightPulseDip * gHighlightPulse));
    DrawShader(list.left + slideX, highlightY, list.right - list.left, gHighlightHeight, color);
    DrawShader(list.left + slideX, highlightY, kHighlightEdgeWidth, gHighlightHeight, gAccentColor);
}

static void DrawRows(const ListArea& list, float scrollOffset, float slideX, float contentFade)
{
    for (int i = 0; i < gVisibleCount; i++)
    {
        const MenuItem& item = gItems[gVisibleItems[i]];
        float cellHeight = gRowHeights[i];
        if (cellHeight < 0.5f)
            continue;
        float rowY = list.top + gRowTop[i] - scrollOffset;
        if (rowY + cellHeight <= list.top || rowY >= list.top + list.height)
            continue;
        float fullHeight = ItemHeight(item);
        float rowAlpha = (item.parentSubmenu != 0 && fullHeight > 0.0f) ? (cellHeight / fullHeight) : 1.0f;   // children fade in while growing
        BeginClip(rowY, rowY + cellHeight, true);
        SetDrawFade(contentFade * rowAlpha);
        DrawRow(gVisibleItems[i], list.left + slideX, list.right + slideX, rowY);
    }
}

static void DrawScrollbar(const ListArea& list, float scrollOffset)
{
    float barX = gPanelWidth - gContentPadding - kScrollbarWidth;
    DrawShader(barX, list.top, kScrollbarWidth, list.height, kColorScrollTrack);
    if (gContentHeight > list.height)
    {
        float thumbHeight = list.height * (list.height / gContentHeight);
        if (thumbHeight < kScrollThumbMinHeight) thumbHeight = kScrollThumbMinHeight;
        float fraction = scrollOffset / MaxScrollOffset(list.height);
        if (fraction > 1.0f) fraction = 1.0f;
        if (fraction < 0.0f) fraction = 0.0f;
        float thumbY = floorf(list.top + (list.height - thumbHeight) * fraction + 0.5f);
        DrawShader(barX, thumbY, kScrollbarWidth, floorf(thumbHeight + 0.5f), gAccentColor);
    }
    else
        DrawShader(barX, list.top, kScrollbarWidth, list.height, gAccentColor);
}

static void DrawList(const ListArea& list)
{
    ComputeRowMetrics();
    int selectedPosition = FindVisiblePosition(gSelectedItem);
    float scrollOffset = UpdateScroll(list.height, selectedPosition);
    UpdateHighlight(selectedPosition);

    float transition = EaseOutCubic(gTabTransition);
    float slideX = (float)gTabDirection * kTabSlidePixels * (1.0f - transition);
    float baseFade = GetDrawFade();
    float contentFade = baseFade * transition;

    BeginClip(list.top, list.top + list.height, false);
    SetDrawFade(contentFade);
    if (selectedPosition >= 0)
        DrawHighlight(list, scrollOffset, slideX);
    DrawRows(list, scrollOffset, slideX, contentFade);
    SetDrawFade(baseFade);
    EndClip();

    DrawScrollbar(list, scrollOffset);
}

void DrawPanel()
{
    float baseScale = GetDrawScale();
    ApplyPanelTransform(baseScale);
    PrepareFrameState();
    DrawPanelFrame();

    float tabY = gTitleHeight + kTabBarTopGap;
    DrawTabBar(tabY);

    ListArea list;
    list.left = gContentPadding;
    list.right = gPanelWidth - gContentPadding - kScrollbarWidth - kListRightGap;
    list.top = tabY + gTabBarHeight + kListGap;
    list.height = gPanelHeight - list.top - kFooterHeight - kFooterBottomGap;
    SetListClipBounds(list.top, list.height);
    DrawList(list);

    DrawFooter(gPanelWidth * 0.5f, gPanelHeight - kFooterHeight - kFooterTopGap, kFooterHeight);
    DrawDropdownOverlay();

    BuildVisibleList(false);
    SetDrawScale(baseScale);
}

static void UpdateLayout()
{
    gPanelWidth = MenuWidth;
    gTitleHeight = HeaderHeight;
    gTabBarHeight = TabsHeight;
    gRowExtraHeight = OptionSpacing - kDefaultOptionSpacing;
    gContentPadding = kPanelPadding + (ContentPadding - kDefaultContentPadding);
    if (gContentPadding < 0.0f) gContentPadding = 0.0f;
    gPanelHeight = kPanelHeight + (gTitleHeight - kTitleHeight) + (gTabBarHeight - kTabBarHeight);

    float scale = MenuScale;
    float maxScaleForWidth = kScreenWidth / gPanelWidth;
    float maxScaleForHeight = kScreenHeight / gPanelHeight;
    if (scale > maxScaleForWidth) scale = maxScaleForWidth;
    if (scale > maxScaleForHeight) scale = maxScaleForHeight;
    SetDrawScale(scale);
}

static void UpdateTheme()
{
    const float* rgb = kAccentColors[MenuColor];
    gAccentColor[0] = rgb[0];
    gAccentColor[1] = rgb[1];
    gAccentColor[2] = rgb[2];
    gAccentColor[3] = 1.0f;
    gOpacity = kOpacityLevels[MenuOpacity];
}

static void DrawAll()
{
    SetDrawFade(1.0f);
    if (bShowFps)
        DrawFpsCounter();
    if (gInfoProgress > 0.001f)
        DrawServerInfo();
    SetDrawFade(1.0f);
    if (gbMenuOpen || gOpenProgress > 0.0f)
        DrawPanel();
    SetDrawFade(1.0f);
}

void RunFrame()
{
    RebuildItems();
    HandleInput();
    gMenuBlocksInput = (gbMenuOpen || gOpenProgress > 0.0f) ? 1 : 0;

    InitDrawResources();
    if (!AreDrawResourcesReady())
        return;

    UpdateTheme();
    UpdateLayout();
    BeginAnimationFrame();
    UpdateOpenAnimation();
    UpdateInfoAnimation();
    DrawAll();
}
