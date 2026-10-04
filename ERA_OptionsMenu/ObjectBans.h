#pragma once

#include "EOption.h"
#include <array>
#include <set>
#include <functional>

namespace era_options
{
enum class BanObjectKind { Spell, Artifact, Hero, SecondarySkill, Count };
constexpr int BanObjectKindCount = static_cast<int>(BanObjectKind::Count);
enum class BanSpellSource { Guilds, Shrines, Scholars, ScrollsAndPandora, StartingHeroes, Count };
constexpr int BanSourceCount = static_cast<int>(BanSpellSource::Count);
constexpr int HeroSpellCount = 70;
constexpr int TitansLightning = 57;
constexpr int BanHeroCount = 156, BanSkillCount = 28;
const char *BanSourceKey(int source);
const char *BanObjectKey(BanObjectKind kind);
bool ValidBanObjectId(BanObjectKind kind, int id);
bool CanHireBanReplacement(int owner, unsigned hireMask, int player, bool placed, bool offered, bool banned);
template <typename Flag> inline bool NativeBanFlag(const Flag *flags, int count, int id)
{
    return flags && id >= 0 && id < count && flags[id] != 0;
}
bool NativeHeroBanned(int owner, unsigned hireMask, bool offeredInTavern);
bool CanReplaceBannedSpell(int id, int level, int requestedLevel, bool creatureSpell, bool banned, bool disabled);

struct ObjectBanRules
{
    std::set<int> spells, artifacts, heroes, skills;
    std::array<bool, BanSourceCount> sources = {{false, false, false, false, false}};
    bool IsBanned(BanObjectKind kind, int id) const;
    bool SetBanned(BanObjectKind kind, int id, bool banned);
    const std::set<int> &Ids(BanObjectKind kind) const;
    std::set<int> &Ids(BanObjectKind kind);
};

struct LegacyObjectBan { int option; BanObjectKind kind; int first; int last; };
const std::vector<LegacyObjectBan> &LegacyObjectBans();
const std::array<int, BanSourceCount> &LegacyBanSources();
ObjectBanRules ImportLegacyObjectBans(const std::function<int(int)> &readOption);
std::string SerializeObjectBans(const ObjectBanRules &rules);
bool ParseObjectBans(const std::string &json, ObjectBanRules &rules, std::string &error);

struct BanObjectInfo
{
    int id = -1;
    std::string name, description;
    int groupMask = 0;
    int level = 0;
    bool selectable = true;
};
struct BanObjectFilter { std::string query; int group = 0; int level = 0; int status = 0; int firstRow = 0; };
std::vector<int> FilterBanObjects(const std::vector<BanObjectInfo> &objects, const ObjectBanRules &rules,
    BanObjectKind kind, const BanObjectFilter &filter, unsigned codePage);
} // namespace era_options
