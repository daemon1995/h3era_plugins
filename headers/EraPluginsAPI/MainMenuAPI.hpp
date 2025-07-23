#pragma once
#include <functional>
#include <string>
#include <windows.h>
namespace mainmenu
{
class H3DlgCaptionButton;

enum eMenuList :INT
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
typedef BOOL(__stdcall *TRegisterMainMenuWidget)(const MenuWidgetInfo &info);
typedef H3DlgCaptionButton *(__stdcall *TGetMainMenuWidgetByName)(const char *name);
typedef int(__stdcall *TGetMainMenuWidgetId)(const char *name);

static constexpr LPCSTR _PLUGIN_NAME = "Interface_MainMenuAPI.era";
#define DECLARE_PLUGIN_FUNC(name, returnType, argTypes, argNames)                                                      \
    inline returnType name(argTypes)                                                                                   \
    {                                                                                                                  \
        HINSTANCE hApi = LoadLibraryA(_PLUGIN_NAME);                                                                   \
        if (auto func = reinterpret_cast<T##name>(GetProcAddress(hApi, #name)))                                        \
        {                                                                                                              \
            return func(argNames);                                                                                     \
        }                                                                                                              \
        return 0;                                                                                                      \
    }

// »спользование макроса дл€ объ€влени€ функций:
DECLARE_PLUGIN_FUNC(RegisterMainMenuWidget, BOOL, const MenuWidgetInfo &info, info)
DECLARE_PLUGIN_FUNC(GetMainMenuWidgetByName, H3DlgCaptionButton *, const char *name, name)
DECLARE_PLUGIN_FUNC(GetMainMenuWidgetId, int, const char *name, name)

} // namespace mainmenu
