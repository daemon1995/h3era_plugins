#pragma once
#include "../headers/EraPluginsAPI/HelpDialogAPI.hpp"
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
    RESIZE_DLG,
    HELP,
    HEROES,
    SECONDARY_SKILLS,
    SPELLS,
    LAST = SPELLS,
    SEARCH = 110,
    SEARCH_CLEAR = 111
};
} // namespace buttons

// Public logical pages. Values intentionally match the corresponding header
// button IDs so integrations can persist or pass them without another map.
enum class eHelpPage : int
{
    MODS = static_cast<int>(era_help::Page::Mods),
    HOTKEYS = static_cast<int>(era_help::Page::Hotkeys),
    CREATURES = static_cast<int>(era_help::Page::Creatures),
    ARTIFACTS = static_cast<int>(era_help::Page::Artifacts),
    TOWNS = static_cast<int>(era_help::Page::Towns),
    HEROES = static_cast<int>(era_help::Page::Heroes),
    SECONDARY_SKILLS = static_cast<int>(era_help::Page::SecondarySkills),
    SPELLS = static_cast<int>(era_help::Page::Spells)
};

} // namespace main
