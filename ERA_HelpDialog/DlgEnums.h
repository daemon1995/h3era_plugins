#pragma once
namespace main
{
    namespace buttons
    {
        enum eButton
        {
            NONE = -1,
            FIRST = 100,
            MODLIST = FIRST,
            HOTKEYS,
            CREATURES,
            ARTIFACTS,
            TOWNS,
            MAXIMIZE,
            HELP,
            LAST = HELP
        };
    } // namespace buttons
    namespace hkcategories
    {
        enum eType : int
        {
            ANY_DLG = -1,
            NONE,
            ADV_MAP_DLG,
            HERO_DLG,
            TOWN_DLG,
            COMBAT_DLG,
            MAIN_MENU_DLG,
            OTHER_DLG
        };

    } // namespace hkcategories

} // namespace main