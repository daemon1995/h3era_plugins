// dllmain.cpp : Определяет точку входа для приложения DLL.
// #define _H3API_PLUGINS_
#define _H3API_PLUGINS_
#define ERA_MODLIST
#define _WOG_
#include "..\headers\Era\era.cpp"
#include "TestDlg.h"
#include "framework.h"

#include "TTFontToFont.h"

// #include "..\..\headers\H3API_RK\single_header\H3API.hpp"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}
namespace widerMenu
{
constexpr bool ENABLE_WIDER_MAIN_MENU = true;
constexpr int ORIGINAL_MENU_WIDTH = 800;
constexpr int ORIGINAL_MENU_HEIGHT = 600;

int LerpChannel(int first, int second, DWORD fraction)
{
    return first + ((second - first) * static_cast<int>(fraction) >> 16);
}

// Resize a snapshot instead of the live draw buffer: source and destination
// overlap once the image becomes wider than the original 800x600 area.
void StretchBackground(H3LoadedPcx16 *destination, H3LoadedPcx16 *source, int width, int height)
{
    const DWORD xStep = width > 1 ? ((source->width - 1) << 16) / (width - 1) : 0;
    const DWORD yStep = height > 1 ? ((source->height - 1) << 16) / (height - 1) : 0;

    if (H3BitMode::Get() == 4)
    {
        for (int y = 0; y < height; ++y)
        {
            const DWORD sourceY = y * yStep;
            const int y0 = sourceY >> 16;
            const int y1 = std::min(y0 + 1, source->height - 1);
            const DWORD fy = sourceY & 0xFFFF;
            const PUINT8 row0 = source->buffer + y0 * source->scanlineSize;
            const PUINT8 row1 = source->buffer + y1 * source->scanlineSize;
            PUINT8 destinationRow = destination->buffer + y * destination->scanlineSize;

            for (int x = 0; x < width; ++x)
            {
                const DWORD sourceX = x * xStep;
                const int x0 = sourceX >> 16;
                const int x1 = std::min(x0 + 1, source->width - 1);
                const DWORD fx = sourceX & 0xFFFF;

                for (int channel = 0; channel < 4; ++channel)
                {
                    const int upper = LerpChannel(row0[x0 * 4 + channel], row0[x1 * 4 + channel], fx);
                    const int lower = LerpChannel(row1[x0 * 4 + channel], row1[x1 * 4 + channel], fx);
                    destinationRow[x * 4 + channel] = static_cast<BYTE>(LerpChannel(upper, lower, fy));
                }
            }
        }
        return;
    }

    for (int y = 0; y < height; ++y)
    {
        const DWORD sourceY = y * yStep;
        const int y0 = sourceY >> 16;
        const int y1 = std::min(y0 + 1, source->height - 1);
        const DWORD fy = sourceY & 0xFFFF;
        const UINT16 *row0 = reinterpret_cast<UINT16 *>(source->buffer + y0 * source->scanlineSize);
        const UINT16 *row1 = reinterpret_cast<UINT16 *>(source->buffer + y1 * source->scanlineSize);
        UINT16 *destinationRow = reinterpret_cast<UINT16 *>(destination->buffer + y * destination->scanlineSize);

        for (int x = 0; x < width; ++x)
        {
            const DWORD sourceX = x * xStep;
            const int x0 = sourceX >> 16;
            const int x1 = std::min(x0 + 1, source->width - 1);
            const DWORD fx = sourceX & 0xFFFF;

            const UINT16 topLeft = row0[x0];
            const UINT16 topRight = row0[x1];
            const UINT16 bottomLeft = row1[x0];
            const UINT16 bottomRight = row1[x1];

            const int red0 = LerpChannel((topLeft >> 11) & 31, (topRight >> 11) & 31, fx);
            const int red1 = LerpChannel((bottomLeft >> 11) & 31, (bottomRight >> 11) & 31, fx);
            const int green0 = LerpChannel((topLeft >> 5) & 63, (topRight >> 5) & 63, fx);
            const int green1 = LerpChannel((bottomLeft >> 5) & 63, (bottomRight >> 5) & 63, fx);
            const int blue0 = LerpChannel(topLeft & 31, topRight & 31, fx);
            const int blue1 = LerpChannel(bottomLeft & 31, bottomRight & 31, fx);

            const int red = LerpChannel(red0, red1, fy);
            const int green = LerpChannel(green0, green1, fy);
            const int blue = LerpChannel(blue0, blue1, fy);
            destinationRow[x] = static_cast<UINT16>((red << 11) | (green << 5) | blue);
        }
    }
}

// MainLoop has just composed the original 800x600 menu background at this
// point. Publish its wide copy now; the quick-start scenario constructor may
// alter the draw buffer afterwards, but its Run() (and thus screen redraw) is
// skipped by quickStart::StartSelectedMap.
_LHF_(ExpandMainMenuBackground)
{
    H3LoadedPcx16 *drawBuffer = P_WindowManager->GetDrawBuffer();
    if (!drawBuffer || drawBuffer->width < ORIGINAL_MENU_WIDTH || drawBuffer->height < ORIGINAL_MENU_HEIGHT)
        return EXEC_DEFAULT;

    const int width = std::min(H3GameWidth::Get(), drawBuffer->width);
    const int height = std::min(H3GameHeight::Get(), drawBuffer->height);
    if (width < 1280 || height < 720)
        return EXEC_DEFAULT;

    H3LoadedPcx16 *source = H3LoadedPcx16::Create(ORIGINAL_MENU_WIDTH, ORIGINAL_MENU_HEIGHT);
    if (!source)
        return EXEC_DEFAULT;

    const int bytesPerPixel = H3BitMode::Get();
    const int sourceRowSize = ORIGINAL_MENU_WIDTH * bytesPerPixel;
    for (int y = 0; y < ORIGINAL_MENU_HEIGHT; ++y)
    {
        libc::memcpy(source->buffer + y * source->scanlineSize, drawBuffer->buffer + y * drawBuffer->scanlineSize,
                     sourceRowSize);
    }

    StretchBackground(drawBuffer, source, width, height);
    source->Destroy();
    P_WindowManager->H3Redraw(0, 0, width, height);
    return EXEC_DEFAULT;
}

} // namespace widerMenu
namespace quickStart
{
// Both features are independent. Put the required map in the game's Maps directory.
constexpr bool START_MAP_FROM_MAIN_MENU = true;
constexpr LPCSTR MAP_FILE_NAME = "Arrogance.h3m";
constexpr bool START_BATTLE_ON_GAME_ENTER = true;

constexpr int MAIN_MENU_NEW_GAME = Era::EGameMenuTarget::PAGE_NEW_GAME;
constexpr int NEW_GAME_SINGLE_SCENARIO = 100;
constexpr int DIALOG_OK = 30722;
constexpr DWORD MAIN_MENU_JUMP_TO = 0x00697728;
constexpr DWORD SELECT_SCENARIO_START_GAME = 0x0058BFB0;
constexpr DWORD ADVENTURE_MANAGER_START_BATTLE = 0x75ADD9;

bool autoStartMapPending = START_MAP_FROM_MAIN_MENU;
bool battleStartPending = false;
bool battleStartedForCurrentMap = false;

LPCSTR FileNamePart(LPCSTR path)
{
    LPCSTR result = path;
    if (!path)
        return h3_NullString;

    for (LPCSTR cursor = path; *cursor; ++cursor)
    {
        if (*cursor == '\\' || *cursor == '/')
            result = cursor + 1;
    }
    return result;
}

// This is the New Game / Campaign / Tutorial screen. Selecting item 100
// follows the game's normal single-scenario path without showing the screen.
void __stdcall ChooseSingleScenario(HiHook *hook, H3BaseDlg *dlg)
{
    if (autoStartMapPending)
    {
        P_WindowManager->resultItemID = NEW_GAME_SINGLE_SCENARIO;
        return;
    }

    THISCALL_1(void, hook->GetDefaultFunc(), dlg);
}

// The scenario dialog constructor has already enumerated Maps and initialized
// all player settings. Select the requested map and invoke the same routine as
// the dialog's Begin button (0x58BFB0).
void __stdcall StartSelectedMap(HiHook *hook, H3SelectScenarioDialog *dlg, int runMode)
{
    if (autoStartMapPending)
    {
        for (UINT i = 0; i < dlg->currentMapsList.Size(); ++i)
        {
            LPCSTR fileName = dlg->currentMapsList[i].playersInfo.filename;
            if (libc::strcmpi(FileNamePart(fileName), MAP_FILE_NAME) == 0)
            {
                dlg->UpdateForSelectedScenario(i, FALSE);

                if (THISCALL_1(char, SELECT_SCENARIO_START_GAME, dlg))
                {
                    autoStartMapPending = false;
                    P_WindowManager->resultItemID = DIALOG_OK;
                    // Do not call H3SelectScenarioDialog::Run below. Its dialog
                    // loop reaches H3BaseDlg::Redraw at 0x602E54; returning here
                    // keeps the prepared dialog strictly in the draw buffer.
                    return;
                }
                break;
            }
        }

        // Missing/invalid map: return to the regular scenario dialog so the
        // user can choose a map instead of being left in a broken menu state.
        autoStartMapPending = false;
    }

    THISCALL_2(void, hook->GetDefaultFunc(), dlg, runMode);
}

H3Hero *FindAttackingHero()
{
    H3Player *player = P_Game->GetPlayer();
    if (!player)
        return nullptr;

    const int heroId = player->heroIDs[3];
    H3Hero *hero = nullptr;
    if (heroId < 0 || heroId >= 156)
        hero = P_Game->GetHero(heroId);
    if (hero)
        return hero;

    hero = player->GetActiveHero();
    if (hero)
        return hero;

    for (int i = 0; i < 8; ++i)
    {
        const int heroId = player->heroIDs[i];
        if (heroId < 0 || heroId >= 156)
            continue;

        hero = P_Game->GetHero(heroId);
        if (hero)
            return hero;
    }
    return nullptr;
}

bool StartBattleAtMapBeginning()
{
    if (!START_BATTLE_ON_GAME_ENTER || battleStartedForCurrentMap)
        return true;

    H3Hero *attacker = FindAttackingHero();
    H3AdventureManager *advManager = P_AdventureManager->Get();
    if (!attacker || !advManager || !attacker->army.HasCreatures())
        return false;

    battleStartedForCurrentMap = true;

    H3Army defenders;
    defenders.ClearAndGive(rand() % NH3Creatures::CHAOS_HYDRA, rand() % 0x0FFF);
    THISCALL_11(int, ADVENTURE_MANAGER_START_BATTLE, advManager, attacker->mixedPosition.Mixed(), attacker,
                &attacker->army, -1, nullptr, nullptr, &defenders, -1, TRUE, FALSE);
    return true;
}

// OnGameEnter is emitted before the executive manager makes the adventure
// manager active. Starting a battle there leaves both managers running. The
// first map-screen message is late enough: CallManager can now disable the
// adventure manager, run combat exclusively and restore the map afterwards.
int __stdcall StartPendingBattle(HiHook *hook, H3AdventureManager *advManager, H3Msg *msg)
{
    if (battleStartPending && advManager->status == H3Manager::ACTIVE && P_ExecutiveMgr->active_mgr == advManager)
    {
        if (StartBattleAtMapBeginning())
            battleStartPending = false;
    }

    return THISCALL_2(int, hook->GetDefaultFunc(), advManager, msg);
}
} // namespace quickStart

void ShowCreatureTableDialog();

_LHF_(MainWindow_F1)
{
    c->return_address = 0x4F877D;
    return NO_EXEC_DEFAULT;

    return EXEC_DEFAULT;
}

#include <cstdio>
#include <initializer_list>
#include <type_traits>

template <typename... Ints> void Debug(Ints... values) noexcept
{
    char buffer[1024];

    size_t pos = 0;

    (void)std::initializer_list<int>{
        ([&] { pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, "%d ", int(values)); }(), 0)...};

    if (pos > 0)
        buffer[pos - 1] = '\0';
    libc::sprintf(Era::z[1], "%s", buffer);
    Era::ExecErmCmd("IF:L^%z1^");
}

H3LoadedDef *__stdcall LoadDEF(HiHook *hook, LPCSTR defName)
{
    H3LoadedDef *result = THISCALL_1(H3LoadedDef *, hook->GetDefaultFunc(), defName);

    result->ColorToPlayer(rand() % 8);
    // H3Messagebox("pol_LoadDEF called.");
    return result;
}
H3LoadedPcx16 *tempBuffer = nullptr;
int bufferHeight = 13;
int bufferX = 0;

_LHF_(AfterAdvMapTilesDraw)
{

    libc::memset(tempBuffer->buffer, 123, tempBuffer->buffSize);

    constexpr int marginX = 1;
    constexpr int marginY = 1;
    const int workingHeight = bufferHeight - marginY * 2;
    const int max = tempBuffer->width - (marginX * 2);
    // Подставляем в плейсхолдеры значения и загружаем в буфер

    static int counter = 0;

    const int value = counter;

    // Отрисовываем

    if (counter++ >= max)
    {
        counter = 0;
    }
    //  tempBuffer->AdjustHueSaturation(marginX, marginY, value, workingHeight, 0.75f, 1.f);
    libc::sprintf(h3_TextBuffer, "%d/%d", value, max);

    auto drawBuffer = P_WindowManager->GetDrawBuffer();
    tempBuffer->DrawToPcx16(bufferX, 8, 1, drawBuffer, value);

    return EXEC_DEFAULT;
}

int __stdcall H3ScenarioDlg_UpdateMapInfo(HiHook *h, H3SelectScenarioDialog *dlg)
{

    int result = THISCALL_1(int, h->GetDefaultFunc(), dlg);
    if (dlg->randomMapGeneration)
    {
    }

    const auto &mapInfo = dlg->CurrentMap();

    const int size = mapInfo.mapDimension;
    sprintf_s(h3_TextBuffer, 0x300u, "%dx%d", size, size);
    H3TinyFont *font = H3TinyFont::Get();
    font->TextDraw(P_WindowManager->screenPcx16, h3_TextBuffer, dlg->GetX() + 712, dlg->GetY() + 55, 35, 16,
                   eTextColor(4), eTextAlignment(5));
    Debug(1);
    return result;
}
#include <atomic>
#include <thread>
std::atomic<BOOL> test;
static void __stdcall PlayCombatResultMP3(HiHook *h, DWORD snd, char *name, BOOL play_on_start, BOOL loop)
{

    //  THISCALL_1(void, 0x59B310, snd);
    // THISCALL_1(void, 0x059B380, snd);
    // test = 1;

    // if (!test)
    {

        Era::ExecErmCmd("MP:P0/0;");
        THISCALL_4(void, h->GetDefaultFunc(), snd, name, play_on_start, loop);
    }
    test = 1;
}
_LHF_(Dlg_BattleResults_StopVictoryMusic)
{
    // Era::ExecErmCmd("MP:P0/0;");

    /* o_SoundMgr->f0[0x8C] = 0;
     o_SoundMgr->f0[0x8D] = 0;
     o_SoundMgr->f0[0x8E] = 0;
     o_SoundMgr->f0[0x8F] = 0;*/
    //  CALL_1(void, __thiscall, 0x059AF00, o_SoundMgr);
    //  CALL_4(void, __thiscall, 0x059AFB0, o_SoundMgr, "", 0, 0);

    // CALL_2(void, __thiscall, 0x059A090, o_SoundMgr, 1);
    //  CALL_2(void, __thiscall, 0x059A090, o_SoundMgr,0);
    //  //CALL_1(void, __thiscall, 0x059B310, o_SoundMgr);
    //  o_SoundMgr->f0[0x8C] = 0;
    //// Era::ExecErmCmd("MP:P0/0");
    // storedValue = IntAt(0x06987A8 +8);
    // if (true)
    //{
    // IntAt(0x06987A8 + 8) = 0;
    //}
    return EXEC_DEFAULT;
}

H3Font *fontPtr = nullptr;

char *fontPath = "Roboto Condensed Medium";
char *fontName = "ttest.fnt";
char *testText =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore "
    "magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
    "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

void InitNewFont()
{
    fontPath = "Arial";

    testText = "у меня весь текст со всем оформлением по-прежнему печатается в Word'e, после чего я создаю "
               "соответствующий def. я уже привык к этому, оно долго, но достаточно удобно и пластично) большого "
               "разнообразия тоже не нужно. пока что))";

    testText = EraJS::read("era.locale.testFont.text");
    fontPath = EraJS::read("era.locale.testFont.path");
    int height = EraJS::readInt("era.locale.test.testFont.height");
    // Register in both H3ResourceManager and the legacy font tree used by
    // H3Font::Load and DlgText::Ctor.
    TTFontOptions options;

    if (height > 0)
    {
        options.pixelHeight = height;
    }
    // H3FontLoader medFont(NH3Dlg::Text::MEDIUM);
    // options.pixelHeight += 8;
    // options.bold = true;
    //  options.italic = true;

    fontPtr = CreateH3FontFromTTF(fontPath, fontName, options, true);

    if (fontPtr)
    {
        H3Dlg dlg(H3GameWidth::Get() / 2, H3GameHeight::Get() / 2, -1, -1, 0, 0, 0);
        dlg.AddBackground(0, 0, H3GameWidth::Get() / 2, H3GameHeight::Get() / 2, 1, 0, 1, 0);
        dlg.CreateOKButton();
        dlg.CreateText(40, 40, 200, 65, testText, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                       eTextAlignment::MIDDLE_CENTER);
        dlg.CreateText(40, 100, 300, 65, testText, fontName, eTextColor::REGULAR, eTextAlignment::MIDDLE_CENTER);
        dlg.Start();

        // This now resolves the object from the normal game font cache.
        // fontPtr = H3Font::Load(fontName);
    }
}
_ERH_(OnGameEnter)
{
    quickStart::battleStartedForCurrentMap = false;
    quickStart::battleStartPending = quickStart::START_BATTLE_ON_GAME_ENTER;
}

_ERH_(OnGameLeave)
{
    quickStart::battleStartPending = false;
    quickStart::battleStartedForCurrentMap = false;
}

_LHF_(HooksInit)
{

    // load new font
    if (0)
    {
        InitNewFont();
        return EXEC_DEFAULT;
    }

    if (quickStart::START_MAP_FROM_MAIN_MENU && 0)
    {
        IntAt(0x04CA645 + 6) = 1;
        IntAt(0x04CA37F + 6) = 1;
        _PI->WriteJmp(0x04ED933, 0x04ED9D5);
        _PI->WriteJmp(0x04ED9E0, 0x04EDAD2);

        _PI->WriteHiHook(0x004D5B20, THISCALL_, quickStart::ChooseSingleScenario);
        _PI->WriteHiHook(0x00584EC0, THISCALL_, quickStart::StartSelectedMap);

        // MainLoop consumes this value and enters its normal NEW_GAME branch.
        IntAt(quickStart::MAIN_MENU_JUMP_TO) = Era::EGameMenuTarget::PAGE_NEW_GAME;
    }

    if (quickStart::START_BATTLE_ON_GAME_ENTER && 0)
        _PI->WriteHiHook(0x00408710, THISCALL_, quickStart::StartPendingBattle);

    if (widerMenu::ENABLE_WIDER_MAIN_MENU && 0)
    {
        const DWORD windowWidth = H3GameWidth::Get();
        const DWORD windowHeight = H3GameHeight::Get();
        if (windowWidth >= 1280 && windowHeight >= 720)
        {
            // Immediately after MainLoop copies the 800x600 menu background to
            // the draw buffer and before it branches to the selected menu page.
            _PI->WriteLoHook(0x004EEF44, widerMenu::ExpandMainMenuBackground);
        }
    }

    // disable battleresult mp3
    //
    if (0)
    {
        //    globalPatcher->UndoAllAt(0x0462C65);

        // _PI->WriteHiHook(0x04772E4, THISCALL_, PlayCombatResultMP3);
        //_PI->WriteHiHook(0x0477235, THISCALL_, PlayCombatResultMP3);
        _PI->WriteHiHook(0x0462C65, THISCALL_, PlayCombatResultMP3);

        _PI->WriteLoHook(0x0047724F, Dlg_BattleResults_StopVictoryMusic);
        _PI->WriteLoHook(0x004772FE, Dlg_BattleResults_StopVictoryMusic);
        //_PI->WriteHiHook(0x0462C65, THISCALL_, PlayCombatResultMP3);
    }

    //"HD.Version.CStr"
    LPCSTR hdVersionStr = globalPatcher->VarGetValue<LPCSTR>("HD.Version.CStr", nullptr);
    //"HD.Version.Dword"
    DWORD hdVersionDword = globalPatcher->VarGetValue<DWORD>("HD.Version.Dword", 0);

    if (0)
    {
        _PI->WriteHiHook(0x0584820, THISCALL_, H3ScenarioDlg_UpdateMapInfo);
    }

    // draw progress bar on adventure map
    if (0)
    {
        const int mapViewW = H3GameWidth::Get() - 208;
        const int mapViewH = H3GameHeight::Get() - 56;
        bufferHeight = mapViewH; // / 2;
        const int bufferWidth = mapViewW / 2;
        bufferX = (mapViewW - bufferWidth) / 2;
        tempBuffer = H3LoadedPcx16::Create(bufferWidth, bufferHeight);
        _PI->WriteLoHook(0x040F6CE, AfterAdvMapTilesDraw);
    }

    // _PI->WriteLoHook(0x049CDF6, MapTeamOpen);

    /* _PI->WriteDword(0x0541013 + 2, 808);
     _PI->WriteDword(0x0541159 + 1, 196);*/

    if (0)
    {
        _PI->WriteHiHook(0x055C9C0, THISCALL_, LoadDEF);
    }

    return EXEC_DEFAULT;
    ////_PI->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
    ////_PI->WriteLoHook(0x4EA8B5, DlgDef_Dtor);
    ////
    ////_PI->WriteLoHook(0x5FE9F9, DlgItem_Dtor);
    ////_PI->WriteLoHook(0x5D5926, TownScreen_EndOfRedraw);
    ////_PI->WriteLoHook(0x5C681E, DlgTown_AfterCreate);

    // dlgWidth = H3GameWidth::Get() - 100;
    // dlgHeight = IntAt(0x5C38EF + 1);

    // IntAt(0x5C38F6 + 1) = dlgWidth;
    //// IntAt(0x5C38EF + 1) = dlgHeight;

    ////_PI->WriteDword(0x5C38F6 +1, )

    //// drawBuffer = H3LoadedPcx16::Create(P_WindowManager->screenPcx16->width,
    //// P_WindowManager->screenPcx16->width); drawBuffer =
    //// H3LoadedPcx16::Create(dlgWidth, dlgHeight);

    ////_PI->WriteLoHook(0x4F8767, MainWindow_F1);

    //_PI->WriteWord(0x4F870B, 0x9090);

    // return EXEC_DEFAULT;
}

void EraJSTest()
{
    std::string str = EraJS::read("era.wog.notification.0.name");
    MessageBoxA(NULL, str.c_str(), "Info", MB_OK);
}

_ERH_(OnAfterWog)
{
    // EraJSTest();
    return;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {

        // Era::RestartCurrentProcess();
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::instanceName);
        Era::ConnectEra(hModule, dllText::instanceName);
        _REH_(OnAfterWog);
        _REH_(OnGameEnter);
        _REH_(OnGameLeave);
        //  EraJSTest();

        _PI->WriteLoHook(0x4EEAF2, HooksInit);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Функция для отрисовки диалога
void ShowCreatureTableDialog()
{
    // 1. Создаем диалог
    // Размеры: ширина ~280, высота ~240 (под 5x5 и кнопку)
    H3Dlg dlg(280, 240, -1, -1, 1);

    const int startX = 25;
    const int startY = 20;
    const int stepX = 48; // Ширина CprSmall.def примерно 48px
    const int stepY = 34; // Высота CprSmall.def примерно 32px

    for (int i = 0; i < 25; ++i)
    {
        int col = i % 5;
        int row = i / 5;

        int x = startX + (col * stepX);
        int y = startY + (row * stepY);

        // Создаем элемент Def (портрет существа)
        // "CprSmall.def" — стандартный файл с иконками существ
        // frame = i (ID существа: 0=Копейщик, 1=Алебардщик, и т.д.)
        H3DlgDef *portrait = H3DlgDef::Create(x, y, "CprSmall.def", i);

        // Добавляем элемент в диалог
        dlg.AddItem(portrait);

        // Опционально: можно добавить подсказку при наведении
        portrait->SetHint(P_CreatureInformation[i].nameSingular);
    }

    // 4. Добавляем кнопку OK внизу
    // "iOk.def" — стандартная кнопка ОК
    // ID кнопки = 30722 (стандартный ID для закрытия с результатом OK)
    // HK_ENTER — горячая клавиша Enter
    H3DlgDefButton *btnOk = H3DlgDefButton::Create(110, 200, "iOk.def", 0, 1);
    dlg.AddItem(btnOk);

    // 5. Запускаем диалог
    dlg.Start();
}
