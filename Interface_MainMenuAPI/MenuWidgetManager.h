#pragma once
#include <vector>
#include <unordered_map>
#include <functional>
#include "framework.h"

struct MenuWidgetInfo
{
    std::string name;                      // Renamed from id
    std::string text;
    std::function<void(H3Msg&)> onClick; // onClick now takes H3Msg&
    H3DlgCaptionButton* uiElement = nullptr;
    int id = 0;                            // Integer ID for the dialog item
};

class MenuWidgetManager
{
public:
    static MenuWidgetManager& Get();
    void RegisterWidget(const MenuWidgetInfo& info);
    void CreateWidgets(H3BaseDlg* dlg, const int widgetStartX = 100, const int widgetStartY = 100,
                       const int widgetWidth = 120, const int widgetHeight = 24, const int widgetSpacing = 8);
    void HandleEvent(H3Msg* msg);
    const std::unordered_map<std::string, MenuWidgetInfo>& GetWidgets() const;
    void SetLayoutParams(int width, int height, int startX, int startY, int spacing);

    // Новый API для доступа к виджетам по id
    H3DlgCaptionButton* GetWidgetByName(const std::string& name) const;
    bool HasWidget(const std::string& name) const;

private:
    static constexpr LPCSTR DEF_NAME = "tpthbar.def";
    static constexpr int START_WIDGET_ID = 400; // Starting ID for widgets
    int widgetWidth = 120;
    int widgetHeight = 24;
    int widgetStartX = 100;
    int widgetStartY = 100;
    int widgetSpacing = 8;
    std::unordered_map<std::string, MenuWidgetInfo> widgets;
    MenuWidgetManager() = default;
};

// Экспорт для других плагинов
extern "C" __declspec(dllexport) void __stdcall RegisterMainMenuWidget(const MenuWidgetInfo& info);
extern "C" __declspec(dllexport) H3DlgCaptionButton* __stdcall GetMainMenuWidgetByName(const char* name);
extern "C" __declspec(dllexport) int __stdcall GetMainMenuWidgetIntId(const char* name);
