#include "framework.h"

#include "ModListScanner.h"

namespace dllText
{
const char *PLUGIN_VERSION = "1.10.0";
const char *INSTANCE_NAME = "EraPlugin.AssemblyInformation.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
//	const char* PROJECT_NAME = "$(ProjectName)";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

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

// Объявление структуры и типа функции для регистрации виджета

int __stdcall GameStart(LoHook *h, HookContext *c)
{
    const BOOL needRestart = assemblyModList::Get();
    h->Undo();
    if (needRestart && Era::RestartCurrentProcess())
        return EXEC_DEFAULT;

    AssemblyInformation::Get();
    // info->LoadDataFromJson();
    // UserNotification::Get();

    // Динамически ищем функцию экспорта из Interface_MainMenuAPI

    using namespace mainmenu;
    const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::MAIN | eMenuFlags::AT_BOTTOM);
    MenuWidgetInfo info{NotificationPanel::PARENT_BUTTON_CALLER_NAME, h3_NullString, flags,
                        NotificationPanel::OnPanelCallerClick};
    MainMenu_RegisterWidget(info);

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
