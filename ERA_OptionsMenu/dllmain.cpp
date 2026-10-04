// dllmain.cpp : Defines the entry point for the DLL application.
/**

@NOTES:
    - hook/read original zsetup01.txt/*.ers files the data already read by the WoG:
        - 007792D3;
        - 0077818F;

*/

#include "pch.h"
#include "OptionsRuntime.h"
#include "ObjectBansRuntime.h"
#include "../headers/EraPluginsAPI/MainMenuAPI.hpp"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

BOOL CreateAndRunEraMenuDlg();
BOOL CreateAndRunObjectBansDlg();

namespace
{
constexpr int ScenarioBansId = 4500;
constexpr const char *BansWidget = "era_options_object_bans";
bool bansWidgetRegistered = false;
const char *BansButtonText()
{
    constexpr const char *key = "era_options.bans_ui.menu_button";
    const char *text = Era::tr(key);
    return text && strcmp(text, key) ? text : "Object bans";
}
int __fastcall ObjectBansMenuButtonProc(void *message)
{
    if (message && static_cast<H3Msg *>(message)->IsLeftClick()) CreateAndRunObjectBansDlg();
    return TRUE;
}
void AddScenarioBansButton(H3BaseDlg *dlg, bool campaignFallback = false)
{
    if (!dlg || dlg->GetCaptionButton(ScenarioBansId)) return;
    auto *wog = dlg->GetCaptionButton(4444);
    if (!wog && !campaignFallback) return;
    auto *button = dlg->CreateCaptionButton(wog ? wog->GetX() : 622, wog ? wog->GetY() - 45 : 154,
        wog ? wog->GetWidth() : 158, wog ? wog->GetHeight() : 32, ScenarioBansId,
        wog && wog->GetDef() ? wog->GetDef()->GetName() : "GSPBUT2.DEF", BansButtonText(), NH3Dlg::Text::SMALL, 0);
    if (button) button->SetClickFrame(1);
}
} // namespace

void __stdcall OnOptionsLanguageReload(Era::TEvent *)
{
    // Re-read descriptions on the next normal API/menu call; keep IDs/values.
    era_options::InvalidateOptionTexts();
    if (bansWidgetRegistered) mainmenu::MainMenu_SetDialogButtonText(BansWidget, BansButtonText());
}

int __stdcall Dlg_SelectScenario_Dlg(HiHook *h, H3Msg *msg)
{
    if (msg->itemId == ScenarioBansId && msg->IsLeftClick())
    {
        CreateAndRunObjectBansDlg();
        return 0;
    }

    return THISCALL_1(int, h->GetDefaultFunc(), msg);
}

int __stdcall ScenarioObjectBansProc(HiHook *hook, H3SelectScenarioDialog *dlg, H3Msg *msg)
{
    if (msg && msg->itemId == ScenarioBansId && msg->IsLeftClick())
    {
        CreateAndRunObjectBansDlg();
        return 0;
    }
    return THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
}

void __stdcall NewScenarioDlg_Create(HiHook *hook, H3SelectScenarioDialog *dlg, const int type)
{
    THISCALL_2(int, hook->GetDefaultFunc(), dlg, type);
    if (auto *wog = dlg->GetCaptionButton(4444)) wog->AddHotkey(eVKey::H3VK_W);
    AddScenarioBansButton(dlg);
}
_LHF_(SelectScenarioDlg_Campaign_BeforeRun)
{
    if (!bansWidgetRegistered) AddScenarioBansButton(reinterpret_cast<H3BaseDlg *>(c->ecx), true);
    return EXEC_DEFAULT;
}
_LHF_(ShowEraOptionsInsteadOfWog)
{
    // 0x7790E1 already checked the clicked WoG button at this point. Skip
    // global->current copying, old ItemState processing (0x778ECC), and the
    // current->global copy after the old dialog. Use its real epilogue.
    CreateAndRunEraMenuDlg();
    c->return_address = 0x77925F;
    return NO_EXEC_DEFAULT;
}

_LHF_(ConfluxGrailSpellBan)
{
    static_assert(sizeof(H3Spell) == 0x88, "Conflux loop expects the native spell layout");
    // The native loop keeps a byte offset into the spell table in ESI.
    if (era_options::SkipBannedConfluxSpell(c->esi / sizeof(H3Spell)))
    {
        c->return_address = 0x5BE565;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(HooksInit)
{
    static bool initialized = false;
    if (initialized) return EXEC_DEFAULT;
    initialized = true;
    // MainMenuAPI orders ON_TOP widgets before ordinary widgets, including
    // the existing WoG-options entry, in every menu context.
    mainmenu::MenuWidgetInfo widget{BansWidget, BansButtonText(),
        static_cast<mainmenu::eMenuFlags>(mainmenu::ALL | mainmenu::ON_TOP), ObjectBansMenuButtonProc};
    bansWidgetRegistered = mainmenu::MainMenu_RegisterWidget(widget) != FALSE;
    // JSON is loaded on the first menu/API call, after VFS and Lang are ready.
    _PI->WriteHiHook(0x5FFAC0, THISCALL_, Dlg_SelectScenario_Dlg);

    _PI->WriteLoHook(0x779170, ShowEraOptionsInsteadOfWog);
    _PI->WriteHiHook(0x579CE0, SPLICE_, EXTENDED_, THISCALL_, NewScenarioDlg_Create);
    _PI->WriteHiHook(0x587FD0, SPLICE_, EXTENDED_, THISCALL_, ScenarioObjectBansProc);
    _PI->WriteLoHook(0x45AE90, SelectScenarioDlg_Campaign_BeforeRun); // load campaign
    _PI->WriteLoHook(0x5BE55D, ConfluxGrailSpellBan);

    // Open on an explicit menu/button action, not automatically at game startup.

    //_PI->WriteLoHook(0x4F0900, SelectScenarioDlg_Campaign_BeforeRun); // campaign from MM

    // _PI->WriteHiHook(0x513993, SPLICE_, EXTENDED_, THISCALL_, NewScenarioDlg_Create);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool pluginIsLoaded = false;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!pluginIsLoaded)
        {
            pluginIsLoaded = true;

            globalPatcher = GetPatcher();
            constexpr auto insanceName = "ERA.OptionsMenu.Plugin";
            _PI = globalPatcher->CreateInstance(insanceName);
            Era::ConnectEra(hModule, insanceName);
            Era::RegisterHandler(OnOptionsLanguageReload, "OnAfterReloadLanguageData");
            era_options::RegisterObjectBanEvents();
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
