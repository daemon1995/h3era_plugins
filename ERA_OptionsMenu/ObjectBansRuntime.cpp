#include "ObjectBansRuntime.h"
#include "GameContext.h"
#include "OptionExport.h"
#include "OptionsRuntime.h"
#include "WoGOptionsBridge.h"
#include "pch.h"
#include <fstream>
#include <iterator>

namespace era_options
{
static_assert(sizeof(((H3Main *)nullptr)->heroOwner) == BanHeroCount, "Hero ban pool must match H3Main");
static_assert(sizeof(((H3Main *)nullptr)->bannedSkills) == BanSkillCount, "Skill ban pool must match H3Main");
namespace
{
ObjectBanRules preferences;
bool loaded = false, dirty = false;
std::string loadError;
constexpr const char *MapVersion = "era_options.bans.version";
std::string MapBanKey(BanObjectKind kind, int id)
{
    return std::string("era_options.bans.") + BanObjectKey(kind) + "." + std::to_string(id);
}
std::string MapSourceKey(int source)
{
    return std::string("era_options.bans.source.") + BanSourceKey(source);
}
int ActiveMapBan(BanObjectKind kind, int id)
{
    if (!ValidBanObjectId(kind, id) || Era::GetAssocVarIntValue(MapVersion) != 1)
        return 0;
    return Era::GetAssocVarIntValue(MapBanKey(kind, id).c_str()) != 0;
}
int ActiveMapSource(int source)
{
    if (source < 0 || source >= BanSourceCount ||
        Era::GetAssocVarIntValue(MapVersion) != 1)
        return 0;
    return Era::GetAssocVarIntValue(MapSourceKey(source).c_str()) != 0;
}
void ApplyActivePools()
{
    if (Era::GetAssocVarIntValue(MapVersion) != 1)
        return;
    const int count = (std::max)(0, (std::min)(32768, H3ArtifactCount::Get()));
    for (int id = 0; id < count; ++id)
        if (ActiveMapBan(BanObjectKind::Artifact, id))
            Era::ExecErmCmd(("UN:A" + std::to_string(id) + "/1;").c_str());
    if (ActiveMapSource(static_cast<int>(BanSpellSource::Guilds)))
        for (int id = 0; id < HeroSpellCount; ++id)
            if (ActiveMapBan(BanObjectKind::Spell, id))
                Era::ExecErmCmd(("UN:J0/" + std::to_string(id) + "/1;").c_str());
    auto *game = H3Main::Get();
    if (!game || !game->mainSetup.mapitems || game->mainSetup.mapSize <= 0)
        return;
    for (int id = 0; id < BanSkillCount; ++id)
        if (ActiveMapBan(BanObjectKind::SecondarySkill, id))
            game->bannedSkills[id] = 1;
    for (int id = 0; id < BanHeroCount; ++id)
        if (ActiveMapBan(BanObjectKind::Hero, id))
            game->heroMayBeHiredBy[id].Set(0);
    // Initial tavern offers can exist before ERM initialization. Replace only
    // selected offers, using the native setter to update both hero states.
    // Owned heroes and map prisoners never enter the replacement pool.
    for (int player = 0; player < 8; ++player)
        for (int side = 0; side < 2; ++side)
        {
            const int offer = side ? game->players[player].tavernHeroR : game->players[player].tavernHeroL;
            if (!ActiveMapBan(BanObjectKind::Hero, offer))
                continue;
            std::vector<int> pool;
            for (int id = 0; id < BanHeroCount; ++id)
            {
                const auto &hero = game->heroes[id];
                bool offered = false;
                for (const auto &other : game->players)
                    offered |= other.tavernHeroL == id || other.tavernHeroR == id;
                if (CanHireBanReplacement(game->heroOwner[id], game->heroMayBeHiredBy[id].Get(), player,
                                          hero.x >= 0 || hero.y >= 0 || hero.z >= 0, offered,
                                          ActiveMapBan(BanObjectKind::Hero, id)))
                    pool.push_back(id);
            }
            const int replacement =
                pool.empty() ? -1 : pool[H3Random::MultiplayerRandom(0, static_cast<int>(pool.size()) - 1)];
            Era::ExecErmCmd(("OW:V" + std::to_string(player) + "/" +
                             (side ? "d/" + std::to_string(replacement) : std::to_string(replacement) + "/d") + ";")
                                .c_str());
        }
}
void __stdcall OnNewMap(Era::TEvent *)
{
    try
    {
        // This event occurs only on new maps, after ERA resets associative vars.
        // Saved maps retain their selected object IDs in ERA's own map state.
        EnsureOptionsLoaded();
        ReloadObjectBanPreferences();
        if (!loaded || !loadError.empty())
        {
            if (!loadError.empty())
                OutputDebugStringA(("ERA_OptionsMenu bans: " + loadError).c_str());
            return;
        }
        // Persist the first import even if a map starts without opening the UI.
        std::string saveError;
        if (!SaveObjectBanPreferences(saveError))
            OutputDebugStringA(("ERA_OptionsMenu bans: " + saveError).c_str());
        Era::SetAssocVarIntValue(MapVersion, 0);
        Era::SetAssocVarIntValue("era_options.bans.sources_applied", 0);
        for (int source = 0; source < BanSourceCount; ++source)
            Era::SetAssocVarIntValue(MapSourceKey(source).c_str(), preferences.sources[source] ? 1 : 0);
        for (int index = 0; index < BanObjectKindCount; ++index)
        {
            const auto kind = static_cast<BanObjectKind>(index);
            for (const auto id : preferences.Ids(kind))
                Era::SetAssocVarIntValue(MapBanKey(kind, id).c_str(), 1);
        }
        if (!RetireLegacyObjectBansForNewMap())
        {
            OutputDebugStringA("ERA_OptionsMenu: cannot retire old ban selectors for this new map");
            return;
        }
        Era::SetAssocVarIntValue(MapVersion, 1);
        ApplyActivePools();
    }
    catch (const std::exception &error)
    {
        OutputDebugStringA((std::string("ERA_OptionsMenu bans: ") + error.what()).c_str());
    }
}
void __stdcall OnMapPools(Era::TEvent *)
{
    try
    {
        ApplyActivePools();
    }
    catch (const std::exception &error)
    {
        OutputDebugStringA(error.what());
    }
}
} // namespace

std::string ObjectBanPreferencesPath()
{
    const auto &path = ValuesFilePath();
    return path.substr(0, path.find_last_of("\\/") + 1) + "ERA_OptionsMenu.bans.json";
}
const std::string &ObjectBanLoadError()
{
    return loadError;
}
void ReloadObjectBanPreferences()
{
    if (dirty)
        return; // Keep uncommitted clicks until the parent menu saves.
    loadError.clear();
    const auto path = ObjectBanPreferencesPath();
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        if (!loaded)
        {
            preferences = ImportLegacyObjectBans([](int id) {
                const auto *option = Registry().Find(id);
                int value = 0;
                return option && ReadWogValue(*option, value) ? value : 0;
            });
            loaded = dirty = true;
        }
        return;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.good())
    {
        loadError = "cannot read " + path;
        return;
    }
    const std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ObjectBanRules rules;
    if (!ParseObjectBans(json, rules, loadError))
        return;
    preferences = std::move(rules);
    loaded = true;
}
const ObjectBanRules &ObjectBanPreferences()
{
    if (!loaded)
        ReloadObjectBanPreferences();
    return preferences;
}
ObjectBanRules CurrentMapObjectBans()
{
    ObjectBanRules rules;
    if (!IsOnMap())
        return rules;
    const auto *game = H3Main::Get();
    const int count = (std::max)(0, (std::min)(32768, H3ArtifactCount::Get()));
    for (int id = 0; id < count; ++id)
        if (P_ArtifactSetup[id].disabled || NativeBanFlag(game->artifactsAllowed, 144, id))
            rules.artifacts.insert(id);
    for (int id = 0; id < HeroSpellCount; ++id)
        if (NativeBanFlag(game->disabledSpells, HeroSpellCount, id))
            rules.spells.insert(id);
    for (int id = 0; id < BanSkillCount; ++id)
        if (NativeBanFlag(game->bannedSkills, BanSkillCount, id))
            rules.skills.insert(id);
    for (int id = 0; id < BanHeroCount; ++id)
    {
        bool offered = false;
        for (const auto &player : game->players)
            offered |= player.tavernHeroL == id || player.tavernHeroR == id;
        if (NativeHeroBanned(game->heroOwner[id], game->heroMayBeHiredBy[id].Get(), offered))
            rules.heroes.insert(id);
    }
    const bool managed = Era::GetAssocVarIntValue(MapVersion) == 1;
    for (int source = 0; source < BanSourceCount; ++source)
    {
        int raw = 0;
        rules.sources[source] = managed
                                    ? ActiveMapSource(source) != 0
                                    : ReadWogRawValue(LegacyBanSources()[source], raw, true) && (raw == 1 || raw == 3);
    }
    // Source replacements and other WoG generators read this map's frozen
    // selection; they have no shared native disabled-spell flag. Generic WoG
    // generators use it independently of the five source switches.
    if (managed)
        for (int id = 0; id < HeroSpellCount; ++id)
            if (ActiveMapBan(BanObjectKind::Spell, id))
                rules.spells.insert(id);
    return rules;
}
void SetObjectBan(BanObjectKind kind, int id, bool banned)
{
    if (!loaded || !loadError.empty() || preferences.IsBanned(kind, id) == banned)
        return;
    if (preferences.SetBanned(kind, id, banned))
        dirty = true;
}
void SetObjectBanSource(int source, bool enabled)
{
    if (!loaded || !loadError.empty() || source < 0 || source >= BanSourceCount ||
        preferences.sources[source] == enabled)
        return;
    preferences.sources[source] = enabled;
    dirty = true;
}
bool SaveObjectBanPreferences(std::string &error)
{
    if (!dirty)
        return true;
    try
    {
        if (!WriteOptionsJson(TextToUtf8(ObjectBanPreferencesPath(), GetACP()), SerializeObjectBans(preferences),
                              error))
            return false;
        dirty = false;
        return true;
    }
    catch (const std::exception &exception)
    {
        error = exception.what();
        return false;
    }
}

std::vector<BanObjectInfo> NativeBanObjects(BanObjectKind kind)
{
    std::vector<BanObjectInfo> objects;
    const int counts[] = {limits::TOTAL_SPELLS, (std::max)(0, (std::min)(32768, H3ArtifactCount::Get())), BanHeroCount,
                          BanSkillCount};
    const int index = static_cast<int>(kind);
    if (index < 0 || index >= BanObjectKindCount)
        return objects;
    const int count = counts[index];
    for (int id = 0; id < count; ++id)
    {
        BanObjectInfo entry;
        entry.id = id;
        if (kind == BanObjectKind::Spell)
        {
            const auto &info = P_Spell[id];
            entry.name = info.name ? info.name : "";
            // The UI follows HelpDialog's Fire/Air/Water/Earth order; native
            // school bits start with Air, so map them explicitly.
            const int schools[] = {NH3Spells::NSchool::FIRE, NH3Spells::NSchool::AIR, NH3Spells::NSchool::WATER,
                                   NH3Spells::NSchool::EARTH};
            for (int group = 0; group < 4; ++group)
                if (static_cast<int>(info.school) & schools[group])
                    entry.groupMask |= 1 << group;
            entry.selectable = id < HeroSpellCount && !info.creatureSpell;
            entry.level = entry.selectable ? info.level : 0;
            if (!entry.selectable)
                entry.groupMask |= 16;
            for (int mastery = 0; mastery < 4; ++mastery)
                if (info.description[mastery])
                    entry.description += std::string(info.description[mastery]) + "\n\n";
        }
        else if (kind == BanObjectKind::Artifact)
        {
            const auto &info = P_ArtifactSetup[id];
            entry.name = info.name ? info.name : "";
            entry.description = info.description ? info.description : "";
            const int type = static_cast<int>(info.type);
            entry.groupMask = (type & 30) >> 1;
            if ((type & 1) || !(type & 30))
                entry.groupMask |= 16;
        }
        else if (kind == BanObjectKind::Hero)
        {
            const auto &info = P_HeroInfo[id];
            entry.name = info.name ? info.name : "";
            // First field of the native 0x40-byte hero-class record is its town.
            if (info.heroClass >= 0 && info.heroClass < 18)
            {
                const int town = *reinterpret_cast<const int *>(0x67D868 + info.heroClass * 0x40);
                if (town >= 0 && town < 9)
                    entry.groupMask = 1 << town;
            }
        }
        else if (kind == BanObjectKind::SecondarySkill)
        {
            const auto &info = P_SecondarySkillInfo[id];
            entry.name = info.name ? info.name : "";
            for (int mastery = 0; mastery < 3; ++mastery)
                if (info.description[mastery])
                    entry.description += std::string(info.description[mastery]) + "\n\n";
        }
        if (entry.name.empty())
            entry.name = "#" + std::to_string(id);
        objects.push_back(std::move(entry));
    }
    return objects;
}

void RegisterObjectBanEvents()
{
    Era::RegisterHandler(OnNewMap, "OnBeforeErmInstructions");
    Era::RegisterHandler(OnMapPools, "OnAfterErmInited");
    Era::RegisterHandler(OnMapPools, "OnAfterLoadGame_Quit");
    Era::RegisterHandler(OnMapPools, "OnGameEnter");
}
bool SkipBannedConfluxSpell(int spellId)
{
    return ActiveMapSource(static_cast<int>(BanSpellSource::Guilds)) &&
           (ActiveMapBan(BanObjectKind::Spell, spellId) || spellId == TitansLightning);
}
} // namespace era_options

#pragma comment(linker, "/EXPORT:EraOptions_MapObjectBanned=_EraOptions_MapObjectBanned@8")
#pragma comment(linker, "/EXPORT:EraOptions_MapBanSource=_EraOptions_MapBanSource@4")
#pragma comment(linker, "/EXPORT:EraOptions_ReplacementSpell=_EraOptions_ReplacementSpell@4")
extern "C" int __stdcall EraOptions_MapObjectBanned(int kind, int id)
{
    try
    {
        if (kind < 0 || kind >= era_options::BanObjectKindCount)
            return 0;
        return era_options::ActiveMapBan(static_cast<era_options::BanObjectKind>(kind), id);
    }
    catch (...)
    {
        return 0;
    }
}
extern "C" int __stdcall EraOptions_MapBanSource(int source)
{
    try
    {
        return era_options::ActiveMapSource(source);
    }
    catch (...)
    {
        return 0;
    }
}
extern "C" int __stdcall EraOptions_ReplacementSpell(int level)
{
    try
    {
        if (Era::GetAssocVarIntValue("era_options.bans.version") != 1)
            return -1;
        if (level < 1 || level > 5)
            return -1;
        std::vector<int> pool;
        for (int id = 0; id < era_options::HeroSpellCount; ++id)
        {
            const auto &info = P_Spell[id];
            if (era_options::CanReplaceBannedSpell(id, info.level, level, info.creatureSpell != 0,
                                                   era_options::ActiveMapBan(era_options::BanObjectKind::Spell, id) !=
                                                       0,
                                                   P_Main->disabledSpells[id] != 0))
                pool.push_back(id);
        }
        return pool.empty() ? -1 : pool[H3Random::MultiplayerRandom(0, static_cast<int>(pool.size()) - 1)];
    }
    catch (...)
    {
        return -1;
    }
}
