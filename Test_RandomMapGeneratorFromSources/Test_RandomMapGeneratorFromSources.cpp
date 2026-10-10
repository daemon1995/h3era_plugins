#include "framework.h"
#include "rmg_include/rmg_request.h"

#include <cstddef>
#include <string>

Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;

namespace
{
constexpr const char* INSTANCE_NAME = "EraPlugin.Test_RandomMapGeneratorFromSources.daemon_n";
constexpr unsigned int RANDOM_MAP_REQUEST_CTOR = 0x0054BF00;
constexpr unsigned int RANDOM_MAP_REQUEST_GENERATE = 0x0054C090;
constexpr unsigned int GAME_RAND = 0x00617E5C;

int GameRand()
{
    // Heroes III statically links LIBCMT; use its _rand so option rolls share
    // the same CRT state as the game and the native RMG implementation.
    return CDECL_0(int, GAME_RAND);
}

struct CNetPlayerHandlerPlayerView
{
    unsigned long dpid;
    unsigned char pad04[0x20 - sizeof(unsigned long)];
    int heroIndex;
    int townIndex;
    unsigned char pad28[0x70 - 0x28];
    int playerPos;
    unsigned char pad74[0x7C - 0x74];
};

struct CNetPlayerHandlerView
{
    CNetPlayerHandlerPlayerView humanPlayers[8];
    CNetPlayerHandlerPlayerView computerPlayers[8];
    unsigned char pad7C0[0x10];
};

struct TSingleSelectionWindowView
{
    unsigned char pad0000[0x1064];
    CNetPlayerHandlerView players;
    unsigned char pad1834[0x1898 - 0x1834];
    int commonGameVersion;
    int townHeadingId;
    int randomMapOptions[8];
};

static_assert(sizeof(CNetPlayerHandlerPlayerView) == 0x7C, "player ABI mismatch");
static_assert(sizeof(CNetPlayerHandlerView) == 0x7D0, "player handler ABI mismatch");
static_assert(offsetof(TSingleSelectionWindowView, commonGameVersion) == 0x1898,
              "selection window ABI mismatch");
static_assert(offsetof(TSingleSelectionWindowView, randomMapOptions) == 0x18A0,
              "random map option offset mismatch");

CNetPlayerHandlerPlayerView* GetPlayerInPosition(CNetPlayerHandlerView& players, int position)
{
    for (int index = 0; index < 8; ++index) {
        if (players.humanPlayers[index].playerPos == position)
            return &players.humanPlayers[index];
    }
    return nullptr;
}

unsigned char GenerateMapFromSourceRequest(TSingleSelectionWindowView* window,
                                          const char* mapName)
{
    int humanPlayerCount = window->randomMapOptions[2];
    int humanTeamCount = window->randomMapOptions[3];
    int computerPlayerCount = window->randomMapOptions[4];
    int computerTeamCount = window->randomMapOptions[5];

    if (humanPlayerCount == -1) {
        int seated = 0;
        for (int position = 0; position < 8; ++position) {
            CNetPlayerHandlerPlayerView* player =
                GetPlayerInPosition(window->players, position);
            if (player && player->dpid)
                ++seated;
        }
        humanPlayerCount = seated + GameRand() % (9 - seated);
    }

    if (humanTeamCount == -1)
        humanTeamCount = GameRand() % humanPlayerCount + 1;
    if (humanTeamCount == 0)
        humanTeamCount = humanPlayerCount;
    if (humanTeamCount > humanPlayerCount)
        humanTeamCount = humanPlayerCount;

    if (humanPlayerCount == 8) {
        computerPlayerCount = 0;
        computerTeamCount = 0;
    } else {
        if (computerPlayerCount == -1)
            computerPlayerCount = GameRand() % (9 - humanPlayerCount);
        if (computerPlayerCount + humanPlayerCount > 8)
            computerPlayerCount = 8 - humanPlayerCount;
        if (computerPlayerCount == 0)
            computerTeamCount = 0;
        if (computerTeamCount == -1)
            computerTeamCount = GameRand() % computerPlayerCount + 1;
        if (computerTeamCount == 0)
            computerTeamCount = computerPlayerCount;
        if (computerTeamCount > computerPlayerCount)
            computerTeamCount = humanPlayerCount;
    }

    int monsterStrength = window->randomMapOptions[7] - 1;
    if (monsterStrength == -2)
        monsterStrength = GameRand() % 3 - 1;

    int mapVersion = RMG_MAP_RESTORATION_OF_ERATHIA;
    if (window->commonGameVersion == 2 || window->commonGameVersion == 3)
        mapVersion = RMG_MAP_SHADOW_OF_DEATH;
    else if (window->commonGameVersion == 1)
        mapVersion = RMG_MAP_ARMAGEDDONS_BLADE;

    __declspec(align(4)) unsigned char requestStorage[sizeof(TRandomMapRequest)];
    TRandomMapRequest* request = reinterpret_cast<TRandomMapRequest*>(requestStorage);
    THISCALL_4(void, RANDOM_MAP_REQUEST_CTOR, request,
               window->randomMapOptions[0], window->randomMapOptions[0],
               window->randomMapOptions[1]);
    request->m_humanPlayerCount = humanPlayerCount;
    request->m_humanTeamCount = humanTeamCount;
    request->m_computerPlayerCount = computerPlayerCount;
    request->m_computerTeamCount = computerTeamCount;
    request->m_waterContent = static_cast<ERmgWaterContent>(window->randomMapOptions[6]);
    request->m_monsterStrength = monsterStrength;
    request->m_mapVersion = static_cast<ERmgMapVersion>(mapVersion);

    for (int position = 0; position < 8; ++position) {
        CNetPlayerHandlerPlayerView* player =
            GetPlayerInPosition(window->players, position);
        if (!player)
            player = &window->players.computerPlayers[position];
        if (player->dpid)
            request->m_isHumanSeat[position] = 1;
        request->m_townChoices[position] = static_cast<TTownType>(player->townIndex);
    }

    std::string path("random_maps\\");
    path += mapName;
    const ERandomMapResult result = THISCALL_3(
        ERandomMapResult, RANDOM_MAP_REQUEST_GENERATE, request, path.c_str(), nullptr);
    return result == RANDOM_MAP_OK;
}

// 0x58704B is the CALL site in TSingleSelectionWindow::onWidgetDeselect
// for TSingleSelectionWindow::generateRandomMap(const char*).
unsigned char __stdcall GenerateRandomMapCall(HiHook*,
                                              TSingleSelectionWindowView* selectionWindow,
                                              const char* mapName)
{
    return GenerateMapFromSourceRequest(selectionWindow, mapName);
}

void InstallHooks()
{
    globalPatcher = GetPatcher();
    _PI = globalPatcher->CreateInstance(INSTANCE_NAME);
    _PI->WriteHiHook(0x58704B, CALL_, EXTENDED_, THISCALL_, GenerateRandomMapCall);
}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    (void)module;
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
        InstallHooks();
    return TRUE;
}
