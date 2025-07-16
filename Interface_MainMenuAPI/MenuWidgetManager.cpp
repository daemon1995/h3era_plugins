#include "MenuWidgetManager.h"

MenuWidgetManager &MenuWidgetManager::Get()
{
    static MenuWidgetManager instance;
    return instance;
}

void MenuWidgetManager::RegisterWidget(const MenuWidgetInfo &info)
{
    if (widgets.find(info.name) != widgets.end())
    {
        std::string errorMsg = "Widget with name '" + info.name + "' is already registered!";
        MessageBoxA(NULL, errorMsg.c_str(), "Widget Registration Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    widgets[info.name] = info;
}

void MenuWidgetManager::CreateWidgets(H3BaseDlg *dlg, const int widgetStartX, const int widgetStartY,
                                      const int widgetWidth, const int widgetHeight, const int widgetSpacing)
{
    // Устанавливаем параметры размещения виджетов
    this->widgetStartX = widgetStartX;
    this->widgetStartY = widgetStartY;
    this->widgetWidth = widgetWidth;
    this->widgetHeight = widgetHeight;
    this->widgetSpacing = widgetSpacing;

    const int x = widgetStartX;
    const int y = widgetStartY;
    
    int currentId = START_WIDGET_ID;
    int i = 0;
    for (auto& pair : widgets)
    {
        auto &widget = pair.second;
        widget.id = currentId++; // Assign and increment ID
        widget.uiElement = dlg->CreateCaptionButton(
            x, y + i * (widgetHeight + widgetSpacing), widgetWidth, widgetHeight, widget.id, DEF_NAME,
            widget.text.c_str(), h3::NH3Dlg::Text::MEDIUM, 0);
        widget.uiElement->SetClickFrame(1);
        i++;
    }
}
H3DlgCaptionButton* MenuWidgetManager::GetWidgetByName(const std::string& name) const
{
    auto it = widgets.find(name);
    if (it != widgets.end())
        return it->second.uiElement;
    return nullptr;
}

bool MenuWidgetManager::HasWidget(const std::string& name) const
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
    for (auto it = widgets.begin(); it != widgets.end(); ++it)
    {
        const MenuWidgetInfo& widget = it->second;
        if (widget.uiElement && widget.id == msg->itemId)
        {
            if (widget.onClick)
            {
                widget.onClick(*msg);
            }
        }
    }
}

const std::unordered_map<std::string, MenuWidgetInfo> &MenuWidgetManager::GetWidgets() const
{
    return widgets;
}

extern "C" __declspec(dllexport) void __stdcall RegisterMainMenuWidget(const MenuWidgetInfo &info)
{
    MenuWidgetManager::Get().RegisterWidget(info);
}

extern "C" __declspec(dllexport) H3DlgCaptionButton* __stdcall GetMainMenuWidgetByName(const char* name)
{
    if (!name) return nullptr;
    return MenuWidgetManager::Get().GetWidgetByName(name);
}

extern "C" __declspec(dllexport) int __stdcall GetMainMenuWidgetIntId(const char* name)
{
    if (!name) return 0;
    auto it = MenuWidgetManager::Get().GetWidgets().find(name);
    if (it != MenuWidgetManager::Get().GetWidgets().end())
    {
        return it->second.uiElement->GetID();
    }
    return 0;
}
