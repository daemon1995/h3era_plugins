#include "MainDlg.h"

#include "ArtifactsPage.h"
#include "CreaturesPage.h"
#include "GuideDlg.h"
#include "HeaderPage.h"
#include "HeroesPage.h"
#include "HotkeysPage.h"
#include "ModListDlg.h"
#include "ModPage.h"
#include "PlaceholderPage.h"
#include "SystemFunctions.h"
#include "SpellsPage.h"
#include "TownsPage.h"

extern const H3Town *townFromClick;
extern eCreature creatureFromClick;

namespace main
{

MainDlg *MainDlg::instance = nullptr;

namespace
{
constexpr eHelpPage DEFAULT_PAGE = eHelpPage::MODS;

BOOL IsValidPage(const eHelpPage page) noexcept
{
    switch (page)
    {
    case eHelpPage::MODS:
    case eHelpPage::HOTKEYS:
    case eHelpPage::CREATURES:
    case eHelpPage::ARTIFACTS:
    case eHelpPage::TOWNS:
    case eHelpPage::HEROES:
    case eHelpPage::SECONDARY_SKILLS:
    case eHelpPage::SPELLS:
        return TRUE;
    default:
        return FALSE;
    }
}

int ReadHelpIniInt(LPCSTR key, const int defaultValue) noexcept
{
    h3_TextBuffer[0] = '\0';
    Era::ReadStrFromIni(key, "Help", MainDlg::iniPath, h3_TextBuffer);
    return h3_TextBuffer[0] ? atoi(h3_TextBuffer) : defaultValue;
}

void WriteHelpIniInt(LPCSTR key, const int value) noexcept
{
    libc::sprintf(h3_TextBuffer, "%d", value);
    Era::WriteStrToIni(key, h3_TextBuffer, "Help", MainDlg::iniPath);
}
} // namespace

MainDlg::MainDlg(const int width, const int height, const int x, const int y, const eHelpPage page,
                 const int subtype)
    : H3Dlg(width, height, x, y, 1, 0), initialPage(page), initialSubtype(std::max(0, subtype))
{
    // Disable dialog shadow.
    flags ^= 16;
    instance = this;

    background = H3LoadedPcx16::Create(h3_NullString, width, height);
    memset(background->buffer, 0, background->buffSize);

    H3DlgPcx16 *backDlgPcx = H3DlgPcx16::Create(0, 0, background->width, background->height, 0, nullptr);
    backDlgPcx->SetPcx(background);

    AddItem(backDlgPcx);

    constexpr int borderMargin = 8;
    constexpr int panelsMargin = 4;
    constexpr int headerY = borderMargin;
    constexpr int headerHeight = 60;
    headerX = borderMargin;
    categoriesWidth = 220;

    H3DlgDefButton *okButton = H3DlgDefButton::Create(25, heightDlg - 50, int(eControlId::OK), NH3Dlg::Assets::OKAY_DEF,
                                                      0, 1, TRUE, NH3VKey::H3VK_ENTER);
    okButton->AddHotkey(eVKey::H3VK_ESCAPE);
    okButton->AddHotkey(eVKey::H3VK_F1);

    const int okWidth = okButton->GetWidth();
    const int okHeight = okButton->GetHeight();
    const int okX = width - okWidth - borderMargin;
    const int okY = height - okHeight - borderMargin;
    okButton->SetX(okX - 1);
    okButton->SetY(okY + 1);

    H3RGB565 color(H3RGB888::Highlight());
    CreateFrame(okButton, color, -1, 1);
    AddItem(okButton);

    hintBar = H3DlgHintBar::Create(this, borderMargin + 1, okY + 1,
                                   width - (panelsMargin + okWidth + 2 + borderMargin * 2), okHeight);
    CreateFrame(hintBar, color, -1, 1);
    AddItem(hintBar);

    categoriesY = headerY + headerHeight + panelsMargin;
    categoriesHeight = height - (headerY * 2 + headerHeight + panelsMargin * 2 + okHeight);
    contentX = headerX + categoriesWidth + panelsMargin;
    contentWidth = width - categoriesWidth - headerX * 2 - panelsMargin;

    // The header is always needed. Content sections are created on their first
    // use so opening a targeted page does not allocate every other catalogue.
    headerPage = new HeaderPage(headerX, headerY, width - headerX * 2, headerHeight, this);

    // Select the initial page, but do not activate its items before OnCreate.
    headerPage->SetActiveButton(static_cast<int>(initialPage));
}

MainDlg::~MainDlg()
{
    HidePages();

    instance = nullptr;
    delete townsSection;
    delete heroesSection;
    delete spellsSection;
    delete artifactsSection;
    delete hotkeysSection;
    delete modSection;
    delete placeholderSection;
    delete creaturesSection;
    delete headerPage;

    if (background)
    {
        background->Destroy();
        background = nullptr;
    }

    for (auto *mod : mods)
    {
        delete mod;
    }
    if (resultItemId == buttons::RESIZE_DLG)
    {
        P_WindowManager->resultItemID = buttons::RESIZE_DLG;
    }
}

const Content *MainDlg::ActiveContent() const noexcept
{
    return m_activeMod && m_activeMod->activeCategory ? m_activeMod->activeCategory->content : nullptr;
}

void MainDlg::CallHelpInHelpDlg() const noexcept
{
    help::GuideDlg dlg(500, 500);
    dlg.Start();
}

void MainDlg::DisplayAllHotkeys() noexcept
{
    ShowHotkeys();
}

ModInformation *MainDlg::CallModListDlg(const ModInformation *activeMod) noexcept
{
    (void)activeMod;
    int dropdownX = 0;
    int dropdownY = 0;
    int dropdownWidth = 240;
    if (auto *modsButton = GetH3DlgItem(buttons::MODLIST))
    {
        dropdownX = modsButton->GetX();
        dropdownY = modsButton->GetY() + modsButton->GetHeight();
        dropdownWidth = std::max(dropdownWidth, modsButton->GetWidth());
    }
    const int rowHeight = 34;
    const int maxRows = std::max(1, (H3GameHeight::Get() - dropdownY - 24) / rowHeight);
    const int visibleRows = std::min(maxRows, std::max(1, static_cast<int>(mods.size())));
    const int dropdownHeight = visibleRows * rowHeight + 54;
    list::ModListDlg dlg(dropdownWidth, dropdownHeight, dropdownX, dropdownY, mods);
    dlg.Start();
    return dlg.ResultMod();
}

BOOL MainDlg::EnsureModsLoaded()
{
    if (modsLoaded)
        return !mods.empty();

    modsLoaded = TRUE;
    std::vector<std::string> modNames;
    modList::GetEraModList(modNames, TRUE);
    if (!GetLoadedModsJsonInformation(modNames))
        return FALSE;

    m_activeMod = mods.front();
    return TRUE;
}

HelpSection *MainDlg::EnsureSection(const eHelpPage page)
{
    switch (page)
    {
    case eHelpPage::CREATURES:
        if (!creaturesSection)
            creaturesSection = new CreaturesSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX,
                                                    categoriesY, contentWidth, categoriesHeight, this);
        return creaturesSection;
    case eHelpPage::ARTIFACTS:
        if (!artifactsSection)
            artifactsSection = new ArtifactsSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX,
                                                    categoriesY, contentWidth, categoriesHeight, this);
        return artifactsSection;
    case eHelpPage::HEROES:
        if (!heroesSection)
            heroesSection = new HeroesSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX,
                                              categoriesY, contentWidth, categoriesHeight, this);
        return heroesSection;
    case eHelpPage::SPELLS:
        if (!spellsSection)
            spellsSection = new SpellsSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX,
                                              categoriesY, contentWidth, categoriesHeight, this);
        return spellsSection;
    case eHelpPage::TOWNS:
        if (!townsSection)
            townsSection = new TownsSection(this);
        return townsSection;
    case eHelpPage::HOTKEYS:
        EnsureModsLoaded();
        if (!hotkeysSection)
            hotkeysSection = new HotkeysSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX,
                                                categoriesY, contentWidth, categoriesHeight, this, mods);
        return hotkeysSection;
    case eHelpPage::MODS:
        EnsureModsLoaded();
        if (!modSection)
            modSection = new ModSection(headerX, categoriesY, categoriesWidth, categoriesHeight, contentX, categoriesY,
                                        contentWidth, categoriesHeight, this);
        return modSection;
    case eHelpPage::SECONDARY_SKILLS:
        if (!placeholderSection)
            placeholderSection = new PlaceholderSection(contentX, categoriesY, contentWidth, categoriesHeight, this);
        return placeholderSection;
    default:
        return nullptr;
    }
}

BOOL MainDlg::GetLoadedModsJsonInformation(const std::vector<std::string> &modNames)
{
    UINT modId = 0;
    for (const auto &modName : modNames)
    {
        ModInformation *mod = new ModInformation(modName.c_str(), modId++);
        if (mod->hasSomeInfo)
        {
            mods.emplace_back(mod);
        }
        else
        {
            delete mod;
        }
    }
    return !mods.empty();
}

void MainDlg::SetActiveMod(ModInformation *mod)
{
    if (m_activeMod == mod)
    {
        return;
    }
    if (m_activeMod)
    {
        m_activeMod->SetVisible(FALSE);
    }
    m_activeMod = mod;
    if (m_activeMod)
    {
        m_activeMod->SetVisible(TRUE);
    }
}

void MainDlg::ShowSection(HelpSection *section)
{
    // Treat the sections as mutually exclusive sources of dialog items. Do
    // not rely only on activeSection: a lazily-created page may have become
    // visible while another source was active.
    HelpSection *allSections[] = {creaturesSection, hotkeysSection, modSection, artifactsSection, townsSection,
                                  heroesSection, spellsSection, placeholderSection};
    for (auto *candidate : allSections)
    {
        if (candidate && candidate != section)
            candidate->SetVisible(FALSE);
    }
    activeSection = section;
    if (activeSection)
    {
        activeSection->SetVisible(TRUE);
        activeSection->Redraw();
    }
    // Do not draw from OnCreate: vShowAndRun() has not saved the underlying
    // screen yet. Runtime page switches happen after this dialog becomes the
    // window manager's active dialog and are redrawn normally.
    if (P_WindowManager->lastDlg == this)
    {
        Redraw();
    }
}

BOOL MainDlg::ShowPage(const eHelpPage page, const int subtype)
{
    switch (page)
    {
    case eHelpPage::CREATURES:
        ShowCreatures(subtype);
        return TRUE;
    case eHelpPage::ARTIFACTS:
        ShowArtifacts(subtype);
        return TRUE;
    case eHelpPage::TOWNS:
        ShowTowns(subtype);
        return TRUE;
    case eHelpPage::HEROES:
        ShowHeroes(subtype);
        return TRUE;
    case eHelpPage::SECONDARY_SKILLS:
        ShowPlaceholder(buttons::SECONDARY_SKILLS, "The secondary skills catalogue is not implemented yet.");
        return TRUE;
    case eHelpPage::SPELLS:
        ShowSpells(subtype);
        return TRUE;
    case eHelpPage::HOTKEYS:
        ShowHotkeys(subtype);
        return TRUE;
    case eHelpPage::MODS:
        if (EnsureModsLoaded() && m_activeMod)
        {
            ShowMod(m_activeMod, subtype);
            return TRUE;
        }
        ShowPlaceholder(buttons::MODLIST, "No mod help is available.");
        activePage = eHelpPage::MODS;
        activeSubtype = 0;
        return TRUE;
    default:
        return FALSE;
    }
}

void MainDlg::ShowCreatures(const int subtype)
{
    auto *section = static_cast<CreaturesSection *>(EnsureSection(eHelpPage::CREATURES));
    if (!section)
        return;
    section->SetSubtype(subtype);
    activePage = eHelpPage::CREATURES;
    activeSubtype = section->Subtype();
    headerPage->SetActiveButton(buttons::CREATURES);
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowHotkeys(const int subtype)
{
    auto *section = static_cast<HotkeysSection *>(EnsureSection(eHelpPage::HOTKEYS));
    if (!section)
        return;
    section->SetSubtype(subtype);
    activePage = eHelpPage::HOTKEYS;
    activeSubtype = section->Subtype();
    headerPage->SetActiveButton(buttons::HOTKEYS);
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowMod(ModInformation *mod, const int subtype)
{
    if (!mod)
        return;
    auto *section = static_cast<ModSection *>(EnsureSection(eHelpPage::MODS));
    if (!section)
        return;
    SetActiveMod(mod);
    headerPage->SetActiveButton(buttons::MODLIST);
    section->SetMod(mod);
    section->SetSubtype(subtype);
    activePage = eHelpPage::MODS;
    activeSubtype = section->Subtype();
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowPlaceholder(const int buttonId, LPCSTR text)
{
    auto *section = static_cast<PlaceholderSection *>(EnsureSection(eHelpPage::SECONDARY_SKILLS));
    if (!section)
        return;
    headerPage->SetActiveButton(buttonId);
    section->SetTitle(text);
    activePage = buttonId == buttons::MODLIST ? eHelpPage::MODS : eHelpPage::SECONDARY_SKILLS;
    activeSubtype = 0;
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowArtifacts(const int subtype)
{
    auto *section = static_cast<ArtifactsSection *>(EnsureSection(eHelpPage::ARTIFACTS));
    if (!section)
        return;
    section->SetSubtype(subtype);
    activePage = eHelpPage::ARTIFACTS;
    activeSubtype = section->Subtype();
    headerPage->SetActiveButton(buttons::ARTIFACTS);
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowTowns(const int subtype)
{
    auto *section = static_cast<TownsSection *>(EnsureSection(eHelpPage::TOWNS));
    if (!section)
        return;
    activePage = eHelpPage::TOWNS;
    activeSubtype = 0;
    headerPage->SetActiveButton(buttons::TOWNS);
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowHeroes(const int subtype)
{
    auto *section = static_cast<HeroesSection *>(EnsureSection(eHelpPage::HEROES));
    if (!section)
        return;
    section->SetSubtype(subtype);
    activePage = eHelpPage::HEROES;
    activeSubtype = section->Subtype();
    headerPage->SetActiveButton(buttons::HEROES);
    if (initialized)
        ShowSection(section);
}

void MainDlg::ShowSpells(const int subtype)
{
    auto *section = static_cast<SpellsSection *>(EnsureSection(eHelpPage::SPELLS));
    if (!section)
        return;
    section->SetSubtype(subtype);
    activePage = eHelpPage::SPELLS;
    activeSubtype = section->Subtype();
    headerPage->SetActiveButton(buttons::SPELLS);
    if (initialized)
        ShowSection(section);
}

BOOL MainDlg::OnCreate()
{
    initialized = TRUE;

    // The header is not part of the content page pair and is always active.
    headerPage->SetVisible(TRUE);
    if (!ShowPage(initialPage, initialSubtype))
        ShowPage(DEFAULT_PAGE);
    return TRUE;
}

void MainDlg::HidePages() noexcept
{
    if (headerPage)
    {
        headerPage->SetVisible(FALSE);
    }
    if (creaturesSection)
        creaturesSection->SetVisible(FALSE);
    if (hotkeysSection)
        hotkeysSection->SetVisible(FALSE);
    if (modSection)
        modSection->SetVisible(FALSE);
    if (placeholderSection)
        placeholderSection->SetVisible(FALSE);
    if (artifactsSection)
        artifactsSection->SetVisible(FALSE);
    if (townsSection)
        townsSection->SetVisible(FALSE);
    if (heroesSection)
        heroesSection->SetVisible(FALSE);
    if (spellsSection)
        spellsSection->SetVisible(FALSE);
    activeSection = nullptr;
    initialized = FALSE;
}

void MainDlg::OnOK()
{
    HidePages();
    Stop();
}

void MainDlg::OnCancel()
{
    HidePages();
    Stop();
}

void MainDlg::OnClose(INT itemId)
{
    (void)itemId;
    HidePages();
    Stop();
}

BOOL MainDlg::DialogProc(H3Msg &msg)
{
    if (activeSection && activeSection->ProcessMessage(msg))
    {
        activeSubtype = activeSection->Subtype();
        return 0;
    }

    if (msg.IsLeftClick())
    {
        switch (msg.itemId)
        {
        case buttons::MODLIST:
            EnsureModsLoaded();
            if (ModInformation *selectedMod = CallModListDlg(m_activeMod))
                ShowMod(selectedMod);
            return 0;
        case buttons::HOTKEYS:
            DisplayAllHotkeys();
            return 0;
        case buttons::CREATURES:
            ShowCreatures();
            return 0;
        case buttons::ARTIFACTS:
            ShowArtifacts();
            return 0;
        case buttons::TOWNS:
            ShowTowns();
            return 0;
        case buttons::HEROES:
            ShowHeroes();
            return 0;
        case buttons::SECONDARY_SKILLS:
            ShowPlaceholder(buttons::SECONDARY_SKILLS, "The secondary skills catalogue is not implemented yet.");
            return 0;
        case buttons::SPELLS:
            ShowSpells();
            return 0;
        case buttons::RESIZE_DLG:

            this->resultItemId = buttons::RESIZE_DLG;
            HidePages();
            Stop();

            return 0;
        case buttons::HELP:
            CallHelpInHelpDlg();
            return 0;
        default:
            break;
        }
    }
    else if (msg.IsRightClick())
    {
        if (H3DlgItem *item = GetH3DlgItem(msg.itemId))
        {
            if (LPCSTR rmcHint = *reinterpret_cast<LPCSTR *>(reinterpret_cast<char *>(item) + 0x24))
            {
                H3Messagebox::RMB(rmcHint);
            }
        }
    }

    if (hintBar && hintBar->IsVisible())
    {
        hintBar->ShowHint(&msg);
    }
    return 0;
}

void MainDlg::AssignWithCalledDlg(const H3Town *town, const eCreature creature) noexcept
{
    if (town)
    {
    }
    else if (creature != eCreature::UNDEFINED)
    {
    }
}

BOOL MainDlg::DlgExists()
{
    return instance != nullptr;
}

enum H3DlgVTables : DWORD
{
    H3TownSmallDlg = 0x00640704,
    H3CreatureSmallDlg = 0x06406DC,
};

void MainDlg::PrepareMainDlg(HookContext *c)
{
    (void)c;
    eHelpPage page = DEFAULT_PAGE;
    int subtype = 0;
    BOOL contextualCall = FALSE;
    const H3Town *town = nullptr;
    eCreature creature = eCreature::UNDEFINED;
    const DWORD currentDlgVTable = P_WindowManager->lastDlg
                                       ? *reinterpret_cast<DWORD *>(P_WindowManager->lastDlg)
                                       : 0;
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
        page = eHelpPage::TOWNS;
        contextualCall = TRUE;
    }
    else
    {
        ::townFromClick = nullptr;
    }
    if (creature != eCreature::UNDEFINED)
    {
        page = eHelpPage::CREATURES;
        contextualCall = TRUE;
    }
    else
        ::creatureFromClick = eCreature::UNDEFINED;

    if (contextualCall)
    {
        RunMainDlg(page, subtype, FALSE);
        return;
    }

    page = static_cast<eHelpPage>(ReadHelpIniInt("LastPage", static_cast<int>(DEFAULT_PAGE)));
    if (!IsValidPage(page))
        page = DEFAULT_PAGE;
    subtype = std::max(0, ReadHelpIniInt("LastSubtype", 0));
    RunMainDlg(page, subtype, TRUE);
}

BOOL MainDlg::PrepareMainDlg(const eHelpPage page, const int subtype)
{
    return RunMainDlg(page, subtype, FALSE);
}

BOOL MainDlg::RunMainDlg(const eHelpPage requestedPage, const int subtype, const BOOL rememberPage)
{
    if (DlgExists())
        return FALSE;

    const int storeResult = P_WindowManager->resultItemID;

    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();
    bool isFullScreen = ReadHelpIniInt("FullScreen", 0) != 0;
    eHelpPage pageToOpen = IsValidPage(requestedPage) ? requestedPage : DEFAULT_PAGE;
    int subtypeToOpen = std::max(0, subtype);

    do
    {
        const int dialogWidth = Clamp(800, isFullScreen ? gameWidth - 6 : 800, gameWidth);
        const int dialogHeight = Clamp(600, isFullScreen ? gameHeight - 6 : 600, gameHeight);
        int dialogResult = 0;
        {
            MainDlg dialog(dialogWidth, dialogHeight, -1, -1, pageToOpen, subtypeToOpen);
            dialog.Start();
            pageToOpen = dialog.activePage;
            subtypeToOpen = dialog.activeSubtype;
            dialogResult = P_WindowManager->resultItemID;
        }

        if (dialogResult == buttons::RESIZE_DLG)
        {

            Era::WriteStrToIni("FullScreen", (isFullScreen ^= 1) ? "1" : "0", "Help", MainDlg::iniPath);
            Era::SaveIni(MainDlg::iniPath);
        }
        else
        {
            break;
        }
    } while (TRUE);

    if (rememberPage)
    {
        WriteHelpIniInt("LastPage", static_cast<int>(pageToOpen));
        WriteHelpIniInt("LastSubtype", subtypeToOpen);
        Era::SaveIni(MainDlg::iniPath);
    }
    P_WindowManager->resultItemID = storeResult;
    return TRUE;
}

int __fastcall MainDlg::MainMenuButtonProc(void *msg)
{
    if (auto mes = static_cast<H3Msg *>(msg))
    {
        if (mes->IsLeftClick())
        {
            PrepareMainDlg();
        }
    }
    return true;
}

} // namespace main

DllExport BOOL __stdcall ERAHelp_ShowDialog(const main::eHelpPage page, const int subtype)
{
    return main::MainDlg::PrepareMainDlg(page, subtype);
}
