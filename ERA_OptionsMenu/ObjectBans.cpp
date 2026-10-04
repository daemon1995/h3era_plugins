#include "ObjectBans.h"
#include "OptionMenuModel.h"
#include "../headers/json.hpp"
#include <sstream>
#include <stdexcept>

namespace era_options
{
const char *BanSourceKey(int source)
{
    static const char *keys[] = {"guilds", "shrines", "scholars", "scrolls_and_pandora", "starting_heroes"};
    return source >= 0 && source < BanSourceCount ? keys[source] : "";
}

const char *BanObjectKey(BanObjectKind kind)
{
    static const char *keys[] = {"spell", "artifact", "hero", "skill"};
    const int index = static_cast<int>(kind);
    return index >= 0 && index < BanObjectKindCount ? keys[index] : "";
}
bool ValidBanObjectId(BanObjectKind kind, int id)
{
    const int limits[] = {HeroSpellCount, 32768, BanHeroCount, BanSkillCount};
    const int index = static_cast<int>(kind);
    return index >= 0 && index < BanObjectKindCount && id >= 0 && id < limits[index];
}
bool CanHireBanReplacement(int owner, unsigned hireMask, int player, bool placed, bool offered, bool banned)
{
    return player >= 0 && player < 8 && owner == -1 && (hireMask & (1u << player)) && !placed && !offered && !banned;
}
bool NativeHeroBanned(int owner, unsigned hireMask, bool offeredInTavern)
{
    // 0x40 also marks tavern offers; owned/offered heroes are not unavailable.
    if (offeredInTavern || (owner >= 0 && owner < 8)) return false;
    return owner == 0x40 || !(hireMask & 0xFF);
}
bool CanReplaceBannedSpell(int id, int level, int requestedLevel, bool creatureSpell, bool banned, bool disabled)
{
    return ValidBanObjectId(BanObjectKind::Spell, id) && id != TitansLightning &&
        requestedLevel >= 1 && requestedLevel <= 5 && level == requestedLevel &&
        !creatureSpell && !banned && !disabled;
}
const std::set<int> &ObjectBanRules::Ids(BanObjectKind kind) const
{
    switch (kind)
    {
    case BanObjectKind::Spell: return spells;
    case BanObjectKind::Artifact: return artifacts;
    case BanObjectKind::Hero: return heroes;
    case BanObjectKind::SecondarySkill: return skills;
    default: throw std::invalid_argument("unknown ban object kind");
    }
}
std::set<int> &ObjectBanRules::Ids(BanObjectKind kind)
{
    return const_cast<std::set<int> &>(static_cast<const ObjectBanRules &>(*this).Ids(kind));
}
bool ObjectBanRules::IsBanned(BanObjectKind kind, int id) const
{
    return ValidBanObjectId(kind, id) && Ids(kind).count(id) != 0;
}

bool ObjectBanRules::SetBanned(BanObjectKind kind, int id, bool banned)
{
    if (!ValidBanObjectId(kind, id)) return false;
    auto &ids = Ids(kind);
    if (banned) ids.insert(id); else ids.erase(id);
    return true;
}

const std::vector<LegacyObjectBan> &LegacyObjectBans()
{
    using K = BanObjectKind;
    // One-time import only. Runtime rules use object IDs, never these option IDs.
    static const std::vector<LegacyObjectBan> mappings = {
        {152,K::Spell,0,0}, {153,K::Spell,7,7}, {154,K::Spell,9,9}, {155,K::Spell,8,8},
        {156,K::Spell,6,6}, {221,K::Spell,1,1}, {222,K::Spell,2,2}, {223,K::Spell,26,26},
        {246,K::Spell,5,5}, {247,K::Spell,3,3}, {249,K::Spell,4,4},
        {157,K::Artifact,90,90}, {158,K::Artifact,72,72}, {159,K::Artifact,87,87},
        {160,K::Artifact,89,89}, {161,K::Artifact,86,86}, {162,K::Artifact,88,88},
        {163,K::Artifact,124,124}, {164,K::Artifact,123,123}, {166,K::Artifact,83,83},
        {167,K::Artifact,126,126}, {168,K::Artifact,93,93}, {172,K::Artifact,71,71},
        {175,K::Artifact,125,125}, {176,K::Artifact,141,141}, {177,K::Artifact,155,155},
        {197,K::Artifact,127,127}, {224,K::Artifact,128,128}, {226,K::Artifact,142,142},
        {227,K::Artifact,143,143}, {234,K::Artifact,156,156}, {236,K::Artifact,157,157},
        {238,K::Artifact,146,155}, {241,K::Artifact,159,159}, {243,K::Artifact,160,160}
    };
    return mappings;
}

const std::array<int, BanSourceCount> &LegacyBanSources()
{
    static const std::array<int, BanSourceCount> ids = {{146,147,148,150,151}};
    return ids;
}

ObjectBanRules ImportLegacyObjectBans(const std::function<int(int)> &readOption)
{
    ObjectBanRules rules;
    for (const auto &entry : LegacyObjectBans())
        if (readOption(entry.option) == 1)
            for (int id = entry.first; id <= entry.last; ++id) rules.SetBanned(entry.kind, id, true);
    for (int source = 0; source < BanSourceCount; ++source) rules.sources[source] = readOption(LegacyBanSources()[source]) == 1;
    return rules;
}

std::string SerializeObjectBans(const ObjectBanRules &rules)
{
    nlohmann::json document = {{"version",1}, {"spells",rules.spells}, {"artifacts",rules.artifacts},
        {"heroes",rules.heroes}, {"skills",rules.skills}, {"sources",nlohmann::json::object()}};
    for (int source = 0; source < BanSourceCount; ++source) document["sources"][BanSourceKey(source)] = rules.sources[source];
    return document.dump(2) + "\n";
}

bool ParseObjectBans(const std::string &json, ObjectBanRules &rules, std::string &error)
{
    error.clear();
    try
    {
        const auto document = nlohmann::json::parse(json);
        if (!document.is_object() || !document.contains("version") || document.at("version") != 1)
            throw std::runtime_error("object bans: expected version 1");
        ObjectBanRules parsed;
        const char *keys[] = {"spells", "artifacts", "heroes", "skills"};
        for (int index = 0; index < BanObjectKindCount; ++index)
        {
            const auto kind = static_cast<BanObjectKind>(index);
            const char *key = keys[index];
            // Existing version-1 files predate hero and secondary-skill bans.
            if (index >= 2 && !document.contains(key)) continue;
            if (!document.contains(key) || !document.at(key).is_array()) throw std::runtime_error(std::string(key) + ": expected an array of object IDs");
            for (const auto &entry : document.at(key))
            {
                int id = -1;
                if (!(entry.is_string() || entry.is_number_integer()) ||
                    !ParseInteger(entry.is_string() ? entry.get<std::string>() : entry.dump(), id) || !parsed.SetBanned(kind, id, true))
                    throw std::runtime_error(std::string(key) + ": invalid object ID");
            }
        }
        if (!document.contains("sources") || !document.at("sources").is_object()) throw std::runtime_error("sources: expected an object");
        for (int source = 0; source < BanSourceCount; ++source)
        {
            const auto &sources = document.at("sources");
            const auto key = BanSourceKey(source);
            if (!sources.contains(key)) continue;
            const auto &entry = sources.at(key);
            if (!(entry.is_string() || entry.is_boolean()) ||
                !ParseBoolean(entry.is_string() ? entry.get<std::string>() : entry.dump(), parsed.sources[source]))
                throw std::runtime_error(std::string(key) + ": expected true/false");
        }
        rules = std::move(parsed);
        return true;
    }
    catch (const std::exception &exception) { error = exception.what(); return false; }
}

std::vector<int> FilterBanObjects(const std::vector<BanObjectInfo> &objects, const ObjectBanRules &rules,
    BanObjectKind kind, const BanObjectFilter &filter, unsigned codePage)
{
    const auto query = NormalizeOptionSearch(filter.query, codePage);
    std::vector<int> result;
    for (size_t index = 0; index < objects.size(); ++index)
    {
        const auto &entry = objects[index];
        if (filter.group && (filter.group < 1 || filter.group > 9 || !(entry.groupMask & (1 << (filter.group - 1))))) continue;
        if (filter.level && entry.level != filter.level) continue;
        if (filter.status && rules.IsBanned(kind, entry.id) != (filter.status == 2)) continue;
        auto number = query;
        if (!number.empty() && number.front() == L'#') number.erase(0, 1);
        if (!number.empty() && number.find_first_not_of(L"0123456789") == std::wstring::npos)
        {
            int id;
            if (!ParseInteger(std::string(number.begin(), number.end()), id) || id != entry.id) continue;
        }
        else
        {
            const auto name = NormalizeOptionSearch(entry.name, codePage);
            std::wistringstream words(query);
            std::wstring word;
            bool matches = true;
            while (words >> word) if (name.find(word) == std::wstring::npos) { matches = false; break; }
            if (!matches) continue;
        }
        result.push_back(static_cast<int>(index));
    }
    return result;
}
} // namespace era_options
