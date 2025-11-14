#include "MenuWidgetManager.h"
BOOL MenuWidgetManager::initialized = false;
MenuWidgetManager &MenuWidgetManager::Get()
{
    static MenuWidgetManager instance;
    return instance;
}
BOOL MenuWidgetManager::IsVisible() const noexcept
{
    return isVisible;
}

void MenuWidgetManager::SetVisible(const bool visible)
{
    if (visible != this->isVisible)
    {
        this->isVisible = visible;
        if (visible && !placedOutside && insideBackupScreenPcx)
        {
            // making hd mod wrong main menu drawing offset fix
            const int _x = framedBackground->GetX() + (H3GameWidth::Get() - 800) / 2;
            const int _y = framedBackground->GetY() + (H3GameHeight::Get() - 600) / 2;
            insideBackupScreenPcx->CopyRegion(P_WindowManager->GetDrawBuffer(), _x, _y);
        }

        framedBackground->SetPcx(visible         ? framedBackgroundPcx
                                 : placedOutside ? outsideBackupScreenPcx
                                                 : insideBackupScreenPcx);

        // Era::red
        for (size_t i = 0; i < createdWidgets.size(); i++)
        {
            auto *widget = createdWidgets[i];
            if (auto it = widget->uiElement)
            {
                const int itemId = it->GetID();
                if (itemId >= topWidgetId && itemId <= bottomWidgetId)
                {
                    visible ? it->ShowActivate(), it->Draw(), it->Refresh() : it->HideDeactivate();
                }
            }
        }

        if (auto *it = scrollbar)
        {
            visible ? it->ShowActivate(), it->Draw(), it->Refresh() : it->HideDeactivate();
        }

        for (auto &it : arrows)
            if (it)
                visible ? it->ShowActivate(), it->Draw(), it->Refresh() : it->HideDeactivate();

        if (!visible)
        {
            framedBackground->Draw();
            framedBackground->Refresh();
            framedBackground->HideDeactivate();
        }
    }
}

void MenuWidgetManager::RedrawWidgets(const INT32 firstWidgetIndex, H3BaseDlg *dlg) noexcept
{
    if (firstWidgetIndex >= 0 && firstWidgetIndex != topWidgetId - START_WIDGET_ID &&
        firstWidgetIndex + displayedWidgetsCount <= createdWidgets.size())
    {
        topWidgetId = firstWidgetIndex + START_WIDGET_ID;
        bottomWidgetId = topWidgetId + displayedWidgetsCount - 1;
        if (scrollbar)
        {
            scrollbar->Draw();
            scrollbar->Refresh();
        }

        // int drawnWidgetsCount = 0;
        for (size_t i = 0; i < createdWidgets.size(); i++)
        {
            auto *widget = createdWidgets[i];
            if (auto it = widget->uiElement)
            {
                const int itemId = it->GetID();
                if (itemId >= topWidgetId && itemId <= bottomWidgetId)
                {

                    it->SetX(widgetStartX);
                    it->SetY(widgetStartY + (i - firstWidgetIndex) * (widgetHeight + widgetSpacing));

                    it->ShowActivate();
                    it->Draw();
                    it->Refresh();
                }
                else
                {
                    it->HideDeactivate();
                }
            }
        }
    }
}

VOID __fastcall MenuWidgetManager::DlgScroll_Proc(INT32 tickId, H3BaseDlg *dlg)
{
    Get().RedrawWidgets(tickId, dlg);
}

void MenuWidgetManager::CreateWidgets(H3BaseDlg *dlg, const mainmenu::eMenuFlags menuList)
{

    const size_t registeredWidgetsCount = registeredWidgets.size();
    if (registeredWidgetsCount == 0)
    {
        // no widgets registered for this menu type
        return;
    }

    // createdWidgets.reserve(registeredWidgetsCount);
    menuType = menuList;

    std::vector<LocalMenuWidgetInfo *> topWidgets, bottomWidgets, middleWidgets;

    for (auto &widget : registeredWidgets)
    {
        const int widgetFlags = widget.menuList;
        if (widgetFlags & menuList)
        {
            if (widgetFlags & mainmenu::ON_TOP)
            {
                topWidgets.push_back(&widget);
            }
            else if (widgetFlags & mainmenu::AT_BOTTOM)
            {
                bottomWidgets.push_back(&widget);
            }
            else
            {
                middleWidgets.push_back(&widget);
            }
        }
    }

    createdWidgets.clear();
    createdWidgets.reserve(topWidgets.size() + middleWidgets.size() + bottomWidgets.size());
    createdWidgets.insert(createdWidgets.end(), topWidgets.begin(), topWidgets.end());
    createdWidgets.insert(createdWidgets.end(), middleWidgets.begin(), middleWidgets.end());
    createdWidgets.insert(createdWidgets.end(), bottomWidgets.begin(), bottomWidgets.end());

    createdWidgets.shrink_to_fit();
    size_t currentMenuTypeWidgets = createdWidgets.size();

    LPCSTR fontName = h3::NH3Dlg::Text::MEDIUM;
    H3FontLoader fnt(fontName);
    // put the "main_menu_api_widget" at the top of the list
    constexpr int assetWidth = 174;
    BOOL isBigAsset = false;
    int hideWidgetInd = -1;
    for (size_t i = 0; i < currentMenuTypeWidgets; i++)
    {
        const auto &displayedName = createdWidgets[i]->name;
        if (displayedName == BaseGameWidgets::WIDGET_NAME_HIDE)
        {
            hideWidgetInd = i;
        }
        if (!isBigAsset && fnt->GetLinesCountInText(createdWidgets[i]->text.c_str(), assetWidth) > 1)
        {
            isBigAsset = true;
        }
    }
    // if we have a hide widget, put it at the top of the list
    if (hideWidgetInd >= 0)
    {
        // if there is only one widget, we return
        if (currentMenuTypeWidgets < 2)
        {
            createdWidgets.clear();
            return;
        }

        std::swap(createdWidgets[hideWidgetInd], createdWidgets[0]);
    }

    LPCSTR assetName = isBigAsset ? assets::BIG_BUTTON : assets::SMALL_BUTTON;

    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();

    constexpr int backgroundWidth = 800;
    constexpr int backgroundHeight = 600;
    constexpr int minimalOutsideWidth = 1180;
    constexpr int frameWidth = 4;
    constexpr int scrollbarOffset = 0;

    constexpr int frameGameWidth = backgroundWidth + frameWidth * 2;
    constexpr int frameGameHeight = backgroundHeight + frameWidth * 2;

    constexpr int scrollbarWidth = 16;
    constexpr int arrowHeight = 16;

    const int backgroundX = gameWidth - backgroundWidth >> 1;
    const int backgroundY = gameHeight - backgroundHeight >> 1;

    const int frameOffset = (gameWidth < frameGameWidth) || (gameHeight < frameGameHeight) ? 0 : frameWidth;

    H3DefLoader def(assetName);
    // widgetWidth = def->widthDEF;
    widgetHeight = def->heightDEF;
    //    widgetStartX = backgroundWidth + xFrameOffset;

    placedOutside = gameWidth >= minimalOutsideWidth;

    if (placedOutside && hideWidgetInd >= 0)
    {
        memmove(&createdWidgets[0], &createdWidgets[1], (currentMenuTypeWidgets - 1) * sizeof(LocalMenuWidgetInfo *));
        currentMenuTypeWidgets -= 1; // Remove the hide widget from the list
        createdWidgets.pop_back();
    }

    const BOOL framesInternal =
        (gameWidth < backgroundWidth + frameWidth * 2) || (gameHeight < backgroundHeight + frameWidth * 2);

    frameStartX = placedOutside ? backgroundWidth + frameOffset : 0;
    frameStartY = placedOutside ? -frameOffset : 0; // backgroundY;

    const int availableHeight = backgroundHeight - framesInternal * (frameOffset << 1);

    enum ScrollType
    {
        SLIDER,
        ARROWS
    };
    const ScrollType scrollType = placedOutside && gameWidth <= minimalOutsideWidth + scrollbarWidth ? ARROWS : SLIDER;

    BOOL createScrollbar = false;
    INT widgetsToDraw = currentMenuTypeWidgets;
    INT additionalHeight = 0;
    INT additionalWidth = 0;

    const BOOL widgetsFit =
        availableHeight >= widgetHeight * currentMenuTypeWidgets + widgetSpacing * (currentMenuTypeWidgets - 1);

    if (!widgetsFit)
    {
        createScrollbar = true;
        const int heightPerWidget = widgetHeight + widgetSpacing;

        widgetsToDraw = (availableHeight + widgetSpacing) / heightPerWidget;
        if (scrollType == ScrollType::ARROWS)
        {
            const int arrowsHeight = arrowHeight << 1;

            widgetsToDraw = (availableHeight + widgetSpacing - arrowsHeight) / heightPerWidget;
            additionalHeight += arrowsHeight; // Add height for arrows
        }
        else
        {
            additionalWidth = scrollbarWidth + scrollbarOffset; // Add width for scrollbar
        }
    }

    const int backgroundAreaHeight =
        widgetHeight * widgetsToDraw + widgetSpacing * (widgetsToDraw - 1) + (frameWidth << 1) + additionalHeight;
    const int backgroundAreaWidth = widgetWidth + additionalWidth + (frameWidth << 1);

    framedBackgroundPcx = H3LoadedPcx16::Create(backgroundAreaWidth, backgroundAreaHeight);

    framedBackgroundPcx->BackgroundRegion(0, 0, backgroundAreaWidth, backgroundAreaHeight, true);
    framedBackgroundPcx->SimpleFrameRegion(0, 0, backgroundAreaWidth, backgroundAreaHeight);

    framedBackground =
        H3DlgPcx16::Create(frameStartX, frameStartY, backgroundAreaWidth, backgroundAreaHeight, 0, nullptr);
    framedBackground->SetPcx(framedBackgroundPcx);
    framedBackground->HideDeactivate();

    dlg->AddItem(framedBackground);

    if (placedOutside)
    {
        if (outsideBackupScreenPcx == nullptr)
        {
            outsideBackupScreenPcx = H3LoadedPcx16::Create(backgroundAreaWidth, backgroundHeight + frameOffset * 2);
            outsideBackupScreenPcx->CopyRegion(P_WindowManager->GetDrawBuffer(), framedBackground->GetAbsoluteX(),
                                               framedBackground->GetAbsoluteY());
        }
    }
    else
    {
        insideBackupScreenPcx = H3LoadedPcx16::Create(backgroundAreaWidth, backgroundAreaHeight);
        insideBackupScreenPcx->CopyRegion(P_WindowManager->GetDrawBuffer(), framedBackground->GetAbsoluteX(),
                                          framedBackground->GetAbsoluteY());
    }

    framedBackground->Show();
    isVisible = true;

    int yArrowOffset = 0;
    widgetStartX = frameStartX + frameWidth;

    if (createScrollbar)
    {
        const int arrowStartY = frameStartY + frameWidth;

        if (scrollType == ARROWS)
        {
            yArrowOffset = arrowHeight; // Offset for arrows
            arrows[0] = dlg->CreateButton(widgetStartX, arrowStartY, widgetWidth, arrowHeight, ARROWS_ITEM_IDS[0],
                                          assets::ARROW_UP, 0, 1);
            arrows[1] = dlg->CreateButton(widgetStartX, frameStartY + backgroundAreaHeight - arrowHeight - frameWidth,
                                          widgetWidth, arrowHeight, ARROWS_ITEM_IDS[1], assets::ARROW_DOWN, 0, 1);
        }
        else
        {
            const int ticksCount = createdWidgets.size() - widgetsToDraw + 1;
            scrollbar = dlg->CreateScrollbar(widgetStartX + widgetWidth + scrollbarOffset, arrowStartY, scrollbarWidth,
                                             backgroundAreaHeight - (frameWidth << 1), SCROLLBAR_ITEM_ID, ticksCount,
                                             DlgScroll_Proc, true);
        }
    }

    widgetStartY = yArrowOffset + frameStartY + frameWidth; // Adjust widget start Y position
    // Создаем кнопки для каждого виджета
    int currentId = START_WIDGET_ID;
    topWidgetId = currentId;

    // create displayed widgets
    for (size_t i = 0; i < widgetsToDraw; i++)
    {
        auto *widget = createdWidgets[i];
        widget->id = currentId++; // Assign and increment ID

        widget->uiElement = dlg->CreateCaptionButton(widgetStartX, widgetStartY + i * (widgetHeight + widgetSpacing),
                                                     widgetWidth, widgetHeight, widget->id, assetName,
                                                     widget->text.c_str(), fontName, 0, 0, 0, 0, eTextColor::REGULAR);
        widget->uiElement->SetClickFrame(1);
    }

    // assign hotkeys to first 10 (MAX_WIDGETS_WITH_HOTKEYS) widgets if no scrollbar
    if (createScrollbar == false)
    {
        size_t maxHotkeyWidgetIndex =
            widgetsToDraw < MAX_WIDGETS_WITH_HOTKEYS ? widgetsToDraw : MAX_WIDGETS_WITH_HOTKEYS;
        for (size_t i = 0; i < maxHotkeyWidgetIndex; i++)
        {
            createdWidgets[i]->uiElement->AddHotkey(eVKey::H3VK_1 + i);
        }
    }
    bottomWidgetId = currentId - 1; // Last widget ID
    // create hidden widgets
    for (size_t i = widgetsToDraw; i < currentMenuTypeWidgets; i++)
    {
        auto *widget = createdWidgets[i];
        widget->id = currentId++; // Assign and increment ID

        widget->uiElement = dlg->CreateCaptionButton(0, 0, widgetWidth, widgetHeight, widget->id, assetName,
                                                     widget->text.c_str(), fontName, 0, 0, 0, 0, eTextColor::REGULAR);
        widget->uiElement->SetClickFrame(1);
        widget->uiElement->HideDeactivate();
    }
    displayedWidgetsCount = widgetsToDraw;
}
void MenuWidgetManager::DestroyWidgets(H3BaseDlg *dlg)
{

    if (framedBackground)
    {

        if (isVisible)
        {
            if (!placedOutside && insideBackupScreenPcx)
            {
                framedBackground->SetPcx(insideBackupScreenPcx);
                framedBackground->Draw();
                framedBackground->Refresh();
            }
            else if (outsideBackupScreenPcx)
            {
                framedBackground->SetWidth(outsideBackupScreenPcx->width);
                framedBackground->SetHeight(outsideBackupScreenPcx->height);
                framedBackground->SetPcx(outsideBackupScreenPcx);

                framedBackground->Draw();
                framedBackground->Refresh();
            }
        }

        framedBackground->SetPcx(nullptr);
        framedBackground = nullptr;
    }

    if (insideBackupScreenPcx)
    {
        insideBackupScreenPcx->Destroy();
        insideBackupScreenPcx = nullptr;
    }

    if (framedBackgroundPcx)
    {
        framedBackgroundPcx->Destroy();
        framedBackgroundPcx = nullptr;
    }

    for (auto &wg : registeredWidgets)
    {
        wg.uiElement = nullptr; // Clear UI elements
        wg.id = 0;
    }

    topWidgetId = 0;
    bottomWidgetId = 0;
    displayedWidgetsCount = 0;
    widgetStartX = 0;
    widgetStartY = 0;
    frameStartX = 0;
    frameStartY = 0;
    isVisible = false;
    placedOutside = false;
    scrollbar = nullptr;
    memset(arrows, 0, sizeof(arrows));
    menuType = mainmenu::eMenuFlags::MAIN;
    createdWidgets.clear();
}

void MenuWidgetManager::HandleEvent(H3Msg *msg)
{

    if (isVisible)
    {
        if (msg->itemId >= topWidgetId && msg->itemId <= bottomWidgetId)
        {
            const int widgetInd = msg->itemId - START_WIDGET_ID;
            const LocalMenuWidgetInfo *widget = createdWidgets[widgetInd];

            if (widget && widget->uiElement && widget->customProc && widget->id == msg->itemId)
            {
                if (widget->customProc(msg))
                    return;
            }
        }
        else if (msg->IsLeftClick())
        {

            if (arrows[0] && msg->itemId == arrows[0]->GetID())
            {
                const int previousId = topWidgetId - START_WIDGET_ID - 1;
                RedrawWidgets(previousId, msg->GetDlg());
            }
            else if (arrows[1] && msg->itemId == arrows[1]->GetID())
            {
                const int nextId = topWidgetId - START_WIDGET_ID + 1;
                RedrawWidgets(nextId, msg->GetDlg());
            }
        }
    }
}

INT MenuWidgetManager::RegisteredNumber() const noexcept
{
    return registeredWidgets.size();
}

BOOL MenuWidgetManager::RegisterWidget(const mainmenu::MenuWidgetInfo &info)
{
    std::string name = info.name;
    if (name.empty())
    {
        std::string errorMsg = "Widget with name is empty!";
        MessageBoxA(NULL, errorMsg.c_str(), "Widget Registration Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (widgetIndexes.find(name) != widgetIndexes.end())
    {
        std::string errorMsg = "Widget with name '" + name + "' is already registered!";
        MessageBoxA(NULL, errorMsg.c_str(), "Widget Registration Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    const int lastId = registeredWidgets.size();

    widgetIndexes[name] = lastId;
    registeredWidgets.emplace_back(LocalMenuWidgetInfo{info});
    return TRUE;
}

H3DlgCaptionButton *MenuWidgetManager::GetWidgetByName(LPCSTR name) const
{
    const auto it = widgetIndexes.find(name);
    if (it != widgetIndexes.end())
        return registeredWidgets[it->second].uiElement;
    return nullptr;
}

bool MenuWidgetManager::HasWidget(const std::string &name) const
{
    return widgetIndexes.find(name) != widgetIndexes.end();
}
int MenuWidgetManager::GetWidgetId(const std::string &name) const
{
    const auto it = widgetIndexes.find(name);
    if (it != widgetIndexes.end())
        return registeredWidgets[it->second].id;
    return 0;
}

BOOL MenuWidgetManager::SetWidgetText(const std::string &name, LPCSTR text)
{
    const auto it = widgetIndexes.find(name);
    if (it != widgetIndexes.end())
    {
        auto &wgt = registeredWidgets[it->second];
        if (wgt.text != text)
        {
            wgt.text = text;
            if (auto it = wgt.uiElement)
            {
                it->SetText(text);
                if (isVisible)
                {
                    it->Draw();
                    it->Refresh();
                }
            }

            return true;
        }
    }

    return false;
}

extern "C" __declspec(dllexport) BOOL __stdcall MainMenu_RegisterWidget(const mainmenu::MenuWidgetInfo &info)
{
    if (!info.name)
        return false;
    return MenuWidgetManager::Get().RegisterWidget(info);
}

extern "C" __declspec(dllexport) H3DlgCaptionButton *__stdcall MainMenu_GetDialogButton(const char *name)
{
    if (!name)
        return nullptr;
    return MenuWidgetManager::Get().GetWidgetByName(name);
}

extern "C" __declspec(dllexport) int __stdcall MainMenu_GetDialogButtonId(const char *name)
{
    if (!name)
        return 0;
    return MenuWidgetManager::Get().GetWidgetId(name);
}

extern "C" __declspec(dllexport) BOOL __stdcall MainMenu_SetDialogButtonText(const char *name, const char *text)
{
    if (!name || !text)
        return 0;
    return MenuWidgetManager::Get().SetWidgetText(name, text);
}

LocalMenuWidgetInfo::LocalMenuWidgetInfo(const mainmenu::MenuWidgetInfo &ext)
    : name(ext.name), text(ext.text), menuList(ext.menuList), customProc(ext.customProc), uiElement(nullptr), id(0)
{
}
