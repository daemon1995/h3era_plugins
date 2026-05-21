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
    // ShowCreatureTableDialog();

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
void __stdcall H3CreatureInfoDlg_ShowRMC(HiHook *hook, H3BaseDlg *dlg)
{
    Patch *patch = nullptr;
    if (static_cast<H3CreatureInfoDlg *>(dlg))
    {
        globalDlg = static_cast<H3CreatureInfoDlg *>(dlg);
        patch = _PI->WriteLoHook(0x060306D, RMCdlgProc);
    }
    THISCALL_1(void, hook->GetDefaultFunc(), dlg);
    if (patch)
    {
        patch->Destroy();
        globalDlg = nullptr;
    }
}

_LHF_(HooksInit)
{

    if (0)
    {
        _PI->WriteHiHook(0x05F4B90, THISCALL_, H3CreatureInfoDlg_ShowRMC);
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
    Era::RegisterHandler(OnGameEnter, "OnGameEnter");

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

// static _LHF_(NewScenarioDlg_Create);
//
// void __stdcall NewScenarioDlg_Create(HiHook *hook, H3SelectScenarioDialog *dlg, H3Msg *msg)
//{
//     THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
//
//     H3DlgCaptionButton *bttn = dlg->GetCaptionButton(4444);
//     if (bttn)
//     {
//         bttn->AddHotkey(h3::eVKey::H3VK_W);
//     }
//     bttn = dlg->CreateCaptionButton(bttn->GetX(), bttn->GetY() + 45, bttn->GetWidth(), bttn->GetHeight(), 4500,
//                                     bttn->GetDef()->GetName(), "ERA options", h3::NH3Dlg::Text::SMALL, 0);
//     if (bttn)
//     {
//         bttn->SetClickFrame(1);
//         bttn->AddHotkey(h3::eVKey::H3VK_E);
//     }
// }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::instanceName);
        Era::ConnectEra(hModule, dllText::instanceName);

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
