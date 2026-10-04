#include "../../headers/EraPluginsAPI/HelpDialogAPI.hpp"

// Call from your existing game UI click handler after ERA initialization.
// For TRUE, call while the right mouse button is held. FALSE opens an OK card.
// These functions do not open the full Help library or change its selection.
BOOL ShowCreatureHint(int creatureId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::Creatures, creatureId, rightClick);
}

BOOL ShowSpellHint(int spellId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::Spells, spellId, rightClick);
}

BOOL ShowHeroHint(int heroId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::Heroes, heroId, rightClick);
}

BOOL ShowArtifactHint(int artifactId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::Artifacts, artifactId, rightClick);
}

BOOL ShowSecondarySkillHint(int skillId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::SecondarySkills, skillId, rightClick);
}

BOOL ShowTownHint(int townId, BOOL rightClick)
{
    return era_help::ShowObjectHint(era_help::Page::Towns, townId, rightClick);
}
