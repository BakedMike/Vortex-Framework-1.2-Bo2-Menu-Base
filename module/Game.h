#pragma once

struct Font_s;
struct Material;

//Render
void CL_DrawStretchPicPhysical(float x, float y, float width, float height, float s0, float t0, float s1, float t1, const float* color, Material* material);
void R_AddCmdDrawTextInternal(const char* text, int maxChars, Font_s* font, float x, float y, float xScale, float yScale, float rotation, const float* color, int style);
Material* Material_RegisterHandle(const char* name, int imageTrack, bool errorIfMissing, int waitTime);
void* R_GetFontIconEntryInfo(int localClient, char* name, int fontHeight, Material** outMaterial, float* outWidthRatio, float* outHeightRatio);
Font_s* CL_RegisterFont(const char* name);
int R_TextWidth(int localClient, const char* text, int maxChars, Font_s* font);
int R_TextHeight(Font_s* font);

//Dvars
void* Dvar_FindVar(const char* name);
const char* Dvar_GetString(void* dvar);
int Dvar_GetInt(void* dvar);
bool Dvar_GetBool(void* dvar);
float Dvar_GetFloat(void* dvar);
void Dvar_SetFloat(void* dvar, float value);

//BG_IsTeamBased true when the gametype has more than one team
bool BG_IsTeamBased(void);

struct GamePlayer
{
    int  slot;
    int  team;
    char name[32 + 1];
};

struct GamePlayers
{
    int  count;
    int  localSlot;
    int  hostSlot;
    bool bTeamBased;
    int  localTeam;
    GamePlayer list[18]; // ascending by slot // 18 = max client slots
};

static inline bool ReadGameDword(DWORD address, DWORD* outValue)
{
    if ((address & 3) != 0 || !MmIsAddressValid((PVOID)address))
        return false;
    *outValue = *(volatile DWORD*)address;
    return true;
}

// Keeps printable ASCII, drops ^N colour codes, always null terminates (destSize includes the terminator)
static inline void CopyPlayerName(char* dest, int destSize, const char* source, int sourceLength)
{
    int written = 0;
    for (int i = 0; i < sourceLength && source[i] != 0 && written < destSize - 1; i++)
    {
        unsigned char character = (unsigned char)source[i];
        if (character == '^' && i + 1 < sourceLength && source[i + 1] >= '0' && source[i + 1] <= '9')
        {
            i++;
            continue;
        }
        if (character >= 0x20 && character <= 0x7E && character != '^') // printable ASCII range
            dest[written++] = (char)character;
    }
    dest[written] = 0;
}

// Host client number of the current session, -1 when no party is active
static inline int GetGameHostSlot()
{
    DWORD active = 0, host = 0;
    // 0x82CB5700 / 0x82CA9E38 = party active flags, 0x82CAC160 / 0x82CA0898 = the two PartyData blocks
    if (ReadGameDword(0x82CB5700, &active) && active != 0)
    {
        // 38040 = offset of hostNum inside PartyData, 18 = max client slots
        if (ReadGameDword(0x82CAC160 + 38040, &host) && host < 18) // PartyData hostNum
            return (int)host;
    }
    else if (ReadGameDword(0x82CA9E38, &active) && active != 0)
    {
        // same hostNum offset for the second PartyData block
        if (ReadGameDword(0x82CA0898 + 38040, &host) && host < 18) // PartyData hostNum
            return (int)host;
    }
    return -1;
}

// Max players of the match: cgs value (what the engine uses), falls back to the com_maxclients dvar; 0 if unknown
static inline int GetGameMaxClients(int localClient = 0)
{
    DWORD cgs = 0, maxClients = 0;
    // 0x82BBAE44 = pointer to cgs; cgs stride per local client is 22888, +336 = max clients
    if (ReadGameDword(0x82BBAE44, &cgs) && cgs != 0
        && ReadGameDword(cgs + 22888 * localClient + 336, &maxClients)
        && maxClients >= 1 && maxClients <= 18) // valid range 1..18 clients
        return (int)maxClients;

    void* dvar = Dvar_FindVar("com_maxclients");
    if (dvar && MmIsAddressValid(dvar))
    {
        int value = Dvar_GetInt(dvar);
        if (value >= 1 && value <= 18) // valid range 1..18 clients
            return value;
    }
    return 0;
}

// Reads all valid clientInfo_t slots; false when the game state is not readable (not in a match)
static inline bool ReadGamePlayers(GamePlayers* out, int localClient = 0)
{
    out->count = 0;
    out->localSlot = -1;
    out->hostSlot = -1;
    out->bTeamBased = false;
    out->localTeam = 0;

    DWORD connectionState = 0, clientGame = 0, localClientNumber = 0;
    // connection state array: 0x82C6FDD0
    if (!ReadGameDword(0x82C6FDD0 + 1112 * localClient, &connectionState)
        || (int)connectionState < 9) // 9 = active (in game) connection state
        return false;
    // 0x82BBAE68 = pointer to cg
    if (!ReadGameDword(0x82BBAE68, &clientGame) || clientGame == 0)
        return false;
    clientGame += 554752 * localClient;
    if (!MmIsAddressValid((PVOID)clientGame))
        return false;
    if (ReadGameDword(clientGame, &localClientNumber) && localClientNumber < 18) // client number must be below 18
        out->localSlot = (int)localClientNumber;
    out->hostSlot = GetGameHostSlot();
    out->bTeamBased = BG_IsTeamBased();

    for (int slot = 0; slot < 18; slot++) // 18 client slots
    {
        // clientInfo array at cg+432784, 2056 bytes per slot
        DWORD clientInfo = clientGame + 432784 + 2056 * slot;
        DWORD valid = 0, team = 0;
        if (!MmIsAddressValid((PVOID)(clientInfo + 0x0C + 32))) // +0x0C = name[32]
            continue;
        if (!ReadGameDword(clientInfo, &valid) || valid == 0) // +0x00 = valid flag
            continue;
        const char* rawName = (const char*)(clientInfo + 0x0C);
        if (rawName[0] == 0)
            continue;

        GamePlayer& player = out->list[out->count];
        player.slot = slot;
        ReadGameDword(clientInfo + 0x2C, &team); // +0x2C = team (+0x34 holds the ffa team)
        player.team = (int)team;
        CopyPlayerName(player.name, sizeof(player.name), rawName, 32);
        if (player.name[0] == 0)
            CopyPlayerName(player.name, sizeof(player.name), "Unknown", 7); // 7 = strlen("Unknown")
        if (slot == out->localSlot)
            out->localTeam = player.team;
        out->count++;
    }
    return true;
}
