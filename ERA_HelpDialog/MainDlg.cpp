#include "SystemFunctions.h"
#include "framework.h"
LastActiveDlgModInfo ModInformation::lastActiveModInfo;

namespace main
{

MainDlg *MainDlg::instance = nullptr;

MainDlg::MainDlg(const int width, const int height, const int x, const int y) : H3Dlg(width, height, x, y, 1, 0)
{
    // disable dlg shadow
    flags ^= 16;

    MainDlg::instance = this;

    // set black background
    background = H3LoadedPcx16::Create(h3_NullString, width, height);
    memset(background->buffer, 0, background->buffSize);

    H3DlgPcx16 *backDlgPcx = H3DlgPcx16::Create(0, 0, background->width, background->height, 0, nullptr);
    backDlgPcx->SetPcx(background);

    AddItem(backDlgPcx);

    // create "ok"

    constexpr int borderMargin = 8;
    constexpr int panelsMargin = 4;

    H3DlgDefButton *bttn = H3DlgDefButton::Create(25, heightDlg - 50, int(eControlId::OK), NH3Dlg::Assets::OKAY_DEF, 0,
                                                  1, TRUE, NH3VKey::H3VK_ENTER);

    bttn->AddHotkey(eVKey::H3VK_ESCAPE);
    bttn->AddHotkey(59); // F1 click

    const int okWidth = bttn->GetWidth();
    const int okHeight = bttn->GetHeight();
    const int okX = width - okWidth - borderMargin;
    const int okY = height - okHeight - borderMargin;

    bttn->SetX(okX - 1);
    bttn->SetY(okY + 1);
    H3RGB565 color(H3RGB888::Highlight());

    CreateFrame(bttn, color, -1, 1);
    AddItem(bttn);

    // create hint
    hintBar = H3DlgHintBar::Create(this, borderMargin + 1, okY + 1,
                                   width - (panelsMargin + okWidth + 2 + borderMargin * 2), okHeight);
    // golden frame around
    CreateFrame(hintBar, color, -1, 1);
    AddItem(hintBar);

    /// create 3 GENERAL panels
    /**
    _________________________________________
    |                                       |
    |           HEADER MENU PANEL           |
    |_______________________________________|
    | CATE     |                            |
    | GORIES   |          CONTENT           |
    |          |                            |
    | PANEL    |          PANEL             |
    |          |                            |
    |__________|____________________________|
    */
    constexpr int headerX = borderMargin;

    constexpr int headerY = borderMargin;
    constexpr int headerHeight = 50;

    panels.headerMenuPanel = new HeaderMenuPanel(headerX, headerY, width - headerX * 2, headerHeight, this);

    constexpr int categoriesWidth = 220;
    const int categoriesY = headerY + headerHeight + panelsMargin;
    const int categoriesHeight = height - (headerY * 2 + headerHeight + panelsMargin * 2 + okHeight);
    panels.categoriesPanel = new CategoriesPanel(headerX, categoriesY, categoriesWidth, categoriesHeight, this);

    const int contentX = headerX + categoriesWidth + panelsMargin;
    const int contentWidth = width - categoriesWidth - headerX * 2 - panelsMargin;
    panels.contentPanel = new ContentPanel(contentX, categoriesY, contentWidth, categoriesHeight, this);

    /** create 3 Predefined CONTENT panels:
     * Creatures, Artifacts, Towns
     */

    creaturesContentPanel = new CreaturesContentPanel(contentX, categoriesY, contentWidth, categoriesHeight, this);
    artifactsContentPanel = new ArtifactsContentPanel(contentX, categoriesY, contentWidth, categoriesHeight, this);

    std::vector<std::string> modsVector;

    modList::GetEraModList(modsVector);

    if (GetLoadedModsJsonInformation(modsVector))
    {
        m_activeMod = *(mods.begin());

        panels.categoriesPanel->AssignMod(m_activeMod);
        for (auto &mod : mods)
        {
            for (auto &hk : mod->hotkeysCategory->hotkeys)
            {
                this->hotkeys.emplace_back(&hk);
            }
        }
    }
}

MainDlg::~MainDlg()
{
    // clear dlg ptr
    main::MainDlg::instance = nullptr;

    // clear panels
    for (auto &panel : panels.asArray)
    {
        delete panel;
    }

    // clear mod data (and all inside)
    for (auto &mod : mods)
    {
        delete mod;
    }
}

const Content *MainDlg::ActiveContent() const noexcept
{
    return m_activeMod->activeCategory->content;
}

void MainDlg::CallHelpInHelpDlg() const noexcept
{
    help::GuideDlg dlg(500, 500);

    dlg.Start();
}

void MainDlg::DisplayAllHotkeys() /*const*/ noexcept
{
    if (this->hotkeys.size())
    {

        for (auto &hotkey : this->hotkeys)
        {
        }
    }
}

// this function calls dlg with mods
// and return ptr to the selected one
ModInformation *MainDlg::CallModListDlg(const ModInformation *activeMod) const noexcept
{

    list::ModListDlg dlg(500, 500);
    dlg.Start();
    P_WindowManager->resultItemID;
    // if ()
    {
    }
    return dlg.ResultMod();
}

// this function creates mod list and returns "true"
// if there is at least one mode that may be displayed
BOOL MainDlg::GetLoadedModsJsonInformation(const std::vector<std::string> &modNames)
{

    // parse string
    if (!modNames.empty())
    {

        UINT modId = 0;
        for (auto &modName : modNames)
        {

            ModInformation *mod = new ModInformation(modName.c_str(), modId++);
            // if it has some info
            if (mod->hasSomeInfo)
            {
                mods.emplace_back(mod);
            }
            else
            {
                delete mod;
            }
        }
    }

    return !mods.empty();
}

void MainDlg::SetActiveMod(/*const */ ModInformation *mod)
{
    if (m_activeMod != mod)
    {
        // hide old mod if existed
        if (m_activeMod)
        {
            m_activeMod->SetVisible(false);
        }

        // draw new mod if exists
        if (m_activeMod = mod)
        {
            m_activeMod->SetVisible(true);
        }
    }
}

BOOL MainDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftClick())
    {
        bool headerClick = true;
        switch (msg.itemId)
        {
        case buttons::MODLIST:
            CallModListDlg(m_activeMod);
            break;
        case buttons::HOTKEYS:
            SetActiveMod(0);
            break;
        case buttons::CREATURES:

            break;
        case buttons::ARTIFACTS:

            break;
        case buttons::TOWNS:

            break;
            // maximize button
        case buttons::RESIZE_DLG:
            // P_WindowManager->SetClickedItemId(2020);
            //   restartDlg
            //  this->Stop();
            break;
        case buttons::HELP:
            CallHelpInHelpDlg();
            break;
        default:
            headerClick = false;

            break;
        }
        if (!headerClick)
        {
        }
    }
    else if (msg.IsRightClick())
    {
        // show rmc hint
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
        // Era::ExecErmCmd("IF:L^^");
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

const H3Town *townFromClick = nullptr;
eCreature creatureFromClick = eCreature::UNDEFINED;
enum H3DlgVTables : DWORD
{

    H3TownSmallDlg = 0x00640704,
    H3CreatureSmallDlg = 0x06406DC,
};

void MainDlg::PrepareMainDlg(HookContext *c)
{

    if (!main::MainDlg::DlgExists())
    {
        // P_SoundManager->ClickSound();

        Era::ReadStrFromIni("FullScreen", "Help", main::MainDlg::iniPath, h3_TextBuffer);

        const H3Town *town = nullptr;

        eCreature creature = eCreature::UNDEFINED;

        const DWORD currentDlgVTable = *reinterpret_cast<DWORD *>(P_WindowManager->lastDlg);

        if (currentDlgVTable)
        {

            switch (currentDlgVTable)
            {
            case H3DlgVTables::H3TownSmallDlg:
                town = townFromClick;
                break;
            case H3DlgVTables::H3CreatureSmallDlg:
                creature = creatureFromClick;
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
        else
        {
            townFromClick = nullptr;
        }
        if (creature != eCreature::UNDEFINED)
        {
            //    H3Messagebox(P_CreatureInformation[creature].namePlural);
        }
        else
        {

            creatureFromClick = eCreature::UNDEFINED;
        }
        //   Era::y[55] = **reinterpret_cast<DWORD**>(P_WindowManager->lastDlg);
        //  Era::ExecErmCmd("IF:L^%y55^;");
        const int storeResult = P_WindowManager->resultItemID;
        bool isFullScreen = atoi(h3_TextBuffer);

        do
        {
            const int dialogWidth = isFullScreen ? H3GameWidth::Get() - 6 : 800;
            const int dialogHeight = isFullScreen ? H3GameHeight::Get() - 6 : 600;

            // create help dialog
            main::MainDlg dialog(dialogWidth, dialogHeight);

            // initialize help dialog
            //	dialog.AssignWithCalledDlg();

            // start help dialog

            dialog.Start();

            const int storeResultA = P_WindowManager->ClickedItemID();
            storeResultA;
            //            town ? dialog.RMB_Show(): dialog.Start();
            if (c && town == nullptr && 0)
            {
                c->return_address = 0x04F8710;
                // return NO_EXEC_DEFAULT;
            }
            // reverse isFullScreen after dialog closed

            if (P_WindowManager->resultItemID == main::buttons::RESIZE_DLG)
            {
                Era::WriteStrToIni("FullScreen", (isFullScreen ^= 1) ? "1" : "0", "Help", main::MainDlg::iniPath);
                Era::SaveIni(main::MainDlg::iniPath);
            }
            else
            {
                break;
            }

        } while (TRUE);
        P_WindowManager->resultItemID = storeResult;
    }
}

void MainDlg::MainMenuButtonProc(void *msg)
{
    if (auto mes = static_cast<H3Msg *>(msg))
    {
        if (mes->IsLeftClick())
        {
            PrepareMainDlg();
        }
    }
}

void H3DlgFramedPanel::CreateBorderFrame()
{
    H3RGB565 frameColor = H3RGB888::Highlight();
    panelFrame = H3DlgFrame::Create(0, 0, width, height, -1, frameColor);

    // panelFrame->HideDeactivate();
    // items += panelFrame;
    // THISCALL_3(H3DlgItem *, 0x5AA7B0, this, panelFrame, -1);
    this->AddItem(panelFrame);
    panelFrame->Show();
}

H3DlgFramedPanel::~H3DlgFramedPanel()
{
    // clear items memory
    items.Deref();
}

const H3BaseDlg *H3DlgFramedPanel::Parent() const noexcept
{
    return parent;
}

void H3DlgFramedPanel::AddScrollBar(H3DlgScrollbar *scrollBar) noexcept
{
    this->scrollBar = scrollBar;
    if (scrollBar)
    {
        this->AddItem(scrollBar);
    }
}

H3LoadedPcx16 *H3DlgFramedPanel::GetBackgroundPcx() const noexcept
{
    return dynamic_cast<H3Dlg *>(parent)->GetBackgroundPcx();
}

HeaderMenuPanel::HeaderMenuPanel(const int x, const int y, const int width, const int height, H3BaseDlg *parent)
    : DlgPanel(x, y, width, height, parent)
{
    if (dlgPanel)
    {
        InitPanelItems();
    }
}

HeaderMenuPanel::~HeaderMenuPanel()
{
}

void HeaderMenuPanel::InitPanelItems()
{

    LPCSTR captionButtonDefNam = "RMGmenbt.def";

    H3DefLoader def(captionButtonDefNam);
    // return;
    volatile int counter = 0;
    constexpr eVKey keys[7] = {eVKey::H3VK_M, eVKey::H3VK_K, eVKey::H3VK_C, eVKey::H3VK_A,
                               eVKey::H3VK_T, eVKey::H3VK_R, eVKey::H3VK_H};
    for (auto &bttn : buttons.asArray)
    {
        bttn = H3DlgCaptionButton::Create(
            counter * def->widthDEF * 2 / 3 + 1, 3, buttons::FIRST + counter, captionButtonDefNam,
            EraJS::read(H3String::Format("help.dlg.main.header.%d.name", counter).String()), NH3Dlg::Text::MEDIUM, 0, 0,
            false, keys[counter], 0);

        bttn->SetClickFrame(1);
        this->dlgPanel->AddItem(bttn);

        bttn->SetHints(EraJS::read(H3String::Format("help.dlg.main.header.%d.hint", counter).String()),
                       EraJS::read(H3String::Format("help.dlg.main.header.%d.rmc", counter).String()), false);
        bttn->ShowActivate();
        counter++;
    }
    // set "close dlg" for resize bbtn: nended to use P_
    ByteAt(reinterpret_cast<ADDRESS>(buttons.resize) + 0x44) = true;

    //	return;

    for (size_t i = 1; i < 5; i++)
    {
        auto bttn = buttons.asArray[i];
        const int bX = i * 56 + 256;
        if (bttn) //> 0)
        {
            //		bttn->SetX(bX);
        }
        // auto frame = H3DlgFrame::Create(buttons.asArray[i], 1, frameColor);
        // this->AddItem(frame);
    }
    dlgPanel->CreateBorderFrame();
}

void ContentPanel::InitPanelItems()
{

    const int width = dlgPanel->GetWidth();
    const int height = dlgPanel->GetHeight();

    backPcx = H3DlgPcx16::Create(1, 1, width - 2, height - 2, -1, nullptr);
    auto pcx = H3LoadedPcx16::Create(width - 2, height - 2);

    memset(pcx->buffer, 14, pcx->buffSize);
    backPcx->SetPcx(pcx);

    dlgPanel->CreateBorderFrame();
}
// CreaturesContentPanel::CreaturesContentPanel(const ContentPanel& other)
//	:ContentPanel(other.dlgPanel->GetX(), other.dlgPanel->GetY(),
// other.dlgPanel->GetWidth(), other.dlgPanel->GetY(), other.dlgPanel->Parent())

//{
//	InitPanelItems();

//}

void DlgPanel::CreateItemsGrid(LPCSTR const defName, const int maxItems, H3DlgScrollbar_proc scrollBarProc)
{
    constexpr int X_INTERVAL = 7;
    constexpr int Y_INTERVAL = 6;
    constexpr int xScrollBarPadding = 18;
    constexpr int minPaddingX = 2;
    constexpr int maxPaddingX = 6;
    constexpr int minPaddingY = 2;
    constexpr int maxPaddingY = 6;
    const int width = dlgPanel->GetWidth() - xScrollBarPadding;
    const int height = dlgPanel->GetHeight();

    H3DefLoader def(defName);
    const int defWidth = def->widthDEF;
    const int defHeight = def->heightDEF;

    const int availableWidth = width - defWidth;
    const int availableHeight = height - defHeight;

    const int columns = (availableWidth + defWidth - 1) / (defWidth + minPaddingX);
    const int rows = (availableHeight + defHeight - 1) / (defHeight + minPaddingY);

    const int paddingX =
        std::max(minPaddingX, std::min(maxPaddingX, (availableWidth - (columns - 1) * defWidth) / columns));
    const int paddingY =
        std::max(minPaddingY, std::min(maxPaddingY, (availableHeight - (rows - 1) * defHeight) / rows));

    int x = paddingX;
    int y = paddingY;
    int counter = 0;

    for (int i = 0; i < rows && counter < maxItems; ++i)
    {
        for (int j = 0; j < columns && counter < maxItems; ++j)
        {
            H3DlgDef *monDef = H3DlgDef::Create(x, y, defName, 0);
            this->dlgPanel->AddItem(monDef);
            x += defWidth + paddingX;
            ++counter;
        }
        x = paddingX;
        y += defHeight + paddingY;
    }

    if (scrollBarProc && counter >= maxItems && columns)
    {
        const int extraItems = maxItems + 1 - counter;
        const int scrollBarTicksCount = extraItems / columns + 1;

        auto scrollBar = H3DlgScrollbar::Create(width - xScrollBarPadding, 2, 16, availableHeight, 123,
                                                scrollBarTicksCount, scrollBarProc, false, 1, true);
        this->dlgPanel->AddItem(scrollBar);
    }
}

void CreaturesContentPanel::InitPanelItems()
{
    const int maxMonId = IntAt(0x4A1657) - 1;

    CreateItemsGrid(NH3Dlg::Assets::CREATURE_SMALL, maxMonId, ScrollProc);
}

void __fastcall CreaturesContentPanel::ScrollProc(INT32 tick, H3BaseDlg *dlg)
{
}
void __fastcall ArtifactsContentPanel::ScrollProc(INT32 tick, H3BaseDlg *dlg)
{
}
ContentPanel::ContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
    : DlgPanel(x, y, width, height, parent)
{
    if (dlgPanel)
    {
        InitPanelItems();
    }
}

CreaturesContentPanel::CreaturesContentPanel(const int x, const int y, const int width, const int height,
                                             const H3BaseDlg *parent)
    : DlgPanel(x, y, width, height, parent)
{
    InitPanelItems();
}
ArtifactsContentPanel::ArtifactsContentPanel(const int x, const int y, const int width, const int height,
                                             const H3BaseDlg *parent)
    : DlgPanel(x, y, width, height, parent)
{
    InitPanelItems();
}
void ArtifactsContentPanel::InitPanelItems()
{
    const int maxArtId = P_ArtifactCount;

    CreateItemsGrid(NH3Dlg::Assets::ARTIFACT_DEF, maxArtId, ScrollProc);
}
ContentPanel::~ContentPanel()
{
    if (auto pcx = backPcx->GetPcx())
    {
        pcx->Destroy();
        backPcx->SetPcx(nullptr);
    }
}
CategoriesPanel::CategoriesPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
    : DlgPanel(x, y, width, height, parent), activeMod(nullptr)
{
    if (dlgPanel)
    {
        InitPanelItems();
    }
}
CategoriesPanel::~CategoriesPanel()
{
}

void CategoriesPanel::InitPanelItems()
{

    const int panelWidth = dlgPanel->GetWidth();
    const int panelHeight = dlgPanel->GetHeight();

    constexpr int CATEGORY_HEIGHT = 48;
    constexpr int CATEGORY_INTERVAL = 6;
    constexpr int padding = 1;
    const size_t catsNum = (panelHeight - CATEGORY_INTERVAL) / (CATEGORY_HEIGHT + CATEGORY_INTERVAL);
    // return;

    H3DefLoader townDef(NH3Dlg::Assets::TOWN_SMALL);
    if (catsNum > 0)
    {
        constexpr int x = 1 + padding; // dlgPanel->GetX();
        constexpr int scrollPadding = 16 + padding;

        for (size_t i = 0; i < catsNum; i++)
        {
            const int icoY = i * (CATEGORY_HEIGHT + CATEGORY_INTERVAL) + CATEGORY_INTERVAL;
            constexpr int icoWidth = 48;
            constexpr int icoHeight = 48;

            constexpr int textXOffset = 3;
            //	H3LoadedPcx16* icon = H3LoadedPcx16::Create(icoWidth,
            // icoHeight);
            PanelCategory panelCat;

            H3DlgPcx16 *iconItem = H3DlgPcx16::Create(x, icoY, icoWidth, icoHeight, -1, 0);

            //	iconItem->Show();

            H3DlgTextPcx *textPcx = H3DlgTextPcx::Create(
                x + icoWidth + textXOffset, icoY, panelWidth - icoWidth - textXOffset - scrollPadding - padding * 2 - 2,
                CATEGORY_HEIGHT, h3_NullString, NH3Dlg::Text::MEDIUM, "gem_res.pcx", eTextColor::WHITE,
                eTextAlignment::MIDDLE_CENTER);

            this->dlgPanel->AddItem(iconItem);
            this->dlgPanel->AddItem(textPcx);

            panelCat.category = nullptr;
            panelCat.textPcx = textPcx;
            panelCat.icon = iconItem;
            panelCategories.emplace_back(panelCat);
        }
        H3DlgScrollbar *scrollBar = H3DlgScrollbar::Create(
            panelWidth - scrollPadding, padding, 16, panelHeight - padding * 2, 123, 123, nullptr, false, 1, true);

        this->dlgPanel->AddScrollBar(scrollBar);
        scrollBar->ShowActivate();
    }

    // create border after alll items added
    dlgPanel->CreateBorderFrame();
}

void CategoriesPanel::AssignMod(ModInformation *mod)
{
    if (activeMod != mod)
    {
        // draw new mod panelCategories
        activeMod = mod;
        RedrawCategoryItems();
    }
    // st
}

void CategoriesPanel::RedrawCategoryItems(const int firstItemId)
{

    if (activeMod)
    {
        // combare max items to dra
        const size_t modSize = activeMod->Size();
        const size_t panelSize = panelCategories.size();

        for (size_t i = 0; i < panelSize; i++)
        {
            const size_t dataIndex = i + firstItemId;
            const bool inRange = dataIndex < modSize;

            // set new data if object is in range
            if (inRange)
            {
                if (const Category *categoryToSet = activeMod->categories[dataIndex])
                {
                    panelCategories[i].SetCategory(categoryToSet);

                    //	customButton->Refresh();
                    //	customButton->ParentRedraw();
                }
            }
            else
            {
                panelCategories[i].textPcx->HideDeactivate();
            }
        }

        activeMod->ActiveCategory().ShowContent();
    }
}


// DlgPanel::DlgPanel(const DlgPanel &other)
//     : DlgPanel(other.dlgPanel->GetX(), other.dlgPanel->GetY(), other.dlgPanel->GetWidth(), other.dlgPanel->GetY(),
//              other.dlgPanel->Parent())
//{
//
// }
//  create base class with h3 extended panel
DlgPanel::DlgPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
{

    // if item is allocated
    if (dlgPanel = H3ObjectAllocator<H3DlgFramedPanel>().allocate())
    {
        THISCALL_6(H3DlgFramedPanel *, 0x5AA6D0, dlgPanel, x, y, width, height, parent);
    }
}

DlgPanel::~DlgPanel()
{
    H3ObjectAllocator<H3DlgFramedPanel>().deallocate(dlgPanel);
}

int DlgPanel::GetX() const noexcept
{
    return dlgPanel->GetX();
}

int DlgPanel::GetY() const noexcept
{
    return dlgPanel->GetY();
}

INT32 __fastcall CategoriesPanel::PanelCategory::Proc(H3Msg *msg)
{
    return 0;
}

void CategoriesPanel::PanelCategory::SetCategory(const Category *category)
{

    // if src has pcx and current doesn't
    if (category->iconPcx && !icon->GetPcx())
    {
        // resize and move text item

        textPcx->SetX(textPcx->GetX() + textIcoXOffset);
        textPcx->SetWidth(textPcx->GetWidth() - textIcoXOffset);

        // show icon
        icon->ShowActivate();
    }
    // else if current has and source doesn't
    else if (!category->iconPcx && icon->GetPcx())
    {
        // resize and move text item
        textPcx->SetX(textPcx->GetX() - textIcoXOffset);
        textPcx->SetWidth(textPcx->GetWidth() + textIcoXOffset);

        // hide icon
        icon->HideDeactivate();
    }

    // set new pcx
    icon->SetPcx(category->iconPcx);
    // assign category
    this->category = category; //->UnfocusEdits(false);
    // set text
    textPcx->SetText(category->name);
}

} // namespace main
