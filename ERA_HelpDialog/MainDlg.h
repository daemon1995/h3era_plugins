#pragma once
#include "CataloguePage.h"
#include "ModInformation.h"
#include "SearchPanel.h"
#include <map>

namespace main
{
class HeaderPage;
class HotkeysSection;
class ModSection;
class PlaceholderSection;
class MainDlg : public H3Dlg
{
    static MainDlg *instance;
    HeaderPage *header = nullptr;
    ModSection *modSection = nullptr;
    HotkeysSection *hotkeysSection = nullptr;
    PlaceholderSection *placeholder = nullptr;
    helpdlg::SearchPanel *searchPanel = nullptr;
    DlgSection *activeSection = nullptr;
    std::map<eHelpPage, helpdlg::Catalogue> catalogues;
    std::map<eHelpPage, CatalogueSection *> catalogueSections;
    std::vector<ModInformation *> mods;
    ModInformation *activeMod = nullptr;
    bool modsLoaded = false;
    eHelpPage initialPage, activePage;
    int initialSubtype = 0, activeSubtype = 0;
    int bodyY = 100, bodyHeight = 0, contentX = 232, contentWidth = 0;
    int pendingObject = -1;
    std::string searchQuery;

    BOOL OnCreate() override;
    VOID OnOK() override;
    VOID OnCancel() override;
    VOID OnClose(INT itemId) override;
    BOOL DialogProc(H3Msg &msg) override;
    BOOL OnMouseWheel(INT direction) override;
    bool EnsureModsLoaded();
    const helpdlg::Catalogue &EnsureCatalogue(eHelpPage page);
    CatalogueSection *EnsureCatalogueSection(eHelpPage page);
    void ShowSection(DlgSection *section);
    void ShowMod(ModInformation *mod, int subtype);
    void ShowEmpty(eHelpPage page, LPCSTR text);
    void Search();
    void CloseSearch();
    void ReleaseInputFocus(int keepItemId = -1);
    void NavigateSearch(const helpdlg::SearchEntry &target);
    void PreviewSearch(const helpdlg::SearchEntry &target);
    void ExecuteAction(LPCSTR action);
    void StoreState();
    static BOOL RunMainDlg(eHelpPage page, int subtype, BOOL remember, int objectId = -1);

  public:
    static constexpr LPCSTR iniPath = "ERA_HelpDialog.ini";
    static constexpr LPCSTR MAIN_MENU_WIDGET_NAME = "main_menu_help_dialogue_made_by_a_confused_person";
    MainDlg(int width, int height, int x = -1, int y = -1, eHelpPage page = eHelpPage::MODS, int subtype = 0);
    ~MainDlg();
    BOOL ShowPage(eHelpPage page, int subtype = -1);
    void AssignWithCalledDlg(const H3Town *town = nullptr, eCreature creature = eCreature::UNDEFINED) noexcept;
    static BOOL DlgExists()
    {
        return instance != nullptr;
    }
    static void PrepareMainDlg(HookContext *context = nullptr);
    static BOOL PrepareMainDlg(eHelpPage page, int subtype = 0);
    static BOOL ShowObject(eHelpPage page, int objectId);
    static int __fastcall MainMenuButtonProc(void *message);
};
} // namespace main
DllExport BOOL __stdcall ERAHelp_ShowDialog(main::eHelpPage page, int subtype = 0);
DllExport BOOL __stdcall ERAHelp_ShowObject(main::eHelpPage page, int objectId);
DllExport BOOL __stdcall ERAHelp_ShowObjectHint(main::eHelpPage page, int objectId, BOOL popup);
