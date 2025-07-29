#include "MenuWidgetManager.h"

void BaseGameWidgets::HideButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {
            auto &instance = MenuWidgetManager::Get();
            instance.SetVisible(instance.IsVisible() ^ true);
            msg->GetDlg()->Redraw();
        }
    }
}

void BaseGameWidgets::SystemOptionsButtonProc(void *_msg)
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

void BaseGameWidgets::WoGOptionsButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {
            const int storeValue = IntAt(0x291A430);
            IntAt(0x291A430) = 1;
            auto jumpOverMouseCheck = _PI->WriteJmp(0x07790FB, 0x0779157);
            THISCALL_0(int, 0x07790E1);
            jumpOverMouseCheck->Destroy();
            IntAt(0x291A430) = storeValue;
        }
    }
}

void __stdcall BaseGameWidgets::OnAfterReloadLanguageData(Era::TEvent *e)
{
    LPCSTR buttonNames[3] = {BaseGameWidgets::WIDGET_NAME_SYSTEM_OPTIONS, BaseGameWidgets::WIDGET_NAME_WOG_OPTIONS,
                             BaseGameWidgets::WIDGET_NAME_HIDE};
    LPCSTR buttonTexts[3] = {BaseGameWidgets::WIDGET_TEXT_SYSTEM_OPTIONS, BaseGameWidgets::WIDGET_TEXT_WOG_OPTIONS,
                             BaseGameWidgets::WIDGET_TEXT_HIDE};
    for (size_t i = 0; i < 3; i++)
    {
        MenuWidgetManager::Get().SetWidgetText(buttonNames[i], EraJS::read(buttonTexts[i]));
    }
}
void BaseGameWidgets::RegisterWidgets()
{
    using namespace mainmenu;
    int widgetsRegistred = 0;
    if (EraJS::read("era.api.main_menu.widgets.system_options.create"))
    {
        MenuWidgetInfo systemOptionWidgets{WIDGET_NAME_SYSTEM_OPTIONS, EraJS::read(WIDGET_TEXT_SYSTEM_OPTIONS),
                                           eMenuList::ALL, &SystemOptionsButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(systemOptionWidgets);
    }

    if (EraJS::read("era.api.main_menu.widgets.wog_options.create"))
    {
        MenuWidgetInfo wogOptionWidgets{WIDGET_NAME_WOG_OPTIONS, EraJS::read(WIDGET_TEXT_WOG_OPTIONS), eMenuList::ALL,
                                        &WoGOptionsButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(wogOptionWidgets);
    }
    if (EraJS::read("era.api.main_menu.widgets.hide.create"))
    {
        MenuWidgetInfo hideWidgets{WIDGET_NAME_HIDE, EraJS::read(WIDGET_TEXT_HIDE), eMenuList::ALL, &HideButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(hideWidgets);
    }

    if (widgetsRegistred)
    {
        Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
    }
}
