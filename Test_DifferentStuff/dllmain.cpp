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
LPCSTR instanceName = "EraPlugin.Testing.daemon_n";
}
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

    H3FontLoader fnt(NH3Dlg::Text::TINY);
    // fnt->TextDraw(tempBuffer, h3_TextBuffer, marginX, marginY, tempBuffer->width, workingHeight);

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
void __fastcall b_MsgBox(const char *Mes, int MType, int PosX, int PosY, int Type1, int SType1, int Type2, int SType2,
                         int Par, int Time2Show, int Type3, int SType3)
{
}

H3Font *fontPtr = nullptr;

// Fnt_DrawString_To_Pcx16 at 0x4B4FC0.  The original receives a pointer to
// the already split line and its character count.  We never modify that
// buffer: a temporary justified copy is passed only to the original call.
void __stdcall Font_DrawString(HiHook *h, H3Font *font, const char *text, int textLength,
                               H3LoadedPcx16 *drawBuffer, int x, int y, int arg14, int arg18,
                               int arg1C, int arg20, int color, H3LoadedPcx16 *arg28)
{
    if (!font || !text || textLength <= 0)
    {
        return THISCALL_12(void, h->GetDefaultFunc(), font, text, textLength, drawBuffer, x, y, arg14, arg18,
                            arg1C, arg20, color, arg28);
    }

    // The original function checks the right edge as arg18 + arg20 (see
    // 0x4B50E0).  arg18 is the left edge and arg20 is the available width.
    // The incoming x may already be alignment-adjusted by the game, so use
    // the actual line rectangle for justification.
    const int targetWidth = arg20;
    std::string line(text, static_cast<size_t>(textLength));
    std::string justified = JustifyH3TextLine(font, line.c_str(), targetWidth);

    if (justified == line)
    {
        return THISCALL_12(void, h->GetDefaultFunc(), font, text, textLength, drawBuffer, x, y, arg14, arg18,
                            arg1C, arg20, color, arg28);
    }

    return THISCALL_12(void, h->GetDefaultFunc(), font, justified.c_str(), static_cast<int>(justified.size()),
                        drawBuffer, arg18, y, arg14, arg18, arg1C, arg20, color, arg28);
}

void __stdcall Font_DrawTextToPcx16(HiHook *h, H3Font *font, const char *text, H3LoadedPcx16 *drawBuffer, int x, int y,
                                    int dx, int dy, int color, int flags, int a10)
{

    if (drawBuffer != P_WindowManager->GetDrawBuffer())
    {
        return THISCALL_10(void, h->GetDefaultFunc(), font, text, drawBuffer, x, y, dx, dy, color, flags, a10);
    }
    else
    {
        return THISCALL_10(void, h->GetDefaultFunc(), fontPtr, text, drawBuffer, x, y, dx, dy, color, flags, a10);
    }
    if (true)
    {
    }
    //  drawBuffer->DrawThickFrame(x, y, dx, dy, 1, 0x00FFFF);

    //  char buffer[16];
    //  sprintf(buffer, "%4dx%4d", x, y);
    // if (!stricmp(font->GetName(), NH3Dlg::Text::BIG))
    {
        //  return THISCALL_10(void, h->GetDefaultFunc(), font, text, drawBuffer, x, y, dx, dy, rand()%255, flags, a10);
    }
}

// Keep the game's text layout and replace only glyph rasterization. This
// gives generated fonts grayscale antialiasing without reimplementing
// wrapping/alignment in Font_DrawTextToPcx16.
void __stdcall Font_DrawSymbol(HiHook *h, H3Font *font, unsigned int character, H3LoadedPcx16 *drawBuffer, int x, int y,
                               int color)
{
    if (!DrawCustomH3Glyph(font, character, drawBuffer, x, y, color))
        // THISCALL_N counts the hidden `this` argument too.  The original
        // Fnt_DrawSymbol therefore uses THISCALL_6 here.
        THISCALL_6(void, h->GetDefaultFunc(), font, character, drawBuffer, x, y, color);
}

char *fontPath = "Georgia Italic";
char *fontName = "Arial20.fnt";
char *testText =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore "
    "magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
    "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

void InitNewFont()

{
    testText = "у меня весь текст со всем оформлением по-прежнему печатается в Word'e, после чего я создаю "
               "соответствующий def. я уже привык к этому, оно долго, но достаточно удобно и пластично) большого "
               "разнообразия тоже не нужно. пока что))";
    // Register in both H3ResourceManager and the legacy font tree used by
    // H3Font::Load and DlgText::Ctor.
    TTFontOptions options;
    H3FontLoader medFont(NH3Dlg::Text::MEDIUM);
    options.pixelHeight = medFont->height + 8;
    options.bold = true;
    // options.italic = true;

    fontPtr = CreateH3FontFromTTF(fontPath, fontName, options, true);

    if (fontPtr)
    {

        // _PI->WriteHiHook(0x04B4F00, THISCALL_, Font_DrawSymbol);

        H3Dlg dlg(H3GameWidth::Get(), H3GameHeight::Get(), -1, -1, 0, 0, 0);
        dlg.AddBackground(0, 0, H3GameWidth::Get(), H3GameHeight::Get(), 1, 0, 1, 0);
        dlg.CreateOKButton();
        dlg.CreateText(40, 40, 200, 65, testText, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,eTextAlignment::MIDDLE_CENTER);
        dlg.CreateText(40, 140, 400, 600, testText, fontName, eTextColor::REGULAR, eTextAlignment::MIDDLE_CENTER);
        dlg.Start();

        // This now resolves the object from the normal game font cache.
        // fontPtr = H3Font::Load(fontName);
    }
}
_ERH_(OnGameEnter)
{
}

_LHF_(HooksInit)
{
    
    // load new font
     if (0)
    {
        _PI->WriteHiHook(0x04B4FC0, THISCALL_, Font_DrawString);

        InitNewFont();

    }

    // font draw to pcx
    if (true)
    {


        // _PI->WriteHiHook(0x05BCA99, CALL_, EXTENDED_, THISCALL_, Font_DrawTextToPcx16); // for dlgtxt: fongtext draw
        // _PI->WriteHiHook(0x04B51F0, SPLICE_, EXTENDED_, THISCALL_, Font_DrawTextToPcx16);
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

    // read hd mod ini
    if (0)
    {
        std::string iniSettings = globalPatcher->VarGetValue<LPCSTR>("HD.Dir.Settings", "Default value");

        iniSettings.append("\\era1.ini");

        // Era::ReadStrFromIni("<UI.Ext.ScenarioMgr.Settings>", "", iniSettings.c_str(), h3_TextBuffer);
        //  MessageBoxA(nullptr, h3_TextBuffer, "Value from hota", MB_OK);

        HDIni *hdIni = globalPatcher->VarGetValue<HDIni *>("HD.Ini.Main", nullptr);

        if (hdIni)
        {
            auto entry = hdIni->FindEntry("UI.Ext.ScenarioMgr.Settings");
            MessageBoxA(nullptr, std::to_string((*entry)[1]->data.value).c_str(), "Value from ini", MB_OK);

            //   hdIni->entries[]
            for (size_t i = 0; i < hdIni->lineEntries; i++)
            {
                auto entries = hdIni->entries[i];
                if (entries)
                {
                    //  Era::WriteStrToIni(entries->data.text, "1", "MAIN", iniSettings.c_str());
                }
            }
            //  Era::SaveIni(iniSettings.c_str());
            return EXEC_DEFAULT;
            for (auto i = hdIni->begin(); i != hdIni->end(); i++)
            {
                MessageBoxA(nullptr, std::to_string(i->data.value).c_str(), "Value from ini", MB_OK);
            }
        }
        iniSettings = globalPatcher->VarGetValue<LPCSTR>("HD.Dir.Settings", "Default value");
        iniSettings.append("\\era.ini");
        // iniSettings = "_HD3_Data/Settings/era.ini";

        //  iniSettings = "_HD3_Data/Settings/era.ini";

        Era::ReadStrFromIni("test", "", iniSettings.c_str(), h3_TextBuffer);

        MessageBoxA(nullptr, h3_TextBuffer, "Value from ini", MB_OK);
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
