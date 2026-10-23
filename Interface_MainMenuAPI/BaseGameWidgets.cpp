#include "MenuWidgetManager.h"

int __fastcall BaseGameWidgets::HideButtonProc(void *_msg)
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
    return true;
}

_LHF_(SystemOptionsDlgCtor)
{

    if (auto dlg = *reinterpret_cast<H3BaseDlg **>(c->ebp - 0x20))
    {

        constexpr int pcxWidth = 232;
        constexpr int pcxHeight = 203;
        auto dlgPcx = H3DlgPcx::Create(dlg->GetWidth() - pcxWidth - 14, dlg->GetHeight() - pcxHeight - 15, pcxWidth,
                                       pcxHeight, -1, NH3Dlg::Assets::DIBOXBACK);
        dlgPcx->DeActivate();
        dlg->AddItem(dlgPcx, false);
        c->return_address = 0x05B1DA0;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
int __fastcall BaseGameWidgets::SystemOptionsButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {

            auto skipButtonsCreation = _PI->WriteLoHook(0x05B1BED, SystemOptionsDlgCtor);
            char dlgMemory[104];
            STDCALL_0(VOID, 0x597AA0);             // stop video animation
            THISCALL_1(int, 0x05B1AA0, dlgMemory); // ctor

            THISCALL_1(int, 0x05B33C0, dlgMemory); // run
            THISCALL_1(int, 0x05B3350, dlgMemory); // dtor
            STDCALL_0(VOID, 0x597B50);             // resume video animation

            skipButtonsCreation->Destroy();
        }
    }
    return true;
}

int __fastcall BaseGameWidgets::WoGOptionsButtonProc(void *_msg)
{
    if (auto msg = static_cast<H3Msg *>(_msg))
    {
        if (msg->IsLeftClick())
        {
            const int storeValue = IntAt(0x291A430);
            IntAt(0x291A430) = 1;
            auto jumpOverMouseCheck = _PI->WriteJmp(0x07790FB, 0x0779157);
            STDCALL_0(VOID, 0x597AA0); // stop video animation
            THISCALL_0(int, 0x07790E1);
            STDCALL_0(VOID, 0x597B50); // resume video animation

            jumpOverMouseCheck->Destroy();
            IntAt(0x291A430) = storeValue;
        }
    }
    return true;
}

void __stdcall BaseGameWidgets::OnAfterReloadLanguageData(Era::TEvent *e)
{
    constexpr LPCSTR buttonNames[] = {BaseGameWidgets::WIDGET_NAME_SYSTEM_OPTIONS,
                                      BaseGameWidgets::WIDGET_NAME_WOG_OPTIONS, BaseGameWidgets::WIDGET_NAME_HIDE};
    constexpr LPCSTR buttonTexts[] = {BaseGameWidgets::WIDGET_TEXT_SYSTEM_OPTIONS,
                                      BaseGameWidgets::WIDGET_TEXT_WOG_OPTIONS, BaseGameWidgets::WIDGET_TEXT_HIDE};
    constexpr LPCSTR fallbackTexts[] = {"{System Options}", "{WoG Options}", "{Hide buttons menu}"};
    constexpr int size = std::size(buttonNames);
    for (size_t i = 0; i < size; i++)
    {
        MenuWidgetManager::Get().SetWidgetText(buttonNames[i], mainmenu::config::Text(buttonTexts[i], fallbackTexts[i]));
    }
}
void BaseGameWidgets::RegisterWidgets()
{
    using namespace mainmenu;
    int widgetsRegistred = 0;
    if (mainmenu::config::Enabled("era.main_menu.widgets.visibility_toggle.enabled", true))
    {
        const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::ALL | eMenuFlags::ON_TOP);

        MenuWidgetInfo hideWidgets{WIDGET_NAME_HIDE, config::Text(WIDGET_TEXT_HIDE, "{Hide buttons menu}"), flags, &HideButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(hideWidgets);
    }

    if (mainmenu::config::Enabled("era.main_menu.widgets.system_options.enabled", false))
    {
        MenuWidgetInfo systemOptionWidgets{WIDGET_NAME_SYSTEM_OPTIONS, config::Text(WIDGET_TEXT_SYSTEM_OPTIONS, "{System Options}"),
                                           eMenuFlags::ALL, &SystemOptionsButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(systemOptionWidgets);
    }
    if (mainmenu::config::Enabled("era.main_menu.widgets.wog_options.enabled", true))
    {
        MenuWidgetInfo wogOptionWidgets{WIDGET_NAME_WOG_OPTIONS, config::Text(WIDGET_TEXT_WOG_OPTIONS, "{WoG Options}"), eMenuFlags::ALL,
                                        &WoGOptionsButtonProc};
        widgetsRegistred += MenuWidgetManager::Get().RegisterWidget(wogOptionWidgets);
    }

    if (widgetsRegistred)
    {
        Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
    }
}
