#pragma once

#include "HelpDialogDependencies.h"
#include "DlgEnums.h"
#include "ModInformation.h"

namespace main
{
struct HotKeysCategory;
class HelpSection;
class HeaderPage;
class HotkeysSection;
class ModSection;
class PlaceholderSection;
class CreaturesSection;
class ArtifactsSection;
class TownsSection;
class HeroesSection;
class SpellsSection;

class MainDlg : public H3Dlg
{
    static MainDlg *instance;
    BOOL needResize = FALSE;
    BOOL isFullScreen = FALSE;
    BOOL initialized = FALSE;
	H3DlgPcx16* backgroundDlgPcx16 = nullptr;
    HeaderPage *headerPage = nullptr;
    ModSection *modSection = nullptr;
    PlaceholderSection *placeholderSection = nullptr;
    HotkeysSection *hotkeysSection = nullptr;
    CreaturesSection *creaturesSection = nullptr;
    ArtifactsSection *artifactsSection = nullptr;
    TownsSection *townsSection = nullptr;
    HeroesSection *heroesSection = nullptr;
    SpellsSection *spellsSection = nullptr;
    HelpSection *activeSection = nullptr;

    int headerX = 0;
    int categoriesY = 0;
    int categoriesWidth = 0;
    int categoriesHeight = 0;
    int contentX = 0;
    int contentWidth = 0;

    eHelpPage initialPage = eHelpPage::MODS;
    int initialSubtype = 0;
    eHelpPage activePage = eHelpPage::MODS;
    int activeSubtype = 0;

    ModInformation *m_activeMod = nullptr;
    int lastActiveModId = -1;
    BOOL modsLoaded = FALSE;

    std::vector<ModInformation *> mods;

  public:
    static constexpr LPCSTR iniPath = "ERA_HelpDialog.ini";
    static constexpr LPCSTR MAIN_MENU_WIDGET_NAME = "main_menu_help_dialogue_made_by_a_confused_person";
    static constexpr LPCSTR MAIN_MENU_WIDGET_TEXT = "help_dialogue_button";

    MainDlg(int width, int height, int x = -1, int y = -1, eHelpPage page = eHelpPage::MODS,
            int subtype = 0);
    virtual ~MainDlg();

  private:
    const Content *ActiveContent() const noexcept;
    void CallHelpInHelpDlg() const noexcept;
    void DisplayAllHotkeys() noexcept;
    ModInformation *CallModListDlg(const ModInformation *activeMod) noexcept;
    BOOL EnsureModsLoaded();
    HelpSection *EnsureSection(eHelpPage page);
    BOOL GetLoadedModsJsonInformation(const std::vector<std::string> &modNames);
    void SetActiveMod(ModInformation *mod);
    void ShowSection(HelpSection *section);
    void HidePages() noexcept;
    static BOOL RunMainDlg(eHelpPage page, int subtype, BOOL rememberPage);

    BOOL OnCreate() override;
    VOID OnOK() override;
    VOID OnCancel() override;
    VOID OnClose(INT itemId) override;
    BOOL DialogProc(H3Msg &msg) override;

  public:
    void AssignWithCalledDlg(const H3Town *town = nullptr, const eCreature creature = eCreature::UNDEFINED) noexcept;
    static BOOL DlgExists();

    BOOL ShowPage(eHelpPage page, int subtype = 0);
    void ShowCreatures(int subtype = 0);
    void ShowHotkeys(int subtype = 0);
    void ShowMod(ModInformation *mod, int subtype = 0);
    void ShowPlaceholder(int buttonId, LPCSTR text);
    void ShowArtifacts(int subtype = 0);
    void ShowTowns(int subtype = 0);
    void ShowHeroes(int subtype = 0);
    void ShowSpells(int subtype = 0);
    static void PrepareMainDlg(HookContext *c = nullptr);
    static BOOL PrepareMainDlg(eHelpPage page, int subtype = 0);
    static int __fastcall MainMenuButtonProc(void *msg);
};

} // namespace main

// Opens the dialog on a requested page. subtype is page-specific and defaults
// to the first/all category. The call must be made from the game's UI thread.
DllExport BOOL __stdcall ERAHelp_ShowDialog(main::eHelpPage page, int subtype = 0);
