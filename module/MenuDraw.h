#pragma once

// Low-level rendering: every engine draw call of the menu goes through here
enum TextAlign { TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT };
enum FontId { FONT_BODY, FONT_TITLE };
enum IconId { ICON_A, ICON_B, ICON_X, ICON_Y, ICON_LB, ICON_RB, ICON_DPAD_UP, ICON_DPAD_DOWN, ICON_COUNT };

// Resources (call once per frame before drawing)
void InitDrawResources();
bool AreDrawResourcesReady();

// Transform / fade state (positions passed to the Draw functions are layout units)
void  SetDrawOrigin(float x, float y);
void  SetDrawScale(float scale);
float GetDrawScale();
void  SetDrawFade(float fade);
float GetDrawFade();

// Vertical clip in layout units; bLimitToList also intersects with the list area
void SetListClipBounds(float layoutTop, float layoutHeight);
void BeginClip(float layoutTop, float layoutBottom, bool bLimitToList);
void EndClip();

void MakeColorWithAlpha(float* outColor, const float* rgb, float alpha);

// Primitives
void DrawShader(float x, float y, float width, float height, const float* color);
void DrawBorder(float x, float y, float width, float height, float thickness, const float* color);

// Text is centered vertically inside [y, y + height]; returns the width in physical pixels
float DrawText(const char* text, float x, float y, float height, TextAlign align, float scale, const float* color,
    int style = 0, FontId font = FONT_BODY); // style 0 = normal, 3 = shadowed
float DrawTextBold(const char* text, float x, float y, float height, float scale, const float* color);

// Small rounded badge with a centered white letter; width 0 makes it square
void DrawBadge(float x, float y, float size, const char* letter, const float* color, float width = 0.0f);

// Width in layout units / height in pixels
float TextWidth(const char* text, float scale, FontId font = FONT_BODY);
int   TextHeight(FontId font = FONT_BODY);

// Game button icons
void  ResolveIcons();
bool  IsIconReady(IconId icon);
float IconWidth(IconId icon);
void  DrawIcon(IconId icon, float x, float centerY);
