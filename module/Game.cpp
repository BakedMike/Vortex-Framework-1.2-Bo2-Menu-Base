#include "stdafx.h"

// Rendering
void CL_DrawStretchPicPhysical(float x, float y, float width, float height, float s0, float t0, float s1, float t1, const float* color, Material* material)
{
    ((void(*)(float, float, float, float, float, float, float, float, const float*, Material*))0x82262A20)(x, y, width, height, s0, t0, s1, t1, color, material);
}

void R_AddCmdDrawTextInternal(const char* text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation, const float* color, int style)
{
    ((void(*)(const char*, int, Font_s*, float, float, float, float, float, const float*, int))0x828B8BA0)(text, maxChars, font, x, y, xScale, yScale, rotation, color, style);
}

Material* Material_RegisterHandle(const char* name, int imageTrack, bool errorIfMissing, int waitTime)
{
    return ((Material*(*)(const char*, int, bool, int))0x828B78F0)(name, imageTrack, errorIfMissing, waitTime);
}

void* R_GetFontIconEntryInfo(int localClient, char* name, int fontHeight, Material** outMaterial, float* outWidthRatio, float* outHeightRatio)
{
    return ((void*(*)(int, char*, int, Material**, float*, float*))0x828C3B68)(localClient, name, fontHeight, outMaterial, outWidthRatio, outHeightRatio);
}

Font_s* CL_RegisterFont(const char* name)
{
    return ((Font_s*(*)(const char*))0x82275F78)(name);
}

int R_TextWidth(int localClient, const char* text, int maxChars, Font_s* font)
{
    return ((int(*)(int, const char*, int, Font_s*))0x828B6FD8)(localClient, text, maxChars, font);
}

int R_TextHeight(Font_s* font)
{
    return ((int(*)(Font_s*))0x82490390)(font);
}

// Dvars
void* Dvar_FindVar(const char* name)
{
    return ((void*(*)(const char*))0x82496430)(name);
}

const char* Dvar_GetString(void* dvar)
{
    return ((const char*(*)(void*))0x82496990)(dvar);
}

int Dvar_GetInt(void* dvar)
{
    return ((int(*)(void*))0x82496528)(dvar);
}

bool Dvar_GetBool(void* dvar)
{
    return ((bool(*)(void*))0x824964C0)(dvar);
}

float Dvar_GetFloat(void* dvar)
{
    return ((float(*)(void*))0x82496780)(dvar);
}

void Dvar_SetFloat(void* dvar, float value)
{
    ((void(*)(void*, float))0x824991C8)(dvar, value);
}

// BG_IsTeamBased: true when the gametype has more than one team
bool BG_IsTeamBased(void)
{
    return ((bool(*)(void))0x821A6A50)();
}
