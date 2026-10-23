#pragma once

class BaseGameWidgets
{
  public:
    static constexpr LPCSTR WIDGET_NAME_HIDE = "main_menu_api_widget_hide";
    static constexpr LPCSTR WIDGET_TEXT_HIDE = "era.main_menu.widgets.visibility_toggle.text";

    static constexpr LPCSTR WIDGET_NAME_SYSTEM_OPTIONS = "main_menu_api_widget_system_options";
    static constexpr LPCSTR WIDGET_NAME_WOG_OPTIONS = "main_menu_api_widget_wog_options";
    static constexpr LPCSTR WIDGET_TEXT_SYSTEM_OPTIONS = "era.main_menu.widgets.system_options.text";
    static constexpr LPCSTR WIDGET_TEXT_WOG_OPTIONS = "era.main_menu.widgets.wog_options.text";

  private:
    static int __fastcall HideButtonProc(void *_msg);
    static int __fastcall SystemOptionsButtonProc(void *_msg);
    static int __fastcall WoGOptionsButtonProc(void *_msg);

    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *e);

  public:
    static void RegisterWidgets();
};
