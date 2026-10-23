#include "CreatureBanksExtender.h"
#include "../headers/Config/ExportProvider.h"
namespace {
using namespace configProvider;
int __stdcall ReadBank(int id, EraJS::BankExportInfo *info) { return cbanks::CreatureBanksExtender::GetExportInfo(id, info); }
void BankData(Record &record, const std::string &key, int bank, EraJS::GetBankExportInfo getBank)
{
    EraJS::BankExportInfo info;
    if (getBank && getBank(bank, &info))
    {
        Put(record.text, key + ".name", info.name);
        Put(record.data, key + ".troopPlacement.isBank", info.isBank);
        for (int slot = 0; slot < 7; ++slot)
        {
            Put(record.data, key + ".troopPlacement.attackers." + std::to_string(slot), info.positions[slot]);
            Put(record.data, key + ".troopPlacement.defenders." + std::to_string(slot), info.positions[slot + 7]);
        }
        for (int state = 0; state < 4; ++state)
        {
            const auto &setup = info.states[state];
            const auto stateKey = key + ".states." + std::to_string(state);
            Put(record.data, stateKey + ".creatureRewardType", setup.creatureRewardType);
            Put(record.data, stateKey + ".creatureRewardCount", setup.creatureRewardCount);
            Put(record.data, stateKey + ".chance", setup.chance);
            Put(record.data, stateKey + ".upgrade", setup.upgrade);
            Put(record.data, stateKey + ".artifactTypeCounts", setup.artifactTypeCounts);
            Put(record.data, stateKey + ".guardians.type", setup.guardians.type);
            Put(record.data, stateKey + ".guardians.count", setup.guardians.count);
            Put(record.data, stateKey + ".resources", setup.resources.asArray);
        }
    }


    // Additional reward rules are startup JSON, not the randomized reward of a placed bank.
    // Preserve their original rule (including random spell filters) when it exists.
    if (!getBank) return;
    for (int state = 0; state < 4; ++state)
    {
        const auto stateKey = ObjectKey(eObject::CREATURE_BANK, bank) + ".states." + std::to_string(state);
        CopyText(record.data, stateKey + ".customDef");
        for (const char *field : {"experience", "spellPoints", "luck", "morale", "revealRadius"})
            CopyNumber(record.data, stateKey + "." + field);
        CopyNumber(record.data, stateKey + ".resources.7");
        for (int slot = 0; slot < 4; ++slot)
        {
            const auto id = std::to_string(slot);
            CopyNumber(record.data, stateKey + ".artifactIds." + id);
            CopyNumber(record.data, stateKey + ".skills.primary." + id);
            CopyNumber(record.data, stateKey + ".spells." + id + ".id");
            for (const char *field : {"schools", "levels", "flags"})
                CopyNumber(record.data, stateKey + ".spells." + id + ".bits." + field);
        }
    }
}
}
int __stdcall cbanks::CreatureBanksExtender::CollectConfig(void *, const era_config::Sink *sink)
{
    try
    {
        if (!instance) return era_config::Ok;
        Category category{sink, "creatureBanks", "CreatureBank"};
        for (unsigned bank = 0; bank < instance->Size(); ++bank)
        {
            const int type = instance->manager.GetCreatureBankObjectType(bank);
            const int subtype = type == eObject::CREATURE_BANK ? int(bank) : 0;
            Record record{category, Original(bank, "creatureBanks", 20)};
            record.identity = ObjectIdentity(type, subtype);
            const auto key = ObjectKey(type, subtype);
            BankData(record, key, bank, ReadBank);
            ObjectText(record, key);

        }
        return era_config::Ok;
    }
    catch (...) { return era_config::ProviderFailed; }
}
void RegisterBankConfigProvider()
{ configProvider::Registrar<cbanks::CreatureBanksExtender::CollectConfig>::Install("banks"); }
