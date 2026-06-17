// dllmain.cpp : Определяет точку входа для приложения DLL.
// #define _H3API_PLUGINS_
#define _H3API_PLUGINS_
#define ERA_MODLIST
#define _WOG_
#include "TestDlg.h"
#include "framework.h"
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

_ERH_(OnGameEnter)
{

    return;
    TestDlg dlg(500, 500);
    dlg.Start();
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

H3CreatureInfoDlg *globalDlg = nullptr;

_LHF_(RMCdlgProc)
{
    auto dlg = ValueAt<H3BaseDlg *>(c->ebp + 0x8);
    if (globalDlg == ValueAt<H3BaseDlg *>(c->ebp + 0x8))
    {
        auto dlgDef = globalDlg->animation;
        if (dlgDef)
        {
            DWORD waitUntil = ValueAt<DWORD>(0x6989E8);
            DWORD currentTime = GetTime();

            if (int(currentTime - waitUntil) < 0)
            {
                return EXEC_DEFAULT;
            }

            const BOOL8 isWarMachine = THISCALL_1(BOOL8, 0x047AAB0, globalDlg->creatureId);
            THISCALL_1(void, isWarMachine ? 0x04EB330 : 0x04EB140, globalDlg->animation);
            globalDlg->Redraw();
            waitUntil = ValueAt<DWORD>(0x6989E8);
            int currentTimeA = GetTime() - waitUntil;
            if (currentTimeA < 100)
                currentTimeA = 100;
            ValueAt<DWORD>(0x6989E8) = waitUntil + currentTimeA;
        }
    }
    return EXEC_DEFAULT;
}

H3WavFile *buttonClickSound2 = nullptr;

#include <thread>

void PlayButtonClickSound2()
{

    auto snd = P_SoundManager->Get();

    {
        //     return;
    }
    auto originalSound = DwordAt(0x694DF4);

    BOOL32 backup = snd->clickSoundVar;
    snd->clickSoundVar = 1;
    buttonClickSound2->spinCount = 64; // volume
    buttonClickSound2->debugInfo = PRTL_CRITICAL_SECTION_DEBUG(1);
    buttonClickSound2->lockSemaphore = HANDLE(HANDLE_FLAG_PROTECT_FROM_CLOSE | HANDLE_FLAG_INHERIT);

    THISCALL_2(VOID, 0x59A510, snd, buttonClickSound2);
    snd->clickSoundVar = backup;

    // DwordAt(0x694DF4) = DWORD(buttonClickSound2);
    // snd->ClickSound();
    // DwordAt(0x694DF4) = DWORD(originalSound);
}

void PlaySoundInThread()
{
    PlayButtonClickSound2();
    // std::thread th(PLayButtonClickSound2);
    // th.detach();

    //  auto snd = P_SoundManager->Get();
}

#include <set>
std::set<DWORD> buttonsPressed;

DWORD __stdcall DefButtonOnHotKey(HiHook *hook, H3DlgDefButton *button, H3Msg *msg)
{
    // PlaySoundInThread();
    if (button->IsPressed())
    {
        return 2;
    }
    return THISCALL_2(DWORD, hook->GetDefaultFunc(), button, msg);
}
int counterS = 0;

DWORD __stdcall DefButtonSetClicked(HiHook *hook, H3DlgDefButton *button, H3Msg *msg)
{

    const auto result = THISCALL_2(DWORD, hook->GetDefaultFunc(), button, msg);
    if (counterS == 2)
    {
        counterS = 0;

        PlaySoundInThread();
    }
    return result;
}

DWORD __stdcall DefButtonOnDraw(HiHook *hook, H3DlgDefButton *button)
{

    // PlaySoundInThread();
    if (button->IsPressed() && button->GetParent() == P_WindowManager->lastDlg)
    {
        buttonsPressed.insert(DWORD(button));
    }
    else if (button->IsActive() && buttonsPressed.erase(DWORD(button)))
    {

        PlaySoundInThread();
    }
    return THISCALL_1(DWORD, hook->GetDefaultFunc(), button);
}
void __stdcall DefButtonOnRelease(HiHook *hook, DWORD wnd, int x, int y, int w, int h)
{

    return THISCALL_5(void, hook->GetDefaultFunc(), wnd, x, y, w, h);
}
DWORD __stdcall DefButtonDtor(HiHook *hook, H3DlgDefButton *button)
{

    buttonsPressed.erase(DWORD(button));
    return THISCALL_1(DWORD, hook->GetDefaultFunc(), button);
}
DWORD __stdcall Dlg_BattleResults_Proc(HiHook *hook, H3Msg *msg)
{

    auto result = THISCALL_1(DWORD, hook->GetDefaultFunc(), msg);

    if (result == 2)
    {
        PlaySoundInThread();
        // counterS = 2;

        //        result = 3;
    }
    // else if (result == 3)
    //{
    //     return 2;
    // }
    return result;
}
DWORD __stdcall Dlg_BattleResults_Dtor(HiHook *hook, H3Msg *msg)
{

    auto result = THISCALL_1(DWORD, hook->GetDefaultFunc(), msg);
    PlaySoundInThread();

    // counterS = 2;

    // else if (result == 3)
    //{
    //     return 2;
    // }
    return result;
}
_LHF_(DefButtonOnProc)
{
    // PlayButtonClickSound2();
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

_LHF_(HdWog_hooh)
{

    return EXEC_DEFAULT;
}

_LHF_(HooksInit)
{

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
    auto snd = P_SoundManager->Get();
    snd->ClickSound();
    //"HD.Version.CStr"
    LPCSTR hdVersionStr = globalPatcher->VarGetValue<LPCSTR>("HD.Version.CStr", nullptr);
    //"HD.Version.Dword"
    DWORD hdVersionDword = globalPatcher->VarGetValue<DWORD>("HD.Version.Dword", 0);

    if (0)
    {
        _PI->WriteHiHook(0x0584820, THISCALL_, H3ScenarioDlg_UpdateMapInfo);
    }
    // double click on sound
    if (0)
    {
        buttonClickSound2 = H3WavFile::Load("BUTTON2.WAV");
        if (buttonClickSound2)
        {
            // _PI->WriteHiHook(0x0456171, THISCALL_, DefButtonOnRelease);
            // _PI->WriteLoHook(0x0455E81, DefButtonOnProc);
            // PI->WriteHiHook(0x04562C7, THISCALL_, DefButtonOnHotKey);
            // _PI->WriteHiHook(0x0456540, THISCALL_, DefButtonSetClicked);
            _PI->WriteHiHook(0x0456620, THISCALL_, DefButtonOnDraw);
            _PI->WriteHiHook(0x0455DD0, THISCALL_, DefButtonDtor);
            // _PI->WriteHiHook(0x04716E0, THISCALL_, Dlg_BattleResults_Proc);
            // _PI->WriteHiHook(0x047724F, THISCALL_, Dlg_BattleResults_Dtor);
            _PI->WriteHiHook(0x04772FE, THISCALL_, Dlg_BattleResults_Dtor);
            _PI->WriteHiHook(0x047724F, THISCALL_, Dlg_BattleResults_Dtor);

            // _PI->WriteByte(0x04A69A2, 0xEB);
        }
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

    return;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::instanceName);
        Era::ConnectEra(hModule, dllText::instanceName);
        _REH_(OnAfterWog);
        _REH_(OnGameEnter);
        _PI->WriteLoHook(0x4EEAF2, HooksInit);

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
