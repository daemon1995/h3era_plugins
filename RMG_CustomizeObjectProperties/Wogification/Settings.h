#pragma once
#include <array>
#include <string>
#include <vector>

namespace wogification
{
enum Kind { Target, Source };
// Targets may inherit weight/limit with -1. Source chance/preserve are nonnegative.
enum Field { Enabled, Rate, Limit, Distance, Levels, MapKinds, FieldCount };
using Values = std::array<int, FieldCount>;
struct Object
{
    std::string id, name, provider, category, initializer, requiresPlugin, requiresVariable;
    int type = -1, subtype = -1;
    bool target = false, source = false;
    std::array<Values, 2> defaults, current;
    std::vector<int> groups, sourceGroups, targetGroups;
};
enum ProcessField { CreatureEnabled, CreatureLevels, MaxAggression, QuestCreatures,
    ArtifactEnabled, ArtifactLevels, BankArtifacts, BankBanners, ScrollEnabled,
    ScrollLevel1, ScrollLevel2, ScrollLevel3, ScrollLevel4, ScrollLevel5, ProcessMaps, ProcessFieldCount };
using ProcessValues = std::array<int,ProcessFieldCount>;
struct Draft : std::vector<std::array<Values,2>>
{
    ProcessValues process{};
    void swap(Draft &other) noexcept { std::vector<std::array<Values,2>>::swap(other); process.swap(other.process); }
};
ProcessValues ProcessDefaults();
bool ValidProcess(const ProcessValues &values);
const std::vector<Object> &Objects();
bool Participates(const Object &object, Kind kind);
bool Pickup(int type);
Draft Current();
Draft Defaults();
bool Valid(const Values &values, Kind kind);
bool ParseInteger(const char *text, int &value);
bool SaveCurrent(const Draft &draft);
bool SaveProfile(const std::string &name, const Draft &draft);
bool LoadProfile(const std::string &name, Draft &draft, std::string &error);
// Absolute/relative INI paths selected by the native profile chooser.
bool SaveProfileFile(const std::string &path, const Draft &draft);
bool LoadProfileFile(const std::string &path, Draft &draft, std::string &error);
std::string ProfileDirectory();
std::vector<std::string> Profiles();
bool ValidProfileName(const std::string &name);
void Install();
void ShowDialog();
const char *Text(const char *key, const char *fallback);
}
