#pragma once
#if (defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || (!defined(_MSVC_LANG) && __cplusplus < 201703L)
#error JsonOverridesSchema.hpp requires C++17; include eraJson.hpp for the compatible reader API.
#endif
#include "eraJson.hpp"

// One list drives both adapters and export. Do not expose resource/owner pointers.
#define ERA_ARTIFACT_FIELDS(X) \
    X(name) X(cost) X(position) X(type) X(description) \
    X(comboArtifactId) X(partOfComboArtifactId) X(disabled) X(hasSpell)

#define ERA_HERO_DATA_FIELDS(X) \
    X(isFemale) X(race) X(heroClass) X(hasSpellbook) X(startingSpell) X(armyType) \
    X(smallPortrait) X(largePortrait) X(roeHero) X(expansionHero) X(campaignHero)

#define ERA_SPELL_FIELDS(X) \
    X(type) X(soundName) X(animationIndex) X(flags) X(name) X(shortName) X(level) X(school) \
    X(spEffect) X(manaCost) X(baseValue) X(chanceToGet) X(aiValue) X(description)

namespace EraJS
{
struct ArtifactSchema
{
#define ERA_ART_KEY(field) static constexpr LPCSTR field = "era.artifacts.%d." #field;
    ERA_ARTIFACT_FIELDS(ERA_ART_KEY)
#undef ERA_ART_KEY
    static constexpr LPCSTR event = "era.artifacts.%d.event";
};
inline bool ValidArtifactNumber(LPCSTR key, long long value, int artifactCount, int combinationCount)
{
    if (!key) return false;
    if (std::strcmp(key, ArtifactSchema::cost) == 0) return value >= 0;
    if (std::strcmp(key, ArtifactSchema::disabled) == 0 || std::strcmp(key, ArtifactSchema::hasSpell) == 0) return value == 0 || value == 1;
    if (std::strcmp(key, ArtifactSchema::type) == 0) return value >= 0 && (value & ~31LL) == 0;
    if (std::strcmp(key, ArtifactSchema::comboArtifactId) == 0) return value >= -1 && value < combinationCount;
    if (std::strcmp(key, ArtifactSchema::partOfComboArtifactId) == 0) return value >= -1 && value < artifactCount;
    if (std::strcmp(key, ArtifactSchema::position) == 0) return value >= 0 && value <= 14;
    return true;
}
template <typename T> inline void ReadArtifactField(T &field, LPCSTR key, int id, int artifactCount, int combinationCount)
{
    if constexpr (std::is_same_v<T, LPCSTR>) ReadField(field, key, id);
    else
    {
        T candidate = field;
        if (ReadField(candidate, key, id) && ValidArtifactNumber(key, static_cast<long long>(candidate), artifactCount, combinationCount))
            field = candidate;
    }
}
template <typename Artifact> inline void ReadArtifact(Artifact &artifact, int id, int artifactCount, int combinationCount = 12)
{
#define ERA_ART_READ(field) ReadArtifactField(artifact.field, ArtifactSchema::field, id, artifactCount, combinationCount);
    ERA_ARTIFACT_FIELDS(ERA_ART_READ)
#undef ERA_ART_READ
}
template <typename Artifact, typename Visitor> inline void VisitArtifact(const Artifact &artifact, Visitor visitor)
{
#define ERA_ART_VISIT(field) visitor(#field, artifact.field);
    ERA_ARTIFACT_FIELDS(ERA_ART_VISIT)
#undef ERA_ART_VISIT
}
template <typename Creature, typename Visitor> inline void VisitCreatureText(const Creature &creature, Visitor visitor)
{
    visitor("singular", creature.nameSingular);
    visitor("plural", creature.namePlural);
    visitor("description", creature.description);
}
template <typename Hero, typename Visitor> inline void VisitHeroData(const Hero &hero, Visitor visitor)
{
#define ERA_HERO_VISIT(field) visitor(#field, hero.field);
    ERA_HERO_DATA_FIELDS(ERA_HERO_VISIT)
#undef ERA_HERO_VISIT
}
template <typename Spell, typename Visitor> inline void VisitSpell(const Spell &spell, Visitor visitor)
{
#define ERA_SPELL_VISIT(field) visitor(#field, spell.field);
    ERA_SPELL_FIELDS(ERA_SPELL_VISIT)
#undef ERA_SPELL_VISIT
}
inline void ReadCreatureDescription(LPCSTR &target, int id)
{
    char key[80], alias[80];
    std::snprintf(key, sizeof(key), "era.monsters.%d.description", id);
    std::snprintf(alias, sizeof(alias), "era.monsters.%d.name.description", id);
    ReadValue(target, key, alias);
}
inline void ReadTownDwelling(LPCSTR &target, int town, int dwelling, bool description)
{
    char key[100], alias[100];
    const char *field = description ? "description" : "name";
    std::snprintf(key, sizeof(key), "era.towns.%d.buildings.%d.%s", town, 30 + dwelling, field);
    std::snprintf(alias, sizeof(alias), "era.towns.%d.dwellings.%d.%s", town, dwelling, field);
    ReadValue(target, key, alias);
}
} // namespace EraJS
