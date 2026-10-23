#include "stdafx.h"

// Text scales are relative to this height (layout units), independent of the engine font
const float kReferenceFontHeight = 18.0f;
const float kBoldOffset = 0.6f;
const float kIconHeight = 16.0f;
const int kDefaultFontPixelHeight = 16;
const float kFallbackCharWidthFactor = 0.5f;

const float kBadgeCornerRatio = 0.1875f;   // 3 px corner cut at 16 px
const float kBadgeLetterScale = 0.66f;
const float kBadgeTextPadding = 4.0f;

const float kIconRatioMin = 0.01f;
const float kIconRatioMax = 16.0f;

const DWORD kBodyFontAddress = 0x83BABE0C; // engine global holding the default UI font pointer
const DWORD kTitleFontAddress = 0x83BABE0C; // same default UI font global
const char* const kFallbackFontName = "fonts/smallDevFont";
const char* const kSecondFallbackFontName = "fonts/consoleFont";

static char gIconFontNames[ICON_COUNT][24] =
{
    "XENONButtonA", "XENONButtonB", "XENONButtonX", "XENONButtonY", "XENONButtonshoulderL", "XENONButtonshoulderR", "XENONButtondpadU", "XENONButtondpadD"
};
const char* const kIconTextEscapes[ICON_COUNT] =
{
    "^BXENONButtonA^", "^BXENONButtonB^", "^BXENONButtonX^", "^BXENONButtonY^", "^BXENONButtonshoulderL^", "^BXENONButtonshoulderR^", "^BXENONButtondpadU^", "^BXENONButtondpadD^"
};

static Material* gWhiteMaterial = NULL;
static Material* gDefaultMaterial = NULL;
static Font_s* gBodyFont = NULL;
static Font_s* gTitleFont = NULL;
static Font_s* gFallbackFont = NULL;

static bool  gbIconReady[ICON_COUNT];
static float gIconWidthRatio[ICON_COUNT];
static float gIconHeightRatio[ICON_COUNT];

static float gDrawScale = 1.0f;
static float gDrawOriginX = 0.0f;
static float gDrawOriginY = 0.0f;
static float gDrawFade = 1.0f;

static bool gbClipActive = false;
static int  gClipTop = 0, gClipBottom = 0;
static int  gListTop = 0, gListBottom = 0;

static int SnapToPixel(float value)
{
    return (int)floorf(value + 0.5f);
}

static int FontPixelHeight(Font_s* font)
{
    int height = R_TextHeight(font);
    return height > 0 ? height : kDefaultFontPixelHeight;
}

static Font_s* GetFont(FontId fontId)
{
    return fontId == FONT_TITLE ? gTitleFont : gBodyFont;
}

static Font_s* ReadUiFont(DWORD address)
{
    if (!MmIsAddressValid((PVOID)address))
        return NULL;
    Font_s* font = *(Font_s* volatile*)address;
    if (!font || !MmIsAddressValid((PVOID)font))
        return NULL;
    return font;
}

// Width in physical pixels for an already scaled text size
static float MeasureTextPixels(const char* text, Font_s* font, float textScale)
{
    if (!font || !text)
        return 0.0f;
    int width = R_TextWidth(0, text, 0x7FFFFFFF, font);
    if (width <= 0)
        return (float)strlen(text) * (float)FontPixelHeight(font) * kFallbackCharWidthFactor * textScale;
    return (float)width * textScale;
}

// Text cannot be clipped, so it fades by the visible fraction of its row cell
static float VisibleFractionInClip(float layoutY, float layoutHeight)
{
    if (!gbClipActive)
        return 1.0f;
    float cellTop = gDrawOriginY + layoutY * gDrawScale;
    float cellHeight = layoutHeight * gDrawScale;
    if (cellHeight <= 0.5f)
        return 1.0f;
    float top = (cellTop > (float)gClipTop) ? cellTop : (float)gClipTop;
    float bottom = (cellTop + cellHeight < (float)gClipBottom) ? (cellTop + cellHeight) : (float)gClipBottom;
    if (bottom <= top)
        return 0.0f;
    return (bottom - top) / cellHeight;
}

// Rect from integer physical pixel edges, never zero sized
static void DrawSnappedRect(int left, int top, int right, int bottom, const float* color)
{
    if (!gWhiteMaterial)
        return;
    if (right <= left) right = left + 1;
    if (bottom <= top) bottom = top + 1;
    if (gbClipActive)
    {
        if (top < gClipTop) top = gClipTop;
        if (bottom > gClipBottom) bottom = gClipBottom;
        if (bottom <= top) return;
    }
    float fadedColor[4] = { color[0], color[1], color[2], color[3] * gDrawFade };
    CL_DrawStretchPicPhysical((float)left, (float)top, (float)(right - left), (float)(bottom - top),
        0.0f, 0.0f, 1.0f, 1.0f, fadedColor, gWhiteMaterial);
}

void InitDrawResources()
{
    if (!gWhiteMaterial)
        gWhiteMaterial = Material_RegisterHandle("white", 0, false, -1);

    // Fonts are re-read every frame, the engine globals can be filled in later
    gBodyFont = ReadUiFont(kBodyFontAddress);
    gTitleFont = ReadUiFont(kTitleFontAddress);
    if (!gBodyFont || !gTitleFont)
    {
        if (!gFallbackFont)
            gFallbackFont = CL_RegisterFont(kFallbackFontName);
        if (!gFallbackFont)
            gFallbackFont = CL_RegisterFont(kSecondFallbackFontName);
        if (!gBodyFont) gBodyFont = gFallbackFont;
        if (!gTitleFont) gTitleFont = gBodyFont;
    }
}

bool AreDrawResourcesReady()
{
    return gWhiteMaterial != NULL && gBodyFont != NULL;
}

void SetDrawOrigin(float x, float y)
{
    gDrawOriginX = x;
    gDrawOriginY = y;
}

void SetDrawScale(float scale) { gDrawScale = scale; }
float GetDrawScale() { return gDrawScale; }
void SetDrawFade(float fade) { gDrawFade = fade; }
float GetDrawFade() { return gDrawFade; }

void SetListClipBounds(float layoutTop, float layoutHeight)
{
    gListTop = SnapToPixel(gDrawOriginY + layoutTop * gDrawScale);
    gListBottom = SnapToPixel(gDrawOriginY + (layoutTop + layoutHeight) * gDrawScale);
}

void BeginClip(float layoutTop, float layoutBottom, bool bLimitToList)
{
    int top = SnapToPixel(gDrawOriginY + layoutTop * gDrawScale);
    int bottom = SnapToPixel(gDrawOriginY + layoutBottom * gDrawScale);
    if (bLimitToList)
    {
        if (top < gListTop) top = gListTop;
        if (bottom > gListBottom) bottom = gListBottom;
    }
    gClipTop = top;
    gClipBottom = bottom;
    gbClipActive = true;
}

void EndClip()
{
    gbClipActive = false;
}

void MakeColorWithAlpha(float* outColor, const float* rgb, float alpha)
{
    outColor[0] = rgb[0];
    outColor[1] = rgb[1];
    outColor[2] = rgb[2];
    outColor[3] = alpha;
}

void DrawShader(float x, float y, float width, float height, const float* color)
{
    if (!gWhiteMaterial || width <= 0.0f || height <= 0.0f)
        return;
    float pixelX = gDrawOriginX + x * gDrawScale;
    float pixelY = gDrawOriginY + y * gDrawScale;
    DrawSnappedRect(SnapToPixel(pixelX), SnapToPixel(pixelY),
        SnapToPixel(pixelX + width * gDrawScale), SnapToPixel(pixelY + height * gDrawScale), color);
}

// Snapped outer edges, top/bottom full width, left/right between them (no overlap)
void DrawBorder(float x, float y, float width, float height, float thickness, const float* color)
{
    if (!gWhiteMaterial || width <= 0.0f || height <= 0.0f)
        return;
    float pixelX = gDrawOriginX + x * gDrawScale;
    float pixelY = gDrawOriginY + y * gDrawScale;
    int left = SnapToPixel(pixelX), right = SnapToPixel(pixelX + width * gDrawScale);
    int top = SnapToPixel(pixelY), bottom = SnapToPixel(pixelY + height * gDrawScale);
    if (right <= left) right = left + 1;
    if (bottom <= top) bottom = top + 1;
    int edge = (int)(thickness * gDrawScale + 0.5f);
    if (edge < 1) edge = 1;
    if (2 * edge >= (bottom - top) || 2 * edge >= (right - left))
    {
        DrawSnappedRect(left, top, right, bottom, color);
        return;
    }
    DrawSnappedRect(left, top, right, top + edge, color);
    DrawSnappedRect(left, bottom - edge, right, bottom, color);
    DrawSnappedRect(left, top + edge, left + edge, bottom - edge, color);
    DrawSnappedRect(right - edge, top + edge, right, bottom - edge, color);
}

float DrawText(const char* text, float x, float y, float height, TextAlign align, float scale, const float* color,
    int style, FontId fontId)
{
    Font_s* font = GetFont(fontId);
    if (!font || !text || !text[0])
        return 0.0f;

    int fontHeight = FontPixelHeight(font);
    float textScale = scale * gDrawScale * kReferenceFontHeight / (float)fontHeight;
    float widthPixels = MeasureTextPixels(text, font, textScale);

    float pixelX = gDrawOriginX + x * gDrawScale;
    if (align == TEXT_ALIGN_CENTER) pixelX -= widthPixels * 0.5f;
    else if (align == TEXT_ALIGN_RIGHT) pixelX -= widthPixels;
    // text vertical centre = box centre, so baseline = centre + half the text height
    float centerY = gDrawOriginY + (y + height * 0.5f) * gDrawScale;
    float textHeight = (float)fontHeight * textScale;
    float baseline = centerY + textHeight * 0.5f;
    pixelX = (float)SnapToPixel(pixelX);
    baseline = (float)SnapToPixel(baseline);

    float visible = VisibleFractionInClip(y, height);
    if (visible <= 0.0f)
        return widthPixels;

    float fadedColor[4] = { color[0], color[1], color[2], color[3] * (gDrawFade * visible) };
    R_AddCmdDrawTextInternal(text, 0x7FFFFFFF, font, pixelX, baseline, textScale, textScale, 0.0f, fadedColor, style);
    return widthPixels;
}

// Fake bold: the same text drawn twice with a small x offset
float DrawTextBold(const char* text, float x, float y, float height, float scale, const float* color)
{
    DrawText(text, x + kBoldOffset, y, height, TEXT_ALIGN_LEFT, scale, color);
    return DrawText(text, x, y, height, TEXT_ALIGN_LEFT, scale, color);
}

// Rounded square (3 overlapping rects) with a bold white letter; width 0 = square, long text shrinks to fit
void DrawBadge(float x, float y, float size, const char* letter, const float* color, float width)
{
    const float badgeWidth = (width > 0.0f) ? width : size;
    const float corner = floorf(size * kBadgeCornerRatio + 0.5f);
    const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    DrawShader(x + corner, y, badgeWidth - 2.0f * corner, size, color);
    DrawShader(x, y + corner, badgeWidth, size - 2.0f * corner, color);
    DrawShader(x + 1.0f, y + 1.0f, badgeWidth - 2.0f, size - 2.0f, color);

    float scale = kBadgeLetterScale;
    float letterWidth = TextWidth(letter, scale);
    if (letterWidth > badgeWidth - kBadgeTextPadding && letterWidth > 0.0f)
        scale *= (badgeWidth - kBadgeTextPadding) / letterWidth;
    DrawText(letter, x + badgeWidth * 0.5f, y, size, TEXT_ALIGN_CENTER, scale, white);
}

float TextWidth(const char* text, float scale, FontId fontId)
{
    Font_s* font = GetFont(fontId);
    if (!font)
        return 0.0f;
    float textScale = scale * gDrawScale * kReferenceFontHeight / (float)FontPixelHeight(font);
    return MeasureTextPixels(text, font, textScale) / gDrawScale;
}

int TextHeight(FontId fontId)
{
    return FontPixelHeight(GetFont(fontId));
}

// Retries every frame until the fonticon asset resolves the name to a real (non-$default) material
void ResolveIcons()
{
    if (!gWhiteMaterial || !gBodyFont)
        return;
    if (!gDefaultMaterial)
        gDefaultMaterial = Material_RegisterHandle("$default", 0, false, -1);

    int fontHeight = FontPixelHeight(gBodyFont);
    for (int i = 0; i < ICON_COUNT; i++)
    {
        if (gbIconReady[i])
            continue;
        Material* material = NULL;
        float widthRatio = 0.0f, heightRatio = 0.0f;
        void* entry = R_GetFontIconEntryInfo(0, gIconFontNames[i], fontHeight, &material, &widthRatio, &heightRatio);
        bool bResolved = entry && material && (Material*)entry != gDefaultMaterial && material != gDefaultMaterial;
        bool bRatiosValid = widthRatio > kIconRatioMin && heightRatio > kIconRatioMin
            && widthRatio < kIconRatioMax && heightRatio < kIconRatioMax;
        if (bResolved && bRatiosValid)
        {
            gIconWidthRatio[i] = widthRatio;
            gIconHeightRatio[i] = heightRatio;
            gbIconReady[i] = true;
        }
    }
}

bool IsIconReady(IconId icon)
{
    return icon >= 0 && icon < ICON_COUNT && gbIconReady[icon];
}

// Layout width of an icon drawn at kIconHeight
float IconWidth(IconId icon)
{
    return kIconHeight * gIconWidthRatio[icon] / gIconHeightRatio[icon];
}

void DrawIcon(IconId icon, float x, float centerY)
{
    if (!gBodyFont) return;
    int fontHeight = FontPixelHeight(gBodyFont);
    float heightPixels = (float)SnapToPixel(kIconHeight * gDrawScale);
    if (heightPixels < 1.0f) heightPixels = 1.0f;
    float textScale = heightPixels / ((float)fontHeight * gIconHeightRatio[icon]);
    float pixelX = (float)SnapToPixel(gDrawOriginX + x * gDrawScale);
    float baseline = (float)SnapToPixel(gDrawOriginY + centerY * gDrawScale) + (float)fontHeight * textScale * 0.5f;
    float color[4] = { 1.0f, 1.0f, 1.0f, gDrawFade };

    R_AddCmdDrawTextInternal(kIconTextEscapes[icon], 0x7FFFFFFF, gBodyFont, pixelX, baseline, textScale, textScale, 0.0f, color, 0);
}