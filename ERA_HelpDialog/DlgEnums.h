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
    RESIZE_DLG,
    HELP,
    HEROES,
    SECONDARY_SKILLS,
    SPELLS,
    LAST = SPELLS
};
} // namespace buttons

// Public logical pages. Values intentionally match the corresponding header
// button IDs so integrations can persist or pass them without another map.
enum class eHelpPage : int
{
    MODS = buttons::MODLIST,
    HOTKEYS = buttons::HOTKEYS,
    CREATURES = buttons::CREATURES,
    ARTIFACTS = buttons::ARTIFACTS,
    TOWNS = buttons::TOWNS,
    HEROES = buttons::HEROES,
    SECONDARY_SKILLS = buttons::SECONDARY_SKILLS,
    SPELLS = buttons::SPELLS
};

} // namespace main
