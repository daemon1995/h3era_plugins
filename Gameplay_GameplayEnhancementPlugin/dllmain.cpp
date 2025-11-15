// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
const char *PLUGIN_VERSION = "1.7.0";
const char *INSTANCE_NAME = "EraPlugin.GameplayFeatures.daemon_n";
const char *PLUGIN_AUTHOR = "daemon_n";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText

void __stdcall OnReportVersion(Era::TEvent *e)
{
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}

constexpr LPCSTR GEM_OPTIONS_BUTTON_NAME = "gem_map_menu";
constexpr LPCSTR GEM_COMBAT_BUTTON_NAMES[] = {"gem_real_bg", "gem_temp_bg", "manualf",  "automf",    "autowm",
                                              "choiceB",     "optionB",     "mithrilB", "indicator", "mit_res"};

// redraw adventure manager dialog buttons to player color in multiplayer
void __stdcall H3AdventureMgrDlg__SetButtonsPlayerColor(HiHook *h, H3AdventureMgrDlg *dlg, char isMultiplayer, int a3)
{
    THISCALL_3(void, h->GetDefaultFunc(), dlg, isMultiplayer, a3);

    if (isMultiplayer)
    {
        const int buttonId = Era::GetButtonID(GEM_OPTIONS_BUTTON_NAME);
        auto button = dlg->GetDefButton(buttonId);

        if (button)
            button->Draw();

        int minButtonId = 0, maxButtonId = 0;
        for (LPCSTR btnName : GEM_COMBAT_BUTTON_NAMES)
        {
            const int cmbButtonId = Era::GetButtonID(btnName);
            if (cmbButtonId > -1)
            {
                if (cmbButtonId < minButtonId || minButtonId == 0)
                    minButtonId = cmbButtonId;
                if (cmbButtonId > maxButtonId)
                    maxButtonId = cmbButtonId;
            }
        }

        if (minButtonId && maxButtonId)
        {
            dlg->RedrawItemRange(a3, minButtonId, maxButtonId); // redraw combat buttons
        }
    }
}

_LHF_(HooksInit)
{

    scroll::MapScroller::Get();

    // if (EraJS::readInt("gem_plugin.combat_hints.enable"))
    graphics::GraphicsEnhancements::Get();
    cmbhints::CombatHints::Get();

    features::GameplayFeature::Get();
    ERI::ExtendedResourcesInfo::Get();
    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";
    if (globalPatcher->GetInstance(vipPluginInstanceName) == nullptr)
    {
        advMapHints::AdventureMapHints::Init(globalPatcher->CreateInstance(vipPluginInstanceName));
    }
    artifacts::ArtifactHints::Get();

    _PI->WriteHiHook(0x0403F60, THISCALL_, H3AdventureMgrDlg__SetButtonsPlayerColor);

    return EXEC_DEFAULT;
}

const char *demoBttn = "iDEMO.def";
Patch *demolishButtonPatch = nullptr;
_LHF_(WoG_BeforeTownbuildingDemolishQuestion)
{

    demolishButtonPatch->Apply();
    return EXEC_DEFAULT;
}

_LHF_(WoG_AfterTownbuildingDemolishQuestion)
{
    demolishButtonPatch->Undo();
    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginIsOn = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsOn)
        {
            pluginIsOn = true;

            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);

            _PI->WriteLoHook(0x4EEAF2, HooksInit);
            _PI->WriteLoHook(0x070AD9A, WoG_BeforeTownbuildingDemolishQuestion);
            _PI->WriteLoHook(0x070C1A1, WoG_AfterTownbuildingDemolishQuestion);
            demolishButtonPatch = _PI->CreateDwordPatch(0x04F738A + 1, (int)demoBttn);
            // move and resize iam00.def (next hero buttn)
            constexpr BYTE defWidth = 32;

            _PI->WriteByte(0x401A85 + 1, defWidth);        // set width
            _PI->WriteDword(0x401A8C + 1, 679 + defWidth); // set y
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
