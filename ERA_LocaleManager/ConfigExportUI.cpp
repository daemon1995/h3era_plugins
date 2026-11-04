#include "../headers/EraPluginsAPI/ConfigExportUIAPI.h"
#include "ExportManager.h"
namespace
{
bool registered = false;
int __stdcall ShowExport(unsigned mode)
{
    if (mode > era_config_ui::Translations)
        return 0;
    try
    {
        ExportDlg dialog(mode);
        dialog.Start();
        return 1;
    }
    catch (const std::exception &error)
    {
        H3Messagebox::Show(error.what());
        return 0;
    }
    catch (...)
    {
        return 0;
    }
}
int __fastcall Menu(void *msg)
{
    if (msg && static_cast<H3Msg *>(msg)->IsLeftClick())
    {
        ShowExport(era_config_ui::Configuration);
    }
    return 1;
}
const era_config_ui::Api api{sizeof(api), 1, ShowExport};
const char *MenuText()
{
    bool found = false;
    const char *text = EraJS::read("era.configExport.menu", found);
    return found ? text : "Export configuration";
}
void __stdcall InitializeMenu(Era::TEvent *)
{
    if (registered)
        return;
    mainmenu::MenuWidgetInfo info{"ERA.ConfigExport", MenuText(), mainmenu::MAIN, Menu};
    registered = mainmenu::MainMenu_RegisterWidget(info) != FALSE;
}
void __stdcall Reload(Era::TEvent *)
{
    if (registered)
        mainmenu::MainMenu_SetDialogButtonText("ERA.ConfigExport", MenuText());
}
} // namespace
void InitializeConfigExportUi()
{
    era_services::Publish(era_config_ui::SERVICE, &api);
    // Era::RegisterHandler(InitializeMenu, "OnReportVersion");
    Era::RegisterHandler(Reload, "OnAfterReloadLanguageData");
}
