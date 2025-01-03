#include "framework.h"
#include "SystemFunctions.h"
#include <regex>
#include <sstream> // for std::istringstream


namespace main
{

HelpDlg *HelpDlg::instance = nullptr;
LastActiveDlgModInfo Mod::lastActiveModInfo;



HelpDlg::HelpDlg(const int width, const int height, const int x, const int y) : H3Dlg(width, height, x, y, 1, 0)
{
    // disable dlg shadow
    flags ^= 16;

    HelpDlg::instance = this;
    
    needResize = false;

    // set black background
    background = H3LoadedPcx16::Create(h3_NullString, width, height);
    memset(background->buffer, 0, background->buffSize);

    H3DlgPcx16 *bg = H3DlgPcx16::Create(0, 0, background->width, background->height, 0, nullptr);
    bg->SetPcx(background);

    AddItem(bg);

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
    hintBar = H3DlgHintBar::Create(this,borderMargin + 1, okY + 1, width - (panelsMargin + okWidth + 2 + borderMargin * 2), okHeight);
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

    std::string nonParsedModNamesString = external::GetLoadedGameMods();


    if (GetLoadedModsJsonInformation(nonParsedModNamesString))
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

HelpDlg::~HelpDlg()
{
    // clear dlg ptr
    main::HelpDlg::instance = nullptr;

    // clear panels
    for (auto &panel : panels.asArray)
    {
        delete panel;
    }

    // clear mod data (and all inside)
    for (auto &mod : mods)
    {
        mod->~Mod();
    }
}

const Content *HelpDlg::ActiveContent() const noexcept
{
    return m_activeMod->activeCategory->content;
}

void HelpDlg::CallHelpInHelpDlg() const noexcept
{
    help::HelpInHelpDlg dlg(500, 500);
    
    dlg.Start();
}

void HelpDlg::DisplayAllHotkeys() /*const*/ noexcept
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
Mod *HelpDlg::CallModListDlg(const Mod *activeMod) const noexcept
{
    list::ModListDlg dlg(500, 500);
    dlg.Start();
    P_WindowManager->resultItemID;
    // if ()
    {
    }
    return dlg.ResultMod();
}

//	return "";  // Возвращаем пустую строку, если ничего не найдено
//}

std::string ExtractModNameFromPath(const std::string &input)
{
    // Ищем начало пути
    size_t startPos = input.find("$ <= $");
    if (startPos == std::string::npos)
        return "";
    startPos += 12; // Пропускаем "$ <= $"

    // Ищем конец пути до квадратной скобки
    size_t endPos = input.find("[", startPos);
    if (endPos == std::string::npos)
        return "";

    // Извлекаем путь и убираем пробелы по краям
    std::string path = input.substr(startPos, endPos - startPos);
    size_t first = path.find_first_not_of(' ');
    size_t last = path.find_last_not_of(' ');

    return path.substr(first, last - first + 1);
}



// this function creates mod list and returns "true"
// if there is at least one mode that may be displayed
BOOL HelpDlg::GetLoadedModsJsonInformation(const std::string& nonParsedModNamesString)
{
    BOOL result = false;
    std::vector<std::string> modNames;

    // parse string
    if (!nonParsedModNamesString.empty())
    {



        std::istringstream stream(nonParsedModNamesString);
        std::string line;
        // Читаем строку по строке, разделяя по символу '\n'
        UINT modId = 0;
        while (std::getline(stream, line))
        {
            std::string parsedLine = ExtractModNameFromPath(line);
            // H3Messagebox(parsedLine.c_str());
            if (!parsedLine.empty())
            {
                modNames.emplace_back(parsedLine);

                // create mod
                Mod* mod = new Mod(parsedLine.c_str(), modId++);
                if (mod->hasSomeInfo)
                {
                    result = true;
                }
                // if it has some info
                if (mod->hasSomeInfo)
                {
                    mods.emplace_back(mod);
                }
                else
                {
                    mod->~Mod();
                }
            }
        }
    }

    return result;
}

void HelpDlg::SetActiveMod(/*const */ Mod *mod)
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

BOOL HelpDlg::DialogProc(H3Msg &msg)
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
        case buttons::MAXIMIZE:
            needResize = true;
            // restartDlg
            this->Stop();
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


void HelpDlg::AssignWithCalledDlg(const H3Town* town, const eCreature creature) noexcept
{
    

    if (town)
    {


    }
    else if (creature != eCreature::UNDEFINED)
    {

    }


}

BOOL HelpDlg::DlgExists()
{
    return instance != nullptr;
}
BOOL HelpDlg::NeedResizeScreen() const noexcept
{
    return needResize;
}

void H3DlgFramedPanel::CreateBorderFrame()
{
    H3RGB565 frameColor = H3RGB888::Highlight();
    panelFrame = H3DlgFrame::Create(0, 0, width, height, -1, frameColor);

    //panelFrame->HideDeactivate();
    //items += panelFrame;
    //THISCALL_3(H3DlgItem *, 0x5AA7B0, this, panelFrame, -1);
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

void H3DlgFramedPanel::AddScrtollBar(H3DlgScrollbar *scrollBar) noexcept
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
    : IPanel(x, y, width, height, parent)
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

    LPCSTR defName = "RMGmenbt.def";

    H3DefLoader def(defName);
    // return;
    volatile int counter = 0;
    for (auto &bttn : buttons.asArray)
    {
        bttn =
            H3DlgCaptionButton::Create(counter * def->widthDEF * 2 / 3 + 1, 3, buttons::FIRST + counter, defName,
                                       EraJS::read(H3String::Format("help.text.main.header.%d.name", counter).String()),
                                       NH3Dlg::Text::MEDIUM, 0, 0, false, 0, 0);

        bttn->SetClickFrame(1);
        this->dlgPanel->AddItem(bttn);

        bttn->SetHints(EraJS::read(H3String::Format("help.text.main.header.%d.hint", counter).String()),
                       EraJS::read(H3String::Format("help.text.main.header.%d.rmc", counter).String()), false);
        bttn->ShowActivate();
        counter++;

        //	const int bX = counter++ * 56 + 256;
        //	bttn->SetX(bX);
    }



    buttons.artifacts->AddHotkey(eVKey::H3VK_A);
    buttons.towns->AddHotkey(eVKey::H3VK_T);
    buttons.creatures->AddHotkey(eVKey::H3VK_C);
    buttons.resize->AddHotkey(eVKey::H3VK_M);
    buttons.help->AddHotkey(eVKey::H3VK_H);
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

Mod::Mod(LPCSTR modFolderName, const UINT id) : name(modFolderName), hasSomeInfo(false), id(id), activeCategory(nullptr)
{

    json = jsonBase;
    json.Append(modFolderName);

    categories.clear();
    // start parsing panelCategories
    // first parse hotkeys

    if (hotkeysCategory = CreateHotkeysCategory())
    {
        categories.emplace_back(hotkeysCategory);
    }

    // create native catigories while json is parsed
    int startIndex = 0;
    while (auto category = CreateNativeCategory(startIndex++))
    {
        categories.emplace_back(category);
    }

    if (!categories.empty())
    {
        hasSomeInfo = true;
    }
}

Mod::~Mod()
{
    for (auto &cat : categories)
    {
        if (cat)
        {
            cat->~Category();
        }
        // delete cat->content;
        //
    }
}

HotKeysCategory *Mod::CreateHotkeysCategory() const noexcept
{

    HotKeysCategory *result = nullptr;
    // H3String jsonKey = H3String::Format("%s.panelCategories.hotkeys",
    // json.String());

    bool readSucces = false;
    std::vector<HotKey> hotkeys;
    LPCSTR hkName = EraJS::read(H3String::Format("%s.categories.hotkeys.name", json.String()).String(), readSucces);
    if (readSucces)
    {
        int hotkeyId = 0;
        while (true)
        {
            H3String key = EraJS::read(
                H3String::Format("%s.categories.hotkeys.content.%d.key", json.String(), hotkeyId).String(), readSucces);
            if (!readSucces || key.Empty())
            {
                break;
            }

            const int type = EraJS::readInt(
                H3String::Format("%s.categories.hotkeys.content.%d.type", json.String(), hotkeyId).String());
            H3String description = EraJS::read(
                H3String::Format("%s.categories.hotkeys.content.%d.description", json.String(), hotkeyId).String());


            hotkeys.emplace_back(HotKey{ hkcategories::eType(type), key, description});
            hotkeyId++;
        }
    }
    // if added at least one hotkey
    if (hotkeys.size())
    {

        result = new HotKeysCategory();
        result->hotkeys = hotkeys;
        result->name = hkName;
        result->content = new Content();
    }

    return result;
}

Category *Mod::CreateNativeCategory(const int index) const noexcept
{
    Category *result = nullptr;
    bool readSucces = false;

    LPCSTR catName = EraJS::read(H3String::Format("%s.categories.%d.name", json.String(), index).String(), readSucces);
    if (readSucces)
    {

        if (result = new Category())
        {
            result->name = catName;

            result->content = new Content();
            result->content->text =
                EraJS::read(H3String::Format("%s.categories.%d.content", json.String(), index).String());

            // H3String defName =
        }
    }

    return result;
}

const Category &Mod::ActiveCategory() const noexcept
{
    return *activeCategory;
}

size_t Mod::Size() const noexcept
{
    return categories.size();
}

void Mod::SetVisible(const BOOL state)
{
}

void Mod::StoreModInfoAsActive() const noexcept
{
    //lastActiveModInfo.scrollBarPos  = this->activeCategory
    lastActiveModInfo.categoryId = m_lastActiveCategoryId;
    
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

void IPanel::CreateItemsGrid(LPCSTR const defName, const int maxItems, H3DlgScrollbar_proc scrollBarProc)
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

    const int paddingX = std::max(minPaddingX, std::min(maxPaddingX, (availableWidth - (columns - 1) * defWidth) / columns));
    const int paddingY = std::max(minPaddingY, std::min(maxPaddingY, (availableHeight - (rows - 1) * defHeight) / rows));

    int x = paddingX;
    int y = paddingY;
    int counter = 0;

    for (int i = 0; i < rows && counter < maxItems; ++i)
    {
        for (int j = 0; j < columns && counter < maxItems; ++j)
        {
            H3DlgDef* monDef = H3DlgDef::Create(x, y, defName, 0);
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

        scrollBar = H3DlgScrollbar::Create(width - xScrollBarPadding, 2, 16, availableHeight, 123, scrollBarTicksCount, scrollBarProc, false, 1, true);
        this->dlgPanel->AddItem(scrollBar);

    }
}


void CreaturesContentPanel::InitPanelItems()
{

    const int maxMonId = IntAt(0x4A1657) - 1;

    CreateItemsGrid(NH3Dlg::Assets::CREATURE_SMALL, maxMonId, ScrollProc);
}

void __fastcall CreaturesContentPanel::ScrollProc(INT32 tick, H3BaseDlg* dlg)
{


}
void __fastcall ArtifactsContentPanel::ScrollProc(INT32 tick, H3BaseDlg* dlg)
{


}
ContentPanel::ContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
    : IPanel(x, y, width, height, parent)
{
    if (dlgPanel)
    {
        InitPanelItems();
    }
}

CreaturesContentPanel::CreaturesContentPanel(const int x, const int y, const int width, const int height,
                                             const H3BaseDlg *parent)
    : IPanel(x, y, width, height, parent)
{
    InitPanelItems();
}
ArtifactsContentPanel::ArtifactsContentPanel(const int x, const int y, const int width, const int height,
                                             const H3BaseDlg *parent)
    : IPanel(x, y, width, height, parent)
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
    : IPanel(x, y, width, height, parent), activeMod(nullptr)
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

    const int CATEGORY_WIDTH = dlgPanel->GetWidth();
    constexpr int CATEGORY_HEIGHT = 48;
    constexpr int CATEGORY_INTERVAL = 6;
    const int height = dlgPanel->GetHeight();
    constexpr int padding = 1;
    const int catsNum = (height - CATEGORY_INTERVAL) / (CATEGORY_HEIGHT + CATEGORY_INTERVAL);
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
                x + icoWidth + textXOffset, icoY,
                CATEGORY_WIDTH - icoWidth - textXOffset - scrollPadding - padding * 2 - 2, CATEGORY_HEIGHT,
                h3_NullString, NH3Dlg::Text::MEDIUM, "gem_res.pcx", eTextColor::WHITE, eTextAlignment::MIDDLE_CENTER);

            this->dlgPanel->AddItem(iconItem);
            this->dlgPanel->AddItem(textPcx);

            panelCat.category = nullptr;
            panelCat.textPcx = textPcx;
            panelCat.icon = iconItem;
            panelCategories.emplace_back(panelCat);
        }
        H3DlgScrollbar *scrollBar =
            H3DlgScrollbar::Create(this->dlgPanel->GetWidth() - scrollPadding, padding, 16,
                                   this->dlgPanel->GetHeight() - padding * 2, 123, 123, nullptr, false, 1, true);

        this->dlgPanel->AddScrtollBar(scrollBar);
        scrollBar->ShowActivate();
    }

    // create border after alll items added
    dlgPanel->CreateBorderFrame();
}

void CategoriesPanel::AssignMod(Mod *mod)
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

Category::~Category()
{
    if (content)
    {
        delete content;
        content = nullptr;
    }
}
void Category::ShowContent() const noexcept
{
}
//IPanel::IPanel(const IPanel &other)
//    : IPanel(other.dlgPanel->GetX(), other.dlgPanel->GetY(), other.dlgPanel->GetWidth(), other.dlgPanel->GetY(),
//             other.dlgPanel->Parent())
//{
//
//}
// create base class with h3 extended panel
IPanel::IPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
{

    // if item is allocated
    if (dlgPanel = H3ObjectAllocator<H3DlgFramedPanel>().allocate())
    {
        THISCALL_6(H3DlgFramedPanel *, 0x5AA6D0, dlgPanel, x, y, width, height, parent);
    }
}

IPanel::~IPanel()
{
    delete dlgPanel;
}

int IPanel::GetX() const noexcept
{
    return dlgPanel->GetX();
}

int IPanel::GetY() const noexcept
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
    // else if current has and sorc doesn't
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
