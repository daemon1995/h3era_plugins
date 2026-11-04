#include "framework.h"
#include "../headers/Config/ExportProvider.h"
#include "../headers/Era/JsonOverridesSchema.hpp"
#include "../headers/Era/GameJsonTables.hpp"
#include "../headers/Era/GameDwellingInfo.hpp"
namespace {
using namespace configProvider;
void CollectTowns(Category &category)
{
    const int townCount = H3TownCount::Get();
    auto *commonNames = *reinterpret_cast<LPCSTR **>(0x5B97FC + 2);
    auto *commonDescriptions = *reinterpret_cast<LPCSTR **>(0x5B9833 + 2);
    Record common{category};
    for (int building = 0; building < eBuildings::SPEC17; ++building)
    {
        const auto key = "era.towns.buildings." + std::to_string(building);
        Put(common.text, key + ".name", commonNames[building]);
        Put(common.text, key + ".description", commonDescriptions[building]);
    }

    auto *randomNames = *reinterpret_cast<LPCSTR **>(0x4CA9D0 + 3);
    auto *specialNames = *reinterpret_cast<LPCSTR **>(0x5B98BC + 2);
    auto *specialDescriptions = *reinterpret_cast<LPCSTR **>(0x5B98C7 + 2);
    auto *blacksmithDescriptions = *reinterpret_cast<LPCSTR **>(0x5D2E64 + 3);
    auto *dwellingNames = *reinterpret_cast<LPCSTR **>(0x5B9923 + 2);
    auto *dwellingDescriptions = *reinterpret_cast<LPCSTR **>(0x5B9957 + 2);
    const int dwellingsPerTown = ByteAt(0x5B995F + 2);
    const int dwellingTownCount = dwellingsPerTown ? DwordAt(0x5B9962 + 2) / dwellingsPerTown : 0;
    for (int town = 0; town <= townCount; ++town)
    {
        const bool neutral = town == townCount;
        Record record{category, neutral || Original(town, "towns", 10)};
        const auto key = "era.towns." + std::to_string(neutral ? -1 : town);
        Put(record.text, key + ".name", H3TownNames::Get()[neutral ? -1 : town]);
        if (!neutral)
            for (int index = 0; index < 16; ++index)
                Put(record.text, key + ".names." + std::to_string(index), randomNames[town * 16 + index]);
        for (int special = 0; special < 11; ++special)
        {
            const int building = special == 10 ? eBuildings::RESOURCE_SILO : eBuildings::SPEC17 + special;
            const auto buildingKey = key + ".buildings." + std::to_string(building);
            Put(record.text, buildingKey + ".name", specialNames[town * 11 + special]);
            Put(record.text, buildingKey + ".description", specialDescriptions[town * 11 + special]);
        }
        Put(record.text, key + ".buildings." + std::to_string(eBuildings::BLACKSMITH) + ".description", blacksmithDescriptions[town]);
        if (town < dwellingTownCount)
            for (int dwelling = 0; dwelling < dwellingsPerTown; ++dwelling)
            {
                const auto buildingKey = key + ".buildings." + std::to_string(30 + dwelling);
                Put(record.text, buildingKey + ".name", dwellingNames[town * dwellingsPerTown + dwelling]);
                Put(record.text, buildingKey + ".description", dwellingDescriptions[town * dwellingsPerTown + dwelling]);
            }

    }
}
void CollectNativeObjects(std::vector<Category> &categories)
{
    auto *names = H3ObjectName::Get();
    for (int type = 0; type < limits::OBJECTS; ++type)
    {
        Record record{categories[2], Original(type, "mapObjects", limits::OBJECTS - 1)};
        record.identity = "type:" + std::to_string(type);
        Put(record.text, "era.objects." + std::to_string(type), names[type]);

    }
    if (!era_services::Find<era_services::Ownership>(era_services::DWELLING_OWNER))
        for (int type : {dwellings::SINGLE_TYPE, dwellings::MULTIPLE_TYPE})
        {
            const int count = type == dwellings::SINGLE_TYPE ? EraJS::GameTables::Dwelling1Capacity() : EraJS::GameTables::Dwelling4Capacity();
            for (int id = 0; id < count; ++id)
            {
                EraJS::DwellingExportInfo info;
                if (!EraJS::GameDwellings::ReadNative(type, id, info)) continue;
                Record record{categories[3], Original(id, type == dwellings::SINGLE_TYPE ? "dwellings1" : "dwellings4", 100)};
                record.identity = ObjectIdentity(type, id);
                const auto key = ObjectKey(type, id);
                if (info.name) Put(record.text, key + ".name", info.name);
                PutArray(record.data, key + ".creatures", info.creatures, info.slotCount);
                ObjectText(record, key);

            }
        }
    if (!era_services::Find<era_services::Ownership>(era_services::BANK_OWNER))
        for (int id = 0; id < EraJS::GameTables::CreatureBankCapacity(); ++id)
        {
            int type = eObject::CREATURE_BANK;
            if (id == eCrBank::DERELICT_SHIP) type = eObject::DERELICT_SHIP;
            if (id == eCrBank::DRAGON_UTOPIA) type = eObject::DRAGON_UTOPIA;
            if (id == eCrBank::CRYPT) type = eObject::CRYPT;
            if (id == eCrBank::SHIPWRECK) type = eObject::SHIPWRECK;
            const int subtype = type == eObject::CREATURE_BANK ? id : 0;
            Record record{categories[4], Original(id, "creatureBanks", 20)};
            record.identity = ObjectIdentity(type, subtype);
            Put(record.text, ObjectKey(type, subtype) + ".name", P_CreatureBankSetup[id].name.String());

        }
}
void Build(const era_config::Sink *sink)
{
    using namespace configProvider;
    constexpr bool overrides = true;
    const bool artifactOwner = era_services::Find<era_services::Ownership>(era_services::ARTIFACT_OWNER) != nullptr;
    std::vector<Category> categories = {
        {sink, "creatures", "Creatures"}, {sink, "artifacts", "Artifact"},
        {sink, "objects", "Object"}, {sink, "dwellings", "Dwelling"},
        {sink, "creatureBanks", "CreatureBank"}, {sink, "towns", "Town"},
        {sink, "heroes", "Hero"}, {sink, "spells", "Spell"},
        {sink, "secondarySkills", "SecondarySkill"}};
    const int creatureCount = (std::max)(0, IntAt(0x4A1657));
    for (int id = 0; id < creatureCount; ++id)
    {
        const auto &info = P_CreatureInformation[id];
        if ((!info.nameSingular || !*info.nameSingular) && (!info.namePlural || !*info.namePlural)) continue;
        Record record{categories[0], Original(id, "creatures", 196)};
        record.identity = std::to_string(id);
        const auto key = "era.monsters." + std::to_string(id);
        EraJS::VisitCreatureText(info, [&](LPCSTR field, LPCSTR value) {
            Put(record.text, key + (std::strcmp(field, "description") ? ".name." : ".") + field, value);
        });

    }
    const int artifactCount = (std::max)(0, IntAt(0x717020));
    const auto *events = EraJS::GameTables::ArtifactEvents();
    for (int id = 0; !artifactOwner && id < artifactCount; ++id)
    {
        Record record{categories[1], Original(id, "artifacts", 170)};
        record.identity = std::to_string(id);
        const auto key = "era.artifacts." + std::to_string(id);
        EraJS::VisitArtifact(P_ArtifactSetup[id], [&](LPCSTR field, const auto &value) {
            if constexpr (std::is_convertible_v<decltype(value), LPCSTR>) Put(record.text, key + "." + field, value);
            else if (overrides) Put(record.data, key + "." + field, value);
        });
        if (events && id < EraJS::GameTables::ArtifactEventCapacity()) Put(record.text, key + ".event", events[id]);

    }
    CollectNativeObjects(categories);
    CollectTowns(categories[5]);
    for (int id = 0; id < static_cast<int>(H3HeroCount::Get()); ++id)
    {
        Record record{categories[6], Original(id, "heroes", 155)};
        record.identity = std::to_string(id);
        const auto key = "era.heroes." + std::to_string(id);
        Put(record.text, key + ".name", P_HeroInfo[id].name);
        Put(record.text, key + ".specialty.short", P_HeroSpecialty[id].spShort);
        Put(record.text, key + ".specialty.full", P_HeroSpecialty[id].spFull);
        Put(record.text, key + ".specialty.description", P_HeroSpecialty[id].spDescr);
        Put(record.text, key + ".biography", EraJS::GameTables::HeroBiographies()[id]);
        if (overrides) EraJS::VisitHeroData(P_HeroInfo[id], [&](LPCSTR field, const auto &value) { Put(record.data, key + "." + field, value); });

    }
    for (int id = 0; id < limits::TOTAL_SPELLS; ++id)
    {
        Record record{categories[7], Original(id, "spells", limits::TOTAL_SPELLS - 1)};
        record.identity = std::to_string(id);
        const auto key = "era.spells." + std::to_string(id);
        EraJS::VisitSpell(P_Spell[id], [&](LPCSTR field, const auto &value) {
            const bool text = !std::strcmp(field, "name") || !std::strcmp(field, "shortName") || !std::strcmp(field, "description");
            if (text) Put(record.text, key + "." + field, value);
            else if (overrides) Put(record.data, key + "." + field, value);
        });

    }
    for (int id = 0; id < limits::SECONDARY_SKILLS; ++id)
    {
        Record record{categories[8], Original(id, "secondarySkills", limits::SECONDARY_SKILLS - 1)};
        record.identity = std::to_string(id);
        const auto key = "era.secondarySkills." + std::to_string(id);
        Put(record.text, key + ".name", P_SecondarySkillInfo[id].name);
        Put(record.text, key + ".description", P_SecondarySkillInfo[id].description);

    }
}
int __stdcall Collect(void *, const era_config::Sink *sink)
{
    try { Build(sink); return era_config::Ok; } catch (...) { return era_config::ProviderFailed; }
}
}
void RegisterNativeConfigProvider() { configProvider::Registrar<Collect>::Install("native.tables"); }
