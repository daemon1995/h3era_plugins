#include "framework.h"

namespace dllText
{
const char *PLUGIN_VERSION = "1.76";
const char *INSTANCE_NAME = "EraPlugin.AssemblyInformation.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
//	const char* PROJECT_NAME = "$(ProjectName)";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText56


Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

void __stdcall OnReportVersion(Era::TEvent *e)
{

    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

BOOL __stdcall _IsIconic(HWND hwnd)
{

    return false;
}

#include <windows.h>
// Объявление структуры и типа функции для регистрации виджета
typedef struct MenuWidgetInfo
{
    std::string name;
    std::string text;
    std::function<void(H3Msg&)> onClick;
	H3DlgCaptionButton* uiElement = nullptr; // Указатель на элемент интерфейса
	int id = 0; // Идентификатор виджета
} MenuWidgetInfo;

typedef void(__stdcall* RegisterMainMenuWidget_t)(const MenuWidgetInfo& info);

void TestWidgetClick(H3Msg& msg) {
    if (msg.itemId == 400)
    {
        MessageBoxA(NULL, "Test widget clicked!", "Widget", MB_OK);

    }
}

int __stdcall GameStart(LoHook *h, HookContext *c)
{
    AssemblyInformation::Get();
    // info->LoadDataFromJson();
    // UserNotification::Get();


                // Динамически ищем функцию экспорта из Interface_MainMenuAPI
    HMODULE hApi = GetModuleHandleA("Interface_MainMenuAPI.era");
    if (hApi) {
        auto reg = (RegisterMainMenuWidget_t)GetProcAddress(hApi, "RegisterMainMenuWidget");
        if (!reg) {
            // Попробуем другие варианты имени функции
            reg = (RegisterMainMenuWidget_t)GetProcAddress(hApi, "_RegisterMainMenuWidget");
        }
        if (!reg) {
            reg = (RegisterMainMenuWidget_t)GetProcAddress(hApi, "_RegisterMainMenuWidget@4");
        }
        if (reg) {
            MenuWidgetInfo infso{ "notification_panel_caller", "notification_panel_caller Widget", &TestWidgetClick };
            reg(infso);
        } else {
            MessageBoxA(NULL, "Function RegisterMainMenuWidget not found!", "Debug", MB_OK);
        }
    } else {
        MessageBoxA(NULL, "Module Interface_MainMenuAPI.era not found!", "Debug", MB_OK);
    }
    h->Undo();
    return EXEC_DEFAULT;

    DWORD_PTR *pIsIconic = reinterpret_cast<DWORD_PTR *>(0x0063A2A8); // Адрес указателя на IsIconic

    // Сохраняем оригинальный указатель на IsIconic
    DWORD_PTR originalIsIconic = *pIsIconic;

    // Меняем защиту памяти на доступную для записи
    DWORD oldProtect;
    if (VirtualProtect(pIsIconic, sizeof(DWORD_PTR), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        // Заменяем указатель на нашу функцию
        *pIsIconic = reinterpret_cast<DWORD_PTR>(&_IsIconic);

        // Возвращаем исходную защиту памяти
        VirtualProtect(pIsIconic, sizeof(DWORD_PTR), oldProtect, &oldProtect);
    }

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        if (!plugin_On)
        {

            plugin_On = 1;

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            globalPatcher = GetPatcher();
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteLoHook(0x4EDFFD, GameStart);


        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

SYSTEMTIME operator-(const SYSTEMTIME &pSr, const SYSTEMTIME &pSl)
{
    SYSTEMTIME t_res;
    FILETIME v_ftime;
    ULARGE_INTEGER v_ui;
    __int64 v_right, v_left, v_res;

    SystemTimeToFileTime(&pSr, &v_ftime);
    v_ui.LowPart = v_ftime.dwLowDateTime;
    v_ui.HighPart = v_ftime.dwHighDateTime;
    v_right = v_ui.QuadPart;

    SystemTimeToFileTime(&pSl, &v_ftime);
    v_ui.LowPart = v_ftime.dwLowDateTime;
    v_ui.HighPart = v_ftime.dwHighDateTime;
    v_left = v_ui.QuadPart;

    v_res = v_right - v_left;
    v_ui.QuadPart = v_res;
    v_ftime.dwLowDateTime = v_ui.LowPart;
    v_ftime.dwHighDateTime = v_ui.HighPart;

    FileTimeToSystemTime(&v_ftime, &t_res);
    return t_res;
}

EXTERN_C __declspec(dllexport) SYSTEMTIME &GetTimeDifference(const SYSTEMTIME &pSr, const SYSTEMTIME &pSl,
                                                             SYSTEMTIME &res)
{

    res = pSr - pSl;
    return res;
}
