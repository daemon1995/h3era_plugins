//#include "DlgEnums.h"
//#include "framework.h"
//
//// DlgPanel::DlgPanel(const DlgPanel &other)
////     : DlgPanel(other.dlgPanel->GetX(), other.dlgPanel->GetY(), other.dlgPanel->GetWidth(), other.dlgPanel->GetY(),
////              other.dlgPanel->Parent())
////{
////
//// }
////  create base class with h3 extended panel
//DlgPanel::DlgPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
//{
//
//    // if item is allocated
//    if (dlgPanel = H3ObjectAllocator<H3DlgFramedPanel>().allocate())
//    {
//        THISCALL_6(H3DlgFramedPanel *, 0x5AA6D0, dlgPanel, x, y, width, height, parent);
//    }
//}
//void ContentPanel::InitPanelItems()
//{
//
//    const int width = dlgPanel->GetWidth();
//    const int height = dlgPanel->GetHeight();
//
//    backPcx = H3DlgPcx16::Create(1, 1, width - 2, height - 2, -1, nullptr);
//    auto pcx = H3LoadedPcx16::Create(width - 2, height - 2);
//
//    memset(pcx->buffer, 14, pcx->buffSize);
//    backPcx->SetPcx(pcx);
//
//    dlgPanel->CreateBorderFrame();
//}
//// CreaturesContentPanel::CreaturesContentPanel(const ContentPanel& other)
////	:ContentPanel(other.dlgPanel->GetX(), other.dlgPanel->GetY(),
//// other.dlgPanel->GetWidth(), other.dlgPanel->GetY(), other.dlgPanel->Parent())
//
////{
////	InitPanelItems();
//
////}
//
//void DlgPanel::CreateItemsGrid(LPCSTR const defName, const int maxItems, H3DlgScrollbar_proc scrollBarProc)
//{
//    constexpr int X_INTERVAL = 7;
//    constexpr int Y_INTERVAL = 6;
//    constexpr int xScrollBarPadding = 18;
//    constexpr int minPaddingX = 2;
//    constexpr int maxPaddingX = 6;
//    constexpr int minPaddingY = 2;
//    constexpr int maxPaddingY = 6;
//    const int width = dlgPanel->GetWidth() - xScrollBarPadding;
//    const int height = dlgPanel->GetHeight();
//
//    H3DefLoader def(defName);
//    const int defWidth = def->widthDEF;
//    const int defHeight = def->heightDEF;
//
//    const int availableWidth = width - defWidth;
//    const int availableHeight = height - defHeight;
//
//    const int columns = (availableWidth + defWidth - 1) / (defWidth + minPaddingX);
//    const int rows = (availableHeight + defHeight - 1) / (defHeight + minPaddingY);
//
//    const int paddingX =
//        std::max(minPaddingX, std::min(maxPaddingX, (availableWidth - (columns - 1) * defWidth) / columns));
//    const int paddingY =
//        std::max(minPaddingY, std::min(maxPaddingY, (availableHeight - (rows - 1) * defHeight) / rows));
//
//    int x = paddingX;
//    int y = paddingY;
//    int counter = 0;
//
//    for (int i = 0; i < rows && counter < maxItems; ++i)
//    {
//        for (int j = 0; j < columns && counter < maxItems; ++j)
//        {
//            H3DlgDef *monDef = H3DlgDef::Create(x, y, defName, 0);
//            this->dlgPanel->AddItem(monDef);
//            x += defWidth + paddingX;
//            ++counter;
//        }
//        x = paddingX;
//        y += defHeight + paddingY;
//    }
//
//    if (scrollBarProc && counter >= maxItems && columns)
//    {
//        const int extraItems = maxItems + 1 - counter;
//        const int scrollBarTicksCount = extraItems / columns + 1;
//
//        scrollBar = H3DlgScrollbar::Create(width - xScrollBarPadding, 2, 16, availableHeight, 123, scrollBarTicksCount,
//                                           scrollBarProc, false, 1, true);
//        this->dlgPanel->AddItem(scrollBar);
//    }
//}
//
//void CreaturesContentPanel::InitPanelItems()
//{
//
//    const int maxMonId = IntAt(0x4A1657) - 1;
//
//    CreateItemsGrid(NH3Dlg::Assets::CREATURE_SMALL, maxMonId, ScrollProc);
//}
//
//void __fastcall CreaturesContentPanel::ScrollProc(INT32 tick, H3BaseDlg *dlg)
//{
//}
//void __fastcall ArtifactsContentPanel::ScrollProc(INT32 tick, H3BaseDlg *dlg)
//{
//}
//ContentPanel::ContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
//    : DlgPanel(x, y, width, height, parent)
//{
//    if (dlgPanel)
//    {
//        InitPanelItems();
//    }
//}
//
//CreaturesContentPanel::CreaturesContentPanel(const int x, const int y, const int width, const int height,
//                                             const H3BaseDlg *parent)
//    : DlgPanel(x, y, width, height, parent)
//{
//    InitPanelItems();
//}
//ArtifactsContentPanel::ArtifactsContentPanel(const int x, const int y, const int width, const int height,
//                                             const H3BaseDlg *parent)
//    : DlgPanel(x, y, width, height, parent)
//{
//    InitPanelItems();
//}
//void ArtifactsContentPanel::InitPanelItems()
//{
//    const int maxArtId = P_ArtifactCount;
//
//    CreateItemsGrid(NH3Dlg::Assets::ARTIFACT_DEF, maxArtId, ScrollProc);
//}
//ContentPanel::~ContentPanel()
//{
//    if (auto pcx = backPcx->GetPcx())
//    {
//        pcx->Destroy();
//        backPcx->SetPcx(nullptr);
//    }
//}
//CategoriesPanel::CategoriesPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent)
//    : DlgPanel(x, y, width, height, parent), activeMod(nullptr)
//{
//    if (dlgPanel)
//    {
//        InitPanelItems();
//    }
//}
//CategoriesPanel::~CategoriesPanel()
//{
//}
//
//void CategoriesPanel::InitPanelItems()
//{
//
//    const int CATEGORY_WIDTH = dlgPanel->GetWidth();
//    constexpr int CATEGORY_HEIGHT = 48;
//    constexpr int CATEGORY_INTERVAL = 6;
//    const int height = dlgPanel->GetHeight();
//    constexpr int padding = 1;
//    const size_t catsNum = (height - CATEGORY_INTERVAL) / (CATEGORY_HEIGHT + CATEGORY_INTERVAL);
//    // return;
//
//    H3DefLoader townDef(NH3Dlg::Assets::TOWN_SMALL);
//    if (catsNum > 0)
//    {
//        constexpr int x = 1 + padding; // dlgPanel->GetX();
//        constexpr int scrollPadding = 16 + padding;
//
//        for (size_t i = 0; i < catsNum; i++)
//        {
//            const int icoY = i * (CATEGORY_HEIGHT + CATEGORY_INTERVAL) + CATEGORY_INTERVAL;
//            constexpr int icoWidth = 48;
//            constexpr int icoHeight = 48;
//
//            constexpr int textXOffset = 3;
//            //	H3LoadedPcx16* icon = H3LoadedPcx16::Create(icoWidth,
//            // icoHeight);
//            PanelCategory panelCat;
//
//            H3DlgPcx16 *iconItem = H3DlgPcx16::Create(x, icoY, icoWidth, icoHeight, -1, 0);
//
//            //	iconItem->Show();
//
//            H3DlgTextPcx *textPcx = H3DlgTextPcx::Create(
//                x + icoWidth + textXOffset, icoY,
//                CATEGORY_WIDTH - icoWidth - textXOffset - scrollPadding - padding * 2 - 2, CATEGORY_HEIGHT,
//                h3_NullString, NH3Dlg::Text::MEDIUM, "gem_res.pcx", eTextColor::WHITE, eTextAlignment::MIDDLE_CENTER);
//
//            this->dlgPanel->AddItem(iconItem);
//            this->dlgPanel->AddItem(textPcx);
//
//            panelCat.category = nullptr;
//            panelCat.textPcx = textPcx;
//            panelCat.icon = iconItem;
//            panelCategories.emplace_back(panelCat);
//        }
//        H3DlgScrollbar *scrollBar =
//            H3DlgScrollbar::Create(this->dlgPanel->GetWidth() - scrollPadding, padding, 16,
//                                   this->dlgPanel->GetHeight() - padding * 2, 123, 123, nullptr, false, 1, true);
//
//        this->dlgPanel->AddScrollBar(scrollBar);
//        scrollBar->ShowActivate();
//    }
//
//    // create border after alll items added
//    dlgPanel->CreateBorderFrame();
//}
//
//void CategoriesPanel::AssignMod(main::ModInformation *mod)
//{
//    if (activeMod != mod)
//    {
//        // draw new mod panelCategories
//        activeMod = mod;
//        RedrawCategoryItems();
//    }
//    // st
//}
//
//void CategoriesPanel::RedrawCategoryItems(const int firstItemId)
//{
//
//    if (activeMod)
//    {
//        // combare max items to dra
//        const size_t modSize = activeMod->Size();
//        const size_t panelSize = panelCategories.size();
//
//        for (size_t i = 0; i < panelSize; i++)
//        {
//            const size_t dataIndex = i + firstItemId;
//            const bool inRange = dataIndex < modSize;
//
//            // set new data if object is in range
//            if (inRange)
//            {
//                if (const main::Category *categoryToSet = activeMod->categories[dataIndex])
//                {
//                    panelCategories[i].SetCategory(categoryToSet);
//
//                    //	customButton->Refresh();
//                    //	customButton->ParentRedraw();
//                }
//            }
//            else
//            {
//                panelCategories[i].textPcx->HideDeactivate();
//            }
//        }
//
//        activeMod->ActiveCategory().ShowContent();
//    }
//}
//
//void H3DlgFramedPanel::CreateBorderFrame()
//{
//    H3RGB565 frameColor = H3RGB888::Highlight();
//    panelFrame = H3DlgFrame::Create(0, 0, width, height, -1, frameColor);
//
//    // panelFrame->HideDeactivate();
//    // items += panelFrame;
//    // THISCALL_3(H3DlgItem *, 0x5AA7B0, this, panelFrame, -1);
//    this->AddItem(panelFrame);
//    panelFrame->Show();
//}
//
//H3DlgFramedPanel::~H3DlgFramedPanel()
//{
//    // clear items memory
//    items.Deref();
//}
//
//const H3BaseDlg *H3DlgFramedPanel::Parent() const noexcept
//{
//    return parent;
//}
//
//void H3DlgFramedPanel::AddScrollBar(H3DlgScrollbar *scrollBar) noexcept
//{
//    this->scrollBar = scrollBar;
//    if (scrollBar)
//    {
//        this->AddItem(scrollBar);
//    }
//}
//
//H3LoadedPcx16 *H3DlgFramedPanel::GetBackgroundPcx() const noexcept
//{
//    return dynamic_cast<H3Dlg *>(parent)->GetBackgroundPcx();
//}
//
//HeaderMenuPanel::HeaderMenuPanel(const int x, const int y, const int width, const int height, H3BaseDlg *parent)
//    : DlgPanel(x, y, width, height, parent)
//{
//    if (dlgPanel)
//    {
//        InitPanelItems();
//    }
//}
//
//HeaderMenuPanel::~HeaderMenuPanel()
//{
//}
//
//void HeaderMenuPanel::InitPanelItems()
//{
//
//    LPCSTR captionButtonDefNam = "RMGmenbt.def";
//
//    H3DefLoader def(captionButtonDefNam);
//    // return;
//    volatile int counter = 0;
//    constexpr eVKey keys[7] = {eVKey::H3VK_M, eVKey::H3VK_K, eVKey::H3VK_C, eVKey::H3VK_A,
//                               eVKey::H3VK_T, eVKey::H3VK_R, eVKey::H3VK_H};
//    for (auto &bttn : buttons.asArray)
//    {
//        bttn = H3DlgCaptionButton::Create(
//            counter * def->widthDEF * 2 / 3 + 1, 3, main::buttons::FIRST + counter, captionButtonDefNam,
//            EraJS::read(H3String::Format("help.dlg.main.header.%d.name", counter).String()), NH3Dlg::Text::MEDIUM, 0, 0,
//            false, keys[counter], 0);
//
//        bttn->SetClickFrame(1);
//        this->dlgPanel->AddItem(bttn);
//
//        bttn->SetHints(EraJS::read(H3String::Format("help.dlg.main.header.%d.hint", counter).String()),
//                       EraJS::read(H3String::Format("help.dlg.main.header.%d.rmc", counter).String()), false);
//        bttn->ShowActivate();
//        counter++;
//    }
//    // set "close dlg" for resize bbtn: nended to use P_
//    ByteAt(reinterpret_cast<ADDRESS>(buttons.resize) + 0x44) = true;
//
//    //	return;
//
//    for (size_t i = 1; i < 5; i++)
//    {
//        auto bttn = buttons.asArray[i];
//        const int bX = i * 56 + 256;
//        if (bttn) //> 0)
//        {
//            //		bttn->SetX(bX);
//        }
//        // auto frame = H3DlgFrame::Create(buttons.asArray[i], 1, frameColor);
//        // this->AddItem(frame);
//    }
//    dlgPanel->CreateBorderFrame();
//}
//
//DlgPanel::~DlgPanel()
//{
//    delete dlgPanel;
//}
//
//int DlgPanel::GetX() const noexcept
//{
//    return dlgPanel->GetX();
//}
//
//int DlgPanel::GetY() const noexcept
//{
//    return dlgPanel->GetY();
//}
//
//INT32 __fastcall CategoriesPanel::PanelCategory::Proc(H3Msg *msg)
//{
//    return 0;
//}
//
//void CategoriesPanel::PanelCategory::SetCategory(const main::Category *category)
//{
//
//    // if src has pcx and current doesn't
//    if (category->iconPcx && !icon->GetPcx())
//    {
//        // resize and move text item
//
//        textPcx->SetX(textPcx->GetX() + textIcoXOffset);
//        textPcx->SetWidth(textPcx->GetWidth() - textIcoXOffset);
//
//        // show icon
//        icon->ShowActivate();
//    }
//    // else if current has and sorc doesn't
//    else if (!category->iconPcx && icon->GetPcx())
//    {
//        // resize and move text item
//        textPcx->SetX(textPcx->GetX() - textIcoXOffset);
//        textPcx->SetWidth(textPcx->GetWidth() + textIcoXOffset);
//
//        // hide icon
//        icon->HideDeactivate();
//    }
//
//    // set new pcx
//    icon->SetPcx(category->iconPcx);
//    // assign category
//    this->category = category; //->UnfocusEdits(false);
//    // set text
//    textPcx->SetText(category->name);
//}
