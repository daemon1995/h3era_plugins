#pragma once
#include "framework.h"
#include <functional>
#include <unordered_map>
#include <vector>

enum eMenuList
{
    Main = 0x1,
    NewGame = 0x2,
    LoadGame = 0x4,
    Campaign = 0x8,
    All = 0xF
};
struct MenuWidgetInfo
{
    std::string name; // Renamed from id
    std::string text;
    eMenuList menuList = eMenuList::Main;
    std::function<void(void *)> onClick;
    H3DlgCaptionButton *uiElement = nullptr;
    int id = 0; // Integer ID for the dialog item
};

class MenuWidgetManager
{
  public:
    static BOOL initialized;
    static constexpr LPCSTR WIDGET_NAME_HIDE = "main_menu_api_widget_hide";
    static constexpr LPCSTR WIDGET_NAME_OPTIONS = "main_menu_api_widget_options";
    static constexpr LPCSTR WIDGET_TEXT_SHOW = "era.api.main_menu.show";
    static constexpr LPCSTR WIDGET_TEXT_HIDE = "era.api.main_menu.hide";
    static constexpr LPCSTR WIDGET_TEXT_OPTIONS = "era.api.main_menu.options";

  private:
    static constexpr LPCSTR DEF_NAME = "tpthbar.def";
    static constexpr int START_WIDGET_ID = 400; // Starting ID for widgets
    BOOL visible = TRUE;
    int widgetWidth = 120;
    int widgetHeight = 24;
    int widgetStartX = 800;
    int widgetStartY = 0;
    int widgetSpacing = 8;
    H3LoadedPcx16 *backgroundPcx = nullptr;
    H3LoadedPcx16 *backupScreen = nullptr;
    H3DlgPcx16 *background = nullptr;
    std::unordered_map<std::string, MenuWidgetInfo> widgets;
    std::vector<MenuWidgetInfo *> createdWidgets;
    MenuWidgetManager() = default;

  private:
    void SetVisible(const bool visible);

  public:
    BOOL RegisterWidget(const MenuWidgetInfo &info);
    void CreateWidgets(H3BaseDlg *dlg, const eMenuList menuList, const int widgetStartX = 0, const int widgetStartY = 0,
                       const int widgetWidth = 120, const int widgetHeight = 20, const int widgetSpacing = 2);
    void DestroyWidgets(H3BaseDlg *dlg);
    void HandleEvent(H3Msg *msg);
    const std::unordered_map<std::string, MenuWidgetInfo> &GetWidgets() const;
    void SetLayoutParams(int width, int height, int startX, int startY, int spacing);

    // Новый API для доступа к виджетам по id
    H3DlgCaptionButton *GetWidgetByName(const std::string &name) const;
    bool HasWidget(const std::string &name) const;

  public:
    static MenuWidgetManager &Get();
    static void OnHideButtonProc(void *_msg);
    static void OnOptionsButtonProc(void *_msg);
};

// Экспорт для других плагинов
extern "C" __declspec(dllexport) BOOL __stdcall RegisterMainMenuWidget(const MenuWidgetInfo &info);
extern "C" __declspec(dllexport) H3DlgCaptionButton *__stdcall GetMainMenuWidgetByName(const char *name);
extern "C" __declspec(dllexport) int __stdcall GetMainMenuWidgetId(const char *name);
