#pragma once

// Called once per frame from the R_EndFrame hook, before the original runs
void RunMenuFrame();

enum MenuTab { TAB_AIMBOT, TAB_ANTIAIM, TAB_VISUAL, TAB_MISC, TAB_PLAYERS, TAB_SETTINGS, TAB_COUNT };

// Menu content, rebuilt every frame
void BuildTab(int tab);
const char* TabName(int tab);
void BuildPlayerOptionsPage(int clientIndex);   // sub page of a player row
