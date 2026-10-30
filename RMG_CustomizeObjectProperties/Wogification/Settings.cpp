#include "../pch.h"

#include "../AtomicIniSave.h"
#include "../RMGObjectsEditor.h"
#include "Settings.h"
#include <climits>
#include <map>
#include <set>

namespace wogification
{
namespace
{
constexpr const char *ROOT = "wogify.objectReplacement.";
constexpr const char *INI = "Runtime/Wogification.ini";
constexpr const char *PROFILE_DIR = "Runtime/WogificationProfiles";
constexpr const char *FIELDS[2][FieldCount] = {{"enabled", "weight", "mapLimit", "minDistance", "levels", "mapKinds"},
                                               {"enabled", "chance", "preserve", "group", "levels", "mapKinds"}};
std::vector<Object> objects;
std::map<std::pair<int, int>, size_t> byPair;
std::map<int, std::vector<size_t>> additions;
std::map<int, int> originalTargets;
bool loaded = false;
bool running = false;
bool randomMap = false;
H3MainSetup *runMap = nullptr;
struct Site
{
    int x, y, z, type, subtype;
};
std::vector<Site> sites;
std::map<int, std::pair<int, int>> originalSources;
std::set<int> consumed;
std::map<int, bool> sourceRolls;
int mapSize = 0;
int pickupType = -1;
bool pickupRandom = false;
std::map<int, int> pickupCounts;
constexpr const char *PROCESS_FIELDS[ProcessFieldCount] = {
    "creatures.enabled",        "creatures.levels",      "creatures.maxAggression",
    "creatures.questCreatures", "artifacts.enabled",     "artifacts.levels",
    "artifacts.bankRewards",    "artifacts.bankBanners", "scrolls.enabled",
    "scrolls.level1",           "scrolls.level2",        "scrolls.level3",
    "scrolls.level4",           "scrolls.level5",        "mapKinds"};
ProcessValues processDefaults{}, processCurrent{};
const char *FieldName(const Object &object, int kind, int field)
{
    return object.type == eObject::MONSTER && kind == Target && field == Rate ? "chance" : FIELDS[kind][field];
}

std::string Root(const std::string &key)
{
    return std::string(ROOT) + key;
}
int Read(const std::string &key, int fallback)
{
    bool found = false;
    const int value = EraJS::readInt(key, found);
    return found ? value : fallback;
}
std::string String(const std::string &key, const std::string &fallback = "")
{
    bool found = false;
    const char *value = EraJS::read(key, found);
    return found && value && *value ? value : fallback;
}
bool Identity(const std::string &id)
{
    if (id.empty() || id.size() > 96)
        return false;
    for (unsigned char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    return true;
}
std::string Fold(std::string text)
{
    if (!text.empty())
        CharLowerBuffA(&text[0], static_cast<DWORD>(text.size()));
    return text;
}
bool ProtectedObject(int type, int subtype)
{
    switch (type)
    {
    case eObject::PRISON:
    case eObject::SEER_HUT:
    case eObject::QUEST_GUARD:
        return true;
    }
    // A wildcard source can cover both ordinary and quest subtypes. Check
    // registered subtypes individually when their actual map instance is used.
    return subtype >= 0 &&
           extendersManager::ObjectExtenderManager::GetObjectPage(type, subtype) == extender::ePagePrison;
}
size_t Add(int type, int subtype)
{
    const auto key = std::make_pair(type, subtype);
    const auto old = byPair.find(key);
    if (old != byPair.end())
        return old->second;
    Object object;
    object.type = type;
    object.subtype = subtype;
    object.id = "object_" + std::to_string(type) + "_" + (subtype < 0 ? "any" : std::to_string(subtype));
    const char *name = RMGObjectInfo::GetObjectName(type, subtype < 0 ? 0 : subtype);
    object.name = name && *name ? name : object.id;
    if (subtype < 0)
        object.name += " (*)";
    object.provider = "WoG";
    object.defaults[Target] = {{1, -1, -1, 0, 3, 1}};
    object.defaults[Source] = {{1, 0, 0, -1, 3, 1}};
    object.current = object.defaults;
    byPair[key] = objects.size();
    objects.push_back(object);
    return objects.size() - 1;
}
bool PickupId(int type, int id)
{
    if (type == eObject::ARTIFACT)
        return id >= eArtifact::CENTAUR_AXE && id < IntAt(0x717020);
    if (type != eObject::MONSTER || id < 0 || id >= IntAt(0x4A1657) ||
        (id >= eCreature::CATAPULT && id <= eCreature::ARROW_TOWER))
        return false;
    return Era::IsCommanderId && !Era::IsCommanderId(id);
}
void LoadPickups()
{
    for (int type : {int(eObject::ARTIFACT), int(eObject::MONSTER)})
    {
        const int count = std::max(0, std::min(32768, IntAt(type == eObject::ARTIFACT ? 0x717020 : 0x4A1657)));
        const auto section = Root("pickups.") + (type == eObject::ARTIFACT ? "artifacts." : "creatures.");
        const int last = Read(section + "sourceLastId",
                              type == eObject::ARTIFACT ? int(eArtifact::CORNUCOPIA) : int(eCreature::TROLL));
        for (int id = 0; id < count; ++id)
        {
            if (!PickupId(type, id))
                continue;
            const char *name = RMGObjectInfo::GetObjectName(type, id);
            if (!name || !*name || (type == eObject::MONSTER && P_Creatures[id].aiValue <= 0))
                continue;
            auto &object = objects[Add(type, id)];
            object.source = id <= last;
            object.target = id > last;
            object.defaults[Source] = {{1, 100, 0, -1, 3, 1}};
            object.defaults[Target] = {{1, -1, -1, 0, 3, 1}};
            object.category = type == eObject::ARTIFACT ? Text("artifacts") : Text("creatures");
            object.provider = id <= last ? "SoD" : "Mods";
            const auto base = section + std::to_string(id);
            for (int k = 0; k < 2; ++k)
            {
                const auto role = base + (k == Target ? ".target." : ".source.");
                if (Read(role + "available", k == Target ? object.target : object.source))
                {
                    if (k == Target)
                        object.target = true;
                    else
                        object.source = true;
                }
                auto values = object.defaults[k];
                for (int f = 0; f < FieldCount; ++f)
                    values[f] = Read(role + FieldName(object, k, f), values[f]);
                if (Valid(values, static_cast<Kind>(k)))
                    object.defaults[k] = values;
            }
        }
    }
}
std::string Section(const Object &object, Kind kind)
{
    return std::string(kind == Target ? "target:" : "source:") + object.id;
}

bool ReadIni(const char *path, Draft &draft, bool requireVersion, std::string &error)
{
    char buffer[512]{};
    if (requireVersion &&
        (!Era::ReadStrFromIni("schemaVersion", "Wogification", path, buffer) || std::strcmp(buffer, "1")))
    {
        error = Text("profileVersion");
        return false;
    }
    for (size_t i = 0; i < objects.size(); ++i)
        for (int k = 0; k < 2; ++k)
        {
            if (Pickup(objects[i].type))
                continue; // Superseded by the process pages.
            const auto section = Section(objects[i], static_cast<Kind>(k));
            for (const auto &binding :
                 {std::make_pair("type", objects[i].type), std::make_pair("subtype", objects[i].subtype)})
                if (Era::ReadStrFromIni(binding.first, section.c_str(), path, buffer))
                {
                    int value;
                    if (!ParseInteger(buffer, value) || value != binding.second)
                    {
                        error = section + ": " + Text("profileIdentityChanged");
                        return false;
                    }
                }
            for (int f = 0; f < FieldCount; ++f)
                if (Era::ReadStrFromIni(FieldName(objects[i], k, f), section.c_str(), path, buffer))
                {
                    int value;
                    if (!ParseInteger(buffer, value))
                    {
                        error = section + ": " + FIELDS[k][f];
                        return false;
                    }
                    draft[i][k][f] = value;
                }
            // Old profiles used -1 for inherited source values. Resolve on input;
            // neither the draft nor the next saved file retains negative values.
            if (k == Source)
                for (int field : {Rate, Limit})
                    if (draft[i][k][field] == -1)
                        draft[i][k][field] = objects[i].defaults[k][field];
            if (!Valid(draft[i][k], static_cast<Kind>(k)))
            {
                error = section;
                return false;
            }
            if (k == Source && draft[i][k][Distance] >= 0 &&
                std::find(objects[i].sourceGroups.begin(), objects[i].sourceGroups.end(), draft[i][k][Distance]) ==
                    objects[i].sourceGroups.end())
            {
                error = section + ": " + Text("group");
                return false;
            }
        }
    for (int f = 0; f < ProcessFieldCount; ++f)
        if (Era::ReadStrFromIni(PROCESS_FIELDS[f], "Process", path, buffer) && !ParseInteger(buffer, draft.process[f]))
        {
            error = PROCESS_FIELDS[f];
            return false;
        }
    if (!ValidProcess(draft.process))
    {
        error = Text("invalidProcess");
        return false;
    }
    return true;
}
bool Save(const char *path, const Draft &draft)
{
    if (draft.size() != objects.size() || !ValidProcess(draft.process))
        return false;
    for (size_t i = 0; i < draft.size(); ++i)
    {
        if (!Valid(draft[i][Target], Target) || !Valid(draft[i][Source], Source))
            return false;
        const int group = draft[i][Source][Distance];
        if (group >= 0 && std::find(objects[i].sourceGroups.begin(), objects[i].sourceGroups.end(), group) ==
                              objects[i].sourceGroups.end())
            return false;
    }
    return rmgsettings::SaveIniAtomically(path, [&draft](const char *stage) {
        bool ok = Era::WriteStrToIni("schemaVersion", "1", "Wogification", stage) != FALSE;
        for (size_t i = 0; i < objects.size(); ++i)
            for (int k = 0; k < 2; ++k)
            {
                if (Pickup(objects[i].type))
                    continue;
                const auto section = Section(objects[i], static_cast<Kind>(k));
                bool changed = false;
                for (int f = 0; f < FieldCount; ++f)
                    changed = changed || draft[i][k][f] != objects[i].defaults[k][f];
                if (changed)
                {
                    if (!Era::WriteStrToIni("type", std::to_string(objects[i].type).c_str(), section.c_str(), stage))
                        ok = false;
                    if (!Era::WriteStrToIni("subtype", std::to_string(objects[i].subtype).c_str(), section.c_str(),
                                            stage))
                        ok = false;
                }
                for (int f = 0; f < FieldCount; ++f)
                    if (draft[i][k][f] != objects[i].defaults[k][f])
                        if (!Era::WriteStrToIni(FieldName(objects[i], k, f), std::to_string(draft[i][k][f]).c_str(),
                                                section.c_str(), stage))
                            ok = false;
            }
        for (int f = 0; f < ProcessFieldCount; ++f)
            if (draft.process[f] != processDefaults[f] &&
                !Era::WriteStrToIni(PROCESS_FIELDS[f], std::to_string(draft.process[f]).c_str(), "Process", stage))
                ok = false;
        return ok;
    });
}
void Load()
{
    if (loaded)
        return;
    // Preserve legacy rule conditions; the editor adds a user-settings layer.
    std::vector<int> groups;
    for (int i = 0; i < 4096; ++i)
    {
        const int group = Read(Root("objects.order." + std::to_string(i)), -1);
        if (group < 0)
            break;
        if (std::find(groups.begin(), groups.end(), group) != groups.end())
            continue;
        groups.push_back(group);
        const std::string base = Root("objects." + std::to_string(group));
        for (int kind = 0; kind < 2; ++kind)
            for (int n = 0; n < 4096; ++n)
            {
                const std::string entry = base + (kind == Target ? ".targets." : ".sources.") + std::to_string(n);
                const int type = Read(entry + ".type", -1), subtype = Read(entry + ".subtype", -1);
                if (type < 0)
                {
                    if (kind == Target)
                        originalTargets[group] = n;
                    break;
                }
                if (type > 231 || subtype < (kind == Target ? 0 : -1) || subtype > 32767)
                    continue;
                auto &object = objects[Add(type, subtype)];
                if (kind == Target)
                    object.target = true;
                else
                {
                    if (!object.source)
                        object.defaults[Source][Rate] = std::max(0, std::min(100, Read(entry + ".chance", 0)));
                    object.defaults[Source][Limit] = std::max(
                        object.defaults[Source][Limit], std::max(0, std::min(65535, Read(base + ".preserveLast", 1))));
                    object.source = true;
                }
                auto &roleGroups = kind == Target ? object.targetGroups : object.sourceGroups;
                if (std::find(roleGroups.begin(), roleGroups.end(), group) == roleGroups.end())
                    roleGroups.push_back(group);
                if (std::find(object.groups.begin(), object.groups.end(), group) == object.groups.end())
                    object.groups.push_back(group);
            }
    }
    std::set<std::string> identities;
    for (const auto &object : objects)
        identities.insert(Fold(object.id));
    std::set<std::pair<int, int>> catalogPairs;
    const bool knownCatalog = Read(Root("catalog.schemaVersion"), 1) == 1;
    if (!knownCatalog)
        Era::WriteLog("Wogification", "Catalog", "Unsupported catalog version; legacy rules retained.");
    for (int i = 0; knownCatalog && i < 8192; ++i)
    {
        const auto id = String(Root("catalog.order." + std::to_string(i)));
        if (id.empty())
            break;
        if (!Identity(id))
            continue;
        const std::string base = Root("catalog.objects." + id);
        const int type = Read(base + ".type", -1), subtype = Read(base + ".subtype", -2);
        if (type < 0 || type > 231 || subtype < -1 || subtype > 32767 || !catalogPairs.insert({type, subtype}).second)
            continue;
        auto &object = objects[Add(type, subtype)];
        if (Fold(id) != Fold(object.id) && identities.count(Fold(id)))
            continue;
        identities.erase(Fold(object.id));
        identities.insert(Fold(id));
        object.id = id;
        object.name = String(base + ".name", object.name);
        object.provider = String(base + ".provider", object.provider);
        object.category = String(base + ".category", "");
        object.initializer = String(base + ".initializer");
        object.requiresPlugin = String(base + ".requiresPlugin");
        object.requiresVariable = String(base + ".requiresVariable");
        if (!Identity(object.initializer))
            object.initializer.clear();
        for (int k = 0; k < 2; ++k)
        {
            const std::string role = base + (k == Target ? ".target." : ".source.");
            if (Read(role + "available", k == Target ? object.target : object.source))
            {
                if (k == Target && subtype >= 0)
                    object.target = true;
                else if (k == Source)
                    object.source = true;
            }
            auto values = object.defaults[k];
            for (int f = 0; f < FieldCount; ++f)
                values[f] = Read(role + FIELDS[k][f], values[f]);
            if (k == Source)
                for (int field : {Rate, Limit})
                    if (values[field] == -1)
                        values[field] = object.defaults[k][field];
            if (Valid(values, static_cast<Kind>(k)))
                object.defaults[k] = values;
        }
        // A mod can join existing semantic pools without duplicating their source lists.
        if (object.target)
            for (int g = 0; g < 4096; ++g)
            {
                const int group = Read(base + ".targetGroups." + std::to_string(g), -1);
                if (group < 0)
                    break;
                if (!originalTargets.count(group) || std::find(object.targetGroups.begin(), object.targetGroups.end(),
                                                               group) != object.targetGroups.end())
                    continue;
                additions[group].push_back(byPair[{type, subtype}]);
                object.targetGroups.push_back(group);
                if (std::find(object.groups.begin(), object.groups.end(), group) == object.groups.end())
                    object.groups.push_back(group);
            }
    }
    // Expose every new identity, including additions without an RMG generator
    // or a wogification catalog entry. Unassigned targets remain opt-in/inert.
    for (const auto &identity : extendersManager::ObjectExtenderManager::GetAddedObjects())
    {
        const bool known = byPair.count(identity) != 0;
        auto &object = objects[Add(identity.first, identity.second)];
        if (!object.target)
        {
            object.target = true;
            object.defaults[Target][Enabled] = 0;
            if (!known)
                object.provider = "Mods";
        }
    }
    LoadPickups();
    processDefaults = {{1, 255, 0, 0, 1, 31, 0, 1, 1, 1, 1, 0, 0, 0, 1}};
    auto configured = processDefaults;
    for (int f = 0; f < ProcessFieldCount; ++f)
        configured[f] = Read("wogify.process." + std::string(PROCESS_FIELDS[f]), configured[f]);
    if (ValidProcess(configured))
        processDefaults = configured;
    processCurrent = processDefaults;
    for (auto &object : objects)
        object.current = object.defaults;
    loaded = true;
    auto draft = Defaults();
    std::string error;
    if (ReadIni(INI, draft, false, error))
    {
        for (size_t i = 0; i < objects.size(); ++i)
            objects[i].current = draft[i];
        processCurrent = draft.process;
    }
    else
        Era::WriteLog("Wogification", "Settings", ("Invalid INI; defaults used: " + error).c_str());
}
const Object *Find(int type, int subtype, Kind kind)
{
    auto it = byPair.find({type, subtype});
    if (kind == Source && (it == byPair.end() || !Participates(objects[it->second], kind)))
        it = byPair.find({type, -1});
    return it == byPair.end() || !Participates(objects[it->second], kind) ? nullptr : &objects[it->second];
}
bool Available(const Object &object)
{
    return (object.requiresPlugin.empty() || GetModuleHandleA(object.requiresPlugin.c_str())) &&
           (object.requiresVariable.empty() || Era::GetAssocVarIntValue(object.requiresVariable.c_str()));
}
int Position(int x, int y, int z)
{
    return (z * mapSize + y) * mapSize + x;
}
bool Active(int x, int y, int z)
{
    return running && runMap == &P_Game->mainSetup && mapSize == runMap->mapSize && x >= 0 && y >= 0 && x < mapSize &&
           y < mapSize && z >= 0 && z <= (runMap->hasUnderground ? 1 : 0);
}
bool Scope(const Values &v, int z)
{
    return (v[Levels] & (1 << z)) && (v[MapKinds] & (randomMap ? 2 : 1));
}
} // namespace

const char *Text(const char *key)
{
    return EraJS::read("wogify.dialog." + std::string(key));
}
bool ParseInteger(const char *text, int &value)
{
    return EraJS::ParseNumber(text, value);
}
bool Valid(const Values &v, Kind kind)
{
    return v[Enabled] >= 0 && v[Enabled] <= 1 && v[Rate] >= (kind == Target ? -1 : 0) &&
           v[Rate] <= (kind == Target ? 100000 : 100) && v[Limit] >= (kind == Target ? -1 : 0) && v[Limit] <= 65535 &&
           v[Distance] >= (kind == Target ? 0 : -1) && v[Distance] <= (kind == Target ? 255 : 1000000) &&
           v[Levels] >= 1 && v[Levels] <= 3 && v[MapKinds] >= 1 && v[MapKinds] <= 3;
}
const std::vector<Object> &Objects()
{
    Load();
    return objects;
}
bool Participates(const Object &object, Kind kind)
{
    if (Pickup(object.type))
        return PickupId(object.type, object.subtype) && (kind == Target ? object.target : object.source);
    if (ProtectedObject(object.type, object.subtype))
        return false;
    return kind == Target
               ? object.target && extendersManager::ObjectExtenderManager::IsAddedObject(object.type, object.subtype)
               : object.source &&
                     extendersManager::ObjectExtenderManager::IsOriginalObject(object.type, object.subtype);
}
bool Pickup(int type)
{
    return type == eObject::ARTIFACT || type == eObject::MONSTER;
}
ProcessValues ProcessDefaults()
{
    Load();
    return processDefaults;
}
bool ValidProcess(const ProcessValues &v)
{
    for (int f = 0; f < ProcessFieldCount; ++f)
    {
        const int maximum = f == CreatureLevels                      ? 255
                            : f == ArtifactLevels                    ? 31
                            : f == ProcessMaps                       ? 3
                            : f >= ScrollLevel1 && f <= ScrollLevel5 ? 3
                                                                     : 1;
        if (v[f] < 0 || v[f] > maximum || (f == ProcessMaps && v[f] == 0))
            return false;
    }
    return true;
}
Draft Current()
{
    Load();
    Draft result;
    result.process = processCurrent;
    for (const auto &object : objects)
        result.push_back(object.current);
    return result;
}
Draft Defaults()
{
    Load();
    Draft result;
    result.process = processDefaults;
    for (const auto &object : objects)
        result.push_back(object.defaults);
    return result;
}
bool SaveCurrent(const Draft &draft)
{
    Load();
    if (running || !Save(INI, draft))
        return false;
    for (size_t i = 0; i < objects.size(); ++i)
        objects[i].current = draft[i];
    processCurrent = draft.process;
    return true;
}
bool ValidProfileName(const std::string &name)
{
    if (!Identity(name) || name.size() > 64)
        return false;
    std::string upper(name);
    CharUpperBuffA(&upper[0], static_cast<DWORD>(upper.size()));
    return upper != "CON" && upper != "PRN" && upper != "AUX" && upper != "NUL" &&
           !(upper.size() == 4 && (upper.substr(0, 3) == "COM" || upper.substr(0, 3) == "LPT") && upper[3] >= '0' &&
             upper[3] <= '9');
}
bool SaveProfile(const std::string &name, const Draft &draft)
{
    Load();
    if (!ValidProfileName(name))
        return false;
    return SaveProfileFile(std::string(PROFILE_DIR) + "/" + name + ".ini", draft);
}
bool LoadProfile(const std::string &name, Draft &draft, std::string &error)
{
    if (!ValidProfileName(name))
    {
        error = Text("profileNameError");
        return false;
    }
    return LoadProfileFile(std::string(PROFILE_DIR) + "/" + name + ".ini", draft, error);
}
std::string ProfileDirectory()
{
    char path[MAX_PATH]{};
    const DWORD length = GetFullPathNameA(PROFILE_DIR, MAX_PATH, path, nullptr);
    return length && length < MAX_PATH ? std::string(path) : std::string();
}
namespace
{
bool ProfilePath(const std::string &input, std::string &path)
{
    if (input.size() < 5 || input.find('\0') != std::string::npos ||
        lstrcmpiA(input.c_str() + input.size() - 4, ".ini"))
        return false;
    char full[MAX_PATH]{};
    const DWORD length = GetFullPathNameA(input.c_str(), MAX_PATH, full, nullptr);
    if (!length || length >= MAX_PATH)
        return false;
    path = full;
    return true;
}
} // namespace
bool SaveProfileFile(const std::string &input, const Draft &draft)
{
    Load();
    std::string path;
    return ProfilePath(input, path) && Save(path.c_str(), draft);
}
bool LoadProfileFile(const std::string &input, Draft &draft, std::string &error)
{
    error.clear();
    Load();
    std::string path;
    if (!ProfilePath(input, path))
    {
        error = Text("profilePathError");
        return false;
    }
    auto replacement = Defaults();
    Era::ClearIniCache(path.c_str());
    if (!ReadIni(path.c_str(), replacement, true, error))
        return false;
    std::set<std::string> known;
    for (const auto &object : objects)
    {
        known.insert(Fold(Section(object, Target)));
        known.insert(Fold(Section(object, Source)));
    }
    std::vector<char> sections(65536);
    char absolutePath[MAX_PATH]{};
    const DWORD pathLength = GetFullPathNameA(path.c_str(), MAX_PATH, absolutePath, nullptr);
    if (!pathLength || pathLength >= MAX_PATH)
    {
        error = Text("profilePathTooLong");
        return false;
    }
    DWORD length = 0;
    for (;;)
    {
        length = GetPrivateProfileSectionNamesA(sections.data(), static_cast<DWORD>(sections.size()), absolutePath);
        if (length < sections.size() - 2)
            break;
        if (sections.size() >= 4 * 1024 * 1024)
        {
            error = Text("profileSectionsTooLarge");
            return false;
        }
        sections.resize(sections.size() * 2);
    }
    int missing = 0;
    for (const char *section = sections.data(); *section; section += std::strlen(section) + 1)
        if ((!std::strncmp(section, "target:", 7) || !std::strncmp(section, "source:", 7)) &&
            !known.count(Fold(section)))
            ++missing;
    if (missing)
        error = std::string(Text("missingObjects")) + std::to_string(missing);
    draft.swap(replacement);
    return true;
}
std::vector<std::string> Profiles()
{
    std::vector<std::string> result;
    WIN32_FIND_DATAA file{};
    const HANDLE search = FindFirstFileA((std::string(PROFILE_DIR) + "/*.ini").c_str(), &file);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string name(file.cFileName);
                name.resize(name.size() - 4);
                if (ValidProfileName(name))
                    result.push_back(name);
            }
        } while (FindNextFileA(search, &file));
        FindClose(search);
    }
    std::sort(result.begin(), result.end());
    return result;
}
} // namespace wogification

#define WOG_EXPORT(name, args)                                                                                         \
    extern "C" __declspec(dllexport) int __stdcall name args                                                           \
    try
#pragma comment(linker, "/EXPORT:RMG_GetWogificationApiVersion=_RMG_GetWogificationApiVersion@0")
WOG_EXPORT(RMG_GetWogificationApiVersion, ())
{
    return 1;
}
catch (...)
{
    return 0;
}
#pragma comment(linker, "/EXPORT:RMG_WogifySetting=_RMG_WogifySetting@20")
WOG_EXPORT(RMG_WogifySetting, (int kind, int type, int subtype, int field, int fallback))
{
    using namespace wogification;
    Load();
    if (kind < 0 || kind > 1 || field < 0 || field >= FieldCount)
        return fallback;
    if (Pickup(type))
        return field == Enabled ? 0 : fallback;
    if (field == Enabled && ProtectedObject(type, subtype))
        return 0;
    if (field == Enabled &&
        !(kind == Target ? extendersManager::ObjectExtenderManager::IsAddedObject(type, subtype)
                         : extendersManager::ObjectExtenderManager::IsOriginalObject(type, subtype)))
        return 0;
    const auto *object = Find(type, subtype, static_cast<Kind>(kind));
    if (!object)
        return fallback;
    if (field == Enabled && (!Available(*object) || (kind == Target && object->targetGroups.empty())))
        return 0;
    const int value = object->current[kind][field];
    return value < 0 ? fallback : value;
}
catch (...)
{
    return fallback;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyReadInt=_RMG_WogifyReadInt@8")
WOG_EXPORT(RMG_WogifyReadInt, (const char *key, int fallback))
{
    using namespace wogification;
    Load();
    if (!key)
        return fallback;
    const std::string prefix = Root("objects.");
    if (std::strncmp(key, prefix.c_str(), prefix.size()))
        return fallback;
    char *end = nullptr;
    const char *start = key + prefix.size();
    const long groupValue = std::strtol(start, &end, 10);
    if (end == start || groupValue < 0 || groupValue > 1000000 || std::strncmp(end, ".targets.", 9))
        return fallback;
    start = end + 9;
    const long indexValue = std::strtol(start, &end, 10);
    if (end == start || indexValue < 0 || indexValue > 8192 || *end != '.')
        return fallback;
    const int group = static_cast<int>(groupValue), index = static_cast<int>(indexValue);
    const char *fieldName = end + 1;
    const auto original = originalTargets.find(group);
    if (original == originalTargets.end() || index < original->second)
        return fallback;
    const auto extra = additions.find(group);
    if (extra == additions.end() || index - original->second >= int(extra->second.size()))
        return fallback;
    const auto &object = objects[extra->second[index - original->second]];
    const std::string field(fieldName);
    if (field == "type")
        return object.type;
    if (field == "subtype")
        return object.subtype;
    if (field == "weight")
        return !Participates(object, Target) || !Available(object) ? 0
               : object.defaults[Target][Rate] < 0                 ? 1
                                                                   : object.defaults[Target][Rate];
    return Read(Root("catalog.objects." + object.id + ".placement." + field), fallback);
}
catch (...)
{
    return fallback;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyBegin=_RMG_WogifyBegin@4")
WOG_EXPORT(RMG_WogifyBegin, (int isRandom))
{
    using namespace wogification;
    Load();
    running = false;
    sites.clear();
    originalSources.clear();
    consumed.clear();
    sourceRolls.clear();
    runMap = &P_Game->mainSetup;
    mapSize = runMap->mapSize;
    randomMap = isRandom != 0;
    if (mapSize < 1 || mapSize > 256)
        return 0;
    // Snapshot only live instances; deleted details can remain in the native vector.
    for (unsigned n = 0; n < runMap->objectDetails.Count(); ++n)
    {
        const auto &d = runMap->objectDetails[n];
        if (d.num >= runMap->objectAttributes.Count())
            continue;
        const auto &a = runMap->objectAttributes[d.num];
        int column = -1, row = -1;
        for (int y = 0; y < a.height && y < 6 && row < 0; ++y)
            for (int x = 0; x < a.width && x < 8; ++x)
                if ((a.entrances.m_bitsPacked >> (47 - y * 8 - x)) & 1)
                {
                    column = x;
                    row = y;
                    break;
                }
        const int x = int(d.x) - column, y = int(d.y) - row, z = d.z;
        if (row < 0 || x < 0 || y < 0 || x >= mapSize || y >= mapSize || z > (runMap->hasUnderground ? 1 : 0))
            continue;
        const auto *tile = runMap->GetMapItem(x, y, z);
        if (!tile ||
            (tile->objectType != eObject::HERO && (tile->objectType != a.type || tile->objectSubtype != a.subtype)) ||
            std::none_of(tile->objectDrawing.begin(), tile->objectDrawing.end(),
                         [n](const H3ObjectDraw &draw) { return draw.sprite == n; }))
            continue;
        sites.push_back({x, y, z, a.type, a.subtype});
        if (tile->objectType != eObject::HERO)
            originalSources[Position(x, y, z)] = {a.type, a.subtype};
    }
    running = true;
    return 1;
}
catch (...)
{
    using namespace wogification;
    running = false;
    runMap = nullptr;
    return 0;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyAllowSource=_RMG_WogifyAllowSource@28")
WOG_EXPORT(RMG_WogifyAllowSource, (int x, int y, int z, int type, int subtype, int group, int rollPassed))
{
    using namespace wogification;
    if (!Active(x, y, z))
        return 0;
    if (Pickup(type))
        return 0;
    if (ProtectedObject(type, subtype))
        return 0;
    if (!extendersManager::ObjectExtenderManager::IsOriginalObject(type, subtype))
        return 0;
    const int p = Position(x, y, z);
    const auto original = originalSources.find(p);
    if (consumed.count(p) || original == originalSources.end() || original->second != std::make_pair(type, subtype))
        return 0;
    const auto *object = Find(type, subtype, Source);
    if (!object)
        return rollPassed != 0;
    const auto &v = object->current[Source];
    if (!Available(*object) || !v[Enabled] || !Scope(v, z) || (v[Distance] >= 0 && v[Distance] != group))
        return 0;
    // One chance result per original instance across competing rule groups.
    return sourceRolls.emplace(p, rollPassed != 0).first->second;
}
catch (...)
{
    return 0;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyTargetWeight=_RMG_WogifyTargetWeight@24")
WOG_EXPORT(RMG_WogifyTargetWeight, (int x, int y, int z, int type, int subtype, int fallback))
{
    using namespace wogification;
    if (!Active(x, y, z))
        return 0;
    if (Pickup(type))
        return 0;
    if (ProtectedObject(type, subtype))
        return 0;
    if (!extendersManager::ObjectExtenderManager::IsAddedObject(type, subtype))
        return 0;
    const auto *object = Find(type, subtype, Target);
    if (!object)
        return fallback;
    const auto &v = object->current[Target];
    if (object->targetGroups.empty() || !Available(*object) || !v[Enabled] || !Scope(v, z))
        return 0;
    int count = 0;
    for (const auto &site : sites)
        if (site.type == type && site.subtype == subtype)
        {
            ++count;
            const int dx = site.x - x, dy = site.y - y;
            if (site.z == z && dx * dx + dy * dy < v[Distance] * v[Distance])
                return 0;
        }
    if (v[Limit] >= 0 && count >= v[Limit])
        return 0;
    return v[Rate] < 0 ? fallback : v[Rate];
}
catch (...)
{
    return 0;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyNotify=_RMG_WogifyNotify@20")
WOG_EXPORT(RMG_WogifyNotify, (int x, int y, int z, int type, int subtype))
{
    using namespace wogification;
    if (!Active(x, y, z))
        return 0;
    consumed.insert(Position(x, y, z));
    sites.erase(
        std::remove_if(sites.begin(), sites.end(), [=](const Site &s) { return s.x == x && s.y == y && s.z == z; }),
        sites.end());
    sites.push_back({x, y, z, type, subtype});
    const auto *object = Find(type, subtype, Target);
    if (object && !object->initializer.empty())
    {
        std::array<int, 16> saved;
        int *args = *Era::GetArgXVars();
        std::copy(args, args + 16, saved.begin());
        const auto command = "!!FU(" + object->initializer + "):P" + std::to_string(x) + "/" + std::to_string(y) + "/" +
                             std::to_string(z) + "/" + std::to_string(type) + "/" + std::to_string(subtype) + ";";
        Era::ExecErmCmd(command.c_str());
        std::copy(saved.begin(), saved.end(), args);
    }
    return 1;
}
catch (...)
{
    return -1;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyEnd=_RMG_WogifyEnd@0")
WOG_EXPORT(RMG_WogifyEnd, ())
{
    using namespace wogification;
    running = false;
    sites.clear();
    originalSources.clear();
    consumed.clear();
    sourceRolls.clear();
    runMap = nullptr;
    return 1;
}
catch (...)
{
    return 0;
}

#pragma comment(linker, "/EXPORT:RMG_WogifyPickupBegin=_RMG_WogifyPickupBegin@8")
WOG_EXPORT(RMG_WogifyPickupBegin, (int type, int isRandom))
{
    using namespace wogification;
    pickupType = -1;
    pickupCounts.clear();
    Load();
    if (!Pickup(type))
        return 0;
    int x = -2, y = 0, z = 0;
    while (Era::FindNextObject(type, -1, &x, &y, &z, static_cast<Era::ESearchDirection>(-2)))
        ++pickupCounts[P_Game->mainSetup.GetMapItem(x, y, z)->objectSubtype];
    if (type == eObject::ARTIFACT && processCurrent[BankArtifacts])
        for (const auto &bank : P_Game->creatureBanks)
            for (int artifact : bank.artifacts)
                ++pickupCounts[artifact];
    pickupRandom = isRandom != 0;
    pickupType = type;
    return 1;
}
catch (...)
{
    wogification::pickupType = -1;
    return 0;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyPickupSetting=_RMG_WogifyPickupSetting@24")
WOG_EXPORT(RMG_WogifyPickupSetting, (int kind, int type, int id, int field, int z, int fallback))
{
    using namespace wogification;
    if (type != pickupType || kind < 0 || kind > 1 || field < 0 || field >= FieldCount || z < 0 || z > 1)
        return 0;
    const auto *object = Find(type, id, static_cast<Kind>(kind));
    if (!object || !Participates(*object, static_cast<Kind>(kind)))
        return field == Enabled ? 0 : fallback;
    const auto &v = object->current[kind];
    if (field == Enabled)
        return Available(*object) && v[Enabled] && (v[Levels] & (1 << z)) && (v[MapKinds] & (pickupRandom ? 2 : 1)) &&
               (kind == Source ? pickupCounts[id] > v[Limit] : v[Limit] < 0 || pickupCounts[id] < v[Limit]);
    return v[field] < 0 ? fallback : v[field];
}
catch (...)
{
    return 0;
}

#pragma comment(linker, "/EXPORT:RMG_WogifyProcessSetting=_RMG_WogifyProcessSetting@12")
WOG_EXPORT(RMG_WogifyProcessSetting, (int field, int isRandom, int fallback))
{
    using namespace wogification;
    Load();
    if (field < 0 || field >= ProcessFieldCount)
        return fallback;
    if (!(processCurrent[ProcessMaps] & (isRandom ? 2 : 1)))
        return 0;
    return processCurrent[field];
}
catch (...)
{
    return fallback;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyQuestCreature=_RMG_WogifyQuestCreature@12")
WOG_EXPORT(RMG_WogifyQuestCreature, (int x, int y, int z))
{
    for (const auto *quest : P_Game->mainSetup.quests)
        if (quest && quest->GetQuestType() == eQuest::DEFEAT_MONSTER)
        {
            const auto position = quest->GetCoordinates();
            if (position.x == x && position.y == y && position.z == z)
                return 1;
        }
    return 0;
}
catch (...)
{
    return 1;
}
#pragma comment(linker, "/EXPORT:RMG_WogifyPickupNotify=_RMG_WogifyPickupNotify@12")
WOG_EXPORT(RMG_WogifyPickupNotify, (int type, int source, int target))
{
    using namespace wogification;
    if (pickupType != type || !PickupId(type, target))
        return 0;
    if (pickupCounts[source] > 0)
        --pickupCounts[source];
    ++pickupCounts[target];
    return 1;
}
catch (...)
{
    return 0;
}
