// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

namespace dllText
{
LPCSTR PLUGIN_VERSION = "0.01";
LPCSTR INSTANCE_NAME = "EraPlugin.Help.daemon_n";
LPCSTR PLUGIN_AUTHOR = "daemon_n";
//	const char* PROJECT_NAME = "$(ProjectName)";
LPCSTR PLUGIN_DATA = __DATE__;
} // namespace dllText

Patcher *globalPatcher;
PatcherInstance *_PI;

enum H3DlgVTables : DWORD
{

    H3TownSmallDlg = 0x00640704,
    H3CreatureSmallDlg = 0x06406DC,
};

const H3Town *townFromClick = nullptr;
eCreature creatureFromClick = eCreature::UNDEFINED;

// Main window procedure

_LHF_(MainWindow_F1)
{
    if (!main::HelpDlg::DlgExists())
    {
        P_SoundManager->ClickSound();

        Era::ReadStrFromIni("FullScreen", "Help", main::HelpDlg::iniPath, h3_TextBuffer);

        const H3Town *town = nullptr;

        eCreature creature = eCreature::UNDEFINED;

        const DWORD currentDlgVTable = *reinterpret_cast<DWORD *>(P_WindowManager->lastDlg);

        if (currentDlgVTable)
        {

            switch (currentDlgVTable)
            {
            case H3DlgVTables::H3TownSmallDlg:
                town = ::townFromClick;
                break;
            case H3DlgVTables::H3CreatureSmallDlg:
                creature = ::creatureFromClick;
                break;
            default:
                break;
            }
        }

        if (town)
        {
            //   H3Messagebox(town->name);
            //            libc::sprintf(Era::z[0], "%s", town->name);
            //          Era::ExecErmCmd("IF:L^%z1^;");
        }
        if (creature != eCreature::UNDEFINED)
        {
            //    H3Messagebox(P_CreatureInformation[creature].namePlural);
        }
        //   Era::y[55] = **reinterpret_cast<DWORD**>(P_WindowManager->lastDlg);
        //  Era::ExecErmCmd("IF:L^%y55^;");

        bool isFullScreen = atoi(h3_TextBuffer);
        do
        {
            const int dialogWidth = isFullScreen ? H3GameWidth::Get() - 6 : 800;
            const int dialogHeight = isFullScreen ? H3GameHeight::Get() - 6 : 600;

            // create help dialog
            main::HelpDlg dialog(dialogWidth, dialogHeight);

            // initialize help dialog
            //	dialog.AssignWithCalledDlg();

            // start help dialog

            dialog.Start();
            //            town ? dialog.RMB_Show(): dialog.Start();
            if (town == nullptr && 0)
            {
                c->return_address = 0x04F8710;
                return NO_EXEC_DEFAULT;
            }
            // reverse isFullScreen after dialog closed
            if (dialog.NeedResizeScreen())
            {
                Era::WriteStrToIni("FullScreen", (isFullScreen ^= 1) ? "1" : "0", "Help", main::HelpDlg::iniPath);
                Era::SaveIni(main::HelpDlg::iniPath);
            }
            else
            {
                break;
            }

        } while (TRUE);
    }

    // skip original code
    c->return_address = 0x4F877D;
    return NO_EXEC_DEFAULT;
}

void __stdcall H3TownSmallDlg__Constructor(HiHook *hook, const H3Dlg *dlg, const H3Town *town, const int info)
{
    ::townFromClick = town;
    THISCALL_3(void, hook->GetDefaultFunc(), dlg, town, info);
}

void __stdcall H3CreatureSmallDlg__Constructor(HiHook *hook, const H3Dlg *dlg, const int vision,
                                               const eCreature creature, const int count, const int diploType,
                                               const int cost)
{
    ::creatureFromClick = creature;
    THISCALL_6(void, hook->GetDefaultFunc(), dlg, vision, creature, count, diploType, cost);
}
_LHF_(HooksInit)
{

    // intercept F1 already pressed
    _PI->WriteLoHook(0x4F8751, MainWindow_F1);
    
    /** Intercept other dlgs to marke needed dlgs are openeed (F1 may be pressed) to show help from there.
    * By default supported only creatures and towns because only those are in the help dlg rn
    */
    _PI->WriteHiHook(0x530600, THISCALL_, H3TownSmallDlg__Constructor);
    _PI->WriteHiHook(0x52FDA0, THISCALL_, H3CreatureSmallDlg__Constructor);


    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static BOOL pluginIsOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            Era::ConnectEra();
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
