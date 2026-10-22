#include "pch.h"
#include "../headers/Config/ExportProvider.h"
#include "../headers/Era/GameJsonTables.hpp"
#include "../headers/Era/JsonObjectExport.hpp"
#include "../headers/Objects/ObjectLimits.h"
#include <map>
#include <set>
extern "C" LPCSTR __stdcall GetObjectExportProperty(int);
extern "C" int __stdcall GetOriginalObjectSubtypeLimit(int);
extern "C" BOOL __stdcall GetRmgExportInfo(int, int, EraJS::RmgExportInfo *);
extern "C" BOOL __stdcall GetRmgExportOverrides(int, int, EraJS::RmgExportOverridesInfo *);
namespace {
using namespace configProvider;
void RmgScope(const Fields &json, const std::string &key, int type, int subtype, EraJS::GetRmgExportOverrides getOverrides)
{
    constexpr LPCSTR fields[] = {"enabled", "map", "zone", "value", "density"};
    EraJS::RmgExportOverridesInfo info;
    if (getOverrides && getOverrides(type, subtype, &info))
    {
        PutRmgOverrides(json, key, info.fieldMask, info.fields);
    }
    else
    {
        // Older hosts can still export explicit configuration; never synthesize
        // fixed values from their already resolved runtime snapshot.
        for (LPCSTR field : fields) CopyNumber(json, key + "." + field);
    }
}
void ObjectSettings(Record &record, const std::string &key, int type, int subtype,
                    EraJS::GetRmgExportOverrides getOverrides, EraJS::GetRmgExportInfo getRmg)
{
    // Retain differing ancestor settings once in the merged output, so exporting
    // one category still preserves the aggregate limits and type inheritance.
    RmgScope(record.data, "RMG.objectGeneration", -1, -1, getOverrides);
    const auto typeKey = "RMG.objectGeneration." + std::to_string(type);
    RmgScope(record.data, typeKey, type, -1, getOverrides);
    RmgScope(record.data, key, type, subtype, getOverrides);
    CopyText(record.data, typeKey + ".sound.loop");
    CopyText(record.data, key + ".sound.loop");
    EraJS::RmgExportInfo info;
    if (getRmg && getRmg(type, subtype, &info))
    {
        int values[] = {info.enabled, info.map, info.zone, info.value, info.density};
        if (type == eObject::CREATURE_GENERATOR1 || type == eObject::CREATURE_GENERATOR4)
        {
            // Full export must preserve automatic valuation too, rather than
            // freezing the creature AI value calculated during initialization.
            int configuredValue = -1;
            bool found = false;
            const int typeValue = EraJS::readInt(typeKey + ".value", found);
            if (found) configuredValue = typeValue;
            const int subtypeValue = EraJS::readInt(key + ".value", found);
            if (found) configuredValue = subtypeValue;
            values[rmgExport::VALUE] = (std::max)(-1, configuredValue);
        }
        PutRmgOverrides(record.rmgDefaults, key, (1U << rmgExport::FIELD_COUNT) - 1, values);
    }
}
int BankId(int type, int subtype)
{
    switch (type)
    {
    case eObject::CREATURE_BANK: return subtype;
    case eObject::DERELICT_SHIP: return eCrBank::DERELICT_SHIP;
    case eObject::DRAGON_UTOPIA: return eCrBank::DRAGON_UTOPIA;
    case eObject::CRYPT: return eCrBank::CRYPT;
    case eObject::SHIPWRECK: return eCrBank::SHIPWRECK;
    default: return -1;
    }
}
void CollectObjects(std::vector<Category> &categories)
{
    auto &objects = categories[0], &dwellings = categories[1], &banks = categories[2];
    const auto getProperty = GetObjectExportProperty;
    const auto getOriginalLimit = GetOriginalObjectSubtypeLimit;
    const auto getRmg = GetRmgExportInfo;
    const auto getOverrides = GetRmgExportOverrides;

    using Identity = std::pair<int, int>;
    std::map<Identity, std::vector<std::string>> definitions;
    std::set<Identity> identities;
    if (getProperty)
        for (int index = 0; ; ++index)
        {
            const char *line = getProperty(index);
            if (!line) break;
            int type = -1, subtype = -1;
            if (std::sscanf(line, "%*s %*s %*s %*s %*s %d %d", &type, &subtype) != 2 ||
                !object_config::IsObjectType(type) || !object_config::IsObjectSubtype(subtype)) continue;
            const Identity identity{type, subtype};
            identities.insert(identity);
            auto &lines = definitions[identity];
            const std::string text = line;
            if (std::find(lines.begin(), lines.end(), text) == lines.end()) lines.push_back(text);
        }
    if (const auto *game = H3Main::Get())
        for (int type = 0; type < limits::OBJECTS; ++type)
            for (const auto &object : game->mainSetup.objectLists[type])
                if (object_config::IsObjectSubtype(object.subtype)) identities.insert({type, object.subtype});

    // Include native records even when no graphics prototype exists for them.
    for (const auto type : {eObject::CREATURE_GENERATOR1, eObject::CREATURE_GENERATOR4})
    {
        const int count = type == eObject::CREATURE_GENERATOR1 ? EraJS::GameTables::Dwelling1Capacity() : EraJS::GameTables::Dwelling4Capacity();
        for (int id = 0; id < count; ++id) identities.insert({type, id});
    }
    for (int id = 0; id < EraJS::GameTables::CreatureBankCapacity(); ++id)
    {
        int type = eObject::CREATURE_BANK;
        for (int special : {eObject::DERELICT_SHIP, eObject::DRAGON_UTOPIA, eObject::CRYPT, eObject::SHIPWRECK})
            if (BankId(special, 0) == id) type = special;
        identities.insert({type, type == eObject::CREATURE_BANK ? id : 0});
    }

    for (const auto &identity : identities)
    {
        const int type = identity.first, subtype = identity.second, bank = BankId(type, subtype);
        const bool isDwelling = type == eObject::CREATURE_GENERATOR1 || type == eObject::CREATURE_GENERATOR4;
        auto &category = isDwelling ? dwellings : bank >= 0 ? banks : objects;
        int originalLimit = getOriginalLimit ? getOriginalLimit(type) : 0;
        bool original = subtype <= originalLimit;
        if (isDwelling) original = Original(subtype, type == eObject::CREATURE_GENERATOR1 ? "dwellings1" : "dwellings4", 100);
        else if (bank >= 0) original = Original(bank, "creatureBanks", 20);
        else
        {
            const auto config = "objectSubtypes." + std::to_string(type);
            original = subtype <= OriginalLimit(config, originalLimit);
        }
        Record record{category, original};
        record.identity = ObjectIdentity(type, subtype);
        const auto key = ObjectKey(type, subtype);
        ObjectText(record, key);
        {
            const auto definition = definitions.find(identity);
            if (definition != definitions.end()) PutArray(record.data, key + ".properties", definition->second.data(), unsigned(definition->second.size()));
            ObjectSettings(record, key, type, subtype, getOverrides, getRmg);
        }
        if (type == eObject::UNIVERSITY)
            for (int skill = 0; skill < limits::SECONDARY_SKILLS; ++skill)
                CopyNumber(record.data, key + ".skills.allowed." + std::to_string(skill));

    }
}

int __stdcall Collect(void *, const era_config::Sink *sink)
{
    try
    {
        std::vector<Category> categories = {
            {sink, "objects", "Object"}, {sink, "dwellings", "Dwelling"}, {sink, "creatureBanks", "CreatureBank"}};
        CollectObjects(categories);
        return era_config::Ok;
    }
    catch (...) { return era_config::ProviderFailed; }
}
}
void RegisterRmgConfigProvider() { configProvider::Registrar<Collect>::Install("objects.rmg"); }
