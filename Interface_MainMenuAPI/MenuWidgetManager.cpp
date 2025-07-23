#include "MenuWidgetManager.h"
BOOL MenuWidgetManager::initialized = false;
MenuWidgetManager &MenuWidgetManager::Get()
{
    static MenuWidgetManager instance;
    return instance;
}

void MenuWidgetManager::SetVisible(const bool visible)
{
    if (visible != this->visible)
    {

        this->visible = visible;
        if (visible)
        {
            // making hd mod wrong main menu drawing offset fix
            const int _x = background->GetX() + (H3GameWidth::Get() - 800) / 2;
            const int _y = background->GetY() + (H3GameHeight::Get() - 600) / 2;
            backupScreen->CopyRegion(P_WindowManager->GetDrawBuffer(), _x, _y);
        }
        background->SetPcx(visible ? backgroundPcx : backupScreen);

        for (auto &wg : createdWidgets)
        {
            if (auto *it = wg->uiElement)
            {
                visible ? it->ShowActivate(), it->Draw(), it->Refresh() : it->HideDeactivate();
            }
        }

        if (!visible)
        {
            background->Draw();
            background->Refresh();
        }
    }
}
void MenuWidgetManager::OnHideButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {

            auto &instance = Get();
            instance.SetVisible(instance.visible ^ true);
            msg->GetDlg()->Redraw();
        }
    }
}
void MenuWidgetManager::OnOptionsButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {

            auto skipButtonsCreation = _PI->WriteJmp(0x05B1BED, 0x05B1DA0);
            char dlgMemory[104];
            THISCALL_1(int, 0x05B1AA0, dlgMemory); // ctor
            THISCALL_1(int, 0x05B33C0, dlgMemory); // run
            THISCALL_1(int, 0x05B3350, dlgMemory); // dtor

            skipButtonsCreation->Destroy();
        }
    }
}
BOOL MenuWidgetManager::RegisterWidget(const MenuWidgetInfo &info)
{
    if (widgets.find(info.name) != widgets.end())
    {
        std::string errorMsg = "Widget with name '" + info.name + "' is already registered!";
        MessageBoxA(NULL, errorMsg.c_str(), "Widget Registration Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    widgets[info.name] = info;
    return TRUE;
}

void MenuWidgetManager::CreateWidgets(H3BaseDlg *dlg, const eMenuList menuList, const int widgetStartX,
                                      const int widgetStartY, const int widgetWidthA, const int widgetHeightA,
                                      const int widgetSpacing)
{
    // Устанавливаем параметры размещения виджетов
    SetLayoutParams(widgetWidthA, widgetHeightA, widgetStartX, widgetStartY, widgetSpacing);

    createdWidgets.clear();
    createdWidgets.reserve(widgets.size());
    for (auto &pair : widgets)
    {
        MenuWidgetInfo &widget = pair.second;
        if (widget.menuList & menuList)
        {
            createdWidgets.push_back(&widget);
        }
    }
    const int widgetCount = createdWidgets.size();
    if (widgetCount < 2)
    {
        return;
    }

    // put the "main_menu_api_widget" at the top of the list
    for (size_t i = 1; i < widgetCount; i++)
    {
        if (createdWidgets[i]->name == WIDGET_NAME_HIDE)
        {
            std::swap(createdWidgets[i], createdWidgets[0]);
            break;
        }
    }

    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();
    constexpr int backgroundWidth = 800;
    constexpr int backgroundHeight = 600;
    const int backgroundX = gameWidth - backgroundWidth >> 1;
    const int backgroundY = gameHeight - backgroundHeight >> 1;

    constexpr int frameWidth = 4;

    const int xFrameOffset = (gameWidth > backgroundWidth + 8) ? frameWidth : 0;
    const int yFrameOffset = (gameHeight > backgroundHeight + 8) ? frameWidth : 0;

    H3DefLoader def(DEF_NAME);
    widgetWidth = def->widthDEF;
    widgetHeight = def->heightDEF;

    const int backgroundAreaHeight = widgetHeight * widgetCount + widgetSpacing * (widgetCount - 1) + (frameWidth << 1);
    const int backgroundAreaWidth = Clamp(100, widgetWidth, 200) + (frameWidth << 1);

    backgroundPcx = H3LoadedPcx16::Create(backgroundAreaWidth, backgroundAreaHeight);
    backupScreen = H3LoadedPcx16::Create(backgroundAreaWidth, backgroundAreaHeight);

    backgroundPcx->BackgroundRegion(0, 0, backgroundAreaWidth, backgroundAreaHeight, true);
    backgroundPcx->SimpleFrameRegion(0, 0, backgroundAreaWidth, backgroundAreaHeight);
    background = H3DlgPcx16::Create(widgetStartX - xFrameOffset, widgetStartY - yFrameOffset, backgroundAreaWidth,
                                    backgroundAreaHeight, 0, nullptr);

    background->SetPcx(backgroundPcx);
    background->Hide();
    dlg->AddItem(background);
    backupScreen->CopyRegion(P_WindowManager->GetDrawBuffer(), widgetStartX - xFrameOffset + backgroundX,
                             widgetStartY - yFrameOffset + backgroundY);
    background->Show();
    visible = true;
    const int x = widgetStartX; // +xFrameOffset;
    const int y = widgetStartY; // +yFrameOffset;

    // Создаем кнопки для каждого виджета
    int currentId = START_WIDGET_ID;
    int i = 0;
    for (auto *widget : createdWidgets)
    {
        widget->id = currentId++; // Assign and increment ID

        widget->uiElement =
            dlg->CreateCaptionButton(x, y + i * (widgetHeight + widgetSpacing), widgetWidth, widgetHeight, widget->id,
                                     DEF_NAME, widget->text.c_str(), h3::NH3Dlg::Text::MEDIUM, 0);
        widget->uiElement->SetClickFrame(1);
        widget->uiElement->SetFrame(3);
        i++;
    }
}
void MenuWidgetManager::DestroyWidgets(H3BaseDlg *dlg)
{

    background->SetPcx(nullptr);

    if (backgroundPcx)
    {
        backgroundPcx->Destroy();
        backgroundPcx = nullptr;
    }
    if (backupScreen)
    {
        backupScreen->Destroy();
        backupScreen = nullptr;
    }

    for (auto &wg : widgets)
    {
        wg.second.uiElement = nullptr; // Clear UI elements
        wg.second.id = 0;              // Clear onClick handlers
    }

    createdWidgets.clear();
}
H3DlgCaptionButton *MenuWidgetManager::GetWidgetByName(const std::string &name) const
{
    auto it = widgets.find(name);
    if (it != widgets.end())
        return it->second.uiElement;
    return nullptr;
}

bool MenuWidgetManager::HasWidget(const std::string &name) const
{
    return widgets.find(name) != widgets.end();
}

void MenuWidgetManager::SetLayoutParams(int width, int height, int startX, int startY, int spacing)
{
    widgetWidth = width;
    widgetHeight = height;
    widgetStartX = startX;
    widgetStartY = startY;
    widgetSpacing = spacing;
}

void MenuWidgetManager::HandleEvent(H3Msg *msg)
{
    for (const MenuWidgetInfo *widget : createdWidgets)
    {
        if (widget->uiElement && widget->id == msg->itemId && widget->onClick)
        {
            widget->onClick(msg);
        }
    }
}

const std::unordered_map<std::string, MenuWidgetInfo> &MenuWidgetManager::GetWidgets() const
{
    return widgets;
}

extern "C" __declspec(dllexport) BOOL __stdcall RegisterMainMenuWidget(const MenuWidgetInfo &info)
{
    return MenuWidgetManager::Get().RegisterWidget(info);
}

extern "C" __declspec(dllexport) H3DlgCaptionButton *__stdcall GetMainMenuWidgetByName(const char *name)
{
    if (!name)
        return nullptr;
    return MenuWidgetManager::Get().GetWidgetByName(name);
}

extern "C" __declspec(dllexport) int __stdcall GetMainMenuWidgetId(const char *name)
{
    if (name)
    {
        auto it = MenuWidgetManager::Get().GetWidgets().find(name);
        if (it != MenuWidgetManager::Get().GetWidgets().end())
        {
            return it->second.uiElement->GetID();
        }
    }
    return 0;
}
