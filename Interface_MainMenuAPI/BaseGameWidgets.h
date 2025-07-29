#pragma once

class BaseGameWidgets
{
  public:
    static constexpr LPCSTR WIDGET_NAME_HIDE = "main_menu_api_widget_hide";
    static constexpr LPCSTR WIDGET_TEXT_SHOW = "era.api.main_menu.widgets.hide.text_show";
    static constexpr LPCSTR WIDGET_TEXT_HIDE = "era.api.main_menu.widgets.hide.text_hide";

    static constexpr LPCSTR WIDGET_NAME_SYSTEM_OPTIONS = "main_menu_api_widget_system_options";
    static constexpr LPCSTR WIDGET_NAME_WOG_OPTIONS = "main_menu_api_widget_wog_options";
    static constexpr LPCSTR WIDGET_TEXT_SYSTEM_OPTIONS = "era.api.main_menu.widgets.system_options.text";
    static constexpr LPCSTR WIDGET_TEXT_WOG_OPTIONS = "era.api.main_menu.widgets.wog_options.text";

  private:
    static void HideButtonProc(void *_msg);

    static void SystemOptionsButtonProc(void *_msg);
    static void WoGOptionsButtonProc(void *_msg);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *e);

  public:
    static void RegisterWidgets();
};
