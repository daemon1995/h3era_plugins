#pragma once
#include "framework.h"
#include <functional>
#include <unordered_map>
#include <vector>

struct LocalMenuWidgetInfo
{
    std::string name; // Renamed from id
    std::string text;
    mainmenu::eMenuList menuList = mainmenu::eMenuList::MAIN;
    int(__fastcall *customProc)(void *) = nullptr;
    H3DlgCaptionButton *uiElement = nullptr;
    int id = 0; // Integer ID for the dialog item
  public:
    LocalMenuWidgetInfo(const mainmenu::MenuWidgetInfo &ext);
};

class MenuWidgetManager
{
  public:
    static BOOL initialized;

  private:
    struct assets
    {
        static constexpr LPCSTR BIG_BUTTON = "emenubig.def";
        static constexpr LPCSTR SMALL_BUTTON = "emenusml.def";
        static constexpr LPCSTR ARROW_UP = "emenuup.def";
        static constexpr LPCSTR ARROW_DOWN = "emenudwn.def";
    };
    static constexpr int ARROWS_ITEM_IDS[2] = {397, 398}; // Starting ID for widgets
    static constexpr int SCROLLBAR_ITEM_ID = 399;         // Starting ID for widgets
    static constexpr int START_WIDGET_ID = 400;           // Starting ID for widgets
    BOOL isVisible = true;
    BOOL placedOutside = false;
    int widgetWidth = 174;
    int widgetHeight = 24;

    int frameStartX = 0;
    int frameStartY = 0;
    int widgetStartX = 0;
    int widgetStartY = 0;
    int widgetSpacing = 2;

    int topWidgetId = 0;
    int bottomWidgetId = 0;
    int displayedWidgetsCount = 0;
    mainmenu::eMenuList menuType = mainmenu::eMenuList::MAIN;
    H3LoadedPcx16 *framedBackgroundPcx = nullptr;
    H3LoadedPcx16 *insideBackupScreenPcx = nullptr;
    H3LoadedPcx16 *outsideBackupScreenPcx = nullptr;
    // H3LoadedPcx16 *backupScreens[4] = {nullptr, nullptr, nullptr, nullptr};
    H3DlgScrollbar *scrollbar = nullptr;
    H3DlgDefButton *arrows[2] = {nullptr, nullptr};
    H3DlgPcx16 *framedBackground = nullptr;
    std::unordered_map<std::string, int> widgetIndexes;
    std::vector<LocalMenuWidgetInfo> registeredWidgets;
    std::vector<LocalMenuWidgetInfo *> createdWidgets;
    MenuWidgetManager() = default;

  private:
    // void ScrollArrows(const int step) noexcept;
    void RedrawWidgets(const INT32 firstWidgetIndex, H3BaseDlg *dlg) noexcept;

  public:
    INT RegisteredNumber() const noexcept;
    BOOL RegisterWidget(const mainmenu::MenuWidgetInfo &info);
    void CreateWidgets(H3BaseDlg *dlg, const mainmenu::eMenuList menuList);
    void DestroyWidgets(H3BaseDlg *dlg);
    void HandleEvent(H3Msg *msg);
    BOOL IsVisible() const noexcept;
    void SetVisible(const bool visible);

    // Новый API для доступа к виджетам по id
    H3DlgCaptionButton *GetWidgetByName(LPCSTR name) const;
    bool HasWidget(const std::string &name) const;
    int GetWidgetId(const std::string &name) const;
    BOOL SetWidgetText(const std::string &name, LPCSTR text);

  public:
    static MenuWidgetManager &Get();
    static VOID __fastcall DlgScroll_Proc(INT32 tickId, H3BaseDlg *dlg);
};

// Экспорт для других плагинов
extern "C" __declspec(dllexport) BOOL __stdcall MainMenu_RegisterWidget(const mainmenu::MenuWidgetInfo &info);
extern "C" __declspec(dllexport) H3DlgCaptionButton *__stdcall MainMenu_GetDialogButton(const char *name);
extern "C" __declspec(dllexport) int __stdcall MainMenu_GetDialogButtonId(const char *name);
extern "C" __declspec(dllexport) BOOL __stdcall MainMenu_SetDialogButtonText(const char *name, const char *text);
